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

#define MODULE_TAG "mpp_drm"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/drm.h>
#include <linux/drm_mode.h>

#include "os_mem.h"
#include "allocator_drm.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_runtime.h"

#define DRM_FUNCTION                (0x00000001)
#define DRM_DEVICE                  (0x00000002)
#define DRM_IOCTL                   (0x00000004)

#define drm_dbg(flag, fmt, ...)     _mpp_dbg_f(drm_debug, flag, fmt, ## __VA_ARGS__)
#define drm_dbg_func(fmt, ...)      drm_dbg(DRM_FUNCTION, fmt, ## __VA_ARGS__)
#define drm_dbg_dev(fmt, ...)       drm_dbg(DRM_DEVICE, fmt, ## __VA_ARGS__)
#define drm_dbg_ioctl(fmt, ...)     drm_dbg(DRM_IOCTL, fmt, ## __VA_ARGS__)

static RK_U32 drm_debug = 0;

/* memory type definitions. */
enum drm_rockchip_gem_mem_type {
    /* Physically Continuous memory. */
    ROCKCHIP_BO_CONTIG      = 1 << 0,
    /* cachable mapping. */
    ROCKCHIP_BO_CACHABLE    = 1 << 1,
    /* write-combine mapping. */
    ROCKCHIP_BO_WC          = 1 << 2,
    ROCKCHIP_BO_SECURE      = 1 << 3,
    /* keep kmap for cma buffer or alloc kmap for other type memory */
    ROCKCHIP_BO_ALLOC_KMAP  = 1 << 4,
    /* alloc page with gfp_dma32 */
    ROCKCHIP_BO_DMA32       = 1 << 5,
    ROCKCHIP_BO_MASK        = ROCKCHIP_BO_CONTIG | ROCKCHIP_BO_CACHABLE |
                              ROCKCHIP_BO_WC | ROCKCHIP_BO_SECURE | ROCKCHIP_BO_ALLOC_KMAP |
                              ROCKCHIP_BO_DMA32,
};

typedef struct {
    RK_U32  alignment;
    RK_S32  drm_device;
    RK_U32  flags;
} allocator_ctx_drm;

/* use renderD128 first to avoid GKI kernel permission issue */
static const char *dev_drm[] = {
    "/dev/dri/renderD128",
    "/dev/dri/card0",
};

static RK_U32 to_rockchip_gem_mem_flag(RK_U32 flags)
{
    RK_U32 ret = 0;

    if (flags & MPP_ALLOC_FLAG_DMA32)
        ret |= ROCKCHIP_BO_DMA32;

    if (flags & MPP_ALLOC_FLAG_CACHABLE)
        ret |= ROCKCHIP_BO_CACHABLE;

    if (flags & MPP_ALLOC_FLAG_CMA)
        ret |= ROCKCHIP_BO_CONTIG;

    return ret;
}

static int drm_ioctl(int fd, int req, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, req, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    drm_dbg_ioctl("%x ret %d: %s\n", req, ret, strerror(errno));

    return ret;
}

static int drm_handle_to_fd(int fd, RK_U32 handle, int *map_fd, RK_U32 flags)
{
    int ret;
    struct drm_prime_handle dph;

    memset(&dph, 0, sizeof(struct drm_prime_handle));
    dph.handle = handle;
    dph.fd = -1;
    dph.flags = flags;

    if (map_fd == NULL)
        return -EINVAL;

    ret = drm_ioctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &dph);
    if (ret < 0)
        return ret;

    *map_fd = dph.fd;

    drm_dbg_func("dev %d handle %d flags %x get fd %d\n", fd, handle, dph.flags, *map_fd);

    if (*map_fd < 0) {
        mpp_err_f("map ioctl returned negative fd\n");
        return -EINVAL;
    }

    return ret;
}

static int drm_alloc(int fd, size_t len, size_t align, RK_U32 *handle, RK_U32 flags)
{
    int ret;
    struct drm_mode_create_dumb dmcb;

    memset(&dmcb, 0, sizeof(struct drm_mode_create_dumb));
    dmcb.bpp = 8;
    dmcb.width = (len + align - 1) & (~(align - 1));
    dmcb.height = 1;
    dmcb.flags = to_rockchip_gem_mem_flag(flags);

    if (handle == NULL)
        return -EINVAL;

    ret = drm_ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &dmcb);
    if (ret < 0)
        return ret;

    *handle = dmcb.handle;
    drm_dbg_func("dev %d alloc aligned %d flags %x|%x handle %d size %lld\n", fd,
                 align, flags, dmcb.flags, dmcb.handle, dmcb.size);

    return ret;
}

static int drm_free(int fd, RK_U32 handle)
{
    struct drm_mode_destroy_dumb data = {
        .handle = handle,
    };
    return drm_ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &data);
}

