/*
*
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
#define MODULE_TAG "hal_vp8d_api"

#include <string.h>

#include "rk_type.h"
#include "mpp_hal.h"
#include "mpp_platform.h"
#include "mpp_env.h"
#include "hal_vp8d_vdpu1.h"
#include "hal_vp8d_vdpu2.h"

RK_U32 hal_vp8d_debug = 0;

static MPP_RET hal_vp8d_reg_gen(void *hal, HalTaskInfo *task)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->reg_gen)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->reg_gen(hal, task);
}

static MPP_RET hal_vp8d_start(void *hal, HalTaskInfo *task)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->start)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->start(hal, task);
}

static MPP_RET hal_vp8d_wait(void *hal, HalTaskInfo *task)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->wait)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->wait(hal, task);
}

static MPP_RET hal_vp8d_reset(void *hal)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->reset)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->reset(hal);
}

static MPP_RET hal_vp8d_flush(void *hal)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->flush)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->flush(hal);
}

static MPP_RET hal_vp8d_control(void *hal, MpiCmd cmd_type, void *param)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->control)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->control(hal, cmd_type, param);
}

static MPP_RET hal_vp8d_deinit(void *hal)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->deinit)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->deinit(hal);
}

static MPP_RET hal_vp8d_init(void *hal, MppHalCfg *cfg)
{
    VP8DHalContext_t *self = (VP8DHalContext_t *)hal;
    VpuHwMode hw_mode = MODE_NULL;
    RK_U32 hw_flag = 0;

    if (NULL == self || NULL == cfg)
        return MPP_ERR_NULL_PTR;

    mpp_env_get_u32("hal_vp8d_debug", &hal_vp8d_debug, 0);

    memset(self, 0, sizeof(VP8DHalContext_t));

    hw_flag = mpp_get_vcodec_type();
    if (hw_flag & HAVE_VDPU1)
        hw_mode = VDPU1_MODE;
    if (hw_flag & HAVE_VDPU2)
        hw_mode = VDPU2_MODE;

    switch (hw_mode) {
    case VDPU2_MODE:
        self->hal_api = &hal_vp8d_vdpu2;
        break;
    case VDPU1_MODE:
        self->hal_api = &hal_vp8d_vdpu1;
        break;
    default:
        return MPP_ERR_INIT;
        break;
    }

    if (NULL == self->hal_api || NULL == self->hal_api->init)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->init(hal, cfg);
}


const MppHalApi hal_api_vp8d = {
    .name = "vp8d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP8,
    .ctx_size = sizeof(VP8DHalContext_t),
    .flag = 0,
    .init = hal_vp8d_init,
    .deinit = hal_vp8d_deinit,
    .reg_gen = hal_vp8d_reg_gen,
    .start = hal_vp8d_start,
    .wait = hal_vp8d_wait,
    .reset = hal_vp8d_reset,
    .flush = hal_vp8d_flush,
    .control = hal_vp8d_control,
};
