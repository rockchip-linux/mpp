/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_CALLBACK_H__
#define __MPP_CALLBACK_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef MPP_RET (*MppCallBack)(const char *caller, void *ctx, RK_S32 cmd, void *param);

typedef struct DecCallBackCtx_t {
    MppCallBack callBack;
    void        *ctx;
    RK_S32      cmd;
} MppCbCtx;

#ifdef __cplusplus
extern "C" {
#endif

#define mpp_callback(ctx, param)  mpp_callback_f(__FUNCTION__, ctx, param)

MPP_RET mpp_callback_f(const char *caller, MppCbCtx *ctx, void *param);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_CALLBACK_H__ */
