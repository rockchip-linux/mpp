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

#define MODULE_TAG "mpi_enc_utils"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_buffer.h"

#include "rk_mpi.h"
#include "utils.h"
#include "mpp_common.h"
#include "mpi_enc_utils.h"

#define MAX_FILE_NAME_LENGTH        256

RK_S32 mpi_enc_width_default_stride(RK_S32 width, MppFrameFormat fmt)
{
    RK_S32 stride = 0;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP :
    case MPP_FMT_YUV420SP_VU : {
        stride = MPP_ALIGN(width, 8);
    } break;
    case MPP_FMT_YUV420P : {
        /* NOTE: 420P need to align to 16 so chroma can align to 8 */
        stride = MPP_ALIGN(width, 16);
    } break;
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP:
    case MPP_FMT_YUV422SP_VU: {
        /* NOTE: 422 need to align to 8 so chroma can align to 16 */
        stride = MPP_ALIGN(width, 8);
    } break;
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY : {
        /* NOTE: for vepu limitation */
        stride = MPP_ALIGN(width, 8) * 2;
    } break;
    case MPP_FMT_RGB888 :
    case MPP_FMT_BGR888 : {
        /* NOTE: for vepu limitation */
        stride = MPP_ALIGN(width, 8) * 3;
    } break;
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        /* NOTE: for vepu limitation */
        stride = MPP_ALIGN(width, 8) * 4;
    } break;
    default : {
        mpp_err_f("do not support type %d\n", fmt);
    } break;
    }

    return stride;
}

MpiEncTestArgs *mpi_enc_test_cmd_get(void)
{
    MpiEncTestArgs *args = mpp_calloc(MpiEncTestArgs, 1);

    return args;
}

