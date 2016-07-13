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

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

static RK_U32 drm_debug = 0;

#define DRM_FUNCTDRM                (0x00000001)
#define DRM_DEVICE                  (0x00000002)
#define DRM_CLINET                  (0x00000004)
#define DRM_IOCTL                   (0x00000008)

#define DRM_DETECT_IOMMU_DISABLE    (0x0)   /* use DRM_HEAP_TYPE_DMA */
#define DRM_DETECT_IOMMU_ENABLE     (0x1)   /* use DRM_HEAP_TYPE_SYSTEM */
#define DRM_DETECT_NO_DTS           (0x2)   /* use DRM_HEAP_TYPE_CARVEOUT */

#define drm_dbg(flag, fmt, ...) _mpp_dbg(drm_debug, flag, fmt, ## __VA_ARGS__)

static int drm_ioctl(int fd, int req, void *arg)
{
    int ret = ioctl(fd, req, arg);
    if (ret < 0) {
        mpp_err("drm_ioctl %x failed with code %d: %s\n", req,
                ret, strerror(errno));
        return -errno;
    }
    return ret;
}

static int drm_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
                     unsigned int flags, RK_U32 *handle)
{
    int ret;
    struct drm_mode_create_dumb dmcb;
    memset(&dmcb, 0, sizeof(struct drm_mode_create_dumb));
    dmcb.bpp = align;
    dmcb.width = ((len + align) & (~align)) / align;
    dmcb.height = 8;
    dmcb.size = (len + align) & (~align);

    if (handle == NULL)
        return -EINVAL;

    ret = drm_ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &dmcb);
    if (ret < 0)
        return ret;
    *handle = dmcb.handle;
    (void)heap_mask;
    (void)flags;
    return ret;
}

static int drm_handle_to_fd(int fd, RK_U32 handle, int *map_fd, RK_U32 flags)
{
    int ret;
    struct drm_prime_handle dph;
    memset(&dph, 0, sizeof(struct drm_prime_handle));
    dph.handle = handle;
    dph.fd = 1;
    dph.flags = 0;

    if (map_fd == NULL)
        return -EINVAL;

    ret = drm_ioctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &dph);
    if (ret < 0) {
        return ret;
    }

    *map_fd = dph.fd;

    if (*map_fd < 0) {
        mpp_err("map ioctl returned negative fd\n");
        return -EINVAL;
    }
    (void)flags;
    return ret;
}

static int drm_free(int fd, RK_U32 handle)
{
    struct drm_mode_destroy_dumb data = {
        .handle = handle,
    };
    return drm_ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &data);
}

static int drm_map(int fd, RK_U32 handle, size_t length, int prot,
                   int flags, off_t offset, unsigned char **ptr, int *map_fd)
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
    if (ret < 0)
        return ret;

    ret = drm_ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &dmmd);
    if (ret < 0)
        return ret;

    *ptr = mmap(NULL, length, prot, flags, *map_fd, dmmd.offset);
    if (*ptr == MAP_FAILED) {
        mpp_err("mmap failed: %s\n", strerror(errno));
        return -errno;
    }
    (void)offset;
    return ret;
}

#include <dirent.h>

static const char *search_name = NULL;

int _compare_name(const struct dirent *dir)
{
    if (search_name && strstr(dir->d_name, search_name))
        return 1;

    return 0;
}

/*
 * directory search functdrm:
 * search directory with dir_name on path.
 * if found match dir append name on path and return
 *
 * return 0 for failure
 * return positive value for length of new path
 */
RK_S32 find_dir_in_path(char *path, const char *dir_name, size_t max_length)
{
    struct dirent **dir;
    RK_S32 path_len = strnlen(path, max_length);
    RK_S32 new_path_len = 0;
    RK_S32 n;

    search_name = dir_name;
    n = scandir(path, &dir, _compare_name, alphasort);
    if (n <= 0) {
        mpp_err("scan %s for %s return %d\n", path, dir_name, n);
    } else {
        mpp_assert(n == 1);

        new_path_len = path_len;
        new_path_len += snprintf(path + path_len, max_length - path_len - 1,
                                 "/%s", dir[0]->d_name);
        free(dir[0]);
        free(dir);
    }
    search_name = NULL;
    return new_path_len;
}

static char *dts_devices[] = {
    "vpu_service",
    "hevc_service",
    "rkvdec",
    "rkvenc",
};

