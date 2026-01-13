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

#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_buffer.h"

#include "rk_mpi.h"
#include "mpp_common.h"
#include "mpp_rc_api.h"

#include "mpp_opt.h"
#include "mpi_enc_utils.h"

#define MAX_FILE_NAME_LENGTH        256

static RK_S32 qbias_arr_hevc[18] = {
    3, 6, 13, 171, 171, 171, 171,
    3, 6, 13, 171, 171, 220, 171, 85, 85, 85, 85
};

static RK_S32 qbias_arr_avc[18] = {
    3, 6, 13, 683, 683, 683, 683,
    3, 6, 13, 683, 683, 683, 683, 341, 341, 341, 341
};

static RK_S32 aq_rnge_arr[10] = {
    5, 5, 10, 12, 12,
    5, 5, 10, 12, 12
};

static RK_S32 aq_thd_smart[16] = {
    1,  3,  3,  3,  3,  3,  5,  5,
    8,  8,  8, 15, 15, 20, 25, 28
};

static RK_S32 aq_step_smart[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    0,  1,  2,  3,  4,  6,  8, 10
};

static RK_S32 aq_thd[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 25
};

static RK_S32 aq_step_i_ipc[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    5,  7,  7,  8,
};

static RK_S32 aq_step_p_ipc[16] = {
    -8, -7, -6, -5,
    -4, -2, -1, -1,
    0,  2,  3,  4,
    6,  8,  9,  10,
};

RK_S32 mpi_enc_width_default_stride(RK_S32 width, MppFrameFormat fmt)
{
    RK_S32 stride = 0;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV400 :
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
    case MPP_FMT_YUV444SP :
    case MPP_FMT_YUV444P : {
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

static RK_S32 mpi_enc_utils_load_file(MppEncTestObjSet *obj_set)
{
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    MppEncArgs cmd_obj = obj_set->cmd_obj;
    char *name = cmd->file_cfg;
    char *enc_cfg_pos = NULL;
    char *enc_args_pos = NULL;
    const char str_enc_cfg[] = "\"enc_cfg\" :";
    const char str_enc_args[] = "\"enc_args\" :";
    RK_S32 ret = MPP_NOK;
    RK_S32 fd = -1;
    RK_S32 size = 0;
    void *buf = NULL;

    fd = open(name, O_RDWR);
    if (fd < 0) {
        mpp_err_f("cfg file %s open failed for %s\n", name, strerror(errno));
        goto error;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        mpp_err_f("lseek %s failed for %s\n", name, strerror(errno));
        goto error;
    }

    lseek(fd, 0, SEEK_SET);
    mpp_logi("load cfg file %s size %d\n", name, size);

    buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (!buf) {
        mpp_err_f("mmap fd %d size %d failed\n", fd, size);
        goto error;
    }

    enc_cfg_pos = strstr(buf, str_enc_cfg);
    if (enc_cfg_pos != NULL) {
        enc_cfg_pos += strlen(str_enc_cfg);
        mpp_enc_cfg_apply(cfg_obj, MPP_CFG_STR_FMT_JSON, enc_cfg_pos);
        ret = MPP_OK;
    }

    enc_args_pos = strstr(buf, str_enc_args);
    if (enc_args_pos != NULL) {
        enc_args_pos += strlen(str_enc_args);
        mpp_enc_args_apply(cmd_obj, MPP_CFG_STR_FMT_JSON, enc_args_pos);
        ret = MPP_OK;
    }

error:
    if (buf) {
        munmap(buf, size);
        buf = NULL;
    }
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

static void mpi_enc_sync_cmd(MpiEncTestArgs *cmd, MppEncCfg cfg)
{
    mpp_enc_cfg_get_s32(cfg, "codec:type", (RK_S32 *)&cmd->type);
    mpp_enc_cfg_get_s32(cfg, "prep:width", &cmd->width);
    mpp_enc_cfg_get_s32(cfg, "prep:height", &cmd->height);
    mpp_enc_cfg_get_s32(cfg, "prep:hor_stride", &cmd->hor_stride);
    mpp_enc_cfg_get_s32(cfg, "prep:ver_stride", &cmd->ver_stride);
    mpp_enc_cfg_get_s32(cfg, "prep:format", (RK_S32 *)&cmd->format);
    mpp_enc_cfg_get_s32(cfg, "prep:mirroring", (RK_S32 *)&cmd->mirroring);
    mpp_enc_cfg_get_s32(cfg, "prep:rotation", (RK_S32 *)&cmd->rotation);
    mpp_enc_cfg_get_s32(cfg, "prep:flip", (RK_S32 *)&cmd->flip);
    mpp_enc_cfg_get_s32(cfg, "rc:mode", &cmd->rc_mode);
    mpp_enc_cfg_get_s32(cfg, "rc:gop", &cmd->gop_len);
    mpp_enc_cfg_get_s32(cfg, "rc:fps_in_flex", &cmd->fps_in_flex);
    mpp_enc_cfg_get_s32(cfg, "rc:fps_in_num", &cmd->fps_in_num);
    mpp_enc_cfg_get_s32(cfg, "rc:fps_in_denom", &cmd->fps_in_den);
    mpp_enc_cfg_get_s32(cfg, "rc:fps_out_flex", &cmd->fps_out_flex);
    mpp_enc_cfg_get_s32(cfg, "rc:fps_out_num", &cmd->fps_out_num);
    mpp_enc_cfg_get_s32(cfg, "rc:fps_out_denom", &cmd->fps_out_den);
    mpp_enc_cfg_get_s32(cfg, "rc:qp_init", &cmd->qp_init);
    mpp_enc_cfg_get_s32(cfg, "rc:qp_min", &cmd->qp_min);
    mpp_enc_cfg_get_s32(cfg, "rc:qp_max", &cmd->qp_max);
    mpp_enc_cfg_get_s32(cfg, "rc:qp_min_i", &cmd->qp_min_i);
    mpp_enc_cfg_get_s32(cfg, "rc:qp_max_i", &cmd->qp_max_i);
    mpp_enc_cfg_get_s32(cfg, "rc:fqp_min_i", &cmd->fqp_min_i);
    mpp_enc_cfg_get_s32(cfg, "rc:fqp_max_i", &cmd->fqp_max_i);
    mpp_enc_cfg_get_s32(cfg, "rc:fqp_min_p", &cmd->fqp_min_p);
    mpp_enc_cfg_get_s32(cfg, "rc:fqp_max_p", &cmd->fqp_max_p);
    mpp_enc_cfg_get_s32(cfg, "rc:bps_target", &cmd->bps_target);
    mpp_enc_cfg_get_s32(cfg, "rc:bps_max", &cmd->bps_max);
    mpp_enc_cfg_get_s32(cfg, "rc:bps_min", &cmd->bps_min);
    mpp_enc_cfg_get_s32(cfg, "tune:scene_mode", &cmd->scene_mode);
    mpp_enc_cfg_get_s32(cfg, "tune:deblur_en", &cmd->deblur_en);
    mpp_enc_cfg_get_s32(cfg, "h265:diff_cu_qp_delta_depth", &cmd->cu_qp_delta_depth);
    mpp_enc_cfg_get_s32(cfg, "split:mode", (RK_S32 *)&cmd->split_mode);
    mpp_enc_cfg_get_s32(cfg, "split:arg", (RK_S32 *)&cmd->split_arg);
    mpp_enc_cfg_get_s32(cfg, "split:out", (RK_S32 *)&cmd->split_out);
}

RK_S32 mpi_enc_opt_i(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;

    if (next) {
        size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
        if (len) {
            cmd->file_input = mpp_calloc(char, len + 1);
            strcpy(cmd->file_input, next);
            name_to_frame_format(cmd->file_input, &cmd->format);

            if (cmd->type_src == MPP_VIDEO_CodingUnused)
                name_to_coding_type(cmd->file_input, &cmd->type_src);
        }

        return 1;
    }

    mpp_err("input file is invalid\n");
    return 0;
}

RK_S32 mpi_enc_opt_o(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;

    if (next) {
        size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
        if (len) {
            cmd->file_output = mpp_calloc(char, len + 1);
            strcpy(cmd->file_output, next);
            name_to_coding_type(cmd->file_output, &cmd->type);
        }

        return 1;
    }

    mpp_log("output file is invalid\n");
    return 0;
}

RK_S32 mpi_enc_opt_w(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->width = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "prep:width", cmd->width);
        return 1;
    }

    mpp_err("invalid input width\n");
    return 0;
}

RK_S32 mpi_enc_opt_h(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->height = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "prep:height", cmd->height);
        return 1;
    }

    mpp_err("invalid input height\n");
    return 0;
}

