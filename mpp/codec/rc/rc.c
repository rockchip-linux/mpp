/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2016 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "rc"

#include <math.h>
#include <memory.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_common.h"

#include "rc_debug.h"
#include "rc.h"
#include "rc_api.h"
#include "rc_base.h"

typedef struct MppRcImpl_t {
    void            *ctx;
    const RcImplApi *api;
    RcCfg           cfg;

    RcFpsCfg        fps;
    RK_S32          frm_cnt;

    RK_U32          frm_send;
    RK_U32          frm_done;
} MppRcImpl;

RK_U32 rc_debug = 0;

static const char default_rc_api[] = "default";

MPP_RET rc_init(RcCtx *ctx, MppCodingType type, const char **request_name)
{
    MPP_RET ret = MPP_NOK;
    MppRcImpl *p = NULL;
    RcImplApi *api = NULL;
    const char *name = NULL;

    mpp_env_get_u32("rc_debug", &rc_debug, 0);

    if (NULL == request_name || NULL == *request_name)
        name = default_rc_api;
    else
        name = *request_name;

    rc_dbg_func("enter type %x name %s\n", type, name);

    api = rc_api_get(type, name);

    mpp_assert(api);

    if (api) {
        void *rc_ctx = mpp_calloc_size(void, api->ctx_size);

        p = mpp_calloc(MppRcImpl, 1);
        if (NULL == p || NULL == rc_ctx) {
            mpp_err_f("failed to create context size %d\n", api->ctx_size);
            MPP_FREE(p);
            MPP_FREE(rc_ctx);
            ret = MPP_ERR_MALLOC;
        } else {
            p->ctx = rc_ctx;
            p->api = api;
            p->frm_cnt = -1;
            if (request_name && *request_name)
                mpp_log("using rc impl %s\n", api->name);
            ret = MPP_OK;
        }
    }

    *ctx = p;
    if (request_name)
        *request_name = name;

    rc_dbg_func("leave %p\n", p);

    return ret;
}

MPP_RET rc_deinit(RcCtx ctx)
{
    MppRcImpl *p = (MppRcImpl *)ctx;
    const RcImplApi *api = p->api;
    MPP_RET ret = MPP_OK;

    rc_dbg_func("enter %p\n", ctx);

    if (api && api->deinit && p->ctx) {
        ret = api->deinit(p->ctx);
        MPP_FREE(p->ctx);
    }

    MPP_FREE(p);

    rc_dbg_func("leave %p\n", ctx);

    return ret;
}

MPP_RET rc_update_usr_cfg(RcCtx ctx, RcCfg *cfg)
{
    MppRcImpl *p = (MppRcImpl *)ctx;
    const RcImplApi *api = p->api;
    MPP_RET ret = MPP_OK;

    rc_dbg_func("enter %p\n", ctx);

    p->cfg = *cfg;
    p->fps = cfg->fps;

    if (api && api->init && p->ctx)
        api->init(p->ctx, &p->cfg);

    rc_dbg_func("leave %p\n", ctx);

    return ret;
}

MPP_RET rc_frm_check_drop(RcCtx ctx, EncRcTask *task)
{
    MppRcImpl *p = (MppRcImpl *)ctx;
    const RcImplApi *api = p->api;
    MPP_RET ret = MPP_OK;

    rc_dbg_func("enter %p\n", ctx);

    if (api && api->check_drop && p->ctx && task) {
        ret = api->check_drop(p->ctx, task);
        return ret;
    } else {
        RcFpsCfg *cfg = &p->fps;
        RK_S32 frm_cnt  = p->frm_cnt;
        RK_S32 rate_in  = cfg->fps_in_num * cfg->fps_out_denom;
        RK_S32 rate_out = cfg->fps_out_num * cfg->fps_in_denom;
        RK_S32 drop = 0;

        mpp_assert(cfg->fps_in_denom >= 1);
        mpp_assert(cfg->fps_out_denom >= 1);
        mpp_assert(rate_in >= rate_out);

        // frame counter is inited to (rate_in - rate_out)  to encode first frame
        if (frm_cnt < 0)
            frm_cnt = rate_in - rate_out;

        frm_cnt += rate_out;

        if (frm_cnt < rate_in)
            drop = 1;
        else
            frm_cnt -= rate_in;

        p->frm_cnt = frm_cnt;
        task->frm.drop = drop;
    }

    rc_dbg_func("leave %p drop %d\n", ctx, task->frm.drop);

    return ret;
}

#define MPP_ENC_RC_FUNC(flow, func) \
    MPP_RET rc_##flow##_##func(RcCtx ctx, EncRcTask *task)      \
    {                                                           \
        MppRcImpl *p = (MppRcImpl *)ctx;                        \
        const RcImplApi *api = p->api;                          \
        if (!api || !api->flow##_##func || !p->ctx || !task)    \
            return MPP_OK;                                      \
        return api->flow##_##func(p->ctx, task);                \
    }

MPP_ENC_RC_FUNC(check, reenc)
MPP_ENC_RC_FUNC(frm, start)
MPP_ENC_RC_FUNC(frm, end)
MPP_ENC_RC_FUNC(hal, start)
MPP_ENC_RC_FUNC(hal, end)
