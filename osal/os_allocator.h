/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef __OS_ALLOCATOR_H__
#define __OS_ALLOCATOR_H__

#include "mpp_allocator.h"

typedef MPP_RET (*OsAllocatorFunc)(void *ctx, MppBufferInfo *info);

typedef struct os_allocator_t {
    MPP_RET (*open)(void **ctx, MppAllocatorCfg *cfg);
    MPP_RET (*close)(void *ctx);

    OsAllocatorFunc alloc;
    OsAllocatorFunc free;
    OsAllocatorFunc import;
    OsAllocatorFunc release;
    OsAllocatorFunc mmap;
} os_allocator;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET os_allocator_get(os_allocator *api, MppBufferType type);

#ifdef __cplusplus
}
#endif

#endif /*__OS_ALLOCATOR_H__*/


