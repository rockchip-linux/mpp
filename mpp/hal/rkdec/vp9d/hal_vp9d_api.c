/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_vp9d_api"

#include <string.h>

#include "mpp_env.h"

#include "hal_vp9d_debug.h"
#include "hal_vp9d_api.h"
#include "hal_vp9d_ctx.h"
#include "hal_vp9d_rkv.h"
#include "hal_vp9d_vdpu34x.h"

RK_U32 hal_vp9d_debug = 0;

MPP_RET hal_vp9d_init(void *ctx, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;
    MppClientType client_type = VPU_CLIENT_RKVDEC;
    RK_U32 hw_id = 0;

    ret = mpp_dev_init(&cfg->dev, client_type);
    if (ret) {
        mpp_err("mpp_dev_init failed ret: %d\n", ret);
        return ret;
    }

    hw_id = mpp_get_client_hw_id(client_type);
    p->dev = cfg->dev;
    p->hw_id = hw_id;
    p->client_type = client_type;
    if (hw_id == HWID_VDPU34X || hw_id == HWID_VDPU38X)
        p->api = &hal_vp9d_vdpu34x;
    else
        p->api = &hal_vp9d_rkv;

    cfg->support_fast_mode = 1;

    p->slots = cfg->frame_slots;
    p->dec_cb = cfg->dec_cb;
    p->fast_mode = cfg->cfg->base.fast_parse;
    p->packet_slots = cfg->packet_slots;

    mpp_env_get_u32("hal_vp9d_debug", &hal_vp9d_debug, 0);

    ret = p->api->init(ctx, cfg);

    return ret;
}

MPP_RET hal_vp9d_deinit(void *ctx)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->deinit)
        ret = p->api->deinit(ctx);

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }

    return ret;
}

MPP_RET hal_vp9d_gen_regs(void *ctx, HalTaskInfo *task)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->reg_gen)
        ret = p->api->reg_gen(ctx, task);

    return ret;
}

MPP_RET hal_vp9d_start(void *ctx, HalTaskInfo *task)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->start)
        ret = p->api->start(ctx, task);

    return ret;
}

MPP_RET hal_vp9d_wait(void *ctx, HalTaskInfo *task)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->wait)
        ret = p->api->wait(ctx, task);

    return ret;
}

MPP_RET hal_vp9d_reset(void *ctx)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->reset)
        ret = p->api->reset(ctx);

    return ret;
}

MPP_RET hal_vp9d_flush(void *ctx)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->flush)
        ret = p->api->flush(ctx);

    return ret;
}

MPP_RET hal_vp9d_control(void *ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_NOK;
    HalVp9dCtx *p = (HalVp9dCtx *)ctx;

    if (p && p->api && p->api->control)
        ret = p->api->control(ctx, cmd, param);

    return ret;
}

const MppHalApi hal_api_vp9d = {
    .name = "vp9d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(HalVp9dCtx),
    .flag = 0,
    .init = hal_vp9d_init,
    .deinit = hal_vp9d_deinit,
    .reg_gen = hal_vp9d_gen_regs,
    .start = hal_vp9d_start,
    .wait = hal_vp9d_wait,
    .reset = hal_vp9d_reset,
    .flush = hal_vp9d_flush,
    .control = hal_vp9d_control,
};
