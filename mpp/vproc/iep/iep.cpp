/*
 * Copyright 2018 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "iep"

#include <math.h>
#include <fcntl.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "iep_api.h"
#include "iep.h"

#define X   (-1)

static RK_S32 enh_alpha_table[][25] = {
    //      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24
    /*0*/  {X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X },
    /*1*/  {0,  8, 12, 16, 20, 24, 28,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X },
    /*2*/  {0,  4,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X },
    /*3*/  {0,  X,  X,  8,  X,  X, 12,  X,  X, 16,  X,  X, 20,  X,  X, 24,  X,  X, 28,  X,  X,  X,  X,  X,  X },
    /*4*/  {0,  2,  4,  6,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 },
    /*5*/  {0,  X,  X,  X,  X,  8,  X,  X,  X,  X, 12,  X,  X,  X,  X, 16,  X,  X,  X,  X, 20,  X,  X,  X,  X },
    /*6*/  {0,  X,  X,  4,  X,  X,  8,  X,  X, 10,  X,  X, 12,  X,  X, 14,  X,  X, 16,  X,  X, 18,  X,  X, 20 },
    /*7*/  {0,  X,  X,  X,  X,  X,  X,  8,  X,  X,  X,  X,  X,  X, 12,  X,  X,  X,  X,  X,  X, 16,  X,  X,  X },
    /*8*/  {0,  1,  2,  3,  4,  5,  6,  7,  8,  X,  9,  X, 10,  X, 11,  X, 12,  X, 13,  X, 14,  X,  15, X, 16 }
};

RK_U32 iep_debug = 0;

#define IEP_DBG_FUNCTION            (0x00000001)
#define IEP_DBG_IMAGE               (0x00000010)

#define iep_dbg(flag, fmt, ...)     _mpp_dbg(iep_debug, flag, fmt, ## __VA_ARGS__)
#define iep_dbg_f(flag, fmt, ...)   _mpp_dbg_f(iep_debug, flag, fmt, ## __VA_ARGS__)

#define iep_dbg_func(fmt, ...)      iep_dbg(IEP_DBG_FUNCTION, fmt, ## __VA_ARGS__)

static const char *iep_name = "/dev/iep";

typedef struct IepCtxImpl_t {
    IepMsg      msg;
    RK_S32      fd;
    RK_S32      pid;
    IepHwCap    cap;
} IepCtxImpl;

MPP_RET iep_init(IepCtx *ctx)
{
    if (NULL == ctx) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    RK_S32 fd = -1;
    IepCtxImpl *impl = NULL;

    mpp_env_get_u32("iep_debug", &iep_debug, 0);
    *ctx = NULL;

    do {
        impl = mpp_calloc(IepCtxImpl, 1);
        if (NULL == impl) {
            mpp_err_f("failed to alloc context\n");
            break;
        }

        fd = open(iep_name, O_RDWR);
        if (fd < 0) {
            mpp_err("can NOT find iep device %s\n", iep_name);
            break;
        }

        if (0 > ioctl(fd, IEP_QUERY_CAP, &impl->cap)) {
            iep_dbg_func("Query IEP capability failed, using default cap\n");
            IepHwCap *cap = &impl->cap;

            cap->scaling_supported = 0;
            cap->i4_deinterlace_supported = 1;
            cap->i2_deinterlace_supported = 1;
            cap->compression_noise_reduction_supported = 1;
            cap->sampling_noise_reduction_supported = 1;
            cap->hsb_enhancement_supported = 1;
            cap->cg_enhancement_supported = 1;
            cap->direct_path_supported = 1;
            cap->max_dynamic_width = 1920;
            cap->max_dynamic_height = 1088;
            cap->max_static_width = 8192;
            cap->max_static_height = 8192;
            cap->max_enhance_radius = 3;
        }

        impl->fd = fd;
        impl->pid = getpid();

        *ctx = impl;
        return MPP_OK;
    } while (0);

    if (fd > 0)
        close(fd);

    MPP_FREE(impl);

    return MPP_NOK;
}

MPP_RET iep_deinit(IepCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    IepCtxImpl *impl = (IepCtxImpl *)ctx;

    if (impl->fd > 0) {
        close(impl->fd);
        impl->fd = -1;
    }

    mpp_free(impl);
    return MPP_OK;
}

