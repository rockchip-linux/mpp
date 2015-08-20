/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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
#include "os_allocator.h"

#include "mpp_mem.h"
#include "mpp_log.h"

/*
 * Linux only support MPP_BUFFER_TYPE_NORMAL so far
 * we can support MPP_BUFFER_TYPE_V4L2 later
 */
typedef struct {
    size_t          alignment;
    MppBufferType   type;
} allocator_impl;

MPP_RET os_allocator_open(void **ctx, size_t alignment, MppBufferType type)
{
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Linux do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL : {
        p = mpp_malloc(allocator_impl, 1);
        if (NULL == p) {
            *ctx = NULL;
            mpp_err("os_allocator_open Linux failed to allocate context\n");
            return MPP_ERR_MALLOC;
        }

        p->alignment = alignment;
    } break;
    default : {
        mpp_err("os_allocator_open Window do not accept type %d\n");
    } break;
    }

    *ctx = p;
    return MPP_OK;
}

MPP_RET os_allocator_alloc(void *ctx, MppBufferData *data, size_t size)
{
    MPP_RET ret = MPP_OK;
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_alloc Linux found NULL context input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_impl *)ctx;
    switch (p->type) {
    case MPP_BUFFER_TYPE_NORMAL : {
        ret = os_malloc(&data->ptr, p->alignment, size);
    } break;
    case MPP_BUFFER_TYPE_V4L2 : {
        // TODO: support v4l2 vb2 buffer queue
        mpp_err("os_allocator_alloc Linux MPP_BUFFER_TYPE_V4L2 will support later\n");
        ret = MPP_ERR_UNKNOW;
    } break;
    default : {
        mpp_err("os_allocator_alloc Linux do not accept type %d\n", p->type);
        ret = MPP_ERR_UNKNOW;
    } break;
    }
    return ret;
}

MPP_RET os_allocator_free(void *ctx, MppBufferData *data)
{
    (void) ctx;
    os_free(data->ptr);
    return MPP_OK;
}

MPP_RET os_allocator_close(void *ctx)
{
    if (ctx) {
        mpp_free(ctx);
        return MPP_OK;
    }
    mpp_err("os_allocator_close Linux found NULL context input\n");
    return MPP_NOK;
}

