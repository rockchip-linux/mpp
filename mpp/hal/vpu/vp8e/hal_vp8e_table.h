/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_VP8E_TABLE_H__
#define __HAL_VP8E_TABLE_H__

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

extern const RK_S32 inter_level_tbl[128];

extern RK_S32 const offset_tbl[];

#endif /*__HAL_VP8E_TBL_H__*/
