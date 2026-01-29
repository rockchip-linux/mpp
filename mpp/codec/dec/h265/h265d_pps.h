/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_PPS_H
#define H265D_PPS_H

#include "h265d_sps.h"
#include "mpp_soc.h"

typedef struct H265dPps_t {
    RK_U32 sps_id;
    RK_U32 pps_id;

    RK_U32 sign_data_hiding_flag;
    RK_U32 cabac_init_present_flag;

    RK_S32 num_ref_idx_l0_default_active;
    RK_S32 num_ref_idx_l1_default_active;
    RK_S32 pic_init_qp_minus26;

    RK_U32 constrained_intra_pred_flag;
    RK_U32 transform_skip_enabled_flag;

    RK_U32 cu_qp_delta_enabled_flag;
    RK_S32 diff_cu_qp_delta_depth;

    RK_S32 cb_qp_offset;
    RK_S32 cr_qp_offset;
    RK_U32 pic_slice_level_chroma_qp_offsets_present_flag;
    RK_U32 weighted_pred_flag;
    RK_U32 weighted_bipred_flag;
    RK_U32 output_flag_present_flag;
    RK_U32 transquant_bypass_enable_flag;

    RK_U32 dependent_slice_segments_enabled_flag;
    RK_U32 tiles_enabled_flag;
    RK_U32 entropy_coding_sync_enabled_flag;

    RK_S32 num_tile_columns;
    RK_S32 num_tile_rows;
    RK_U32 uniform_spacing_flag;
    RK_U32 loop_filter_across_tiles_enabled_flag;

    RK_U32 seq_loop_filter_across_slices_enabled_flag;

    RK_U32 deblocking_filter_control_present_flag;
    RK_U32 deblocking_filter_override_enabled_flag;
    RK_U32 disable_dbf;
    RK_S32 beta_offset;
    RK_S32 tc_offset;

    RK_U32 scaling_list_data_present_flag;
    H265dScalingList scaling_list;

    RK_U32 lists_modification_present_flag;
    RK_S32 log2_parallel_merge_level;
    RK_S32 num_extra_slice_header_bits;
    RK_U32 slice_header_extension_present_flag;

    RK_U32 pps_extension_flag;
    RK_U32 pps_range_extensions_flag;
    RK_U32 log2_max_transform_skip_block_size;
    RK_U32 cross_component_prediction_enabled_flag;
    RK_U32 chroma_qp_offset_list_enabled_flag;
    RK_U32 diff_cu_chroma_qp_offset_depth;
    RK_U32 chroma_qp_offset_list_len_minus1;
    RK_S8 cb_qp_offset_list[6];
    RK_S8 cr_qp_offset_list[6];
    RK_U32 log2_sao_offset_scale_luma;
    RK_U32 log2_sao_offset_scale_chroma;

    RK_S32 column_width_size;
    RK_S32 row_height_size;
    RK_U32 column_width_offset;  /* Offset from struct start to tile data */
    RK_U32 row_height_offset;    /* Offset from struct start to tile data */
    RK_U32 total_size;           /* Total size including tile data for memcmp */
} H265dPps;

/* Helper macros to access tile data via offsets */
#define H265D_PPS_TILE_DATA(pps) ((RK_U8*)(pps) + sizeof(H265dPps))
#define H265D_PPS_COLUMN_WIDTH(pps) ((RK_U32*)(H265D_PPS_TILE_DATA(pps) + (pps)->column_width_offset))
#define H265D_PPS_ROW_HEIGHT(pps)   ((RK_U32*)(H265D_PPS_TILE_DATA(pps) + (pps)->row_height_offset))

#ifdef  __cplusplus
extern "C" {
#endif

H265dPps *h265d_nal_pps(BitReadCtx_t *bit, const H265dSps *sps_list[]);
RK_S32 h265d_pps_check(const H265dPps *pps, const H265dSps *sps,
                       const MppDecHwCap *hw_info);

#ifdef  __cplusplus
}
#endif

#endif /* H265D_PPS_H */
