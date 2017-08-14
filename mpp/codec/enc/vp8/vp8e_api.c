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
#define MODULE_TAG  "vp8e_api"

#include <string.h>
#include <vp8e_syntax.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_rc.h"
#include "mpp_controller.h"

#include "vp8e_api.h"
#include "vp8e_rc.h"
#include "vp8e_syntax.h"
#include "vp8e_debug.h"

RK_U32 vp8e_rc_debug = 0;

typedef struct {
    /* config from mpp_enc */
    MppEncCfgSet    *cfg;
    MppEncCfgSet    *set;

    /* internal rate control config*/
    RK_U32          rc_ready;
    RK_U32          prep_ready;
//    MppRateControl  *rc;
    Vp8eRc          *rc;

    /* output to hal */
    RcSyntax        syntax;

    RcHalResult     result;

} Vp8eCtx;

MPP_RET vp8e_init(void *ctx, ControllerCfg *ctrl_cfg)
{
    MPP_RET ret = MPP_OK;
    Vp8eCtx *p = (Vp8eCtx *)ctx;

    MppEncRcCfg *rc_cfg = &ctrl_cfg->cfg->rc;
    MppEncPrepCfg *prep = &ctrl_cfg->cfg->prep;

    vp8e_rc_dbg_func("enter\n");
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
//    prep->format = MPP_FMT_YUV420SP;
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
    rc_cfg->skip_cnt = 0;


    p->rc = mpp_calloc(Vp8eRc, 1);
    if (NULL == p->rc) {
        mpp_err_f("failed to malloc vp8_rc\n");
        ret = MPP_ERR_MALLOC;
        goto __ERR_RET;
    }

    vp8e_init_rc(p->rc, ctrl_cfg->cfg);

    mpp_env_get_u32("vp8e_debug", &vp8e_rc_debug, 0);

    vp8e_rc_dbg_func("leave ret %d\n", ret);
    return ret;

__ERR_RET:
    vp8e_rc_dbg_func("leave ret %d\n", ret);
    return ret;
}

MPP_RET vp8e_deinit(void *ctx)
{
    Vp8eCtx *p = (Vp8eCtx *)ctx;

    vp8e_rc_dbg_func("enter\n");
    if (p) {
        if (p->rc)
            mpp_free(p->rc);
        mpp_free(p);
    }

    vp8e_rc_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET vp8e_encode(void *ctx, HalEncTask *task)
{
    Vp8eCtx *p = (Vp8eCtx *)ctx;

    RcSyntax *rc_syn = &p->syntax;
    MppEncCfgSet *cfg = p->cfg;

    vp8e_rc_dbg_func("enter\n");


    vp8e_before_pic_rc(p->rc);

    if (rc_syn->bit_target <= 0) {
        RK_S32 mb_width = MPP_ALIGN(cfg->prep.width, 16) >> 4;
        RK_S32 mb_height = MPP_ALIGN(cfg->prep.height, 16) >> 4;
        rc_syn->bit_target = mb_width * mb_height;
    }

    task->syntax.data   = p->rc;
    task->syntax.number = 1;
    task->valid = 1;
    task->is_intra = p->rc->curr_frame_intra;

    vp8e_rc_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET vp8e_reset(void *ctx)
{
    (void)ctx;

    return MPP_OK;
}


MPP_RET vp8e_flush(void *ctx)
{
    Vp8eCtx *p = (Vp8eCtx *)ctx;
    (void)p;

    return MPP_OK;
}

MPP_RET vp8e_config(void *ctx, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    Vp8eCtx *p = (Vp8eCtx *)ctx;

    vp8e_rc_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);
    switch (cmd) {
    case MPP_ENC_SET_RC_CFG : {
        vp8e_rc_dbg_cfg("update vp8e rc config");
        vp8e_update_rc_cfg(p->rc, &p->set->rc);
        vp8e_init_rc(p->rc, p->set);
    } break;
    default: {
        mpp_err("No correspond cmd found, and can not config!");
        ret = MPP_NOK;
    } break;
    }

    vp8e_rc_dbg_func("leave ret %d\n", ret);
    return ret;
}

MPP_RET vp8e_callback(void *ctx, void *feedback)
{
    vp8e_rc_dbg_func("enter\n");
    Vp8eCtx *p = (Vp8eCtx *)ctx;
    Vp8eFeedback *fb = (Vp8eFeedback *)feedback;
    RcHalResult *result = (RcHalResult *)fb->result;

    vp8e_after_pic_rc(p->rc, result->bits);
    p->rc->curr_frame_intra = (result->type == INTRA_FRAME);

    vp8e_rc_dbg_func("leave\n");
    return MPP_OK;
}

const ControlApi api_vp8e_controller = {
    "vp8e_control",
    MPP_VIDEO_CodingVP8,
    sizeof(Vp8eCtx),
    0,
    vp8e_init,
    vp8e_deinit,
    vp8e_encode,
    vp8e_reset,
    vp8e_flush,
    vp8e_config,
    vp8e_callback,
};

