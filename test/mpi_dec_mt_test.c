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

#define MODULE_TAG "mpi_dec_mt_test"

#include <string.h>
#include <pthread.h>

#include "rk_mpi.h"

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpi_dec_utils.h"

typedef struct {
    MpiDecTestCmd   *cmd;
    MppCtx          ctx;
    MppApi          *mpi;

    volatile RK_U32 loop_end;

    /* buffer for stream data reading */
    char            *buf;

    /* input and output */
    MppBufferGroup  frm_grp;
    MppPacket       packet;
    size_t          packet_size;
    MppFrame        frame;

    FILE            *fp_output;
    RK_S32          frame_count;
    RK_S32          frame_num;
    FileReader      reader;

    /* runtime flag */
    RK_U32          quiet;
} MpiDecMtLoopData;

void *thread_input(void *arg)
{
    MpiDecMtLoopData *data = (MpiDecMtLoopData *)arg;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    MppPacket packet = data->packet;
    FileReader reader = data->reader;
    RK_U32 quiet = data->quiet;

    mpp_log_q(quiet, "put packet thread start\n");

    do {
        RK_U32 pkt_eos = 0;
        FileBufSlot *slot = NULL;
        MPP_RET ret = reader_read(reader, &slot);
        if (ret)
            break;

        mpp_packet_set_data(packet, slot->data);
        mpp_packet_set_size(packet, slot->size);
        mpp_packet_set_pos(packet, slot->data);
        mpp_packet_set_length(packet, slot->size);

        pkt_eos = slot->eos;
        // setup eos flag
        if (pkt_eos) {
            if (data->frame_num < 0 || data->frame_count < data->frame_num) {
                mpp_log_q(quiet, "%p loop again\n", ctx);
                reader_rewind(reader);
                pkt_eos = 0;
            } else {
                mpp_log_q(quiet, "%p found last packet\n", ctx);
                mpp_packet_set_eos(packet);
            }
        }

        // send packet until it success
        do {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret) {
                mpp_assert(0 == mpp_packet_get_length(packet));
                break;

            }
            // if failed wait a moment and retry
            msleep(1);
        } while (!data->loop_end);

        if (pkt_eos)
            break;
    } while (!data->loop_end);

    mpp_log_q(quiet, "put packet thread end\n");

    return NULL;
}

void *thread_output(void *arg)
{
    MpiDecMtLoopData *data = (MpiDecMtLoopData *)arg;
    MpiDecTestCmd *cmd = data->cmd;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    RK_U32 quiet = data->quiet;

    mpp_log_q(quiet, "get frame thread start\n");

    // then get all available frame and release
    do {
        RK_U32 frm_eos = 0;
        MppFrame frame = NULL;
        MPP_RET ret = mpi->decode_get_frame(ctx, &frame);

        if (ret) {
            mpp_err("decode_get_frame failed ret %d\n", ret);
            continue;
        }

        if (NULL == frame) {
            msleep(1);
            continue;
        }

        if (mpp_frame_get_info_change(frame)) {
            // found info change and create buffer group for decoding
            RK_U32 width = mpp_frame_get_width(frame);
            RK_U32 height = mpp_frame_get_height(frame);
            RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
            RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
            RK_U32 buf_size = mpp_frame_get_buf_size(frame);

            mpp_log_q(quiet, "decode_get_frame get info changed found\n");
            mpp_log_q(quiet, "decoder require buffer w:h [%d:%d] stride [%d:%d] size %d\n",
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

            /* Use limit config to limit buffer count to 24 */
            ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
            if (ret) {
                mpp_err("limit buffer group failed ret %d\n", ret);
                break;
            }

            ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
            if (ret) {
                mpp_err("info change ready failed ret %d\n", ret);
                break;
            }
        } else {
            char log_buf[256];
            RK_S32 log_size = sizeof(log_buf) - 1;
            RK_S32 log_len = 0;
            RK_U32 err_info = mpp_frame_get_errinfo(frame);
            RK_U32 discard = mpp_frame_get_discard(frame);

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

        if ((data->frame_num > 0 && (data->frame_count >= data->frame_num)) ||
            ((data->frame_num == 0) && frm_eos))
            data->loop_end = 1;
    } while (!data->loop_end);

    mpp_log_q(quiet, "get frame thread end\n");

    return NULL;
}

int mt_dec_decode(MpiDecTestCmd *cmd)
{
    MPP_RET ret         = MPP_OK;
    FileReader reader   = cmd->reader;

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

    pthread_t thd_in;
    pthread_t thd_out = 0;
    pthread_attr_t attr;
    MpiDecMtLoopData data;

    mpp_log("mpi_dec_mt_test start\n");
    memset(&data, 0, sizeof(data));

    if (cmd->have_output) {
        data.fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == data.fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
    }

    ret = mpp_packet_init(&packet, NULL, 0);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_log("mpi_dec_mt_test decoder test start w %d h %d type %d\n", width, height, type);

    // decoder demo
    ret = mpp_create(&ctx, &mpi);
    if (ret) {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_OUT;
    }

    // NOTE: timeout value please refer to MppPollType definition
    //  0   - non-block call (default)
    // -1   - block call
    // +val - timeout value in ms
    {
        MppPollType timeout = MPP_POLL_BLOCK;
        MppParam param = &timeout;

        ret = mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, param);
        if (ret) {
            mpp_err("Failed to set output timeout %d ret %d\n", timeout, ret);
            goto MPP_TEST_OUT;
        }
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
    data.reader         = reader;
    data.quiet          = cmd->quiet;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&thd_in, &attr, thread_input, &data);
    if (ret) {
        mpp_err("failed to create thread for input ret %d\n", ret);
        goto THREAD_END;
    }

    ret = pthread_create(&thd_out, &attr, thread_output, &data);
    if (ret) {
        mpp_err("failed to create thread for output ret %d\n", ret);
        goto THREAD_END;
    }

    if (cmd->frame_num < 0) {
        // wait for input then quit decoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop decoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        data.loop_end = 1;
    }

THREAD_END:
    pthread_attr_destroy(&attr);

    pthread_join(thd_in, NULL);
    pthread_join(thd_out, NULL);

    ret = mpi->reset(ctx);
    if (ret) {
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

    return ret;
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiDecTestCmd  cmd_ctx;
    MpiDecTestCmd* cmd = &cmd_ctx;

    memset((void*)cmd, 0, sizeof(*cmd));
    // default use block mode
    cmd->timeout = -1;

    // parse the cmd option
    ret = mpi_dec_test_cmd_init(cmd, argc, argv);
    if (ret)
        goto RET;

    mpi_dec_test_cmd_options(cmd);

    ret = mt_dec_decode(cmd);
    if (MPP_OK == ret)
        mpp_log("test success\n");
    else
        mpp_err("test failed ret %d\n", ret);

RET:
    mpi_dec_test_cmd_deinit(cmd);

    return ret;
}

