/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define  MODULE_TAG "mpp_enc_hal"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_enc_hal.h"
#include "mpp_frame_impl.h"

#include "hal_h264e_api_v2.h"
#include "hal_h265e_api_v2.h"
#include "hal_jpege_api_v2.h"
#include "hal_vp8e_api_v2.h"

static const MppEncHalApi *hw_enc_apis[] = {
#if HAVE_H264E
    &hal_api_h264e_v2,
#endif
#if HAVE_H265E
    &hal_api_h265e_v2,
#endif
#if HAVE_JPEGE
    &hal_api_jpege_v2,
#endif
#if HAVE_VP8E
    &hal_api_vp8e_v2,
#endif
};

typedef struct MppEncHalImpl_t {
    MppCodingType       coding;

    void                *ctx;
    const MppEncHalApi  *api;

    HalTaskGroup        tasks;
} MppEncHalImpl;

MPP_RET mpp_enc_hal_init(MppEncHal *ctx, MppEncHalCfg *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        mpp_err_f("found NULL input ctx %p cfg %p\n", ctx, cfg);
        return MPP_ERR_NULL_PTR;
    }
    *ctx = NULL;

    MppEncHalImpl *p = mpp_calloc(MppEncHalImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(hw_enc_apis); i++) {
        if (cfg->coding == hw_enc_apis[i]->coding) {
            p->coding       = cfg->coding;
            p->api          = hw_enc_apis[i];
            p->ctx          = mpp_calloc_size(void, p->api->ctx_size);

            MPP_RET ret = p->api->init(p->ctx, cfg);
            if (ret) {
                mpp_err_f("hal %s init failed ret %d\n", hw_enc_apis[i]->name, ret);
                break;
            }

            ret = hal_task_group_init(&p->tasks, TASK_BUTT, cfg->task_cnt,
                                      sizeof(EncAsyncTaskInfo));
            if (ret) {
                mpp_err_f("hal_task_group_init failed ret %d\n", ret);
                break;
            }

            cfg->tasks = p->tasks;
            *ctx = p;
            return MPP_OK;
        }
    }

    mpp_err_f("could not found coding type %d\n", cfg->coding);
    mpp_free(p->ctx);
    mpp_free(p);

    return MPP_NOK;
}

MPP_RET mpp_enc_hal_deinit(MppEncHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppEncHalImpl *p = (MppEncHalImpl*)ctx;
    p->api->deinit(p->ctx);
    mpp_free(p->ctx);
    if (p->tasks)
        hal_task_group_deinit(p->tasks);
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_enc_hal_prepare(void *hal)
{
    if (NULL == hal) {
        mpp_err_f("found NULL input ctx %p\n", hal);
        return MPP_ERR_NULL_PTR;
    }

    MppEncHalImpl *p = (MppEncHalImpl*)hal;
    if (!p->api || !p->api->prepare)
        return MPP_OK;

    return p->api->prepare(p->ctx);
}

MPP_RET mpp_enc_hal_check_part_mode(MppEncHal ctx)
{
    MppEncHalImpl *p = (MppEncHalImpl*)ctx;

    if (p && p->api && p->api->part_start && p->api->part_wait)
        return MPP_OK;

    return MPP_NOK;
}

#define MPP_ENC_HAL_TASK_FUNC(func) \
    MPP_RET mpp_enc_hal_##func(void *hal, HalEncTask *task)             \
    {                                                                   \
        if (NULL == hal || NULL == task) {                              \
            mpp_err_f("found NULL input ctx %p task %p\n", hal, task);  \
            return MPP_ERR_NULL_PTR;                                    \
        }                                                               \
                                                                        \
        MppEncHalImpl *p = (MppEncHalImpl*)hal;                         \
        if (!p->api || !p->api->func)                                   \
            return MPP_OK;                                              \
                                                                        \
        return p->api->func(p->ctx, task);                              \
    }

MPP_ENC_HAL_TASK_FUNC(get_task)
MPP_ENC_HAL_TASK_FUNC(gen_regs)
MPP_ENC_HAL_TASK_FUNC(start)
MPP_ENC_HAL_TASK_FUNC(wait)
MPP_ENC_HAL_TASK_FUNC(part_start)
MPP_ENC_HAL_TASK_FUNC(part_wait)
MPP_ENC_HAL_TASK_FUNC(ret_task)
