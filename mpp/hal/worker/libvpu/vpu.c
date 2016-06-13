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

#define MODULE_TAG "vpu"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

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

    switch (type) {
    case VPU_DEC_RKV: {
        fd = open("/dev/rkvdec", O_RDWR);
        type = VPU_DEC;
        break;
    }
    case VPU_DEC_HEVC: {
        fd = open("/dev/hevc_service", O_RDWR);
        type = VPU_DEC;
        break;
    }
    case VPU_DEC_PP:
    case VPU_PP:
    case VPU_DEC:
    case VPU_ENC: {
        fd = open("/dev/vpu_service", O_RDWR);
        break;
    }
    default: {
        fd = -1;
        break;
    }
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

RK_S32 VPUClientGetIOMMUStatus()
{
    static RK_U32 once = 1;
    if (once) {
        mpp_log("get iommu status always return true\n");
        once = 0;
    }
    return 1;
}

