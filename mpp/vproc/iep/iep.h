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

/* This header is for communication between userspace driver and kernel driver */

#ifndef __IEP_H__
#define __IEP_H__

#include <sys/ioctl.h>
#include "rk_type.h"

/* Capability for current iep version using by userspace to determine iep features */
typedef struct IepHwCap_t {
    RK_U8   scaling_supported;
    RK_U8   i4_deinterlace_supported;
    RK_U8   i2_deinterlace_supported;
    RK_U8   compression_noise_reduction_supported;
    RK_U8   sampling_noise_reduction_supported;
    RK_U8   hsb_enhancement_supported;
    RK_U8   cg_enhancement_supported;
    RK_U8   direct_path_supported;
    RK_U16  max_dynamic_width;
    RK_U16  max_dynamic_height;
    RK_U16  max_static_width;
    RK_U16  max_static_height;
    RK_U8   max_enhance_radius;
} IepHwCap;

#define IEP_IOC_MAGIC 'i'

#define IEP_SET_PARAMETER_REQ           _IOW(IEP_IOC_MAGIC, 1, unsigned long)
#define IEP_SET_PARAMETER_DEINTERLACE   _IOW(IEP_IOC_MAGIC, 2, unsigned long)
#define IEP_SET_PARAMETER_ENHANCE       _IOW(IEP_IOC_MAGIC, 3, unsigned long)
#define IEP_SET_PARAMETER_CONVERT       _IOW(IEP_IOC_MAGIC, 4, unsigned long)
#define IEP_SET_PARAMETER_SCALE         _IOW(IEP_IOC_MAGIC, 5, unsigned long)
#define IEP_GET_RESULT_SYNC             _IOW(IEP_IOC_MAGIC, 6, unsigned long)
#define IEP_GET_RESULT_ASYNC            _IOW(IEP_IOC_MAGIC, 7, unsigned long)
#define IEP_SET_PARAMETER               _IOW(IEP_IOC_MAGIC, 8, unsigned long)
#define IEP_RELEASE_CURRENT_TASK        _IOW(IEP_IOC_MAGIC, 9, unsigned long)
#define IEP_GET_IOMMU_STATE             _IOR(IEP_IOC_MAGIC, 10, unsigned long)
#define IEP_QUERY_CAP                   _IOR(IEP_IOC_MAGIC, 11, IepHwCap)

enum {
    DEI_MODE_BYPASS_DIS         = 0x0,
    DEI_MODE_I4O2               = 0x1,
    DEI_MODE_I4O1B              = 0x2,
    DEI_MODE_I4O1T              = 0x3,
    DEI_MODE_I2O1B              = 0x4,
    DEI_MODE_I2O1T              = 0x5,
    DEI_MODE_BYPASS             = 0x6,
};

// for rgb_enhance_mode
enum {
    RGB_ENHANCE_BYPASS          = 0x0,
    RGB_ENHANCE_DENOISE         = 0x1,
    RGB_ENHANCE_DETAIL          = 0x2,
    RGB_ENHANCE_EDGE            = 0x3,
};

// for rgb_contrast_enhance_mode
enum {
    RGB_CONTRAST_CC_P_DDE       = 0x0,      // cg prior to dde
    RGB_CONTRAST_DDE_P_CC       = 0x1,      // dde prior to cg
};

// for video mode
enum {
    BLACK_SCREEN                = 0x0,
    BLUE_SCREEN                 = 0x1,
    COLOR_BAR                   = 0x2,
    NORMAL_MODE                 = 0x3,
};

/*
//          Alpha    Red     Green   Blue
{  4, 32, {{32,24,  24,16,  16, 8,   8, 0 }}, GGL_RGBA },   // IEP_FORMAT_ARGB_8888
{  4, 32, {{32,24,   8, 0,  16, 8,  24,16 }}, GGL_RGB  },   // IEP_FORMAT_ABGR_8888
{  4, 32, {{ 8, 0,  32,24,  24,16,  16, 8 }}, GGL_RGB  },   // IEP_FORMAT_RGBA_8888
{  4, 32, {{ 8, 0,  16, 8,  24,16,  32,24 }}, GGL_BGRA },   // IEP_FORMAT_BGRA_8888
{  2, 16, {{ 0, 0,  16,11,  11, 5,   5, 0 }}, GGL_RGB  },   // IEP_FORMAT_RGB_565
{  2, 16, {{ 0, 0,   5, 0,  11, 5,  16,11 }}, GGL_RGB  },   // IEP_FORMAT_RGB_565
*/

// for hardware format value
enum {
    IEP_MSG_FMT_ARGB_8888       = 0x0,
    IEP_MSG_FMT_ABGR_8888       = 0x1,
    IEP_MSG_FMT_RGBA_8888       = 0x2,
    IEP_MSG_FMT_BGRA_8888       = 0x3,
    IEP_MSG_FMT_RGB_565         = 0x4,
    IEP_MSG_FMT_BGR_565         = 0x5,

