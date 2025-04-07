/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_META_H__
#define __KMPP_META_H__

#include "mpp_meta.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KmppMetaKey MppMetaKey

#define kmpp_meta_get_f(meta)   kmpp_meta_get(meta, __FUNCTION__)
#define kmpp_meta_put_f(meta)   kmpp_meta_put(meta, __FUNCTION__)
#define kmpp_meta_size_f(meta)  kmpp_meta_size(meta, __FUNCTION__)
#define kmpp_meta_dump_f(meta)  kmpp_meta_dump(meta, __FUNCTION__)

rk_s32 kmpp_meta_get(KmppMeta *meta, const char *caller);
rk_s32 kmpp_meta_put(KmppMeta meta, const char *caller);
rk_s32 kmpp_meta_size(KmppMeta meta, const char *caller);
rk_s32 kmpp_meta_dump(KmppMeta meta, const char *caller);
rk_s32 kmpp_meta_dump_all(const char *caller);

rk_s32 kmpp_meta_set_s32(KmppMeta meta, KmppMetaKey key, rk_s32 val);
rk_s32 kmpp_meta_set_s64(KmppMeta meta, KmppMetaKey key, rk_s64 val);
rk_s32 kmpp_meta_set_ptr(KmppMeta meta, KmppMetaKey key, void  *val);
rk_s32 kmpp_meta_get_s32(KmppMeta meta, KmppMetaKey key, rk_s32 *val);
rk_s32 kmpp_meta_get_s64(KmppMeta meta, KmppMetaKey key, rk_s64 *val);
rk_s32 kmpp_meta_get_ptr(KmppMeta meta, KmppMetaKey key, void  **val);
rk_s32 kmpp_meta_get_s32_d(KmppMeta meta, KmppMetaKey key, rk_s32 *val, rk_s32 def);
rk_s32 kmpp_meta_get_s64_d(KmppMeta meta, KmppMetaKey key, rk_s64 *val, rk_s64 def);
rk_s32 kmpp_meta_get_ptr_d(KmppMeta meta, KmppMetaKey key, void  **val, void *def);

rk_s32 kmpp_meta_set_obj(KmppMeta meta, KmppMetaKey key, KmppObj obj);
rk_s32 kmpp_meta_get_obj(KmppMeta meta, KmppMetaKey key, KmppObj *obj);
rk_s32 kmpp_meta_get_obj_d(KmppMeta meta, KmppMetaKey key, KmppObj *obj, KmppObj def);
rk_s32 kmpp_meta_set_shm(KmppMeta meta, KmppMetaKey key, KmppShmPtr *sptr);
rk_s32 kmpp_meta_get_shm(KmppMeta meta, KmppMetaKey key, KmppShmPtr *sptr);
rk_s32 kmpp_meta_get_shm_d(KmppMeta meta, KmppMetaKey key, KmppShmPtr *sptr, KmppShmPtr *def);

#ifdef __cplusplus
}
#endif

#endif /*__KMPP_META_H__*/
