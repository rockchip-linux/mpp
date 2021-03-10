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

#ifndef __H264E_PPS_H__
#define __H264E_PPS_H__

#include "mpp_packet.h"
#include "mpp_enc_cfg.h"

#include "h264_syntax.h"

typedef struct H264ePps_t {
    RK_S32      pps_id;
    RK_S32      sps_id;

    // 0 - CAVLC 1 - CABAC
    RK_S32      entropy_coding_mode;

    RK_S32      bottom_field_pic_order_in_frame_present;
    RK_S32      num_slice_groups;

    RK_S32      num_ref_idx_l0_default_active;
    RK_S32      num_ref_idx_l1_default_active;

    RK_S32      weighted_pred;
    RK_S32      weighted_bipred_idc;

    RK_S32      pic_init_qp;
    RK_S32      pic_init_qs;

    RK_S32      chroma_qp_index_offset;
    RK_S32      second_chroma_qp_index_offset_present;
    RK_S32      second_chroma_qp_index_offset;

    RK_S32      deblocking_filter_control;
    RK_S32      constrained_intra_pred;
    RK_S32      redundant_pic_cnt;

    RK_S32      transform_8x8_mode;

    // Only support flat and default scaling list
    RK_S32      pic_scaling_matrix_present;
    RK_S32      use_default_scaling_matrix[H264_SCALING_MATRIX_TYPE_BUTT];
} H264ePps;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264e_pps_update(H264ePps *pps, MppEncCfgSet *cfg);
MPP_RET h264e_pps_to_packet(H264ePps *pps, MppPacket packet, RK_S32 *len);
MPP_RET h264e_pps_dump(H264ePps *pps);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_PPS_H__ */
