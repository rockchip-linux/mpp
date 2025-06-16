/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_MEM_POOL_H__
#define __MPP_MEM_POOL_H__

#include "mpp_mem.h"

typedef void* MppMemPool;

#ifdef __cplusplus
extern "C" {
#endif

#define mpp_mem_pool_init_f(name, size) mpp_mem_pool_init(name, size, __FUNCTION__)
#define mpp_mem_pool_deinit_f(pool)     mpp_mem_pool_deinit(pool, __FUNCTION__);

#define mpp_mem_pool_get_f(pool)        mpp_mem_pool_get(pool, __FUNCTION__)
#define mpp_mem_pool_put_f(pool, p)     mpp_mem_pool_put(pool, p, __FUNCTION__)

MppMemPool mpp_mem_pool_init(const char *name, size_t size, const char *caller);
void mpp_mem_pool_deinit(MppMemPool pool, const char *caller);

void *mpp_mem_pool_get(MppMemPool pool, const char *caller);
void mpp_mem_pool_put(MppMemPool pool, void *p, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_MEM_POOL_H__*/
