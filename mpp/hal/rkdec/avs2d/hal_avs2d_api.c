/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_avs2d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_device.h"
#include "mpp_platform.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "hal_avs2d_api.h"
#include "hal_avs2d_rkv.h"
#include "hal_avs2d_vdpu382.h"
#include "hal_avs2d_vdpu383.h"

RK_U32 avs2d_hal_debug = 0;

static void explain_input_buffer(Avs2dHalCtx_t *p_hal, HalDecTask *task)
{
    memcpy(&p_hal->syntax, task->syntax.data, sizeof(Avs2dSyntax_t));
}

MPP_RET hal_avs2d_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    AVS2D_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == hal);

    FUN_CHECK(ret = p_hal->hal_api->deinit(hal));

    if (p_hal->buf_group) {
        FUN_CHECK(ret = mpp_buffer_group_put(p_hal->buf_group));
    }

    //!< mpp_device_init
    if (p_hal->dev) {
        ret = mpp_dev_deinit(p_hal->dev);
        if (ret)
            mpp_err("mpp_device_deinit failed. ret: %d\n", ret);
    }

__RETURN:
    AVS2D_HAL_TRACE("Out.");
    return ret;
__FAILED:
    return ret;
}

MPP_RET hal_avs2d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = NULL;
    const MppHalApi *api;

    AVS2D_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == hal);

    mpp_env_get_u32("avs2d_debug", &avs2d_hal_debug, 0);

    p_hal = (Avs2dHalCtx_t *)hal;
    memset(p_hal, 0, sizeof(Avs2dHalCtx_t));


    RK_U32 hw_id = mpp_get_client_hw_id(VPU_CLIENT_RKVDEC);

    switch (hw_id) {
    case HWID_VDPU383 : {
        api = &hal_avs2d_vdpu383;
    } break;
    case HWID_VDPU382_RK3528 : {
        api = &hal_avs2d_vdpu382;
    } break;
    default : {
        api = &hal_avs2d_rkvdpu;
    } break;
    }

    ret = mpp_dev_init(&cfg->dev, VPU_CLIENT_RKVDEC);
    if (ret) {
        mpp_err("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }
    cfg->hw_info = mpp_get_dec_hw_info_by_client_type(VPU_CLIENT_RKVDEC);
    p_hal->hw_info = cfg->hw_info;
    cfg->support_fast_mode = 1;

    p_hal->cfg = cfg->cfg;
    p_hal->dev = cfg->dev;
    p_hal->dec_cb = cfg->dec_cb;
    p_hal->frame_slots  = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;
    p_hal->fast_mode    = cfg->cfg->base.fast_parse && cfg->support_fast_mode;

    //< get buffer group
    if (p_hal->buf_group == NULL)
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_ION));

    //!< run init funtion
    FUN_CHECK(ret = api->init(hal, cfg));

    p_hal->hal_api = api;

__RETURN:
    AVS2D_HAL_TRACE("Out.");
    return ret;
__FAILED:
    hal_avs2d_deinit(hal);
    return ret;
}

MPP_RET hal_avs2d_gen_regs(void *hal, HalTaskInfo *task)
{
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    explain_input_buffer(hal, &task->dec);
    if (!p_hal || !p_hal->hal_api || !p_hal->hal_api->reg_gen)
        return MPP_NOK;

    return p_hal->hal_api->reg_gen(hal, task);
}

#define HAL_AVS2D_TASK_FUNC(func) \
    static MPP_RET hal_avs2d_##func(void *hal, HalTaskInfo *task)   \
    {                                                               \
        Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;                \
                                                                    \
        if (!p_hal || !p_hal->hal_api || !p_hal->hal_api->func)     \
            return MPP_OK;                                          \
                                                                    \
        return p_hal->hal_api->func(hal, task);                     \
    }

HAL_AVS2D_TASK_FUNC(start);
HAL_AVS2D_TASK_FUNC(wait);

const MppHalApi hal_api_avs2d = {
    .name     = "avs2d_rkdec",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVS2,
    .ctx_size = sizeof(Avs2dHalCtx_t),
    .flag     = 0,
    .init     = hal_avs2d_init,
    .deinit   = hal_avs2d_deinit,
    .reg_gen  = hal_avs2d_gen_regs,
    .start    = hal_avs2d_start,
    .wait     = hal_avs2d_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = NULL,
};