RK_S32 mpi_enc_opt_hstride(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->hor_stride = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "prep:hor_stride", cmd->hor_stride);
        return 1;
    }

    mpp_err("invalid input horizontal stride\n");
    return 0;
}

RK_S32 mpi_enc_opt_vstride(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->ver_stride = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "prep:ver_stride", cmd->ver_stride);
        return 1;
    }

    mpp_err("invalid input vertical stride\n");
    return 0;
}

RK_S32 mpi_enc_opt_f(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        long number = 0;
        MppFrameFormat format = MPP_FMT_BUTT;

        if (MPP_OK == str_to_frm_fmt(next, &number)) {
            format = (MppFrameFormat)number;

            if (MPP_FRAME_FMT_IS_BE(format) &&
                (MPP_FRAME_FMT_IS_YUV(format) || MPP_FRAME_FMT_IS_RGB(format))) {
                cmd->format = format;
                if (cfg_obj)
                    mpp_enc_cfg_set_s32(cfg_obj, "prep:format", (RK_S32)cmd->format);
                return 1;
            }

            mpp_err("invalid input format 0x%x\n", format);
        }
    }

    cmd->format = MPP_FMT_YUV420SP;
    if (cfg_obj)
        mpp_enc_cfg_set_s32(cfg_obj, "prep:format", (RK_S32)cmd->format);

    return 0;
}

RK_S32 mpi_enc_opt_t(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    MppCodingType type = MPP_VIDEO_CodingUnused;

    if (next) {
        type = (MppCodingType)atoi(next);
        if (!mpp_check_support_format(MPP_CTX_ENC, type)) {
            cmd->type = type;
            if (cfg_obj)
                mpp_enc_cfg_set_s32(cfg_obj, "codec:type", (RK_S32)type);
        }
        return 1;
    }

    mpp_err("invalid input coding type %d\n", type);
    return 0;
}

RK_S32 mpi_enc_opt_tsrc(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppCodingType type = MPP_VIDEO_CodingUnused;

    if (next) {
        type = (MppCodingType)atoi(next);
        if (!mpp_check_support_format(MPP_CTX_DEC, type))
            cmd->type_src = type;
        return 1;
    }

    mpp_err("invalid input coding type %d\n", type);
    return 0;
}


RK_S32 mpi_enc_opt_n(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;

    if (next) {
        cmd->frame_num = atoi(next);
        return 1;
    }

    mpp_err("invalid input max number of frames\n");
    return 0;
}

RK_S32 mpi_enc_opt_g(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    RK_S32 cnt = 0;

    if (next) {
        cnt = sscanf(next, "%d:%d:%d",
                     &cmd->gop_mode, &cmd->gop_len, &cmd->vi_len);
        if (cnt) {
            if (cmd->gop_len) {
                if (cfg_obj)
                    mpp_enc_cfg_set_s32(cfg_obj, "rc:gop", cmd->gop_len);
            }

            return 1;
        }
    }

    mpp_err("invalid gop mode use -g gop_mode:gop_len:vi_len\n");
    return 0;
}

RK_S32 mpi_enc_opt_rc(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    RK_S32 cnt = 0;

    if (next) {
        cnt = sscanf(next, "%d", &cmd->rc_mode);
        if (cnt) {
            if (cfg_obj)
                mpp_enc_cfg_set_s32(cfg_obj, "rc:mode", cmd->rc_mode);
            return 1;
        }
    }

    mpp_err("invalid rate control usage -rc rc_mode\n");
    mpp_err("rc_mode 0:vbr 1:cbr 2:fixqp 3:avbr 4:smtrc\n");
    return 0;
}

