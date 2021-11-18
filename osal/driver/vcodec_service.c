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
#include "mpp_soc.h"
#include "mpp_platform.h"
#include "vcodec_service.h"
#include "vcodec_service_api.h"

#define MAX_REGS_COUNT      3
#define MPX_EXTRA_INFO_NUM  16
#define MAX_INFO_COUNT      16
#define INFO_FORMAT_TYPE    3

typedef struct MppReq_t {
    RK_U32 *req;
    RK_U32  size;
} MppReq;

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
    RK_U64              fmt;
    RK_S32              fd;
    RK_S32              max_regs;
    RK_U32              reg_size;

    RK_S32              reg_send_idx;
    RK_S32              reg_poll_idx;

    VcodecRegCfg        regs[MAX_REGS_COUNT];

    RK_S32              info_count;
    MppDevInfoCfg       info[MAX_INFO_COUNT];
} MppDevVcodecService;

/* For vpu1 / vpu2 */
static const char *mpp_vpu_dev[] = {
    "/dev/vpu_service",
    "/dev/vpu-service",
    "/dev/mpp_service",
};

/* For hevc 4K decoder */
static const char *mpp_hevc_dev[] = {
    "/dev/hevc_service",
    "/dev/hevc-service",
    "/dev/mpp_service",
};

/* For H.264/H.265/VP9 4K decoder */
static const char *mpp_rkvdec_dev[] = {
    "/dev/rkvdec",
    "/dev/mpp_service",
};

/* For H.264 4K encoder */
static const char *mpp_rkvenc_dev[] = {
    "/dev/rkvenc",
    "/dev/mpp_service",
};

/* For avs+ decoder */
static const char *mpp_avsd_dev[] = {
    "/dev/avsd",
    "/dev/mpp_service",
};

/* For H.264 / jpeg encoder */
static const char *mpp_vepu_dev[] = {
    "/dev/vepu",
    "/dev/mpp_service",
};

/* For H.265 encoder */
static const char *mpp_h265e_dev[] = {
    "/dev/h265e",
    "/dev/mpp_service",
};

