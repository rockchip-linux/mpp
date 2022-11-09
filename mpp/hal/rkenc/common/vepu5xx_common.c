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

#define MODULE_TAG "vepu5xx_common.c"

#include "mpp_log.h"
#include "mpp_common.h"
#include "vepu5xx_common.h"

const VepuRgb2YuvCfg vepu_rgb2limit_yuv_cfg_set[] = {
    /* MPP_BT601_FULL_RGB_TO_LIMIT_YUV */
    {
        .color = MPP_FRAME_SPC_RGB, .dst_range = MPP_FRAME_RANGE_UNSPECIFIED,
        ._2y = {.r_coeff = 66, .g_coeff = 129, .b_coeff = 25, .offset = 16},
        ._2u = {.r_coeff = -38, .g_coeff = -74, .b_coeff = 112, .offset = 128},
        ._2v = {.r_coeff = 112, .g_coeff = -94, .b_coeff = -18, .offset = 128},
    },
    /* MPP_BT709_FULL_RGB_TO_LIMIT_YUV */
    {
        .color = MPP_FRAME_SPC_BT709, .dst_range = MPP_FRAME_RANGE_UNSPECIFIED,
        ._2y = {.r_coeff = 47, .g_coeff = 157, .b_coeff = 16, .offset = 16},
        ._2u = {.r_coeff = -26, .g_coeff = -87, .b_coeff = 112, .offset = 128},
        ._2v = {.r_coeff = 112, .g_coeff = -102, .b_coeff = -10, .offset = 128},
    },
};


const VepuRgb2YuvCfg vepu_rgb2full_yuv_cfg_set[] = {
    /* MPP_BT601_FULL_RGB_TO_FULL_YUV */
    {
        .color = MPP_FRAME_SPC_RGB, .dst_range = MPP_FRAME_RANGE_JPEG,
        ._2y = {.r_coeff = 77, .g_coeff = 150, .b_coeff = 29, .offset = 0},
        ._2u = {.r_coeff = -43, .g_coeff = -85, .b_coeff = 128, .offset = 128},
        ._2v = {.r_coeff = 128, .g_coeff = -107, .b_coeff = -21, .offset = 128},
    },
    /* MPP_BT709_FULL_RGB_TO_FULL_YUV */
    {
        .color = MPP_FRAME_SPC_BT709, .dst_range = MPP_FRAME_RANGE_JPEG,
        ._2y = {.r_coeff = 54, .g_coeff = 183, .b_coeff = 18, .offset = 0},
        ._2u = {.r_coeff = -29, .g_coeff = -99, .b_coeff = 128, .offset = 128},
        ._2v = {.r_coeff = 128, .g_coeff = -116, .b_coeff = -12, .offset = 128},
    },
};

const VepuRgb2YuvCfg *get_rgb2yuv_cfg(MppFrameColorRange range, MppFrameColorSpace color)
{
    const VepuRgb2YuvCfg *cfg;
    RK_U32 size;
    RK_U32 i;

    /* only jpeg full range, others limit range */
    if (range == MPP_FRAME_RANGE_JPEG) {
        /* set default cfg BT.601 */
        cfg = &vepu_rgb2full_yuv_cfg_set[0];
        size = MPP_ARRAY_ELEMS(vepu_rgb2full_yuv_cfg_set);
    } else {
        /* set default cfg BT.601 */
        cfg = &vepu_rgb2limit_yuv_cfg_set[0];
        size = MPP_ARRAY_ELEMS(vepu_rgb2limit_yuv_cfg_set);
    }

    for (i = 0; i < size; i++)
        if (cfg[i].color == color)
            return &cfg[i];

    return cfg;
}