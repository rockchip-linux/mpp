/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_REF_POOL_H
#define MPP_REF_POOL_H

#include "rk_type.h"

/* header of reference pool */
typedef struct MppRefPool_t {
    RK_S32 capacity;
    RK_S32 count;
    void *slots;
} MppRefPool;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpp_ref_pool_init(MppRefPool *pool, RK_U32 capacity);
void mpp_ref_pool_deinit(MppRefPool *pool);
void mpp_ref_pool_cleanup(MppRefPool *pool);

RK_S32 mpp_ref_pool_get(MppRefPool *pool, RK_U32 size);
void mpp_ref_pool_ref(MppRefPool *pool, RK_S32 idx);
void mpp_ref_pool_put(MppRefPool *pool, RK_S32 idx);

void *mpp_ref_pool_ptr(MppRefPool *pool, RK_S32 idx);
RK_U32 mpp_ref_pool_size(MppRefPool *pool, RK_S32 idx);

#ifdef __cplusplus
}
#endif

#endif /* MPP_REF_POOL_H */
