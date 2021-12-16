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

#define MODULE_TAG "mpi_dec_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpi_dec_utils.h"
#include "utils.h"

typedef struct {
    MpiDecTestCmd   *cmd;
    MppCtx          ctx;
    MppApi          *mpi;
    RK_U32          quiet;

    /* end of stream flag when set quit the loop */
    RK_U32          loop_end;

    /* input and output */
    MppBufferGroup  frm_grp;
    MppPacket       packet;
    MppFrame        frame;

    FILE            *fp_output;
    RK_S32          frame_count;
    RK_S32          frame_num;

    RK_S64          first_pkt;
    RK_S64          first_frm;

    size_t          max_usage;
    float           frame_rate;
    RK_S64          elapsed_time;
    RK_S64          delay;
} MpiDecLoopData;

static int dec_simple(MpiDecLoopData *data)
{
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos  = 0;
    MPP_RET ret = MPP_OK;
    MpiDecTestCmd *cmd = data->cmd;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    MppPacket packet = data->packet;
    FileBufSlot *slot = NULL;
    RK_U32 quiet = data->quiet;

    // when packet size is valid read the input binary file
    ret = reader_read(cmd->reader, &slot);

    mpp_assert(ret == MPP_OK);
    mpp_assert(slot);

    pkt_eos = slot->eos;

    if (pkt_eos) {
        if (data->frame_num < 0 || data->frame_num > data->frame_count) {
            mpp_log_q(quiet, "%p loop again\n", ctx);
            reader_rewind(cmd->reader);
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
        RK_S32 times = 30;

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
                    msleep(1);
                    goto try_again;
                }
                mpp_err("%p decode_get_frame failed too much time\n", ctx);
            }
            if (ret) {
                mpp_err("%p decode_get_frame failed ret %d\n", ret, ctx);
                break;
            }

            if (frame) {
                if (mpp_frame_get_info_change(frame)) {
                    RK_U32 width = mpp_frame_get_width(frame);
                    RK_U32 height = mpp_frame_get_height(frame);
                    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
                    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
                    RK_U32 buf_size = mpp_frame_get_buf_size(frame);

                    mpp_log_q(quiet, "%p decode_get_frame get info changed found\n", ctx);
                    mpp_log_q(quiet, "%p decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                              ctx, width, height, hor_stride, ver_stride, buf_size);

                    /*
                     * NOTE: We can choose decoder's buffer mode here.
                     * There are three mode that decoder can support:
                     *
                     * Mode 1: Pure internal mode
                     * In the mode user will NOT call MPP_DEC_SET_EXT_BUF_GROUP
                     * control to decoder. Only call MPP_DEC_SET_INFO_CHANGE_READY
                     * to let decoder go on. Then decoder will use create buffer
                     * internally and user need to release each frame they get.
                     *
                     * Advantage:
                     * Easy to use and get a demo quickly
                     * Disadvantage:
                     * 1. The buffer from decoder may not be return before
                     * decoder is close. So memroy leak or crash may happen.
                     * 2. The decoder memory usage can not be control. Decoder
                     * is on a free-to-run status and consume all memory it can
                     * get.
                     * 3. Difficult to implement zero-copy display path.
                     *
                     * Mode 2: Half internal mode
                     * This is the mode current test code using. User need to
                     * create MppBufferGroup according to the returned info
                     * change MppFrame. User can use mpp_buffer_group_limit_config
                     * function to limit decoder memory usage.
                     *
                     * Advantage:
                     * 1. Easy to use
                     * 2. User can release MppBufferGroup after decoder is closed.
                     *    So memory can stay longer safely.
                     * 3. Can limit the memory usage by mpp_buffer_group_limit_config
                     * Disadvantage:
                     * 1. The buffer limitation is still not accurate. Memory usage
                     * is 100% fixed.
                     * 2. Also difficult to implement zero-copy display path.
                     *
                     * Mode 3: Pure external mode
                     * In this mode use need to create empty MppBufferGroup and
                     * import memory from external allocator by file handle.
                     * On Android surfaceflinger will create buffer. Then
                     * mediaserver get the file handle from surfaceflinger and
                     * commit to decoder's MppBufferGroup.
                     *
                     * Advantage:
                     * 1. Most efficient way for zero-copy display
                     * Disadvantage:
                     * 1. Difficult to learn and use.
                     * 2. Player work flow may limit this usage.
                     * 3. May need a external parser to get the correct buffer
                     * size for the external allocator.
                     *
                     * The required buffer size caculation:
                     * hor_stride * ver_stride * 3 / 2 for pixel data
                     * hor_stride * ver_stride / 2 for extra info
                     * Total hor_stride * ver_stride * 2 will be enough.
                     *
                     * For H.264/H.265 20+ buffers will be enough.
                     * For other codec 10 buffers will be enough.
                     */

                    if (NULL == data->frm_grp) {
                        /* If buffer group is not set create one and limit it */
                        ret = mpp_buffer_group_get_internal(&data->frm_grp, MPP_BUFFER_TYPE_ION);
                        if (ret) {
                            mpp_err("%p get mpp buffer group failed ret %d\n", ctx, ret);
                            break;
                        }

                        /* Set buffer to mpp decoder */
                        ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, data->frm_grp);
                        if (ret) {
                            mpp_err("%p set buffer group failed ret %d\n", ctx, ret);
                            break;
                        }
                    } else {
                        /* If old buffer group exist clear it */
                        ret = mpp_buffer_group_clear(data->frm_grp);
                        if (ret) {
                            mpp_err("%p clear buffer group failed ret %d\n", ctx, ret);
                            break;
                        }
                    }

                    /* Use limit config to limit buffer count to 24 with buf_size */
                    ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
                    if (ret) {
                        mpp_err("%p limit buffer group failed ret %d\n", ctx, ret);
                        break;
                    }

                    /*
                     * All buffer group config done. Set info change ready to let
                     * decoder continue decoding
                     */
                    ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                    if (ret) {
                        mpp_err("%p info change ready failed ret %d\n", ctx, ret);
                        break;
                    }
                } else {
                    char log_buf[256];
                    RK_S32 log_size = sizeof(log_buf) - 1;
                    RK_S32 log_len = 0;
                    RK_U32 err_info = mpp_frame_get_errinfo(frame);
                    RK_U32 discard = mpp_frame_get_discard(frame);

                    if (!data->first_frm)
                        data->first_frm = mpp_time();

                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        "decode get frame %d", data->frame_count);

                    if (mpp_frame_has_meta(frame)) {
                        MppMeta meta = mpp_frame_get_meta(frame);
                        RK_S32 temporal_id = 0;

                        mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id);

                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " tid %d", temporal_id);
                    }

                    if (err_info || discard) {
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " err %x discard %x", err_info, discard);
                    }
                    mpp_log_q(quiet, "%p %s\n", ctx, log_buf);

                    data->frame_count++;
                    if (data->fp_output && !err_info)
                        dump_mpp_frame_to_file(frame, data->fp_output);

                    fps_calc_inc(cmd->fps);
                }
                frm_eos = mpp_frame_get_eos(frame);
                mpp_frame_deinit(&frame);
                get_frm = 1;
            }

            // try get runtime frame memory usage
            if (data->frm_grp) {
                size_t usage = mpp_buffer_group_usage(data->frm_grp);
                if (usage > data->max_usage)
                    data->max_usage = usage;
            }

            // if last packet is send but last frame is not found continue
            if (pkt_eos && pkt_done && !frm_eos) {
                msleep(1);
                continue;
            }

            if (frm_eos) {
                mpp_log_q(quiet, "%p found last packet\n", ctx);
                break;
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
         * frame which resolution is 1080p needs 2 ms,so here we sleep 1ms
         * * is enough.
         */
        msleep(1);
    } while (1);

    return ret;
}