static void dump_iep_img(IepImg *img)
{
    mpp_log("image    %p\n", img);
    mpp_log("act_w    %d\n", img->act_w);
    mpp_log("act_h    %d\n", img->act_h);
    mpp_log("x_off    %d\n", img->x_off);
    mpp_log("y_off    %d\n", img->y_off);
    mpp_log("vir_w    %d\n", img->vir_w);
    mpp_log("vir_h    %d\n", img->vir_h);
    mpp_log("format   %d\n", img->format);
    mpp_log("mem_addr %08x\n", img->mem_addr);
    mpp_log("uv_addr  %08x\n", img->uv_addr);
    mpp_log("v_addr   %08x\n", img->v_addr);
}

static RK_U32 check_yuv_enhance_param(IepCmdParamYuvEnhance *yuv_enh)
{
    RK_U32 ret = 0;
    if (yuv_enh->saturation < 0 || yuv_enh->saturation > 1.992) {
        mpp_err("Invalidate parameter, yuv_enh_saturation!\n");
        ret |= 0x0001;
    }

    if (yuv_enh->contrast < 0 || yuv_enh->contrast > 1.992) {
        mpp_err("Invalidate parameter, yuv_enh_contrast!\n");
        ret |= 0x0002;
    }

    if (yuv_enh->brightness < -32 || yuv_enh->brightness > 31) {
        mpp_err("Invalidate parameter, yuv_enh_brightness!\n");
        ret |= 0x0004;
    }

    if (yuv_enh->hue_angle < -30 || yuv_enh->hue_angle > 30) {
        mpp_err("Invalidate parameter, yuv_enh_hue_angle!\n");
        ret |= 0x0008;
    }

    if (yuv_enh->video_mode < 0 || yuv_enh->video_mode > 3) {
        mpp_err("Invalidate parameter, video_mode!\n");
        ret |= 0x0010;
    }

    if (yuv_enh->color_bar_y > 127 || yuv_enh->color_bar_u > 127 ||
        yuv_enh->color_bar_v > 127) {
        mpp_err("Invalidate parameter, color_bar_*!\n");
        ret |= 0x0020;
    }
    return ret;
}

static RK_U32 check_rgb_enhance_param(IepCmdParamRgbEnhance *rgb_enh)
{
    RK_U32 ret = 0;
    if (rgb_enh->alpha_base > 8 || rgb_enh->alpha_base < 0 ||
        rgb_enh->alpha_num  < 0 || rgb_enh->alpha_num  > 24) {
        mpp_err("Invalidate parameter, enh_alpha_base or enh_alpha_num!\n");
        ret |= 0x0001;
    }

    if (enh_alpha_table[rgb_enh->alpha_base][rgb_enh->alpha_num] == -1) {
        mpp_err("Invalidate parameter, enh_alpha_base or enh_alpha_num!\n");
        ret |= 0x0002;
    }

    if (rgb_enh->threshold > 255 || rgb_enh->threshold < 0) {
        mpp_err("Invalidate parameter, enh_threshold!\n");
        ret |= 0x0004;
    }

    if (rgb_enh->radius > 4 || rgb_enh->radius < 1) {
        mpp_err("Invalidate parameter, enh_radius!\n");
        ret |= 0x0008;
    }

    if (rgb_enh->order < IEP_RGB_ENHANCE_ORDER_CG_DDE ||
        rgb_enh->order > IEP_RGB_ENHANCE_ORDER_DDE_CG) {
        mpp_err("Invalidate parameter, rgb_contrast_enhance_mode!\n");
        ret |= 0x0010;
    }

    if (rgb_enh->mode < IEP_RGB_ENHANCE_MODE_NO_OPERATION ||
        rgb_enh->mode > IEP_RGB_ENHANCE_MODE_EDGE_ENHANCE) {
        mpp_err("Invalidate parameter, rgb_enhance_mode!\n");
        ret |= 0x0020;
    }

    if (rgb_enh->coe > 3.96875 || rgb_enh->coe < 0) {
        mpp_err("Invalidate parameter, rgb_enh_coe!\n");
        ret |= 0x0040;
    }

    return ret;
}

