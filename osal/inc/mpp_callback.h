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
