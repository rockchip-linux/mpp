/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Versdrm 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITDRMS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissdrms and
 * limitatdrms under the License.
 */

#define MODULE_TAG "mpp_dma_heap"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "os_mem.h"
#include "allocator_dma_heap.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_runtime.h"

#define DMA_HEAP_VALID_FD_FLAGS         (O_CLOEXEC | O_ACCMODE)
#define DMA_HEAP_VALID_HEAP_FLAGS       (0)

struct dma_heap_allocation_data {
    RK_U64 len;
    RK_U32 fd;
    RK_U32 fd_flags;
    RK_U64 heap_flags;
};

#define DMA_HEAP_IOC_MAGIC              'H'
#define DMA_HEAP_IOCTL_ALLOC            _IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)

static RK_U32 dma_heap_debug = 0;

#define DMA_HEAP_OPS                    (0x00000001)
#define DMA_HEAP_DEVICE                 (0x00000002)
#define DMA_HEAP_IOCTL                  (0x00000004)

#define dma_heap_dbg(flag, fmt, ...)    _mpp_dbg(dma_heap_debug, flag, fmt, ## __VA_ARGS__)
#define dma_heap_dbg_f(flag, fmt, ...)  _mpp_dbg_f(dma_heap_debug, flag, fmt, ## __VA_ARGS__)

#define dma_heap_dbg_ops(fmt, ...)      dma_heap_dbg(DMA_HEAP_OPS, fmt, ## __VA_ARGS__)
#define dma_heap_dbg_dev(fmt, ...)      dma_heap_dbg(DMA_HEAP_DEVICE, fmt, ## __VA_ARGS__)

typedef struct {
    RK_U32  alignment;
    RK_S32  device;
    RK_U32  flags;
} allocator_ctx_dmaheap;

typedef enum DmaHeapType_e {
    DMA_HEAP_CMA        = (1 << 0),
    DMA_HEAP_CACHABLE   = (1 << 1),
    DMA_HEAP_DMA64      = (1 << 2),
    DMA_HEAP_TYPE_MASK  = DMA_HEAP_CMA | DMA_HEAP_CACHABLE | DMA_HEAP_DMA64,
    DMA_HEAP_TYPE_NB,
} DmaHeapType;

static const char *heap_names[] = {
    "system-uncached-dma32",    /* 0 - default */
    "cma-uncached",             /* 1 -                                      DMA_HEAP_CMA */
    "system-dma32",             /* 2 -                  DMA_HEAP_CACHABLE                */
    "cma",                      /* 3 -                  DMA_HEAP_CACHABLE | DMA_HEAP_CMA */
    "system-uncached",          /* 4 - DMA_HEAP_DMA64                                    */
    "cma-uncached",             /* 5 - DMA_HEAP_DMA64                     | DMA_HEAP_CMA */
    "system",                   /* 6 - DMA_HEAP_DMA64 | DMA_HEAP_CACHABLE                */
    "cma",                      /* 7 - DMA_HEAP_DMA64 | DMA_HEAP_CACHABLE | DMA_HEAP_CMA */
};

static int heap_fds[DMA_HEAP_TYPE_NB];
static pthread_once_t dma_heap_once = PTHREAD_ONCE_INIT;
static spinlock_t dma_heap_lock;

static int dma_heap_alloc(int fd, size_t len, RK_S32 *dmabuf_fd, RK_U32 flags)
{
    int ret;
    struct dma_heap_allocation_data data = {
        .len = len,
        .fd_flags = O_RDWR | O_CLOEXEC,
        .heap_flags = flags,
    };

    memset(&data, 0, sizeof(data));
    data.len = len;
    data.fd_flags = O_RDWR | O_CLOEXEC;
    data.heap_flags = flags;

    ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &data);

    dma_heap_dbg(DMA_HEAP_IOCTL, "ioctl alloc ret %d %s\n", ret, strerror(errno));

    if (ret < 0)
        return ret;

    *dmabuf_fd = data.fd;

    return ret;
}

static void heap_fds_init(void)
{
    memset(heap_fds, -1, sizeof(heap_fds));
    mpp_spinlock_init(&dma_heap_lock);
}

static int heap_fd_open(DmaHeapType type)
{
    mpp_assert(type < DMA_HEAP_TYPE_NB);

    mpp_spinlock_lock(&dma_heap_lock);

    if (heap_fds[type] <= 0) {
        static const char *heap_path = "/dev/dma_heap/";
        char name[64];
        int fd;

        snprintf(name, sizeof(name) - 1, "%s%s", heap_path, heap_names[type]);
        fd = open(name, O_RDWR | O_CLOEXEC);
        mpp_assert(fd > 0);

        dma_heap_dbg(DMA_HEAP_DEVICE, "open dma heap dev %s fd %d\n", name, fd);
        heap_fds[type] = fd;
    }

    mpp_spinlock_unlock(&dma_heap_lock);

    return heap_fds[type];
}


