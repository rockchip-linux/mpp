/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#include <stdio.h>

#include "os_mem.h"
#include "mpp_mem.h"
#include "mpp_debug.h"

#include "allocator_std.h"

typedef struct {
    size_t              alignment;
    MppAllocFlagType    flags;
    RK_S32              fd_count;
} allocator_ctx;

static MPP_RET allocator_std_open(void **ctx, size_t alignment, MppAllocFlagType flags)
{
    allocator_ctx *p = NULL;

    if (NULL == ctx) {
        mpp_err_f("do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_err_f("Warning: std allocator should be used on simulation mode only\n");

    p = mpp_malloc(allocator_ctx, 1);
    if (p) {
        p->alignment = alignment;
        p->flags = flags;
        p->fd_count = 0;
    }

    *ctx = p;
    return p ? MPP_OK : MPP_NOK;
}

static MPP_RET allocator_std_alloc(void *ctx, MppBufferInfo *info)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL context input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_err_f("Warning: std allocator should be used on simulation mode only\n");
    (void)info;

    return MPP_NOK;
}

static MPP_RET allocator_std_free(void *ctx, MppBufferInfo *info)
{
    (void) ctx;
    if (info->ptr)
        os_free(info->ptr);
    return MPP_OK;
}

static MPP_RET allocator_std_import(void *ctx, MppBufferInfo *info)
{
    allocator_ctx *p = (allocator_ctx *)ctx;
    mpp_assert(ctx);
    mpp_assert(info->ptr);
    mpp_assert(info->size);
    info->hnd   = NULL;
    info->fd    = p->fd_count++;
    return MPP_OK;
}

static MPP_RET allocator_std_release(void *ctx, MppBufferInfo *info)
{
    (void) ctx;
    mpp_assert(info->ptr);
    mpp_assert(info->size);
    info->ptr   = NULL;
    info->size  = 0;
    info->hnd   = NULL;
    info->fd    = -1;
    return MPP_OK;
}

static MPP_RET allocator_std_mmap(void *ctx, MppBufferInfo *info)
{
    mpp_assert(ctx);
    mpp_assert(info->ptr);
    mpp_assert(info->size);
    return MPP_OK;
}

static MPP_RET allocator_std_close(void *ctx)
{
    MPP_FREE(ctx);
    return MPP_OK;
}

static MppAllocFlagType os_allocator_std_flags(void *ctx)
{
    (void) ctx;
    return MPP_ALLOC_FLAG_NONE;
}

os_allocator allocator_std = {
    .type = MPP_BUFFER_TYPE_NORMAL,
    .name = "std",
    .open = allocator_std_open,
    .close = allocator_std_close,
    .alloc = allocator_std_alloc,
    .free = allocator_std_free,
    .import = allocator_std_import,
    .release = allocator_std_release,
    .mmap = allocator_std_mmap,
    .flags = os_allocator_std_flags,
};
