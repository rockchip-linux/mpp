/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_dec_multi_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "utils.h"
#include "mpi_dec_utils.h"
#include <pthread.h>

/* For each instance thread setup */
typedef struct {
    MppCtx          ctx;
    MppApi          *mpi;

    /* end of stream flag when set quit the loop */
    RK_U32          eos;

    /* buffer for stream data reading */
    char            *buf;

    /* input and output */
    MppBufferGroup  frm_grp;
    MppBufferGroup  pkt_grp;
    MppPacket       packet;
    size_t          packet_size;
    MppFrame        frame;

    FILE            *fp_input;
    FILE            *fp_output;
    RK_U32          frame_count;

    RK_S64          first_pkt;
    RK_S64          first_frm;
    RK_S32          frame_num;
    FileReader      reader;
} MpiDecCtx;

/* For each instance thread return value */
typedef struct {
    volatile float  frame_rate;
    RK_S64          elapsed_time;
    RK_S64          frame_count;
} MpiDecCtxRet;

typedef struct {
    MpiDecTestCmd   *cmd;           // pointer to global command line info

    pthread_t       thd;            // thread for for each instance
    MpiDecCtx       ctx;            // context of decoder
    MpiDecCtxRet    ret;            // return of decoder
} MpiDecCtxInfo;

static int multi_dec_simple(MpiDecCtx *data)
{
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos  = 0;
    RK_U32 err_info = 0;
    MPP_RET ret = MPP_OK;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    char   *buf = data->buf;
    MppPacket packet = data->packet;
    MppFrame  frame  = NULL;
    FileReader reader = data->reader;
    size_t read_size = 0;

    data->eos = pkt_eos = reader_read(reader, &buf, &read_size);
    if (pkt_eos) {
        if (data->frame_num < 0) {
            mpp_log("%p loop again\n", ctx);
            reader_rewind(reader);
            data->eos = pkt_eos = 0;
        } else {
            mpp_log("%p found last packet\n", ctx);
        }
    }

    mpp_packet_set_data(packet, buf);
    if (read_size > mpp_packet_get_size(packet))
        mpp_packet_set_size(packet, read_size);
    mpp_packet_set_pos(packet, buf);
    mpp_packet_set_length(packet, read_size);
    // setup eos flag
    if (pkt_eos)
        mpp_packet_set_eos(packet);

    do {
        RK_S32 times = 5;
        // send the packet first if packet is not done
        if (!pkt_done) {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret) {
                pkt_done = 1;
                if (!data->first_pkt)
                    data->first_pkt = mpp_time();
            }
        }

        // then get all available frame and release
        do {
            RK_S32 get_frm = 0;
            RK_U32 frm_eos = 0;

        try_again:
            ret = mpi->decode_get_frame(ctx, &frame);
            if (MPP_ERR_TIMEOUT == ret) {
                if (times > 0) {
                    times--;
                    msleep(2);
                    goto try_again;
                }
                mpp_err("decode_get_frame failed too much time\n");
            }
            if (MPP_OK != ret) {
                mpp_err("decode_get_frame failed ret %d\n", ret);
                break;
            }

            if (frame) {
                if (mpp_frame_get_info_change(frame)) {
                    RK_U32 width = mpp_frame_get_width(frame);
                    RK_U32 height = mpp_frame_get_height(frame);
                    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
                    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
                    RK_U32 buf_size = mpp_frame_get_buf_size(frame);

                    mpp_log("decode_get_frame get info changed found\n");
                    mpp_log("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                            width, height, hor_stride, ver_stride, buf_size);

                    if (NULL == data->frm_grp) {
                        /* If buffer group is not set create one and limit it */
                        ret = mpp_buffer_group_get_internal(&data->frm_grp, MPP_BUFFER_TYPE_ION);
                        if (ret) {
                            mpp_err("get mpp buffer group failed ret %d\n", ret);
                            break;
                        }

                        /* Set buffer to mpp decoder */
                        ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, data->frm_grp);
                        if (ret) {
                            mpp_err("set buffer group failed ret %d\n", ret);
                            break;
                        }
                    } else {
                        /* If old buffer group exist clear it */
                        ret = mpp_buffer_group_clear(data->frm_grp);
                        if (ret) {
                            mpp_err("clear buffer group failed ret %d\n", ret);
                            break;
                        }
                    }

                    /* Use limit config to limit buffer count to 24 with buf_size */
                    ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
                    if (ret) {
                        mpp_err("limit buffer group failed ret %d\n", ret);
                        break;
                    }

                    /*
                     * All buffer group config done. Set info change ready to let
                     * decoder continue decoding
                     */
                    ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                    if (ret) {
                        mpp_err("info change ready failed ret %d\n", ret);
                        break;
                    }
                } else {
                    if (!data->first_frm)
                        data->first_frm = mpp_time();

                    err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
                    if (err_info) {
                        mpp_log("decoder_get_frame get err info:%d discard:%d.\n",
                                mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
                    }
                    mpp_log("decode_get_frame get frame %d\n", data->frame_count);
                    data->frame_count++;
                    if (data->fp_output && !err_info)
                        dump_mpp_frame_to_file(frame, data->fp_output);
                }
                frm_eos = mpp_frame_get_eos(frame);
                mpp_frame_deinit(&frame);
                frame = NULL;
                get_frm = 1;
            }

            // if last packet is send but last frame is not found continue
            if (pkt_eos && pkt_done && !frm_eos) {
                msleep(1);
                continue;
            }

            if (frm_eos) {
                // mpp_log("found last frame\n");
                break;
            }
            if (data->frame_num > 0 && data->frame_count >= (RK_U32)data->frame_num) {
                data->eos = 1;
                break;
            }

            if (get_frm)
                continue;
            break;
        } while (1);

        if (pkt_done)
            break;

        /*
         * why sleep here:
         * mpi->decode_put_packet will failed when packet in internal queue is
         * full,waiting the package is consumed .Usually hardware decode one
         * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
         * * is enough.
         */
        msleep(1);
    } while (1);

    return ret;
}

