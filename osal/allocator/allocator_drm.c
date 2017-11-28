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

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/drm.h>
#include <linux/drm_mode.h>
#include <drm/rockchip_drm.h>

#include "os_mem.h"
#include "allocator_drm.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_runtime.h"

static RK_U32 drm_debug = 0;

#define DRM_FUNCTION                (0x00000001)
#define DRM_DEVICE                  (0x00000002)
#define DRM_CLIENT                  (0x00000004)
#define DRM_IOCTL                   (0x00000008)

#define drm_dbg(flag, fmt, ...) _mpp_dbg_f(drm_debug, flag, fmt, ## __VA_ARGS__)

#if defined(ANDROID) && !defined(__LP64__)
#include <errno.h> /* for EINVAL */

extern void *__mmap2(void *, size_t, int, int, int, size_t);

static inline void *drm_mmap(void *addr, size_t length, int prot, int flags,
                             int fd, loff_t offset)
{
    /* offset must be aligned to 4096 (not necessarily the page size) */
    if (offset & 4095) {
        errno = EINVAL;
        return MAP_FAILED;
    }

    return __mmap2(addr, length, prot, flags, fd, (size_t) (offset >> 12));
}

#  define drm_munmap(addr, length) \
              munmap(addr, length)

#else

/* assume large file support exists */
#  define drm_mmap(addr, length, prot, flags, fd, offset) \
              mmap(addr, length, prot, flags, fd, offset)

#  define drm_munmap(addr, length) \
              munmap(addr, length)

#endif

typedef struct {
    RK_U32  alignment;
    RK_S32  drm_device;
} allocator_ctx_drm;

static const char *dev_drm = "/dev/dri/renderD128";

static int drm_ioctl(int fd, int req, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, req, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    drm_dbg(DRM_FUNCTION, "drm_ioctl %x with code %d: %s", req,
            ret, strerror(errno));

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

    drm_dbg(DRM_FUNCTION, "get fd %d", *map_fd);

    if (*map_fd < 0) {
        mpp_err("map ioctl returned negative fd\n");
        return -EINVAL;
    }

    return ret;
}

static int drm_fd_to_handle(int fd, int map_fd, RK_U32 *handle, RK_U32 flags)
{
    int ret;
    struct drm_prime_handle dph;

    dph.fd = map_fd;
    dph.flags = flags;

    ret = drm_ioctl(fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &dph);
    if (ret < 0) {
        return ret;
    }

    *handle = dph.handle;

    drm_dbg(DRM_FUNCTION, "get handle %d", *handle);

    return ret;
}

static int drm_alloc(int fd, size_t len, size_t align, RK_U32 *handle)
{
    int ret;
    struct drm_rockchip_gem_create gem;

    drm_dbg(DRM_FUNCTION, "len %ld aligned %ld\n", len, align);

    memset(&gem, 0, sizeof gem);
    gem.size = (len + align - 1) & (~(align - 1));

    if (handle == NULL)
        return -EINVAL;

    ret = drm_ioctl(fd, DRM_IOCTL_ROCKCHIP_GEM_CREATE, &gem);
    if (ret < 0)
        return ret;

    *handle = gem.handle;
    drm_dbg(DRM_FUNCTION, "fd %d get handle %d aligned %d size %lld\n",
            fd, *handle, align, gem.size);

    return ret;
}

static int drm_free(int fd, RK_U32 handle)
{
    struct drm_gem_close data = {
        .handle = handle,
        .pad = 0
    };
    return drm_ioctl(fd, DRM_IOCTL_GEM_CLOSE, &data);
}

