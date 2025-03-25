/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_IMPL_H__
#define __KMPP_OBJ_IMPL_H__

#include "kmpp_obj.h"

rk_s32 kmpp_obj_impl_set_s32(KmppEntry *tbl, void *entry, rk_s32 val);
rk_s32 kmpp_obj_impl_get_s32(KmppEntry *tbl, void *entry, rk_s32 *val);
rk_s32 kmpp_obj_impl_set_u32(KmppEntry *tbl, void *entry, rk_u32 val);
rk_s32 kmpp_obj_impl_get_u32(KmppEntry *tbl, void *entry, rk_u32 *val);
rk_s32 kmpp_obj_impl_set_s64(KmppEntry *tbl, void *entry, rk_s64 val);
rk_s32 kmpp_obj_impl_get_s64(KmppEntry *tbl, void *entry, rk_s64 *val);
rk_s32 kmpp_obj_impl_set_u64(KmppEntry *tbl, void *entry, rk_u64 val);
rk_s32 kmpp_obj_impl_get_u64(KmppEntry *tbl, void *entry, rk_u64 *val);
rk_s32 kmpp_obj_impl_set_st(KmppEntry *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_st(KmppEntry *tbl, void *entry, void *val);

rk_s32 kmpp_obj_impl_set_shm(KmppEntry *tbl, void *entry, KmppShmPtr *val);
rk_s32 kmpp_obj_impl_get_shm(KmppEntry *tbl, void *entry, KmppShmPtr *val);

rk_s32 kmpp_obj_impl_set_obj(KmppEntry *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_obj(KmppEntry *tbl, void *entry, void **val);
rk_s32 kmpp_obj_impl_set_ptr(KmppEntry *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_ptr(KmppEntry *tbl, void *entry, void **val);
rk_s32 kmpp_obj_impl_set_fp(KmppEntry *tbl, void *entry, void *val);
rk_s32 kmpp_obj_impl_get_fp(KmppEntry *tbl, void *entry, void **val);

#endif /* __KMPP_OBJ_IMPL_H__ */