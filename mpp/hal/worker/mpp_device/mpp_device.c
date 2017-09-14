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

#define MODULE_TAG "mpp_device"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <video/rk_vpu_service.h>

#include "mpp_env.h"
#include "mpp_log.h"

#include "mpp_device.h"
#include "mpp_platform.h"

#include "vpu.h"

#define VPU_IOC_WRITE(nr, size)    _IOC(_IOC_WRITE, VPU_IOC_MAGIC, (nr), (size))

static RK_U32 mpp_device_debug = 0;

static RK_S32 mpp_device_get_client_type(MppDevCtx *ctx, MppCtxType coding, MppCodingType type)
{
    RK_S32 client_type = -1;

    if (coding == MPP_CTX_ENC)
        client_type = VPU_ENC;
    else { /* MPP_CTX_DEC */
        client_type = VPU_DEC;
        if (ctx->pp_enable)
            client_type = VPU_DEC_PP;
    }
    (void)ctx;
    (void)type;
    (void)coding;

    return client_type;
}

RK_S32 mpp_device_init(MppDevCtx *ctx, MppCtxType coding, MppCodingType type)
{
    RK_S32 dev = -1;
    const char *name = NULL;

    ctx->coding = coding;
    ctx->type = type;
    if (ctx->platform)
        name = mpp_get_platform_dev_name(coding, type, ctx->platform);
    else
        name = mpp_get_vcodec_dev_name(coding, type);
    if (name) {
        dev = open(name, O_RDWR);
        if (dev > 0) {
            RK_S32 ret = 0;
            RK_S32 client_type = mpp_device_get_client_type(ctx, coding, type);

            ctx->client_type = client_type;
            ret = ioctl(dev, VPU_IOC_SET_CLIENT_TYPE, client_type);
            if (ret) {
                close(dev);
                dev = -2;
            }
            ctx->client_type = client_type;
        } else
            mpp_err_f("failed to open device %s, errno %d, error msg: %s\n",
                      name, errno, strerror(errno));
    } else
        mpp_err_f("failed to find device for coding %d type %d\n", coding, type);

    return dev;
}

MPP_RET mpp_device_deinit(RK_S32 dev)
{
    if (dev > 0)
        close(dev);

    return MPP_OK;
}

MPP_RET mpp_device_send_reg(RK_S32 dev, RK_U32 *regs, RK_U32 nregs)
{
    MPP_RET ret;
    struct vpu_request req;

    if (mpp_device_debug) {
        RK_U32 i;

        for (i = 0; i < nregs; i++) {
            mpp_log("set reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(dev, VPU_IOC_SET_REG, &req);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

MPP_RET mpp_device_wait_reg(RK_S32 dev, RK_U32 *regs, RK_U32 nregs)
{
    MPP_RET ret;
    struct vpu_request req;

    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;

    ret = (RK_S32)ioctl(dev, VPU_IOC_GET_REG, &req);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_GET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    if (mpp_device_debug) {
        RK_U32 i;
        nregs >>= 2;

        for (i = 0; i < nregs; i++) {
            mpp_log("get reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    return ret;
}

MPP_RET mpp_device_send_reg_with_id(RK_S32 dev, RK_S32 id, void *param,
                                    RK_S32 size)
{
    MPP_RET ret = MPP_NOK;

    if (param == NULL) {
        mpp_err_f("input param is NULL");
        return ret;
    }

    ret = (RK_S32)ioctl(dev, VPU_IOC_WRITE(id, size), param);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_WRITE failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

RK_S32 mpp_device_control(MppDevCtx *ctx, MppDevCmd cmd, void* param)
{
    switch (cmd) {
    case MPP_DEV_GET_MMU_STATUS : {
        ctx->mmu_status = 1;
        *((RK_U32 *)param) = ctx->mmu_status;
    } break;
    case MPP_DEV_ENABLE_POSTPROCCESS : {
        ctx->pp_enable = 1;
    } break;
    case MPP_DEV_SET_HARD_PLATFORM : {
        ctx->platform = *((RK_U32 *)param);
    } break;
    default : {
    } break;
    }

    return 0;
}
