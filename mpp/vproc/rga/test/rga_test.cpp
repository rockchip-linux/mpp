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

#include <string.h>

#include "rk_type.h"
#include "mpp_log.h"
#include "mpp_frame.h"
#include "mpp_common.h"
#include "mpp_err.h"

#include "utils.h"
#include "rga_api.h"

#define MAX_NAME_LENGTH 256

typedef struct RgaTestCmd_t {
    RK_U32         src_w;
    RK_U32         src_h;
    RK_U32         dst_w;
    RK_U32         dst_h;
    MppFrameFormat src_fmt;
    MppFrameFormat dst_fmt;
    char           input_file[MAX_NAME_LENGTH];
    char           output_file[MAX_NAME_LENGTH];
    RK_U32         have_input;
    RK_U32         have_output;
} RgaTestCmd;

void usage()
{
    mpp_log("usage:./rga_test -i input -o output_file"
            " -w src_w -h src_h -f src_fmt -dst_w dst_w"
            " -dst_h dst_h -dst_fmt dst_fmt\n");
}

static RK_S32 rga_test_parse_options(int argc, char **argv, RgaTestCmd *cmd)
{
    const char *opt;
    const char *next;
    RK_S32 optindex = 1;
    RK_S32 handleoptions = 1;
    RK_S32 err = MPP_NOK;

    if (argc < 2 || cmd == NULL) {
        err = 1;
        return err;
    }

    while (optindex < argc) {
        opt = (const char *) argv[optindex++];
        next = (const char *) argv[optindex];

        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
            if (opt[1] == '-') {
                if (opt[2] != '\0') {
                    opt++;
                } else {
                    handleoptions = 0;
                    break;
                }
            }

            opt++;

            switch (*opt) {
            case 'i' :
                if (next) {
                    strncpy(cmd->input_file, next, MAX_NAME_LENGTH);
                    cmd->input_file[strlen(next)] = '\0';
                    cmd->have_input = 1;
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_ERR;
                }
                break;
            case 'o' :
                if (next) {
                    strncpy(cmd->output_file, next, MAX_NAME_LENGTH);
                    cmd->output_file[strlen(next)] = '\0';
                    cmd->have_output = 1;
                } else {
                    mpp_err("output file is invalid\n");
                    goto PARSE_ERR;
                }
                break;
            case 'w' :
                if (next) {
                    cmd->src_w = atoi(next);
                } else {
                    mpp_err("src width is invalid\n");
                    goto PARSE_ERR;
                }
                break;
            case 'h' :
                if (next) {
                    cmd->src_h = atoi(next);
                } else {
                    mpp_err("src height is invalid\n");
                    goto PARSE_ERR;
                }
                break;
            case 'f' :
                if (next) {
                    cmd->src_fmt = (MppFrameFormat) atoi(next);
                    err = ((cmd->src_fmt >= MPP_FMT_YUV_BUTT && cmd->src_fmt < MPP_FRAME_FMT_RGB) ||
                           cmd->src_fmt >= MPP_FMT_RGB_BUTT);
                } else {
                    mpp_err("src fmt is invalid\n");
                    goto PARSE_ERR;
                }
                break;
            case 'd' :
                if ((*(opt + 1) != '\0') && !strncmp(opt, "dst_w", 5)) {
                    cmd->dst_w = atoi(next);
                } else if ((*(opt + 1) != '\0') && !strncmp(opt, "dst_h", 5)) {
                    cmd->dst_h = atoi(next);
                } else if ((*(opt + 1) != '\0') && !strncmp(opt, "dst_fmt", 5)) {
                    cmd->dst_fmt = (MppFrameFormat) atoi(next);
                } else {
                    mpp_err("dst parameters is invalid\n");
                    goto PARSE_ERR;
                }
                break;
            }
        }
    }
PARSE_ERR:
    return err;
}

