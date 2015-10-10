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

#define MODULE_TAG "mpp_allocator"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_allocator.h"
#include "mpp_allocator_impl.h"

#include "os_allocator.h"

#define MPP_ALLOCATOR_LOCK(p)   pthread_mutex_lock(&(p)->lock);
#define MPP_ALLOCATOR_UNLOCK(p) pthread_mutex_unlock(&(p)->lock);

MPP_RET mpp_allocator_alloc(MppAllocator allocator, MppBufferInfo *info)
{
    if (NULL == allocator || NULL == info) {
        mpp_err_f("invalid input: allocator %p info %p\n",
                  allocator, info);
        return MPP_ERR_UNKNOW;
    }

    MPP_RET ret = MPP_OK;
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;
    MPP_ALLOCATOR_LOCK(p);
    if (p->os_api.alloc)
        ret = p->os_api.alloc(p->ctx, info);
    MPP_ALLOCATOR_UNLOCK(p);

    return ret;
}

MPP_RET mpp_allocator_free(MppAllocator allocator, MppBufferInfo *info)
{
    if (NULL == allocator || NULL == info) {
        mpp_err_f("invalid input: allocator %p info %p\n",
                  allocator, info);
        return MPP_ERR_UNKNOW;
    }

    MPP_RET ret = MPP_OK;
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;
    MPP_ALLOCATOR_LOCK(p);
    if (p->os_api.free)
        ret = p->os_api.free(p->ctx, info);
    MPP_ALLOCATOR_UNLOCK(p);

    return ret;
}

MPP_RET mpp_allocator_import(MppAllocator allocator, MppBufferInfo *info)
{
    if (NULL == info) {
        mpp_err_f("invalid input: info %p\n", info);
        return MPP_ERR_UNKNOW;
    }

    MPP_RET ret = MPP_OK;
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;
    if (p->os_api.import) {
        MPP_ALLOCATOR_LOCK(p);
        ret = p->os_api.import(p->ctx, info);
        MPP_ALLOCATOR_UNLOCK(p);
    }

    return ret;
}


MPP_RET mpp_allocator_release(MppAllocator allocator, MppBufferInfo *info)
{
    if (NULL == allocator || NULL == info) {
        mpp_err("mpp_allocator_alloc invalid input: allocator %p info %p\n",
                allocator, info);
        return MPP_ERR_UNKNOW;
    }

    MPP_RET ret = MPP_OK;
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;
    if (p->os_api.release) {
        MPP_ALLOCATOR_LOCK(p);
        ret = p->os_api.release(p->ctx, info);
        MPP_ALLOCATOR_UNLOCK(p);
    }

    return ret;
}

static MppAllocatorApi mpp_allocator_api = {
    sizeof(mpp_allocator_api),
    1,
    mpp_allocator_alloc,
    mpp_allocator_free,
    mpp_allocator_import,
    mpp_allocator_release,
};

MPP_RET mpp_alloctor_get(MppAllocator *allocator, MppAllocatorApi **api, MppBufferType type)
{
    if (NULL == allocator || NULL == api || type >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err("mpp_alloctor_get invalid input: buffer %p api %p type %d\n",
                allocator, api, type);
        return MPP_ERR_UNKNOW;
    }

    MppAllocatorImpl *p = mpp_malloc(MppAllocatorImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_alloctor_get failed to malloc allocator context\n");
        return MPP_ERR_NULL_PTR;
    } else
        p->type = type;


    MPP_RET ret = os_allocator_get(&p->os_api, type);
    if (MPP_OK == ret) {
        p->alignment = SZ_4K;
        ret = p->os_api.open(&p->ctx, p->alignment);
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
        mpp_err("mpp_alloctor_get type %d failed\n", type);
        mpp_free(p);
        *allocator  = NULL;
        *api        = NULL;
    }

    return ret;
}

MPP_RET mpp_alloctor_put(MppAllocator *allocator)
{
    if (NULL == allocator) {
        mpp_err("mpp_alloctor_put invalid input: buffer %p\n", allocator);
        return MPP_ERR_NULL_PTR;
    }

    MppAllocatorImpl *p = (MppAllocatorImpl *)*allocator;
    *allocator = NULL;
    if (p->os_api.close)
        p->os_api.close(p->ctx);
    pthread_mutex_destroy(&p->lock);
    mpp_free(p);

    return MPP_OK;
}