static RK_U32 check_msg_image(IepMsg *msg)
{
    RK_U32 ret = 0;
    IepMsgImg *src = &msg->src;
    IepMsgImg *dst = &msg->dst;

    if (!((src->format < IEP_FORMAT_RGB_BUTT) ||
          (src->format >= IEP_FORMAT_YUV_BASE && src->format < IEP_FORMAT_YUV_BUTT))) {
        mpp_err("Invalidate parameter, i/o format!\n");
        ret |= 0x0001;
    }

    if (src->act_w > 4096 || src->act_h > 8192) {
        mpp_err("Invalidate parameter, source image size!\n");
        ret |= 0x0002;
    }

    if (dst->act_w > 4096 || dst->act_h > 4096) {
        mpp_err("Invalidate parameter, destination image size!\n");
        ret |= 0x0004;
    }

    RK_S32 scl_fct_h = src->act_w > dst->act_w ? (src->act_w * 1000 / dst->act_w) : (dst->act_w * 1000 / src->act_w);
    RK_S32 scl_fct_v = src->act_h > dst->act_h ? (src->act_h * 1000 / dst->act_h) : (dst->act_h * 1000 / src->act_h);

    if (scl_fct_h > 16000 || scl_fct_v > 16000) {
        mpp_err("Invalidate parameter, scale factor!\n");
        ret |= 0x0008;
    }
    return ret;
}

// rr = 1.7 rg = 1 rb = 0.6
static RK_U32 cg_tab[] = {
    0x01010100, 0x03020202, 0x04030303, 0x05040404,
    0x05050505, 0x06060606, 0x07070606, 0x07070707,
    0x08080807, 0x09080808, 0x09090909, 0x0a090909,
    0x0a0a0a0a, 0x0b0a0a0a, 0x0b0b0b0b, 0x0c0b0b0b,
    0x0c0c0c0c, 0x0c0c0c0c, 0x0d0d0d0d, 0x0d0d0d0d,
    0x0e0e0d0d, 0x0e0e0e0e, 0x0e0e0e0e, 0x0f0f0f0f,
    0x0f0f0f0f, 0x10100f0f, 0x10101010, 0x10101010,
    0x11111110, 0x11111111, 0x11111111, 0x12121212,
    0x12121212, 0x12121212, 0x13131313, 0x13131313,
    0x13131313, 0x14141414, 0x14141414, 0x14141414,
    0x15151515, 0x15151515, 0x15151515, 0x16161615,
    0x16161616, 0x16161616, 0x17161616, 0x17171717,
    0x17171717, 0x17171717, 0x18181818, 0x18181818,
    0x18181818, 0x19191818, 0x19191919, 0x19191919,
    0x19191919, 0x1a1a1a19, 0x1a1a1a1a, 0x1a1a1a1a,
    0x1a1a1a1a, 0x1b1b1b1b, 0x1b1b1b1b, 0x1b1b1b1b,
    0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
    0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
    0x23222120, 0x27262524, 0x2b2a2928, 0x2f2e2d2c,
    0x33323130, 0x37363534, 0x3b3a3938, 0x3f3e3d3c,
    0x43424140, 0x47464544, 0x4b4a4948, 0x4f4e4d4c,
    0x53525150, 0x57565554, 0x5b5a5958, 0x5f5e5d5c,
    0x63626160, 0x67666564, 0x6b6a6968, 0x6f6e6d6c,
    0x73727170, 0x77767574, 0x7b7a7978, 0x7f7e7d7c,
    0x83828180, 0x87868584, 0x8b8a8988, 0x8f8e8d8c,
    0x93929190, 0x97969594, 0x9b9a9998, 0x9f9e9d9c,
    0xa3a2a1a0, 0xa7a6a5a4, 0xabaaa9a8, 0xafaeadac,
    0xb3b2b1b0, 0xb7b6b5b4, 0xbbbab9b8, 0xbfbebdbc,
    0xc3c2c1c0, 0xc7c6c5c4, 0xcbcac9c8, 0xcfcecdcc,
    0xd3d2d1d0, 0xd7d6d5d4, 0xdbdad9d8, 0xdfdedddc,
    0xe3e2e1e0, 0xe7e6e5e4, 0xebeae9e8, 0xefeeedec,
    0xf3f2f1f0, 0xf7f6f5f4, 0xfbfaf9f8, 0xfffefdfc,
    0x06030100, 0x1b150f0a, 0x3a322922, 0x63584e44,
    0x95887b6f, 0xcebfb0a2, 0xfffeedde, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};

