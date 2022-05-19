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

#include "vpu.h"
#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_platform.h"

#include "mpp_service.h"
#include "vcodec_service.h"

#define VPU_EXTRA_INFO_SIZE                 12
#define VPU_EXTRA_INFO_MAGIC                (0x4C4A46)
#define VPU_MPP_FLAGS_MULTI_MSG             (0x00000001)
#define VPU_MPP_FLAGS_LAST_MSG              (0x00000002)

#define MPX_PATCH_NUM       16

typedef struct VpuPatchInfo_t {
    RK_U32          reg_idx;
    RK_U32          offset;
} VpuPatchInfo;

typedef struct VpuExtraInfo_t {
    RK_U32          magic;      // Fix magic value 0x4C4A46
    RK_U32          count;      // valid patch info count
    VpuPatchInfo    patchs[MPX_PATCH_NUM];
} VpuExtraInfo;

typedef struct VPUReq {
    RK_U32 *req;
    RK_U32  size;
} VPUReq_t;

static RK_U32 vpu_debug = 0;

/* 0 original version,  > 1 for others version */
static RK_S32 ioctl_version = 0;

static RK_S32 vpu_api_set_client_type(int dev, RK_S32 client_type)
{
    static RK_S32 vpu_api_ioctl_version = -1;
    RK_S32 ret;

    if (ioctl_version > 0) {
        MppReqV1 mpp_req;
        RK_U32 vcodec_type;
        RK_U32 client_data;

        vcodec_type = mpp_get_vcodec_type();

        switch (client_type) {
        case VPU_ENC:
            if (vcodec_type & HAVE_VDPU1)
                client_data = VPU_CLIENT_VEPU1;
            else if (vcodec_type & HAVE_VDPU2)
                client_data = VPU_CLIENT_VEPU2;
            break;
        default:
            break;
        }

        mpp_req.cmd = MPP_CMD_INIT_CLIENT_TYPE;
        mpp_req.flag = 0;
        mpp_req.size = sizeof(client_data);
        mpp_req.offset = 0;
        mpp_req.data_ptr = REQ_DATA_PTR(&client_data);
        ret = (RK_S32)ioctl(dev, MPP_IOC_CFG_V1, &mpp_req);
    } else {
        if (vpu_api_ioctl_version < 0) {
            ret = ioctl(dev, VPU_IOC_SET_CLIENT_TYPE, client_type);
            if (!ret) {
                vpu_api_ioctl_version = 0;
            } else {
                ret = ioctl(dev, VPU_IOC_SET_CLIENT_TYPE_U32, client_type);
                if (!ret)
                    vpu_api_ioctl_version = 1;
            }

            if (ret)
                mpp_err_f("can not find valid client type ioctl\n");

            mpp_assert(ret == 0);
        } else {
            RK_U32 cmd = (vpu_api_ioctl_version == 0) ?
                         (VPU_IOC_SET_CLIENT_TYPE) :
                         (VPU_IOC_SET_CLIENT_TYPE_U32);

            ret = ioctl(dev, cmd, client_type);
        }
    }

    if (ret)
        mpp_err_f("set client type failed ret %d errno %d\n", ret, errno);

    return ret;
}


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
        ctx_type  = MPP_CTX_DEC;
        break;
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

    path = mpp_get_vcodec_dev_name(ctx_type, coding);
    fd = open(path, O_RDWR | O_CLOEXEC);

    mpp_env_get_u32("vpu_debug", &vpu_debug, 0);

    ioctl_version = mpp_get_ioctl_version();

    if (fd == -1) {
        mpp_err_f("failed to open %s, errno = %d, error msg: %s\n",
                  path, errno, strerror(errno));
        return -1;
    }

    ret = vpu_api_set_client_type(fd, type);
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
    VPUReq_t req;

    if (vpu_debug) {
        RK_U32 i;

        for (i = 0; i < nregs; i++)
            mpp_log("set reg[%03d]: %08x\n", i, regs[i]);
    }

    if (ioctl_version > 0) {
        MppReqV1 reqs[3];
        RK_U32 reg_size = nregs;

        VpuExtraInfo *extra_info = (VpuExtraInfo*)(regs + (nregs - VPU_EXTRA_INFO_SIZE));

        reqs[0].cmd = MPP_CMD_SET_REG_WRITE;
        reqs[0].flag = 0;
        reqs[0].offset = 0;
        reqs[0].size =  reg_size * sizeof(RK_U32);
        reqs[0].data_ptr = REQ_DATA_PTR((void*)regs);
        reqs[0].flag |= VPU_MPP_FLAGS_MULTI_MSG;

        reqs[1].cmd = MPP_CMD_SET_REG_READ;
        reqs[1].flag = 0;
        reqs[1].offset = 0;
        reqs[1].size =  reg_size * sizeof(RK_U32);
        reqs[1].data_ptr = REQ_DATA_PTR((void*)regs);

        if (extra_info && extra_info->magic == VPU_EXTRA_INFO_MAGIC) {
            reg_size = nregs - VPU_EXTRA_INFO_SIZE;
            reqs[2].cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
            reqs[2].flag = 0;
            reqs[2].offset = 0;
            reqs[2].size = extra_info->count * sizeof(extra_info->patchs[0]);
            reqs[2].data_ptr = REQ_DATA_PTR((void *)&extra_info->patchs[0]);

            reqs[0].size =  reg_size * sizeof(RK_U32);
            reqs[1].size =  reg_size * sizeof(RK_U32);
            reqs[1].flag |= VPU_MPP_FLAGS_MULTI_MSG;
            reqs[2].flag |= VPU_MPP_FLAGS_LAST_MSG;
            ret = (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, &reqs);
        } else {
            MppReqV1 reqs_tmp[2];
            reqs[1].flag |= VPU_MPP_FLAGS_LAST_MSG;
            memcpy(reqs_tmp, reqs, sizeof(MppReqV1) * 2);
            ret = (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, &reqs_tmp);
        }

    } else {
        nregs *= sizeof(RK_U32);
        req.req     = regs;
        req.size    = nregs;
        ret = (RK_S32)ioctl(fd, VPU_IOC_SET_REG, &req);
    }

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
    VPUReq_t req;
    (void)len;

    if (ioctl_version > 0) {
        MppReqV1 mpp_req;
        RK_U32 reg_size = nregs;
        VpuExtraInfo *extra_info = (VpuExtraInfo*)(regs + (nregs - VPU_EXTRA_INFO_SIZE));

        if (extra_info && extra_info->magic == VPU_EXTRA_INFO_MAGIC) {
            reg_size -= 2;
        } else {
            reg_size -= VPU_EXTRA_INFO_SIZE;
        }

        mpp_req.cmd = MPP_CMD_POLL_HW_FINISH;
        mpp_req.flag = 0;
        mpp_req.offset = 0;
        mpp_req.size =  reg_size * sizeof(RK_U32);
        mpp_req.data_ptr = REQ_DATA_PTR((void*)regs);
        ret = (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, &mpp_req);
    } else {
        nregs *= sizeof(RK_U32);
        req.req     = regs;
        req.size    = nregs;

        ret = (RK_S32)ioctl(fd, VPU_IOC_GET_REG, &req);
    }

    if (ret) {
        mpp_err_f("ioctl VPU_IOC_GET_REG failed ret %d errno %d %s\n", ret, errno, strerror(errno));
        *cmd = VPU_SEND_CONFIG_ACK_FAIL;
    } else
        *cmd = VPU_SEND_CONFIG_ACK_OK;

    if (vpu_debug) {
        RK_U32 i;

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
    fd = open("/dev/vpu_service", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        fd = open("/dev/vpu-service", O_RDWR | O_CLOEXEC);
    }
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
