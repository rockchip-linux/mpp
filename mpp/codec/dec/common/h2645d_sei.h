/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#ifndef _H2645D_SEI_H_
#define _H2645D_SEI_H_

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_bitread.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET check_encoder_sei_info(BitReadCtx_t *gb, RK_S32 payload_size, RK_U32 *is_match);

#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _H2645D_SEI_H_ */