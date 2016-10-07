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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "os_mem.h"
#include "os_allocator.h"
#include "allocator_ion.h"
#include "allocator_drm.h"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

typedef struct {
    RK_U32  alignment;
    RK_S32  fd_count;
} allocator_ctx_normal;

MPP_RET os_allocator_normal_open(void **ctx, size_t alignment)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_normal *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(allocator_ctx_normal, 1);
    if (NULL == p) {
        mpp_err("os_allocator_open Android failed to allocate context\n");
        ret = MPP_ERR_MALLOC;
    } else
        p->alignment = alignment;

    p->fd_count = 0;

    *ctx = p;
    return ret;
}

MPP_RET os_allocator_normal_alloc(void *ctx, MppBufferInfo *info)
{
    allocator_ctx_normal *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_normal *)ctx;
    info->fd = p->fd_count++;
    return os_malloc(&info->ptr, p->alignment, info->size);
}

MPP_RET os_allocator_normal_free(void *ctx, MppBufferInfo *info)
{
    (void)ctx;
    if (info->ptr)
        os_free(info->ptr);
    return MPP_OK;
}

MPP_RET os_allocator_normal_import(void *ctx, MppBufferInfo *info)
{
    allocator_ctx_normal *p = (allocator_ctx_normal *)ctx;
    mpp_assert(ctx);
    mpp_assert(info->ptr);
    mpp_assert(info->size);
    info->hnd   = NULL;
    info->fd    = p->fd_count++;
    return MPP_OK;
}

MPP_RET os_allocator_normal_release(void *ctx, MppBufferInfo *info)
{
    (void)ctx;
    mpp_assert(info->ptr);
    mpp_assert(info->size);
    info->ptr   = NULL;
    info->size  = 0;
    info->hnd   = NULL;
    info->fd    = -1;
    return MPP_OK;
}

MPP_RET os_allocator_normal_close(void *ctx)
{
    if (ctx) {
        mpp_free(ctx);
        return MPP_OK;
    }
    mpp_err("os_allocator_close Linux found NULL context input\n");
    return MPP_NOK;
}

static os_allocator allocator_normal = {
    os_allocator_normal_open,
    os_allocator_normal_alloc,
    os_allocator_normal_free,
    os_allocator_normal_import,
    os_allocator_normal_release,
    os_allocator_normal_close,
};

MPP_RET os_allocator_get(os_allocator *api, MppBufferType type)
{
    MPP_RET ret = MPP_OK;

    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL :
    case MPP_BUFFER_TYPE_V4L2 : {
        *api = allocator_normal;
    }
    break;
    case MPP_BUFFER_TYPE_ION : {
        *api = allocator_ion;
    }
    break;
    case MPP_BUFFER_TYPE_DRM : {
#ifdef HAVE_DRM
        *api = allocator_drm;
#else
        *api = allocator_normal;
#endif
    }
    break;
    default : {
        ret = MPP_NOK;
    }
    break;
    }
    return ret;
}

