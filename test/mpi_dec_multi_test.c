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
#include <pthread.h>

#include "rk_mpi.h"

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpi_dec_utils.h"

/* For each instance thread setup */
typedef struct {
    MpiDecTestCmd   *cmd;
    MppCtx          ctx;
    MppApi          *mpi;

    /* end of stream flag when set quit the loop */
    RK_U32          loop_end;

    /* input and output */
    DecBufMgr       buf_mgr;
    MppBufferGroup  frm_grp;
    MppPacket       packet;
    MppFrame        frame;

    FILE            *fp_output;
    RK_S32          packet_count;
    RK_S32          frame_count;
    RK_S32          frame_num;

    RK_S64          first_pkt;
    RK_S64          first_frm;

    /* runtime flag */
    RK_U32          quiet;
} MpiDecMultiCtx;

/* For each instance thread return value */
typedef struct {
    float           frame_rate;
    RK_S64          elapsed_time;
    RK_S32          frame_count;
    RK_S64          delay;
} MpiDecMultiCtxRet;

typedef struct {
    MpiDecTestCmd       *cmd;       // pointer to global command line info

    pthread_t           thd;        // thread for for each instance
    MpiDecMultiCtx      ctx;        // context of decoder
    MpiDecMultiCtxRet   ret;        // return of decoder
} MpiDecMultiCtxInfo;

static int multi_dec_simple(MpiDecMultiCtx *data)
{
    MpiDecTestCmd *cmd = data->cmd;
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos  = 0;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    MppPacket packet = data->packet;
    FileReader reader = cmd->reader;
    FileBufSlot *slot = NULL;
    RK_U32 quiet = data->quiet;
    MPP_RET ret = reader_index_read(reader, data->packet_count++, &slot);

    mpp_assert(ret == MPP_OK);
    mpp_assert(slot);

    pkt_eos = slot->eos;

    if (pkt_eos) {
        if (data->frame_num < 0 || data->frame_num > data->frame_count) {
            mpp_log_q(quiet, "%p loop again\n", ctx);
            data->packet_count = 0;
            pkt_eos = 0;
        } else {
            mpp_log_q(quiet, "%p found last packet\n", ctx);
            data->loop_end = 1;
        }
    }

    mpp_packet_set_data(packet, slot->data);
    mpp_packet_set_size(packet, slot->size);
    mpp_packet_set_pos(packet, slot->data);
    mpp_packet_set_length(packet, slot->size);
    // setup eos flag
    if (pkt_eos)
        mpp_packet_set_eos(packet);

    do {
        RK_U32 frm_eos = 0;
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
            MppFrame frame = NULL;

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
            if (ret) {
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
                    MppBufferGroup grp = NULL;

                    mpp_log_q(quiet, "decode_get_frame get info changed found\n");
                    mpp_log_q(quiet, "decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                              width, height, hor_stride, ver_stride, buf_size);

                    if (MPP_FRAME_FMT_IS_FBC(cmd->format)) {
                        MppFrame frm = NULL;

                        mpp_frame_init(&frm);
                        mpp_frame_set_width(frm, width);
                        mpp_frame_set_height(frm, height);
                        mpp_frame_set_fmt(frm, cmd->format);

                        ret = mpi->control(ctx, MPP_DEC_SET_FRAME_INFO, frm);
                        mpp_frame_deinit(&frm);

                        if (ret) {
                            mpp_err("set fbc frame info failed\n");
                            break;
                        }
                    }

                    grp = dec_buf_mgr_setup(data->buf_mgr, buf_size, 24, cmd->buf_mode);
                    /* Set buffer to mpp decoder */
                    ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, grp);
                    if (ret) {
                        mpp_err("%p set buffer group failed ret %d\n", ctx, ret);
                        break;
                    }
                    data->frm_grp = grp;

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
                    RK_U32 err_info = 0;

                    if (!data->first_frm)
                        data->first_frm = mpp_time();

                    err_info = mpp_frame_get_errinfo(frame) |
                               mpp_frame_get_discard(frame);
                    if (err_info) {
                        mpp_log_q(quiet, "decoder_get_frame get err info:%d discard:%d.\n",
                                  mpp_frame_get_errinfo(frame),
                                  mpp_frame_get_discard(frame));
                    }
                    mpp_log_q(quiet, "decode_get_frame get frame %d\n",
                              data->frame_count);

                    data->frame_count++;
                    if (data->fp_output && !err_info)
                        dump_mpp_frame_to_file(frame, data->fp_output);

                    fps_calc_inc(cmd->fps);
                }
                frm_eos = mpp_frame_get_eos(frame);
                mpp_frame_deinit(&frame);
                get_frm = 1;
            }

            // if last packet is send but last frame is not found continue
            if (pkt_eos && pkt_done && !frm_eos) {
                msleep(1);
                continue;
            }

            if ((data->frame_num > 0 && (data->frame_count >= data->frame_num)) ||
                ((data->frame_num == 0) && frm_eos))
                break;

            if (get_frm)
                continue;
            break;
        } while (1);

        if ((data->frame_num > 0 && (data->frame_count >= data->frame_num)) ||
            ((data->frame_num == 0) && frm_eos)) {
            data->loop_end = 1;
            break;
        }

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

