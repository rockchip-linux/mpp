/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __RC_VEPU_540_H__
#define __RC_VEPU_540_H__

#include "rk_type.h"

#define VEPU540_BLK_RC_CTRL_LEVELS      9

typedef struct RcVepu540Set_t {
    RK_S32  rc_en;
    RK_S32  rc_mode;

    RK_S32  ctu_num;
    RK_S32  ctu_ebits;

    RK_S32  qp_pic;
    RK_S32  qp_min;
    RK_S32  qp_max;
    RK_S32  qp_range;

    RK_S32  aqmode_en;
    RK_S32  aq_strength;
    RK_S32  rc_fact0;
    RK_S32  rc_fact1;

    RK_S32  target_error[VEPU_BLK_RC_CTRL_LEVELS];
    RK_S32  delta_qp[VEPU_BLK_RC_CTRL_LEVELS];
} RcVepu540Set;

typedef struct RcVepu540Ret_t {
    RK_S32  stream_size;

    RK_U32  qp_sum;
    RK_U64  sse_sum;
} RcVepu540Ret;

#endif /* __RC_VEPU_540_H__ */
