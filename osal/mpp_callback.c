/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_callback"

#include "mpp_callback.h"

MPP_RET mpp_callback_f(const char *caller, MppCbCtx *ctx, void *param)
{
    if (ctx && ctx->ctx && ctx->callBack)
        return ctx->callBack(caller, ctx->ctx, ctx->cmd, param);

    return MPP_OK;
}
