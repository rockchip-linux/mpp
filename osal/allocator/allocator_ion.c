/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_ion"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#if defined(ARMLINUX)
#include <ion.h>
#elif defined(ANDROID)
#include <linux/ion.h>
#endif

#include "os_mem.h"
#include "allocator_ion.h"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

static RK_U32 ion_debug = 0;

#define ION_FUNCTION                (0x00000001)
#define ION_DEVICE                  (0x00000002)
#define ION_CLINET                  (0x00000004)
#define ION_IOCTL                   (0x00000008)

#define ION_DETECT_IOMMU_DISABLE    (0x0)   /* use ION_HEAP_TYPE_DMA */
#define ION_DETECT_IOMMU_ENABLE     (0x1)   /* use ION_HEAP_TYPE_SYSTEM */
#define ION_DETECT_NO_DTS           (0x2)   /* use ION_HEAP_TYPE_CARVEOUT */

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

#include <dirent.h>

static const char *search_name = NULL;

int _compare_name(const struct dirent *dir)
{
    if (search_name && strstr(dir->d_name, search_name))
        return 1;

    return 0;
}

/*
 * directory search function:
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
    RK_S32 ret = ION_DETECT_IOMMU_DISABLE;
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
                        ret = ION_DETECT_IOMMU_ENABLE;
                }
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        mpp_err("can not find dts for all possible devices\n");
        ret = ION_DETECT_NO_DTS;
    }

    return ret;
}

typedef struct {
    RK_U32  alignment;
    RK_S32  ion_device;
} allocator_ctx_ion;

#define VPU_IOC_MAGIC                       'l'
#define VPU_IOC_PROBE_IOMMU_STATUS          _IOR(VPU_IOC_MAGIC, 5, unsigned long)
#define VPU_IOC_PROBE_HEAP_STATUS           _IOR(VPU_IOC_MAGIC, 6, unsigned long)
const char *dev_ion = "/dev/ion";
static RK_S32 ion_heap_id = -1;
static RK_U32 ion_heap_mask = ION_HEAP_SYSTEM_MASK;

MPP_RET os_allocator_ion_open(void **ctx, size_t alignment)
{
    RK_S32 fd;
    allocator_ctx_ion *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    fd = open(dev_ion, O_RDWR);
    if (fd < 0) {
        mpp_err("open %s failed!\n", dev_ion);
        return MPP_ERR_UNKNOW;
    }

    ion_dbg(ION_DEVICE, "open ion dev fd %d\n", fd);

    p = mpp_malloc(allocator_ctx_ion, 1);
    if (NULL == p) {
        close(fd);
        mpp_err("os_allocator_open Android failed to allocate context\n");
        return MPP_ERR_MALLOC;
    } else {
        /*
         * do heap id detection here:
         * if there is no vpu_service use default ION_HEAP_TYPE_SYSTEM_CONTIG
         * if there is vpu_service then check the iommu_enable status
         */
        if (ion_heap_id < 0) {
            int detect_result = check_sysfs_iommu();
            const char *heap_name = NULL;

            switch (detect_result) {
            case ION_DETECT_IOMMU_DISABLE : {
                ion_heap_mask   = (1 << ION_HEAP_TYPE_DMA);
                ion_heap_id     = ION_HEAP_TYPE_DMA;
                heap_name = "ION_HEAP_TYPE_DMA";
            } break;
            case ION_DETECT_IOMMU_ENABLE : {
                ion_heap_mask   = (1 << ION_HEAP_TYPE_SYSTEM);
                ion_heap_id     = ION_HEAP_TYPE_SYSTEM;
                heap_name = "ION_HEAP_TYPE_SYSTEM";
            } break;
            case ION_DETECT_NO_DTS : {
                ion_heap_mask   = (1 << ION_HEAP_TYPE_CARVEOUT);
                ion_heap_id     = ION_HEAP_TYPE_CARVEOUT;
                heap_name = "ION_HEAP_TYPE_CARVEOUT";
            } break;
            default : {
                mpp_err("invalid detect result %d\n", detect_result);
                ion_heap_mask   = (1 << ION_HEAP_TYPE_DMA);
                ion_heap_id     = ION_HEAP_TYPE_DMA;
                heap_name = "ION_HEAP_TYPE_DMA";
            } break;
            }
            mpp_log("using ion heap %s\n", heap_name);
        }
        p->alignment    = alignment;
        p->ion_device   = fd;
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
                    ion_heap_mask, 0,
                    (ion_user_handle_t *)&info->hnd);
    if (ret) {
        mpp_err("os_allocator_ion_alloc ion_alloc failed ret %d\n", ret);
        return ret;
    }
    ret = ion_map(p->ion_device, (ion_user_handle_t)((intptr_t)info->hnd), info->size,
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
    ion_free(p->ion_device, (ion_user_handle_t)((intptr_t)data->hnd));
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

os_allocator allocator_ion = {
    os_allocator_ion_open,
    os_allocator_ion_alloc,
    os_allocator_ion_free,
    os_allocator_ion_import,
    os_allocator_ion_release,
    os_allocator_ion_close,
};