static MPP_RET os_allocator_dma_heap_open(void **ctx, MppAllocatorCfg *cfg)
{
    allocator_ctx_dmaheap *p;
    DmaHeapType type = 0;
    RK_S32 fd;

    mpp_env_get_u32("dma_heap_debug", &dma_heap_debug, 0);

    pthread_once(&dma_heap_once, heap_fds_init);

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    if (cfg->flags & (MPP_BUFFER_FLAGS_CONTIG >> 16))
        type |= DMA_HEAP_CMA;

    if (cfg->flags & (MPP_BUFFER_FLAGS_CACHABLE >> 16))
        type |= DMA_HEAP_CACHABLE;

    fd = heap_fd_open(type);
    if (fd < 0) {
        mpp_err_f("open dma heap type %x failed!\n", type);
        return MPP_ERR_UNKNOW;
    }

    p = mpp_malloc(allocator_ctx_dmaheap, 1);
    if (NULL == p) {
        close(fd);
        mpp_err_f("failed to allocate context\n");
        return MPP_ERR_MALLOC;
    } else {
        /*
         * default drm use cma, do nothing here
         */
        p->alignment    = cfg->alignment;
        p->flags        = cfg->flags;
        p->device       = fd;
        *ctx = p;
    }

    dma_heap_dbg_ops("dev %d open heap type %x:%x\n", fd, cfg->flags, type);

    return MPP_OK;
}

static MPP_RET os_allocator_dma_heap_alloc(void *ctx, MppBufferInfo *info)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_dmaheap *p = NULL;

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_dmaheap *)ctx;

    ret = dma_heap_alloc(p->device, info->size, (RK_S32 *)&info->fd, p->flags);

    dma_heap_dbg_ops("dev %d alloc %3d size %d\n", p->device, info->fd, info->size);

    if (ret) {
        mpp_err_f("dma_heap_alloc failed ret %d\n", ret);
        return ret;
    }

    info->ptr = NULL;
    return ret;
}

static MPP_RET os_allocator_dma_heap_import(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_dmaheap *p = (allocator_ctx_dmaheap *)ctx;
    RK_S32 fd_ext = data->fd;
    MPP_RET ret = MPP_OK;

    mpp_assert(fd_ext > 0);

    data->fd = dup(fd_ext);
    data->ptr = NULL;

    dma_heap_dbg_ops("dev %d import %3d -> %3d\n", p->device, fd_ext, data->fd);

    mpp_assert(data->fd > 0);

    return ret;
}

static MPP_RET os_allocator_dma_heap_free(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_dmaheap *p = NULL;
    MPP_RET ret = MPP_OK;

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_dmaheap *)ctx;

    dma_heap_dbg_ops("dev %d free  %3d size %d ptr %p\n", p->device,
                     data->fd, data->size, data->ptr);

    if (data->ptr) {
        munmap(data->ptr, data->size);
        data->ptr = NULL;
    }
    close(data->fd);

    return ret;
}

static MPP_RET os_allocator_dma_heap_close(void *ctx)
{
    int ret;
    allocator_ctx_dmaheap *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_close doesn't accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_dmaheap *)ctx;
    dma_heap_dbg_ops("dev %d close", p->device);

    ret = close(p->device);
    mpp_free(p);
    if (ret < 0)
        return (MPP_RET) - errno;

    return MPP_OK;
}

static MPP_RET os_allocator_dma_heap_mmap(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_dmaheap *p;
    MPP_RET ret = MPP_OK;
    if (NULL == ctx) {
        mpp_err("os_allocator_close do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    p = (allocator_ctx_dmaheap *)ctx;

    if (NULL == ctx)
        return MPP_ERR_NULL_PTR;

    if (NULL == data->ptr) {
        int flags = PROT_READ;

        if (fcntl(data->fd, F_GETFL) & O_RDWR)
            flags |= PROT_WRITE;

        data->ptr = mmap(NULL, data->size, flags, MAP_SHARED, data->fd, 0);
        if (data->ptr == MAP_FAILED) {
            mpp_err("mmap failed: %s\n", strerror(errno));
            data->ptr = NULL;
            return -errno;
        }

        dma_heap_dbg_ops("dev %d mmap  %3d ptr  %p (%s)\n", p->device,
                         data->fd, data->ptr,
                         flags & PROT_WRITE ? "RDWR" : "RDONLY");
    }

    return ret;
}

os_allocator allocator_dma_heap = {
    .open = os_allocator_dma_heap_open,
    .close = os_allocator_dma_heap_close,
    .alloc = os_allocator_dma_heap_alloc,
    .free = os_allocator_dma_heap_free,
    .import = os_allocator_dma_heap_import,
    .release = os_allocator_dma_heap_free,
    .mmap = os_allocator_dma_heap_mmap,
};
