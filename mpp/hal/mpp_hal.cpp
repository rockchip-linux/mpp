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

#define  MODULE_TAG "mpp_hal"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_hal.h"
#include "mpp_frame_impl.h"

#include "hal_h263d_api.h"
#include "hal_h264d_api.h"
#include "hal_h265d_api.h"
#include "hal_vp8d_api.h"
#include "hal_vp9d_api.h"
#include "hal_avsd_api.h"
#include "hal_avs2d_api.h"
#include "hal_m2vd_api.h"
#include "hal_mpg4d_api.h"
#include "hal_jpegd_api.h"
#include "hal_av1d_api.h"

// for test and demo
#include "hal_dummy_dec_api.h"
#include "hal_dummy_enc_api.h"

/*
 * all hardware api static register here
 */
static const MppHalApi *hw_apis[] = {
#if HAVE_AVSD
    &hal_api_avsd,
    &hal_api_avsd_plus,
#endif
#if HAVE_AVS2D
    &hal_api_avs2d,
#endif
#if HAVE_H263D
    &hal_api_h263d,
#endif
#if HAVE_H264D
    &hal_api_h264d,
#endif
#if HAVE_H265D
    &hal_api_h265d,
#endif
#if HAVE_MPEG2D
    &hal_api_m2vd,
#endif
#if HAVE_MPEG4D
    &hal_api_mpg4d,
#endif
#if HAVE_VP8D
    &hal_api_vp8d,
#endif
#if HAVE_VP9D
    &hal_api_vp9d,
#endif
#if HAVE_JPEGD
    &hal_api_jpegd,
#endif
#if HAVE_AV1D
    &hal_api_av1d,
#endif
    &hal_api_dummy_dec,
    &hal_api_dummy_enc,
};

typedef struct MppHalImpl_t {
    void            *ctx;
    const MppHalApi *api;

    HalTaskGroup    tasks;
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

    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(hw_apis); i++) {
        if (cfg->type   == hw_apis[i]->type &&
            cfg->coding == hw_apis[i]->coding) {
            p->api  = hw_apis[i];
            p->ctx  = mpp_calloc_size(void, p->api->ctx_size);

            MPP_RET ret = p->api->init(p->ctx, cfg);
            if (ret) {
                mpp_err_f("hal %s init failed ret %d\n", hw_apis[i]->name, ret);
                break;
            }

            *ctx = p;
            return MPP_OK;
        }
    }

    mpp_err_f("could not found coding type %d\n", cfg->coding);
    mpp_free(p->ctx);
    mpp_free(p);

    return MPP_NOK;
}

MPP_RET mpp_hal_deinit(MppHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;
    p->api->deinit(p->ctx);
    mpp_free(p->ctx);
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_hal_reg_gen(MppHal ctx, HalTaskInfo *task)
{
    if (NULL == ctx || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", ctx, task);
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;
    return p->api->reg_gen(p->ctx, task);
}

MPP_RET mpp_hal_hw_start(MppHal ctx, HalTaskInfo *task)
{
    if (NULL == ctx || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", ctx, task);
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;
    return p->api->start(p->ctx, task);
}

MPP_RET mpp_hal_hw_wait(MppHal ctx, HalTaskInfo *task)
{
    if (NULL == ctx || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", ctx, task);
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;
    return p->api->wait(p->ctx, task);
}

MPP_RET mpp_hal_reset(MppHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    if (NULL == p->api || NULL == p->api->reset)
        return MPP_OK;

    return p->api->reset(p->ctx);
}

MPP_RET mpp_hal_flush(MppHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    if (NULL == p->api || NULL == p->api->flush)
        return MPP_OK;

    return p->api->flush(p->ctx);
}

MPP_RET mpp_hal_control(MppHal ctx, MpiCmd cmd, void *param)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    if (NULL == p->api || NULL == p->api->control)
        return MPP_OK;

    return p->api->control(p->ctx, cmd, param);
}
