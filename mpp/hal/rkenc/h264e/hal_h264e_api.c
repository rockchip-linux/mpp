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
#ifdef RKPLATFORM
#include <unistd.h>
#endif

#include "mpp_common.h"
#include "vpu.h"
#include "mpp_hal.h"
#include "mpp_env.h"
#include "hal_h264e_api.h"
#include "hal_h264e.h"
#include "hal_h264e_vpu.h"
#include "hal_h264e_rkv.h"

RK_U32 h264e_hal_log_mode = 0;

MPP_RET hal_h264e_init(void *hal, MppHalCfg *cfg)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    mpp_env_get_u32("h264e_hal_debug", &h264e_hal_log_mode, 0x00000111);
    if (!access("/dev/rkvenc", F_OK))
        cfg->device_id = HAL_RKVENC;
    else
        cfg->device_id = HAL_VEPU;

    switch (cfg->device_id) {
    case HAL_VEPU:
        api->init    = hal_h264e_vpu_init;
        api->deinit  = hal_h264e_vpu_deinit;
        api->reg_gen = hal_h264e_vpu_gen_regs;
        api->start   = hal_h264e_vpu_start;
        api->wait    = hal_h264e_vpu_wait;
        api->reset   = hal_h264e_vpu_reset;
        api->flush   = hal_h264e_vpu_flush;
        api->control = hal_h264e_vpu_control;
        break;
    case HAL_RKVENC:
        api->init    = hal_h264e_rkv_init;
        api->deinit  = hal_h264e_rkv_deinit;
        api->reg_gen = hal_h264e_rkv_gen_regs;
        api->start   = hal_h264e_rkv_start;
        api->wait    = hal_h264e_rkv_wait;
        api->reset   = hal_h264e_rkv_reset;
        api->flush   = hal_h264e_rkv_flush;
        api->control = hal_h264e_rkv_control;
        break;
    default:
        mpp_err("invalid device_id: %d", cfg->device_id);
        return MPP_NOK;
    }

    return api->init(hal, cfg);
}

MPP_RET hal_h264e_deinit(void *hal)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->deinit(hal);
}

MPP_RET hal_h264e_gen_regs(void *hal, HalTaskInfo *task)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->reg_gen(hal, task);
}

MPP_RET hal_h264e_start(void *hal, HalTaskInfo *task)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->start(hal, task);
}

MPP_RET hal_h264e_wait(void *hal, HalTaskInfo *task)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->wait(hal, task);
}

MPP_RET hal_h264e_reset(void *hal)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->reset(hal);
}

MPP_RET hal_h264e_flush(void *hal)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->flush(hal);
}

MPP_RET hal_h264e_control(void *hal, RK_S32 cmd_type, void *param)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    MppHalApi *api = &ctx->api;

    return api->control(hal, cmd_type, param);
}


const MppHalApi hal_api_h264e = {
    "h264e_rkvenc",
    MPP_CTX_ENC,
    MPP_VIDEO_CodingAVC,
    sizeof(h264e_hal_context),
    0,
    hal_h264e_init,
    hal_h264e_deinit,
    hal_h264e_gen_regs,
    hal_h264e_start,
    hal_h264e_wait,
    hal_h264e_reset,
    hal_h264e_flush,
    hal_h264e_control,
};


