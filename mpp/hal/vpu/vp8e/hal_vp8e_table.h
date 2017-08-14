/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_VP8E_TABLE_H__
#define __HAL_VP8E_TABLE_H__

#include "rk_type.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_putbit.h"

extern RK_S32 const default_prob_coeff_tbl[4][8][3][11];

extern RK_S32 const default_prob_mv_tbl[2][19];

extern RK_S32 const vp8_prob_cost_tbl[];

extern RK_S32 const coeff_update_prob_tbl[4][8][3][11];

extern RK_S32 const mv_update_prob_tbl[2][19];

extern RK_S32 const default_skip_false_prob_tbl[128];

extern RK_S32 const y_mode_prob_tbl[4];

extern RK_S32 const uv_mode_prob_tbl[3];

extern Vp8eTree const mv_tree_tbl[];

extern RK_S32 const dc_q_lookup_tbl[QINDEX_RANGE];

extern RK_S32 const ac_q_lookup_tbl[QINDEX_RANGE];

extern RK_S32 const q_rounding_factors_tbl[QINDEX_RANGE];

extern RK_S32 const q_zbin_factors_tbl[QINDEX_RANGE];

extern RK_S32 const zbin_boost_tbl[16];

extern RK_S32 const vp8_split_penalty_tbl[128];

extern RK_S32 const weight_tbl[128];

extern RK_S32 const intra4_mode_tree_penalty_tbl[];

extern RK_S32 const intra16_mode_tree_penalty_tbl[];

const RK_S32 const inter_level_tbl[128];

extern RK_S32 const offset_tbl[];

#endif /*__HAL_VP8E_TBL_H__*/
