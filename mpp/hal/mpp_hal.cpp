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

typedef struct {
    MppCodingType   mCoding;

    void            *mHalCtx;
    MppHalApi       *api;

    HalTaskGroup    tasks;
    RK_U32          task_count;
} MppHalImpl;

MPP_RET mpp_hal_init(MppHal *ctx, MppHalCfg *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        mpp_err_f("found NULL input ctx %p cfg %p\n", ctx, cfg);
        return MPP_ERR_NULL_PTR;
    }
    *ctx = NULL;

    MppHalImpl *p = mpp_calloc(MppHalImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    cfg->task_count = 2;
    MPP_RET ret = hal_task_group_init(&cfg->tasks, cfg->type, cfg->task_count);
    if (ret) {
        mpp_err_f("hal_task_group_init failed ret %d\n", ret);
        mpp_free(p);
        return MPP_ERR_MALLOC;
    }

    p->tasks        = cfg->tasks;
    p->task_count   = cfg->task_count;
    *ctx = p;
    return MPP_OK;
}

MPP_RET mpp_hal_deinit(MppHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;
    hal_task_group_deinit(p->tasks);
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_hal_reg_gen(MppHal ctx, HalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}

MPP_RET mpp_hal_hw_start(MppHal ctx, HalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}

MPP_RET mpp_hal_hw_wait(MppHal ctx, HalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}


