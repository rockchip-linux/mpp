/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_ALLOCATOR_H__
#define __MPP_ALLOCATOR_H__

#include "mpp_buffer.h"

typedef enum MppAllocFlagType_e {
    MPP_ALLOC_FLAG_NONE         = 0,
    MPP_ALLOC_FLAG_DMA32        = (1 << 0),
    MPP_ALLOC_FLAG_CACHABLE     = (1 << 1),
    MPP_ALLOC_FLAG_CMA          = (1 << 2),
    MPP_ALLOC_FLAG_TYPE_MASK    = MPP_ALLOC_FLAG_CMA | MPP_ALLOC_FLAG_CACHABLE | MPP_ALLOC_FLAG_DMA32,
    MPP_ALLOC_FLAG_TYPE_NB,
} MppAllocFlagType;

typedef void *MppAllocator;

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

/* NOTE: flag may be updated by allocator */
MPP_RET mpp_allocator_get(MppAllocator *allocator, MppAllocatorApi **api,
                          MppBufferType type, MppAllocFlagType flag);
MPP_RET mpp_allocator_put(MppAllocator *allocator);
MppAllocFlagType mpp_allocator_get_flags(const MppAllocator allocator);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ALLOCATOR_H__*/
