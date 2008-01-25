/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define  MODULE_TAG "mpp_hal"

#include "mpp_mem.h"
#include "mpp_log.h"

#include "mpp.h"
#include "mpp_hal.h"
#include "mpp_frame_impl.h"


MPP_RET mpp_hal_init(MppHal **ctx, MppHalCfg *cfg)
{
    *ctx = mpp_malloc(MppHal, 1);
    (void)cfg;
    return MPP_OK;
}

MPP_RET mpp_hal_deinit(MppHal *ctx)
{
    mpp_free(ctx);
    return MPP_OK;
}

MPP_RET mpp_hal_reg_gen(MppHal *ctx, MppHalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}

MPP_RET mpp_hal_hw_start(MppHal *ctx, MppHalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}

MPP_RET mpp_hal_hw_wait(MppHal *ctx, MppHalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}


