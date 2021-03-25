/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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
#ifndef __FILM_GRAIN_NOISE_TABLE_H__
#define __FILM_GRAIN_NOISE_TABLE_H__
#include "rk_type.h"

void GenerateLumaGrainBlock(RK_S32 luma_grain_block[][82], RK_S32 bitdepth,
                            RK_U8 num_y_points, RK_S32 grain_scale_shift,
                            RK_S32 ar_coeff_lag, RK_S32 ar_coeffs_y[],
                            RK_S32 ar_coeff_shift, RK_S32 grain_min, RK_S32 grain_max,
                            RK_U16 random_seed);

void GenerateChromaGrainBlock(
    RK_S32 luma_grain_block[][82], RK_S32 cb_grain_block[][44],
    RK_S32 cr_grain_block[][44], RK_S32 bitdepth, RK_U8 num_y_points, RK_U8 num_cb_points,
    RK_U8 num_cr_points, RK_S32 grain_scale_shift, RK_S32 ar_coeff_lag,
    RK_S32 ar_coeffs_cb[], RK_S32 ar_coeffs_cr[], RK_S32 ar_coeff_shift, RK_S32 grain_min,
    RK_S32 grain_max, RK_U8 chroma_scaling_from_luma, RK_U16 random_seed);

#endif  //__FILM_GRAIN_NOISE_TABLE_H__