/*
 * Copyright 2018 Rockchip Electronics Co. LTD
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

#include <stdio.h>
#include <sys/mman.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "os_mem.h"

#include "allocator_ext_dma.h"

typedef struct {
    size_t alignment;
} allocator_ctx;

static MPP_RET allocator_ext_dma_open(void **ctx, MppAllocatorCfg *cfg)
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
        p->alignment = cfg->alignment;
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
    ptr = mmap(NULL, info->size, PROT_READ | PROT_WRITE,
               MAP_SHARED, info->fd, offset);
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

os_allocator allocator_ext_dma = {
    .open = allocator_ext_dma_open,
    .close = allocator_ext_dma_close,
    .alloc = allocator_ext_dma_alloc,
    .free = allocator_ext_dma_free,
    .import = allocator_ext_dma_import,
    .release = allocator_ext_dma_release,
    .mmap = allocator_ext_dma_mmap,
};
