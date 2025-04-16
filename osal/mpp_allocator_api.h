/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_ALLOCATOR_API_H__
#define __MPP_ALLOCATOR_API_H__

#include "mpp_allocator.h"

typedef MPP_RET (*OsAllocatorFunc)(void *ctx, MppBufferInfo *info);

typedef struct os_allocator_t {
    MppBufferType type;
    const char *name;

    MPP_RET (*open)(void **ctx, size_t alignment, MppAllocFlagType flags);
    MPP_RET (*close)(void *ctx);

    OsAllocatorFunc alloc;
    OsAllocatorFunc free;
    OsAllocatorFunc import;
    OsAllocatorFunc release;
    OsAllocatorFunc mmap;

    /* allocator real flag update callback */
    MppAllocFlagType (*flags)(void *ctx);
} os_allocator;

#endif /* __MPP_ALLOCATOR_API_H__ */
