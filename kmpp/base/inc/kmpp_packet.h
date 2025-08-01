/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_PACKET_H__
#define __KMPP_PACKET_H__

#include "rk_type.h"

#define KMPP_PACKET_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, s32, rk_s32,      size,           FLAG_NONE, size) \
    ENTRY(prefix, s32, rk_s32,      length,         FLAG_NONE, length) \
    ENTRY(prefix, s64, rk_s64,      pts,            FLAG_NONE, pts) \
    ENTRY(prefix, s64, rk_s64,      dts,            FLAG_NONE, dts) \
    ENTRY(prefix, u32, rk_u32,      status,         FLAG_NONE, status) \
    ENTRY(prefix, u32, rk_u32,      temporal_id,    FLAG_NONE, temporal_id) \
    STRCT(prefix, shm, KmppShmPtr,  data,           FLAG_NONE, data) \
    STRCT(prefix, shm, KmppShmPtr,  buffer,         FLAG_NONE, buffer) \
    STRCT(prefix, shm, KmppShmPtr,  pos,            FLAG_NONE, pos) \
    ENTRY(prefix, u32, rk_u32,      flag,           FLAG_NONE, flag)

#ifdef __cplusplus
extern "C" {
#endif

#define KMPP_OBJ_NAME           kmpp_packet
#define KMPP_OBJ_INTF_TYPE      KmppPacket
#define KMPP_OBJ_ENTRY_TABLE    KMPP_PACKET_ENTRY_TABLE
#include "kmpp_obj_func.h"

rk_s32 kmpp_packet_get_meta(KmppPacket packet, KmppMeta *meta);

#ifdef __cplusplus
}
#endif

#endif /*__KMPP_PACKET_H__*/