/*
 * Copyright 2021 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_MEM_POOL_H__
#define __MPP_MEM_POOL_H__

#include <stdlib.h>
#include "mpp_mem.h"
#include "mpp_common.h"

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
