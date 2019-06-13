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

#define MODULE_TAG "mpp_allocator"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_allocator.h"
#include "mpp_allocator_impl.h"

#include "os_allocator.h"

#include <linux/drm.h>

#define MPP_ALLOCATOR_LOCK(p)   pthread_mutex_lock(&(p)->lock);
#define MPP_ALLOCATOR_UNLOCK(p) pthread_mutex_unlock(&(p)->lock);

typedef enum OsAllocatorApiId_e {
    ALLOC_API_ALLOC,
    ALLOC_API_FREE,
    ALLOC_API_IMPORT,
    ALLOC_API_RELEASE,
    ALLOC_API_MMAP,
    ALLOC_API_BUTT,
} OsAllocatorApiId;

static MPP_RET mpp_allocator_api_wrapper(MppAllocator allocator,
                                         MppBufferInfo *info,
                                         OsAllocatorApiId id)
{
    if (NULL == allocator || NULL == info || id >= ALLOC_API_BUTT) {
        mpp_err_f("invalid input: allocator %p info %p id %d\n",
                  allocator, info, id);
        return MPP_ERR_UNKNOW;
    }

    MPP_RET ret = MPP_NOK;
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;
    OsAllocatorFunc func;

    MPP_ALLOCATOR_LOCK(p);
    switch (id) {
    case ALLOC_API_ALLOC : {
        func = p->os_api.alloc;
    } break;
    case ALLOC_API_FREE : {
        func = p->os_api.free;
    } break;
    case ALLOC_API_IMPORT : {
        func = p->os_api.import;
    } break;
    case ALLOC_API_RELEASE : {
        func = p->os_api.release;
    } break;
    case ALLOC_API_MMAP : {
        func = p->os_api.mmap;
    } break;
    default : {
        func = NULL;
    } break;
    }

    if (func && p->ctx)
        ret = func(p->ctx, info);

    MPP_ALLOCATOR_UNLOCK(p);

    return ret;
}

static MPP_RET mpp_allocator_alloc(MppAllocator allocator, MppBufferInfo *info)
{
    return mpp_allocator_api_wrapper(allocator, info, ALLOC_API_ALLOC);
}

static MPP_RET mpp_allocator_free(MppAllocator allocator, MppBufferInfo *info)
{
    return mpp_allocator_api_wrapper(allocator, info, ALLOC_API_FREE);
}

static MPP_RET mpp_allocator_import(MppAllocator allocator, MppBufferInfo *info)
{
    return mpp_allocator_api_wrapper(allocator, info, ALLOC_API_IMPORT);
}

static MPP_RET mpp_allocator_release(MppAllocator allocator,
                                     MppBufferInfo *info)
{
    return mpp_allocator_api_wrapper(allocator, info, ALLOC_API_RELEASE);
}

static MPP_RET mpp_allocator_mmap(MppAllocator allocator, MppBufferInfo *info)
{
    return mpp_allocator_api_wrapper(allocator, info, ALLOC_API_MMAP);
}

static MppAllocatorApi mpp_allocator_api = {
    .size = sizeof(mpp_allocator_api),
    .version = 1,
    .alloc = mpp_allocator_alloc,
    .free = mpp_allocator_free,
    .import = mpp_allocator_import,
    .release =  mpp_allocator_release,
    .mmap  = mpp_allocator_mmap,
};

MPP_RET mpp_allocator_get(MppAllocator *allocator,
                          MppAllocatorApi **api, MppBufferType type)
{
    MppBufferType buffer_type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    RK_U32 flags = (type & MPP_BUFFER_FLAGS_MASK) >> 16;

    if (NULL == allocator || NULL == api || buffer_type >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err_f("invalid input: allocator %p api %p type %d\n",
                  allocator, api, buffer_type);
        return MPP_ERR_UNKNOW;
    }

    MppAllocatorImpl *p = mpp_malloc(MppAllocatorImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_allocator_get failed to malloc allocator context\n");
        return MPP_ERR_NULL_PTR;
    } else {
        p->type = buffer_type;
        p->flags = flags;
    }


    MPP_RET ret = os_allocator_get(&p->os_api, buffer_type);

    if (MPP_OK == ret) {
        MppAllocatorCfg cfg = {
            .alignment = SZ_4K,
            .flags = flags,
        };
        ret = p->os_api.open(&p->ctx, &cfg);
    }
    if (MPP_OK == ret) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&p->lock, &attr);
        pthread_mutexattr_destroy(&attr);

        *allocator  = p;
        *api        = &mpp_allocator_api;
    } else {
        mpp_err("mpp_allocator_get type %d failed\n", type);
        mpp_free(p);
        *allocator  = NULL;
        *api        = NULL;
    }

    return ret;
}

MPP_RET mpp_allocator_put(MppAllocator *allocator)
{
    if (NULL == allocator) {
        mpp_err_f("invalid input: allocator %p\n", allocator);
        return MPP_ERR_NULL_PTR;
    }

    MppAllocatorImpl *p = (MppAllocatorImpl *)*allocator;
    *allocator = NULL;
    if (p->os_api.close && p->ctx)
        p->os_api.close(p->ctx);
    pthread_mutex_destroy(&p->lock);
    mpp_free(p);

    return MPP_OK;
}

