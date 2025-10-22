/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_IOC_H__
#define __KMPP_IOC_H__

#include "rk_type.h"

typedef void* KmppIoc;

#define KMPP_IOC_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, u32,  rk_u32,         def,    FLAG_NONE,  def) \
    ENTRY(prefix, u32,  rk_u32,         cmd,    FLAG_NONE,  cmd) \
    ENTRY(prefix, u32,  rk_u32,         flags,  FLAG_NONE,  flags) \
    ENTRY(prefix, u32,  rk_u32,         id,     FLAG_NONE,  id) \
    ENTRY(prefix, s32,  rk_s32,         ret,    FLAG_NONE,  ret) \
    STRCT(prefix, shm,  KmppShmPtr,     ctx,    FLAG_NONE,  ctx) \
    STRCT(prefix, shm,  KmppShmPtr,     in,     FLAG_NONE,  in) \
    STRCT(prefix, shm,  KmppShmPtr,     out,    FLAG_NONE,  out)

#define KMPP_OBJ_NAME                   kmpp_ioc
#define KMPP_OBJ_INTF_TYPE              KmppIoc
#define KMPP_OBJ_ENTRY_TABLE            KMPP_IOC_ENTRY_TABLE
#include "kmpp_obj_func.h"

#endif /*__KMPP_IOC_H__*/
