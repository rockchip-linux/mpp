/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_platform.h"

#include "mpp_hal.h"
#include "hal_h263d_base.h"
#include "hal_h263d_vdpu1.h"
#include "hal_h263d_vdpu2.h"

RK_U32 h263d_hal_debug = 0;

void vpu_h263d_get_buffer_by_index(hal_h263_ctx *ctx, RK_S32 index,
                                   MppBuffer *buffer)
{
    if (index >= 0) {
        mpp_buf_slot_get_prop(ctx->frm_slots, index, SLOT_BUFFER, buffer);
        mpp_assert(*buffer);
    }
}

static MPP_RET hal_h263d_gen_regs(void *hal, HalTaskInfo *task)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.reg_gen(hal, task);
}

static MPP_RET hal_h263d_start(void *hal, HalTaskInfo *task)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.start(hal, task);
}

static MPP_RET hal_h263d_wait(void *hal, HalTaskInfo *task)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.wait(hal, task);
}

static MPP_RET hal_h263d_reset(void *hal)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.reset(hal);
}

static MPP_RET hal_h263d_flush(void *hal)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.flush(hal);
}

static MPP_RET hal_h263d_control(void *hal, RK_S32 cmd_type, void *param)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.control(hal, cmd_type, param);
}

static MPP_RET hal_h263d_deinit(void *hal)
{
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    return ctx->hal_api.deinit(hal);
}

static MPP_RET hal_h263d_init(void *hal, MppHalCfg *cfg)
{
    MppHalApi *p_api = NULL;
    hal_h263_ctx *p_hal = (hal_h263_ctx *)hal;
    VpuHardMode hard_mode = MODE_NULL;
    RK_U32 hw_flag = 0;

    mpp_env_get_u32("h263d_hal_debug", &h263d_hal_debug, 0);

    memset(p_hal, 0, sizeof(hal_h263_ctx));
    p_api = &p_hal->hal_api;

    p_hal->frm_slots = cfg->frame_slots;
    p_hal->pkt_slots = cfg->packet_slots;

    hw_flag = mpp_get_vcodec_type();
    if (hw_flag & HAVE_VPU2)
        hard_mode = VDPU2_MODE;
    if (hw_flag & HAVE_VPU1)
        hard_mode = VDPU1_MODE;

    switch (hard_mode) {
    case VDPU2_MODE : {
        mpp_log("the VDPU2_MODE is used currently!\n");
        p_api->init    = hal_vpu2_h263d_init;
        p_api->deinit  = hal_vpu2_h263d_deinit;
        p_api->reg_gen = hal_vpu2_h263d_gen_regs;
        p_api->start   = hal_vpu2_h263d_start;
        p_api->wait    = hal_vpu2_h263d_wait;
        p_api->reset   = hal_vpu2_h263d_reset;
        p_api->flush   = hal_vpu2_h263d_flush;
        p_api->control = hal_vpu2_h263d_control;
    } break;
    case VDPU1_MODE : {
        mpp_log("the VDPU1_MODE is used currently!\n");
        p_api->init    = hal_vpu1_h263d_init;
        p_api->deinit  = hal_vpu1_h263d_deinit;
        p_api->reg_gen = hal_vpu1_h263d_gen_regs;
        p_api->start   = hal_vpu1_h263d_start;
        p_api->wait    = hal_vpu1_h263d_wait;
        p_api->reset   = hal_vpu1_h263d_reset;
        p_api->flush   = hal_vpu1_h263d_flush;
        p_api->control = hal_vpu1_h263d_control;
    } break;
    default : {
        mpp_err("unknow vpu type:%d.", hard_mode);
        return MPP_ERR_INIT;
    } break;
    }

    return p_api->init(hal, cfg);
}

const MppHalApi hal_api_h263d = {
    "h263d_vpu",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingH263,
    sizeof(hal_h263_ctx),
    0,
    hal_h263d_init,
    hal_h263d_deinit,
    hal_h263d_gen_regs,
    hal_h263d_start,
    hal_h263d_wait,
    hal_h263d_reset,
    hal_h263d_flush,
    hal_h263d_control,
};