RK_S32 mpi_enc_opt_bps(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    RK_S32 cnt = 0;

    if (next) {
        cnt = sscanf(next, "%d:%d:%d",
                     &cmd->bps_target, &cmd->bps_min, &cmd->bps_max);
        if (cnt) {
            if (cfg_obj) {
                mpp_enc_cfg_set_s32(cfg_obj, "rc:bps_target", cmd->bps_target);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:bps_min", cmd->bps_min);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:bps_max", cmd->bps_max);
            }
            return 1;
        }
    }

    mpp_err("invalid bit rate usage -bps bps_target:bps_min:bps_max\n");
    return 0;
}

RK_S32 mpi_enc_opt_fps(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    RK_S32 ret;

    if (next) {
        RK_U32 num = sscanf(next, "%d:%d:%d/%d:%d:%d",
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
                    " use \"-fps numerator:denominator:flex\""
                    " for set the input to the same fps as the output, such as 50:1:1\n"
                    " or \"-fps numerator:denominator:flex/numerator:denominator:flex\""
                    " for set input and output separately, such as 40:1:1/30:1:0\n");
        } break;
        }
        ret = (num > 0 && num <= 6) ? 1 : 0;

        if (cfg_obj && ret != 0) {
            mpp_enc_cfg_set_s32(cfg_obj, "rc:fps_in_num", cmd->fps_in_num);
            mpp_enc_cfg_set_s32(cfg_obj, "rc:fps_in_den", cmd->fps_in_den);
            mpp_enc_cfg_set_s32(cfg_obj, "rc:fps_in_flex", cmd->fps_in_flex);
            mpp_enc_cfg_set_s32(cfg_obj, "rc:fps_out_num", cmd->fps_out_num);
            mpp_enc_cfg_set_s32(cfg_obj, "rc:fps_out_den", cmd->fps_out_den);
            mpp_enc_cfg_set_s32(cfg_obj, "rc:fps_out_flex", cmd->fps_out_flex);
        }

        return ret;
    }

    mpp_err("invalid output frame rate\n");
    return 0;
}

RK_S32 mpi_enc_opt_qc(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    RK_S32 cnt = 0;

    if (next) {
        cnt = sscanf(next, "%d:%d:%d:%d:%d", &cmd->qp_init,
                     &cmd->qp_min, &cmd->qp_max, &cmd->qp_min_i, &cmd->qp_max_i);
        if (cnt) {
            if (cfg_obj) {
                mpp_enc_cfg_set_s32(cfg_obj, "rc:qp_init", cmd->qp_init);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:qp_min", cmd->qp_min);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:qp_max", cmd->qp_max);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:qp_min_i", cmd->qp_min_i);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:qp_max_i", cmd->qp_max_i);
            }

            return 1;
        }
    }

    mpp_err("invalid quality control usage -qc qp_init:min:max:min_i:max_i\n");
    return 0;
}

RK_S32 mpi_enc_opt_fqc(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;
    RK_S32 cnt = 0;

    if (next) {
        cnt = sscanf(next, "%d:%d:%d:%d", &cmd->fqp_min_i, &cmd->fqp_max_i,
                     &cmd->fqp_min_p, &cmd->fqp_max_p);
        if (cnt) {
            if (cfg_obj) {
                mpp_enc_cfg_set_s32(cfg_obj, "rc:fqp_min_i", cmd->fqp_min_i);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:fqp_max_i", cmd->fqp_max_i);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:fqp_min_p", cmd->fqp_min_p);
                mpp_enc_cfg_set_s32(cfg_obj, "rc:fqp_max_p", cmd->fqp_max_p);
            }

            return 1;
        }
    }

    mpp_err("invalid frame quality control usage -fqc min_i:max_i:min_p:max_p\n");
    return 0;
}

RK_S32 mpi_enc_opt_s(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;

    cmd->nthreads = -1;
    if (next) {
        cmd->nthreads = atoi(next);
        if (cmd->nthreads >= 1)
            return 1;
    }

    mpp_err("invalid nthreads %d\n", cmd->nthreads);
    cmd->nthreads = 1;
    return 0;
}

RK_S32 mpi_enc_opt_l(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;

    if (next) {
        cmd->loop_cnt = atoi(next);
        return 1;
    }

    mpp_err("invalid loop count\n");
    return 0;
}

RK_S32 mpi_enc_opt_v(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;

    if (next) {
        if (strstr(next, "q"))
            cmd->quiet = 1;
        if (strstr(next, "f"))
            cmd->trace_fps = 1;

        return 1;
    }

    return 0;
}

RK_S32 mpi_enc_opt_cfg(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = NULL;
    RK_S32 ret;

    if (next) {
        size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
        if (len) {
            if (cmd->file_cfg) {
                mpp_free(cmd->file_cfg);
                cmd->file_cfg = NULL;
            }

            cmd->file_cfg = mpp_calloc(char, len + 1);
            strncpy(cmd->file_cfg, next, len);

            if (obj_set->cfg_obj == NULL) {
                mpp_enc_cfg_init(&cfg_obj);
                obj_set->cfg_obj = cfg_obj;
            }

            ret = mpi_enc_utils_load_file(obj_set);
            if (ret)
                return -1;

            mpi_enc_sync_cmd(cmd, obj_set->cfg_obj);

            return 1;
        }
    }

    mpp_err("invalid encoder cfg file\n");

    return 0;
}

RK_S32 mpi_enc_opt_slt(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;

    if (next) {
        size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
        if (len) {
            cmd->file_slt = mpp_calloc(char, len + 1);
            strncpy(cmd->file_slt, next, len);

            return 1;
        }
    }

    mpp_err("input slt verify file is invalid\n");
    return 0;
}

RK_S32 mpi_enc_opt_step(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;

    if (next) {
        cmd->frm_step = atoi(next);
        return 1;
    }

    mpp_err("invalid input frame step\n");
    return 0;
}

RK_S32 mpi_enc_opt_sm(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->scene_mode = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:scene_mode", cmd->scene_mode);
        return 1;
    }

    mpp_err("invalid scene mode\n");
    return 0;
}

RK_S32 mpi_enc_opt_qpdd(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->cu_qp_delta_depth = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "h265:diff_cu_qp_delta_depth", cmd->cu_qp_delta_depth);
        return 1;
    }

    mpp_err("invalid cu_qp_delta_depth\n");
    return 0;
}

RK_S32 mpi_enc_opt_dbe(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->deblur_en = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:deblur_en", cmd->deblur_en);
        return 1;
    }

    mpp_err("invalid deblur en\n");
    return 0;
}

