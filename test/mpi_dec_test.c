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

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"

#include "utils.h"

#define MPI_DEC_LOOP_COUNT          4
#define MPI_STREAM_SIZE             (SZ_512K)
#define MAX_FILE_NAME_LENGTH        256

typedef struct {
    char            file_input[MAX_FILE_NAME_LENGTH];
    char            file_output[MAX_FILE_NAME_LENGTH];
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    RK_U32          debug;

    RK_U32          have_input;
    RK_U32          have_output;
} MpiDecTestCmd;

static OptionInfo mpi_dec_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input bitstream"},
    {"h",               "height",               "the height of input bitstream"},
    {"f",               "type",                 "input stream coding type"},
    {"d",               "debug",                "debug flag"},
};

int mpi_dec_test(MpiDecTestCmd *cmd)
{
    MPP_RET ret         = MPP_OK;
    RK_U32 found_eos    = 0;
    FILE *fp_input      = NULL;
    FILE *fp_output     = NULL;

    // base flow context
    MppCtx ctx          = NULL;
    MppApi *mpi         = NULL;

    // input / output
    MppPacket packet    = NULL;
    MppFrame  frame     = NULL;

    MpiCmd mpi_cmd      = MPP_CMD_BASE;
    MppParam param      = NULL;
    RK_U32 need_split    = 1;

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    MppCodingType type  = cmd->type;

    // resources
    char *buf           = NULL;
    size_t packet_size  = MPI_STREAM_SIZE;
    size_t read_size    = 0;

    mpp_log("mpi_dec_test start\n");

    if (cmd->have_input) {
        fp_input = fopen(cmd->file_input, "rb");
        if (NULL == fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            goto MPP_TEST_OUT;
        }
    }

    if (cmd->have_output) {
        fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == fp_output) {
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

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_OUT;
    }

    while (!found_eos) {
        read_size = fread(buf, 1, packet_size, fp_input);
        mpp_log("read packet length %u\n", read_size);
        if (read_size != packet_size || feof(fp_input)) {
            mpp_log("found last packet\n");
            found_eos = 1;
        }

        // write data to packet
        mpp_packet_write(packet, 0, buf, read_size);
        // reset pos
        mpp_packet_set_pos(packet, buf);
        // setup eos flag
        if (found_eos)
            mpp_packet_set_eos(packet);

        ret = mpi->decode_put_packet(ctx, packet);
        if (MPP_OK != ret) {
            goto MPP_TEST_OUT;
        }

        msleep(50);

        ret = mpi->decode_get_frame(ctx, &frame);
        if (MPP_OK != ret) {
            goto MPP_TEST_OUT;
        }

        if (frame) {
            if (mpp_frame_get_info_change(frame)) {
                mpp_log("decode_get_frame get info changed found\n");
                mpi->control(ctx, MPP_CODEC_SET_INFO_CHANGE_READY, NULL);
            } else {
                if (fp_output)
                    dump_mpp_frame_to_file(frame, fp_output);
            }
            mpp_frame_deinit(&frame);
        }
    }

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

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (buf) {
        mpp_free(buf);
        buf = NULL;
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
        mpp_log("mpi_dec_test success\n");
    else
        mpp_err("mpi_dec_test failed ret %d\n", ret);

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

    mpi_dec_test(cmd);

    mpp_env_set_u32("mpi_debug", 0x0);
    return 0;
}

