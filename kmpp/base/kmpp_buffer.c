/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_buffer"

#include <asm/ioctl.h>
#include "mpp_mem.h"

#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_lock.h"
#include "mpp_thread.h"
#include "kmpp_obj.h"
#include "kmpp_buffer_impl.h"

#define KMPP_OBJ_NAME               kmpp_buf_grp_cfg
#define KMPP_OBJ_INTF_TYPE          KmppBufGrpCfg
#define KMPP_OBJ_IMPL_TYPE          KmppBufGrpCfgImpl
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_BUF_GRP_CFG
#define KMPP_OBJ_ENTRY_TABLE        KMPP_BUF_GRP_CFG_ENTRY_TABLE
#include "kmpp_obj_helper.h"

KmppBufGrpCfg kmpp_buf_grp_to_cfg(KmppBufGrp grp)
{
    if (grp) {
        KmppBufGrpPriv *priv = (KmppBufGrpPriv *)kmpp_obj_to_priv((KmppObj)grp);

        if (priv)
            return (KmppBufGrpCfg)(priv->obj);
    }

    return NULL;
}

rk_s32 kmpp_buf_grp_setup(KmppBufGrp group)
{
    KmppBufGrpImpl *impl = kmpp_obj_to_entry(group);

    if (!impl)
        mpp_loge_f("invalid NULL group\n");

    return kmpp_obj_ioctl_f(group, 0, group, NULL);
}

rk_s32 kmpp_buf_grp_impl_init(void *entry, KmppObj obj, const char *caller)
{
    KmppBufGrpPriv *priv = (KmppBufGrpPriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid grp %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    {
        KmppBufGrpCfg cfg;
        KmppShmPtr sptr;

        kmpp_obj_get_shm(obj, "cfg", &sptr);
        kmpp_obj_get_by_sptr_f(&cfg, &sptr);

        mpp_assert(cfg);

        priv->obj = cfg;
        priv->impl = kmpp_obj_to_entry(cfg);
    }

    return rk_ok;
}

rk_s32 kmpp_buf_grp_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    KmppBufGrpPriv *priv = (KmppBufGrpPriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid grp %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    if (priv->obj) {
        kmpp_obj_put_impl(priv->obj, caller);
        priv->obj = NULL;
    }

    priv->impl = NULL;

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_buf_grp
#define KMPP_OBJ_INTF_TYPE          KmppBufGrp
#define KMPP_OBJ_IMPL_TYPE          KmppBufGrpImpl
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_BUF_GRP
#define KMPP_OBJ_FUNC_INIT          kmpp_buf_grp_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_buf_grp_impl_deinit
#define KMPP_OBJ_PRIV_SIZE          sizeof(KmppBufGrpPriv)
#include "kmpp_obj_helper.h"

#define KMPP_OBJ_NAME               kmpp_buf_cfg
#define KMPP_OBJ_INTF_TYPE          KmppBufCfg
#define KMPP_OBJ_IMPL_TYPE          KmppBufCfgImpl
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_BUF_CFG
#define KMPP_OBJ_ENTRY_TABLE        KMPP_BUF_CFG_ENTRY_TABLE
#include "kmpp_obj_helper.h"

KmppBufCfg kmpp_buffer_to_cfg(KmppBuffer buf)
{
    if (buf) {
        KmppBufPriv *priv = (KmppBufPriv *)kmpp_obj_to_priv((KmppObj)buf);

        if (priv)
            return (KmppBufCfg)(priv->obj);
    }

    return NULL;
}

rk_s32 kmpp_buffer_setup(KmppBuffer buffer)
{
    KmppBufferImpl *impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);

    if (!impl)
        mpp_loge_f("invalid NULL buffer\n");

    return kmpp_obj_ioctl_f(buffer, 0, buffer, NULL);
}

rk_s32 kmpp_buffer_impl_init(void *entry, KmppObj obj, const char *caller)
{
    KmppBufPriv *priv = (KmppBufPriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid buf %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    {
        KmppBufCfg cfg;
        KmppShmPtr sptr;

        kmpp_obj_get_shm(obj, "cfg", &sptr);
        kmpp_obj_get_by_sptr_f(&cfg, &sptr);

        mpp_assert(cfg);

        priv->obj = cfg;
        priv->impl = kmpp_obj_to_entry(cfg);
    }

    return rk_ok;
}

rk_s32 kmpp_buffer_impl_deinit(void *entry,  KmppObj obj, const char *caller)
{
    KmppBufPriv *priv = (KmppBufPriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid buf %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    if (priv->obj) {
        kmpp_obj_put_impl(priv->obj, caller);
        priv->obj = NULL;
    }

    priv->impl = NULL;

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_buffer
#define KMPP_OBJ_INTF_TYPE          KmppBuffer
#define KMPP_OBJ_IMPL_TYPE          KmppBufferImpl
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_BUFFER
#define KMPP_OBJ_FUNC_INIT          kmpp_buffer_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_buffer_impl_deinit
#define KMPP_OBJ_PRIV_SIZE          sizeof(KmppBufPriv)
#include "kmpp_obj_helper.h"
