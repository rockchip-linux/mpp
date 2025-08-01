/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#include "kmpp_packet_impl.h"

rk_s32 kmpp_packet_get_meta(KmppPacket packet, KmppMeta *meta)
{
    KmppPacketPriv *priv = NULL;
    KmppShmPtr sptr;
    rk_s32 ret;

    if (!packet || !meta) {
        mpp_loge_f("invalid packet %p meta %p\n", packet, meta);
        return rk_nok;
    }

    priv = (KmppPacketPriv *)kmpp_obj_to_priv(packet);
    if (priv->meta) {
        *meta = priv->meta;
        return rk_ok;
    }

    kmpp_obj_get_shm(packet, "meta", &sptr);
    ret = kmpp_obj_get_by_sptr_f(priv->meta, &sptr);
    if (ret) {
        *meta = NULL;
        mpp_loge_f("self_meta get obj by sptr failed, ret %d\n", ret);
        return ret;
    }

    *meta = priv->meta;

    return rk_ok;
}

static rk_s32 kmpp_packet_impl_init(void *entry, KmppObj obj, const char *caller)
{
    KmppPacketPriv *priv = (KmppPacketPriv *)kmpp_obj_to_priv(obj);
    (void)entry;

    if (!priv) {
        mpp_loge_f("invalid %p without priv at %s\n", obj, caller);
        return rk_nok;
    }

    priv->meta = NULL;

    return rk_ok;
}

static rk_s32 kmpp_packet_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    KmppPacketPriv *priv = (KmppPacketPriv *)kmpp_obj_to_priv(obj);
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

#define KMPP_OBJ_NAME               kmpp_packet
#define KMPP_OBJ_INTF_TYPE          KmppPacket
#define KMPP_OBJ_IMPL_TYPE          KmppPacketImpl
#define KMPP_OBJ_FUNC_INIT          kmpp_packet_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_packet_impl_deinit
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_PACKET
#define KMPP_OBJ_ENTRY_TABLE        KMPP_PACKET_ENTRY_TABLE
#define KMPP_OBJ_PRIV_SIZE          sizeof(KmppPacketPriv)
#include "kmpp_obj_helper.h"