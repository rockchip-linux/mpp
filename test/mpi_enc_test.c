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

#define MODULE_TAG "mpi_enc_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "utils.h"

#include "vpu_api.h"

#define MPI_ENC_IO_COUNT            (4)
#define MAX_FILE_NAME_LENGTH        256

typedef struct {
    char            file_input[MAX_FILE_NAME_LENGTH];
    char            file_output[MAX_FILE_NAME_LENGTH];
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    MppFrameFormat  format;
    RK_U32          debug;

    RK_U32          have_input;
    RK_U32          have_output;
} MpiEncTestCmd;

static OptionInfo mpi_enc_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input bitstream"},
    {"h",               "height",               "the height of input bitstream"},
    {"f",               "format",               "the picture format of input bitstream"},
    {"t",               "type",                 "input stream coding type"},
    {"d",               "debug",                "debug flag"},
};

int mpi_enc_test(MpiEncTestCmd *cmd)
{
    MPP_RET ret             = MPP_OK;
    RK_U32 frm_eos          = 0;
    RK_U32 pkt_eos          = 0;
    FILE *fp_input          = NULL;
    FILE *fp_output         = NULL;

    // base flow context
    MppCtx ctx              = NULL;
    MppApi *mpi             = NULL;
    MppEncConfig mpp_cfg;

    // input / output
    RK_S32 i;
    MppBufferGroup frm_grp  = NULL;
    MppBufferGroup pkt_grp  = NULL;
    MppFrame  frame         = NULL;
    MppPacket packet        = NULL;
    MppBuffer frm_buf[MPI_ENC_IO_COUNT] = { NULL };
    MppBuffer pkt_buf[MPI_ENC_IO_COUNT] = { NULL };

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    MppFrameFormat fmt  = cmd->format;
    MppCodingType type  = cmd->type;

    // resources
    size_t frame_size   = hor_stride * ver_stride * 3 / 2;
    size_t packet_size  = width * height;           /* NOTE: packet buffer may overflow */
    size_t read_size    = 0;
    RK_U32 frame_count  = 0;
    RK_U64 stream_size  = 0;

    mpp_log("mpi_enc_test start\n");

    if (cmd->have_input) {
        fp_input = fopen(cmd->file_input, "rb");
        if (NULL == fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            ret = MPP_ERR_OPEN_FILE;
            goto MPP_TEST_OUT;
        }
    }

    if (cmd->have_output) {
        fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            ret = MPP_ERR_OPEN_FILE;
            goto MPP_TEST_OUT;
        }
    }

    ret = mpp_buffer_group_get_internal(&frm_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("failed to get buffer group for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&pkt_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("failed to get buffer group for output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    for (i = 0; i < MPI_ENC_IO_COUNT; i++) {
        ret = mpp_buffer_get(frm_grp, &frm_buf[i], frame_size);
        if (ret) {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_buffer_get(pkt_grp, &pkt_buf[i], packet_size);
        if (ret) {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }
    }

    mpp_log("mpi_enc_test encoder test start w %d h %d type %d\n", width, height, type);

    // encoder demo
    ret = mpp_create(&ctx, &mpi);

    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(ctx, MPP_CTX_ENC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_OUT;
    }

    memset(&mpp_cfg, 0, sizeof(mpp_cfg));
    mpp_cfg.size        = sizeof(mpp_cfg);
    mpp_cfg.width       = width;
    mpp_cfg.height      = height;
    mpp_cfg.format      = fmt;
    mpp_cfg.rc_mode     = 0;
    mpp_cfg.skip_cnt    = 0;
    mpp_cfg.fps_in      = 30;
    mpp_cfg.fps_out     = 30;
    mpp_cfg.bps         = width * height * 2 * mpp_cfg.fps_in;
    mpp_cfg.qp          = 24;
    mpp_cfg.gop         = 60;

    mpp_cfg.profile     = 100;
    mpp_cfg.level       = 41;
    mpp_cfg.cabac_en    = 1;

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, &mpp_cfg);
    if (MPP_OK != ret) {
        mpp_err("mpi control enc set cfg failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
    if (MPP_OK != ret) {
        mpp_err("mpi control enc get extra info failed\n");
        goto MPP_TEST_OUT;
    }

    /* get and write sps/pps for H.264 */
    if (packet) {
        void *ptr   = mpp_packet_get_pos(packet);
        size_t len  = mpp_packet_get_length(packet);

        if (fp_output)
            fwrite(ptr, 1, len, fp_output);

        packet = NULL;
    }

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        mpp_err("mpp_frame_init failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, hor_stride);
    mpp_frame_set_ver_stride(frame, ver_stride);

    i = 0;
    while (!pkt_eos) {
        MppTask task = NULL;
        RK_S32 index = i++;
        MppBuffer frm_buf_in  = frm_buf[index];
        MppBuffer pkt_buf_out = pkt_buf[index];
        void *buf = mpp_buffer_get_ptr(frm_buf_in);

        if (i == MPI_ENC_IO_COUNT)
            i = 0;

        read_size = fread(buf, 1, frame_size, fp_input);
        if (read_size != frame_size || feof(fp_input)) {
            mpp_log("found last frame\n");
            frm_eos = 1;
        }

        mpp_frame_set_buffer(frame, frm_buf_in);
        mpp_frame_set_eos(frame, frm_eos);

        mpp_packet_init_with_buffer(&packet, pkt_buf_out);

        do {
            ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);
            if (ret) {
                mpp_err("mpp task input dequeue failed\n");
                goto MPP_TEST_OUT;
            }
            if (task == NULL) {
                mpp_log("mpi dequeue from MPP_PORT_INPUT fail, task equal with NULL!");
                msleep(3);
            } else {
                MppFrame frame_out = NULL;

                mpp_task_meta_get_frame (task, MPP_META_KEY_INPUT_FRM,  &frame_out);
                if (frame_out)
                    mpp_assert(frame_out == frame);

                break;
            }
        } while (1);


        mpp_task_meta_set_frame (task, MPP_META_KEY_INPUT_FRM,  frame);
        mpp_task_meta_set_packet(task, MPP_META_KEY_OUTPUT_PKT, packet);

        ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);
        if (ret) {
            mpp_err("mpp task input enqueue failed\n");
            goto MPP_TEST_OUT;
        }

        msleep(20);

        do {
            ret = mpi->dequeue(ctx, MPP_PORT_OUTPUT, &task);
            if (ret) {
                mpp_err("mpp task output dequeue failed\n");
                goto MPP_TEST_OUT;
            }

            if (task) {
                MppFrame packet_out = NULL;

                mpp_task_meta_get_packet(task, MPP_META_KEY_OUTPUT_PKT, &packet_out);

                mpp_assert(packet_out == packet);
                if (packet) {
                    // write packet to file here
                    void *ptr   = mpp_packet_get_pos(packet);
                    size_t len  = mpp_packet_get_length(packet);

                    pkt_eos = mpp_packet_get_eos(packet);

                    if (fp_output)
                        fwrite(ptr, 1, len, fp_output);
                    mpp_packet_deinit(&packet);

                    mpp_log_f("encoded frame %d size %d\n", frame_count, len);
                    stream_size += len;

                    if (pkt_eos) {
                        mpp_log("found last packet\n");
                        mpp_assert(frm_eos);
                    }
                }
                frame_count++;

                ret = mpi->enqueue(ctx, MPP_PORT_OUTPUT, task);
                if (ret) {
                    mpp_err("mpp task output enqueue failed\n");
                    goto MPP_TEST_OUT;
                }
                break;
            }
        } while (1);

        if (frm_eos && pkt_eos)
            break;
    }

    ret = mpi->reset(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (frame) {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    for (i = 0; i < MPI_ENC_IO_COUNT; i++) {
        if (frm_buf[i]) {
            mpp_buffer_put(frm_buf[i]);
            frm_buf[i] = NULL;
        }

        if (pkt_buf[i]) {
            mpp_buffer_put(pkt_buf[i]);
            pkt_buf[i] = NULL;
        }
    }

    if (frm_grp) {
        mpp_buffer_group_put(frm_grp);
        frm_grp = NULL;
    }

    if (pkt_grp) {
        mpp_buffer_group_put(pkt_grp);
        pkt_grp = NULL;
    }

    if (fp_output) {
        fclose(fp_output);
        fp_output = NULL;
    }

    if (fp_input) {
        fclose(fp_input);
        fp_input = NULL;
    }

    if (MPP_OK == ret)
        mpp_log("mpi_enc_test success total frame %d bps %lld\n",
                frame_count, (RK_U64)((stream_size * 8 * mpp_cfg.fps_out) / frame_count));
    else
        mpp_err("mpi_enc_test failed ret %d\n", ret);

    return ret;
}


static void mpi_enc_test_help()
{
    mpp_log("usage: mpi_enc_test [options]\n");
    show_options(mpi_enc_cmd);
    mpp_show_support_format();
}

static RK_S32 mpi_enc_test_parse_options(int argc, char **argv, MpiEncTestCmd* cmd)
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
                    mpi_enc_test_help();
                    err = 1;
                    goto PARSE_OPINIONS_OUT;
                } else if (next) {
                    cmd->height = atoi(next);
                } else {
                    mpp_log("input height is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'f':
                if (next) {
                    cmd->format = (MppFrameFormat)atoi(next);
                    err = ((cmd->format >= MPP_FMT_YUV_BUTT && cmd->format < MPP_FRAME_FMT_RGB) ||
                            cmd->format >= MPP_FMT_RGB_BUTT);
                }

                if (!next || err) {
                    mpp_err("invalid input format %d\n", cmd->format);
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 't':
                if (next) {
                    cmd->type = (MppCodingType)atoi(next);
                    err = mpp_check_support_format(MPP_CTX_ENC, cmd->type);
                }

                if (!next || err) {
                    mpp_err("invalid input coding type\n");
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

static void mpi_enc_test_show_options(MpiEncTestCmd* cmd)
{
    mpp_log("cmd parse result:\n");
    mpp_log("input  file name: %s\n", cmd->file_input);
    mpp_log("output file name: %s\n", cmd->file_output);
    mpp_log("width      : %d\n", cmd->width);
    mpp_log("height     : %d\n", cmd->height);
    mpp_log("type       : %d\n", cmd->type);
    mpp_log("debug flag : %x\n", cmd->debug);
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiEncTestCmd  cmd_ctx;
    MpiEncTestCmd* cmd = &cmd_ctx;

    memset((void*)cmd, 0, sizeof(*cmd));

    // parse the cmd option
    ret = mpi_enc_test_parse_options(argc, argv, cmd);
    if (ret) {
        if (ret < 0) {
            mpp_err("mpi_enc_test_parse_options: input parameter invalid\n");
        }

        mpi_enc_test_help();
        return ret;
    }

    mpi_enc_test_show_options(cmd);

    mpp_env_set_u32("mpi_debug", cmd->debug);

    mpi_enc_test(cmd);

    mpp_env_set_u32("mpi_debug", 0x0);
    return 0;
}

