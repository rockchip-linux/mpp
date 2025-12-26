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

#define MODULE_TAG "hal_m2vd_api"

#include <string.h>

#include "mpp_env.h"

#include "mpp_platform.h"
#include "hal_m2vd_base.h"
#include "hal_m2vd_vpu1.h"
#include "hal_m2vd_vpu2.h"

RK_U32 m2vh_debug = 0;

static MPP_RET hal_m2vd_gen_regs(void *hal, HalTaskInfo *task)
{
    M2vdHalCtx *self = (M2vdHalCtx *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->reg_gen)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->reg_gen(hal, task);
}

static MPP_RET hal_m2vd_start(void *hal, HalTaskInfo *task)
{
    M2vdHalCtx *self = (M2vdHalCtx *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->start)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->start(hal, task);
}

static MPP_RET hal_m2vd_wait(void *hal, HalTaskInfo *task)
{
    M2vdHalCtx *self = (M2vdHalCtx *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->wait)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->wait(hal, task);
}

static MPP_RET hal_m2vd_deinit(void *hal)
{
    M2vdHalCtx *self = (M2vdHalCtx *)hal;

    if (NULL == self || NULL == self->hal_api || NULL == self->hal_api->deinit)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->deinit(hal);
}

static MPP_RET hal_m2vd_init(void *hal, MppHalCfg *cfg)
{
    M2vdHalCtx *self = (M2vdHalCtx *)hal;
    VpuHwMode hw_mode = MODE_NULL;
    RK_U32 hw_flag = 0;

    if (NULL == self || NULL == cfg)
        return MPP_ERR_NULL_PTR;

    memset(self, 0, sizeof(M2vdHalCtx));

    mpp_env_get_u32("m2vh_debug", &m2vh_debug, 0);

    hw_flag = mpp_get_vcodec_type();
    if (hw_flag & HAVE_VDPU1)
        hw_mode = VDPU1_MODE;
    if (hw_flag & HAVE_VDPU2)
        hw_mode = VDPU2_MODE;

    switch (hw_mode) {
    case VDPU2_MODE:
        self->hal_api = &hal_m2vd_vdpu2;
        break;
    case VDPU1_MODE:
        self->hal_api = &hal_m2vd_vdpu1;
        break;
    default:
        mpp_err("unknow vpu mode %d.", hw_mode);
        return MPP_ERR_INIT;
    }

    if (NULL == self->hal_api || NULL == self->hal_api->init)
        return MPP_ERR_NULL_PTR;

    return self->hal_api->init(hal, cfg);
}

const MppHalApi hal_api_m2vd = {
    .name = "m2vd_vdpu",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingMPEG2,
    .ctx_size = sizeof(M2vdHalCtx),
    .flag = 0,
    .init = hal_m2vd_init,
    .deinit = hal_m2vd_deinit,
    .reg_gen = hal_m2vd_gen_regs,
    .start = hal_m2vd_start,
    .wait = hal_m2vd_wait,
    .reset = NULL,
    .flush = NULL,
    .control = NULL,
};