/* For jpeg decoder */
static const char *mpp_jpegd_dev[] = {
    "/dev/mpp_service",
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

const char *mpp_get_platform_dev_name(MppCtxType type, MppCodingType coding, RK_U32 platform)
{
    const char *dev = NULL;

    if ((platform & HAVE_RKVDEC) && (type == MPP_CTX_DEC) &&
        (coding == MPP_VIDEO_CodingAVC ||
         coding == MPP_VIDEO_CodingHEVC ||
         coding == MPP_VIDEO_CodingAVS2 ||
         coding == MPP_VIDEO_CodingVP9)) {
        dev = mpp_find_device(mpp_rkvdec_dev);
    } else if ((platform & HAVE_HEVC_DEC) && (type == MPP_CTX_DEC) &&
               (coding == MPP_VIDEO_CodingHEVC)) {
        dev = mpp_find_device(mpp_hevc_dev);
    } else if ((platform & HAVE_AVSDEC) && (type == MPP_CTX_DEC) &&
               (coding == MPP_VIDEO_CodingAVS)) {
        dev = mpp_find_device(mpp_avsd_dev);
    } else if ((platform & HAVE_RKVENC) && (type == MPP_CTX_ENC) &&
               (coding == MPP_VIDEO_CodingAVC)) {
        dev = mpp_find_device(mpp_rkvenc_dev);
    } else if ((platform & HAVE_VEPU22) && (type == MPP_CTX_ENC) &&
               (coding == MPP_VIDEO_CodingHEVC)) {
        dev = mpp_find_device(mpp_h265e_dev);
    } else {
        if (type == MPP_CTX_ENC)
            dev = mpp_find_device(mpp_vepu_dev);

        if (dev == NULL)
            dev = mpp_find_device(mpp_vpu_dev);
    }

    return dev;
}

const char *mpp_get_vcodec_dev_name(MppCtxType type, MppCodingType coding)
{
    const char *dev = NULL;
    RockchipSocType soc_type = mpp_get_soc_type();

    switch (soc_type) {
    case ROCKCHIP_SOC_RK3036 : {
        /* rk3036 do NOT have encoder */
        if (type == MPP_CTX_ENC)
            dev = NULL;
        else if (coding == MPP_VIDEO_CodingHEVC && type == MPP_CTX_DEC)
            dev = mpp_find_device(mpp_hevc_dev);
        else
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3066 :
    case ROCKCHIP_SOC_RK3188 : {
        /* rk3066/rk3188 have vpu1 only */
        dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3288 :
    case ROCKCHIP_SOC_RK312X :
    case ROCKCHIP_SOC_RK3368 :
    case ROCKCHIP_SOC_RK3326 :
    case ROCKCHIP_SOC_PX30 : {
        /*
         * rk3288/rk312x/rk3368 have codec:
         * 1 - vpu1
         * 2 - RK hevc decoder
         */
        if (coding == MPP_VIDEO_CodingHEVC && type == MPP_CTX_DEC)
            dev = mpp_find_device(mpp_hevc_dev);
        else
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3128H : {
        /*
         * rk3128H have codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265 1080p@60fps decoder
         * NOTE: rk3128H do NOT have jpeg encoder
         */
        if (type == MPP_CTX_DEC &&
            (coding == MPP_VIDEO_CodingAVC ||
             coding == MPP_VIDEO_CodingHEVC))
            dev = mpp_find_device(mpp_rkvdec_dev);
        else if (type == MPP_CTX_ENC && coding == MPP_VIDEO_CodingMJPEG)
            dev = NULL;
        else if (type == MPP_CTX_DEC && coding == MPP_VIDEO_CodingVP9)
            dev = NULL;
        else
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3399 :
    case ROCKCHIP_SOC_RK3229 : {
        /*
         * rk3399/rk3229 have codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265/VP9 4K decoder
         */
        if (type == MPP_CTX_DEC &&
            (coding == MPP_VIDEO_CodingAVC ||
             coding == MPP_VIDEO_CodingHEVC ||
             coding == MPP_VIDEO_CodingVP9))
            dev = mpp_find_device(mpp_rkvdec_dev);
        else
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3228 : {
        /*
         * rk3228 have codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265 4K decoder
         * NOTE: rk3228 do NOT have jpeg encoder
         */
        if (type == MPP_CTX_DEC &&
            (coding == MPP_VIDEO_CodingAVC ||
             coding == MPP_VIDEO_CodingHEVC))
            dev = mpp_find_device(mpp_rkvdec_dev);
        else if (type == MPP_CTX_ENC && coding == MPP_VIDEO_CodingMJPEG)
            dev = NULL;
        else
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3228H : {
        /*
         * rk3228h has codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265 4K decoder
         * 3 - avs+ decoder
         * 4 - H.265 encoder
         */
        if (type == MPP_CTX_ENC) {
            if (coding == MPP_VIDEO_CodingHEVC)
                dev = mpp_find_device(mpp_h265e_dev);
            else
                dev = mpp_find_device(mpp_vepu_dev);
        } else if (type == MPP_CTX_DEC) {
            if (coding == MPP_VIDEO_CodingAVS)
                dev = mpp_find_device(mpp_avsd_dev);
            else if (coding == MPP_VIDEO_CodingAVC ||
                     coding == MPP_VIDEO_CodingHEVC)
                dev = mpp_find_device(mpp_rkvdec_dev);
            else
                dev = mpp_find_device(mpp_vpu_dev);
        }
    } break;
    case ROCKCHIP_SOC_RK3328 : {
        /*
         * rk3228 has codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 4 - H.265 encoder
         */
        if (type == MPP_CTX_ENC) {
            if (coding == MPP_VIDEO_CodingHEVC)
                dev = mpp_find_device(mpp_h265e_dev);
            else
                dev = mpp_find_device(mpp_vepu_dev);
        } else if (type == MPP_CTX_DEC) {
            if (coding == MPP_VIDEO_CodingAVC ||
                coding == MPP_VIDEO_CodingHEVC ||
                coding == MPP_VIDEO_CodingVP9) {
                dev = mpp_find_device(mpp_rkvdec_dev);
            } else
                dev = mpp_find_device(mpp_vpu_dev);
        }
    } break;
    case ROCKCHIP_SOC_RV1108 : {
        /*
         * rv1108 has codec:
         * 1 - vpu2
         * 2 - RK H.264 4K decoder
         * 3 - RK H.264 4K encoder
         */
        if (coding == MPP_VIDEO_CodingAVC) {
            if (type == MPP_CTX_ENC)
                dev = mpp_find_device(mpp_rkvenc_dev);
            else
                dev = mpp_find_device(mpp_rkvdec_dev);
        } else if (coding == MPP_VIDEO_CodingMJPEG)
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RV1109 :
    case ROCKCHIP_SOC_RV1126 : {
        /*
         * rv1108 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264/H.265 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         */
        if (coding == MPP_VIDEO_CodingAVC || coding == MPP_VIDEO_CodingHEVC) {
            if (type == MPP_CTX_ENC)
                dev = mpp_find_device(mpp_rkvenc_dev);
            else
                dev = mpp_find_device(mpp_rkvdec_dev);
        } else if (coding == MPP_VIDEO_CodingMJPEG)
            dev = mpp_find_device(mpp_vpu_dev);
    } break;
    case ROCKCHIP_SOC_RK3566 :
    case ROCKCHIP_SOC_RK3568 : {
        /*
         * rk3566/rk3568 has codec:
         * 1 - vpu2 for jpeg/vp8 encoder and decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         * 3 - RK jpeg decoder
         */
        if (type == MPP_CTX_DEC) {
            if (coding == MPP_VIDEO_CodingAVC ||
                coding == MPP_VIDEO_CodingHEVC ||
                coding == MPP_VIDEO_CodingVP9)
                dev = mpp_find_device(mpp_rkvdec_dev);
            else if (coding == MPP_VIDEO_CodingMJPEG)
                dev = mpp_find_device(mpp_jpegd_dev);
            else
                dev = mpp_find_device(mpp_vpu_dev);
        } else if (type == MPP_CTX_ENC) {
            if (coding == MPP_VIDEO_CodingAVC ||
                coding == MPP_VIDEO_CodingHEVC)
                dev = mpp_find_device(mpp_rkvenc_dev);
            else if (coding == MPP_VIDEO_CodingMJPEG ||
                     coding == MPP_VIDEO_CodingVP8)
                dev = mpp_find_device(mpp_vpu_dev);
            else
                dev = NULL;
        }
    } break;
    case ROCKCHIP_SOC_RK3588 : {
        /*
         * rk3588 has codec:
         * 1 - RK H.264/H.265/VP9/AVS2 8K decoder
         * 2 - RK H.264/H.265 8K encoder
         * 3 - vpu2 for jpeg/vp8 encoder and decoder
         * 4 - RK jpeg decoder
         */
        if (type == MPP_CTX_DEC) {
            if (coding == MPP_VIDEO_CodingAVC ||
                coding == MPP_VIDEO_CodingHEVC ||
                coding == MPP_VIDEO_CodingAVS2 ||
                coding == MPP_VIDEO_CodingVP9)
                dev = mpp_find_device(mpp_rkvdec_dev);
            else if (coding == MPP_VIDEO_CodingMJPEG)
                dev = mpp_find_device(mpp_jpegd_dev);
            else
                dev = mpp_find_device(mpp_vpu_dev);
        } else if (type == MPP_CTX_ENC) {
            if (coding == MPP_VIDEO_CodingAVC ||
                coding == MPP_VIDEO_CodingHEVC)
                dev = mpp_find_device(mpp_rkvenc_dev);
            else if (coding == MPP_VIDEO_CodingMJPEG ||
                     coding == MPP_VIDEO_CodingVP8)
                dev = mpp_find_device(mpp_vpu_dev);
            else
                dev = NULL;
        }
    } break;
    default : {
        /* default case for unknown compatible  */
        RK_U32 vcodec_type = mpp_get_vcodec_type();

        dev = mpp_get_platform_dev_name(type, coding, vcodec_type);
    } break;
    }

    return dev;
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

static void update_extra_info(VcodecExtraInfo *info, char* fmt, VcodecRegCfg* send_cfg)
{
    void *reg_set = send_cfg->reg_set;
    RK_U32 reg_size = send_cfg->reg_size;

    if (info->count) {
        if (!strstr(fmt, "mjpeg")) {
            RK_U32 *reg = (RK_U32*)reg_set;
            RK_U32 i = 0;

            for (i = 0; i < info->count; i++) {
                VcodecExtraSlot *slot = &info->slots[i];

                reg[slot->reg_idx] |= (slot->offset << 10);
            }
            info->count = 0;
        } else {
            void *extra_data = reg_set + reg_size;
            RK_S32 extra_size = sizeof(send_cfg->extra_info);

            memcpy(extra_data, info, extra_size);
            send_cfg->reg_size += extra_size;
            extra_info_init(info);
        }
    }
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
        name = mpp_find_device(mpp_vepu_dev);
        if (NULL == name)
            name = mpp_find_device(mpp_vpu_dev);
        client_type = VPU_ENC;
        reg_size = VEPU2_REGISTERS;
    } break;
    default : {
        mpp_err_f("unsupported client type %d\n", type);
        return ret;
    } break;
    }

    p->fd = open(name, O_RDWR | O_CLOEXEC);
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
        VcodecExtraSlot *slot;
        RK_U32 i;

        for (i = 0; i < extra->count; i++) {
            slot = &extra->slots[i];

            if (slot->reg_idx == cfg->reg_idx) {
                mpp_err_f("reg[%d] offset has been set, cover old %d -> %d\n",
                          slot->reg_idx, slot->offset, cfg->offset);
                slot->offset = cfg->offset;
                return MPP_OK;
            }
        }

        slot = &extra->slots[extra->count];
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
    char *fmt = (char*)&p->fmt;

    update_extra_info(extra, fmt, send_cfg);

    MPP_RET ret = vcodec_service_ioctl(p->fd, VPU_IOC_SET_REG,
                                       reg_set, send_cfg->reg_size);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    p->reg_send_idx++;
    if (p->reg_send_idx >= p->max_regs)
        p->reg_send_idx = 0;
    p->info_count = 0;

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

MPP_RET vcodec_service_set_info(void *ctx, MppDevInfoCfg *cfg)
{
    MppDevVcodecService *p = (MppDevVcodecService *)ctx;

    if (!p->info_count)
        memset(p->info, 0, sizeof(p->info));

    if (p->info_count >= MAX_INFO_COUNT) {
        mpp_err("info count reach max\n");
        return MPP_NOK;
    }

    memcpy(&p->info[p->info_count], cfg, sizeof(MppDevInfoCfg));
    p->info_count++;

    if (cfg->type == INFO_FORMAT_TYPE) {
        p->fmt = cfg->data;
    }
    return MPP_OK;
}

const MppDevApi vcodec_service_api = {
    "vcodec_service",
    sizeof(MppDevVcodecService),
    vcodec_service_init,
    vcodec_service_deinit,
    NULL,
    NULL,
    NULL,
    vcodec_service_reg_wr,
    vcodec_service_reg_rd,
    vcodec_service_fd_trans,
    NULL,
    vcodec_service_set_info,
    vcodec_service_cmd_send,
    vcodec_service_cmd_poll,
};
