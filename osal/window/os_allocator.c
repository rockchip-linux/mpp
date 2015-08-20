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

#include "os_mem.h"
#include "os_allocator.h"

#include "mpp_mem.h"
#include "mpp_log.h"

/*
 * window only support MPP_BUFFER_TYPE_NORMAL
 */

typedef struct {
    size_t alignment;
} allocator_impl;

MPP_RET os_allocator_open(void **ctx, size_t alignment, MppBufferType type)
{
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Window do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL : {
        p = mpp_malloc(allocator_impl, 1);
        if (NULL == p) {
            *ctx = NULL;
            mpp_err("os_allocator_open Window failed to allocate context\n");
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
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_alloc Window found NULL context input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_impl *)ctx;
    return (MPP_RET)os_malloc(&data->ptr, p->alignment, size);
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
    mpp_err("os_allocator_close Window found NULL context input\n");
    return MPP_NOK;
}

