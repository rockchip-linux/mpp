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

#ifndef __IEP_API_H__
#define __IEP_API_H__

#include "rk_type.h"
#include "mpp_err.h"

#include "iep_common.h"

/*
 * IepCmdParamSetSrc is parameter for:
 * IEP_CMD_SET_SRC
 * IEP_CMD_SET_DST
 * IEP_CMD_SET_DEI_SRC1
 * IEP_CMD_SET_DEI_DST1
 */
typedef struct IepCmdParamImage_t {
    IepImg  image;
} IepCmdParamImage;

typedef enum IepDeiMode_e {
    IEP_DEI_MODE_DISABLE,
    IEP_DEI_MODE_I2O1,
    IEP_DEI_MODE_I4O1,
    IEP_DEI_MODE_I4O2,
    IEP_DEI_MODE_BYPASS,
    IEP_DEI_MODE_BUTT,
} IepDeiMode;

typedef enum IepDeiFldOrder {
    IEP_DEI_FLD_ORDER_TOP_FIRST,
    IEP_DEI_FLD_ORDER_BOT_FIRST,
    IEP_DEI_FLD_ORDER_BUTT,
} IepDeiFldOrder;

/* IepCmdParamDeiCfg is parameter for IEP_CMD_SET_DEI_CFG */
typedef struct IepCmdParamDeiCfg_t {
    IepDeiMode      dei_mode;
    IepDeiFldOrder  dei_field_order;

    RK_U32          dei_high_freq_en;
    RK_U32          dei_high_freq_fct;  // [0, 127]

    RK_U32          dei_ei_mode;        // deinterlace edge interpolation 0: disable, 1: enable
    RK_U32          dei_ei_smooth;      // deinterlace edge interpolation for smooth effect 0: disable, 1: enable
    RK_U32          dei_ei_sel;         // deinterlace edge interpolation select
    RK_U32          dei_ei_radius;      // deinterlace edge interpolation radius [0, 3]
} IepCmdParamDeiCfg;

typedef enum IepVideoMode_e {
    IEP_VIDEO_MODE_BLACK_SCREEN,
    IEP_VIDEO_MODE_BLUE_SCREEN,
    IEP_VIDEO_MODE_COLOR_BAR,
    IEP_VIDEO_MODE_NORMAL_VIDEO
} IepVideoMode;

/* IepCmdParamYuvEnhance is parameter for IEP_CMD_SET_YUV_ENHANCE */
typedef struct IepCmdParamYuvEnhance_t {
    float           saturation;         // [0, 1.992]
    float           contrast;           // [0, 1.992]
    RK_S8           brightness;         // [-32, 31]
    float           hue_angle;          // [-30, 30]
    IepVideoMode    video_mode;
    RK_U8           color_bar_y;        // [0, 127]
    RK_U8           color_bar_u;        // [0, 127]
    RK_U8           color_bar_v;        // [0, 127]
} IepCmdParamYuvEnhance;

typedef enum IepRgbEnhanceMode_e {
    IEP_RGB_ENHANCE_MODE_NO_OPERATION,
    IEP_RGB_ENHANCE_MODE_DENOISE,
    IEP_RGB_ENHANCE_MODE_DETAIL_ENHANCE,
    IEP_RGB_ENHANCE_MODE_EDGE_ENHANCE
} IepRgbEnhanceMode;

typedef enum IepRgbEnhanceOrder_e {
    IEP_RGB_ENHANCE_ORDER_CG_DDE,       // CG(Contrast & Gammar) prior to DDE (De-noise, Detail & Edge Enhance)
    IEP_RGB_ENHANCE_ORDER_DDE_CG        // DDE prior to CG
} IepRgbEnhanceOrder;

/* IepCmdParamRgbEnhance is parameter for IEP_CMD_SET_RGB_ENHANCE */
typedef struct IepCmdParamRgbEnhance {
    float               coe;            // [0, 3.96875]
    IepRgbEnhanceMode   mode;
    RK_U8               cg_en;          // sw_rgb_con_gam_en
    double              cg_rr;
    double              cg_rg;
    double              cg_rb;
    IepRgbEnhanceOrder  order;

    /// more than this value considered as detail, and less than this value considered as noise.
    RK_S32              threshold;      // [0, 255]

    /// combine the original pixel and enhanced pixel
    /// if (enh_alpha_num / enh_alpha_base <= 1 ) enh_alpha_base = 8, else enh_alpha_base = 4
    /// (1/8 ... 8/8) (5/4 ... 24/4)
    RK_S32              alpha_num;      // [0, 24]
    RK_S32              alpha_base;     // {4, 8}
    RK_S32              radius;         // [1, 4]
} IepCmdParamRgbEnhance;

typedef enum IepScaleAlg_e {
    IEP_SCALE_ALG_HERMITE,
    IEP_SCALE_ALG_SPLINE,
    IEP_SCALE_ALG_CATROM,
    IEP_SCALE_ALG_MITCHELL
} IepScaleAlg;

/* IepCmdParamScale is parameter for IEP_CMD_SET_SCALE */
typedef struct IepCmdParamScale_t {
    IepScaleAlg         scale_alg;
} IepCmdParamScale;

typedef enum IepColorConvertMode_e {
    IEP_COLOR_MODE_BT601_L,
    IEP_COLOR_MODE_BT601_F,
    IEP_COLOR_MODE_BT709_L,
    IEP_COLOR_MODE_BT709_F
} IepColorConvertMode;

/* IepCmdParamColorConvert is parameter for IEP_CMD_SET_COLOR_CONVERT */
typedef struct IepCmdParamColorConvert_t {
    IepColorConvertMode rgb2yuv_mode;
    IepColorConvertMode yuv2rgb_mode;
    RK_U8               rgb2yuv_input_clip; // 0:R/G/B [0, 255], 1:R/G/B [16, 235]
    RK_U8               yuv2rgb_input_clip; // 0:Y/U/V [0, 255], 1:Y [16, 235] U/V [16, 240]
    RK_U8               global_alpha_value; // global alpha value for output ARGB
    RK_U8               dither_up_en;
    RK_U8               dither_down_en;
} IepCmdParamColorConvert;

/* IepCap is parameter for IEP_CMD_QUERY_CAP */
typedef struct IepCap_t {
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
} IepCap;

typedef void* IepCtx;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET iep_init(IepCtx *ctx);
MPP_RET iep_deinit(IepCtx ctx);
MPP_RET iep_control(IepCtx ctx, IepCmd cmd, void *param);

iep_com_ctx* rockchip_iep_api_alloc_ctx(void);
void rockchip_iep_api_release_ctx(iep_com_ctx *com_ctx);

#ifdef __cplusplus
}
#endif

#endif /* __IEP_API_H__ */