static int multi_dec_advanced(MpiDecCtx *data)
{
    RK_U32 pkt_eos  = 0;
    MPP_RET ret = MPP_OK;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    char   *buf = data->buf;
    MppPacket packet = data->packet;
    MppFrame  frame  = data->frame;
    MppTask task = NULL;
    size_t read_size = fread(buf, 1, data->packet_size, data->fp_input);

    if (read_size != data->packet_size || feof(data->fp_input)) {
        // mpp_log("found last packet\n");

        // setup eos flag
        data->eos = pkt_eos = 1;
    }

    // reset pos
    mpp_packet_set_pos(packet, buf);
    mpp_packet_set_length(packet, read_size);
    // setup eos flag
    if (pkt_eos)
        mpp_packet_set_eos(packet);

    ret = mpi->poll(ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("mpp input poll failed\n");
        return ret;
    }

    ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);  /* input queue */
    if (ret) {
        mpp_err("mpp task input dequeue failed\n");
        return ret;
    }

    mpp_assert(task);

    mpp_task_meta_set_packet(task, KEY_INPUT_PACKET, packet);
    mpp_task_meta_set_frame (task, KEY_OUTPUT_FRAME,  frame);

    ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);  /* input queue */
    if (ret) {
        mpp_err("mpp task input enqueue failed\n");
        return ret;
    }

    /* poll and wait here */
    ret = mpi->poll(ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("mpp output poll failed\n");
        return ret;
    }

    ret = mpi->dequeue(ctx, MPP_PORT_OUTPUT, &task); /* output queue */
    if (ret) {
        mpp_err("mpp task output dequeue failed\n");
        return ret;
    }

    mpp_assert(task);

    if (task) {
        MppFrame frame_out = NULL;
        mpp_task_meta_get_frame(task, KEY_OUTPUT_FRAME, &frame_out);
        //mpp_assert(packet_out == packet);

        if (frame) {
            /* write frame to file here */
            MppBuffer buf_out = mpp_frame_get_buffer(frame_out);

            if (buf_out) {
                void *ptr = mpp_buffer_get_ptr(buf_out);
                size_t len  = mpp_buffer_get_size(buf_out);

                if (data->fp_output)
                    fwrite(ptr, 1, len, data->fp_output);

                mpp_log("decoded frame %d size %d\n", data->frame_count, len);
            }

            if (mpp_frame_get_eos(frame_out)) {
                ; //mpp_log("found eos frame\n");
            }
        }

        /* output queue */
        ret = mpi->enqueue(ctx, MPP_PORT_OUTPUT, task);
        if (ret)
            mpp_err("mpp task output enqueue failed\n");
    }

    return ret;
}

