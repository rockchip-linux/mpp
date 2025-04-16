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
#define DMA_HEAP_CHECK                  (0x00000008)

#define dma_heap_dbg(flag, fmt, ...)    _mpp_dbg(dma_heap_debug, flag, fmt, ## __VA_ARGS__)
#define dma_heap_dbg_f(flag, fmt, ...)  _mpp_dbg_f(dma_heap_debug, flag, fmt, ## __VA_ARGS__)

#define dma_heap_dbg_ops(fmt, ...)      dma_heap_dbg(DMA_HEAP_OPS, fmt, ## __VA_ARGS__)
#define dma_heap_dbg_dev(fmt, ...)      dma_heap_dbg(DMA_HEAP_DEVICE, fmt, ## __VA_ARGS__)
#define dma_heap_dbg_ioctl(fmt, ...)    dma_heap_dbg(DMA_HEAP_IOCTL, fmt, ## __VA_ARGS__)
#define dma_heap_dbg_chk(fmt, ...)      dma_heap_dbg(DMA_HEAP_CHECK, fmt, ## __VA_ARGS__)

typedef struct {
    RK_U32  alignment;
    RK_S32  device;
    RK_U32  flags;
} allocator_ctx_dmaheap;

typedef struct DmaHeapInfo_t {
    const char  *name;
    RK_S32      fd;
    RK_U32      flags;
} DmaHeapInfo;

static DmaHeapInfo heap_infos[MPP_ALLOC_FLAG_TYPE_NB] = {
    {   "system-uncached",          -1,                                                MPP_ALLOC_FLAG_NONE , },   /* 0 */
    {   "system-uncached-dma32",    -1,                                                MPP_ALLOC_FLAG_DMA32, },   /* 1 */
    {   "system",                   -1,                      MPP_ALLOC_FLAG_CACHABLE                       , },   /* 2 */
    {   "system-dma32",             -1,                      MPP_ALLOC_FLAG_CACHABLE | MPP_ALLOC_FLAG_DMA32, },   /* 3 */
    {   "cma-uncached",             -1, MPP_ALLOC_FLAG_CMA                                                 , },   /* 4 */
    {   "cma-uncached-dma32",       -1, MPP_ALLOC_FLAG_CMA                           | MPP_ALLOC_FLAG_DMA32, },   /* 5 */
    {   "cma",                      -1, MPP_ALLOC_FLAG_CMA | MPP_ALLOC_FLAG_CACHABLE                       , },   /* 6 */
    {   "cma-dma32",                -1, MPP_ALLOC_FLAG_CMA | MPP_ALLOC_FLAG_CACHABLE | MPP_ALLOC_FLAG_DMA32, },   /* 7 */
};

static int try_open_path(const char *name)
{
    static const char *heap_path = "/dev/dma_heap/";
    char buf[64];
    int fd;

    snprintf(buf, sizeof(buf) - 1, "%s%s", heap_path, name);
    fd = open(buf, O_RDONLY | O_CLOEXEC); // read permission is enough

    dma_heap_dbg_ops("open dma_heap %-24s -> fd %d\n", name, fd);

    return fd;
}

static MPP_RET try_flip_flag(RK_U32 orig, RK_U32 flag)
{
    DmaHeapInfo *dst = &heap_infos[orig];
    DmaHeapInfo *src;
    RK_U32 used;

    if (orig & flag)
        used = (RK_U32)(orig & (~flag));
    else
        used = (RK_U32)(orig | flag);

    src = &heap_infos[used];
    if (src->fd > 0) {
        /* found valid heap use it */
        dst->fd = mpp_dup(src->fd);
        dst->flags = src->flags;

        dma_heap_dbg_chk("dma-heap type %x %s remap to %x %s\n",
                         orig, dst->name, used, src->name);
    }

    return dst->fd > 0 ? MPP_OK : MPP_NOK;
}

