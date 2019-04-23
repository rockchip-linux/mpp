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
#else
#include "ion.h"
#endif

#include "os_mem.h"
#include "allocator_ion.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_thread.h"

static RK_U32 ion_debug = 0;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_mutex_t scandir_lock;

#define ION_FUNCTION                (0x00000001)
#define ION_DEVICE                  (0x00000002)
#define ION_CLINET                  (0x00000004)
#define ION_IOCTL                   (0x00000008)

#define ION_DETECT_IOMMU_DISABLE    (0x0)   /* use ION_HEAP_TYPE_DMA */
#define ION_DETECT_IOMMU_ENABLE     (0x1)   /* use ION_HEAP_TYPE_SYSTEM */
#define ION_DETECT_NO_DTS           (0x2)   /* use ION_HEAP_TYPE_CARVEOUT */

#define ion_dbg(flag, fmt, ...)     _mpp_dbg(ion_debug, flag, fmt, ## __VA_ARGS__)
#define ion_dbg_f(flag, fmt, ...)   _mpp_dbg_f(ion_debug, flag, fmt, ## __VA_ARGS__)
#define ion_dbg_func(fmt, ...)      ion_dbg_f(ION_FUNCTION, fmt, ## __VA_ARGS__)

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
    int ret = -EINVAL;
    struct ion_allocation_data data = {
        .len = len,
        .align = align,
        .heap_id_mask = heap_mask,
        .flags = flags,
    };

    ion_dbg_func("enter: fd %d len %d align %d heap_mask %x flags %x",
                 fd, len, align, heap_mask, flags);

    if (handle) {
        ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
        if (ret >= 0)
            *handle = data.handle;
    }

    ion_dbg_func("leave: ret %d\n", ret);

    return ret;
}

static int ion_free(int fd, ion_user_handle_t handle)
{
    int ret;
    struct ion_handle_data data = {
        .handle = handle,
    };

    ion_dbg_func("enter: fd %d\n", fd);
    ret = ion_ioctl(fd, ION_IOC_FREE, &data);
    ion_dbg_func("leave: ret %d\n", ret);
    return ret;
}

static int ion_map_fd(int fd, ion_user_handle_t handle, int *map_fd)
{
    int ret;
    struct ion_fd_data data = {
        .handle = handle,
    };

    if (map_fd == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_MAP, &data);
    if (ret < 0)
        return ret;

    *map_fd = data.fd;
    if (*map_fd < 0) {
        mpp_err("map ioctl returned negative fd\n");
        return -EINVAL;
    }

    return 0;
}

static int ion_mmap(int fd, size_t length, int prot, int flags, off_t offset,
                    void **ptr)
{
    static unsigned long pagesize_mask = 0;
    if (ptr == NULL)
        return -EINVAL;

    if (!pagesize_mask)
        pagesize_mask = sysconf(_SC_PAGESIZE) - 1;
    offset = offset & (~pagesize_mask);

    *ptr = mmap(NULL, length, prot, flags, fd, offset);
    if (*ptr == MAP_FAILED) {
        mpp_err("mmap failed: %s\n", strerror(errno));
        *ptr = NULL;
        return -errno;
    }
    return 0;
}

#include <dirent.h>

static const char *search_name = NULL;

static int _compare_name(const struct dirent *dir)
{
    if (search_name && strstr(dir->d_name, search_name))
        return 1;

    return 0;
}

static void scandir_lock_init(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&scandir_lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

/*
 * directory search function:
 * search directory with dir_name on path.
 * if found match dir append name on path and return
 *
 * return 0 for failure
 * return positive value for length of new path
 */
static RK_S32 find_dir_in_path(char *path, const char *dir_name,
                               size_t max_length)
{
    struct dirent **dir;
    RK_S32 path_len = strnlen(path, max_length);
    RK_S32 new_path_len = 0;
    RK_S32 n;

    pthread_once(&once, scandir_lock_init);
    pthread_mutex_lock(&scandir_lock);
    search_name = dir_name;
    n = scandir(path, &dir, _compare_name, alphasort);
    if (n <= 0) {
        mpp_log("scan %s for %s return %d\n", path, dir_name, n);
    } else {
        mpp_assert(n == 1);

        new_path_len = path_len;
        new_path_len += snprintf(path + path_len, max_length - path_len - 1,
                                 "/%s", dir[0]->d_name);
        free(dir[0]);
        free(dir);
    }
    search_name = NULL;
    pthread_mutex_unlock(&scandir_lock);
    return new_path_len;
}

static RK_S32 check_sysfs_iommu()
{
    RK_U32 i = 0;
    RK_U32 dts_info_found = 0;
    RK_U32 ion_info_found = 0;
    RK_S32 ret = ION_DETECT_IOMMU_DISABLE;
    char path[256];
    static char *dts_devices[] = {
        "vpu_service",
        "hevc_service",
        "rkvdec",
        "rkvenc",
        "vpu_combo",
    };
    static char *system_heaps[] = {
        "vmalloc",
        "system-heap",
    };

    mpp_env_get_u32("ion_debug", &ion_debug, 0);
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
                    if (fread(&iommu_enabled, sizeof(RK_U32), 1, iommu_fp))
                        mpp_log("%s iommu_enabled %d\n", dts_devices[i],
                                (iommu_enabled > 0));
                    fclose(iommu_fp);
                    if (iommu_enabled)
                        ret = ION_DETECT_IOMMU_ENABLE;
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
                ret = ION_DETECT_IOMMU_ENABLE;
                ion_info_found = 1;
                break;
            }
        }
    }

    if (!dts_info_found && !ion_info_found) {
        mpp_err("can not find any hint from all possible devices\n");
        ret = ION_DETECT_NO_DTS;
    }

    return ret;
}

