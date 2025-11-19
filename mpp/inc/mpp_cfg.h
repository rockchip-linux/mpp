/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_CFG_H
#define MPP_CFG_H

#include "mpp_internal.h"
#include "kmpp_obj_impl.h"

/* header size 128 byte */
typedef struct MppCfgInfoHead_t {
    char            version[116];
    RK_S32          info_size;
    RK_S32          info_count;
    RK_S32          node_count;
} MppCfgInfoHead;

#ifdef  __cplusplus
extern "C" {
#endif

#define MPP_CFG_SET_s32(entry, cfg, val) (kmpp_obj_impl_set_s32)(entry, cfg, val)
#define MPP_CFG_GET_s32(entry, cfg, val) (kmpp_obj_impl_get_s32)(entry, cfg, (RK_S32 *)(val))
#define MPP_CFG_SET_u32(entry, cfg, val) (kmpp_obj_impl_set_u32)(entry, cfg, val)
#define MPP_CFG_GET_u32(entry, cfg, val) (kmpp_obj_impl_get_u32)(entry, cfg, (RK_U32 *)(val))
#define MPP_CFG_SET_s64(entry, cfg, val) (kmpp_obj_impl_set_s64)(entry, cfg, val)
#define MPP_CFG_GET_s64(entry, cfg, val) (kmpp_obj_impl_get_s64)(entry, cfg, (RK_S64 *)(val))
#define MPP_CFG_SET_u64(entry, cfg, val) (kmpp_obj_impl_set_u64)(entry, cfg, val)
#define MPP_CFG_GET_u64(entry, cfg, val) (kmpp_obj_impl_get_u64)(entry, cfg, (RK_U64 *)(val))
#define MPP_CFG_SET_st(entry, cfg, val)  (kmpp_obj_impl_set_st)(entry, cfg, val)
#define MPP_CFG_GET_st(entry, cfg, val)  (kmpp_obj_impl_get_st)(entry, cfg, (void *)(val))
#define MPP_CFG_SET_ptr(entry, cfg, val) (kmpp_obj_impl_set_ptr)(entry, cfg, val)
#define MPP_CFG_GET_ptr(entry, cfg, val) (kmpp_obj_impl_get_ptr)(entry, cfg, (void **)(val))

#define CHECK_CFG_ENTRY(node, name, type) \
    check_cfg_entry(node, name, type, __FUNCTION__)

MPP_RET check_cfg_entry(KmppEntry *node, const char *name, ElemType type,
                        const char *func);

#ifdef  __cplusplus
}
#endif

#endif /* MPP_CFG_H */
