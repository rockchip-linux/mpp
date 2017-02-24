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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/drm.h>
#include <linux/drm_mode.h>

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

typedef struct {
    RK_U32  alignment;
    RK_S32  drm_device;
} allocator_ctx_drm;

static const char *dev_drm = "/dev/dri/card0";

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

static void* drm_mmap(int fd, size_t len, int prot, int flags, loff_t offset)
{
    static unsigned long pagesize_mask = 0;
#if !defined(__gnu_linux__)
    func_mmap64 fp_mmap64 = mpp_rt_get_mmap64();
#endif

    if (fd < 0)
        return NULL;

    if (!pagesize_mask)
        pagesize_mask = sysconf(_SC_PAGESIZE) - 1;

    len = (len + pagesize_mask) & ~pagesize_mask;

    if (offset & 4095) {
        return NULL;
    }

#if !defined(__gnu_linux__)
    if (fp_mmap64)
        return fp_mmap64(NULL, len, prot, flags, fd, offset);

    return NULL;
#else
    return mmap64(NULL, len, prot, flags, fd, offset);
#endif
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
    if (ret < 0) {
        return ret;
    }

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

static int drm_map(int fd, RK_U32 handle, size_t length, int prot,
                   int flags, unsigned char **ptr, int *map_fd)
{
    int ret;
    struct drm_mode_map_dumb dmmd;
    memset(&dmmd, 0, sizeof(dmmd));
    dmmd.handle = handle;

    if (map_fd == NULL)
        return -EINVAL;
    if (ptr == NULL)
        return -EINVAL;

    ret = drm_handle_to_fd(fd, handle, map_fd, 0);
    drm_dbg(DRM_FUNCTION, "drm_map fd %d", *map_fd);
    if (ret < 0)
        return ret;

    ret = drm_ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &dmmd);
    if (ret < 0) {
        close(*map_fd);
        return ret;
    }

    drm_dbg(DRM_FUNCTION, "dev fd %d length %d", fd, length);

    *ptr = drm_mmap(fd, length, prot, flags, dmmd.offset);
    if (*ptr == MAP_FAILED) {
        close(*map_fd);
        *map_fd = -1;
        mpp_err("mmap failed: %s\n", strerror(errno));
        return -errno;
    }

    return ret;
}

static int drm_alloc(int fd, size_t len, size_t align, RK_U32 *handle)
{
    int ret;
    struct drm_mode_create_dumb dmcb;

    drm_dbg(DRM_FUNCTION, "len %ld aligned %ld\n", len, align);

    memset(&dmcb, 0, sizeof(struct drm_mode_create_dumb));
    dmcb.bpp = 8;
    dmcb.width = (len + align - 1) & (~(align - 1));
    dmcb.height = 1;
    dmcb.size = dmcb.width * dmcb.bpp;

    drm_dbg(DRM_FUNCTION, "fd %d aligned %d size %lld\n", fd, align, dmcb.size);

    if (handle == NULL)
        return -EINVAL;

    ret = drm_ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &dmcb);
    if (ret < 0)
        return ret;
    *handle = dmcb.handle;

    drm_dbg(DRM_FUNCTION, "get handle %d size %d", *handle, dmcb.size);

    return ret;
}

static int drm_free(int fd, RK_U32 handle)
{
    struct drm_mode_destroy_dumb data = {
        .handle = handle,
    };
    return drm_ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &data);
}

static MPP_RET os_allocator_drm_open(void **ctx, size_t alignment)
{
    RK_S32 fd;
    allocator_ctx_drm *p;

    drm_dbg(DRM_FUNCTION, "enter");

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
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
        mpp_err("os_allocator_open Android failed to allocate context\n");
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
        mpp_err("os_allocator_close Android do not accept NULL input\n");
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
    ret = drm_map(p->drm_device, (RK_U32)((intptr_t)info->hnd), info->size,
                  PROT_READ | PROT_WRITE, MAP_SHARED,
                  (unsigned char **)&info->ptr, &info->fd);
    if (ret) {
        mpp_err("os_allocator_drm_alloc drm_map failed ret %d\n", ret);
        return ret;
    }
    return ret;
}

static MPP_RET os_allocator_drm_import(void *ctx, MppBufferInfo *data)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_drm *p = (allocator_ctx_drm *)ctx;
    struct drm_mode_map_dumb dmmd;
    memset(&dmmd, 0, sizeof(dmmd));

    drm_dbg(DRM_FUNCTION, "enter");
    // NOTE: do not use the original buffer fd,
    //       use dup fd to avoid unexpected external fd close
    data->fd = dup(data->fd);

    ret = drm_fd_to_handle(p->drm_device, data->fd, (RK_U32 *)&data->hnd, 0);

    drm_dbg(DRM_FUNCTION, "get handle %d", (RK_U32)(data->hnd));

    dmmd.handle = (RK_U32)(data->hnd);

    ret = drm_ioctl(p->drm_device, DRM_IOCTL_MODE_MAP_DUMB, &dmmd);
    if (ret < 0)
        return ret;

    drm_dbg(DRM_FUNCTION, "dev fd %d length %d", p->drm_device, data->size);

    data->ptr = drm_mmap(p->drm_device, data->size, PROT_READ | PROT_WRITE,
                         MAP_SHARED, dmmd.offset);
    if (data->ptr == MAP_FAILED) {
        mpp_err("mmap failed: %s\n", strerror(errno));
        return -errno;
    }

    drm_dbg(DRM_FUNCTION, "leave");

    return ret;
}

static MPP_RET os_allocator_drm_release(void *ctx, MppBufferInfo *data)
{
    (void)ctx;
    munmap(data->ptr, data->size);
    close(data->fd);
    return MPP_OK;
}

static MPP_RET os_allocator_drm_free(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_drm *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;
    munmap(data->ptr, data->size);
    close(data->fd);
    drm_free(p->drm_device, (RK_U32)((intptr_t)data->hnd));
    return MPP_OK;
}

static MPP_RET os_allocator_drm_close(void *ctx)
{
    int ret;
    allocator_ctx_drm *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
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

os_allocator allocator_drm = {
    .open = os_allocator_drm_open,
    .close = os_allocator_drm_close,
    .alloc = os_allocator_drm_alloc,
    .free = os_allocator_drm_free,
    .import = os_allocator_drm_import,
    .release = os_allocator_drm_release,
    .mmap = NULL,
};