typedef struct {
    RK_U32  alignment;
    RK_S32  ion_device;
} allocator_ctx_ion;

static const char *dev_ion = "/dev/ion";
static RK_S32 ion_heap_id = -1;
static RK_U32 ion_heap_mask = ION_HEAP_SYSTEM_MASK;
static pthread_mutex_t lock;

static MPP_RET allocator_ion_open(void **ctx, MppAllocatorCfg *cfg)
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
        pthread_mutex_lock(&lock);
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
        pthread_mutex_unlock(&lock);
        p->alignment    = cfg->alignment;
        p->ion_device   = fd;
        *ctx = p;
    }

    return MPP_OK;
}

static MPP_RET allocator_ion_alloc(void *ctx, MppBufferInfo *info)
{
    MPP_RET ret = MPP_OK;
    int fd = -1;
    ion_user_handle_t hnd = -1;
    allocator_ctx_ion *p = NULL;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ion_dbg_func("enter: ctx %p size %d\n", ctx, info->size);

    p = (allocator_ctx_ion *)ctx;
    ret = ion_alloc(p->ion_device, info->size, p->alignment, ion_heap_mask,
                    0, &hnd);
    if (ret)
        mpp_err_f("ion_alloc failed ret %d\n", ret);
    else {
        ret = ion_map_fd(p->ion_device, hnd, &fd);
        if (ret)
            mpp_err_f("ion_map_fd failed ret %d\n", ret);
    }

    info->fd  = fd;
    info->ptr = NULL;
    info->hnd = (void *)(intptr_t)hnd;

    ion_dbg_func("leave: ret %d handle %d fd %d\n", ret, hnd, fd);
    return ret;
}

static MPP_RET allocator_ion_import(void *ctx, MppBufferInfo *data)
{
    MPP_RET ret = MPP_NOK;
    allocator_ctx_ion *p = (allocator_ctx_ion *)ctx;
    struct ion_fd_data fd_data;

    ion_dbg_func("enter: ctx %p dev %d fd %d size %d\n",
                 ctx, p->ion_device, data->fd, data->size);

    fd_data.fd = data->fd;
    ret = ion_ioctl(p->ion_device, ION_IOC_IMPORT, &fd_data);
    if (0 > fd_data.handle) {
        mpp_err_f("fd %d import failed for %s\n", data->fd, strerror(errno));
        goto RET;
    }

    data->hnd = (void *)(intptr_t)fd_data.handle;
    ret = ion_map_fd(p->ion_device, fd_data.handle, &data->fd);
    data->ptr = NULL;
RET:
    ion_dbg_func("leave: ret %d handle %d\n", ret, data->hnd);
    return ret;
}

static MPP_RET allocator_ion_mmap(void *ctx, MppBufferInfo *data)
{
    MPP_RET ret = MPP_OK;

    if (NULL == ctx) {
        mpp_err_f("do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ion_dbg_func("enter: ctx %p fd %d size %d\n", ctx, data->fd, data->size);

    if (NULL == data->ptr)
        ret = ion_mmap(data->fd, data->size, PROT_READ | PROT_WRITE,
                       MAP_SHARED, 0, &data->ptr);

    ion_dbg_func("leave: ret %d ptr %p\n", ret, data->ptr);
    return ret;
}

static MPP_RET allocator_ion_free(void *ctx, MppBufferInfo *data)
{
    allocator_ctx_ion *p = NULL;
    if (NULL == ctx) {
        mpp_err_f("do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ion_dbg_func("enter: ctx %p fd %d ptr %p size %d\n",
                 ctx, data->fd, data->ptr, data->size);

    p = (allocator_ctx_ion *)ctx;
    if (data->ptr) {
        munmap(data->ptr, data->size);
        data->ptr = NULL;
    }

    if (data->fd > 0) {
        close(data->fd);
        data->fd = -1;
    }

    if (data->hnd) {
        ion_free(p->ion_device, (ion_user_handle_t)((intptr_t)data->hnd));
        data->hnd = NULL;
    }

    ion_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET allocator_ion_close(void *ctx)
{
    int ret;
    allocator_ctx_ion *p;

    if (NULL == ctx) {
        mpp_err("os_allocator_close Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ion_dbg_func("enter: ctx\n", ctx);

    p = (allocator_ctx_ion *)ctx;
    ret = close(p->ion_device);
    mpp_free(p);
    if (ret < 0)
        ret = (MPP_RET) - errno;

    ion_dbg_func("leave: ret %d\n", ret);

    return ret;
}

os_allocator allocator_ion = {
    .open = allocator_ion_open,
    .close = allocator_ion_close,
    .alloc = allocator_ion_alloc,
    .free = allocator_ion_free,
    .import = allocator_ion_import,
    .release = allocator_ion_free,
    .mmap = allocator_ion_mmap,
};