__attribute__ ((constructor))
void dma_heap_init(void)
{
    DmaHeapInfo *info = NULL;
    RK_U32 all_success = 1;
    RK_U32 i;

    mpp_env_get_u32("dma_heap_debug", &dma_heap_debug, 0);

    /* go through all heap first */
    for (i = 0; i < MPP_ARRAY_ELEMS(heap_infos); i++) {
        info = &heap_infos[i];

        if (info->fd > 0)
            continue;

        info->fd = try_open_path(info->name);
        if (info->fd <= 0)
            all_success = 0;
    }

    if (!all_success) {
        /* check remaining failed heap mapping */
        for (i = 0; i < MPP_ARRAY_ELEMS(heap_infos); i++) {
            info = &heap_infos[i];

            if (info->fd > 0)
                continue;

            /* if original heap failed then try revert cacheable flag */
            if (MPP_OK == try_flip_flag((RK_U32)i, MPP_ALLOC_FLAG_CACHABLE))
                continue;

            /* if cacheable heap failed then try revert dma32 flag */
            if (MPP_OK == try_flip_flag((RK_U32)i, MPP_ALLOC_FLAG_DMA32))
                continue;

            /* if dma32 heap failed then try revert both cacheable and dma32 flag */
            if (MPP_OK == try_flip_flag((RK_U32)i, MPP_ALLOC_FLAG_CACHABLE | MPP_ALLOC_FLAG_DMA32))
                continue;

            dma_heap_dbg_chk("dma-heap type %x - %s remap failed\n", i, info->name);
        }
    }
}

__attribute__ ((destructor))
void dma_heap_deinit(void)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(heap_infos); i++) {
        DmaHeapInfo *info = &heap_infos[i];

        if (info->fd > 0) {
            close(info->fd);
            info->fd = -1;
        }
    }
}

static int dma_heap_alloc(int fd, size_t len, RK_S32 *dmabuf_fd, RK_U32 flags)
{
    struct dma_heap_allocation_data data;
    int ret;

    memset(&data, 0, sizeof(data));
    data.len = len;
    data.fd_flags = O_RDWR | O_CLOEXEC;
    data.heap_flags = 0; // heap_flags should be set to 0

    ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &data);
    if (ret < 0) {
        mpp_err("ioctl alloc failed for %s\n", strerror(errno));
        return ret;
    }

    dma_heap_dbg_ioctl("ioctl alloc get fd %d\n", data.fd);

    *dmabuf_fd = data.fd;

    (void) flags;
    return ret;
}

static MPP_RET os_allocator_dma_heap_open(void **ctx, size_t alignment, MppAllocFlagType flags)
{
    allocator_ctx_dmaheap *p;
    DmaHeapInfo *info = NULL;
    RK_U32 type = 0;

    mpp_env_get_u32("dma_heap_debug", &dma_heap_debug, dma_heap_debug);

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    info = &heap_infos[flags];
    if (info->fd <= 0) {
        mpp_err_f("open dma heap type %x %s failed!\n", type, info->name);
        return MPP_ERR_UNKNOW;
    }

    p = mpp_malloc(allocator_ctx_dmaheap, 1);
    if (NULL == p) {
        mpp_err_f("failed to allocate context\n");
        return MPP_ERR_MALLOC;
    } else {
        p->alignment    = alignment;
        p->flags        = info->flags;
        p->device       = info->fd;
        *ctx = p;
    }

    dma_heap_dbg_ops("dev %d open heap type %x:%x\n", p->device, flags, info->flags);

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

    data->fd = mpp_dup(fd_ext);
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
    if (NULL == ctx) {
        mpp_err("os_allocator_close doesn't accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_FREE(ctx);

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

static MppAllocFlagType os_allocator_dma_heap_flags(void *ctx)
{
    allocator_ctx_dmaheap *p = (allocator_ctx_dmaheap *)ctx;

    return p ? (MppAllocFlagType)p->flags : MPP_ALLOC_FLAG_NONE;
}

os_allocator allocator_dma_heap = {
    .type = MPP_BUFFER_TYPE_DMA_HEAP,
    .name = "dma_heap",
    .open = os_allocator_dma_heap_open,
    .close = os_allocator_dma_heap_close,
    .alloc = os_allocator_dma_heap_alloc,
    .free = os_allocator_dma_heap_free,
    .import = os_allocator_dma_heap_import,
    .release = os_allocator_dma_heap_free,
    .mmap = os_allocator_dma_heap_mmap,
    .flags = os_allocator_dma_heap_flags,
};
