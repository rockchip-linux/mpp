/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "vcodec_service"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_list.h"
#include "mpp_common.h"

#include "vpu.h"
#include "vcodec_service.h"
#include "vcodec_service_api.h"

#define MAX_REGS_COUNT      3
#define MPX_EXTRA_INFO_NUM  16

typedef struct VcodecExtraSlot_t {
    RK_U32              reg_idx;
    RK_U32              offset;
} VcodecExtraSlot;

typedef struct VcodecExtraInfo_t {
    RK_U32              magic;      // Fix magic value 0x4C4A46
    RK_U32              count;      // valid patch info count
    VcodecExtraSlot     slots[MPX_EXTRA_INFO_NUM];
} VcodecExtraInfo;

typedef struct VcodecRegCfg_t {
    RK_U32              reg_size;
    VcodecExtraInfo     extra_info;

    void                *reg_set;
    void                *reg_get;
} VcodecRegCfg;

typedef struct MppDevVcodecService_t {
    RK_S32              client_type;
    RK_S32              fd;
    RK_S32              max_regs;
    RK_U32              reg_size;

    RK_S32              reg_send_idx;
    RK_S32              reg_poll_idx;

    VcodecRegCfg        regs[MAX_REGS_COUNT];
} MppDevVcodecService;

/* For vpu1 / vpu2 */
static const char *mpp_vpu_dev[] = {
    "/dev/vpu_service",
    "/dev/vpu-service",
};

/* For hevc 4K decoder */
static const char *mpp_hevc_dev[] = {
    "/dev/hevc_service",
    "/dev/hevc-service",
};

/* For H.264/H.265/VP9 4K decoder */
static const char *mpp_rkvdec_dev[] = {
    "/dev/rkvdec",
};

/* For H.264 4K encoder */
static const char *mpp_rkvenc_dev[] = {
    "/dev/rkvenc",
};

/* For avs+ decoder */
static const char *mpp_avsd_dev[] = {
    "/dev/avsd",
};

/* For H.264 / jpeg encoder */
static const char *mpp_vepu_dev[] = {
    "/dev/vepu",
};

#define mpp_find_device(dev) _mpp_find_device(dev, MPP_ARRAY_ELEMS(dev))

static const char *_mpp_find_device(const char **dev, RK_U32 size)
{
    RK_U32 i;

    for (i = 0; i < size; i++)
        if (!access(dev[i], F_OK))
            return dev[i];

    return NULL;
}

static RK_S32 vcodec_service_ioctl(RK_S32 fd, RK_S32 cmd, void *regs, RK_S32 size)
{
    MppReq req;

    req.req = regs;
    req.size = size;

    return (RK_S32)ioctl(fd, cmd, &req);
}

static void extra_info_init(VcodecExtraInfo *info)
{
    info->magic = EXTRA_INFO_MAGIC;
    info->count = 0;
}

MPP_RET vcodec_service_init(void *ctx, MppClientType type)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;
    static RK_S32 vcodec_ioctl_version = -1;
    VPU_CLIENT_TYPE client_type;
    const char *name = NULL;
    RK_U32 reg_size = 0;
    RK_S32 max_regs = 2;
    MPP_RET ret = MPP_NOK;

    switch (type) {
    case VPU_CLIENT_VDPU1 : {
        name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_DEC;
        reg_size = VDPU1_REGISTERS;
    } break;
    case VPU_CLIENT_VDPU2 : {
        name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_DEC;
        reg_size = VDPU2_REGISTERS;
    } break;
    case VPU_CLIENT_VDPU1_PP : {
        name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_DEC_PP;
        reg_size = VDPU1_PP_REGISTERS;
    } break;
    case VPU_CLIENT_VDPU2_PP : {
        name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_DEC_PP;
        reg_size = VDPU2_PP_REGISTERS;
    } break;
    case VPU_CLIENT_HEVC_DEC : {
        name = mpp_find_device(mpp_hevc_dev);
        client_type = VPU_DEC;
        reg_size = RKHEVC_REGISTERS;
        max_regs = 3;
    } break;
    case VPU_CLIENT_RKVDEC : {
        name = mpp_find_device(mpp_rkvdec_dev);
        client_type = VPU_DEC;
        reg_size = RKVDEC_REGISTERS;
        max_regs = 3;
    } break;
    case VPU_CLIENT_AVSPLUS_DEC : {
        name = mpp_find_device(mpp_avsd_dev);
        client_type = VPU_DEC;
        reg_size = AVSD_REGISTERS;
    } break;
    case VPU_CLIENT_RKVENC : {
        name = mpp_find_device(mpp_rkvenc_dev);
        client_type = VPU_ENC;
        reg_size = AVSD_REGISTERS;
    } break;
    case VPU_CLIENT_VEPU1 : {
        name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_ENC;
        reg_size = VEPU1_REGISTERS;
    } break;
    case VPU_CLIENT_VEPU2 : {
        name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_ENC;
        reg_size = VEPU2_REGISTERS;
    } break;
    case VPU_CLIENT_VEPU2_LITE : {
        name = mpp_find_device(mpp_vepu_dev);
        if (name == NULL)
            name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_ENC;
        reg_size = VEPU2_REGISTERS;
    } break;
    default : {
        mpp_err_f("unsupported client type %d\n", type);
        return ret;
    } break;
    }

    p->fd = open(name, O_RDWR);
    if (p->fd < 0) {
        mpp_err("open vcodec_service %s failed\n", name);
        return ret;
    }

    if (vcodec_ioctl_version < 0) {
        ret = (RK_S32)ioctl(p->fd, VPU_IOC_SET_CLIENT_TYPE, (unsigned long)client_type);
        if (!ret) {
            vcodec_ioctl_version = 0;
        } else {
            ret = (RK_S32)ioctl(p->fd, VPU_IOC_SET_CLIENT_TYPE_U32, (RK_U32)client_type);
            if (!ret)
                vcodec_ioctl_version = 1;
        }
        mpp_assert(ret == MPP_OK);
    } else {
        RK_U32 cmd = (vcodec_ioctl_version == 0) ?
                     (VPU_IOC_SET_CLIENT_TYPE) :
                     (VPU_IOC_SET_CLIENT_TYPE_U32);

        ret = (RK_S32)ioctl(p->fd, cmd, client_type);
    }

    p->max_regs = max_regs;
    p->reg_size = reg_size * sizeof(RK_U32);
    {
        RK_S32 i;

        for (i = 0; i < max_regs; i++) {
            VcodecRegCfg *reg = &p->regs[i];

            reg->reg_size = p->reg_size;
            extra_info_init(&reg->extra_info);
        }
    }


    return ret;
}

