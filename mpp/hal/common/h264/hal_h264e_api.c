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

#define MODULE_TAG "hal_h264e_api"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpp_common.h"
#include "mpp_platform.h"
#include "mpp_device.h"
#include "mpp_hal.h"
#include "mpp_env.h"
#include "hal_h264e_api.h"
#include "hal_h264e_com.h"
#include "hal_h264e_vepu2.h"
#include "hal_h264e_rkv.h"
#include "hal_h264e_vepu1.h"

RK_U32 hal_h264e_debug = 0;

MPP_RET hal_h264e_init(void *hal, MppHalCfg *cfg)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;
    MppEncCodecCfg *codec = &cfg->cfg->codec;
    MppEncH264Cfg *h264 = &codec->h264;
    MppEncH264VuiCfg *vui = &h264->vui;
    MppEncH264RefCfg *ref = &h264->ref;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_U32 vcodec_type = 0;

    mpp_env_get_u32("hal_h264e_debug", &hal_h264e_debug, 0x00000001);

    vcodec_type = mpp_get_vcodec_type();
    if (vcodec_type & HAVE_RKVENC) {
        api->init    = hal_h264e_rkv_init;
        api->deinit  = hal_h264e_rkv_deinit;
        api->reg_gen = hal_h264e_rkv_gen_regs;
        api->start   = hal_h264e_rkv_start;
        api->wait    = hal_h264e_rkv_wait;
        api->reset   = hal_h264e_rkv_reset;
        api->flush   = hal_h264e_rkv_flush;
        api->control = hal_h264e_rkv_control;
        hw_cfg->hw_type = H264E_RKV;
    } else if (vcodec_type & HAVE_VPU2) {
        api->init    = hal_h264e_vepu2_init;
        api->deinit  = hal_h264e_vepu2_deinit;
        api->reg_gen = hal_h264e_vepu2_gen_regs;
        api->start   = hal_h264e_vepu2_start;
        api->wait    = hal_h264e_vepu2_wait;
        api->reset   = hal_h264e_vepu2_reset;
        api->flush   = hal_h264e_vepu2_flush;
        api->control = hal_h264e_vepu2_control;
        hw_cfg->hw_type = H264E_VPU;
    } else if (vcodec_type & HAVE_VPU1) {
        api->init = hal_h264e_vepu1_init;
        api->deinit  = hal_h264e_vepu1_deinit;
        api->reg_gen = hal_h264e_vepu1_gen_regs;
        api->start   = hal_h264e_vepu1_start;
        api->wait    = hal_h264e_vepu1_wait;
        api->reset   = hal_h264e_vepu1_reset;
        api->flush   = hal_h264e_vepu1_flush;
        api->control = hal_h264e_vepu1_control;
        hw_cfg->hw_type = H264E_VPU;
    } else {
        mpp_err("vcodec type %08x can not find H.264 encoder device\n",
                vcodec_type);
        return MPP_NOK;
    }

    /*
     * default codec:
     * High Profile
     * frame mode
     * all flag enabled
     */
    codec->change = 0;
    codec->coding = MPP_VIDEO_CodingAVC;
    h264->profile = 100;
    h264->level = 31;
    h264->entropy_coding_mode = 1;
    h264->cabac_init_idc = 0;
    h264->transform8x8_mode = 1;
    h264->constrained_intra_pred_mode = 0;
    h264->chroma_cb_qp_offset = 0;
    h264->chroma_cr_qp_offset = 0;
    h264->deblock_disable = 0;
    h264->deblock_offset_alpha = 0;
    h264->deblock_offset_beta = 0;
    h264->use_longterm = 0;
    h264->qp_init = 26;
    h264->qp_max = 48;
    h264->qp_min = 16;
    h264->qp_max_step = 8;
    h264->intra_refresh_mode = 0;
    h264->intra_refresh_arg = 0;
    h264->slice_mode = 0;
    h264->slice_arg = 0;
    h264->vui.change = 0;
    h264->sei.change = 0;

    vui->b_vui          = 1;

    ref->i_frame_reference = H264E_NUM_REFS;
    ref->i_dpb_size = H264E_NUM_REFS;
    ref->i_ref_pos = 1;
    ref->i_long_term_en = H264E_LONGTERM_REF_EN;
    ref->hw_longterm_mode = 0;
    ref->i_long_term_internal = 0;

    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

    ctx->sei_mode = 0;
    ctx->qp_scale = 1;

    return api->init(hal, cfg);
}

MPP_RET hal_h264e_deinit(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->deinit(hal);
}

MPP_RET hal_h264e_gen_regs(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->reg_gen(hal, task);
}

MPP_RET hal_h264e_start(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->start(hal, task);
}

MPP_RET hal_h264e_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->wait(hal, task);
}

MPP_RET hal_h264e_reset(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->reset(hal);
}

MPP_RET hal_h264e_flush(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->flush(hal);
}

MPP_RET hal_h264e_control(void *hal, RK_S32 cmd_type, void *param)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppHalApi *api = &ctx->api;

    return api->control(hal, cmd_type, param);
}


const MppHalApi hal_api_h264e = {
    .name = "h264e_rkvenc",
    .type = MPP_CTX_ENC,
    .coding = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(H264eHalContext),
    .flag = 0,
    .init = hal_h264e_init,
    .deinit = hal_h264e_deinit,
    .reg_gen = hal_h264e_gen_regs,
    .start = hal_h264e_start,
    .wait = hal_h264e_wait,
    .reset = hal_h264e_reset,
    .flush = hal_h264e_flush,
    .control = hal_h264e_control,
};


