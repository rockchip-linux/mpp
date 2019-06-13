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

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp_device.h"
#include "mpp_device_patch.h"
#include "mpp_platform.h"

#include "vpu.h"

#define VPU_IOC_MAGIC                       'l'

#define VPU_IOC_SET_CLIENT_TYPE             _IOW(VPU_IOC_MAGIC, 1, unsigned long)
#define VPU_IOC_SET_REG                     _IOW(VPU_IOC_MAGIC, 3, unsigned long)
#define VPU_IOC_GET_REG                     _IOW(VPU_IOC_MAGIC, 4, unsigned long)

#define VPU_IOC_SET_CLIENT_TYPE_U32         _IOW(VPU_IOC_MAGIC, 1, unsigned int)

#define VPU_IOC_WRITE(nr, size)             _IOC(_IOC_WRITE, VPU_IOC_MAGIC, (nr), (size))

typedef struct MppReq_t {
    RK_U32 *req;
    RK_U32  size;
} MppReq;

#define MAX_TIME_RECORD                     4

typedef struct MppDevCtxImpl_t {
    MppCtxType type;
    MppCodingType coding;
    RK_S32 client_type;
    RK_U32 platform;    // platfrom for vcodec to init
    RK_U32 mmu_status;  // 0 disable, 1 enable
    RK_U32 pp_enable;   // postprocess, 0 disable, 1 enable
    RK_S32 vpu_fd;

    RK_S64 time_start[MAX_TIME_RECORD];
    RK_S64 time_end[MAX_TIME_RECORD];
    RK_S32 idx_send;
    RK_S32 idx_wait;
    RK_S32 idx_total;
} MppDevCtxImpl;

#define MPP_DEVICE_DBG_REG                  (0x00000001)
#define MPP_DEVICE_DBG_TIME                 (0x00000002)

static RK_U32 mpp_device_debug = 0;

static RK_S32 mpp_device_set_client_type(int dev, RK_S32 client_type)
{
    static RK_S32 mpp_device_ioctl_version = -1;
    RK_S32 ret;

    if (mpp_device_ioctl_version < 0) {
        ret = ioctl(dev, VPU_IOC_SET_CLIENT_TYPE, (unsigned long)client_type);
        if (!ret) {
            mpp_device_ioctl_version = 0;
        } else {
            ret = ioctl(dev, VPU_IOC_SET_CLIENT_TYPE_U32, (RK_U32)client_type);
            if (!ret)
                mpp_device_ioctl_version = 1;
        }

        if (ret)
            mpp_err_f("can not find valid client type ioctl\n");

        mpp_assert(ret == 0);
    } else {
        RK_U32 cmd = (mpp_device_ioctl_version == 0) ?
                     (VPU_IOC_SET_CLIENT_TYPE) :
                     (VPU_IOC_SET_CLIENT_TYPE_U32);

        ret = ioctl(dev, cmd, client_type);
    }

    if (ret)
        mpp_err_f("set client type failed ret %d errno %d\n", ret, errno);

    return ret;
}

static RK_S32 mpp_device_get_client_type(MppDevCtx ctx, MppCtxType type, MppCodingType coding)
{
    RK_S32 client_type = -1;
    MppDevCtxImpl *p;

    if (NULL == ctx || type >= MPP_CTX_BUTT ||
        (coding >= MPP_VIDEO_CodingMax || coding <= MPP_VIDEO_CodingUnused)) {
        mpp_err_f("found NULL input ctx %p coding %d type %d\n", ctx, coding, type);
        return MPP_ERR_NULL_PTR;
    }

    p = (MppDevCtxImpl *)ctx;

    if (type == MPP_CTX_ENC)
        client_type = VPU_ENC;
    else { /* MPP_CTX_DEC */
        client_type = VPU_DEC;
        if (p->pp_enable)
            client_type = VPU_DEC_PP;
    }

    (void) type;

    return client_type;
}