MPP_RET vcodec_service_deinit(void *ctx)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;

    if (p->fd)
        close(p->fd);

    return MPP_OK;
}

MPP_RET vcodec_service_reg_wr(void *ctx, MppDevRegWrCfg *cfg)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;
    VcodecRegCfg *send_cfg = &p->regs[p->reg_send_idx];

    mpp_assert(cfg->offset == 0);
    send_cfg->reg_set = cfg->reg;
    send_cfg->reg_size = cfg->size;

    if (p->reg_size != cfg->size)
        mpp_err_f("reg size mismatch wr %d rd %d\n",
                  p->reg_size, cfg->size);

    return MPP_OK;
}

MPP_RET vcodec_service_reg_rd(void *ctx, MppDevRegRdCfg *cfg)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;
    VcodecRegCfg *send_cfg = &p->regs[p->reg_send_idx];

    mpp_assert(cfg->offset == 0);
    send_cfg->reg_get = cfg->reg;
    if (send_cfg->reg_size != cfg->size)
        mpp_err_f("reg size mismatch rd %d rd %d\n",
                  send_cfg->reg_size, cfg->size);

    return MPP_OK;
}

MPP_RET vcodec_service_fd_trans(void *ctx, MppDevRegOffsetCfg *cfg)
{
    if (cfg->offset) {
        MppDevVcodecService *p = (MppDevVcodecService *)ctx;
        VcodecRegCfg *send_cfg = &p->regs[p->reg_send_idx];
        VcodecExtraInfo *extra = &send_cfg->extra_info;
        VcodecExtraSlot *slot = &extra->slots[extra->count];

        slot->reg_idx = cfg->reg_idx;
        slot->offset = cfg->offset;
        extra->count++;
    }

    return MPP_OK;
}

MPP_RET vcodec_service_cmd_send(void *ctx)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;
    VcodecRegCfg *send_cfg = &p->regs[p->reg_send_idx];
    VcodecExtraInfo *extra = &send_cfg->extra_info;
    void *reg_set = send_cfg->reg_set;
    RK_U32 reg_size = send_cfg->reg_size;

    if (extra->count) {
        void *extra_data = reg_set + reg_size;
        RK_S32 extra_size = sizeof(send_cfg->extra_info);

        memcpy(extra_data, extra, extra_size);
        reg_size += extra_size;

        extra_info_init(extra);
    }

    MPP_RET ret = vcodec_service_ioctl(p->fd, VPU_IOC_SET_REG,
                                       reg_set, reg_size);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    p->reg_send_idx++;
    if (p->reg_send_idx >= p->max_regs)
        p->reg_send_idx = 0;

    return ret;
}

MPP_RET vcodec_service_cmd_poll(void *ctx)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;
    VcodecRegCfg *poll_cfg = &p->regs[p->reg_poll_idx];
    void *reg_get = poll_cfg->reg_get;
    RK_S32 reg_size = poll_cfg->reg_size;
    MPP_RET ret = vcodec_service_ioctl(p->fd, VPU_IOC_GET_REG,
                                       reg_get, reg_size);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_GET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    p->reg_poll_idx++;
    if (p->reg_poll_idx >= p->max_regs)
        p->reg_poll_idx = 0;

    return ret;
}

const MppDevApi vcodec_service_api = {
    "vcodec_service",
    sizeof(MppDevVcodecService),
    vcodec_service_init,
    vcodec_service_deinit,
    vcodec_service_reg_wr,
    vcodec_service_reg_rd,
    vcodec_service_fd_trans,
    NULL,
    vcodec_service_cmd_send,
    vcodec_service_cmd_poll,
};
