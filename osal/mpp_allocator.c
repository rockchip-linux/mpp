/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_allocator"

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_runtime.h"
#include "mpp_thread.h"
#include "mpp_allocator.h"

#include "allocator_std.h"
#include "allocator_ion.h"
#include "allocator_drm.h"
#include "allocator_ext_dma.h"
#include "allocator_dma_heap.h"

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

typedef struct MppAllocatorImpl_t {
    pthread_mutex_t     lock;
    MppBufferType       type;
    MppAllocFlagType    flags;
    size_t              alignment;
    os_allocator        os_api;
    void                *ctx;
} MppAllocatorImpl;

static MPP_RET mpp_allocator_api_wrapper(MppAllocator allocator,
                                         MppBufferInfo *info,
                                         OsAllocatorApiId id)
{
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;
    OsAllocatorFunc func;
    MPP_RET ret = MPP_NOK;

    if (!p || !info || id >= ALLOC_API_BUTT) {
        mpp_err_f("invalid input: allocator %p info %p id %d\n",
                  allocator, info, id);
        return MPP_ERR_UNKNOW;
    }

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

MPP_RET mpp_allocator_get(MppAllocator *allocator, MppAllocatorApi **api,
                          MppBufferType type, MppAllocFlagType flags)
{
    MppBufferType buffer_type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    MppAllocatorImpl *p = NULL;

    do {
        if (!allocator || !api || buffer_type >= MPP_BUFFER_TYPE_BUTT) {
            mpp_err_f("invalid input: allocator %p api %p type %d\n",
                      allocator, api, buffer_type);
            break;
        }

        p = mpp_malloc(MppAllocatorImpl, 1);
        if (!p) {
            mpp_err_f("failed to malloc allocator context\n");
            break;
        }

        p->type = buffer_type;
        p->flags = flags;

        switch (buffer_type) {
        case MPP_BUFFER_TYPE_NORMAL : {
            p->os_api = allocator_std;
        } break;
        case MPP_BUFFER_TYPE_ION : {
            p->os_api = (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DMA_HEAP)) ? allocator_dma_heap :
                        (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_ION)) ? allocator_ion :
                        (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DRM)) ? allocator_drm :
                        allocator_std;
        } break;
        case MPP_BUFFER_TYPE_EXT_DMA: {
            p->os_api = allocator_ext_dma;
        } break;
        case MPP_BUFFER_TYPE_DRM : {
            p->os_api = (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DMA_HEAP)) ? allocator_dma_heap :
                        (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DRM)) ? allocator_drm :
                        (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_ION)) ? allocator_ion :
                        allocator_std;
        } break;
        case MPP_BUFFER_TYPE_DMA_HEAP: {
            p->os_api = (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DMA_HEAP)) ? allocator_dma_heap :
                        (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DRM)) ? allocator_drm :
                        (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_ION)) ? allocator_ion :
                        allocator_std;
        } break;
        default : {
        } break;
        }

        if (p->os_api.open(&p->ctx, SZ_4K, flags))
            break;

        /* update the real buffer type and flags */
        p->type = p->os_api.type;
        if (p->os_api.flags)
            p->flags = p->os_api.flags(p->ctx);

        {
            pthread_mutexattr_t attr;

            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&p->lock, &attr);
            pthread_mutexattr_destroy(&attr);
        }

        *allocator = p;
        *api = &mpp_allocator_api;

        return MPP_OK;
    } while (0);

    MPP_FREE(p);
    *allocator  = NULL;
    *api        = NULL;

    return MPP_NOK;
}

MPP_RET mpp_allocator_put(MppAllocator *allocator)
{
    MppAllocatorImpl *p = (MppAllocatorImpl *)*allocator;

    if (!p) {
        mpp_err_f("invalid input: allocator %p\n", allocator);
        return MPP_ERR_NULL_PTR;
    }

    if (!p)
        return MPP_OK;
    *allocator = NULL;

    if (p->os_api.close && p->ctx)
        p->os_api.close(p->ctx);
    pthread_mutex_destroy(&p->lock);
    mpp_free(p);

    return MPP_OK;
}

MppAllocFlagType mpp_allocator_get_flags(const MppAllocator allocator)
{
    MppAllocatorImpl *p = (MppAllocatorImpl *)allocator;

    return p ? p->flags : MPP_ALLOC_FLAG_NONE;
}
