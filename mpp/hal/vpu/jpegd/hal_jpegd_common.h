/*
 *
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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
#ifndef __HAL_JPEGD_COMMON_H__
#define __HAL_JPEGD_COMMON_H__

#include "hal_jpegd_base.h"

#define BRIGHTNESS                                  4    /* -128 ~ 127 */
#define CONTRAST                                    0    /* -64 ~ 64 */
#define SATURATION                                  0    /* -64 ~ 128 */
#define PP_IN_FORMAT_YUV422INTERLAVE                0
#define PP_IN_FORMAT_YUV420SEMI                     1
#define PP_IN_FORMAT_YUV420PLANAR                   2
#define PP_IN_FORMAT_YUV400                         3
#define PP_IN_FORMAT_YUV422SEMI                     4
#define PP_IN_FORMAT_YUV420SEMITIELED               5
#define PP_IN_FORMAT_YUV440SEMI                     6
#define PP_IN_FORMAT_YUV444_SEMI                    7
#define PP_IN_FORMAT_YUV411_SEMI                    8

#define PP_OUT_FORMAT_RGB565                        0
#define PP_OUT_FORMAT_ARGB                          1
#define PP_OUT_FORMAT_YUV422INTERLAVE               3
#define PP_OUT_FORMAT_YUV420INTERLAVE               5

#define MIN_WIDTH                         (48)      /* 48 Bytes */
#define MIN_HEIGHT                        (48)      /* 48 Bytes */
#define MAX_WIDTH                         (8*1024)  /* 4K Bytes */
#define MAX_HEIGHT                        (8*1024)  /* 4K Bytes */
#define MAX_STREAM_LENGTH                 (MAX_WIDTH * MAX_HEIGHT) /* 16M Bytes */

static const RK_U8 zzOrder[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

#define PP_RGB_CFG_LENTH (10)
typedef struct PpRgbCfg_t {
    MppFrameFormat fmt;
    RK_U8   pp_out_fmt;
    RK_U8   out_endian;
    RK_U8   swap_16;
    RK_U8   swap_32;
    RK_U8   rgb_in_32;
    RK_U8   r_padd;
    RK_U8   g_padd;
    RK_U8   b_padd;
    RK_U8   r_dither;
    RK_U8   g_dither;
    RK_U8   b_dither;
    RK_U32  r_mask;
    RK_U32  g_mask;
    RK_U32  b_mask;
} PpRgbCfg;

PpRgbCfg* get_pp_rgb_Cfg(MppFrameFormat fmt);
RK_U32 jpegd_vdpu_tail_0xFF_patch(MppBuffer stream, RK_U32 length);

void jpegd_write_qp_ac_dc_table(JpegdHalCtx *ctx,
                                JpegdSyntax*syntax);

void jpegd_check_have_pp(JpegdHalCtx *ctx);
MPP_RET jpegd_setup_output_fmt(JpegdHalCtx *ctx, JpegdSyntax *syntax,
                               RK_S32 output);

MPP_RET jpeg_image_check_size(RK_U32 hor_stride, RK_U32 ver_stride);

#endif /* __HAL_JPEGD_COMMON_H__ */
