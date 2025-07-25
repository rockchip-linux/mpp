/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame"

#include "kmpp_frame_impl.h"
#include "kmpp_obj_impl.h"

static rk_s32 kmpp_frame_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    KmppFrameImpl *impl = (KmppFrameImpl *)entry;

    if (impl->self_meta) {
        kmpp_obj_put_impl(impl->self_meta, caller);
        impl->self_meta = NULL;
    } else {
        mpp_logw_f("self_meta is NULL, obj %p at %s\n", obj, caller);
    }

    return rk_ok;
}

rk_s32 kmpp_frame_get_meta(KmppFrame frame, KmppMeta *meta)
{
    KmppFrameImpl *impl = (KmppFrameImpl *)kmpp_obj_to_entry(frame);
    KmppShmPtr sptr;
    rk_s32 ret;

    if (!impl || !meta) {
        mpp_loge_f("invalid impl %p or meta %p\n", impl, meta);
        return rk_nok;
    }

    if (impl->self_meta) {
        *meta = impl->self_meta;
        return rk_ok;
    }

    kmpp_obj_get_shm(frame, "meta", &sptr);
    ret = kmpp_obj_get_by_sptr_f(&impl->self_meta, &sptr);
    if (ret) {
        *meta = NULL;
        mpp_loge_f("self_meta get obj by sptr failed, ret %d\n", ret);
        return ret;
    }

    *meta = impl->self_meta;
    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_frame
#define KMPP_OBJ_INTF_TYPE          KmppFrame
#define KMPP_OBJ_IMPL_TYPE          KmppFrameImpl
#define KMPP_OBJ_FUNC_DEINIT        kmpp_frame_impl_deinit
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_FRAME
#define KMPP_OBJ_ENTRY_TABLE        KMPP_FRAME_ENTRY_TABLE
#include "kmpp_obj_helper.h"
