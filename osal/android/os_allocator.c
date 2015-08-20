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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ion.h>

#include "os_mem.h"
#include "os_allocator.h"

#include "mpp_mem.h"
#include "mpp_log.h"

typedef struct {
    MppBufferType   type;
    RK_S32          ion_device;
    RK_U32          alignment;
} allocator_impl;

static int ion_open()
{
    int fd = open("/dev/ion", O_RDWR);
    if (fd < 0)
        mpp_err("open /dev/ion failed!\n");
    return fd;
}

static int ion_close(int fd)
{
    int ret = close(fd);
    if (ret < 0)
        return -errno;
    return ret;
}

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

static int ion_share(int fd, ion_user_handle_t handle, int *share_fd)
{
    int ret;
    struct ion_fd_data data = {
        .handle = handle,
    };

    if (share_fd == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_SHARE, &data);
    if (ret < 0)
        return ret;
    *share_fd = data.fd;
    if (*share_fd < 0) {
        mpp_err("share ioctl returned negative fd\n");
        return -EINVAL;
    }
    return ret;
}

static int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
                        unsigned int flags, int *handle_fd)
{
    ion_user_handle_t handle;
    int ret;

    ret = ion_alloc(fd, len, align, heap_mask, flags, &handle);
    if (ret < 0)
        return ret;
    ret = ion_share(fd, handle, handle_fd);
    ion_free(fd, handle);
    return ret;
}

MPP_RET os_allocator_open(void **ctx, size_t alignment, MppBufferType type)
{
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(allocator_impl, 1);
    if (NULL == p) {
        *ctx = NULL;
        mpp_err("os_allocator_open Android failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL :
    case MPP_BUFFER_TYPE_ION : {
        p = mpp_malloc(allocator_impl, 1);
        if (NULL == p) {
            *ctx = NULL;
            mpp_err("os_allocator_open Android failed to allocate context\n");
            return MPP_ERR_MALLOC;
        }

        p->alignment = alignment;
        p->type      = type;
        if (MPP_BUFFER_TYPE_ION == type)
            p->ion_device   = ion_open();
    } break;
    default : {
        mpp_err("os_allocator_open Window do not accept type %d\n");
    } break;
    }

    *ctx = p;
    return 0;
}

MPP_RET os_allocator_alloc(void *ctx, MppBufferData *data, size_t size)
{
    MPP_RET ret = MPP_OK;
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_impl *)ctx;

    switch (p->type) {
    case MPP_BUFFER_TYPE_NORMAL : {
        ret = os_malloc(&data->ptr, p->alignment, size);
    } break;
    case MPP_BUFFER_TYPE_ION : {
        ret = ion_alloc_fd(p->ion_device, size, p->alignment, ION_HEAP_TYPE_CARVEOUT, 0, &data->fd);
    } break;
    default : {
        mpp_err("os_allocator_alloc Linux do not accept type %d\n", p->type);
        ret = MPP_ERR_UNKNOW;
    } break;
    }
    return ret;
}

MPP_RET os_allocator_free(void *ctx, MppBufferData *data)
{
    MPP_RET ret = MPP_OK;
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_impl *)ctx;

    switch (p->type) {
    case MPP_BUFFER_TYPE_NORMAL : {
        os_free(data->ptr);
    } break;
    case MPP_BUFFER_TYPE_ION : {
        close(data->fd);
    } break;
    default : {
        mpp_err("os_allocator_alloc Linux do not accept type %d\n", p->type);
        ret = MPP_ERR_UNKNOW;
    } break;
    }

    return ret;
}

MPP_RET os_allocator_close(void *ctx)
{
    MPP_RET ret = MPP_OK;
    allocator_impl *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_impl *)ctx;

    switch (p->type) {
    case MPP_BUFFER_TYPE_NORMAL : {
    } break;
    case MPP_BUFFER_TYPE_ION : {
        ion_close(p->ion_device);
    } break;
    default : {
        mpp_err("os_allocator_alloc Linux do not accept type %d\n", p->type);
        ret = MPP_ERR_UNKNOW;
    } break;
    }

    return ret;
}

