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
#define MODULE_TAG  "vp8e_api_v2"

#include <string.h>
#include <vp8e_syntax.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_rc.h"
#include "mpp_enc_cfg_impl.h"

#include "vp8e_api_v2.h"
#include "vp8e_rc.h"
#include "vp8e_syntax.h"

#define VP8E_DBG_FUNCTION            (0x00000001)
#define VP8E_DBG_CFG                 (0x00000002)

#define vp8e_dbg_cfg(fmt, ...)    _mpp_dbg_f(vp8e_debug, VP8E_DBG_CFG, fmt, ## __VA_ARGS__)
#define vp8e_dbg_fun(fmt, ...)    _mpp_dbg_f(vp8e_debug, VP8E_DBG_FUNCTION, fmt, ## __VA_ARGS__)

RK_U32 vp8e_debug = 0;
#define VP8E_SYN_BUTT 2

typedef struct {
    /* config from mpp_enc */
    MppEncCfgSet    *cfg;
    MppEncCfgSet    *set;

    /* internal rate control config*/
    Vp8eRc          *rc;

    Vp8eSyntax      vp8e_syntax[VP8E_SYN_BUTT];
} Vp8eCtx;

static MPP_RET vp8e_init(void *ctx, EncImplCfg *ctrl_cfg)
{
    MPP_RET ret = MPP_OK;
    Vp8eCtx *p = (Vp8eCtx *)ctx;

    MppEncRcCfg *rc_cfg = &ctrl_cfg->cfg->rc;
    MppEncPrepCfg *prep = &ctrl_cfg->cfg->prep;

    vp8e_dbg_fun("enter\n");
    if (NULL == ctx || NULL == ctrl_cfg) {
        mpp_err_f("Init failed, contex or controller cfg is null!\n");
        ret = MPP_NOK;
        goto __ERR_RET;
    }

    p->cfg = ctrl_cfg->cfg;
    p->set = ctrl_cfg->set;

    /*
     * default prep:
     * 720p
     * YUV420SP
     */
    prep->change = 0;
    prep->width = 1280;
    prep->height = 720;
    prep->hor_stride = 1280;
    prep->ver_stride = 720;
    prep->format = MPP_FMT_YUV420SP;
    prep->rotation = MPP_ENC_ROT_0;
    prep->mirroring = 0;
    prep->denoise = 0;

    /*
     * default rc_cfg:
     * CBR
     * 2Mbps +-25%
     * 30fps
     * gop 60
     */
    rc_cfg->change = 0;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;
    rc_cfg->bps_target = 2000 * 1000;
    rc_cfg->bps_max = rc_cfg->bps_target * 5 / 4;
    rc_cfg->bps_min = rc_cfg->bps_target * 3 / 4;
    rc_cfg->fps_in_flex = 0;
    rc_cfg->fps_in_num = 30;
    rc_cfg->fps_in_denorm = 1;
    rc_cfg->fps_out_flex = 0;
    rc_cfg->fps_out_num = 30;
    rc_cfg->fps_out_denorm = 1;
    rc_cfg->gop = 60;
    rc_cfg->max_reenc_times = 1;

    p->rc = mpp_calloc(Vp8eRc, 1);
    memset(p->rc, 0, sizeof(Vp8eRc));
    p->rc->frame_coded = 1;
    if (NULL == p->rc) {
        mpp_err_f("failed to malloc vp8_rc\n");
        ret = MPP_ERR_MALLOC;
        goto __ERR_RET;
    }

    mpp_env_get_u32("vp8e_debug", &vp8e_debug, 0);

    vp8e_dbg_fun("leave ret %d\n", ret);
    return ret;

__ERR_RET:
    vp8e_dbg_fun("leave ret %d\n", ret);
    return ret;
}

static MPP_RET vp8e_deinit(void *ctx)
{
    Vp8eCtx *p = (Vp8eCtx *)ctx;

    vp8e_dbg_fun("enter\n");

    if (p->rc)
        mpp_free(p->rc);

    vp8e_dbg_fun("leave\n");
    return MPP_OK;
}

static MPP_RET vp8e_start(void *ctx, HalEncTask *task)
{
    (void)ctx;
    (void)task;

    return MPP_OK;
}

static MPP_RET vp8e_reset(void *ctx)
{
    (void)ctx;

    return MPP_OK;
}

static MPP_RET vp8e_flush(void *ctx)
{
    Vp8eCtx *p = (Vp8eCtx *)ctx;
    (void)p;

    return MPP_OK;
}
static MPP_RET vp8e_proc_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    mpp_assert(change);
    if (change) {
        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            if ((src->width < 0 || src->width > 1920) ||
                (src->height < 0 || src->height > 3840) ||
                (src->hor_stride < 0 || src->hor_stride > 3840) ||
                (src->ver_stride < 0 || src->ver_stride > 3840)) {
                mpp_err("invalid input w:h [%d:%d] [%d:%d]\n",
                        src->width, src->height,
                        src->hor_stride, src->ver_stride);
                ret = MPP_NOK;
            }
            dst->width = src->width;
            dst->height = src->height;
            dst->ver_stride = src->ver_stride;
            dst->hor_stride = src->hor_stride;
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            if ((src->format < MPP_FRAME_FMT_RGB &&
                 src->format >= MPP_FMT_YUV_BUTT) ||
                src->format >= MPP_FMT_RGB_BUTT) {
                mpp_err("invalid format %d\n", src->format);
                ret = MPP_NOK;
            }
            dst->format = src->format;
        }
        dst->change |= src->change;
        vp8e_dbg_cfg("width %d height %d ver_srtride %d format 0x%x\n",
                     dst->width, dst->height, dst->ver_stride, dst->hor_stride, dst->format);
    }
    return ret;
}

