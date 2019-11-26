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

#ifndef __HAL_H264E_VPU_TBL_H__
#define __HAL_H264E_VPU_TBL_H__

#include "rk_type.h"

#define H264E_CABAC_TABLE_BUF_SIZE  (52*2*464)

/* H.264 motion estimation parameters */
extern const RK_U32 h264_prev_mode_favor[52];
/* sqrt(2^((qp-12)/3))*8 */
extern const RK_U32 h264_diff_mv_penalty[52];
/* 31*sqrt(2^((qp-12)/3))/4 */
extern const RK_U32 h264_diff_mv_penalty4p[52];
extern const RK_U32 h264_intra16_favor[52];
extern const RK_U32 h264_inter_favor[52];
extern const RK_U32 h264_skip_sad_penalty[52];
extern const RK_S32 h264_context_init_intra[460][2];
extern const RK_S32 h264_context_init[3][460][2];

#endif