static MPP_RET os_allocator_drm_open(void **ctx, size_t alignment)
{
    RK_S32 fd;
    allocator_ctx_drm *p;

    drm_dbg(DRM_FUNCTION, "enter");

    if (NULL == ctx) {
        mpp_err("os_allocator_open does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    mpp_env_get_u32("drm_debug", &drm_debug, 0);

    fd = open(dev_drm, O_RDWR);
    if (fd < 0) {
        mpp_err("open %s failed!\n", dev_drm);
        return MPP_ERR_UNKNOW;
    }

    drm_dbg(DRM_DEVICE, "open drm dev fd %d\n", fd);

    p = mpp_malloc(allocator_ctx_drm, 1);
    if (NULL == p) {
        close(fd);
        mpp_err("os_allocator_open failed to allocate context\n");
        return MPP_ERR_MALLOC;
    } else {
        /*
         * default drm use cma, do nothing here
         */
        p->alignment    = alignment;
        p->drm_device   = fd;
        *ctx = p;
    }

    drm_dbg(DRM_FUNCTION, "leave");

    return MPP_OK;
}

static MPP_RET os_allocator_drm_alloc(void *ctx, MppBufferInfo *info)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_drm *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_alloc does not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;
    drm_dbg(DRM_FUNCTION, "alignment %d size %d", p->alignment, info->size);
    ret = drm_alloc(p->drm_device, info->size, p->alignment,
                    (RK_U32 *)&info->hnd);
    if (ret) {
        mpp_err("os_allocator_drm_alloc drm_alloc failed ret %d\n", ret);
        return ret;
    }
    drm_dbg(DRM_FUNCTION, "handle %d", (RK_U32)((intptr_t)info->hnd));
    ret = drm_handle_to_fd(p->drm_device, (RK_U32)((intptr_t)info->hnd),
                           &info->fd, 0);

    if (ret) {
        mpp_err("os_allocator_drm_alloc handle_to_fd failed ret %d\n", ret);
        return ret;
    }
    info->ptr = NULL;
    return ret;
}

static MPP_RET os_allocator_drm_import(void *ctx, MppBufferInfo *data)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_drm *p = (allocator_ctx_drm *)ctx;

    drm_dbg(DRM_FUNCTION, "enter");

    ret = drm_fd_to_handle(p->drm_device, data->fd, (RK_U32 *)&data->hnd, 0);

    drm_dbg(DRM_FUNCTION, "get handle %d", (intptr_t)(data->hnd));

    ret = drm_handle_to_fd(p->drm_device, (RK_U32)((intptr_t)data->hnd),
                           &data->fd, 0);

    data->ptr = NULL;
    drm_dbg(DRM_FUNCTION, "leave");

    return ret;
}

static MPP_RET os_allocator_drm_free(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_drm *p = NULL;

    if (NULL == ctx) {
        drm_dbg(DRM_FUNCTION, "invalid ctx");
        return MPP_ERR_NULL_PTR;
    }
    p = (allocator_ctx_drm *)ctx;

    if (data->ptr) {
        drm_munmap(data->ptr, data->size);
        data->ptr = NULL;
    }
    close(data->fd);

    return drm_free(p->drm_device, (RK_U32)((intptr_t)data->hnd));
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
    drm_dbg(DRM_FUNCTION, "close fd %d", p->drm_device);

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
        struct drm_rockchip_gem_map_off gem_map;

        memset(&gem_map, 0, sizeof gem_map);
        gem_map.handle = (RK_U32)(intptr_t)data->hnd;

        ret = drm_ioctl(p->drm_device, DRM_IOCTL_ROCKCHIP_GEM_MAP_OFFSET, &gem_map);
        if (ret) {
            mpp_err("rockchip_gem_map_offset failed: %s\n", strerror(ret));
            return ret;
        }

        data->ptr = drm_mmap(NULL, data->size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, p->drm_device, gem_map.offset);
        if (data->ptr == MAP_FAILED) {
            mpp_err("mmap failed: %s\n", strerror(errno));
            data->ptr = NULL;
            return -errno;
        }
    }

    return ret;
}

os_allocator allocator_drm = {
    .open = os_allocator_drm_open,
    .close = os_allocator_drm_close,
    .alloc = os_allocator_drm_alloc,
    .free = os_allocator_drm_free,
    .import = os_allocator_drm_import,
    .release = os_allocator_drm_free,
    .mmap = os_allocator_drm_mmap,
};
