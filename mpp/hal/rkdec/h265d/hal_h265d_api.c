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

#define MODULE_TAG "hal_h265d_api"

#include <stdio.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"

#include "hal_h265d_ctx.h"
#include "hal_h265d_api.h"
#include "hal_h265d_rkv.h"
#include "hal_h265d_vdpu34x.h"

RK_U32 hal_h265d_debug = 0;

MPP_RET hal_h265d_init(void *ctx, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;
    MppClientType client_type = VPU_CLIENT_BUTT;
    RK_U32 vcodec_type = mpp_get_vcodec_type();
    RK_U32 hw_id = 0;

    if (!(vcodec_type & (HAVE_RKVDEC | HAVE_HEVC_DEC))) {
        mpp_err_f("Can not found valid H.265 decoder hardware on platform %08x\n", vcodec_type);
        return ret;
    }

    client_type = (vcodec_type & HAVE_HEVC_DEC) ?
                  VPU_CLIENT_HEVC_DEC : VPU_CLIENT_RKVDEC;

    ret = mpp_dev_init(&cfg->dev, client_type);
    if (ret) {
        mpp_err("mpp_dev_init failed ret: %d\n", ret);
        return ret;
    }

    hw_id = mpp_get_client_hw_id(client_type);
    p->dev = cfg->dev;
    p->is_v345 = (hw_id == HWID_VDPU345);
    p->is_v34x = (hw_id == HWID_VDPU34X);
    p->client_type = client_type;

    if (hw_id == HWID_VDPU34X)
        p->api = &hal_h265d_vdpu34x;
    else
        p->api = &hal_h265d_rkv;

    p->slots = cfg->frame_slots;
    p->dec_cb = cfg->dec_cb;
    p->fast_mode = cfg->cfg->base.fast_parse;
    p->packet_slots = cfg->packet_slots;

    mpp_env_get_u32("hal_h265d_debug", &hal_h265d_debug, 0);

    ret = p->api->init(ctx, cfg);

    return ret;
}

MPP_RET hal_h265d_deinit(void *ctx)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->deinit)
        ret = p->api->deinit(ctx);

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }

    return ret;
}

MPP_RET hal_h265d_gen_regs(void *ctx, HalTaskInfo *task)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->reg_gen)
        ret = p->api->reg_gen(ctx, task);

    return ret;
}

MPP_RET hal_h265d_start(void *ctx, HalTaskInfo *task)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->start)
        ret = p->api->start(ctx, task);

    return ret;
}

MPP_RET hal_h265d_wait(void *ctx, HalTaskInfo *task)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->wait)
        ret = p->api->wait(ctx, task);

    return ret;
}

MPP_RET hal_h265d_reset(void *ctx)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->reset)
        ret = p->api->reset(ctx);

    return ret;
}

MPP_RET hal_h265d_flush(void *ctx)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->flush)
        ret = p->api->flush(ctx);

    return ret;
}

MPP_RET hal_h265d_control(void *ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p = (HalH265dCtx *)ctx;

    if (p && p->api && p->api->control)
        ret = p->api->control(ctx, cmd, param);

    return ret;
}

const MppHalApi hal_api_h265d = {
    .name = "h265d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(HalH265dCtx),
    .flag = 0,
    .init = hal_h265d_init,
    .deinit = hal_h265d_deinit,
    .reg_gen = hal_h265d_gen_regs,
    .start = hal_h265d_start,
    .wait = hal_h265d_wait,
    .reset = hal_h265d_reset,
    .flush = hal_h265d_flush,
    .control = hal_h265d_control,
};