RK_S32 mpi_enc_opt_dbs(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->deblur_str = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:deblur_str", cmd->deblur_str);
        return 1;
    }

    mpp_err("invalid deblur str\n");

    return 0;
}

RK_S32 mpi_enc_opt_help(void *ctx, const char *next)
{
    (void)ctx;
    (void)next;
    return -1;
}

RK_S32 mpi_enc_opt_atf(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        RK_S32 val = atoi(next);

        if (val >= 0 && val <= 3 ) {
            cmd->anti_flicker_str = val;
            cmd->atf_str = val;
        } else {
            cmd->anti_flicker_str = 0;
            cmd->atf_str = 0;
            mpp_err("invalid atf_str %d set to default 0\n", val);
        }
        if (cfg_obj) {
            mpp_enc_cfg_set_s32(cfg_obj, "tune:atf_str", cmd->atf_str);
            mpp_enc_cfg_set_s32(cfg_obj, "tune:anti_flicker_str", cmd->anti_flicker_str);
        }

        return 1;
    }

    mpp_err("invalid cu_qp_delta_depth\n");
    return 0;
}

RK_S32 mpi_enc_opt_atl(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->atl_str = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:atl_str", cmd->atl_str);
        return 1;
    }

    mpp_err("invalid atl_str\n");
    return 0;
}

RK_S32 mpi_enc_opt_atr_i(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->atr_str_i = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:atr_str_i", cmd->atr_str_i);
        return 1;
    }

    mpp_err("invalid atr_str_i\n");
    return 0;
}

RK_S32 mpi_enc_opt_atr_p(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->atr_str_p = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:atr_str_p", cmd->atr_str_p);
        return 1;
    }

    mpp_err("invalid atr_str_p\n");
    return 0;
}

RK_S32 mpi_enc_opt_sao_i(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->sao_str_i = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:sao_str_i", cmd->sao_str_i);
        return 1;
    }

    mpp_err("invalid sao_str_i\n");
    return 0;
}

RK_S32 mpi_enc_opt_sao_p(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->sao_str_p = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:sao_str_p", cmd->sao_str_p);
        return 1;
    }

    mpp_err("invalid sao_str_p\n");
    return 0;
}

RK_S32 mpi_enc_opt_bc(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->rc_container = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:rc_container", cmd->rc_container);
        return 1;
    }

    mpp_err("invalid bitrate container\n");
    return 0;
}

RK_S32 mpi_enc_opt_bias_i(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->bias_i = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "hw:qbias_i", cmd->bias_i);
        return 1;
    }

    mpp_err("invalid bias i\n");
    return 0;
}

RK_S32 mpi_enc_opt_bias_p(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->bias_p = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "hw:qbias_p", cmd->bias_p);
        return 1;
    }

    mpp_err("invalid bias p\n");
    return 0;
}

RK_S32 mpi_enc_opt_lmd(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->lambda_idx_p = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:lambda_idx_p", cmd->lambda_idx_p);
        return 1;
    }

    mpp_err("invalid lambda idx\n");
    return 0;
}

RK_S32 mpi_enc_opt_lmdi(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->lambda_idx_i = atoi(next);
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:lambda_idx_i", cmd->lambda_idx_i);
        return 1;
    }

    mpp_err("invalid intra lambda idx\n");
    return 0;
}

RK_S32 mpi_enc_opt_speed(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;
    MppEncCfg cfg_obj = obj_set->cfg_obj;

    if (next) {
        cmd->speed = atoi(next);
        if (cmd->speed > 3 || cmd->speed < 0) {
            cmd->speed = 0;
            mpp_err("invalid speed %d set to default 0\n", cmd->speed);
        }
        if (cfg_obj)
            mpp_enc_cfg_set_s32(cfg_obj, "tune:speed", cmd->speed);
        return 1;
    }

    mpp_err("invalid speed mode\n");
    return 0;
}

RK_S32 mpi_enc_opt_kmpp(void *ctx, const char *next)
{
    MppEncTestObjSet* obj_set = (MppEncTestObjSet *)ctx;
    MpiEncTestArgs *cmd = (MpiEncTestArgs *)obj_set->cmd;

    if (next) {
        cmd->kmpp_en = atoi(next);
        if (cmd->kmpp_en) {
            if (access("/dev/vcodec", F_OK | R_OK | W_OK)) {
                mpp_err("failed to access /dev/vcodec, check kmpp devices\n");
                return -1;
            }
        }
        return 1;
    }

    mpp_err("invalid kmpp enable\n");
    return 0;
}

