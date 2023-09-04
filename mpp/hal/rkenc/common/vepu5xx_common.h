/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#ifndef __VEPU5XX_COMMON_H__
#define __VEPU5XX_COMMON_H__

#include "rk_type.h"
#include "mpp_frame.h"

typedef struct VepuRgb2YuvCoeffs_t {
    RK_S16 r_coeff;
    RK_S16 g_coeff;
    RK_S16 b_coeff;
    RK_S16 offset;
} VepuRgb2YuvCoeffs;

/* formula: y(r, g, b) = (r_coeff x r + g_coeff x g + b_coeff x b + 128) >> 8 + offset */
typedef struct VepuRgb2YuvCfg_t {
    /* YUV colorspace type */
    MppFrameColorSpace color;
    /* MPEG vs JPEG YUV range */
    MppFrameColorRange dst_range;
    /* coeffs of rgb 2 yuv */
    VepuRgb2YuvCoeffs  _2y;
    VepuRgb2YuvCoeffs  _2u;
    VepuRgb2YuvCoeffs  _2v;
} VepuRgb2YuvCfg;

/**
 * @brief Get rgb2yuv cfg by color range and color space. If not found, return default cfg.
 *        default cfg's yuv range - limit.
 *        default cfg's color space - BT.601.
 */
const VepuRgb2YuvCfg* get_rgb2yuv_cfg(MppFrameColorRange range, MppFrameColorSpace color);

extern const RK_U32 vepu580_540_h264_flat_scl_tab[576];

extern const RK_U32 klut_weight[24];
extern const RK_U32 lamd_satd_qp[52];
extern const RK_U32 lamd_moda_qp[52];
extern const RK_U32 lamd_modb_qp[52];

#endif /* __VEPU5XX_COMMON_H__ */