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

MPP_RET mpp_allocator_alloc(MppAllocator allocator, MppBufferData **data, size_t size)
{
    if (NULL == allocator || NULL == data) {
        mpp_err("mpp_allocator_alloc invalid input: allocator %p data %p\n",
                allocator, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size) {
        *data = NULL;
        return MPP_NOK;
    }

    MppBufferData *pdata = mpp_malloc(MppBufferData, 1);
    if (NULL == pdata) {
        mpp_err("mpp_allocator_alloc failed to alloc MppBufferData\n");
        *data = NULL;
        return MPP_ERR_MALLOC;
    }

    MppAllocatorImpl *palloc = (MppAllocatorImpl *)allocator;
    MPP_ALLOCATOR_LOCK(palloc);

    int ret = os_allocator_alloc(palloc->allocator, pdata, palloc->alignment, size);
    *data = pdata;

    MPP_ALLOCATOR_UNLOCK(palloc);

    return (0 == ret) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_allocator_free(MppAllocator allocator, MppBufferData **data)
{
    if (NULL == allocator || NULL == data) {
        mpp_err("mpp_allocator_alloc invalid input: allocator %p data %p\n",
                allocator, data);
        return MPP_ERR_UNKNOW;
    }

    MppAllocatorImpl *palloc = (MppAllocatorImpl *)allocator;
    MPP_ALLOCATOR_LOCK(palloc);

    MppBufferData    *pdata  = *data;
    *data = NULL;
    os_allocator_free(palloc->allocator, pdata);

    MPP_ALLOCATOR_UNLOCK(palloc);

    mpp_free(pdata);

    return MPP_OK;
}

MPP_RET mpp_alloctor_get(MppAllocator *allocator, MppAllocatorApi **api)
{
    if (NULL == allocator || NULL == api) {
        mpp_err("mpp_alloctor_get invalid input: buffer %p api %p\n",
                allocator, api);
        return MPP_ERR_UNKNOW;
    }

    MppAllocatorImpl *palloc = mpp_malloc(MppAllocatorImpl, 1);
    if (NULL == palloc) {
        mpp_err("mpp_alloctor_get failed to malloc allocator context\n");
        return MPP_ERR_NULL_PTR;
    }

    MppAllocatorApi *papi = mpp_malloc(MppAllocatorApi, 1);
    if (NULL == papi) {
        mpp_err("mpp_alloctor_get failed to malloc api context\n");
        mpp_free(palloc);
        return MPP_ERR_NULL_PTR;
    }

    os_allocator_open(&palloc->allocator);
    papi->size      = sizeof(papi->size);
    papi->version   = 1;
    papi->alloc     = mpp_allocator_alloc;
    papi->free      = mpp_allocator_free;

    palloc->api         = papi;
    palloc->alignment   = SZ_4K;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&palloc->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    return MPP_OK;
}

MPP_RET mpp_alloctor_put(MppAllocator *allocator)
{
    if (NULL == allocator) {
        mpp_err("mpp_alloctor_put invalid input: buffer %p\n", allocator);
        return MPP_ERR_NULL_PTR;
    }

    MppAllocatorImpl *p = (MppAllocatorImpl *)*allocator;
    *allocator = NULL;
    os_allocator_close(p->allocator);

    mpp_assert(p->api);
    mpp_free(p->api);
    if (p)
        mpp_free(p);

    return MPP_OK;
}

