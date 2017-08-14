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

#define MODULE_TAG "hal_vp8e_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp_hal.h"
#include "mpp_platform.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_vepu1.h"
#include "hal_vp8e_vepu2.h"
#include "hal_vp8e_debug.h"

RK_U32 vp8e_hal_debug = 0;

static MPP_RET hal_vp8e_gen_regs(void *hal, HalTaskInfo *task)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    ctx->rc = (Vp8eRc *)task->enc.syntax.data;

    return ctx->hal_api.reg_gen(ctx, task);
}

static MPP_RET hal_vp8e_start(void *hal, HalTaskInfo *task)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    return ctx->hal_api.start(ctx, task);
}

static MPP_RET hal_vp8e_wait(void *hal, HalTaskInfo *task)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    return ctx->hal_api.wait(ctx, task);
}

static MPP_RET hal_vp8e_reset(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    return ctx->hal_api.reset(ctx);
}

static MPP_RET hal_vp8e_flush(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    return ctx->hal_api.flush(ctx);
}

static MPP_RET hal_vp8e_control(void *hal, RK_S32 cmd_type, void *param)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    return ctx->hal_api.control(ctx, cmd_type, param);
}

static MPP_RET hal_vp8e_deinit(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    return ctx->hal_api.deinit(ctx);
}

static MPP_RET hal_vp8e_init(void *hal, MppHalCfg *cfg)
{
    MppHalApi   *p_api = NULL;
    VpuHardMode hard_mode = MODE_NULL;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    if (ctx == NULL)
        return MPP_ERR_VALUE;
    memset(ctx, 0, sizeof(HalVp8eCtx));

    mpp_env_get_u32("vp8e_debug", &vp8e_hal_debug, 0);

    p_api = &ctx->hal_api;
    {
        RK_U32 hw_flag = mpp_get_vcodec_type();
        if (hw_flag & HAVE_VPU2)
            hard_mode = VDPU2_MODE;
        else if (hw_flag & HAVE_VPU1)
            hard_mode = VDPU1_MODE;
        else {
            mpp_err_f("Failed to init due to unsupported hard mode, hw_flag = %d\n", hw_flag);
            return  MPP_ERR_INIT;
        }
    }
    switch (hard_mode) {
    case VDPU2_MODE:
        p_api->init    = hal_vp8e_vepu2_init;
        p_api->deinit  = hal_vp8e_vepu2_deinit;
        p_api->reg_gen = hal_vp8e_vepu2_gen_regs;
        p_api->start   = hal_vp8e_vepu2_start;
        p_api->wait    = hal_vp8e_vepu2_wait;
        p_api->reset   = hal_vp8e_vepu2_reset;
        p_api->flush   = hal_vp8e_vepu2_flush;
        p_api->control = hal_vp8e_vepu2_control;
        break;
    case VDPU1_MODE:
        p_api->init    = hal_vp8e_vepu1_init;
        p_api->deinit  = hal_vp8e_vepu1_deinit;
        p_api->reg_gen = hal_vp8e_vepu1_gen_regs;
        p_api->start   = hal_vp8e_vepu1_start;
        p_api->wait    = hal_vp8e_vepu1_wait;
        p_api->reset   = hal_vp8e_vepu1_reset;
        p_api->flush   = hal_vp8e_vepu1_flush;
        p_api->control = hal_vp8e_vepu1_control;
        break;
    default:
        break;
    }

    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

    return p_api->init(ctx, cfg);
}

const MppHalApi hal_api_vp8e = {
    .name = "vp8e",
    .type = MPP_CTX_ENC,
    .coding = MPP_VIDEO_CodingVP8,
    .ctx_size = sizeof(HalVp8eCtx),
    .flag = 0,
    .init = hal_vp8e_init,
    .deinit = hal_vp8e_deinit,
    .reg_gen = hal_vp8e_gen_regs,
    .start = hal_vp8e_start,
    .wait = hal_vp8e_wait,
    .reset = hal_vp8e_reset,
    .flush = hal_vp8e_flush,
    .control = hal_vp8e_control,
};

