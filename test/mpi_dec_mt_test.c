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

#include "utils.h"
#include "mpi_dec_utils.h"

typedef struct {
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

    FILE            *fp_input;
    FILE            *fp_output;
    RK_U64          frame_count;
    RK_S32          frame_num;
    FileReader      reader;

    /* runtime flag */
    RK_U32          quiet;
} MpiDecLoopData;

void *thread_input(void *arg)
{
    MpiDecLoopData *data = (MpiDecLoopData *)arg;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    char   *buf = data->buf;
    MppPacket packet = data->packet;
    FileReader reader = data->reader;

    mpp_log("put packet thread start\n");

    do {
        RK_U32 pkt_done = 0;
        RK_U32 pkt_eos  = 0;
        size_t read_size = 0;

        pkt_eos = reader_read(reader, &buf, &read_size);

        if (pkt_eos)
            reader_rewind(reader);

        mpp_packet_set_data(packet, buf);
        if (read_size > mpp_packet_get_size(packet))
            mpp_packet_set_size(packet, read_size);
        mpp_packet_set_pos(packet, buf);
        mpp_packet_set_length(packet, read_size);
        // setup eos flag
        if (pkt_eos) {
            mpp_packet_set_eos(packet);
            // mpp_log("found last packet\n");
        } else
            mpp_packet_clr_eos(packet);

        // send packet until it success
        do {
            MPP_RET ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret)
                pkt_done = 1;
            else {
                // if failed wait a moment and retry
                msleep(5);
            }
        } while (!pkt_done);
    } while (!data->loop_end);

    mpp_log("put packet thread end\n");

    return NULL;
}

void *thread_output(void *arg)
{
    MpiDecLoopData *data = (MpiDecLoopData *)arg;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    MppFrame  frame  = NULL;
    RK_U32 quiet = data->quiet;

    mpp_log("get frame thread start\n");

    // then get all available frame and release
    do {
        RK_S32 times = 5;
        MPP_RET ret = MPP_OK;

    GET_AGAIN:
        ret = mpi->decode_get_frame(ctx, &frame);
        if (MPP_ERR_TIMEOUT == ret) {
            if (times > 0) {
                times--;
                msleep(2);
                goto GET_AGAIN;
            }
            mpp_err("decode_get_frame failed too much time\n");
        }
        if (MPP_OK != ret) {
            mpp_err("decode_get_frame failed ret %d\n", ret);
            continue;
        }

        if (frame) {
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
                // found normal output frame
                RK_U32 err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
                if (err_info)
                    mpp_log_q(quiet, "decoder_get_frame get err info:%d discard:%d.\n",
                              mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));

                if (data->fp_output && !err_info)
                    dump_mpp_frame_to_file(frame, data->fp_output);

                // calculate fps here
                static RK_S64 last_time = 0;
                static RK_S64 last_count = 0;

                if (data->frame_count == 0) {
                    last_time = mpp_time();
                    last_count = 0;
                } else {
                    RK_S64 now = mpp_time();
                    RK_S64 elapsed = now - last_time;

                    // print on each second
                    if (elapsed >= 1000000) {
                        RK_S64 frames = data->frame_count - last_count;
                        float fps = (float)frames * 1000000 / elapsed;

                        mpp_log("decoded %10lld frame %7.2f fps\n",
                                data->frame_count, fps);

                        last_time = now;
                        last_count = data->frame_count;
                    }
                }
                data->frame_count++;
            }

            if (mpp_frame_get_eos(frame)) {
                mpp_log_q(quiet, "found last frame loop again\n");
                // when get a eos status mpp need a reset to restart decoding
                ret = mpi->reset(ctx);
                if (MPP_OK != ret)
                    mpp_err("mpi->reset failed\n");
            }

            mpp_frame_deinit(&frame);
            frame = NULL;
            if (data->frame_num > 0 && data->frame_count >= (RK_U32)data->frame_num) {
                data->loop_end = 1;
                mpp_log("%p reach max frame number %d\n", ctx, data->frame_count);
                break;
            }
        }
    } while (!data->loop_end);

    mpp_log("get frame thread end\n");

    return NULL;
}

int mt_dec_decode(MpiDecTestCmd *cmd)
{
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
    FileReader reader   = NULL;

    // resources
    size_t packet_size  = MPI_DEC_STREAM_SIZE;

    pthread_t thd_in;
    pthread_t thd_out;
    pthread_attr_t attr;
    MpiDecLoopData data;

    mpp_log("mpi_dec_test start\n");
    memset(&data, 0, sizeof(data));

    if (cmd->have_input) {
        data.fp_input = fopen(cmd->file_input, "rb");
        if (NULL == data.fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            goto MPP_TEST_OUT;
        }

        fseek(data.fp_input, 0L, SEEK_END);
        file_size = ftell(data.fp_input);
        rewind(data.fp_input);
        reader_init(&reader, cmd->file_input, data.fp_input);
        mpp_log("input file size %ld\n", file_size);
    }

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

    data.ctx            = ctx;
    data.mpi            = mpi;
    data.loop_end       = 0;
    data.packet         = packet;
    data.packet_size    = packet_size;
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
    // wait for input then quit decoding
    mpp_log("*******************************************\n");
    mpp_log("**** Press Enter to stop loop decoding ****\n");
    mpp_log("*******************************************\n");

    getc(stdin);
    data.loop_end = 1;
    ret = mpi->reset(ctx);
    if (MPP_OK != ret)
        mpp_err("mpi->reset failed\n");

THREAD_END:
    pthread_attr_destroy(&attr);
    mpp_log("all threads end\n");
    pthread_join(thd_in, NULL);
    pthread_join(thd_out, NULL);

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

    reader_deinit(&reader);

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

    if (data.fp_input) {
        fclose(data.fp_input);
        data.fp_input = NULL;
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
    ret = mpi_dec_test_parse_options(argc, argv, cmd);
    if (ret) {
        if (ret < 0) {
            mpp_err("mpi_dec_test_parse_options: input parameter invalid\n");
        }

        mpi_dec_test_help();
        return ret;
    }

    mpi_dec_test_show_options(cmd);

    mpp_env_set_u32("mpi_debug", cmd->debug);

    ret = mt_dec_decode(cmd);
    if (MPP_OK == ret)
        mpp_log("test success\n");
    else
        mpp_err("test failed ret %d\n", ret);

    mpp_env_set_u32("mpi_debug", 0x0);
    return ret;
}

