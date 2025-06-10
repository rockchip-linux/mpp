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

#define mpp_mem_pool_init(size)     mpp_mem_pool_init_f(__FUNCTION__, size)
#define mpp_mem_pool_deinit(pool)   mpp_mem_pool_deinit_f(__FUNCTION__, pool);

#define mpp_mem_pool_get(pool)      mpp_mem_pool_get_f(__FUNCTION__, pool)
#define mpp_mem_pool_put(pool, p)   mpp_mem_pool_put_f(__FUNCTION__, pool, p)

MppMemPool mpp_mem_pool_init_f(const char *caller, size_t size);
void mpp_mem_pool_deinit_f(const char *caller, MppMemPool pool);

void *mpp_mem_pool_get_f(const char *caller, MppMemPool pool);
void mpp_mem_pool_put_f(const char *caller, MppMemPool pool, void *p);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_MEM_POOL_H__*/
