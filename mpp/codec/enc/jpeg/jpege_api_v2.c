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

#define MODULE_TAG "jpege_api_v2"

#include <string.h>

#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "jpege_debug.h"
#include "jpege_api.h"
#include "jpege_syntax.h"
#include "mpp_enc_cfg_impl.h"

typedef struct {
    MppEncCfgSet    *cfg;
    JpegeSyntax     syntax;
} JpegeCtx;

RK_U32 jpege_debug = 0;

static MPP_RET jpege_init_v2(void *ctx, EncImplCfg *cfg)
{
    JpegeCtx *p = (JpegeCtx *)ctx;

    mpp_env_get_u32("jpege_debug", &jpege_debug, 0);
    jpege_dbg_func("enter ctx %p\n", ctx);

    p->cfg = cfg->cfg;

    mpp_assert(cfg->coding = MPP_VIDEO_CodingMJPEG);
    cfg->task_count = 1;

    /* init default fps config */
    MppEncRcCfg *rc = &p->cfg->rc;
    rc->fps_in_flex = 0;
    rc->fps_in_num = 30;
    rc->fps_in_denorm = 1;
    rc->fps_out_flex = 0;
    rc->fps_out_num = 30;
    rc->fps_out_denorm = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_deinit_v2(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_proc_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    mpp_assert(change);
    if (change) {
        MppEncPrepCfg bak = *dst;

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT)
            dst->format = src->format;

        if (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)
            dst->rotation = src->rotation;

        /* jpeg encoder do not have mirring / denoise feature */

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            if (dst->rotation == MPP_ENC_ROT_90 || dst->rotation == MPP_ENC_ROT_270) {
                dst->width = src->height;
                dst->height = src->width;
            } else {
                dst->width = src->width;
                dst->height = src->height;
            }
            dst->hor_stride = src->hor_stride;
            dst->ver_stride = src->ver_stride;
        }

        if (dst->width < 16 && dst->width > 8192) {
            mpp_err_f("invalid width %d is not in range [16..8192]\n", dst->width);
            ret = MPP_NOK;
        }

        if (dst->height < 16 && dst->height > 8192) {
            mpp_err_f("invalid height %d is not in range [16..8192]\n", dst->height);
            ret = MPP_NOK;
        }

        if (dst->format != MPP_FMT_YUV420SP     &&
            dst->format != MPP_FMT_YUV420P      &&
            dst->format != MPP_FMT_YUV422SP_VU  &&
            dst->format != MPP_FMT_YUV422_YUYV  &&
            dst->format != MPP_FMT_YUV422_UYVY  &&
            dst->format != MPP_FMT_RGB888       &&
            dst->format != MPP_FMT_BGR888) {
            mpp_err_f("invalid format %d is not supportted\n", dst->format);
            ret = MPP_NOK;
        }

        dst->change |= change;

        // parameter checking
        if (dst->width > dst->hor_stride || dst->height > dst->ver_stride) {
            mpp_err("invalid size w:h [%d:%d] stride [%d:%d]\n",
                    dst->width, dst->height, dst->hor_stride, dst->ver_stride);
            ret = MPP_ERR_VALUE;
        }

        if (ret) {
            mpp_err_f("failed to accept new prep config\n");
            *dst = bak;
        } else {
            mpp_log_f("MPP_ENC_SET_PREP_CFG w:h [%d:%d] stride [%d:%d]\n",
                      dst->width, dst->height,
                      dst->hor_stride, dst->ver_stride);
        }
    }

    return ret;
}

static MPP_RET jpege_proc_rc_cfg(MppEncRcCfg *dst, MppEncRcCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncRcCfg bak = *dst;

        if (change & MPP_ENC_RC_CFG_CHANGE_RC_MODE)
            dst->rc_mode = src->rc_mode;

        if (change & MPP_ENC_RC_CFG_CHANGE_BPS) {
            dst->bps_target = src->bps_target;
            dst->bps_max = src->bps_max;
            dst->bps_min = src->bps_min;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_FPS_IN) {
            dst->fps_in_flex = src->fps_in_flex;
            dst->fps_in_num = src->fps_in_num;
            dst->fps_in_denorm = src->fps_in_denorm;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
            dst->fps_out_flex = src->fps_out_flex;
            dst->fps_out_num = src->fps_out_num;
            dst->fps_out_denorm = src->fps_out_denorm;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_GOP)
            dst->gop = src->gop;

        // parameter checking
        if (dst->rc_mode >= MPP_ENC_RC_MODE_BUTT) {
            mpp_err("invalid rc mode %d should be RC_MODE_VBR or RC_MODE_CBR\n",
                    src->rc_mode);
            ret = MPP_ERR_VALUE;
        }
        if (dst->quality >= MPP_ENC_RC_QUALITY_BUTT) {
            mpp_err("invalid quality %d should be from QUALITY_WORST to QUALITY_BEST\n",
                    dst->quality);
            ret = MPP_ERR_VALUE;
        }
        if (dst->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
            if ((dst->bps_target >= 100 * SZ_1M || dst->bps_target <= 1 * SZ_1K) ||
                (dst->bps_max    >= 100 * SZ_1M || dst->bps_max    <= 1 * SZ_1K) ||
                (dst->bps_min    >= 100 * SZ_1M || dst->bps_min    <= 1 * SZ_1K)) {
                mpp_err("invalid bit per second %d [%d:%d] out of range 1K~100M\n",
                        dst->bps_target, dst->bps_min, dst->bps_max);
                ret = MPP_ERR_VALUE;
            }
        }

        dst->change |= change;

        if (ret) {
            mpp_err_f("failed to accept new rc config\n");
            *dst = bak;
        } else {
            mpp_log_f("MPP_ENC_SET_RC_CFG bps %d [%d : %d] fps [%d:%d] gop %d\n",
                      dst->bps_target, dst->bps_min, dst->bps_max,
                      dst->fps_in_num, dst->fps_out_num, dst->gop);
        }
    }

    return ret;
}

