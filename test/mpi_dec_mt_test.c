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

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "utils.h"

#define MPI_DEC_LOOP_COUNT          4
#define MPI_DEC_STREAM_SIZE         (SZ_4K)
#define MAX_FILE_NAME_LENGTH        256

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
} MpiDecLoopData;

typedef struct {
    char            file_input[MAX_FILE_NAME_LENGTH];
    char            file_output[MAX_FILE_NAME_LENGTH];
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    RK_U32          debug;

    RK_U32          have_input;
    RK_U32          have_output;

    RK_S32          timeout;
} MpiDecTestCmd;

static OptionInfo mpi_dec_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input bitstream"},
    {"h",               "height",               "the height of input bitstream"},
    {"t",               "type",                 "input stream coding type"},
    {"d",               "debug",                "debug flag"},
    {"x",               "timeout",              "output timeout interval"},
};

void *thread_input(void *arg)
{
    MpiDecLoopData *data = (MpiDecLoopData *)arg;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    char   *buf = data->buf;
    MppPacket packet = data->packet;

    mpp_log("put packet thread start\n");

    do {
        RK_U32 pkt_done = 0;
        RK_U32 pkt_eos  = 0;
        size_t read_size = fread(buf, 1, data->packet_size, data->fp_input);

        if (read_size != data->packet_size || feof(data->fp_input)) {
            // setup eos flag
            pkt_eos = 1;

            // reset file to start and clear eof / ferror
            clearerr(data->fp_input);
            rewind(data->fp_input);
        }

        // write data to packet
        mpp_packet_write(packet, 0, buf, read_size);
        // reset pos and set valid length
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

                mpp_log("decode_get_frame get info changed found\n");
                mpp_log("decoder require buffer w:h [%d:%d] stride [%d:%d] size %d\n",
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
                if (0/*err_info*/)
                    mpp_log("decoder_get_frame get err info:%d discard:%d.\n",
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

                        last_time = now + 1000000;
                        last_count = data->frame_count;
                    }
                }
                data->frame_count++;
            }

            if (mpp_frame_get_eos(frame)) {
                // mpp_log("found last frame\n");
                // when get a eos status mpp need a reset to restart decoding
                ret = mpi->reset(ctx);
                if (MPP_OK != ret)
                    mpp_err("mpi->reset failed\n");
            }

            mpp_frame_deinit(&frame);
            frame = NULL;
        }
    } while (!data->loop_end);

    mpp_log("get frame thread end\n");

    return NULL;
}

int mpi_dec_test_decode(MpiDecTestCmd *cmd)
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

    // resources
    char *buf           = NULL;
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
        mpp_log("input file size %ld\n", file_size);
    }

    if (cmd->have_output) {
        data.fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == data.fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
    }

    buf = mpp_malloc(char, packet_size);
    if (NULL == buf) {
        mpp_err("mpi_dec_test malloc input stream buffer failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_packet_init(&packet, buf, packet_size);
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
    data.buf            = buf;
    data.packet         = packet;
    data.packet_size    = packet_size;
    data.frame          = frame;
    data.frame_count    = 0;

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

    msleep(500);
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

    if (frame) {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (buf) {
        mpp_free(buf);
        buf = NULL;
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

static void mpi_dec_test_help()
{
    mpp_log("usage: mpi_dec_test [options]\n");
    show_options(mpi_dec_cmd);
    mpp_show_support_format();
}

static RK_S32 mpi_dec_test_parse_options(int argc, char **argv, MpiDecTestCmd* cmd)
{
    const char *opt;
    const char *next;
    RK_S32 optindex = 1;
    RK_S32 handleoptions = 1;
    RK_S32 err = MPP_NOK;

    if ((argc < 2) || (cmd == NULL)) {
        err = 1;
        return err;
    }

    /* parse options */
    while (optindex < argc) {
        opt  = (const char*)argv[optindex++];
        next = (const char*)argv[optindex];

        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
            if (opt[1] == '-') {
                if (opt[2] != '\0') {
                    opt++;
                } else {
                    handleoptions = 0;
                    continue;
                }
            }

            opt++;

            switch (*opt) {
            case 'i':
                if (next) {
                    strncpy(cmd->file_input, next, MAX_FILE_NAME_LENGTH);
                    cmd->file_input[strlen(next)] = '\0';
                    cmd->have_input = 1;
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'o':
                if (next) {
                    strncpy(cmd->file_output, next, MAX_FILE_NAME_LENGTH);
                    cmd->file_output[strlen(next)] = '\0';
                    cmd->have_output = 1;
                } else {
                    mpp_log("output file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'd':
                if (next) {
                    cmd->debug = atoi(next);;
                } else {
                    mpp_err("invalid debug flag\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'w':
                if (next) {
                    cmd->width = atoi(next);
                } else {
                    mpp_err("invalid input width\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'h':
                if ((*(opt + 1) != '\0') && !strncmp(opt, "help", 4)) {
                    mpi_dec_test_help();
                    err = 1;
                    goto PARSE_OPINIONS_OUT;
                } else if (next) {
                    cmd->height = atoi(next);
                } else {
                    mpp_log("input height is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 't':
                if (next) {
                    cmd->type = (MppCodingType)atoi(next);
                    err = mpp_check_support_format(MPP_CTX_DEC, cmd->type);
                }

                if (!next || err) {
                    mpp_err("invalid input coding type\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'x':
                if (next) {
                    cmd->timeout = atoi(next);
                }
                if (!next || cmd->timeout < 0) {
                    mpp_err("invalid output timeout interval\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            default:
                goto PARSE_OPINIONS_OUT;
                break;
            }

            optindex++;
        }
    }

    err = 0;

PARSE_OPINIONS_OUT:
    return err;
}

static void mpi_dec_test_show_options(MpiDecTestCmd* cmd)
{
    mpp_log("cmd parse result:\n");
    mpp_log("input  file name: %s\n", cmd->file_input);
    mpp_log("output file name: %s\n", cmd->file_output);
    mpp_log("width      : %4d\n", cmd->width);
    mpp_log("height     : %4d\n", cmd->height);
    mpp_log("type       : %d\n", cmd->type);
    mpp_log("debug flag : %x\n", cmd->debug);
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

    ret = mpi_dec_test_decode(cmd);
    if (MPP_OK == ret)
        mpp_log("test success\n");
    else
        mpp_err("test failed ret %d\n", ret);

    mpp_env_set_u32("mpi_debug", 0x0);
    return ret;
}

