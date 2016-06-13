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

#include "os_mem.h"
#include "os_allocator.h"

#include "mpp_mem.h"
#include "mpp_log.h"

/*
 * window only support MPP_BUFFER_TYPE_NORMAL
 */

typedef struct {
    size_t alignment;
} allocator_ctx;

MPP_RET os_allocator_open(void **ctx, size_t alignment)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Window do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(allocator_ctx, 1);
    if (NULL == p) {
        mpp_err("os_allocator_open Window failed to allocate context\n");
        ret = MPP_ERR_MALLOC;
    } else
        p->alignment = alignment;

    *ctx = p;
    return ret;
}

MPP_RET os_allocator_alloc(void *ctx, MppBufferInfo *info)
{
    allocator_ctx *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_alloc Window found NULL context input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx *)ctx;
    return (MPP_RET)os_malloc(&info->ptr, p->alignment, info->size);
}

MPP_RET os_allocator_free(void *ctx, MppBufferInfo *info)
{
    (void) ctx;
    if (info->ptr)
        os_free(info->ptr);
    return MPP_OK;
}

MPP_RET os_allocator_import(void *ctx, MppBufferInfo *info)
{
    (void) ctx;
    mpp_assert(info->ptr);
    mpp_assert(info->size);
    info->hnd   = NULL;
    info->fd    = -1;
    return MPP_OK;
}

MPP_RET os_allocator_release(void *ctx, MppBufferInfo *info)
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

MPP_RET os_allocator_close(void *ctx)
{
    if (ctx) {
        mpp_free(ctx);
        return MPP_OK;
    }
    mpp_err("os_allocator_close Window found NULL context input\n");
    return MPP_NOK;
}

static os_allocator allocator_window = {
    os_allocator_open,
    os_allocator_alloc,
    os_allocator_free,
    os_allocator_import,
    os_allocator_release,
    os_allocator_close,
};

MPP_RET os_allocator_get(os_allocator *api, MppBufferType type)
{
    MPP_RET ret = MPP_OK;
    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL :
    case MPP_BUFFER_TYPE_ION :
    case MPP_BUFFER_TYPE_V4L2 : {
        *api = allocator_window;
    } break;
    default : {
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

