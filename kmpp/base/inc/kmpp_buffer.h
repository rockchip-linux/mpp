/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_BUFFER_H__
#define __KMPP_BUFFER_H__

#include "mpp_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* KmppBufGrpCfg;
typedef void* KmppBufCfg;
typedef void* KmppDmaBuf;

typedef enum {
    KMPP_BUFFER_INTERNAL,
    KMPP_BUFFER_EXTERNAL,
    KMPP_BUFFER_MODE_BUTT,
} KmppBufferMode;

/*
 * KmppBufGrp is the ioctl entry for KmppBufGrpCfg
 * KmppBufGrpCfg is the buffer group config data entry
 * Usage:
 * 1. kmpp_buf_grp_get() to get a buffer group
 * 2. kmpp_buf_grp_to_cfg() to get the buffer group config data entry
 * 3. kmpp_buf_grp_cfg_set_xxx to write buffer group config
 * 4. kmpp_buf_grp_setup() to activate buffer group config
 * 5. kmpp_buf_grp_put() to release buffer group with group config
 */
#define KMPP_OBJ_NAME                   kmpp_buf_grp
#define KMPP_OBJ_INTF_TYPE              KmppBufGrp
#include "kmpp_obj_func.h"

KmppBufGrpCfg kmpp_buf_grp_to_cfg(KmppBufGrp grp);
rk_s32 kmpp_buf_grp_setup(KmppBufGrp grp);

#define KMPP_BUF_GRP_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, u32,  rk_u32,         flag,       FLAG_NONE,  flag) \
    ENTRY(prefix, u32,  rk_u32,         count,      FLAG_NONE,  count) \
    ENTRY(prefix, u32,  rk_u32,         size,       FLAG_NONE,  size) \
    ENTRY(prefix, u32,  KmppBufferMode, mode,       FLAG_NONE,  mode) \
    ENTRY(prefix, s32,  rk_s32,         fd,         FLAG_NONE,  fd) \
    ENTRY(prefix, s32,  rk_s32,         grp_id,     FLAG_NONE,  grp_id) \
    ENTRY(prefix, s32,  rk_s32,         used,       FLAG_NONE,  used) \
    ENTRY(prefix, s32,  rk_s32,         unused,     FLAG_NONE,  unused) \
    STRCT(prefix, shm,  KmppShmPtr,     name,       FLAG_NONE,  name) \
    STRCT(prefix, shm,  KmppShmPtr,     allocator,  FLAG_NONE,  allocator)

#define KMPP_OBJ_NAME                   kmpp_buf_grp_cfg
#define KMPP_OBJ_INTF_TYPE              KmppBufGrpCfg
#define KMPP_OBJ_ENTRY_TABLE            KMPP_BUF_GRP_CFG_ENTRY_TABLE
#include "kmpp_obj_func.h"

/*
 * KmppBuffer is the ioctl entry for KmppBufCfg
 * KmppBufCfg is the buffer config data entry
 * Usage:
 * 1. kmpp_buffer_get() to get a buffer
 * 2. kmpp_buffer_to_cfg() to get the buffer config data entry
 * 3. kmpp_buffer_cfg_set_xxx to write buffer config
 * 4. kmpp_buffer_setup() to activate buffer config
 * 5. kmpp_buffer_put() to release buffer with config
 */
#define KMPP_OBJ_NAME                   kmpp_buffer
#define KMPP_OBJ_INTF_TYPE              KmppBuffer
#include "kmpp_obj_func.h"

KmppBufCfg kmpp_buffer_to_cfg(KmppBuffer buf);
rk_s32 kmpp_buffer_setup(KmppBuffer buffer);
rk_s32 kmpp_buffer_inc_ref(KmppBuffer buffer);
rk_s32 kmpp_buffer_flush(KmppBuffer buffer);

#define KMPP_BUF_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, u32,  rk_u32,         size,       FLAG_NONE,  size) \
    ENTRY(prefix, u32,  rk_u32,         offset,     FLAG_NONE,  offset) \
    ENTRY(prefix, u32,  rk_u32,         flag,       FLAG_NONE,  flag) \
    ENTRY(prefix, s32,  rk_s32,         fd,         FLAG_NONE,  fd) \
    ENTRY(prefix, s32,  rk_s32,         index,      FLAG_NONE,  index) \
    ENTRY(prefix, s32,  rk_s32,         grp_id,     FLAG_NONE,  grp_id) \
    ENTRY(prefix, s32,  rk_s32,         buf_gid,    FLAG_NONE,  buf_gid) \
    ENTRY(prefix, s32,  rk_s32,         buf_uid,    FLAG_NONE,  buf_uid) \
    STRCT(prefix, shm,  KmppShmPtr,     sptr,       FLAG_NONE,  sptr) \
    STRCT(prefix, shm,  KmppShmPtr,     group,      FLAG_NONE,  group) \
    ALIAS(prefix, uptr, rk_u64,         uptr,       FLAG_NONE,  uptr) \
    ALIAS(prefix, uptr, rk_u64,         upriv,      FLAG_NONE,  upriv) \
    ALIAS(prefix, ufp,  rk_u64,         ufp,        FLAG_NONE,  ufp)

#define KMPP_OBJ_NAME                   kmpp_buf_cfg
#define KMPP_OBJ_INTF_TYPE              KmppBufCfg
#define KMPP_OBJ_ENTRY_TABLE            KMPP_BUF_CFG_ENTRY_TABLE
#include "kmpp_obj_func.h"

#ifdef __cplusplus
}
#endif

#endif /*__KMPP_BUFFER_H__*/
