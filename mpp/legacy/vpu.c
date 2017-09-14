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

#ifdef RKPLATFORM
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <video/rk_vpu_service.h>

#include <rk_mpi.h>
#include <mpp_env.h>
#include <mpp_log.h>
#include <mpp_common.h>
#include <mpp_platform.h>

#include "vpu.h"

#define VPU_IOC_WRITE(nr, size)    _IOC(_IOC_WRITE, VPU_IOC_MAGIC, (nr), (size))

static RK_U32 vpu_debug = 0;

int VPUClientInit(VPU_CLIENT_TYPE type)
{
    int ret;
    int fd;
    const char *path;
    MppCtxType ctx_type;
    MppCodingType coding = MPP_VIDEO_CodingAutoDetect;

    switch (type) {
    case VPU_DEC_HEVC:
        coding = MPP_VIDEO_CodingHEVC;
        ctx_type  = MPP_CTX_DEC;
        type = VPU_DEC;
        break;
    case VPU_DEC_AVS:
        coding = MPP_VIDEO_CodingAVS;
        ctx_type  = MPP_CTX_DEC;
        type = VPU_DEC;
        break;
    case VPU_DEC_RKV:
        type = VPU_DEC;
    case VPU_DEC:
    case VPU_DEC_PP:
    case VPU_PP:
        ctx_type  = MPP_CTX_DEC;
        break;
    case VPU_ENC:
    case VPU_ENC_RKV:
        ctx_type = MPP_CTX_ENC;
        break;
    default:
        return -1;
        break;
    }

    path = mpp_get_vcodec_dev_name (ctx_type, coding);
    fd = open(path, O_RDWR);

    mpp_env_get_u32("vpu_debug", &vpu_debug, 0);

    if (fd == -1) {
        mpp_err_f("failed to open %s, errno = %d, error msg: %s\n",
                  path, errno, strerror(errno));
        return -1;
    }

    ret = ioctl(fd, VPU_IOC_SET_CLIENT_TYPE, type);
    if (ret) {
        return -2;
    }
    return fd;
}

RK_S32 VPUClientRelease(int socket)
{
    int fd = socket;
    if (fd > 0) {
        close(fd);
    }
    return VPU_SUCCESS;
}

RK_S32 VPUClientSendReg(int socket, RK_U32 *regs, RK_U32 nregs)
{
    int fd = socket;
    RK_S32 ret;
    struct vpu_request req;

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
    int fd = socket;
    RK_S32 ret;
    struct vpu_request req;
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
    int fd = socket;
    RK_S32 ret;
    struct vpu_request req;
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
#else
int VPUClientInit(VPU_CLIENT_TYPE type)
{
    (void)type;
    return 0;
}

RK_S32 VPUClientRelease(int socket)
{
    (void)socket;
    return 0;
}

RK_S32 VPUClientSendReg(int socket, RK_U32 *regs, RK_U32 nregs)
{
    (void)socket;
    (void)regs;
    (void)nregs;
    return 0;
}

RK_S32 VPUClientSendReg2(RK_S32 socket, RK_S32 offset, RK_S32 size, void *param)
{
    (void)socket;
    (void)offset;
    (void)size;
    (void)param;
    return 0;
}

RK_S32 VPUClientWaitResult(int socket, RK_U32 *regs, RK_U32 nregs, VPU_CMD_TYPE *cmd, RK_S32 *len)
{
    (void)socket;
    (void)regs;
    (void)nregs;
    (void)cmd;
    (void)len;
    return 0;
}

RK_S32 VPUClientGetHwCfg(int socket, RK_U32 *cfg, RK_U32 cfg_size)
{
    (void)socket;
    (void)cfg;
    (void)cfg_size;
    return 0;
}

RK_U32 VPUCheckSupportWidth()
{
    return 0;
}

RK_S32 VPUClientGetIOMMUStatus()
{
    return 1;
}

#endif