static MppOptInfo enc_opts[] = {
    {"i",       "input_file",           "input frame file",                         mpi_enc_opt_i},
    {"o",       "output_file",          "output encoded bitstream file",            mpi_enc_opt_o},
    {"w",       "width",                "the width of input picture",               mpi_enc_opt_w},
    {"h",       "height",               "the height of input picture",              mpi_enc_opt_h},
    {"hstride", "hor_stride",           "the horizontal stride of input picture",   mpi_enc_opt_hstride},
    {"vstride", "ver_stride",           "the vertical stride of input picture",     mpi_enc_opt_vstride},
    {"f",       "format",               "the format of input picture",              mpi_enc_opt_f},
    {"t",       "type",                 "output stream coding type",                mpi_enc_opt_t},
    {"tsrc",    "source type",          "input file source coding type",            mpi_enc_opt_tsrc},
    {"n",       "max frame number",     "max encoding frame number",                mpi_enc_opt_n},
    {"g",       "gop reference mode",   "gop_mode:gop_len:vi_len",                  mpi_enc_opt_g},
    {"rc",      "rate control mode",    "rc_mode, 0:vbr 1:cbr 2:fixqp 3:avbr 4:smtrc", mpi_enc_opt_rc},
    {"bps",     "bps target:min:max",   "set tareget:min:max bps",                  mpi_enc_opt_bps},
    {"fps",     "in/output fps",        "set input and output frame rate",          mpi_enc_opt_fps},
    {"qc",      "quality control",      "set qp_init:min:max:min_i:max_i",          mpi_enc_opt_qc},
    {"fqc",     "frame quality control", "set fqp min_i:max_i:min_p:max_p",         mpi_enc_opt_fqc},
    {"s",       "instance_nb",          "number of instances",                      mpi_enc_opt_s},
    {"v",       "trace option",         "q - quiet f - show fps",                   mpi_enc_opt_v},
    {"l",       "loop count",           "loop encoding times for each frame",       mpi_enc_opt_l},
    {"cfg",     "cfg_file",             "enc cfg file",                             mpi_enc_opt_cfg},
    {"slt",     "slt file",             "slt verify data file",                     mpi_enc_opt_slt},
    {"step",    "frame step",           "frame step, only for NV12 in slt test",    mpi_enc_opt_step},
    {"sm",      "scene mode",           "scene_mode, 0:default 1:ipc",              mpi_enc_opt_sm},
    {"qpdd",    "cu_qp_delta_depth",    "cu_qp_delta_depth, 0:1:2",                 mpi_enc_opt_qpdd},
    {"dbe",     "deblur enable",        "deblur_en or qpmap_en, 0:close 1:open",           mpi_enc_opt_dbe},
    {"dbs",     "deblur strength",      "deblur_str 0~3: hw + sw scheme; 4~7: hw scheme",  mpi_enc_opt_dbs},
    {"atf",     "anti_flicker_str",     "anti_flicker_str, 0:off 1 2 3",            mpi_enc_opt_atf},
    {"atl",     "atl_str",              "atl_str, 0:off 1 open",                    mpi_enc_opt_atl},
    {"atr_i",   "atr_str_i",            "atr_str_i, 0:off 1 2 3",                   mpi_enc_opt_atr_i},
    {"atr_p",   "atr_str_p",            "atr_str_p, 0:off 1 2 3",                   mpi_enc_opt_atr_p},
    {"sao_i",   "sao_str_i",            "sao_str_i, 0:off 1 2 3",                   mpi_enc_opt_sao_i},
    {"sao_p",   "sao_str_p",            "sao_str_p, 0:off 1 2 3",                   mpi_enc_opt_sao_p},
    {"bc",      "bitrate container",    "rc_container, 0:off 1:weak 2:strong",      mpi_enc_opt_bc},
    {"ibias",   "bias i",               "bias_i",                                   mpi_enc_opt_bias_i},
    {"pbias",   "bias p",               "bias_p",                                   mpi_enc_opt_bias_p},
    {"lmd",     "lambda idx",           "lambda_idx_p 0~8",                         mpi_enc_opt_lmd},
    {"lmdi",    "lambda i idx",         "lambda_idx_i 0~8",                         mpi_enc_opt_lmdi},
    {"speed",   "enc speed",            "speed mode",                               mpi_enc_opt_speed},
    {"kmpp",    "kmpp path enable",     "kmpp path enable",                         mpi_enc_opt_kmpp}
};

static RK_U32 enc_opt_cnt = MPP_ARRAY_ELEMS(enc_opts);

RK_S32 mpi_enc_show_help(const char *name)
{
    RK_U32 max_name = 1;
    RK_U32 max_full_name = 1;
    RK_U32 max_help = 1;
    char logs[256];
    RK_U32 len;
    RK_U32 i;

    mpp_log("usage: %s [options]\n", name);

    for (i = 0; i < enc_opt_cnt; i++) {
        MppOptInfo *opt = &enc_opts[i];

        if (opt->name) {
            len = strlen(opt->name);
            if (len > max_name)
                max_name = len;
        }

        if (opt->full_name) {
            len = strlen(opt->full_name);
            if (len > max_full_name)
                max_full_name = len;
        }

        if (opt->help) {
            len = strlen(opt->help);
            if (len > max_help)
                max_help = len;
        }
    }

    snprintf(logs, sizeof(logs) - 1, "-%%-%ds %%-%ds %%-%ds\n", max_name, max_full_name, max_help);

    for (i = 0; i < enc_opt_cnt; i++) {
        MppOptInfo *opt = &enc_opts[i];

        mpp_log(logs, opt->name, opt->full_name, opt->help);
    }
    mpp_show_support_format();
    mpp_show_color_format();

    return -1;
}

void show_enc_fps(RK_S64 total_time, RK_S64 total_count, RK_S64 last_time, RK_S64 last_count)
{
    float avg_fps = (float)total_count * 1000000 / total_time;
    float ins_fps = (float)last_count * 1000000 / last_time;

    mpp_log("encoded %10lld frame fps avg %7.2f ins %7.2f\n",
            total_count, avg_fps, ins_fps);
}

static void mpi_enc_cmd_env_get(MpiEncTestArgs *cmd)
{
    mpp_env_get_u32("mirroring", &cmd->mirroring, cmd->mirroring);
    mpp_env_get_u32("rotation", &cmd->rotation, cmd->rotation);
    mpp_env_get_u32("flip", &cmd->flip, cmd->flip);
    mpp_env_get_u32("split_mode", &cmd->split_mode, cmd->split_mode);
    mpp_env_get_u32("split_arg", &cmd->split_arg, cmd->split_arg);
    mpp_env_get_u32("split_out", &cmd->split_out, cmd->split_out);
    mpp_env_get_u32("osd_enable", &cmd->osd_enable, cmd->osd_enable);
    mpp_env_get_u32("roi_jpeg_enable", &cmd->roi_jpeg_enable, cmd->roi_jpeg_enable);
    mpp_env_get_u32("jpeg_osd_case", &cmd->jpeg_osd_case, cmd->jpeg_osd_case);
    mpp_env_get_u32("osd_mode", &cmd->osd_mode, cmd->osd_mode);
    mpp_env_get_u32("roi_enable", &cmd->roi_enable, cmd->roi_enable);
    mpp_env_get_u32("user_data_enable", &cmd->user_data_enable, cmd->user_data_enable);
    mpp_env_get_u32("constraint_set", &cmd->constraint_set, cmd->constraint_set);
    mpp_env_get_u32("gop_mode", (RK_U32 *)&cmd->gop_mode, (RK_U32)cmd->gop_mode);
    mpp_env_get_u32("sei_mode", &cmd->sei_mode, cmd->sei_mode);
}