int main(int argc, char **argv)
{
    mpp_log("rga test unit\n");

    MPP_RET ret = MPP_NOK;
    RgaTestCmd cmd;
    MppBuffer src_buf = NULL;
    MppBuffer dst_buf = NULL;
    MppFrame src_frm = NULL;
    MppFrame dst_frm = NULL;
    FILE *fin = NULL;
    FILE *fout = NULL;
    RgaCtx ctx = NULL;
    RK_U32 frame_count = 0;

    void *ptr;
    RK_U32 src_w;
    RK_U32 src_h;
    RK_U32 dst_w;
    RK_U32 dst_h;
    RK_U32 src_size;
    RK_U32 dst_size;

    memset(&cmd, 0, sizeof(cmd));
    if (rga_test_parse_options(argc, argv, &cmd)) {
        usage();
        goto END;
    }

    mpp_log("src w:%d h:%d fmt:%d dst w:%d h:%d fmt:%d input:%s output:%s\n",
            cmd.src_w, cmd.src_h, cmd.src_fmt,
            cmd.dst_w, cmd.dst_h, cmd.dst_fmt,
            cmd.input_file, cmd.output_file);

    src_w = cmd.src_w;
    src_h = cmd.src_h;
    dst_w = cmd.dst_w;
    dst_h = cmd.dst_h;
    src_size = src_w * src_h * 4;
    dst_size = dst_w * dst_h * 4;

    if (cmd.have_input) {
        fin = fopen(cmd.input_file, "r");
        if (!fin) {
            mpp_log("open input file %s failed\n", cmd.input_file);
            goto END;
        }
    }

    if (cmd.have_output) {
        fout = fopen(cmd.output_file, "w+");
        if (!fout) {
            mpp_log("open output file %s failed\n", cmd.output_file);
            goto END;
        }
    }

    ret = mpp_buffer_get(NULL, &src_buf, src_size);
    if (ret) {
        mpp_err("failed to get src buffer %d with size %d\n", ret, src_size);
        goto END;
    }

    ret = mpp_buffer_get(NULL, &dst_buf, dst_size);
    if (ret) {
        mpp_err("failed to get dst buffer %d with size %d\n", ret, dst_size);
        goto END;
    }

    ret = mpp_frame_init(&src_frm);
    if (ret) {
        mpp_err("failed to init src frame\n");
        goto END;
    }

    ret = mpp_frame_init(&dst_frm);
    if (ret) {
        mpp_err("failed to init dst frame\n");
        goto END;
    }

    ptr = mpp_buffer_get_ptr(src_buf);
    if (cmd.have_input) {
        ret = read_yuv_image((RK_U8 *)ptr, fin, cmd.src_w, cmd.src_h,
                             cmd.src_w, cmd.src_h, cmd.dst_fmt);
        if (ret) {
            mpp_err("failed to read input file ret:%d\n", ret);
            goto END;
        }
    } else {
        ret = fill_yuv_image((RK_U8 *)ptr, cmd.src_w, cmd.src_h,
                             cmd.src_w, cmd.src_h, cmd.src_fmt, frame_count);
        if (ret) {
            mpp_err("failed to fill input buffer ret:%d\n", ret);
            goto END;
        }
    }

    mpp_frame_set_buffer(src_frm, src_buf);
    mpp_frame_set_width(src_frm, src_w);
    mpp_frame_set_height(src_frm, src_h);
    mpp_frame_set_hor_stride(src_frm, MPP_ALIGN(src_w, 16));
    mpp_frame_set_ver_stride(src_frm, MPP_ALIGN(src_h, 16));
    mpp_frame_set_fmt(src_frm, cmd.src_fmt);

    mpp_frame_set_buffer(dst_frm, dst_buf);
    mpp_frame_set_width(dst_frm, dst_w);
    mpp_frame_set_height(dst_frm, dst_h);
    mpp_frame_set_hor_stride(dst_frm, MPP_ALIGN(dst_w, 16));
    mpp_frame_set_ver_stride(dst_frm, MPP_ALIGN(dst_h, 16));
    mpp_frame_set_fmt(dst_frm, cmd.dst_fmt);

    ret = rga_init(&ctx);
    if (ret) {
        mpp_err("init rga context failed %d\n", ret);
        goto END;
    }

    // start copy process
    ret = rga_control(ctx, RGA_CMD_INIT, NULL);
    if (ret) {
        mpp_err("rga cmd init failed %d\n", ret);
        goto END;
    }

    ret = rga_control(ctx, RGA_CMD_SET_SRC, src_frm);
    if (ret) {
        mpp_err("rga cmd setup source failed %d\n", ret);
        goto END;
    }

    ret = rga_control(ctx, RGA_CMD_SET_DST, dst_frm);
    if (ret) {
        mpp_err("rga cmd setup destination failed %d\n", ret);
        goto END;
    }

    ret = rga_control(ctx, RGA_CMD_RUN_SYNC, NULL);
    if (ret) {
        mpp_err("rga cmd process copy failed %d\n", ret);
        goto END;
    }

    // start field duplicate process
    mpp_frame_set_buffer(src_frm, dst_buf);
    mpp_frame_set_width(src_frm, dst_w);
    mpp_frame_set_height(src_frm, dst_h / 2);
    mpp_frame_set_hor_stride(src_frm, MPP_ALIGN(dst_w, 16) * 2);
    mpp_frame_set_ver_stride(src_frm, MPP_ALIGN(src_h, 16) / 2);
    mpp_frame_set_fmt(src_frm, cmd.dst_fmt);

    ret = rga_control(ctx, RGA_CMD_INIT, NULL);
    if (ret) {
        mpp_err("rga cmd init failed %d\n", ret);
        goto END;
    }

    ret = rga_control(ctx, RGA_CMD_SET_SRC, src_frm);
    if (ret) {
        mpp_err("rga cmd setup source failed %d\n", ret);
        goto END;
    }

    ret = rga_control(ctx, RGA_CMD_SET_DST, dst_frm);
    if (ret) {
        mpp_err("rga cmd setup destination failed %d\n", ret);
        goto END;
    }

    ret = rga_control(ctx, RGA_CMD_RUN_SYNC, NULL);
    if (ret) {
        mpp_err("rga cmd process copy failed %d\n", ret);
        goto END;
    }

    ret = rga_deinit(ctx);
    if (ret) {
        mpp_err("deinit rga context failed %d\n", ret);
        goto END;
    }

    if (cmd.have_output)
        dump_mpp_frame_to_file(dst_frm, fout);

END:
    if (src_frm)
        mpp_frame_deinit(&src_frm);

    if (dst_frm)
        mpp_frame_deinit(&dst_frm);

    if (src_buf)
        mpp_buffer_put(src_buf);

    if (dst_buf)
        mpp_buffer_put(dst_buf);

    if (fin)
        fclose(fin);

    if (fout)
        fclose(fout);

    mpp_log("rga test exit ret %d\n", ret);
    return 0;
}
