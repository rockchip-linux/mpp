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

#ifndef __RC_VEPU_H__
#define __RC_VEPU_H__

#include "rk_type.h"

#define VEPU_BLK_RC_CTRL_LEVELS         7
#define VEPU_BLK_RC_CHECK_POINTS_MAX    10

typedef struct RcVepuSet_t {
    RK_S32  qp_init;
    RK_S32  qp_min;
    RK_S32  qp_max;

    RK_S32  mad_qp_delta;
    RK_S32  mad_threshold;

    /*
     * cp = checkpoint
     *
     * vepu1 and vepu2 use check point to control qp change accoring to runtime
     * bit rate change in one frame.
     *
     * cp_distance_mbs defines how many the mb number between two checkpoints
     *
     * cp_target define the target bitrate on each checkpoint
     *
     * target_error and delta_qp define the qp delta value on the checkpoint when
     * the bitrate error different to cp_target is happened.
     */
    RK_S32  cp_distance_mbs;
    RK_S32  cp_target[VEPU_BLK_RC_CHECK_POINTS_MAX];
    RK_S32  target_error[VEPU_BLK_RC_CTRL_LEVELS];
    RK_S32  delta_qp[VEPU_BLK_RC_CTRL_LEVELS];
} RcVepuSet;

typedef struct RcVepuRet_t {
    RK_S32  stream_size;

    RK_S32  qp_sum;
    RK_S32  mad_count;
    RK_S32  rlc_count;

    RK_S32  cp_bits[VEPU_BLK_RC_CHECK_POINTS_MAX];
} RcVepuRet;

#endif /* __RC_VEPU_H__ */
