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

#include "mpp_env.h"
#include "mpp_log.h"

#include "vpu.h"

#define VPU_IOC_MAGIC                       'l'

#define VPU_IOC_SET_CLIENT_TYPE             _IOW(VPU_IOC_MAGIC, 1, unsigned long)
#define VPU_IOC_GET_HW_FUSE_STATUS          _IOW(VPU_IOC_MAGIC, 2, unsigned long)
#define VPU_IOC_SET_REG                     _IOW(VPU_IOC_MAGIC, 3, unsigned long)
#define VPU_IOC_GET_REG                     _IOW(VPU_IOC_MAGIC, 4, unsigned long)
#define VPU_IOC_PROBE_IOMMU_STATUS          _IOR(VPU_IOC_MAGIC, 5, unsigned long)
#define VPU_IOC_WRITE(nr, size)             _IOC(_IOC_WRITE, VPU_IOC_MAGIC, (nr), (size))

typedef struct VPUReq {
    RK_U32 *req;
    RK_U32  size;
} VPUReq_t;

static RK_U32 vpu_debug = 0;
static RK_S32 vpu_service_status = -1;
#define VPU_SERVICE_TEST    \
    do { \
        if (vpu_service_status < 0) { \
            vpu_service_status = (access("/dev/vpu_service", F_OK) == 0); \
        } \
    } while (0)

static const char *name_rkvdec = "/dev/rkvdec";
static const char *name_rkvenc = "/dev/rkvenc";
static const char *name_hevc_service = "/dev/hevc_service";
static const char *name_vpu_service = "/dev/vpu_service";


int VPUClientInit(VPU_CLIENT_TYPE type)
{
    VPU_SERVICE_TEST;
    int ret;
    int fd;
    const char *name = NULL;

    switch (type) {
    case VPU_DEC_RKV: {
        name = name_rkvdec;
        type = VPU_DEC;
        break;
    }
    case VPU_DEC_HEVC: {
        name = name_hevc_service;
        type = VPU_DEC;
        break;
    }
    case VPU_DEC_PP:
    case VPU_PP:
    case VPU_DEC:
    case VPU_ENC: {
        name = name_vpu_service;
        break;
    }
    case VPU_ENC_RKV: {
        name = name_rkvenc;
        break;
    }
    default: {
        return -1;
        break;
    }
    }

    fd = open(name, O_RDWR);

    mpp_env_get_u32("vpu_debug", &vpu_debug, 0);

    if (fd == -1) {
        mpp_err_f("failed to open %s\n", name);
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

    if (vpu_debug) {
        RK_U32 i;

        for (i = 0; i < nregs; i++) {
            mpp_log("set reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(fd, VPU_IOC_SET_REG, &req);
    if (ret)
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n", ret, errno, strerror(errno));

    return ret;
}

RK_S32 VPUClientSendReg2(RK_S32 socket, RK_S32 offset, RK_S32 size, void *param)
{
    RK_S32 ret = 0;

    if (param == NULL) {
        mpp_err_f("input param is NULL");
        return 1;
    }

    ret = (RK_S32)ioctl(socket, VPU_IOC_WRITE(offset, size), param);
    if (ret)
        mpp_err_f("ioctl VPU_IOC_WRITE failed ret %d", ret);

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

    if (vpu_debug) {
        RK_U32 i;
        nregs >>= 2;

        for (i = 0; i < nregs; i++) {
            mpp_log("get reg[%03d]: %08x\n", i, regs[i]);
        }
    }

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
    return 1;
}

