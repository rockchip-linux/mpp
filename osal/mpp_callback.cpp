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

#define MODULE_TAG "mpp_callback"

#include "mpp_callback.h"

MPP_RET mpp_callback_f(const char *caller, MppCbCtx *ctx, void *param)
{
    if (ctx && ctx->ctx && ctx->callBack)
        return ctx->callBack(caller, ctx->ctx, ctx->cmd, param);

    return MPP_OK;
}