static void setup_cg_tab(double cg_rate, RK_U32 *base)
{
    RK_S32 i;

    for (i = 0; i < 64; i++) {
        RK_U32 rgb_0 = 4 * i;
        RK_U32 rgb_1 = 4 * i + 1;
        RK_U32 rgb_2 = 4 * i + 2;
        RK_U32 rgb_3 = 4 * i + 3;

        /// need modify later
        RK_U32 cg_0 = pow(rgb_0, cg_rate);
        RK_U32 cg_1 = pow(rgb_1, cg_rate);
        RK_U32 cg_2 = pow(rgb_2, cg_rate);
        RK_U32 cg_3 = pow(rgb_3, cg_rate);

        if (cg_0 > 255) cg_0 = 255;
        if (cg_1 > 255) cg_1 = 255;
        if (cg_2 > 255) cg_2 = 255;
        if (cg_3 > 255) cg_3 = 255;

        RK_U32 tab_0 = (RK_U32)cg_0;
        RK_U32 tab_1 = (RK_U32)cg_1;
        RK_U32 tab_2 = (RK_U32)cg_2;
        RK_U32 tab_3 = (RK_U32)cg_3;

        RK_U32 conf_value = (tab_3 << 24) + (tab_2 << 16) + (tab_1 << 8) + tab_0;
        base[i] = conf_value;
    }
}