static MPP_RET vp8e_proc_split_cfg(MppEncSliceSplit *dst, MppEncSliceSplit *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_MODE) {
        dst->split_mode = src->split_mode;
        dst->split_arg = src->split_arg;
    }

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_ARG)
        dst->split_arg = src->split_arg;

    dst->change |= change;

    return ret;
}

static MPP_RET vp8e_proc_rc_cfg(MppEncRcCfg *dst, MppEncRcCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;
    vp8e_dbg_cfg("set rc cfg change %d\n", change);
    if (change) {
        MppEncRcCfg bak = *dst;

        if (change & MPP_ENC_RC_CFG_CHANGE_RC_MODE)
            dst->rc_mode = src->rc_mode;

        if (change & MPP_ENC_RC_CFG_CHANGE_QUALITY)
            dst->quality = src->quality;

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

        if (change & MPP_ENC_RC_CFG_CHANGE_MAX_REENC)
            dst->max_reenc_times = src->max_reenc_times;

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
            vp8e_dbg_cfg("MPP_ENC_SET_RC_CFG bps %d [%d : %d] fps [%d:%d] gop %d\n",
                         dst->bps_target, dst->bps_min, dst->bps_max,
                         dst->fps_in_num, dst->fps_out_num, dst->gop);
        }
    }

    return ret;
}

static MPP_RET vp8e_proc_vp8_cfg(MppEncVp8Cfg *dst, MppEncVp8Cfg *src)
{
    RK_U32 change = src->change;

    if (change & MPP_ENC_VP8_CFG_CHANGE_QP) {
        dst->qp_init = src->qp_init;
        dst->qp_max = src->qp_max;
        dst->qp_min = src->qp_min;
        dst->qp_max_i = src->qp_max_i;
        dst->qp_min_i = src->qp_min_i;
    }
    vp8e_dbg_cfg("rc cfg qp_init %d qp_max %d qp_min %d qp_max_i %d qp_min_i %d\n",
                 dst->qp_init, dst->qp_max, dst->qp_min, dst->qp_max_i, dst->qp_min_i);
    dst->change |= src->change;
    return MPP_OK;
}

static MPP_RET vp8e_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    Vp8eCtx *p = (Vp8eCtx *)ctx;
    MppEncCfgSet *cfg = p->cfg;
    MppEncCfgImpl *impl = (MppEncCfgImpl *)param;
    MppEncCfgSet *src = &impl->cfg;

    vp8e_dbg_fun("enter ctx %p cmd %x param %p\n", ctx, cmd, param);
    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        if (src->prep.change) {
            ret |= vp8e_proc_prep_cfg(&cfg->prep, &src->prep);
            src->prep.change = 0;
        }
        if (src->rc.change) {
            ret |= vp8e_proc_rc_cfg(&cfg->rc, &src->rc);
            src->rc.change = 0;
        }
        if (src->codec.vp8.change) {
            ret |= vp8e_proc_vp8_cfg(&cfg->codec.vp8, &src->codec.vp8);
            src->codec.vp8.change = 0;
        }
        if (src->split.change) {
            ret |= vp8e_proc_split_cfg(&cfg->split, &src->split);
            src->split.change = 0;
        }
    } break;
    case MPP_ENC_SET_RC_CFG : {
        vp8e_dbg_cfg("update vp8e rc config");
        ret |= vp8e_proc_rc_cfg(&cfg->rc, &src->rc);
    } break;
    default: {
        mpp_err("No correspond cmd found, and can not config!");
        ret = MPP_NOK;
    } break;
    }

    vp8e_dbg_fun("leave ret %d\n", ret);
    return ret;
}

static MPP_RET vp8e_proc_hal(void *ctx, HalEncTask *task)
{
    Vp8eCtx *p = (Vp8eCtx *)ctx;
    Vp8eSyntax *syntax = &p->vp8e_syntax[0];
    RK_U32 syn_num = 0;

    syntax[syn_num].type = VP8E_SYN_CFG;
    syntax[syn_num].data = p->cfg;
    syn_num++;
    syntax[syn_num].type = VP8E_SYN_RC;
    syntax[syn_num].data = p->rc;
    syn_num++;
    task->syntax.data = syntax;
    task->syntax.number = syn_num;

    task->valid = 1;
    return MPP_OK;
}

const EncImplApi api_vp8e = {
    .name       = "vp8_control",
    .coding     = MPP_VIDEO_CodingVP8,
    .ctx_size   = sizeof(Vp8eCtx),
    .flag       = 0,
    .init       = vp8e_init,
    .deinit     = vp8e_deinit,
    .proc_cfg   = vp8e_proc_cfg,
    .gen_hdr    = NULL,
    .start      = vp8e_start,
    .proc_dpb   = NULL,
    .proc_hal   = vp8e_proc_hal,
    .add_prefix = NULL,
    .reset      = vp8e_reset,
    .flush      = vp8e_flush,
    .callback   = NULL,
};