MPP_RET mpi_enc_test_objset_update_by_args(MppEncTestObjSet *obj_set, int argc, char **argv, const char *module_tag)
{
    MpiEncTestArgs *cmd = NULL;
    MppEncCfg cfg_obj = NULL;
    MppEncArgs cmd_obj = NULL;
    MppOpt opts = NULL;
    RK_S32 ret = -1;
    RK_U32 i;

    if ((argc < 2) || NULL == argv)
        goto done;

    mpp_enc_args_get(&cmd_obj);
    cmd = kmpp_obj_to_entry(cmd_obj);
    obj_set->cmd_obj = cmd_obj;
    obj_set->cmd = cmd;
    if (cmd == NULL)
        goto done;

    mpp_opt_init(&opts);
    mpp_opt_setup(opts, obj_set);

    for (i = 0; i < enc_opt_cnt; i++)
        mpp_opt_add(opts, &enc_opts[i]);

    /* mark option end */
    mpp_opt_add(opts, NULL);
    ret = mpp_opt_parse(opts, argc, argv);
    if (ret)
        goto done;

    mpi_enc_cmd_env_get(cmd);

    if (cmd->type <= MPP_VIDEO_CodingAutoDetect) {
        mpp_err("invalid type %d\n", cmd->type);
        ret = MPP_NOK;
        goto done;
    }

    if (cmd->rc_mode == MPP_ENC_RC_MODE_BUTT) {
        cmd->rc_mode = (cmd->type == MPP_VIDEO_CodingMJPEG) ?
                       MPP_ENC_RC_MODE_FIXQP : MPP_ENC_RC_MODE_VBR;
        if (obj_set->cfg_obj)
            mpp_enc_cfg_set_s32(obj_set->cfg_obj, "rc:mode", cmd->rc_mode);
    }


    if (!cmd->hor_stride) {
        cmd->hor_stride = mpi_enc_width_default_stride(cmd->width, cmd->format);
        if (obj_set->cfg_obj)
            mpp_enc_cfg_set_s32(obj_set->cfg_obj, "prep:hor_stride", cmd->hor_stride);
    }

    if (!cmd->ver_stride) {
        cmd->ver_stride = MPP_ALIGN(cmd->height, 2);
        if (obj_set->cfg_obj)
            mpp_enc_cfg_set_s32(obj_set->cfg_obj, "prep:ver_stride", cmd->ver_stride);
    }

    if (cmd->type_src == MPP_VIDEO_CodingUnused) {
        if (cmd->width <= 0 || cmd->height <= 0 ||
            cmd->hor_stride <= 0 || cmd->ver_stride <= 0) {
            mpp_err("invalid w:h [%d:%d] stride [%d:%d]\n",
                    cmd->width, cmd->height, cmd->hor_stride, cmd->ver_stride);
            ret = MPP_NOK;
            goto done;
        }
    }

    if (cmd->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        if (!cmd->qp_init) {
            if (cmd->type == MPP_VIDEO_CodingAVC ||
                cmd->type == MPP_VIDEO_CodingHEVC)
                cmd->qp_init = 26;
            if (obj_set->cfg_obj)
                mpp_enc_cfg_set_s32(obj_set->cfg_obj, "rc:qp_init", cmd->qp_init);
        }
    }

    if (cmd->file_cfg) {
        if (cmd->kmpp_en) {
            mpp_err("file_cfg and kmpp_en can not be set at the same time\n");
            ret = MPP_NOK;
            goto done;
        } else {
            ret = MPP_OK;
            goto done;
        }
    }

    if (cmd->trace_fps) {
        fps_calc_init(&cmd->fps);
        mpp_assert(cmd->fps);
        fps_calc_set_cb(cmd->fps, show_enc_fps);
    }

done:
    if (opts) {
        mpp_opt_deinit(opts);
        opts = NULL;
    }
    if (ret)
        mpi_enc_show_help(argv[0]);
    else
        mpp_enc_args_dump(obj_set->cmd_obj, module_tag);

    return ret;
}

