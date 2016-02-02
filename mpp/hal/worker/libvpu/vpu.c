/*
 *
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

#define MODULE_TAG "vpu"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mpp_log.h"

#include "vpu.h"

#define VPU_IOC_MAGIC                       'l'
#define VPU_IOC_SET_CLIENT_TYPE             _IOW(VPU_IOC_MAGIC, 1, unsigned long)
#define VPU_IOC_GET_HW_FUSE_STATUS          _IOW(VPU_IOC_MAGIC, 2, unsigned long)
#define VPU_IOC_SET_REG                     _IOW(VPU_IOC_MAGIC, 3, unsigned long)
#define VPU_IOC_GET_REG                     _IOW(VPU_IOC_MAGIC, 4, unsigned long)
#define VPU_IOC_PROBE_IOMMU_STATUS          _IOR(VPU_IOC_MAGIC, 5, unsigned long)

typedef struct VPUReq {
    RK_U32 *req;
    RK_U32  size;
} VPUReq_t;

static int vpu_service_status = -1;
static int vpu_service_iommu_status = -1;
#define VPU_SERVICE_TEST    \
    do { \
        if (vpu_service_status < 0) { \
            vpu_service_status = (access("/dev/vpu_service", F_OK) == 0); \
        } \
    } while (0)

int VPUClientInit(VPU_CLIENT_TYPE type)
{
    VPU_SERVICE_TEST;
    int ret;
    int fd;

    if (type == VPU_DEC_RKV) {
        fd = open("/dev/rkvdec", O_RDWR);
        type = VPU_DEC;
    } else if (type == VPU_DEC_HEVC) {
        fd = open("/dev/hevc_service", O_RDWR);
        type = VPU_DEC;
    } else {
        fd = open("/dev/vpu_service", O_RDWR);
    }

    if (fd == -1) {
        mpp_err_f("failed to open /dev/rkvdec\n");
        return -1;
    }
    ret = ioctl(fd, VPU_IOC_SET_CLIENT_TYPE, (RK_U32)type);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_SET_CLIENT_TYPE failed ret %d errno %d\n", ret, errno);
        return -2;
    }
    return fd;
}

RK_S32 VPUClientRelease(int socket)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    if (fd > 0) {
        close(fd);
    }
    return VPU_SUCCESS;
}

RK_S32 VPUClientSendReg(int socket, RK_U32 *regs, RK_U32 nregs)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    RK_S32 ret;
    VPUReq_t req;
    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(fd, VPU_IOC_SET_REG, &req);
    if (ret)
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n", ret, errno, strerror(errno));

    return ret;
}

RK_S32 VPUClientWaitResult(int socket, RK_U32 *regs, RK_U32 nregs, VPU_CMD_TYPE *cmd, RK_S32 *len)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    RK_S32 ret;
    VPUReq_t req;
    (void)len;
    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(fd, VPU_IOC_GET_REG, &req);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_GET_REG failed ret %d errno %d %s\n", ret, errno, strerror(errno));
        *cmd = VPU_SEND_CONFIG_ACK_FAIL;
    } else
        *cmd = VPU_SEND_CONFIG_ACK_OK;

    return ret;
}

RK_S32 VPUClientGetHwCfg(int socket, RK_U32 *cfg, RK_U32 cfg_size)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    RK_S32 ret;
    VPUReq_t req;
    req.req     = cfg;
    req.size    = cfg_size;
    ret = (RK_S32)ioctl(fd, VPU_IOC_GET_HW_FUSE_STATUS, &req);
    if (ret)
        mpp_err_f("ioctl VPU_IOC_GET_HW_FUSE_STATUS failed ret %d\n", ret);

    return ret;
}

RK_U32 VPUCheckSupportWidth()
{
    VPUHwDecConfig_t hwCfg;
    int fd = -1;
    fd = open("/dev/vpu_service", O_RDWR);
    memset(&hwCfg, 0, sizeof(VPUHwDecConfig_t));
    if (fd >= 0) {
        if (VPUClientGetHwCfg(fd, (RK_U32*)&hwCfg, sizeof(hwCfg))) {
            mpp_err_f("Get HwCfg failed\n");
            close(fd);
            return -1;
        }
        close(fd);
        fd = -1;
    }
    return hwCfg.maxDecPicWidth;
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
 * directory search function:
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
    if (n < 0) {
        mpp_err("scan %s for %s failed\n", path, dir_name);
    } else {
            while (n > 1) {
                free(dir[--n]);
            }

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
    char path[256];

    snprintf(path, sizeof(path), "/proc/device-tree");
    if (find_dir_in_path(path, "vpu_service", sizeof(path))) {
        if (find_dir_in_path(path, "iommu_enabled", sizeof(path))) {
            FILE *iommu_fp = fopen(path, "rb");

            if (iommu_fp) {
                RK_U32 iommu_enabled = 0;
                fread(&iommu_enabled, sizeof(RK_U32), 1, iommu_fp);
                mpp_log("vpu_service iommu_enabled %d\n", (iommu_enabled > 0));
                fclose(iommu_fp);
                return (iommu_enabled) ? (1) : (0);
            }
        } else {
            mpp_err("can not find dts for iommu_enabled\n");
        }
    } else {
        mpp_err("can not find dts for vpu_service\n");
    }

    return -1;
}

RK_S32 VPUClientGetIOMMUStatus()
{
    int ret = 0;
    if (vpu_service_iommu_status < 0) {
        vpu_service_iommu_status = check_sysfs_iommu();
        if (vpu_service_iommu_status < 0) {
            int fd = -1;
            fd = open("/dev/vpu_service", O_RDWR);
            if (fd >= 0) {
                ret = (RK_S32)ioctl(fd, VPU_IOC_PROBE_IOMMU_STATUS, &vpu_service_iommu_status);
                if (ret) {
                    vpu_service_iommu_status = 0;
                    mpp_err_f("VPUClient: ioctl VPU_IOC_PROBE_IOMMU_STATUS failed ret %d, disable iommu\n", ret);
                }
                close(fd);
            } else {
                vpu_service_iommu_status = 0;
            }
        }
        mpp_log("vpu_service_iommu_status %d", vpu_service_iommu_status);
    }

    return vpu_service_iommu_status;
}