static MPP_RET jpege_proc_jpeg_cfg(MppEncJpegCfg *dst, MppEncJpegCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncJpegCfg bak = *dst;

        if (change & MPP_ENC_JPEG_CFG_CHANGE_QP) {
            dst->quant = src->quant;
        }

        if (dst->quant < 0 || dst->quant > 10) {
            mpp_err_f("invalid quality level %d is not in range [0..10] set to default 8\n");
            dst->quant = 8;
        }

        if (ret) {
            mpp_err_f("failed to accept new rc config\n");
            *dst = bak;
        } else {
            mpp_log_f("MPP_ENC_SET_CODEC_CFG jpeg quant set to %d\n", dst->quant);
            dst->change = src->change;
        }

        dst->change = src->change;
    }

    return ret;
}

static MPP_RET jpege_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    MppEncCfgSet *cfg = p->cfg;
    MPP_RET ret = MPP_OK;

    jpege_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncCfgImpl *impl = (MppEncCfgImpl *)param;
        MppEncCfgSet *src = &impl->cfg;

        if (src->prep.change) {
            ret |= jpege_proc_prep_cfg(&cfg->prep, &src->prep);
            src->prep.change = 0;
        }
        if (src->rc.change) {
            ret |= jpege_proc_rc_cfg(&cfg->rc, &src->rc);
            src->rc.change = 0;
        }
        if (src->codec.jpeg.change) {
            ret |= jpege_proc_jpeg_cfg(&cfg->codec.jpeg, &src->codec.jpeg);
            src->codec.jpeg.change = 0;
        }
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        ret = jpege_proc_prep_cfg(&cfg->prep, param);
    } break;
    case MPP_ENC_SET_RC_CFG : {
        ret = jpege_proc_rc_cfg(&cfg->rc, param);
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncCodecCfg *codec = (MppEncCodecCfg *)param;
        ret = jpege_proc_jpeg_cfg(&cfg->codec.jpeg, &codec->jpeg);
    } break;
    case MPP_ENC_SET_IDR_FRAME :
    case MPP_ENC_SET_OSD_PLT_CFG :
    case MPP_ENC_SET_OSD_DATA_CFG :
    case MPP_ENC_GET_SEI_DATA :
    case MPP_ENC_SET_SEI_CFG : {
    } break;
    default:
        mpp_err_f("No correspond cmd(%08x) found, and can not config!", cmd);
        ret = MPP_NOK;
        break;
    }

    jpege_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET jpege_proc_hal(void *ctx, HalEncTask *task)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    JpegeSyntax *syntax = &p->syntax;
    MppEncCfgSet *cfg = p->cfg;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncCodecCfg *codec = &cfg->codec;

    jpege_dbg_func("enter ctx %p\n", ctx);

    syntax->width       = prep->width;
    syntax->height      = prep->height;
    syntax->hor_stride  = prep->hor_stride;
    syntax->ver_stride  = prep->ver_stride;
    syntax->format      = prep->format;
    syntax->color       = prep->color;
    syntax->quality     = codec->jpeg.quant;

    task->valid = 1;
    task->is_intra = 1;
    task->syntax.data = syntax;
    task->syntax.number = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

const EncImplApi api_jpege = {
    .name       = "jpege_control",
    .coding     = MPP_VIDEO_CodingMJPEG,
    .ctx_size   = sizeof(JpegeCtx),
    .flag       = 0,
    .init       = jpege_init_v2,
    .deinit     = jpege_deinit_v2,
    .proc_cfg   = jpege_proc_cfg,
    .gen_hdr    = NULL,
    .start      = NULL,
    .proc_dpb   = NULL,
    .proc_hal   = jpege_proc_hal,
    .update_hal = NULL,
    .reset      = NULL,
    .flush      = NULL,
    .callback   = NULL,
};