MPP_RET mpi_enc_test_cmd_put(MppEncTestObjSet* obj_set)
{
    MpiEncTestArgs *cmd = obj_set->cmd;

    if (cmd->cfg_ini) {
        iniparser_freedict(cmd->cfg_ini);
        cmd->cfg_ini = NULL;
    }

    if (cmd->fps) {
        fps_calc_deinit(cmd->fps);
        cmd->fps = NULL;
    }

    mpp_enc_args_put(obj_set->cmd_obj);

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
        st_ref[pos].temporal_id = 0;
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
    RK_U32 step_x = MPP_ALIGN(mb_w_max, 8) / 8;
    RK_U32 step_y = MPP_ALIGN(mb_h_max, 16) / 16;
    RK_U32 mb_x = (frame_cnt * step_x) % mb_w_max;
    RK_U32 mb_y = (frame_cnt * step_y) % mb_h_max;
    RK_U32 mb_w = step_x;
    RK_U32 mb_h = step_y;
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

        mb_x += step_x;
        mb_y += step_y;
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

MPP_RET mpi_enc_ctx_init(MpiEncTestData *p, MpiEncTestArgs *cmd, RK_S32 chn)
{
    MPP_RET ret = MPP_OK;

    // get paramter from cmd
    p->type              = cmd->type;
    p->width             = cmd->width;
    p->height            = cmd->height;
    p->hor_stride        = cmd->hor_stride;
    p->ver_stride        = cmd->ver_stride;
    p->fmt               = cmd->format;
    p->frame_num         = cmd->frame_num;
    p->frm_step          = cmd->frm_step;
    if (p->type == MPP_VIDEO_CodingMJPEG && p->frame_num == 0) {
        mpp_log("jpege default encode only one frame. Use -n [num] for rc case\n");
        p->frame_num = 1;
    }

    if (cmd->file_input) {
        if (!strncmp(cmd->file_input, "/dev/video", 10)) {
            mpp_log("open camera device");
            p->cam_ctx = camera_source_init(cmd->file_input, 4, p->width, p->height, p->fmt);
            mpp_log("new framecap ok");
            if (p->cam_ctx == NULL)
                mpp_err("open %s fail", cmd->file_input);
        } else {
            p->fp_input = fopen(cmd->file_input, "rb");
            if (NULL == p->fp_input) {
                mpp_err("failed to open input file %s\n", cmd->file_input);
                mpp_err("create default yuv image for test\n");
            }
        }
    }

    if (cmd->file_output) {
        char temp[256];
        char output_name[256];
        char *path = NULL;
        char *filename = NULL;

        if (cmd->nthreads > 1) {
            strcpy(temp, cmd->file_output);
            split_path_file_inplace(temp, &path, &filename);
            sprintf(output_name, "%s/chn%d_%s", path, chn, filename);
            p->fp_output[chn] = fopen(output_name, "w+b");
        } else if (cmd->nthreads == 1) {
            p->fp_output[chn] = fopen(cmd->file_output, "w+b");
        }

        if (NULL == p->fp_output[chn]) {
            mpp_err("failed to open output file, chn %d\n", chn);
            ret = MPP_ERR_OPEN_FILE;
        }
    }

    if (cmd->file_slt) {
        p->fp_verify = fopen(cmd->file_slt, "wt");
        if (!p->fp_verify)
            mpp_err("failed to open verify file %s\n", cmd->file_slt);
    }

    // update resource parameter
    switch (p->fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY :
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP : {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
    } break;
    case MPP_FMT_YUV400:
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 :
    case MPP_FMT_RGB888 :
    case MPP_FMT_BGR888 :
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64);
    } break;

    default: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
    } break;
    }

    if (MPP_FRAME_FMT_IS_FBC(p->fmt)) {
        if ((p->fmt & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
            p->header_size = MPP_ALIGN(MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16, SZ_4K);
        else
            p->header_size = MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16;
    } else {
        p->header_size = 0;
    }

    return ret;
}

MPP_RET mpi_enc_ctx_deinit(MpiEncTestData *p)
{
    RK_S32 i;

    if (p) {
        if (p->cam_ctx) {
            camera_source_deinit(p->cam_ctx);
            p->cam_ctx = NULL;
        }
        if (p->fp_input) {
            fclose(p->fp_input);
            p->fp_input = NULL;
        }
        for (i = 0; i < MPI_ENC_MAX_CHN; i++) {
            if (p->fp_output[i]) {
                fclose(p->fp_output[i]);
                p->fp_output[i] = NULL;
            }
        }
        if (p->fp_verify) {
            fclose(p->fp_verify);
            p->fp_verify = NULL;
        }
    }
    return MPP_OK;
}

MPP_RET mpi_enc_cfg_setup(MpiEncTestData *p, MpiEncTestArgs *cmd, MppEncCfg cfg_obj)
{
    MppApi *mpi = p->mpi;
    MppCtx ctx = p->ctx;
    MppEncCfg cfg = p->cfg;
    MPP_RET ret;
    MppEncRefCfg ref = NULL;

    /* setup default parameter */
    if (cmd->fps_in_den == 0)
        cmd->fps_in_den = 1;
    if (cmd->fps_in_num == 0)
        cmd->fps_in_num = 30;
    if (cmd->fps_out_den == 0)
        cmd->fps_out_den = 1;
    if (cmd->fps_out_num == 0)
        cmd->fps_out_num = 30;

    if (!cmd->bps_target)
        cmd->bps_target = p->width * p->height / 8 * (cmd->fps_out_num / cmd->fps_out_den);

    if (cfg_obj) {
        kmpp_obj_copy(p->cfg, cfg_obj);
    } else {
        mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);

        /* setup preprocess parameters */
        mpp_enc_cfg_set_s32(cfg, "prep:width", p->width);
        mpp_enc_cfg_set_s32(cfg, "prep:height", p->height);
        mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", p->hor_stride);
        mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", p->ver_stride);
        mpp_enc_cfg_set_s32(cfg, "prep:format", p->fmt);
        mpp_enc_cfg_set_s32(cfg, "prep:range", MPP_FRAME_RANGE_JPEG);

        /* setup rate control parameters */
        mpp_enc_cfg_set_s32(cfg, "rc:mode", cmd->rc_mode);
        mpp_enc_cfg_set_u32(cfg, "rc:max_reenc_times", 0);
        mpp_enc_cfg_set_u32(cfg, "rc:super_mode", 0);

        /* drop frame or not when bitrate overflow */
        mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
        mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
        mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

        /* setup fine tuning paramters */
        mpp_enc_cfg_set_s32(cfg, "tune:anti_flicker_str", cmd->anti_flicker_str);
        mpp_enc_cfg_set_s32(cfg, "tune:atf_str", cmd->atf_str);
        mpp_enc_cfg_set_s32(cfg, "tune:atr_str_i", cmd->atr_str_i);
        mpp_enc_cfg_set_s32(cfg, "tune:atr_str_p", cmd->atr_str_p);
        mpp_enc_cfg_set_s32(cfg, "tune:atl_str", cmd->atl_str);
        mpp_enc_cfg_set_s32(cfg, "tune:deblur_en", cmd->deblur_en);
        mpp_enc_cfg_set_s32(cfg, "tune:deblur_str", cmd->deblur_str);
        mpp_enc_cfg_set_s32(cfg, "tune:sao_str_i", cmd->sao_str_i);
        mpp_enc_cfg_set_s32(cfg, "tune:sao_str_p", cmd->sao_str_p);
        mpp_enc_cfg_set_s32(cfg, "tune:lambda_idx_p", cmd->lambda_idx_p);
        mpp_enc_cfg_set_s32(cfg, "tune:lambda_idx_i", cmd->lambda_idx_i);
        mpp_enc_cfg_set_s32(cfg, "tune:rc_container", cmd->rc_container);
        mpp_enc_cfg_set_s32(cfg, "tune:scene_mode", cmd->scene_mode);
        mpp_enc_cfg_set_s32(cfg, "tune:speed", cmd->speed);
        mpp_enc_cfg_set_s32(cfg, "tune:vmaf_opt", 0);

        mpp_enc_cfg_set_s32(cfg, "hw:qbias_en", 1);
        mpp_enc_cfg_set_s32(cfg, "hw:qbias_i", cmd->bias_i);
        mpp_enc_cfg_set_s32(cfg, "hw:qbias_p", cmd->bias_p);
        mpp_enc_cfg_set_s32(cfg, "hw:skip_bias_en", 0);
        mpp_enc_cfg_set_s32(cfg, "hw:skip_bias", 4);
        mpp_enc_cfg_set_s32(cfg, "hw:skip_sad", 8);
    }

    mpp_enc_cfg_set_s32(cfg, "prep:mirroring", cmd->mirroring);
    mpp_enc_cfg_set_s32(cfg, "prep:rotation", cmd->rotation);
    mpp_enc_cfg_set_s32(cfg, "prep:flip", cmd->flip);

    if (cmd->split_mode) {
        mpp_enc_cfg_set_s32(cfg, "split:mode", cmd->split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", cmd->split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:out", cmd->split_out);
    }

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", cmd->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", cmd->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denom", cmd->fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", cmd->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", cmd->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denom", cmd->fps_out_den);

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", cmd->bps_target);
    switch (cmd->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", (cmd->bps_max != 0) ? cmd->bps_max : cmd->bps_target * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", (cmd->bps_min != 0) ? cmd->bps_min : cmd->bps_target * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR :
    case MPP_ENC_RC_MODE_AVBR : {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", (cmd->bps_max != 0) ? cmd->bps_max : cmd->bps_target * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", (cmd->bps_min != 0) ? cmd->bps_min : cmd->bps_target * 1 / 16);
    } break;
    default : {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", (cmd->bps_max != 0) ? cmd->bps_max : cmd->bps_target * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", (cmd->bps_min != 0) ? cmd->bps_min : cmd->bps_target * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (p->type) {
    case MPP_VIDEO_CodingAVC :
    case MPP_VIDEO_CodingHEVC : {
        switch (cmd->rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            RK_S32 fix_qp = cmd->qp_init;

            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", fix_qp);
        } break;
        case MPP_ENC_RC_MODE_CBR :
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR :
        case MPP_ENC_RC_MODE_SMTRC : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", (cmd->qp_init != 0) ? cmd->qp_init : -1);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", (cmd->qp_max != 0) ? cmd->qp_max : 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", (cmd->qp_min != 0) ? cmd->qp_min : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", (cmd->qp_max_i != 0) ? cmd->qp_max_i : 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", (cmd->qp_min_i != 0) ? cmd->qp_min_i : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", (cmd->fqp_min_i != 0) ? cmd->fqp_min_i : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", (cmd->fqp_max_i != 0) ? cmd->fqp_max_i : 45);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", (cmd->fqp_min_p != 0) ? cmd->fqp_min_p : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", (cmd->fqp_max_p != 0) ? cmd->fqp_max_p : 45);
        } break;
        default : {
            mpp_err_f("unsupport encoder rc mode %d\n", cmd->rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8 : {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", (cmd->qp_init != 0) ? cmd->qp_init : 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  (cmd->qp_max != 0) ? cmd->qp_max : 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  (cmd->qp_min != 0) ? cmd->qp_min : 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", (cmd->qp_max_i != 0) ? cmd->qp_max_i : 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", (cmd->qp_min_i != 0) ? cmd->qp_min_i : 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", (cmd->qp_init != 0) ? cmd->qp_init : 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", (cmd->qp_max != 0) ? cmd->qp_max : 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", (cmd->qp_min != 0) ? cmd->qp_min : 1);
    } break;
    default : {
    } break;
    }

    switch (p->type) {
    case MPP_VIDEO_CodingAVC : {
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        if (cmd->constraint_set & 0x3f0000)
            mpp_enc_cfg_set_s32(cfg, "h264:constraint_set", cmd->constraint_set);
    } break;
    case MPP_VIDEO_CodingHEVC : {
        mpp_enc_cfg_set_s32(cfg, "h265:diff_cu_qp_delta_depth", cmd->cu_qp_delta_depth);
    } break;
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 : {
    } break;
    default : {
        mpp_err_f("unsupport encoder coding type %d\n", p->type);
    } break;
    }

    // config gop_len and ref cfg
    mpp_enc_cfg_set_s32(cfg, "rc:gop", (cmd->gop_len != 0) ? cmd->gop_len : cmd->fps_out_num * 2);

    if (cmd->gop_mode) {
        mpp_enc_ref_cfg_init(&ref);

        if (cmd->gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, cmd->gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, cmd->gop_len, cmd->vi_len);

        mpp_enc_cfg_set_ptr(cfg, "rc:ref_cfg", ref);
    }

    /* setup hardware specified parameters */
    if (cmd->rc_mode == MPP_ENC_RC_MODE_SMTRC) {
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_i", aq_thd_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_p", aq_thd_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_step_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_p", aq_step_smart);
    } else {
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_i", aq_thd);
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_p", aq_thd);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_step_i_ipc);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_p", aq_step_p_ipc);
    }
    mpp_enc_cfg_set_st(cfg, "hw:aq_rnge_arr", aq_rnge_arr);

    if (p->type == MPP_VIDEO_CodingAVC) {
        mpp_enc_cfg_set_st(cfg, "hw:qbias_arr", qbias_arr_avc);
    } else if (p->type == MPP_VIDEO_CodingHEVC) {
        mpp_enc_cfg_set_st(cfg, "hw:qbias_arr", qbias_arr_hevc);
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        goto RET;
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        RcApiBrief rc_api_brief;
        rc_api_brief.type = p->type;
        rc_api_brief.name = (cmd->rc_mode == MPP_ENC_RC_MODE_SMTRC) ?
                            "smart" : "default";

        ret = mpi->control(ctx, MPP_ENC_SET_RC_API_CURRENT, &rc_api_brief);
        if (ret) {
            mpp_err("mpi control enc set rc api failed ret %d\n", ret);
            goto RET;
        }
    }

    if (ref)
        mpp_enc_ref_cfg_deinit(&ref);

    /* optional */
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &cmd->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto RET;
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            goto RET;
        }
    }

    if (cmd->roi_enable) {
        mpp_enc_roi_init(&p->roi_ctx, p->width, p->height, p->type, 4);
        mpp_assert(p->roi_ctx);
    }

RET:
    return ret;
}

MPP_RET mpi_enc_test_objset_get(MppEncTestObjSet **obj_set)
{
    *obj_set = mpp_calloc(MppEncTestObjSet, 1);

    return *obj_set ? MPP_OK : MPP_NOK;
}

MPP_RET mpi_enc_test_objset_put(MppEncTestObjSet *obj_set)
{
    if (!obj_set)
        return MPP_OK;

    if (obj_set->cmd_obj)
        mpi_enc_test_cmd_put(obj_set);
    if (obj_set->cfg_obj)
        mpp_enc_cfg_deinit(obj_set->cfg_obj);

    mpp_free(obj_set);

    return MPP_OK;
}