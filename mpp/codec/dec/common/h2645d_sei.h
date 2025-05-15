/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#ifndef _H2645D_SEI_H_
#define _H2645D_SEI_H_

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_bitread.h"

typedef struct SEI_Recovery_Point_t {
    RK_U32 valid_flag;              // Whether this SEI is valid or not
    RK_S32 recovery_frame_cnt;      // H.264: recovery_frame_cnt; H.265: recovery_poc_cnt
    RK_S32 first_frm_id;            // The frame_num or poc of the frame associated with this SEI
    RK_U32 first_frm_valid;         // The frame associated with this SEI is valid or not
    RK_U32 first_frm_ref_missing;
    RK_S32 recovery_pic_id;         // first_frm_id + recovery_frame_cnt;
} RecoveryPoint;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET check_encoder_sei_info(BitReadCtx_t *gb, RK_S32 payload_size, RK_U32 *is_match);

#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _H2645D_SEI_H_ */