MPP_RET iep_control(IepCtx ctx, IepCmd cmd, void *param)
{
    if (NULL == ctx) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    IepCtxImpl *impl = (IepCtxImpl *)ctx;
    IepMsg *msg = &impl->msg;

    switch (cmd) {
    case IEP_CMD_INIT : {
        memset(msg, 0, sizeof(*msg));
    } break;
    case IEP_CMD_SET_SRC : {
        IepMsgImg *src = &msg->src;

        mpp_assert(param);
        // NOTE: only update which is used
        memcpy(&msg->src, param, sizeof(IepImg));

        if ((src->format == IEP_FORMAT_YCbCr_420_P
             || src->format == IEP_FORMAT_YCbCr_420_SP
             || src->format == IEP_FORMAT_YCbCr_422_P
             || src->format == IEP_FORMAT_YCbCr_422_SP
             || src->format == IEP_FORMAT_YCrCb_420_P
             || src->format == IEP_FORMAT_YCrCb_420_SP
             || src->format == IEP_FORMAT_YCrCb_422_P
             || src->format == IEP_FORMAT_YCrCb_422_SP) &&
            msg->dein_mode == IEP_DEI_MODE_DISABLE) {
            msg->dein_mode = IEP_DEI_MODE_BYPASS;
        }
        if (iep_debug & IEP_DBG_IMAGE) {
            mpp_log("setup src\n");
            dump_iep_img((IepImg *)param);
        }
    } break;
    case IEP_CMD_SET_DST : {
        mpp_assert(param);
        // NOTE: only update which is used
        memcpy(&msg->dst, param, sizeof(IepImg));

        if (iep_debug & IEP_DBG_IMAGE) {
            mpp_log("setup dst\n");
            dump_iep_img((IepImg *)param);
        }
    } break;
    case IEP_CMD_SET_DEI_CFG : {
        IepCmdParamDeiCfg *dei_cfg = (IepCmdParamDeiCfg *)param;

        if (NULL == param) {
            msg->dein_high_fre_en   = 0;
            msg->dein_mode          = IEP_DEI_MODE_I2O1;
            msg->field_order        = IEP_DEI_FLD_ORDER_BOT_FIRST;
            msg->dein_ei_mode       = 0;
            msg->dein_ei_sel        = 0;
            msg->dein_ei_radius     = 0;
            msg->dein_ei_smooth     = 0;
            msg->dein_high_fre_fct  = 0;
        } else {
            msg->dein_high_fre_en   = dei_cfg->dei_high_freq_en;
            msg->dein_mode          = dei_cfg->dei_mode;
            msg->field_order        = dei_cfg->dei_field_order;
            msg->dein_ei_mode       = dei_cfg->dei_ei_mode;
            msg->dein_ei_sel        = dei_cfg->dei_ei_sel;
            msg->dein_ei_radius     = dei_cfg->dei_ei_radius;
            msg->dein_ei_smooth     = dei_cfg->dei_ei_smooth;
            msg->dein_high_fre_fct  = dei_cfg->dei_high_freq_fct;
        }
        switch (msg->dein_mode) {
        case IEP_DEI_MODE_DISABLE : {
            msg->dein_mode = IEP_DEI_MODE_BYPASS;
        } break;
        case IEP_DEI_MODE_I2O1 :
        case IEP_DEI_MODE_I4O1 :
        case IEP_DEI_MODE_I4O2 : {
            // for avoid hardware error we need to config src1 and dst1
            if (!msg->src1.mem_addr)
                memcpy(&msg->src1, &msg->src, sizeof(msg->src));
            if (!msg->dst1.mem_addr)
                memcpy(&msg->dst1, &msg->dst, sizeof(msg->dst));
        } break;
        default : {
        } break;
        }
    } break;
    case IEP_CMD_SET_DEI_SRC1 : {
        mpp_assert(param);
        // NOTE: only update which is used
        memcpy(&msg->src1, param, sizeof(IepImg));
        if (iep_debug & IEP_DBG_IMAGE) {
            mpp_log("setup src1\n");
            dump_iep_img((IepImg *)param);
        }
    } break;
    case IEP_CMD_SET_DEI_DST1 : {
        mpp_assert(param);
        // NOTE: only update which is used
        memcpy(&msg->dst1, param, sizeof(IepImg));
        if (iep_debug & IEP_DBG_IMAGE) {
            mpp_log("setup dst1\n");
            dump_iep_img((IepImg *)param);
        }
    } break;
    case IEP_CMD_SET_YUV_ENHANCE : {
        IepCmdParamYuvEnhance *yuv_enh = (IepCmdParamYuvEnhance *)param;

        if (NULL == yuv_enh) {
            msg->yuv_enhance_en = 1;

            msg->sat_con_int = 0x80;
            msg->contrast_int = 0x80;
            // hue_angle = 0
            msg->cos_hue_int = 0x00;
            msg->sin_hue_int = 0x80;

            msg->yuv_enh_brightness = 0x00;

            msg->video_mode = IEP_VIDEO_MODE_NORMAL_VIDEO;

            msg->color_bar_u = 0;
            msg->color_bar_v = 0;
            msg->color_bar_y = 0;
            break;
        }

        if (check_yuv_enhance_param(yuv_enh)) {
            ret = MPP_NOK;
            break;
        }

        if (msg->src.format >= 1 && msg->src.format <= 5 &&
            msg->dst.format >= 1 && msg->dst.format <= 5) {
            mpp_err("Invalidate parameter, contradition between in/out format and yuv enhance\n");
            ret = MPP_NOK;
            break;
        }

        msg->yuv_enhance_en = 1;

        msg->sat_con_int = (RK_S32)(yuv_enh->saturation * yuv_enh->contrast * 128);
        msg->contrast_int = (RK_S32)(yuv_enh->contrast * 128);
        msg->cos_hue_int = (RK_S32)(cos(yuv_enh->hue_angle) * 128.0);
        msg->sin_hue_int = (RK_S32)(sin(yuv_enh->hue_angle) * 128.0);
        msg->yuv_enh_brightness = yuv_enh->brightness >= 0 ?
                                  yuv_enh->brightness :
                                  (yuv_enh->brightness + 64);

        msg->video_mode  = yuv_enh->video_mode;
        msg->color_bar_y = yuv_enh->color_bar_y;
        msg->color_bar_u = yuv_enh->color_bar_u;
        msg->color_bar_v = yuv_enh->color_bar_v;
    } break;
    case IEP_CMD_SET_RGB_ENHANCE : {
        RK_U32 i;
        IepCmdParamRgbEnhance *rgb_enh = (IepCmdParamRgbEnhance *)param;

        if (NULL == rgb_enh) {
            msg->rgb_color_enhance_en       = 1;
            msg->rgb_enh_coe                = 32;
            msg->rgb_contrast_enhance_mode  = IEP_RGB_ENHANCE_MODE_DETAIL_ENHANCE;
            msg->rgb_cg_en                  = 0;
            msg->enh_threshold              = 255;
            msg->enh_alpha                  = 8;
            msg->enh_radius                 = 3;

            for (i = 0; i < MPP_ARRAY_ELEMS(cg_tab); i++)
                msg->cg_tab[i] = cg_tab[i];

            break;
        }

        if (check_rgb_enhance_param(rgb_enh)) {
            ret = MPP_NOK;
            break;
        }

        if ((msg->src.format & IEP_FORMAT_YUV_BASE) &&
            (msg->dst.format & IEP_FORMAT_YUV_BASE)) {
            mpp_err("Invalidate parameter, contradition between in/out format and yuv enhance\n");
            ret = MPP_NOK;
            break;
        }

        msg->rgb_color_enhance_en       = 1;
        msg->rgb_enh_coe                = (RK_U32)(rgb_enh->coe * 32);
        msg->rgb_contrast_enhance_mode  = rgb_enh->order;
        msg->rgb_cg_en                  = rgb_enh->cg_en;
        msg->rgb_enhance_mode           = rgb_enh->mode;

        msg->enh_threshold = rgb_enh->threshold;
        msg->enh_alpha     =
            enh_alpha_table[rgb_enh->alpha_base][rgb_enh->alpha_num];
        msg->enh_radius    = rgb_enh->radius - 1;

        if (rgb_enh->cg_en) {
            setup_cg_tab(rgb_enh->cg_rb, msg->cg_tab);
            setup_cg_tab(rgb_enh->cg_rg, msg->cg_tab + 64);
            setup_cg_tab(rgb_enh->cg_rb, msg->cg_tab + 128);
        }
    } break;
    case IEP_CMD_SET_SCALE : {
        IepCmdParamScale *scale = (IepCmdParamScale *)param;

        msg->scale_up_mode = (scale) ? (scale->scale_alg) : (IEP_SCALE_ALG_CATROM);
    } break;
    case IEP_CMD_SET_COLOR_CONVERT : {
        IepCmdParamColorConvert *color_cvt = (IepCmdParamColorConvert *)param;

        if (NULL == color_cvt) {
            msg->rgb2yuv_mode       = IEP_COLOR_MODE_BT601_L;
            msg->yuv2rgb_mode       = IEP_COLOR_MODE_BT601_L;
            msg->rgb2yuv_clip_en    = 0;
            msg->yuv2rgb_clip_en    = 0;
            msg->global_alpha_value = 0;
            msg->dither_up_en       = 1;
            msg->dither_down_en     = 1;
            break;
        }

        if (color_cvt->dither_up_en && msg->src.format != IEP_FORMAT_RGB_565) {
            mpp_err("Invalidate parameter, contradiction between dither up enable and source image format!\n");
            ret = MPP_NOK;
            break;
        }

        if (color_cvt->dither_down_en && msg->dst.format != IEP_FORMAT_RGB_565) {
            mpp_err("Invalidate parameter, contradiction between dither down enable and destination image format!\n");
            ret = MPP_NOK;
            break;
        }

        msg->rgb2yuv_mode       = color_cvt->rgb2yuv_mode;
        msg->yuv2rgb_mode       = color_cvt->yuv2rgb_mode;
        msg->rgb2yuv_clip_en    = color_cvt->rgb2yuv_input_clip;
        msg->yuv2rgb_clip_en    = color_cvt->yuv2rgb_input_clip;
        msg->global_alpha_value = color_cvt->global_alpha_value;
        msg->dither_up_en       = color_cvt->dither_up_en;
        msg->dither_down_en     = color_cvt->dither_down_en;
    } break;
    case IEP_CMD_RUN_SYNC : {
        check_msg_image(msg);

        int ops_ret = ioctl(impl->fd, IEP_SET_PARAMETER, msg);
        if (ops_ret < 0)
            mpp_err("pid %d ioctl IEP_SET_PARAMETER failure\n", impl->pid);

        // NOTE: force result sync to avoid iep task queue full
        ops_ret = ioctl(impl->fd, IEP_GET_RESULT_SYNC, 0);
        if (ops_ret)
            mpp_err("pid %d get result failure\n", impl->pid);

        ret = (MPP_RET)ops_ret;
    } break;
    case IEP_CMD_QUERY_CAP : {
        if (param)
            *(IepHwCap **)param = &impl->cap;
        else
            mpp_err("Can NOT query to NULL output\n");
    } break;
    default : {
        mpp_err("invalid command %d\n", cmd);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}