    IEP_MSG_FMT_YCbCr_422_SP    = 0x10,
    IEP_MSG_FMT_YCbCr_422_P     = 0x11,
    IEP_MSG_FMT_YCbCr_420_SP    = 0x12,
    IEP_MSG_FMT_YCbCr_420_P     = 0x13,
    IEP_MSG_FMT_YCrCb_422_SP    = 0x14,
    IEP_MSG_FMT_YCrCb_422_P     = 0x15, // same as IEP_FORMAT_YCbCr_422_P
    IEP_MSG_FMT_YCrCb_420_SP    = 0x16,
    IEP_MSG_FMT_YCrCb_420_P     = 0x17, // same as IEP_FORMAT_YCbCr_420_P
};

/*
 *  + <--------------+  virtual width  +------------> +
 *  |                                                 |
 *  +-------------------------------------------------+---+---+
 *  |                                                 |   ^
 *  |      + <---+  actual width  +--> +              |   |
 *  |      |                           |              |   |
 *  |      +---------------------------+---+--+       |   |
 *  |      |                           |   ^          |   +
 *  |      |                           |   +          | virtual
 *  |      |                           | actual       | height
 *  |      |                           | height       |   +
 *  |      |                           |   +          |   |
 *  |      |                           |   ^          |   |
 *  |      +---------------------------+---+--+       |   |
 *  |                                                 |   ^
 *  +-------------------------------------------------+---+---+
 */
// NOTE: This is for kernel driver communication. keep it for compatibility
typedef struct IepMsgImg_t {
    RK_U16  act_w;              // act_width
    RK_U16  act_h;              // act_height
    RK_S16  x_off;              // x offset for the vir,word unit
    RK_S16  y_off;              // y offset for the vir,word unit

    RK_U16  vir_w;              // unit :pix
    RK_U16  vir_h;              // unit :pix
    RK_U32  format;
    RK_U32  mem_addr;
    RK_U32  uv_addr;
    RK_U32  v_addr;

    RK_U8   rb_swap;            // not be used
    RK_U8   uv_swap;            // not be used

    RK_U8   alpha_swap;         // not be used
} IepMsgImg;

typedef struct IepMsg_t {
    IepMsgImg src;              // src active window
    IepMsgImg dst;              // src virtual window

    IepMsgImg src1;
    IepMsgImg dst1;

    IepMsgImg src_itemp;
    IepMsgImg src_ftemp;

    IepMsgImg dst_itemp;
    IepMsgImg dst_ftemp;

    RK_U8   dither_up_en;
    RK_U8   dither_down_en;     //not to be used

    RK_U8   yuv2rgb_mode;
    RK_U8   rgb2yuv_mode;

    RK_U8   global_alpha_value;

    RK_U8   rgb2yuv_clip_en;
    RK_U8   yuv2rgb_clip_en;

    RK_U8   lcdc_path_en;
    RK_S32  off_x;
    RK_S32  off_y;
    RK_S32  width;
    RK_S32  height;
    RK_S32  layer;

    RK_U8   yuv_3D_denoise_en;

    /// yuv color enhance
    RK_U8   yuv_enhance_en;
    RK_S32  sat_con_int;
    RK_S32  contrast_int;
    RK_S32  cos_hue_int;
    RK_S32  sin_hue_int;
    RK_S8   yuv_enh_brightness;     // -32 < brightness < 31
    RK_U8   video_mode;             // 0 ~ 3
    RK_U8   color_bar_y;            // 0 ~ 127
    RK_U8   color_bar_u;            // 0 ~ 127
    RK_U8   color_bar_v;            // 0 ~ 127


    RK_U8   rgb_enhance_en;         // I don't know its usage

    RK_U8   rgb_color_enhance_en;   // sw_rgb_color_enh_en
    RK_U32  rgb_enh_coe;

    RK_U8   rgb_enhance_mode;       // sw_rgb_enh_sel,dde sel

    RK_U8   rgb_cg_en;              // sw_rgb_con_gam_en
    RK_U32  cg_tab[192];

    // sw_con_gam_order; 0 cg prior to dde, 1 dde prior to cg
    RK_U8   rgb_contrast_enhance_mode;

    RK_S32  enh_threshold;
    RK_S32  enh_alpha;
    RK_S32  enh_radius;

    RK_U8   scale_up_mode;

    RK_U8   field_order;
    RK_U8   dein_mode;
    //DIL High Frequency
    RK_U8   dein_high_fre_en;
    RK_U8   dein_high_fre_fct;
    //DIL EI
    RK_U8   dein_ei_mode;
    RK_U8   dein_ei_smooth;
    RK_U8   dein_ei_sel;
    RK_U8   dein_ei_radius;         // when dein_ei_sel=0 will be used

    RK_U8   dil_mtn_tbl_en;
    RK_U32  dil_mtn_tbl[8];

    RK_U8   vir_addr_enable;

    void    *base;
} IepMsg;

#endif /* __IEP_H__ */
