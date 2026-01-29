/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_SPS_H
#define H265D_SPS_H

#include "h265d_syntax.h"
#include "h265d_vps.h"

typedef enum ScaleSizeId_e {
    SCALE_4X4_COPY = 0,
    SCALE_4X4,
    SCALE_8X8,
    SCALE_16X16
} ScaleSizeId;

typedef struct H265dCrop_t {
    RK_S32 left_offset;
    RK_S32 right_offset;
    RK_S32 top_offset;
    RK_S32 bottom_offset;
} H265dCrop;

typedef struct H265dScalingList_t {
    RK_U8 scaling_lists[4][6][64];
    RK_U8 scaling_list_dc[2][6];
} H265dScalingList;

typedef struct H265dVui_t {
    RK_S32 vui_present;

    RK_S32 sar_num;
    RK_S32 sar_den;

    RK_S32 overscan_info_present_flag;
    RK_S32 overscan_appropriate_flag;

    RK_S32 video_signal_type_present_flag;
    RK_S32 video_format;
    RK_S32 video_full_range_flag;
    RK_S32 colour_description_present_flag;
    RK_U32 colour_primaries;
    RK_U32 transfer_characteristic;
    RK_U32 matrix_coeffs;

    RK_S32 chroma_loc_info_present_flag;
    RK_S32 chroma_sample_loc_type_top_field;
    RK_S32 chroma_sample_loc_type_bottom_field;
    RK_S32 neutra_chroma_indication_flag;

    RK_S32 field_seq_flag;
    RK_S32 frame_field_info_present_flag;

    RK_S32 default_display_window_flag;
    H265dCrop def_disp_win;

    RK_S32 vui_timing_info_present_flag;
    RK_U32 vui_num_units_in_tick;
    RK_U32 vui_time_scale;
    RK_S32 vui_poc_proportional_to_timing_flag;
    RK_S32 vui_num_ticks_poc_diff_one_minus1;
    RK_S32 vui_hrd_parameters_present_flag;

    RK_S32 bitstream_restriction_flag;
    RK_S32 tiles_fixed_structure_flag;
    RK_S32 motion_vectors_over_pic_boundaries_flag;
    RK_S32 restricted_ref_pic_lists_flag;
    RK_S32 min_spatial_segmentation_idc;
    RK_S32 max_bytes_per_pic_denom;
    RK_S32 max_bits_per_min_cu_denom;
    RK_S32 log2_max_mv_length_horizontal;
    RK_S32 log2_max_mv_length_vertical;
} H265dVui;

typedef struct H265dSps_t {
    RK_U32 vps_id;
    RK_U32 sps_id;
    RK_S32 max_sub_layers;
    LayerInfo layers[MAX_SUB_LAYERS];

    RK_S32 chroma_format_idc;
    RK_U32 separate_colour_plane_flag;

    RK_S32 width;
    RK_S32 height;

    H265dCrop conf_win;

    RK_U32 bit_depth;
    RK_U32 log2_max_poc_lsb;
    RK_U32 log2_min_cb_size;
    RK_U32 log2_diff_max_min_coding_block_size;
    RK_U32 log2_min_tb_size;
    RK_U32 log2_max_trafo_size;

    RK_S32 max_transform_hierarchy_depth_inter;
    RK_S32 max_transform_hierarchy_depth_intra;

    RK_U32 scaling_list_enable_flag;
    H265dScalingList scaling_list;

    RK_U32 amp_enabled_flag;
    RK_U32 sao_enabled;

    RK_S32 pcm_enabled_flag;

    struct {
        RK_U32 bit_depth;
        RK_U32 bit_depth_chroma;
        RK_U32 log2_min_pcm_cb_size;
        RK_U32 log2_max_pcm_cb_size;
        RK_U32 loop_filter_disable_flag;
    } pcm;

    RK_U32 nb_st_rps;
    H265dStRps st_rps_sps[MAX_SHORT_TERM_RPS_COUNT];
    H265dLtRpsSps lt_rps_sps;

    RK_U8 sps_temporal_mvp_enabled_flag;
    RK_U8 sps_strong_intra_smoothing_enable_flag;

    H265dVui vui;

    RK_S32 sps_extension_flag;
    RK_S32 sps_range_extension_flag;
    RK_S32 transform_skip_rotation_enabled_flag;
    RK_S32 transform_skip_context_enabled_flag;
    RK_S32 implicit_rdpcm_enabled_flag;
    RK_S32 explicit_rdpcm_enabled_flag;
    RK_S32 extended_precision_processing_flag;
    RK_S32 intra_smoothing_disabled_flag;
    RK_S32 high_precision_offsets_enabled_flag;
    RK_S32 persistent_rice_adaptation_enabled_flag;
    RK_S32 cabac_bypass_alignment_enabled_flag;

    RK_S32 pix_fmt;
    RK_S32 output_width;
    RK_S32 output_height;
    H265dCrop output_window;
    RK_S32 log2_ctb_size;
    RK_S32 ctb_width;
    RK_S32 ctb_height;
    RK_S32 min_cb_width;
    RK_S32 min_cb_height;
    RK_S32 qp_bd_offset;
} H265dSps;

#ifdef  __cplusplus
extern "C" {
#endif

void h265d_set_default_scaling_list(H265dScalingList *sl);
RK_S32 h265d_scaling_list_data(BitReadCtx_t *bit, H265dScalingList *sl, H265dSps *sps);
RK_S32 h265d_st_rps(BitReadCtx_t *bit, H265dStRps *rps, const H265dSps *sps,
                    RK_S32 is_slice_header, RK_U8 *rps_need_update);
RK_S32 h265d_nal_sps(BitReadCtx_t *bit, H265dSps *sps, const H265dVps *vps_list[]);

#ifdef  __cplusplus
}
#endif

#endif /* H265D_SPS_H */