MPP_RET mpi_enc_test_cmd_update_by_args(MpiEncTestArgs* cmd, int argc, char **argv)
{
    const char *opt;
    const char *next;
    RK_S32 optindex = 1;
    RK_S32 handleoptions = 1;
    MPP_RET ret = MPP_NOK;

    if ((argc < 2) || (cmd == NULL))
        return ret;

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
            case 'i' : {
                if (next) {
                    size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
                    if (len) {
                        cmd->file_input = mpp_calloc(char, len + 1);
                        strcpy(cmd->file_input, next);
                        name_to_frame_format(cmd->file_input, &cmd->format);
                    }
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'o' : {
                if (next) {
                    size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
                    if (len) {
                        cmd->file_output = mpp_calloc(char, len + 1);
                        strcpy(cmd->file_output, next);
                        name_to_coding_type(cmd->file_output, &cmd->type);
                    }
                } else {
                    mpp_log("output file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'w' : {
                if (next) {
                    cmd->width = atoi(next);
                } else {
                    mpp_err("invalid input width\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'h' : {
                if (!next)
                    goto PARSE_OPINIONS_OUT;

                if ((*(opt + 1) != '\0') && !strncmp(opt, "help", 4)) {
                    goto PARSE_OPINIONS_OUT;
                } else if (next) {
                    cmd->height = atoi(next);
                }
            } break;
            case 'u' : {
                if (next) {
                    cmd->hor_stride = atoi(next);
                } else {
                    mpp_err("invalid input width\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'v' : {
                if (next) {
                    cmd->ver_stride = atoi(next);
                } else {
                    mpp_log("input height is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'f' : {
                if (next) {
                    if (strstr(next, "x") || strstr(next, "X")) {
                        /* hex value with 0x prefix, use sscanf */
                        sscanf(next, "0x%x", &cmd->format);
                    } else if (strstr(next, "a") || strstr(next, "A") ||
                               strstr(next, "b") || strstr(next, "B") ||
                               strstr(next, "c") || strstr(next, "C") ||
                               strstr(next, "d") || strstr(next, "D") ||
                               strstr(next, "e") || strstr(next, "E") ||
                               strstr(next, "f") || strstr(next, "F")) {
                        /* hex value without 0x prefix, use sscanf */
                        sscanf(next, "%x", &cmd->format);
                    } else {
                        /* decimal value, use atoi */
                        cmd->format = (MppFrameFormat)atoi(next);
                    }
                    ret = (!MPP_FRAME_FMT_IS_LE(cmd->format)) && ((cmd->format >= MPP_FMT_YUV_BUTT && cmd->format < MPP_FRAME_FMT_RGB) ||
                                                                  cmd->format >= MPP_FMT_RGB_BUTT);
                }

                if (!next || ret) {
                    mpp_err("invalid input format %d\n", cmd->format);
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 't' : {
                if (next) {
                    cmd->type = (MppCodingType)atoi(next);
                    ret = mpp_check_support_format(MPP_CTX_ENC, cmd->type);
                }

                if (!next || ret) {
                    mpp_err("invalid input coding type\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'n' : {
                if (next) {
                    cmd->num_frames = atoi(next);
                } else {
                    mpp_err("invalid input max number of frames\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'g' : {
                RK_S32 cnt = 0;

                if (next)
                    cnt = sscanf(next, "%d:%d:%d",
                                 &cmd->gop_mode, &cmd->gop_len, &cmd->vi_len);

                if (!cnt) {
                    mpp_err("invalid gop mode use -g gop_mode:gop_len:vi_len\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'b' : {
                RK_S32 cnt = 0;

                if (next)
                    cnt = sscanf(next, "%d:%d:%d:%d",
                                 &cmd->bps_target, &cmd->bps_min, &cmd->bps_max,
                                 &cmd->rc_mode);

                if (!cnt) {
                    mpp_err("invalid bit rate usage -b bps_target:bps_min:bps_max:rc_mode\n");
                    mpp_err("rc_mode 0:vbr 1:cbr 2:avbr 3:cvbr 4:fixqp\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'r' : {
                if (next) {
                    RK_S32 num = sscanf(next, "%d:%d:%d/%d:%d:%d",
                                        &cmd->fps_in_num, &cmd->fps_in_den, &cmd->fps_in_flex,
                                        &cmd->fps_out_num, &cmd->fps_out_den, &cmd->fps_out_flex);
                    switch (num) {
                    case 1 : {
                        cmd->fps_out_num = cmd->fps_in_num;
                        cmd->fps_out_den = cmd->fps_in_den = 1;
                        cmd->fps_out_flex = cmd->fps_in_flex = 0;
                    } break;
                    case 2 : {
                        cmd->fps_out_num = cmd->fps_in_num;
                        cmd->fps_out_den = cmd->fps_in_den;
                        cmd->fps_out_flex = cmd->fps_in_flex = 0;
                    } break;
                    case 3 : {
                        cmd->fps_out_num = cmd->fps_in_num;
                        cmd->fps_out_den = cmd->fps_in_den;
                        cmd->fps_out_flex = cmd->fps_in_flex;
                    } break;
                    case 4 : {
                        cmd->fps_out_den = 1;
                        cmd->fps_out_flex = 0;
                    } break;
                    case 5 : {
                        cmd->fps_out_flex = 0;
                    } break;
                    case 6 : {
                    } break;
                    default : {
                        mpp_err("invalid in/out frame rate,"
                                " use \"-r numerator:denominator:flex\""
                                " for set the input to the same fps as the output, such as 50:1:1\n"
                                " or \"-r numerator:denominator/flex-numerator:denominator:flex\""
                                " for set input and output separately, such as 40:1:1/30:1:0\n");
                        goto PARSE_OPINIONS_OUT;
                    } break;
                    }
                } else {
                    mpp_err("invalid output frame rate\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'l' : {
                if (next) {
                    cmd->loop_cnt = atoi(next);
                } else {
                    mpp_err("invalid loop count\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            case 'x': {
                if (next) {
                    size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
                    if (len) {
                        cmd->file_cfg = mpp_calloc(char, len + 1);
                        strncpy(cmd->file_cfg, next, len);
                        cmd->cfg_ini = iniparser_load(cmd->file_cfg);
                    }
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
            } break;
            default : {
                mpp_err("skip invalid opt %c\n", *opt);
            } break;
            }

            optindex++;
        }
    }

    ret = MPP_OK;

    /* check essential parameter */
    if (cmd->type <= MPP_VIDEO_CodingAutoDetect) {
        mpp_err("invalid type %d\n", cmd->type);
        ret = MPP_NOK;
    }

    if (!cmd->hor_stride)
        cmd->hor_stride = mpi_enc_width_default_stride(cmd->width, cmd->format);
    if (!cmd->ver_stride)
        cmd->ver_stride = cmd->height;

    if (cmd->width <= 0 || cmd->height <= 0 ||
        cmd->hor_stride <= 0 || cmd->ver_stride <= 0) {
        mpp_err("invalid w:h [%d:%d] stride [%d:%d]\n",
                cmd->width, cmd->height, cmd->hor_stride, cmd->ver_stride);
        ret = MPP_NOK;
    }

PARSE_OPINIONS_OUT:
    return ret;
}

MPP_RET mpi_enc_test_cmd_put(MpiEncTestArgs* cmd)
{
    if (NULL == cmd)
        return MPP_OK;

    if (cmd->cfg_ini) {
        iniparser_freedict(cmd->cfg_ini);
        cmd->cfg_ini = NULL;
    }

    MPP_FREE(cmd->file_input);
    MPP_FREE(cmd->file_output);
    MPP_FREE(cmd->file_cfg);
    MPP_FREE(cmd);

    return MPP_OK;
}

MPP_RET mpi_enc_gen_ref_cfg(MppEncRefCfg ref, RK_S32 gop_mode)
{
    MppEncRefLtFrmCfg lt_ref[4];
    MppEncRefStFrmCfg st_ref[16];
    RK_S32 lt_cnt = 0;
    RK_S32 st_cnt = 0;
    MPP_RET ret = MPP_OK;

    memset(&lt_ref, 0, sizeof(lt_ref));
    memset(&st_ref, 0, sizeof(st_ref));

    switch (gop_mode) {
    case 3 : {
        // tsvc4
        //      /-> P1      /-> P3        /-> P5      /-> P7
        //     /           /             /           /
        //    //--------> P2            //--------> P6
        //   //                        //
        //  ///---------------------> P4
        // ///
        // P0 ------------------------------------------------> P8
        lt_cnt = 1;

        /* set 8 frame lt-ref gap */
        lt_ref[0].lt_idx        = 0;
        lt_ref[0].temporal_id   = 0;
        lt_ref[0].ref_mode      = REF_TO_PREV_LT_REF;
        lt_ref[0].lt_gap        = 8;
        lt_ref[0].lt_delay      = 0;

        st_cnt = 9;
        /* set tsvc4 st-ref struct */
        /* st 0 layer 0 - ref */
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;
        /* st 1 layer 3 - non-ref */
        st_ref[1].is_non_ref    = 1;
        st_ref[1].temporal_id   = 3;
        st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[1].ref_arg       = 0;
        st_ref[1].repeat        = 0;
        /* st 2 layer 2 - ref */
        st_ref[2].is_non_ref    = 0;
        st_ref[2].temporal_id   = 2;
        st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[2].ref_arg       = 0;
        st_ref[2].repeat        = 0;
        /* st 3 layer 3 - non-ref */
        st_ref[3].is_non_ref    = 1;
        st_ref[3].temporal_id   = 3;
        st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[3].ref_arg       = 0;
        st_ref[3].repeat        = 0;
        /* st 4 layer 1 - ref */
        st_ref[4].is_non_ref    = 0;
        st_ref[4].temporal_id   = 1;
        st_ref[4].ref_mode      = REF_TO_PREV_LT_REF;
        st_ref[4].ref_arg       = 0;
        st_ref[4].repeat        = 0;
        /* st 5 layer 3 - non-ref */
        st_ref[5].is_non_ref    = 1;
        st_ref[5].temporal_id   = 3;
        st_ref[5].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[5].ref_arg       = 0;
        st_ref[5].repeat        = 0;
        /* st 6 layer 2 - ref */
        st_ref[6].is_non_ref    = 0;
        st_ref[6].temporal_id   = 2;
        st_ref[6].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[6].ref_arg       = 0;
        st_ref[6].repeat        = 0;
        /* st 7 layer 3 - non-ref */
        st_ref[7].is_non_ref    = 1;
        st_ref[7].temporal_id   = 3;
        st_ref[7].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[7].ref_arg       = 0;
        st_ref[7].repeat        = 0;
        /* st 8 layer 0 - ref */
        st_ref[8].is_non_ref    = 0;
        st_ref[8].temporal_id   = 0;
        st_ref[8].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[8].ref_arg       = 0;
        st_ref[8].repeat        = 0;
    } break;
    case 2 : {
        // tsvc3
        //     /-> P1      /-> P3
        //    /           /
        //   //--------> P2
        //  //
        // P0/---------------------> P4
        lt_cnt = 0;

        st_cnt = 5;
        /* set tsvc4 st-ref struct */
        /* st 0 layer 0 - ref */
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;
        /* st 1 layer 2 - non-ref */
        st_ref[1].is_non_ref    = 1;
        st_ref[1].temporal_id   = 2;
        st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[1].ref_arg       = 0;
        st_ref[1].repeat        = 0;
        /* st 2 layer 1 - ref */
        st_ref[2].is_non_ref    = 0;
        st_ref[2].temporal_id   = 1;
        st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[2].ref_arg       = 0;
        st_ref[2].repeat        = 0;
        /* st 3 layer 2 - non-ref */
        st_ref[3].is_non_ref    = 1;
        st_ref[3].temporal_id   = 2;
        st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[3].ref_arg       = 0;
        st_ref[3].repeat        = 0;
        /* st 4 layer 0 - ref */
        st_ref[4].is_non_ref    = 0;
        st_ref[4].temporal_id   = 0;
        st_ref[4].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[4].ref_arg       = 0;
        st_ref[4].repeat        = 0;
    } break;
    case 1 : {
        // tsvc2
        //   /-> P1
        //  /
        // P0--------> P2
        lt_cnt = 0;

        st_cnt = 3;
        /* set tsvc4 st-ref struct */
        /* st 0 layer 0 - ref */
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;
        /* st 1 layer 2 - non-ref */
        st_ref[1].is_non_ref    = 1;
        st_ref[1].temporal_id   = 1;
        st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[1].ref_arg       = 0;
        st_ref[1].repeat        = 0;
        /* st 2 layer 1 - ref */
        st_ref[2].is_non_ref    = 0;
        st_ref[2].temporal_id   = 0;
        st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[2].ref_arg       = 0;
        st_ref[2].repeat        = 0;
    } break;
    default : {
        mpp_err_f("unsupport gop mode %d\n", gop_mode);
    } break;
    }

    if (lt_cnt || st_cnt) {
        ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

        if (lt_cnt)
            ret = mpp_enc_ref_cfg_add_lt_cfg(ref, lt_cnt, lt_ref);

        if (st_cnt)
            ret = mpp_enc_ref_cfg_add_st_cfg(ref, st_cnt, st_ref);

        /* check and get dpb size */
        ret = mpp_enc_ref_cfg_check(ref);
    }

    return ret;
}

MPP_RET mpi_enc_gen_smart_gop_ref_cfg(MppEncRefCfg ref, RK_S32 gop_len, RK_S32 vi_len)
{
    MppEncRefLtFrmCfg lt_ref[4];
    MppEncRefStFrmCfg st_ref[16];
    RK_S32 lt_cnt = 1;
    RK_S32 st_cnt = 8;
    RK_S32 pos = 0;
    MPP_RET ret = MPP_OK;

    memset(&lt_ref, 0, sizeof(lt_ref));
    memset(&st_ref, 0, sizeof(st_ref));

    ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

    /* set 8 frame lt-ref gap */
    lt_ref[0].lt_idx        = 0;
    lt_ref[0].temporal_id   = 0;
    lt_ref[0].ref_mode      = REF_TO_PREV_LT_REF;
    lt_ref[0].lt_gap        = gop_len;
    lt_ref[0].lt_delay      = 0;

    ret = mpp_enc_ref_cfg_add_lt_cfg(ref, 1, lt_ref);

    /* st 0 layer 0 - ref */
    st_ref[pos].is_non_ref  = 0;
    st_ref[pos].temporal_id = 0;
    st_ref[pos].ref_mode    = REF_TO_PREV_INTRA;
    st_ref[pos].ref_arg     = 0;
    st_ref[pos].repeat      = 0;
    pos++;

    /* st 1 layer 1 - non-ref */
    if (vi_len > 1) {
        st_ref[pos].is_non_ref  = 0;
        st_ref[pos].temporal_id = 1;
        st_ref[pos].ref_mode    = REF_TO_PREV_REF_FRM;
        st_ref[pos].ref_arg     = 0;
        st_ref[pos].repeat      = vi_len - 2;
        pos++;
    }

    st_ref[pos].is_non_ref  = 0;
    st_ref[pos].temporal_id = 0;
    st_ref[pos].ref_mode    = REF_TO_PREV_INTRA;
    st_ref[pos].ref_arg     = 0;
    st_ref[pos].repeat      = 0;
    pos++;

    ret = mpp_enc_ref_cfg_add_st_cfg(ref, pos, st_ref);

    /* check and get dpb size */
    ret = mpp_enc_ref_cfg_check(ref);

    return ret;
}

MPP_RET mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 frame_cnt)
{
    /*
     * osd idx size range from 16x16 bytes(pixels) to hor_stride*ver_stride(bytes).
     * for general use, 1/8 Y buffer is enough.
     */
    static RK_U32 plt_table[8] = {
        MPP_ENC_OSD_PLT_RED,
        MPP_ENC_OSD_PLT_YELLOW,
        MPP_ENC_OSD_PLT_BLUE,
        MPP_ENC_OSD_PLT_GREEN,
        MPP_ENC_OSD_PLT_CYAN,
        MPP_ENC_OSD_PLT_TRANS,
        MPP_ENC_OSD_PLT_BLACK,
        MPP_ENC_OSD_PLT_WHITE,
    };

    if (osd_plt) {
        RK_U32 k = 0;
        RK_U32 base = frame_cnt & 7;

        for (k = 0; k < 256; k++)
            osd_plt->data[k].val = plt_table[(base + k) % 8];
    }
    return MPP_OK;
}

#define STEP_X  3
#define STEP_Y  2
#define STEP_W  2
#define STEP_H  2

MPP_RET mpi_enc_gen_osd_data(MppEncOSDData *osd_data, MppBufferGroup group,
                             RK_U32 width, RK_U32 height, RK_U32 frame_cnt)
{
    MppEncOSDRegion *region = NULL;
    RK_U32 k = 0;
    RK_U32 num_region = 8;
    RK_U32 buf_offset = 0;
    RK_U32 buf_size = 0;
    RK_U32 mb_w_max = MPP_ALIGN(width, 16) / 16;
    RK_U32 mb_h_max = MPP_ALIGN(height, 16) / 16;
    RK_U32 mb_x = (frame_cnt * STEP_X) % mb_w_max;
    RK_U32 mb_y = (frame_cnt * STEP_Y) % mb_h_max;
    RK_U32 mb_w = STEP_W;
    RK_U32 mb_h = STEP_H;
    MppBuffer buf = osd_data->buf;

    if (buf)
        buf_size = mpp_buffer_get_size(buf);

    /* generate osd region info */
    osd_data->num_region = num_region;

    region = osd_data->region;

    for (k = 0; k < num_region; k++, region++) {
        // NOTE: offset must be 16 byte aligned
        RK_U32 region_size = MPP_ALIGN(mb_w * mb_h * 256, 16);

        region->inverse = 1;
        region->start_mb_x = mb_x;
        region->start_mb_y = mb_y;
        region->num_mb_x = mb_w;
        region->num_mb_y = mb_h;
        region->buf_offset = buf_offset;
        region->enable = (mb_w && mb_h);

        buf_offset += region_size;

        mb_x += STEP_X;
        mb_y += STEP_Y;
        if (mb_x >= mb_w_max)
            mb_x -= mb_w_max;
        if (mb_y >= mb_h_max)
            mb_y -= mb_h_max;
    }

    /* create buffer and write osd index data */
    if (buf_size < buf_offset) {
        if (buf)
            mpp_buffer_put(buf);

        mpp_buffer_get(group, &buf, buf_offset);
        if (NULL == buf)
            mpp_err_f("failed to create osd buffer size %d\n", buf_offset);
    }

    if (buf) {
        void *ptr = mpp_buffer_get_ptr(buf);
        region = osd_data->region;

        for (k = 0; k < num_region; k++, region++) {
            mb_w = region->num_mb_x;
            mb_h = region->num_mb_y;
            buf_offset = region->buf_offset;

            memset(ptr + buf_offset, k, mb_w * mb_h * 256);
        }
    }

    osd_data->buf = buf;

    return MPP_OK;
}

static OptionInfo mpi_enc_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input picture"},
    {"h",               "height",               "the height of input picture"},
    {"f",               "format",               "the format of input picture"},
    {"t",               "type",                 "output stream coding type"},
    {"n",               "max frame number",     "max encoding frame number"},
    {"g",               "gop_mode",             "gop reference mode"},
    {"d",               "debug",                "debug flag"},
    {"b",               "bps target:min:max",   "set tareget bps"},
    {"r",               "in/output fps",        "set input and output frame rate"},
    {"l",               "loop count",           "loop encoding times for each frame"},
};

MPP_RET mpi_enc_test_cmd_show_opt(MpiEncTestArgs* cmd)
{
    mpp_log("cmd parse result:\n");
    mpp_log("input  file name: %s\n", cmd->file_input);
    mpp_log("output file name: %s\n", cmd->file_output);
    mpp_log("width      : %d\n", cmd->width);
    mpp_log("height     : %d\n", cmd->height);
    mpp_log("format     : %d\n", cmd->format);
    mpp_log("type       : %d\n", cmd->type);

    return MPP_OK;
}

void mpi_enc_test_help(void)
{
    mpp_log("usage: mpi_enc_test [options]\n");
    show_options(mpi_enc_cmd);
    mpp_show_support_format();
    mpp_show_color_format();
}
