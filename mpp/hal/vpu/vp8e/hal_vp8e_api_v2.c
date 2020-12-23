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

#define MODULE_TAG "hal_vp8e_api_v2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp_enc_hal.h"
#include "mpp_hal.h"
#include "mpp_platform.h"

//#include "hal_vp8e_base.h"
#include "hal_vp8e_vepu2_v2.h"
#include "hal_vp8e_vepu1_v2.h"
#include "hal_vp8e_debug.h"
#include "hal_vp8e_api_v2.h"

typedef struct Halvp8eCtx_t {
    const MppEncHalApi  *api;
    void                *hw_ctx;
} Halvp8eCtx;

RK_U32 vp8e_hal_debug = 0;

static MPP_RET hal_vp8e_init(void *hal, MppEncHalCfg *cfg)
{
    const MppEncHalApi  *p_api = NULL;
    Halvp8eCtx *ctx = (Halvp8eCtx *)hal;
    MppClientType type = VPU_CLIENT_BUTT;
    void* hw_ctx = NULL;

    memset(ctx, 0, sizeof(Halvp8eCtx));

    mpp_env_get_u32("vp8e_hal_debug", &vp8e_hal_debug, 0);

    {
        RK_U32 hw_flag = mpp_get_vcodec_type();
        if (hw_flag & HAVE_VEPU2) {
            p_api = &hal_vp8e_vepu2;
            type = VPU_CLIENT_VEPU2;
        } else if (hw_flag & HAVE_VEPU1) {
            p_api = &hal_vp8e_vepu1;
            type = VPU_CLIENT_VEPU1;
        } else {
            mpp_err_f("Failed to init due to unsupported hard mode, hw_flag = %d\n", hw_flag);
            return MPP_ERR_INIT;
        }
    }

    mpp_assert(p_api);
    mpp_assert(type != VPU_CLIENT_BUTT);

    hw_ctx = mpp_calloc_size(void, p_api->ctx_size);
    if (NULL == hw_ctx)
        return MPP_ERR_MALLOC;

    ctx->hw_ctx = hw_ctx;
    ctx->api = p_api;
    cfg->type = type;

    return p_api->init(hw_ctx, cfg);
}

static MPP_RET hal_vp8e_deinit(void *hal)
{
    Halvp8eCtx *ctx = (Halvp8eCtx *)hal;
    const MppEncHalApi *api = ctx->api;
    void *hw_ctx = ctx->hw_ctx;
    MPP_RET ret = MPP_OK;

    if (!hw_ctx || !api || !api->deinit)
        return MPP_OK;

    ret = api->deinit(hw_ctx);
    MPP_FREE(hw_ctx);
    return ret;
}

#define HAL_VP8E_FUNC(func) \
    static MPP_RET hal_vp8e_##func(void *hal)                       \
    {                                                               \
        Halvp8eCtx *ctx = (Halvp8eCtx *)hal;                        \
        const MppEncHalApi *api = ctx->api;                         \
        void *hw_ctx = ctx->hw_ctx;                                 \
                                                                    \
        if (!hw_ctx || !api || !api->func)                          \
            return MPP_OK;                                          \
                                                                    \
        return api->func(hw_ctx);                                   \
    }

#define HAL_VP8E_TASK_FUNC(func) \
    static MPP_RET hal_vp8e_##func(void *hal, HalEncTask *task)     \
    {                                                               \
        Halvp8eCtx *ctx = (Halvp8eCtx *)hal;                        \
        const MppEncHalApi *api = ctx->api;                         \
        void *hw_ctx = ctx->hw_ctx;                                 \
                                                                    \
        if (!hw_ctx || !api || !api->func)                          \
            return MPP_OK;                                          \
                                                                    \
        return api->func(hw_ctx, task);                             \
    }

HAL_VP8E_FUNC(prepare)
HAL_VP8E_TASK_FUNC(get_task)
HAL_VP8E_TASK_FUNC(gen_regs)
HAL_VP8E_TASK_FUNC(start)
HAL_VP8E_TASK_FUNC(wait)
HAL_VP8E_TASK_FUNC(ret_task)

const MppEncHalApi hal_api_vp8e_v2 = {
    .name       = "hal_vp8e",
    .coding     = MPP_VIDEO_CodingVP8,
    .ctx_size   = sizeof(Halvp8eCtx),
    .flag       = 0,
    .init       = hal_vp8e_init,
    .deinit     = hal_vp8e_deinit,
    .prepare    = hal_vp8e_prepare,
    .get_task   = hal_vp8e_get_task,
    .gen_regs   = hal_vp8e_gen_regs,
    .start      = hal_vp8e_start,
    .wait       = hal_vp8e_wait,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_vp8e_ret_task,
};
