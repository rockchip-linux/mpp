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

#define MODULE_TAG "hal_av1d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_platform.h"
#include "mpp_common.h"

#include "dxva_syntax.h"

#include "hal_av1d_vdpu_reg.h"
#include "hal_av1d_vdpu.h"
#include "hal_av1d_common.h"

RK_U32 hal_av1d_debug = 0;

MPP_RET hal_av1d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret         = MPP_OK;
    Av1dHalCtx *p_hal   = (Av1dHalCtx *)hal;
    RK_U32 vcodec_type  = mpp_get_vcodec_type();
    MppClientType type  = VPU_CLIENT_AV1DEC;

    INP_CHECK(ret, NULL == p_hal);
    memset(p_hal, 0, sizeof(Av1dHalCtx));
    mpp_env_get_u32("hal_av1d_debug", &hal_av1d_debug, 0);

    // check codec_type first
    if (!(vcodec_type & (HAVE_RKVDEC | HAVE_VDPU1 | HAVE_VDPU2))) {
        mpp_err_f("can not found av1 decoder hardware on platform %x\n", vcodec_type);
        return ret;
    }

    p_hal->api = &hal_av1d_vdpu;

    //!< callback function to parser module
    p_hal->dec_cb = cfg->dec_cb;

    ret = mpp_dev_init(&cfg->dev, type);
    if (ret) {
        mpp_err("mpp_dev_init failed ret: %d\n", ret);
        goto __FAILED;
    }

    //< get buffer group
    if (p_hal->buf_group == NULL) {
        FUN_CHECK(ret = mpp_buffer_group_get_internal
                        (&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
    }

    p_hal->dev          = cfg->dev;
    p_hal->cfg          = cfg->cfg;
    p_hal->slots        = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;
    p_hal->fast_mode    = cfg->cfg->base.fast_parse;

    if (p_hal->buf_group == NULL) {
        FUN_CHECK(ret = mpp_buffer_group_get_internal
                        (&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
    }
    //!< run init funtion
    FUN_CHECK(ret = p_hal->api->init(hal, cfg));

__RETURN:
    return MPP_OK;
__FAILED:
    return ret;
}

MPP_RET hal_av1d_deinit(void *hal)
{
    MPP_RET ret         = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal   = (Av1dHalCtx *)hal;

    FUN_CHECK(ret = p_hal->api->deinit(hal));

    if (p_hal->dev) {
        mpp_dev_deinit(p_hal->dev);
        p_hal->dev = NULL;
    }

    if (p_hal->buf_group) {
        FUN_CHECK(ret = mpp_buffer_group_put(p_hal->buf_group));
    }

    return MPP_OK;
__FAILED:
    return ret;
}

MPP_RET hal_av1d_gen_regs(void *hal, HalTaskInfo *task)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_NOK;

    if (p_hal && p_hal->api && p_hal->api->reg_gen)
        ret = p_hal->api->reg_gen(hal, task);
    return ret;
}

MPP_RET hal_av1d_start(void *hal, HalTaskInfo *task)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_NOK;

    if (p_hal && p_hal->api && p_hal->api->start)
        ret = p_hal->api->start(hal, task);
    return ret;
}

MPP_RET hal_av1d_wait(void *hal, HalTaskInfo *task)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_NOK;

    if (p_hal && p_hal->api && p_hal->api->wait)
        ret = p_hal->api->wait(hal, task);
    return ret;
}

MPP_RET hal_av1d_reset(void *hal)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_NOK;

    if (p_hal && p_hal->api && p_hal->api->reset)
        ret = p_hal->api->reset(hal);
    return ret;
}

MPP_RET hal_av1d_flush(void *hal)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_NOK;

    if (p_hal && p_hal->api && p_hal->api->flush)
        ret = p_hal->api->flush(hal);
    return ret;
}

MPP_RET hal_av1d_control(void *hal, MpiCmd cmd_type, void *param)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_NOK;

    if (p_hal && p_hal->api && p_hal->api->control)
        ret = p_hal->api->control(hal, cmd_type, param);
    return ret;
}

const MppHalApi hal_api_av1d = {
    .name       = "av1d_vdpu",
    .type       = MPP_CTX_DEC,
    .coding     = MPP_VIDEO_CodingAV1,
    .ctx_size   = sizeof(Av1dHalCtx),
    .flag       = 0,
    .init       = hal_av1d_init,
    .deinit     = hal_av1d_deinit,
    .reg_gen    = hal_av1d_gen_regs,
    .start      = hal_av1d_start,
    .wait       = hal_av1d_wait,
    .reset      = hal_av1d_reset,
    .flush      = hal_av1d_flush,
    .control    = hal_av1d_control,
};
