/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpi_dec_utils"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_buffer.h"

#include "rk_mpi.h"
#include "utils.h"
#include "mpp_common.h"
#include "mpi_dec_utils.h"

OptionInfo mpi_dec_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"c",               "ops_file",             "input operation config file"},
    {"w",               "width",                "the width of input bitstream"},
    {"h",               "height",               "the height of input bitstream"},
    {"t",               "type",                 "input stream coding type"},
    {"f",               "format",               "output frame format type"},
    {"d",               "debug",                "debug flag"},
    {"x",               "timeout",              "output timeout interval"},
    {"n",               "frame_number",         "max output frame number"},
    {"s",               "instance_nb",          "number of instances"},
    {NULL},
};

void mpi_dec_test_help()
{
    mpp_log("usage: mpi_dec_test [options]\n");
    show_options(mpi_dec_cmd);
    mpp_show_support_format();
}

RK_S32 mpi_dec_test_parse_options(int argc, char **argv, MpiDecTestCmd* cmd)
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
                    strncpy(cmd->file_input, next, MAX_FILE_NAME_LENGTH - 1);
                    cmd->have_input = 1;
                    name_to_coding_type(cmd->file_input, &cmd->type);
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'o':
                if (next) {
                    strncpy(cmd->file_output, next, MAX_FILE_NAME_LENGTH - 1);
                    cmd->have_output = 1;
                } else {
                    mpp_log("output file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'c':
                if (next) {
                    strncpy(cmd->file_config, next, MAX_FILE_NAME_LENGTH - 1);
                    cmd->have_config = 1;

                    // enlarge packet buffer size for large input stream case
                    cmd->pkt_size = SZ_1M;
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
            case 'f':
                if (next) {
                    cmd->format = (MppFrameFormat)atoi(next);
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
            case 'n':
                if (next) {
                    cmd->frame_num = atoi(next);
                    if (cmd->frame_num < 0)
                        mpp_log("infinite loop decoding mode\n");
                } else {
                    mpp_err("invalid frame number\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 's':
                if (next) {
                    cmd->nthreads = atoi(next);
                }
                if (!next || cmd->nthreads < 0) {
                    mpp_err("invalid nthreads\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            default:
                mpp_err("skip invalid opt %c\n", *opt);
                break;
            }

            optindex++;
        }
    }

    err = 0;

PARSE_OPINIONS_OUT:
    return err;
}

void mpi_dec_test_show_options(MpiDecTestCmd* cmd)
{
    mpp_log("cmd parse result:\n");
    mpp_log("input  file name: %s\n", cmd->file_input);
    mpp_log("output file name: %s\n", cmd->file_output);
    mpp_log("config file name: %s\n", cmd->file_config);
    mpp_log("width      : %4d\n", cmd->width);
    mpp_log("height     : %4d\n", cmd->height);
    mpp_log("type       : %d\n", cmd->type);
    mpp_log("debug flag : %x\n", cmd->debug);
    mpp_log("max frames : %d\n", cmd->frame_num);
}