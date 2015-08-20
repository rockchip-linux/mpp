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
    RK_S32 ion_device;
    RK_U32 alignment;
} allocator_ion;

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

int os_allocator_open(void **ctx, size_t alignment)
{
    allocator_ion *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(allocator_ion, 1);
    if (NULL == p) {
        *ctx = NULL;
        mpp_err("os_allocator_open Android failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    p->ion_device   = ion_open();
    p->alignment    = alignment;
    *ctx = p;
    return 0;
}

int os_allocator_alloc(void *ctx, MppBufferData *data, size_t size)
{
    allocator_ion *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ion *)ctx;

    return os_malloc(&data->ptr, p->alignment, size);
}

void os_allocator_free(void *ctx, MppBufferData *data)
{
    allocator_ion *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return ;
    }

    os_free(data->ptr);
}

void os_allocator_close(void *ctx)
{
    allocator_ion *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return ;
    }

    p = (allocator_ion *)ctx;
    ion_close(p->ion_device);
}

