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

#define MODULE_TAG "hal_jpege_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp_hal.h"

#include "jpege_syntax.h"
#include "hal_jpege_api.h"
#include "hal_jpege_hdr.h"
#include "hal_jpege_base.h"
#include "hal_jpege_vepu1.h"
#include "hal_jpege_vepu2.h"

#include "mpp_device.h"
#include "mpp_platform.h"

RK_U32 hal_jpege_debug = 0;

static MPP_RET hal_jpege_gen_regs(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.reg_gen(ctx, task);
}

static MPP_RET hal_jpege_start(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.start(ctx, task);
}

static MPP_RET hal_jpege_wait(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.wait(ctx, task);
}

static MPP_RET hal_jpege_reset(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.reset(ctx);
}

static MPP_RET hal_jpege_flush(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.flush(ctx);
}

static MPP_RET hal_jpege_control(void *hal, RK_S32 cmd_type, void *param)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.control(ctx, cmd_type, param);
}

static MPP_RET hal_jpege_deinit(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    return ctx->hal_api.deinit(ctx);
}

static MPP_RET hal_jpege_init(void *hal, MppHalCfg *cfg)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    MppHalApi   *p_api = NULL;

    VpuHardMode hard_mode = MODE_NULL;
    RK_U32 hw_flag = 0;

    if (ctx == NULL)
        return MPP_ERR_VALUE;
    memset(ctx, 0, sizeof(HalJpegeCtx));

    p_api = &ctx->hal_api;

    // NOTE: rk3036 and rk3228 do NOT have jpeg encoder
    if (NULL == mpp_get_vcodec_dev_name(MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG)) {
        mpp_err("SOC %s do NOT support jpeg encoding\n", mpp_get_soc_name());
        return MPP_ERR_INIT;
    }

    hw_flag = mpp_get_vcodec_type();
    if (hw_flag & HAVE_VPU2)
        hard_mode = VDPU2_MODE;
    if (hw_flag & HAVE_VPU1)
        hard_mode = VDPU1_MODE;

    switch (hard_mode) {
    case VDPU2_MODE:
        p_api->init    = hal_jpege_vepu2_init;
        p_api->deinit  = hal_jpege_vepu2_deinit;
        p_api->reg_gen = hal_jpege_vepu2_gen_regs;
        p_api->start   = hal_jpege_vepu2_start;
        p_api->wait    = hal_jpege_vepu2_wait;
        p_api->reset   = hal_jpege_vepu2_reset;
        p_api->flush   = hal_jpege_vepu2_flush;
        p_api->control = hal_jpege_vepu2_control;
        break;
    case VDPU1_MODE:
        p_api->init    = hal_jpege_vepu1_init;
        p_api->deinit  = hal_jpege_vepu1_deinit;
        p_api->reg_gen = hal_jpege_vepu1_gen_regs;
        p_api->start   = hal_jpege_vepu1_start;
        p_api->wait    = hal_jpege_vepu1_wait;
        p_api->reset   = hal_jpege_vepu1_reset;
        p_api->flush   = hal_jpege_vepu1_flush;
        p_api->control = hal_jpege_vepu1_control;
        break;
    default:
        return MPP_ERR_INIT;
        break;
    }

    return p_api->init(ctx, cfg);
}

const MppHalApi hal_api_jpege = {
    .name = "jpege",
    .type = MPP_CTX_ENC,
    .coding = MPP_VIDEO_CodingMJPEG,
    .ctx_size = sizeof(HalJpegeCtx),
    .flag = 0,
    .init = hal_jpege_init,
    .deinit = hal_jpege_deinit,
    .reg_gen = hal_jpege_gen_regs,
    .start = hal_jpege_start,
    .wait = hal_jpege_wait,
    .reset = hal_jpege_reset,
    .flush = hal_jpege_flush,
    .control = hal_jpege_control,
};
