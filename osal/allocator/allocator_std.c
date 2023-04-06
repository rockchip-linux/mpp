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

#include <stdio.h>

#include "os_mem.h"
#include "mpp_mem.h"
#include "mpp_debug.h"

#include "allocator_std.h"

typedef struct {
    size_t alignment;
    RK_S32 fd_count;
} allocator_ctx;

static MPP_RET allocator_std_open(void **ctx, MppAllocatorCfg *cfg)
{
    if (NULL == ctx) {
        mpp_err_f("do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_err_f("Warning: std allocator should be used on simulation mode only\n");
    (void)cfg;

    *ctx = NULL;
    return MPP_NOK;
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
    if (ctx) {
        mpp_free(ctx);
        return MPP_OK;
    }
    mpp_err_f("found NULL context input\n");
    return MPP_NOK;
}

os_allocator allocator_std = {
    .type = MPP_BUFFER_TYPE_NORMAL,
    .open = allocator_std_open,
    .close = allocator_std_close,
    .alloc = allocator_std_alloc,
    .free = allocator_std_free,
    .import = allocator_std_import,
    .release = allocator_std_release,
    .mmap = allocator_std_mmap,
};

