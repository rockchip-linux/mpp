/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_VP8E_ENTROPY_H__
#define __HAL_VP8E_ENTROPY_H__

#include "rk_type.h"

#include "hal_vp8e_putbit.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET vp8e_init_entropy(void *hal);
MPP_RET vp8e_write_entropy_tables(void *hal);
RK_S32  vp8e_calc_cost_mv(RK_S32 mvd, RK_S32 *mv_prob);
MPP_RET vp8e_calc_coeff_prob(Vp8ePutBitBuf *bitbuf,
                             RK_S32 (*curr)[4][8][3][11], RK_S32 (*prev)[4][8][3][11]);
MPP_RET vp8e_calc_mv_prob(Vp8ePutBitBuf *bitbuf,
                          RK_S32 (*curr)[2][19], RK_S32 (*prev)[2][19]);
MPP_RET vp8e_swap_endian(RK_U32 *buf, RK_U32 bytes);

#ifdef  __cplusplus
}
#endif

#endif /*__HAL_VP8E_ENTROPY_H__*/