void* multi_dec_decode(void *cmd_ctx)
{
    MpiDecCtxInfo *info = (MpiDecCtxInfo *)cmd_ctx;
    MpiDecTestCmd *cmd  = info->cmd;
    MpiDecCtx *dec_ctx  = &info->ctx;
    MpiDecCtxRet *rets  = &info->ret;
    MPP_RET ret         = MPP_OK;
    size_t file_size    = 0;

    // base flow context
    MppCtx ctx          = NULL;
    MppApi *mpi         = NULL;

    // input / output
    MppPacket packet    = NULL;
    MppFrame  frame     = NULL;

    MpiCmd mpi_cmd      = MPP_CMD_BASE;
    MppParam param      = NULL;
    RK_U32 need_split   = 1;
    MppPollType timeout = cmd->timeout;

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    MppCodingType type  = cmd->type;

    // resources
    char *buf           = NULL;
    size_t packet_size  = MPI_DEC_STREAM_SIZE;
    MppBuffer pkt_buf   = NULL;
    MppBuffer frm_buf   = NULL;
    FileReader reader   = NULL;

    // mpp_log("mpi_dec_test start\n");

    if (cmd->have_input) {
        dec_ctx->fp_input = fopen(cmd->file_input, "rb");
        if (NULL == dec_ctx->fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            goto MPP_TEST_OUT;
        }

        fseek(dec_ctx->fp_input, 0L, SEEK_END);
        file_size = ftell(dec_ctx->fp_input);
        rewind(dec_ctx->fp_input);
        // mpp_log("input file size %ld\n", file_size);
    }

    if (cmd->have_output) {
        dec_ctx->fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == dec_ctx->fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
    }

    if (cmd->simple) {
        reader_init(&reader, cmd->file_input, dec_ctx->fp_input);

        ret = mpp_packet_init(&packet, NULL, 0);
        if (ret) {
            mpp_err("mpp_packet_init failed\n");
            goto MPP_TEST_OUT;
        }
    } else {
        RK_U32 hor_stride = MPP_ALIGN(width, 16);
        RK_U32 ver_stride = MPP_ALIGN(height, 16);

        ret = mpp_buffer_group_get_internal(&dec_ctx->frm_grp, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("failed to get buffer group for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_buffer_group_get_internal(&dec_ctx->pkt_grp, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("failed to get buffer group for output packet ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_frame_init(&frame); /* output frame */
        if (MPP_OK != ret) {
            mpp_err("mpp_frame_init failed\n");
            goto MPP_TEST_OUT;
        }

        /*
         * NOTE: For jpeg could have YUV420 and YUV422 the buffer should be
         * larger for output. And the buffer dimension should align to 16.
         * YUV420 buffer is 3/2 times of w*h.
         * YUV422 buffer is 2 times of w*h.
         * So create larger buffer with 2 times w*h.
         */
        ret = mpp_buffer_get(dec_ctx->frm_grp, &frm_buf, hor_stride * ver_stride * 2);
        if (ret) {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        // NOTE: for mjpeg decoding send the whole file
        if (type == MPP_VIDEO_CodingMJPEG) {
            packet_size = file_size;
        }

        ret = mpp_buffer_get(dec_ctx->pkt_grp, &pkt_buf, packet_size);
        if (ret) {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }
        mpp_packet_init_with_buffer(&packet, pkt_buf);
        buf = mpp_buffer_get_ptr(pkt_buf);

        mpp_frame_set_buffer(frame, frm_buf);
    }

    mpp_log("mpi_dec_test decoder test start w %d h %d type %d\n", width, height, type);

    // decoder demo
    ret = mpp_create(&ctx, &mpi);

    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    // NOTE: decoder split mode need to be set before init
    mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    param = &need_split;
    ret = mpi->control(ctx, mpi_cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        goto MPP_TEST_OUT;
    }

    // NOTE: timeout value please refer to MppPollType definition
    //  0   - non-block call (default)
    // -1   - block call
    // +val - timeout value in ms
    if (timeout) {
        param = &timeout;
        ret = mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, param);
        if (MPP_OK != ret) {
            mpp_err("Failed to set output timeout %d ret %d\n", timeout, ret);
            goto MPP_TEST_OUT;
        }
    }

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_OUT;
    }

    dec_ctx->ctx            = ctx;
    dec_ctx->mpi            = mpi;
    dec_ctx->eos            = 0;
    dec_ctx->buf            = buf;
    dec_ctx->packet         = packet;
    dec_ctx->packet_size    = packet_size;
    dec_ctx->frame          = frame;
    dec_ctx->frame_count    = 0;
    dec_ctx->frame_num      = cmd->frame_num;
    dec_ctx->reader         = reader;

    RK_S64 t_s, t_e;

    t_s = mpp_time();
    if (cmd->simple) {
        while (!dec_ctx->eos) {
            multi_dec_simple(dec_ctx);
        }
    } else {
        while (!dec_ctx->eos) {
            multi_dec_advanced(dec_ctx);
        }
    }
    t_e = mpp_time();
    rets->elapsed_time = t_e - t_s;
    rets->frame_count = dec_ctx->frame_count;
    rets->frame_rate = (float)dec_ctx->frame_count * 1000000 / rets->elapsed_time;
    mpp_log("decode %d frame use time %lld ms frm rate %.2f\n",
            dec_ctx->frame_count, rets->elapsed_time / 1000, rets->frame_rate);

    ret = mpi->reset(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    if (packet) {
        mpp_packet_deinit(&packet);
        packet = NULL;
    }

    if (frame) {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (cmd->simple) {
        reader_deinit(&reader);
    } else {
        if (pkt_buf) {
            mpp_buffer_put(pkt_buf);
            pkt_buf = NULL;
        }

        if (frm_buf) {
            mpp_buffer_put(frm_buf);
            frm_buf = NULL;
        }
    }

    if (dec_ctx->pkt_grp) {
        mpp_buffer_group_put(dec_ctx->pkt_grp);
        dec_ctx->pkt_grp = NULL;
    }

    if (dec_ctx->frm_grp) {
        mpp_buffer_group_put(dec_ctx->frm_grp);
        dec_ctx->frm_grp = NULL;
    }

    if (dec_ctx->fp_output) {
        fclose(dec_ctx->fp_output);
        dec_ctx->fp_output = NULL;
    }

    if (dec_ctx->fp_input) {
        fclose(dec_ctx->fp_input);
        dec_ctx->fp_input = NULL;
    }

    return NULL;
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiDecTestCmd  cmd_ctx;
    MpiDecTestCmd* cmd = &cmd_ctx;
    MpiDecCtxInfo *ctxs = NULL;
    RK_S32 i = 0;
    float total_rate = 0.0;

    memset((void*)cmd, 0, sizeof(*cmd));
    cmd->nthreads = 1;

    // parse the cmd option
    ret = mpi_dec_test_parse_options(argc, argv, cmd);
    if (ret) {
        if (ret < 0) {
            mpp_err("mpi_dec_multi_test_parse_options: input parameter invalid\n");
        }

        mpi_dec_test_help();
        return ret;
    }

    mpi_dec_test_show_options(cmd);

    cmd->simple = (cmd->type != MPP_VIDEO_CodingMJPEG) ? (1) : (0);

    ctxs = mpp_calloc(MpiDecCtxInfo, cmd->nthreads);
    if (NULL == ctxs) {
        mpp_err("failed to alloc context for instances\n");
        return -1;
    }

    for (i = 0; i < cmd->nthreads; i++) {
        ctxs[i].cmd = cmd;

        ret = pthread_create(&ctxs[i].thd, NULL, multi_dec_decode, &ctxs[i]);
        if (ret) {
            mpp_log("failed to create thread %d\n", i);
            return ret;
        }
    }

    for (i = 0; i < cmd->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

    for (i = 0; i < cmd->nthreads; i++) {
        total_rate += ctxs[i].ret.frame_rate;
        mpp_log("payload %d frame rate: %.2f first delay %d ms\n", i,
                ctxs[i].ret.frame_rate,
                (ctxs[i].ctx.first_frm - ctxs[i].ctx.first_pkt) / 1000);
    }
    mpp_free(ctxs);
    ctxs = NULL;

    total_rate /= cmd->nthreads;
    mpp_log("average frame rate %d\n", (int)total_rate);

    return (int)total_rate;
}
