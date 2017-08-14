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
