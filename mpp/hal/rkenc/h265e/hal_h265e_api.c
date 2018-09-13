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

#define MODULE_TAG "hal_h265e_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_platform.h"

#include "hal_h265e_api.h"
#include "hal_h265e_base.h"
#include "hal_h265e_vepu22.h"


RK_U32 hal_h265e_debug = 0;

MPP_RET hal_h265e_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_NOK;
    MppHalApi *p_api = NULL;

    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    if (ctx == NULL) {
        mpp_err_f("error: ctx == NULL");
        return MPP_NOK;
    }

    mpp_env_get_u32("hal_h265e_debug", &hal_h265e_debug, 0);
    hal_h265e_dbg_func("enter hal\n", hal);

    memset(ctx, 0, sizeof(HalH265eCtx));
    p_api = &ctx->hal_api;

    // NOTE: rk3036 and rk3228 do NOT have jpeg encoder
    if (NULL == mpp_get_vcodec_dev_name(MPP_CTX_ENC, MPP_VIDEO_CodingHEVC)) {
        mpp_err("SOC %s do NOT support h265 encoding\n", mpp_get_soc_name());
        ret = MPP_ERR_INIT;
        goto FAIL;
    }

    if (!(mpp_get_vcodec_type() & HAVE_H265ENC)) {
        mpp_err("cannot find hardware.\n");
        ret = MPP_ERR_INIT;
        goto FAIL;
    }
    p_api->init    = hal_h265e_vepu22_init;
    p_api->deinit  = hal_h265e_vepu22_deinit;
    p_api->reg_gen = hal_h265e_vepu22_gen_regs;
    p_api->start   = hal_h265e_vepu22_start;
    p_api->wait    = hal_h265e_vepu22_wait;
    p_api->reset   = hal_h265e_vepu22_reset;
    p_api->flush   = hal_h265e_vepu22_flush;
    p_api->control = hal_h265e_vepu22_control;

    p_api->init(ctx, cfg);

    hal_h265e_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
FAIL:
    return ret;
}

MPP_RET hal_h265e_deinit(void *hal)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.deinit(hal);
}

MPP_RET hal_h265e_gen_regs(void *hal, HalTaskInfo *task)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.reg_gen(hal, task);
}

MPP_RET hal_h265e_start(void *hal, HalTaskInfo *task)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.start(hal, task);
}

MPP_RET hal_h265e_wait(void *hal, HalTaskInfo *task)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.wait(hal, task);
}

MPP_RET hal_h265e_reset(void *hal)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.reset(hal);
}

MPP_RET hal_h265e_flush(void *hal)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.flush(hal);
}

MPP_RET hal_h265e_control(void *hal, RK_S32 cmd_type, void *param)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    return ctx->hal_api.control(hal, cmd_type, param);
}

const MppHalApi hal_api_h265e = {
    "h265e_rkv",
    MPP_CTX_ENC,
    MPP_VIDEO_CodingHEVC,
    sizeof(HalH265eCtx),
    0,
    hal_h265e_init,
    hal_h265e_deinit,
    hal_h265e_gen_regs,
    hal_h265e_start,
    hal_h265e_wait,
    hal_h265e_reset,
    hal_h265e_flush,
    hal_h265e_control,
};

