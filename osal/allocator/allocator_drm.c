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

#define DRM_FUNCTION                (0x00000001)
#define DRM_DEVICE                  (0x00000002)
#define DRM_CLIENT                  (0x00000004)
#define DRM_IOCTL                   (0x00000008)

#define DRM_DETECT_IOMMU_DISABLE    (0x0)   /* use DRM_HEAP_TYPE_DMA */
#define DRM_DETECT_IOMMU_ENABLE     (0x1)   /* use DRM_HEAP_TYPE_SYSTEM */
#define DRM_DETECT_NO_DTS           (0x2)   /* use DRM_HEAP_TYPE_CARVEOUT */

#define drm_dbg(flag, fmt, ...) _mpp_dbg_f(drm_debug, flag, fmt, ## __VA_ARGS__)

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

    if (fd < 0)
        return NULL;

    if (!pagesize_mask)
        pagesize_mask = getpagesize() - 1;

    len = (len + pagesize_mask) & ~pagesize_mask;

    if (offset & 4095) {
        return NULL;
    }

    return mmap64(NULL, len, prot, flags, fd, offset);
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

#include <dirent.h>

static const char *search_name = NULL;

static int _compare_name(const struct dirent *dir)
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
static RK_S32 find_dir_in_path(char *path, const char *dir_name, size_t max_length)
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

static RK_S32 check_sysfs_iommu()
{
    RK_U32 i = 0;
    RK_U32 dts_info_found = 0;
    RK_U32 ion_info_found = 0;
    RK_S32 ret = DRM_DETECT_IOMMU_DISABLE;
    char path[256];
    static char *dts_devices[] = {
        "vpu_service",
        "hevc_service",
        "rkvdec",
        "rkvenc",
    };
    static char *system_heaps[] = {
        "vmalloc",
        "system-heap",
    };

    mpp_env_get_u32("drm_debug", &drm_debug, 0);
#ifdef SOFIA_3GR_LINUX
    return ret;
#endif

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
                dts_info_found = 1;
                break;
            }
        }
    }

    if (!dts_info_found) {
        for (i = 0; i < MPP_ARRAY_ELEMS(system_heaps); i++) {
            snprintf(path, sizeof(path), "/sys/kernel/debug/ion/heaps");
            if (find_dir_in_path(path, system_heaps[i], sizeof(path))) {
                mpp_log("%s found\n", system_heaps[i]);
                ret = DRM_DETECT_IOMMU_ENABLE;
                ion_info_found = 1;
                break;
            }
        }
    }

    if (!dts_info_found && !ion_info_found) {
        mpp_err("can not find any hint from all possible devices\n");
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

MPP_RET os_allocator_drm_open(void **ctx, size_t alignment)
{
    RK_S32 fd;
    allocator_ctx_drm *p;

    drm_dbg(DRM_FUNCTION, "enter");

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
        p->alignment    = alignment;
        p->drm_device   = fd;
        *ctx = p;
    }

    drm_dbg(DRM_FUNCTION, "leave");

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
    drm_dbg(DRM_FUNCTION, "alignment %d size %d", p->alignment, info->size);
    ret = drm_alloc(p->drm_device, info->size, p->alignment,
                    (RK_U32 *)&info->hnd);
    if (ret) {
        mpp_err("os_allocator_drm_alloc drm_alloc failed ret %d\n", ret);
        return ret;
    }
    drm_dbg(DRM_FUNCTION, "handle %d", (RK_U32)((intptr_t)info->hnd));
    ret = drm_map(p->drm_device, (RK_U32)((intptr_t)info->hnd), info->size,
                  PROT_READ | PROT_WRITE, MAP_SHARED, (unsigned char **)&info->ptr, &info->fd);
    if (ret) {
        mpp_err("os_allocator_drm_alloc drm_map failed ret %d\n", ret);
        return ret;
    }
    return ret;
}

MPP_RET os_allocator_drm_import(void *ctx, MppBufferInfo *data)
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

    data->ptr = drm_mmap(p->drm_device, data->size, PROT_READ | PROT_WRITE, MAP_SHARED, dmmd.offset);
    if (data->ptr == MAP_FAILED) {
        mpp_err("mmap failed: %s\n", strerror(errno));
        return -errno;
    }

    drm_dbg(DRM_FUNCTION, "leave");

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
    drm_dbg(DRM_FUNCTION, "close fd %d", p->drm_device);
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