static int dec_advanced(MpiDecLoopData *data)
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
        mpp_err("%p mpp input poll failed\n", ctx);
        return ret;
    }

    ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);  /* input queue */
    if (ret) {
        mpp_err("%p mpp task input dequeue failed\n", ctx);
        return ret;
    }

    mpp_assert(task);

    mpp_task_meta_set_packet(task, KEY_INPUT_PACKET, packet);
    mpp_task_meta_set_frame (task, KEY_OUTPUT_FRAME,  frame);

    ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);  /* input queue */
    if (ret) {
        mpp_err("%p mpp task input enqueue failed\n", ctx);
        return ret;
    }

    if (!data->first_pkt)
        data->first_pkt = mpp_time();

    /* poll and wait here */
    ret = mpi->poll(ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("%p mpp output poll failed\n", ctx);
        return ret;
    }

    ret = mpi->dequeue(ctx, MPP_PORT_OUTPUT, &task); /* output queue */
    if (ret) {
        mpp_err("%p mpp task output dequeue failed\n", ctx);
        return ret;
    }

    mpp_assert(task);

    if (task) {
        MppFrame frame_out = NULL;

        mpp_task_meta_get_frame(task, KEY_OUTPUT_FRAME, &frame_out);

        if (frame) {
            if (!data->first_frm)
                data->first_frm = mpp_time();

            /* write frame to file here */
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
            mpp_err("%p mpp task output enqueue failed\n", ctx);
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

void *thread_decode(void *arg)
{
    MpiDecLoopData *data = (MpiDecLoopData *)arg;
    MpiDecTestCmd *cmd = data->cmd;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    RK_S64 t_s, t_e;

    t_s = mpp_time();

    if (cmd->simple) {
        while (!data->loop_end)
            dec_simple(data);
    } else {
        /* NOTE: change output format before jpeg decoding */
        if (MPP_FRAME_FMT_IS_YUV(cmd->format) || MPP_FRAME_FMT_IS_RGB(cmd->format)) {
            MPP_RET ret = mpi->control(ctx, MPP_DEC_SET_OUTPUT_FORMAT, &cmd->format);
            if (ret) {
                mpp_err("Failed to set output format %d\n", cmd->format);
                return NULL;
            }
        }

        while (!data->loop_end)
            dec_advanced(data);
    }

    t_e = mpp_time();
    data->elapsed_time = t_e - t_s;
    data->frame_count = data->frame_count;
    data->frame_rate = (float)data->frame_count * 1000000 / data->elapsed_time;
    data->delay = data->first_frm - data->first_pkt;

    mpp_log("decode %d frames time %lld ms delay %3d ms fps %3.2f\n",
            data->frame_count, (RK_S64)(data->elapsed_time / 1000),
            (RK_S32)(data->delay / 1000), data->frame_rate);

    return NULL;
}

int dec_decode(MpiDecTestCmd *cmd)
{
    // base flow context
    MppCtx ctx          = NULL;
    MppApi *mpi         = NULL;

    // input / output
    MppPacket packet    = NULL;
    MppFrame  frame     = NULL;

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    MppCodingType type  = cmd->type;

    // config for runtime mode
    MppDecCfg cfg       = NULL;
    RK_U32 need_split   = 1;

    // resources
    MppBuffer frm_buf   = NULL;
    pthread_t thd;
    pthread_attr_t attr;
    MpiDecLoopData data;
    MPP_RET ret = MPP_OK;

    mpp_log("mpi_dec_test start\n");
    memset(&data, 0, sizeof(data));
    pthread_attr_init(&attr);

    cmd->simple = (cmd->type != MPP_VIDEO_CodingMJPEG) ? (1) : (0);

    if (cmd->have_output) {
        data.fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == data.fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
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

        ret = mpp_buffer_group_get_internal(&data.frm_grp, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("failed to get buffer group for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_frame_init(&frame); /* output frame */
        if (ret) {
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
        ret = mpp_buffer_get(data.frm_grp, &frm_buf, hor_stride * ver_stride * 4);
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
        mpp_err("%p mpp_init failed\n", ctx);
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

    data.cmd            = cmd;
    data.ctx            = ctx;
    data.mpi            = mpi;
    data.loop_end       = 0;
    data.packet         = packet;
    data.frame          = frame;
    data.frame_count    = 0;
    data.frame_num      = cmd->frame_num;
    data.quiet          = cmd->quiet;

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&thd, &attr, thread_decode, &data);
    if (ret) {
        mpp_err("failed to create thread for input ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    if (cmd->frame_num < 0) {
        // wait for input then quit decoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop decoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        data.loop_end = 1;
    }

    pthread_join(thd, NULL);

    cmd->max_usage = data.max_usage;

    ret = mpi->reset(ctx);
    if (ret) {
        mpp_err("%p mpi->reset failed\n", ctx);
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    if (data.packet) {
        mpp_packet_deinit(&data.packet);
        data.packet = NULL;
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

    if (data.frm_grp) {
        mpp_buffer_group_put(data.frm_grp);
        data.frm_grp = NULL;
    }

    if (data.fp_output) {
        fclose(data.fp_output);
        data.fp_output = NULL;
    }

    if (cfg) {
        mpp_dec_cfg_deinit(cfg);
        cfg = NULL;
    }

    pthread_attr_destroy(&attr);

    return ret;
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiDecTestCmd  cmd_ctx;
    MpiDecTestCmd* cmd = &cmd_ctx;

    memset((void*)cmd, 0, sizeof(*cmd));
    cmd->format = MPP_FMT_BUTT;
    cmd->pkt_size = MPI_DEC_STREAM_SIZE;

    // parse the cmd option
    ret = mpi_dec_test_cmd_init(cmd, argc, argv);
    if (ret)
        goto RET;

    mpi_dec_test_cmd_options(cmd);

    ret = dec_decode(cmd);
    if (MPP_OK == ret)
        mpp_log("test success max memory %.2f MB\n", cmd->max_usage / (float)(1 << 20));
    else
        mpp_err("test failed ret %d\n", ret);

RET:
    mpi_dec_test_cmd_deinit(cmd);

    return ret;
}