static MPP_RET os_allocator_drm_open(void **ctx, size_t alignment, MppAllocFlagType flags)
{
    allocator_ctx_drm *p;
    RK_S32 fd;
    RK_S32 i;

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    mpp_env_get_u32("drm_debug", &drm_debug, 0);

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dev_drm); i++) {
        fd = open(dev_drm[i], O_RDWR | O_CLOEXEC);
        if (fd > 0)
            break;
    }

    if (fd < 0) {
        mpp_err_f("open all drm device failed.\n");
        mpp_err("Please check the following device path and access permission:\n");
        for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dev_drm); i++)
            mpp_err("%s\n", dev_drm[i]);
        return MPP_ERR_UNKNOW;
    } else {
        /* drop master by default to avoid becoming the drm master */
        drm_ioctl(fd, DRM_IOCTL_DROP_MASTER, 0);
    }

    drm_dbg_dev("open drm dev fd %d flags %x\n", fd, flags);

    p = mpp_malloc(allocator_ctx_drm, 1);
    if (NULL == p) {
        close(fd);
        mpp_err_f("failed to allocate context\n");
        return MPP_ERR_MALLOC;
    } else {
        /*
         * default drm use cma, do nothing here
         */
        p->alignment    = alignment;
        p->flags        = flags;
        p->drm_device   = fd;
        *ctx = p;
    }

    return MPP_OK;
}

static MPP_RET os_allocator_drm_alloc(void *ctx, MppBufferInfo *info)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_drm *p = NULL;

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;

    drm_dbg_func("dev %d alloc alignment %d size %d\n", p->drm_device,
                 p->alignment, info->size);

    ret = drm_alloc(p->drm_device, info->size, p->alignment,
                    (RK_U32 *)&info->hnd, p->flags);
    if (ret) {
        mpp_err_f("drm_alloc failed ret %d\n", ret);
        return ret;
    }

    ret = drm_handle_to_fd(p->drm_device, (RK_U32)((intptr_t)info->hnd),
                           &info->fd, DRM_CLOEXEC | DRM_RDWR);

    if (ret) {
        mpp_err_f("handle_to_fd failed ret %d\n", ret);
        return ret;
    }

    drm_dbg_func("dev %d get handle %d with fd %d\n", p->drm_device,
                 (RK_U32)((intptr_t)info->hnd), info->fd);

    /* release handle to reduce iova usage */
    drm_free(p->drm_device, (RK_U32)((intptr_t)info->hnd));
    info->hnd = NULL;
    info->ptr = NULL;

    return ret;
}

static MPP_RET os_allocator_drm_import(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_drm *p = (allocator_ctx_drm *)ctx;
    RK_S32 fd_ext = data->fd;
    MPP_RET ret = MPP_OK;

    drm_dbg_func("enter dev %d fd %d\n", p->drm_device, fd_ext);

    mpp_assert(fd_ext > 0);

    data->fd = mpp_dup(fd_ext);
    data->ptr = NULL;

    if (data->fd <= 0) {
        mpp_err_f(" fd dup return invalid fd %d\n", data->fd);
        ret = MPP_NOK;
    }

    drm_dbg_func("leave dev %d fd %d -> %d\n", p->drm_device, fd_ext, data->fd);

    return ret;
}

static MPP_RET os_allocator_drm_free(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_drm *p = NULL;

    if (NULL == ctx) {
        mpp_err_f("does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;

    drm_dbg_func("dev %d handle %p unmap %p fd %d size %d\n", p->drm_device,
                 data->hnd, data->ptr, data->fd, data->size);

    if (data->ptr) {
        munmap(data->ptr, data->size);
        data->ptr = NULL;
    }

    if (data->fd > 0) {
        close(data->fd);
        data->fd = -1;
    } else {
        mpp_err_f("can not close invalid fd %d\n", data->fd);
    }

    return MPP_OK;
}

static MPP_RET os_allocator_drm_close(void *ctx)
{
    int ret;
    allocator_ctx_drm *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_close doesn't accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;
    drm_dbg_func("dev %d", p->drm_device);

    ret = close(p->drm_device);
    mpp_free(p);
    if (ret < 0)
        return (MPP_RET) - errno;

    return MPP_OK;
}

static MPP_RET os_allocator_drm_mmap(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_drm *p;
    MPP_RET ret = MPP_OK;
    if (NULL == ctx) {
        mpp_err("os_allocator_close do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    p = (allocator_ctx_drm *)ctx;

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

        drm_dbg_func("dev %d mmap fd %d to %p (%s)\n", p->drm_device,
                     data->fd, data->ptr,
                     flags & PROT_WRITE ? "RDWR" : "RDONLY");
    }

    return ret;
}

static MppAllocFlagType os_allocator_drm_flags(void *ctx)
{
    allocator_ctx_drm *p = (allocator_ctx_drm *)ctx;

    return p ? (MppAllocFlagType)p->flags : MPP_ALLOC_FLAG_NONE;
}

os_allocator allocator_drm = {
    .type = MPP_BUFFER_TYPE_DRM,
    .name = "drm",
    .open = os_allocator_drm_open,
    .close = os_allocator_drm_close,
    .alloc = os_allocator_drm_alloc,
    .free = os_allocator_drm_free,
    .import = os_allocator_drm_import,
    .release = os_allocator_drm_free,
    .mmap = os_allocator_drm_mmap,
    .flags = os_allocator_drm_flags,
};