MPP_RET mpp_device_init(MppDevCtx *ctx, MppDevCfg *cfg)
{
    RK_S32 dev = -1;
    const char *name = NULL;
    MppDevCtxImpl *p;

    if (NULL == ctx || NULL == cfg) {
        mpp_err_f("found NULL input ctx %p cfg %p\n", ctx, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    mpp_env_get_u32("mpp_device_debug", &mpp_device_debug, 0);

    p = mpp_calloc(MppDevCtxImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    p->coding = cfg->coding;
    p->type = cfg->type;
    p->platform = cfg->platform;
    p->pp_enable = cfg->pp_enable;

    if (p->platform)
        name = mpp_get_platform_dev_name(p->type, p->coding, p->platform);
    else
        name = mpp_get_vcodec_dev_name(p->type, p->coding);
    if (name) {
        dev = open(name, O_RDWR);
        if (dev > 0) {
            RK_S32 client_type = mpp_device_get_client_type(p, p->type, p->coding);
            RK_S32 ret = mpp_device_set_client_type(dev, client_type);

            if (ret) {
                close(dev);
                dev = -2;
            }
            p->client_type = client_type;
        } else
            mpp_err_f("failed to open device %s, errno %d, error msg: %s\n",
                      name, errno, strerror(errno));
    } else
        mpp_err_f("failed to find device for coding %d type %d\n", p->coding, p->type);

    *ctx = p;
    p->vpu_fd = dev;

    return MPP_OK;
}

MPP_RET mpp_device_deinit(MppDevCtx ctx)
{
    MppDevCtxImpl *p;

    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    p = (MppDevCtxImpl *)ctx;

    if (p->vpu_fd > 0) {
        close(p->vpu_fd);
    } else {
        mpp_err_f("invalid negtive file handle,\n");
    }
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_device_send_reg(MppDevCtx ctx, RK_U32 *regs, RK_U32 nregs)
{
    MPP_RET ret;
    MppReq req;
    MppDevCtxImpl *p;

    if (NULL == ctx || NULL == regs) {
        mpp_err_f("found NULL input ctx %p regs %p\n", ctx, regs);
        return MPP_ERR_NULL_PTR;
    }

    p = (MppDevCtxImpl *)ctx;

    if (mpp_device_debug & MPP_DEVICE_DBG_REG) {
        RK_U32 i;

        for (i = 0; i < nregs; i++) {
            mpp_log("set reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;

    if (mpp_device_debug & MPP_DEVICE_DBG_TIME) {
        p->time_start[p->idx_send++] = mpp_time();
        if (p->idx_send >= MAX_TIME_RECORD)
            p->idx_send = 0;
    }

    ret = (RK_S32)ioctl(p->vpu_fd, VPU_IOC_SET_REG, &req);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

MPP_RET mpp_device_wait_reg(MppDevCtx ctx, RK_U32 *regs, RK_U32 nregs)
{
    MPP_RET ret;
    MppReq req;
    MppDevCtxImpl *p;

    if (NULL == ctx || NULL == regs) {
        mpp_err_f("found NULL input ctx %p regs %p\n", ctx, regs);
        return MPP_ERR_NULL_PTR;
    }

    p = (MppDevCtxImpl *)ctx;

    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(p->vpu_fd, VPU_IOC_GET_REG, &req);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_GET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    if (mpp_device_debug & MPP_DEVICE_DBG_TIME) {
        RK_S32 idx = p->idx_wait;
        p->time_end[idx] = mpp_time();

        mpp_log("task %d time %.2f ms\n", p->idx_total,
                (p->time_end[idx] - p->time_start[idx]) / 1000.0);

        p->idx_total++;
        if (++p->idx_wait >= MAX_TIME_RECORD)
            p->idx_wait = 0;
    }


    if (mpp_device_debug & MPP_DEVICE_DBG_REG) {
        RK_U32 i;
        nregs >>= 2;

        for (i = 0; i < nregs; i++) {
            mpp_log("get reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    return ret;
}

MPP_RET mpp_device_send_reg_with_id(MppDevCtx ctx, RK_S32 id, void *param,
                                    RK_S32 size)
{
    MPP_RET ret = MPP_NOK;
    MppDevCtxImpl *p;

    if (NULL == ctx || NULL == param) {
        mpp_err_f("found NULL input ctx %p param %p\n", ctx, param);
        return MPP_ERR_NULL_PTR;
    }

    p = (MppDevCtxImpl *)ctx;

    ret = (RK_S32)ioctl(p->vpu_fd, VPU_IOC_WRITE(id, size), param);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_WRITE failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

RK_S32 mpp_device_control(MppDevCtx ctx, MppDevCmd cmd, void* param)
{
    MppDevCtxImpl *p;

    if (NULL == ctx || NULL == param) {
        mpp_err_f("found NULL input ctx %p param %p\n", ctx, param);
        return MPP_ERR_NULL_PTR;
    }

    p = (MppDevCtxImpl *)ctx;

    switch (cmd) {
    case MPP_DEV_GET_MMU_STATUS : {
        p->mmu_status = 1;
        *((RK_U32 *)param) = p->mmu_status;
    } break;
    case MPP_DEV_ENABLE_POSTPROCCESS : {
        p->pp_enable = 1;
    } break;
    case MPP_DEV_SET_HARD_PLATFORM : {
        p->platform = *((RK_U32 *)param);
    } break;
    default : {
    } break;
    }

    return 0;
}

void mpp_device_patch_init(RegExtraInfo *extra)
{
    extra->magic = EXTRA_INFO_MAGIC;
    extra->count = 0;
}

void mpp_device_patch_add(RK_U32 *reg, RegExtraInfo *extra, RK_U32 reg_idx,
                          RK_U32 offset)
{
    if (offset < SZ_4M) {
        reg[reg_idx] += (offset << 10);
    } else {
        RegPatchInfo *info = &extra->patchs[extra->count++];
        info->reg_idx = reg_idx;
        info->offset = offset;
    }
}