static int multi_dec_advanced(MpiDecMultiCtx *data)
{
    MPP_RET ret = MPP_OK;
    MpiDecTestCmd *cmd = data->cmd;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    MppPacket packet = NULL;
    MppFrame  frame  = data->frame;
    MppTask task = NULL;
    RK_U32 quiet = data->quiet;
    FileBufSlot *slot = NULL;

    ret = reader_index_read(cmd->reader, 0, &slot);
    mpp_assert(ret == MPP_OK);
    mpp_assert(slot);

    mpp_packet_init_with_buffer(&packet, slot->buf);

    // setup eos flag
    if (slot->eos)
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

    if (!data->first_pkt)
        data->first_pkt = mpp_time();

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

        if (frame) {
            if (!data->first_frm)
                data->first_frm = mpp_time();

            if (data->fp_output)
                dump_mpp_frame_to_file(frame, data->fp_output);

            mpp_log_q(quiet, "%p decoded frame %d\n", ctx, data->frame_count);
            data->frame_count++;

            if (mpp_frame_get_eos(frame_out)) {
                mpp_log_q(quiet, "%p found eos frame\n", ctx);
            }
            fps_calc_inc(cmd->fps);
        }

        if (data->frame_num > 0) {
            if (data->frame_count >= data->frame_num)
                data->loop_end = 1;
        } else if (data->frame_num == 0) {
            if (slot->eos)
                data->loop_end = 1;
        }

        /* output queue */
        ret = mpi->enqueue(ctx, MPP_PORT_OUTPUT, task);
        if (ret)
            mpp_err("mpp task output enqueue failed\n");
    }

    /*
     * The following input port task dequeue and enqueue is to make sure that
     * the input packet can be released. We can directly deinit the input packet
     * after frame output in most cases.
     */
    if (0) {
        mpp_packet_deinit(&packet);
    } else {
        ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);  /* input queue */
        if (ret) {
            mpp_err("%p mpp task input dequeue failed\n", ctx);
            return ret;
        }

        mpp_assert(task);
        if (task) {
            MppPacket packet_out = NULL;

            mpp_task_meta_get_packet(task, KEY_INPUT_PACKET, &packet_out);

            if (!packet_out || packet_out != packet)
                mpp_err_f("mismatch packet %p -> %p\n", packet, packet_out);

            mpp_packet_deinit(&packet_out);

            /* input empty task back to mpp to maintain task status */
            ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);
            if (ret)
                mpp_err("%p mpp task input enqueue failed\n", ctx);
        }
    }

    return ret;
}

