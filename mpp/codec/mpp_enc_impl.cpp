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

#define  MODULE_TAG "mpp_enc"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "h264e_api.h"
#include "h264e_api_v2.h"
#include "jpege_api.h"
#include "h265e_api.h"
#include "h265e_api_v2.h"
#include "vp8e_api.h"
#include "mpp_enc_impl.h"

/*
 * all encoder controller static register here
 */
static const EncImplApi *controllers[] = {
#if HAVE_H264E
    &api_h264e_controller,
#endif
#if HAVE_JPEGE
    &api_jpege_controller,
#endif
#if HAVE_H265E
    &api_h265e_controller,
#endif
#if HAVE_VP8E
    &api_vp8e_controller,
#endif
};

static const EncImplApi *enc_apis[] = {
#if HAVE_H264E
    &api_h264e,
#endif
#if HAVE_H265E
    &api_h265e,
#endif
};

typedef struct EncImplCtx_t {
    EncImplCfg          cfg;
    const EncImplApi    *api;
    void                *ctx;
} EncImplCtx;

MPP_RET enc_impl_init(EncImpl *impl, EncImplCfg *cfg)
{
    if (NULL == impl || NULL == cfg) {
        mpp_err_f("found NULL input controller %p config %p\n", impl, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *impl = NULL;

    RK_U32 i;
    RK_U32 api_cnt = 0;
    const EncImplApi **apis = NULL;

    if (cfg->task_count < 0) {
        apis = enc_apis;
        api_cnt = MPP_ARRAY_ELEMS(enc_apis);
    } else {
        apis = controllers;
        api_cnt = MPP_ARRAY_ELEMS(controllers);
    }

    for (i = 0; i < api_cnt; i++) {
        const EncImplApi *api = apis[i];

        if (cfg->coding == api->coding) {
            EncImplCtx *p = mpp_calloc(EncImplCtx, 1);
            void *ctx = mpp_calloc_size(void, api->ctx_size);

            if (NULL == ctx || NULL == p) {
                mpp_err_f("failed to alloc encoder context\n");
                mpp_free(p);
                mpp_free(ctx);
                return MPP_ERR_MALLOC;
            }

            MPP_RET ret = api->init(ctx, cfg);
            if (MPP_OK != ret) {
                mpp_err_f("failed to init controller\n");
                mpp_free(p);
                mpp_free(ctx);
                return ret;
            }

            p->api  = api;
            p->ctx  = ctx;
            *impl = p;
            return MPP_OK;
        }
    }

    return MPP_NOK;
}

MPP_RET enc_impl_deinit(EncImpl impl)
{
    if (NULL == impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->deinit)
        p->api->deinit(p->ctx);

    mpp_free(p->ctx);
    mpp_free(p);
    return MPP_OK;
}

MPP_RET enc_impl_proc_cfg(EncImpl impl, MpiCmd cmd, void *para)
{
    if (NULL == impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->proc_cfg)
        ret = p->api->proc_cfg(p->ctx, cmd, para);

    return ret;
}

MPP_RET enc_impl_gen_hdr(EncImpl impl, MppPacket pkt)
{
    if (NULL == impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->gen_hdr)
        ret = p->api->gen_hdr(p->ctx, pkt);

    return ret;
}

MPP_RET enc_impl_start(EncImpl impl, HalEncTask *task)
{
    if (NULL == impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->start)
        ret = p->api->start(p->ctx, task);

    return ret;
}

MPP_RET enc_impl_proc_dpb(EncImpl impl, HalEncTask *task)
{
    if (NULL == impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->proc_dpb)
        ret = p->api->proc_dpb(p->ctx, task);

    return ret;
}

MPP_RET enc_impl_proc_hal(EncImpl impl, HalEncTask *task)
{
    if (NULL == impl || NULL == task) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->proc_hal)
        ret = p->api->proc_hal(p->ctx, task);

    return ret;
}

MPP_RET enc_impl_update_hal(EncImpl impl, HalEncTask *task)
{
    if (NULL == impl || NULL == task) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->update_hal)
        ret = p->api->update_hal(p->ctx, task);

    return ret;
}

MPP_RET hal_enc_callback(void *impl, void *err_info)
{
    if (NULL == impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    EncImplCtx *p = (EncImplCtx *)impl;
    if (p->api->callback)
        ret = p->api->callback(p->ctx, err_info);

    return ret;
}
