/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame"

#include "kmpp_frame_impl.h"

rk_s32 kmpp_frame_get_meta(KmppFrame frame, KmppMeta *meta)
{
    KmppFramePriv *priv = NULL;
    KmppShmPtr sptr;
    rk_s32 ret;

    if (!frame || !meta) {
        mpp_loge_f("invalid frame %p meta %p\n", frame, meta);
        return rk_nok;
    }

    priv = (KmppFramePriv *)kmpp_obj_to_priv(frame);
    if (priv->meta) {
        *meta = priv->meta;
        return rk_ok;
    }

    kmpp_obj_get_shm(frame, "meta", &sptr);
    ret = kmpp_obj_get_by_sptr_f(&priv->meta, &sptr);
    if (ret) {
        *meta = NULL;
        mpp_loge_f("self_meta get obj by sptr failed, ret %d\n", ret);
        return ret;
    }

    *meta = priv->meta;

    return rk_ok;
}

static rk_s32 kmpp_frame_impl_init(void *entry, KmppObj obj, const char *caller)
{
    KmppFramePriv *priv = (KmppFramePriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    priv->meta = NULL;

    return rk_ok;
}

static rk_s32 kmpp_frame_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    KmppFramePriv *priv = (KmppFramePriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    if (priv->meta) {
        kmpp_obj_put_impl(priv->meta, caller);
        priv->meta = NULL;
    }

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_frame
#define KMPP_OBJ_INTF_TYPE          KmppFrame
#define KMPP_OBJ_IMPL_TYPE          KmppFrameImpl
#define KMPP_OBJ_FUNC_INIT          kmpp_frame_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_frame_impl_deinit
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_FRAME
#define KMPP_OBJ_ENTRY_TABLE        KMPP_FRAME_ENTRY_TABLE
#define KMPP_OBJ_PRIV_SIZE          sizeof(KmppFramePriv)
#include "kmpp_obj_helper.h"