RK_S32 check_sysfs_iommu()
{
    RK_U32 i = 0, found = 0;
    RK_S32 ret = DRM_DETECT_IOMMU_DISABLE;
    char path[256];

    for (i = 0; i < MPP_ARRAY_ELEMS(dts_devices); i++) {
        snprintf(path, sizeof(path), "/proc/device-tree");
        if (find_dir_in_path(path, dts_devices[i], sizeof(path))) {
            if (find_dir_in_path(path, "iommu_enabled", sizeof(path))) {
                FILE *iommu_fp = fopen(path, "rb");

                if (iommu_fp) {
                    RK_U32 iommu_enabled = 0;
                    fread(&iommu_enabled, sizeof(RK_U32), 1, iommu_fp);
                    mpp_log("%s iommu_enabled %d\n", dts_devices[i], (iommu_enabled > 0));
                    fclose(iommu_fp);
                    if (iommu_enabled)
                        ret = DRM_DETECT_IOMMU_ENABLE;
                }
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        mpp_err("can not find dts for all possible devices\n");
        ret = DRM_DETECT_NO_DTS;
    }

    return ret;
}

typedef struct {
    RK_U32  alignment;
    RK_S32  drm_device;
} allocator_ctx_drm;

#define VPU_IOC_MAGIC                       'l'
#define VPU_IOC_PROBE_IOMMU_STATUS          _IOR(VPU_IOC_MAGIC, 5, unsigned long)
#define VPU_IOC_PROBE_HEAP_STATUS           _IOR(VPU_IOC_MAGIC, 6, unsigned long)

enum {
    DRM_HEAP_TYPE_SYSTEM = 0,
    DRM_HEAP_TYPE_CMA,
    DRM_HEAP_NUMS,
};

const char *dev_drm = "/dev/dri/card0";
static RK_S32 drm_heap_id = -1;
static RK_U32 drm_heap_mask = 1;

MPP_RET os_allocator_drm_open(void **ctx, size_t alignment)
{
    RK_S32 fd;
    allocator_ctx_drm *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

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
        drm_heap_mask = (1 << DRM_HEAP_TYPE_CMA);
        drm_heap_id = DRM_HEAP_TYPE_CMA;
        p->alignment    = alignment;
        p->drm_device   = fd;
        *ctx = p;
    }

    return MPP_OK;
}

MPP_RET os_allocator_drm_alloc(void *ctx, MppBufferInfo *info)
{
    MPP_RET ret = MPP_OK;
    allocator_ctx_drm *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;
    ret = drm_alloc(p->drm_device, info->size, p->alignment,
                    drm_heap_mask, 0,
                    (RK_U32 *)&info->hnd);
    if (ret) {
        mpp_err("os_allocator_drm_alloc drm_alloc failed ret %d\n", ret);
        return ret;
    }
    ret = drm_map(p->drm_device, (RK_U32)((intptr_t)info->hnd), info->size,
                  PROT_READ | PROT_WRITE, MAP_SHARED, (off_t)0,
                  (unsigned char**)&info->ptr, &info->fd);
    if (ret) {
        mpp_err("os_allocator_drm_alloc drm_map failed ret %d\n", ret);
        return ret;
    }
    return ret;
}

MPP_RET os_allocator_drm_import(void *ctx, MppBufferInfo *data)
{
    MPP_RET ret = MPP_OK;
    (void)ctx;
    // NOTE: do not use the original buffer fd,
    //       use dup fd to avoid unexpected external fd close
    data->fd = dup(data->fd);
    /* I don't know whether it is correct for drm */
    data->ptr = mmap(NULL, data->size, PROT_READ | PROT_WRITE, MAP_SHARED, data->fd, 0);
    if (data->ptr == MAP_FAILED) {
        mpp_err_f("map error %s\n", strerror(errno));
        ret = MPP_NOK;
        close(data->fd);
        data->fd = -1;
        data->ptr = NULL;
    }
    return ret;
}

MPP_RET os_allocator_drm_release(void *ctx, MppBufferInfo *data)
{
    (void)ctx;
    munmap(data->ptr, data->size);
    close(data->fd);
    return MPP_OK;
}

MPP_RET os_allocator_drm_free(void *ctx, MppBufferInfo *data)
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

MPP_RET os_allocator_drm_close(void *ctx)
{
    int ret;
    allocator_ctx_drm *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (allocator_ctx_drm *)ctx;
    ret = close(p->drm_device);
    mpp_free(p);
    if (ret < 0)
        return (MPP_RET) - errno;
    return MPP_OK;
}

os_allocator allocator_drm = {
    os_allocator_drm_open,
    os_allocator_drm_alloc,
    os_allocator_drm_free,
    os_allocator_drm_import,
    os_allocator_drm_release,
    os_allocator_drm_close,
};

