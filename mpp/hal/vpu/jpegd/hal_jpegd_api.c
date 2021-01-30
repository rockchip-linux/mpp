/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_jpegd_api"

#include <string.h>

#include "mpp_env.h"

#include "mpp_hal.h"
#include "mpp_platform.h"
#include "hal_jpegd_base.h"
#include "hal_jpegd_vdpu2.h"
#include "hal_jpegd_vdpu1.h"
#include "hal_jpegd_rkv.h"

static MPP_RET hal_jpegd_reg_gen (void *hal, HalTaskInfo *task)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.reg_gen (hal, task);
}

static MPP_RET hal_jpegd_start (void *hal, HalTaskInfo *task)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.start (hal, task);
}

static MPP_RET hal_jpegd_wait (void *hal, HalTaskInfo *task)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.wait (hal, task);
}

static MPP_RET hal_jpegd_reset (void *hal)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.reset (hal);
}

static MPP_RET hal_jpegd_flush (void *hal)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.flush (hal);
}

static MPP_RET hal_jpegd_control (void *hal, MpiCmd cmd_type, void *param)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.control (hal, cmd_type, param);
}

static MPP_RET hal_jpegd_deinit (void *hal)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    return self->hal_api.deinit (hal);
}

static MPP_RET hal_jpegd_init (void *hal, MppHalCfg *cfg)
{
    JpegdHalCtx *self = (JpegdHalCtx *)hal;
    MppHalApi *p_api = NULL;
    VpuHwMode hw_mode = MODE_NULL;
    RK_U32 hw_flag = 0;

    if (NULL == self)
        return MPP_ERR_VALUE;
    memset(self, 0, sizeof(JpegdHalCtx));

    p_api = &self->hal_api;

    hw_flag = mpp_get_vcodec_type();
    if (hw_flag & HAVE_VDPU2)
        hw_mode = VDPU2_MODE;
    if (hw_flag & HAVE_VDPU1)
        hw_mode = VDPU1_MODE;
    if (hw_flag & HAVE_JPEG_DEC)
        hw_mode = RKVDEC_MODE;

    mpp_env_get_u32("jpegd_mode", &hw_mode, hw_mode);

    switch (hw_mode) {
    case VDPU2_MODE:
        p_api->init = hal_jpegd_vdpu2_init;
        p_api->deinit = hal_jpegd_vdpu2_deinit;
        p_api->reg_gen = hal_jpegd_vdpu2_gen_regs;
        p_api->start = hal_jpegd_vdpu2_start;
        p_api->wait = hal_jpegd_vdpu2_wait;
        p_api->reset = hal_jpegd_vdpu2_reset;
        p_api->flush = hal_jpegd_vdpu2_flush;
        p_api->control = hal_jpegd_vdpu2_control;
        break;
    case VDPU1_MODE:
        p_api->init = hal_jpegd_vdpu1_init;
        p_api->deinit = hal_jpegd_vdpu1_deinit;
        p_api->reg_gen = hal_jpegd_vdpu1_gen_regs;
        p_api->start = hal_jpegd_vdpu1_start;
        p_api->wait = hal_jpegd_vdpu1_wait;
        p_api->reset = hal_jpegd_vdpu1_reset;
        p_api->flush = hal_jpegd_vdpu1_flush;
        p_api->control = hal_jpegd_vdpu1_control;
        break;
    case RKVDEC_MODE:
        p_api->init = hal_jpegd_rkv_init;
        p_api->deinit = hal_jpegd_rkv_deinit;
        p_api->reg_gen = hal_jpegd_rkv_gen_regs;
        p_api->start = hal_jpegd_rkv_start;
        p_api->wait = hal_jpegd_rkv_wait;
        p_api->reset = NULL;
        p_api->flush = NULL;
        p_api->control = hal_jpegd_rkv_control;
        break;
    default:
        return MPP_ERR_INIT;
        break;
    }

    return p_api->init (hal, cfg);
}

const MppHalApi hal_api_jpegd = {
    .name = "jpegd",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingMJPEG,
    .ctx_size = sizeof(JpegdHalCtx),
    .flag = 0,
    .init = hal_jpegd_init,
    .deinit = hal_jpegd_deinit,
    .reg_gen = hal_jpegd_reg_gen,
    .start = hal_jpegd_start,
    .wait = hal_jpegd_wait,
    .reset = hal_jpegd_reset,
    .flush = hal_jpegd_flush,
    .control = hal_jpegd_control,
};
