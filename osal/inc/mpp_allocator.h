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

#ifndef __MPP_ALLOCATOR_H__
#define __MPP_ALLOCATOR_H__

#include "rk_type.h"
#include "mpp_buffer.h"

typedef void *MppAllocator;

typedef struct MppAllocatorCfg_t {
    // input
    size_t          alignment;
    RK_U32          flags;
} MppAllocatorCfg;

typedef struct MppAllocatorApi_t {
    RK_U32  size;
    RK_U32  version;

    MPP_RET (*alloc)(MppAllocator allocator, MppBufferInfo *data);
    MPP_RET (*free)(MppAllocator allocator, MppBufferInfo *data);
    MPP_RET (*import)(MppAllocator allocator, MppBufferInfo *data);
    MPP_RET (*release)(MppAllocator allocator, MppBufferInfo *data);
    MPP_RET (*mmap)(MppAllocator allocator, MppBufferInfo *data);
} MppAllocatorApi;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_allocator_get(MppAllocator *allocator,
                          MppAllocatorApi **api, MppBufferType type);
MPP_RET mpp_allocator_put(MppAllocator *allocator);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ALLOCATOR_H__*/

