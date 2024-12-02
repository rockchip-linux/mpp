/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_IMPL_H__
#define __KMPP_OBJ_IMPL_H__

#include "kmpp_obj.h"

rk_s32 kmpp_obj_impl_set_s32(KmppLocTbl *tbl, void *entry, rk_s32 val);
rk_s32 kmpp_obj_impl_get_s32(KmppLocTbl *tbl, void *entry, rk_s32 *val);
rk_s32 kmpp_obj_impl_set_u32(KmppLocTbl *tbl, void *entry, rk_u32 val);
rk_s32 kmpp_obj_impl_get_u32(KmppLocTbl *tbl, void *entry, rk_u32 *val);
rk_s32 kmpp_obj_impl_set_s64(KmppLocTbl *tbl, void *entry, rk_s64 val);
rk_s32 kmpp_obj_impl_get_s64(KmppLocTbl *tbl, void *entry, rk_s64 *val);
rk_s32 kmpp_obj_impl_set_u64(KmppLocTbl *tbl, void *entry, rk_u64 val);
rk_s32 kmpp_obj_impl_get_u64(KmppLocTbl *tbl, void *entry, rk_u64 *val);
rk_s32 kmpp_obj_impl_set_ptr(KmppLocTbl *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_ptr(KmppLocTbl *tbl, void *entry, void **val);
rk_s32 kmpp_obj_impl_set_fp(KmppLocTbl *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_fp(KmppLocTbl *tbl, void *entry, void **val);
rk_s32 kmpp_obj_impl_set_st(KmppLocTbl *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_st(KmppLocTbl *tbl, void *entry, void *val);

#endif /* __KMPP_OBJ_IMPL_H__ */