void* multi_dec_decode(void *cmd_ctx)
{
    MpiDecMultiCtxInfo *info = (MpiDecMultiCtxInfo *)cmd_ctx;
    MpiDecMultiCtx *dec_ctx  = &info->ctx;
    MpiDecMultiCtxRet *rets  = &info->ret;
    MpiDecTestCmd *cmd  = info->cmd;
    MPP_RET ret         = MPP_OK;

    // base flow context
    MppCtx ctx          = NULL;
    MppApi *mpi         = NULL;

    // input / output
    MppPacket packet    = NULL;
    MppFrame  frame     = NULL;

    // config for runtime mode
    MppDecCfg cfg       = NULL;
    RK_U32 need_split   = 1;

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    MppCodingType type  = cmd->type;

    // resources
    MppBuffer frm_buf   = NULL;

    if (cmd->have_output) {
        dec_ctx->fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == dec_ctx->fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
    }

    ret = dec_buf_mgr_init(&dec_ctx->buf_mgr);
    if (ret) {
        mpp_err("dec_buf_mgr_init failed\n");
        goto MPP_TEST_OUT;
    }

    if (cmd->simple) {
        ret = mpp_packet_init(&packet, NULL, 0);
        if (ret) {
            mpp_err("mpp_packet_init failed\n");
            goto MPP_TEST_OUT;
        }
    } else {
        RK_U32 hor_stride = MPP_ALIGN(width, 16);
        RK_U32 ver_stride = MPP_ALIGN(height, 16);

        ret = mpp_frame_init(&frame); /* output frame */
        if (ret) {
            mpp_err("mpp_frame_init failed\n");
            goto MPP_TEST_OUT;
        }

        dec_ctx->frm_grp = dec_buf_mgr_setup(dec_ctx->buf_mgr, hor_stride * ver_stride * 2, 4, cmd->buf_mode);
        if (!dec_ctx->frm_grp) {
            mpp_err("failed to get buffer group for input frame ret %d\n", ret);
            ret = MPP_NOK;
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

        mpp_frame_set_buffer(frame, frm_buf);
    }

    // decoder demo
    ret = mpp_create(&ctx, &mpi);
    if (ret) {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_log("%p mpi_dec_test decoder test start w %d h %d type %d\n",
            ctx, width, height, type);

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_dec_cfg_init(&cfg);

    /* get default config from decoder context */
    ret = mpi->control(ctx, MPP_DEC_GET_CFG, cfg);
    if (ret) {
        mpp_err("%p failed to get decoder cfg ret %d\n", ctx, ret);
        goto MPP_TEST_OUT;
    }

    /*
     * split_parse is to enable mpp internal frame spliter when the input
     * packet is not aplited into frames.
     */
    ret = mpp_dec_cfg_set_u32(cfg, "base:split_parse", need_split);
    if (ret) {
        mpp_err("%p failed to set split_parse ret %d\n", ctx, ret);
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx, MPP_DEC_SET_CFG, cfg);
    if (ret) {
        mpp_err("%p failed to set cfg %p ret %d\n", ctx, cfg, ret);
        goto MPP_TEST_OUT;
    }

    dec_ctx->cmd            = cmd;
    dec_ctx->ctx            = ctx;
    dec_ctx->mpi            = mpi;
    dec_ctx->packet         = packet;
    dec_ctx->frame          = frame;
    dec_ctx->packet_count   = 0;
    dec_ctx->frame_count    = 0;
    dec_ctx->frame_num      = cmd->frame_num;
    dec_ctx->quiet          = cmd->quiet;

    RK_S64 t_s, t_e;

    t_s = mpp_time();
    if (cmd->simple) {
        while (!dec_ctx->loop_end)
            multi_dec_simple(dec_ctx);
    } else {
        while (!dec_ctx->loop_end)
            multi_dec_advanced(dec_ctx);
    }
    t_e = mpp_time();

    ret = mpi->reset(ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

    rets->elapsed_time = t_e - t_s;
    rets->frame_count = dec_ctx->frame_count;
    rets->frame_rate = (float)dec_ctx->frame_count * 1000000 / rets->elapsed_time;
    rets->delay = dec_ctx->first_frm - dec_ctx->first_pkt;

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

    if (!cmd->simple) {
        if (frm_buf) {
            mpp_buffer_put(frm_buf);
            frm_buf = NULL;
        }
    }

    dec_ctx->frm_grp = NULL;
    if (dec_ctx->buf_mgr) {
        dec_buf_mgr_deinit(dec_ctx->buf_mgr);
        dec_ctx->buf_mgr = NULL;
    }

    if (dec_ctx->fp_output) {
        fclose(dec_ctx->fp_output);
        dec_ctx->fp_output = NULL;
    }

    if (cfg) {
        mpp_dec_cfg_deinit(cfg);
        cfg = NULL;
    }

    return NULL;
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiDecTestCmd  cmd_ctx;
    MpiDecTestCmd* cmd = &cmd_ctx;
    MpiDecMultiCtxInfo *ctxs = NULL;
    RK_S32 i = 0;
    float total_rate = 0.0;

    memset((void*)cmd, 0, sizeof(*cmd));
    cmd->nthreads = 1;

    // parse the cmd option
    ret = mpi_dec_test_cmd_init(cmd, argc, argv);
    if (ret)
        goto RET;

    mpi_dec_test_cmd_options(cmd);

    cmd->simple = (cmd->type != MPP_VIDEO_CodingMJPEG) ? (1) : (0);

    ctxs = mpp_calloc(MpiDecMultiCtxInfo, cmd->nthreads);
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

    if (cmd->frame_num < 0) {
        // wait for input then quit decoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop decoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        for (i = 0; i < cmd->nthreads; i++)
            ctxs[i].ctx.loop_end = 1;
    }

    for (i = 0; i < cmd->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

    for (i = 0; i < cmd->nthreads; i++) {
        MpiDecMultiCtxRet *dec_ret = &ctxs[i].ret;

        mpp_log("chn %2d decode %d frames time %lld ms delay %3d ms fps %3.2f\n", i,
                dec_ret->frame_count, (RK_S64)(dec_ret->elapsed_time / 1000),
                (RK_S32)(dec_ret->delay / 1000), dec_ret->frame_rate);

        total_rate += dec_ret->frame_rate;
    }
    mpp_free(ctxs);
    ctxs = NULL;

    total_rate /= cmd->nthreads;
    mpp_log("average frame rate %.2f\n", total_rate);

RET:
    mpi_dec_test_cmd_deinit(cmd);

    return (int)total_rate;
}
