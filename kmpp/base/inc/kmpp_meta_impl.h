/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_META_IMPL_H__
#define __KMPP_META_IMPL_H__

#include "mpp_list.h"
#include "kmpp_meta.h"

#define MPP_TAG_SIZE            32

typedef struct __attribute__((packed)) KmppMetaVal_t {
    rk_u32              state;
    union {
        rk_s32          val_s32;
        rk_s64          val_s64;
        void            *val_ptr;
    };
} KmppMetaVal;

typedef struct __attribute__((packed)) KmppMetaShmVal_t {
    rk_u32              state;
    KmppShmPtr          val_shm;
} KmppMetaObj;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET kmpp_meta_inc_ref(KmppMeta meta);

#ifdef __cplusplus
}
#endif

#endif /*__KMPP_META_IMPL_H__*/
