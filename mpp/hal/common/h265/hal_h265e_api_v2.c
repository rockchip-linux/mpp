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

#define MODULE_TAG "hal_h265e_api_v2"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"

#include "mpp_platform.h"
#include "vepu5xx.h"
#include "hal_h265e_api_v2.h"
#include "hal_h265e_vepu541.h"
#include "hal_h265e_vepu580.h"

typedef struct HalH265eV2Ctx_t {
    const MppEncHalApi  *api;
    void                *hw_ctx;
} HalH265eV2Ctx;

RK_U32 hal_h265e_debug = 0;

static MPP_RET hal_h265ev2_init(void *hal, MppEncHalCfg *cfg)
{
    HalH265eV2Ctx *ctx = (HalH265eV2Ctx *)hal;
    const MppEncHalApi *api = NULL;
    void *hw_ctx = NULL;
    MPP_RET ret = MPP_OK;
    RK_U32 vcodec_type = mpp_get_vcodec_type();

    if (vcodec_type & HAVE_RKVENC) {
        RK_U32 hw_id = mpp_get_client_hw_id(VPU_CLIENT_RKVENC);

        switch (hw_id) {
        case HWID_VEPU58X : {
            api = &hal_h265e_vepu580;
        } break;
        default : {
            api = &hal_h265e_vepu541;
        } break;
        }
    } else {
        mpp_err("vcodec type %08x can not find H.265 encoder device\n",
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

static MPP_RET hal_h265ev2_deinit(void *hal)
{
    HalH265eV2Ctx *ctx = (HalH265eV2Ctx *)hal;
    const MppEncHalApi *api = ctx->api;
    void *hw_ctx = ctx->hw_ctx;
    MPP_RET ret = MPP_OK;

    if (!hw_ctx || !api || !api->deinit)
        return MPP_OK;

    ret = api->deinit(hw_ctx);
    MPP_FREE(hw_ctx);
    return ret;
}

#define HAL_H265E_FUNC(func) \
    static MPP_RET hal_h265ev2_##func(void *hal)                    \
    {                                                               \
        HalH265eV2Ctx *ctx = (HalH265eV2Ctx *)hal;                  \
        const MppEncHalApi *api = ctx->api;                         \
        void *hw_ctx = ctx->hw_ctx;                                 \
                                                                    \
        if (!hw_ctx || !api || !api->func)                          \
            return MPP_OK;                                          \
                                                                    \
        return api->func(hw_ctx);                                   \
    }

#define HAL_H265E_TASK_FUNC(func) \
    static MPP_RET hal_h265ev2_##func(void *hal, HalEncTask *task)  \
    {                                                               \
        HalH265eV2Ctx *ctx = (HalH265eV2Ctx *)hal;                  \
        const MppEncHalApi *api = ctx->api;                         \
        void *hw_ctx = ctx->hw_ctx;                                 \
                                                                    \
        if (!hw_ctx || !api || !api->func)                          \
            return MPP_OK;                                          \
                                                                    \
        return api->func(hw_ctx, task);                             \
    }

HAL_H265E_FUNC(prepare)
HAL_H265E_TASK_FUNC(get_task)
HAL_H265E_TASK_FUNC(gen_regs)
HAL_H265E_TASK_FUNC(start)
HAL_H265E_TASK_FUNC(wait)
HAL_H265E_TASK_FUNC(part_start)
HAL_H265E_TASK_FUNC(part_wait)
HAL_H265E_TASK_FUNC(ret_task)

const MppEncHalApi hal_api_h265e_v2 = {
    .name       = "hal_h265e",
    .coding     = MPP_VIDEO_CodingHEVC,
    .ctx_size   = sizeof(HalH265eV2Ctx),
    .flag       = 0,
    .init       = hal_h265ev2_init,
    .deinit     = hal_h265ev2_deinit,
    .prepare    = hal_h265ev2_prepare,
    .get_task   = hal_h265ev2_get_task,
    .gen_regs   = hal_h265ev2_gen_regs,
    .start      = hal_h265ev2_start,
    .wait       = hal_h265ev2_wait,
    .part_start = hal_h265ev2_part_start,
    .part_wait  = hal_h265ev2_part_wait,
    .ret_task   = hal_h265ev2_ret_task,
};
