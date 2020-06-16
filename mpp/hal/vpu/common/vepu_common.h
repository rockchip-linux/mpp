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
#ifndef __VEPU_COMMON_H__
#define __VEPU_COMMON_H__

#include "mpp_frame.h"

typedef enum VepuFormat_e {
    VEPU_FMT_YUV420PLANAR,          //0
    VEPU_FMT_YUV420SEMIPLANAR,      //1
    VEPU_FMT_YUYV422INTERLEAVED,    //2
    VEPU_FMT_UYVY422INTERLEAVED,    //3
    VEPU_FMT_RGB565,                //4
    VEPU_FMT_RGB555,                //5
    VEPU_FMT_RGB444,                //6
    VEPU_FMT_RGB888,                //7
    VEPU_FMT_RGB101010,             //8
    VEPU_FMT_BUTT,                  //9
} VepuFmt;

typedef struct VepuFormatCfg_t {
    VepuFmt format;
    RK_U8 r_mask;
    RK_U8 g_mask;
    RK_U8 b_mask;
    RK_U8 swap_8_in;
    RK_U8 swap_16_in;
    RK_U8 swap_32_in;
} VepuFormatCfg;

MPP_RET get_vepu_fmt(VepuFormatCfg *cfg, MppFrameFormat format);

#endif