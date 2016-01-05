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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/ion.h>

#include "os_mem.h"
#include "os_allocator.h"

#include "mpp_mem.h"
#include "mpp_log.h"

typedef struct {
    RK_U32  alignment;
} allocator_ctx_normal;

MPP_RET os_allocator_normal_open(void **ctx, size_t alignment)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_normal *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(allocator_ctx_normal, 1);
    if (NULL == p) {
        mpp_err("os_allocator_open Android failed to allocate context\n");
        ret = MPP_ERR_MALLOC;
    } else
        p->alignment = alignment;

    *ctx = p;
    return ret;
}

MPP_RET os_allocator_normal_alloc(void *ctx, MppBufferInfo *info)
{
    allocator_ctx_normal *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_normal *)ctx;
    return os_malloc(&info->ptr, p->alignment, info->size);
}

MPP_RET os_allocator_normal_free(void *ctx, MppBufferInfo *info)
{
    (void) ctx;
    if (info->ptr)
        os_free(info->ptr);
    return MPP_OK;
}

MPP_RET os_allocator_normal_close(void *ctx)
{
    if (ctx) {
        mpp_free(ctx);
        return MPP_OK;
    }
    mpp_err("os_allocator_close Linux found NULL context input\n");
    return MPP_NOK;
}

static os_allocator allocator_normal = {
    os_allocator_normal_open,
    os_allocator_normal_alloc,
    os_allocator_normal_free,
    NULL,
    NULL,
    os_allocator_normal_close,
};

static RK_U32 ion_debug = 0;

#define ION_FUNCTION                (0x00000001)
#define ION_DEVICE                  (0x00000002)
#define ION_CLINET                  (0x00000004)
#define ION_IOCTL                   (0x00000008)

#define ion_dbg(flag, fmt, ...) _mpp_dbg(ion_debug, flag, fmt, ## __VA_ARGS__)

static int ion_ioctl(int fd, int req, void *arg)
{
    int ret = ioctl(fd, req, arg);
    if (ret < 0) {
        mpp_err("ion_ioctl %x failed with code %d: %s\n", req,
                ret, strerror(errno));
        return -errno;
    }
    return ret;
}

static int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
                     unsigned int flags, ion_user_handle_t *handle)
{
    int ret;
    struct ion_allocation_data data = {
        .len = len,
        .align = align,
        .heap_id_mask = heap_mask,
        .flags = flags,
    };

    if (handle == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
    if (ret < 0)
        return ret;
    *handle = data.handle;
    return ret;
}

static int ion_free(int fd, ion_user_handle_t handle)
{
    struct ion_handle_data data = {
        .handle = handle,
    };
    return ion_ioctl(fd, ION_IOC_FREE, &data);
}

static int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
                   int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
    int ret;
    struct ion_fd_data data = {
        .handle = handle,
    };

    if (map_fd == NULL)
        return -EINVAL;
    if (ptr == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_MAP, &data);
    if (ret < 0)
        return ret;
    *map_fd = data.fd;
    if (*map_fd < 0) {
        mpp_err("map ioctl returned negative fd\n");
        return -EINVAL;
    }
    *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
    if (*ptr == MAP_FAILED) {
        mpp_err("mmap failed: %s\n", strerror(errno));
        return -errno;
    }
    return ret;
}

typedef struct {
    RK_U32  alignment;
    RK_S32  ion_device;
    RK_U32  heap_id;
} allocator_ctx_ion;

MPP_RET os_allocator_ion_open(void **ctx, size_t alignment)
{
    RK_S32 fd;
    allocator_ctx_ion *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    fd = open("/dev/ion", O_RDWR);
    if (fd < 0) {
        mpp_err("open /dev/ion failed!\n");
        return MPP_ERR_UNKNOW;
    }

    ion_dbg(ION_DEVICE, "open ion dev fd %d\n", fd);

    p = mpp_malloc(allocator_ctx_ion, 1);
    if (NULL == p) {
        close(fd);
        mpp_err("os_allocator_open Android failed to allocate context\n");
        return MPP_ERR_MALLOC;
    } else {
        p->alignment    = alignment;
        p->ion_device   = fd;
        p->heap_id      = ION_HEAP_TYPE_SYSTEM_CONTIG;  /* ION_HEAP_TYPE_CARVEOUT */
        *ctx = p;
    }

    return MPP_OK;
}

MPP_RET os_allocator_ion_alloc(void *ctx, MppBufferInfo *info)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_ion *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_ion *)ctx;
    ret = ion_alloc(p->ion_device, info->size, p->alignment,
                    p->heap_id, 0,
                    (ion_user_handle_t *)&info->hnd);
    if (ret) {
        mpp_err("os_allocator_ion_alloc ion_alloc failed ret %d\n", ret);
        return ret;
    }
    ret = ion_map(p->ion_device, (ion_user_handle_t)info->hnd, info->size,
                  PROT_READ | PROT_WRITE, MAP_SHARED, (off_t)0,
                  (unsigned char**)&info->ptr, &info->fd);
    if (ret) {
        mpp_err("os_allocator_ion_alloc ion_map failed ret %d\n", ret);
        return ret;
    }
    return ret;
}

MPP_RET os_allocator_ion_import(void *ctx, MppBufferInfo *data)
{
    (void)ctx;
    // NOTE: do not use the original buffer fd,
    //       use dup fd to avoid unexpected external fd close
    data->fd = dup(data->fd);
    data->ptr = mmap(NULL, data->size, PROT_READ | PROT_WRITE, MAP_SHARED, data->fd, 0);
    return (data->ptr) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET os_allocator_ion_release(void *ctx, MppBufferInfo *data)
{
    (void)ctx;
    munmap(data->ptr, data->size);
    close(data->fd);
    return MPP_OK;
}

MPP_RET os_allocator_ion_free(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_ion *p = NULL;
    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_ion *)ctx;
    munmap(data->ptr, data->size);
    close(data->fd);
    ion_free(p->ion_device, (ion_user_handle_t)data->hnd);
    return MPP_OK;
}

MPP_RET os_allocator_ion_close(void *ctx)
{
    int ret;
    allocator_ctx_ion *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_ion *)ctx;
    ret = close(p->ion_device);
    mpp_free(p);
    if (ret < 0)
        return (MPP_RET) - errno;
    return MPP_OK;
}

static os_allocator allocator_ion = {
    os_allocator_ion_open,
    os_allocator_ion_alloc,
    os_allocator_ion_free,
    os_allocator_ion_import,
    os_allocator_ion_release,
    os_allocator_ion_close,
};

MPP_RET os_allocator_get(os_allocator *api, MppBufferType type)
{
    MPP_RET ret = MPP_OK;
    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL :
    case MPP_BUFFER_TYPE_V4L2 : {
        *api = allocator_normal;
    } break;
    case MPP_BUFFER_TYPE_ION : {
        *api = allocator_ion;
    } break;
    default : {
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

