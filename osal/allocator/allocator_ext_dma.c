/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2018 Rockchip Electronics Co., Ltd.
 */

#include <stdio.h>
#include <sys/mman.h>

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "allocator_ext_dma.h"

typedef struct {
    size_t              alignment;
    MppAllocFlagType    flags;
} allocator_ctx;

static MPP_RET allocator_ext_dma_open(void **ctx, size_t alignment, MppAllocFlagType flags)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx *p = NULL;

    if (NULL == ctx) {
        mpp_err_f("do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(allocator_ctx, 1);
    if (NULL == p) {
        mpp_err_f("failed to allocate context\n");
        ret = MPP_ERR_MALLOC;
    } else {
        p->alignment = alignment;
        p->flags = flags;
    }

    *ctx = p;
    return ret;
}

static MPP_RET allocator_ext_dma_alloc(void *ctx, MppBufferInfo *info)
{
    if (!ctx || !info) {
        mpp_err_f("found NULL context input\n");
        return MPP_ERR_VALUE;
    }

    return MPP_ERR_PERM;
}

static MPP_RET allocator_ext_dma_free(void *ctx, MppBufferInfo *info)
{
    if (!ctx || !info) {
        mpp_err_f("found NULL context input\n");
        return MPP_ERR_VALUE;
    }

    return MPP_ERR_PERM;
}

static MPP_RET allocator_ext_dma_import(void *ctx, MppBufferInfo *info)
{
    allocator_ctx *p = (allocator_ctx *)ctx;
    mpp_assert(p);
    mpp_assert(info->size);

    if (info->ptr) {
        mpp_err_f("The ext_dma is not used for userptr\n");
        return MPP_ERR_VALUE;
    }

    return ((info->fd < 0) ? MPP_ERR_VALUE : MPP_OK);
}

static MPP_RET allocator_ext_dma_mmap(void *ctx, MppBufferInfo *info)
{
    void *ptr = NULL;
    int flags = 0;
    unsigned long offset = 0L;
    mpp_assert(ctx);
    mpp_assert(info->size);
    mpp_assert(info->fd >= 0);

    if (info->ptr)
        return MPP_OK;

    /*
     * It is insecure to access the first memory page,
     * usually system doesn't allow this behavior.
     */
    flags = PROT_READ;
    if (fcntl(info->fd, F_GETFL) & O_RDWR)
        flags |= PROT_WRITE;

    ptr = mmap(NULL, info->size, flags, MAP_SHARED, info->fd, offset);
    if (ptr == MAP_FAILED)
        return MPP_ERR_NULL_PTR;

    info->ptr = ptr;

    return MPP_OK;
}

static MPP_RET allocator_ext_dma_release(void *ctx, MppBufferInfo *info)
{
    mpp_assert(ctx);
    mpp_assert(info->size);

    if (info->ptr)
        munmap(info->ptr, info->size);

    info->ptr   = NULL;
    info->hnd   = NULL;
    info->fd    = -1;
    info->size  = 0;

    return MPP_OK;
}

static MPP_RET allocator_ext_dma_close(void *ctx)
{
    if (ctx) {
        mpp_free(ctx);
        return MPP_OK;
    }

    mpp_err_f("found NULL context input\n");
    return MPP_ERR_VALUE;
}

static MppAllocFlagType os_allocator_ext_dma_flags(void *ctx)
{
    allocator_ctx *p = (allocator_ctx *)ctx;

    return p ? (MppAllocFlagType)p->flags : MPP_ALLOC_FLAG_NONE;
}

os_allocator allocator_ext_dma = {
    .type = MPP_BUFFER_TYPE_EXT_DMA,
    .name = "ext_dma",
    .open = allocator_ext_dma_open,
    .close = allocator_ext_dma_close,
    .alloc = allocator_ext_dma_alloc,
    .free = allocator_ext_dma_free,
    .import = allocator_ext_dma_import,
    .release = allocator_ext_dma_release,
    .mmap = allocator_ext_dma_mmap,
    .flags = os_allocator_ext_dma_flags,
};
