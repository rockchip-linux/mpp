/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_ALLOCATOR_H
#define MPP_ALLOCATOR_H

#include "mpp_bit.h"
#include "mpp_buffer.h"

typedef enum MppAllocFlagType_e {
    MPP_ALLOC_FLAG_NONE         = 0U,
    MPP_ALLOC_FLAG_DMA32        = MPP_BIT(0),
    MPP_ALLOC_FLAG_CACHABLE     = MPP_BIT(1),
    MPP_ALLOC_FLAG_CMA          = MPP_BIT(2),
    MPP_ALLOC_FLAG_TYPE_MASK    = MPP_FLAG_OR(MPP_ALLOC_FLAG_CMA, MPP_ALLOC_FLAG_CACHABLE, MPP_ALLOC_FLAG_DMA32),
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

#endif /* MPP_ALLOCATOR_H */
