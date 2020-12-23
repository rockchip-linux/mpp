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

#define MODULE_TAG "hal_h264e_api_v2"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_platform.h"

#include "mpp_enc_hal.h"

#include "hal_h264e_debug.h"
#include "h264e_syntax.h"
#include "hal_h264e_api_v2.h"
#include "hal_h264e_vepu1_v2.h"
#include "hal_h264e_vepu2_v2.h"
#include "hal_h264e_vepu541.h"

typedef struct HalH264eCtx_t {
    const MppEncHalApi  *api;
    void                *hw_ctx;
} HalH264eCtx;

RK_U32 hal_h264e_debug = 0;

static MPP_RET hal_h264e_init(void *hal, MppEncHalCfg *cfg)
{
    HalH264eCtx *ctx = (HalH264eCtx *)hal;
    const MppEncHalApi *api = NULL;
    void *hw_ctx = NULL;
    MPP_RET ret = MPP_OK;
    RK_U32 vcodec_type = mpp_get_vcodec_type();

    mpp_env_get_u32("hal_h264e_debug", &hal_h264e_debug, 0);

    if (vcodec_type & HAVE_RKVENC) {
        api = &hal_h264e_vepu541;
    } else if (vcodec_type & HAVE_VEPU2) {
        api = &hal_h264e_vepu2;
    } else if (vcodec_type & HAVE_VEPU1) {
        api = &hal_h264e_vepu1;
    } else {
        mpp_err("vcodec type %08x can not find H.264 encoder device\n",
                vcodec_type);
        ret = MPP_NOK;
    }

    mpp_assert(api);

    if (!ret)
        hw_ctx = mpp_calloc_size(void, api->ctx_size);

    ctx->api = api;
    ctx->hw_ctx = hw_ctx;

    if (ret)
        return ret;

    ret = api->init(hw_ctx, cfg);
    return ret;
}

static MPP_RET hal_h264e_deinit(void *hal)
{
    HalH264eCtx *ctx = (HalH264eCtx *)hal;
    const MppEncHalApi *api = ctx->api;
    void *hw_ctx = ctx->hw_ctx;
    MPP_RET ret = MPP_OK;

    if (!hw_ctx || !api || !api->deinit)
        return MPP_OK;

    ret = api->deinit(hw_ctx);
    MPP_FREE(hw_ctx);
    return ret;
}

#define HAL_H264E_FUNC(func) \
    static MPP_RET hal_h264e_##func(void *hal)                      \
    {                                                               \
        HalH264eCtx *ctx = (HalH264eCtx *)hal;                      \
        const MppEncHalApi *api = ctx->api;                         \
        void *hw_ctx = ctx->hw_ctx;                                 \
                                                                    \
        if (!hw_ctx || !api || !api->func)                          \
            return MPP_OK;                                          \
                                                                    \
        return api->func(hw_ctx);                                   \
    }

#define HAL_H264E_TASK_FUNC(func) \
    static MPP_RET hal_h264e_##func(void *hal, HalEncTask *task)    \
    {                                                               \
        HalH264eCtx *ctx = (HalH264eCtx *)hal;                      \
        const MppEncHalApi *api = ctx->api;                         \
        void *hw_ctx = ctx->hw_ctx;                                 \
                                                                    \
        if (!hw_ctx || !api || !api->func)                          \
            return MPP_OK;                                          \
                                                                    \
        return api->func(hw_ctx, task);                             \
    }

HAL_H264E_FUNC(prepare)
HAL_H264E_TASK_FUNC(get_task)
HAL_H264E_TASK_FUNC(gen_regs)
HAL_H264E_TASK_FUNC(start)
HAL_H264E_TASK_FUNC(wait)
HAL_H264E_TASK_FUNC(part_start)
HAL_H264E_TASK_FUNC(part_wait)
HAL_H264E_TASK_FUNC(ret_task)

const MppEncHalApi hal_api_h264e_v2 = {
    .name       = "hal_h264e",
    .coding     = MPP_VIDEO_CodingAVC,
    .ctx_size   = sizeof(HalH264eCtx),
    .flag       = 0,
    .init       = hal_h264e_init,
    .deinit     = hal_h264e_deinit,
    .prepare    = hal_h264e_prepare,
    .get_task   = hal_h264e_get_task,
    .gen_regs   = hal_h264e_gen_regs,
    .start      = hal_h264e_start,
    .wait       = hal_h264e_wait,
    .part_start = hal_h264e_part_start,
    .part_wait  = hal_h264e_part_wait,
    .ret_task   = hal_h264e_ret_task,
};
