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

#ifndef __AV1D_CBS_H__
#define __AV1D_CBS_H__

#include "av1.h"

typedef struct AV1RawOBUHeader {
    RK_U8 obu_forbidden_bit;
    RK_U8 obu_type;
    RK_U8 obu_extension_flag;
    RK_U8 obu_has_size_field;
    RK_U8 obu_reserved_1bit;

    RK_U8 temporal_id;
    RK_U8 spatial_id;
    RK_U8 extension_header_reserved_3bits;
} AV1RawOBUHeader;

typedef struct AV1RawColorConfig {
    RK_U8 high_bitdepth;
    RK_U8 twelve_bit;
    RK_U8 mono_chrome;

    RK_U8 color_description_present_flag;
    RK_U8 color_primaries;
    RK_U8 transfer_characteristics;
    RK_U8 matrix_coefficients;

    RK_U8 color_range;
    RK_U8 subsampling_x;
    RK_U8 subsampling_y;
    RK_U8 chroma_sample_position;
    RK_U8 separate_uv_delta_q;
} AV1RawColorConfig;

typedef struct AV1RawTimingInfo {
    RK_U32 num_units_in_display_tick;
    RK_U32 time_scale;

    RK_U8 equal_picture_interval;
    RK_U32 num_ticks_per_picture_minus_1;
} AV1RawTimingInfo;

typedef struct AV1RawDecoderModelInfo {
    RK_U8  buffer_delay_length_minus_1;
    RK_U32 num_units_in_decoding_tick;
    RK_U8  buffer_removal_time_length_minus_1;
    RK_U8  frame_presentation_time_length_minus_1;
} AV1RawDecoderModelInfo;

typedef struct AV1RawSequenceHeader {
    RK_U8 seq_profile;
    RK_U8 still_picture;
    RK_U8 reduced_still_picture_header;

    RK_U8 timing_info_present_flag;
    RK_U8 decoder_model_info_present_flag;
    RK_U8 initial_display_delay_present_flag;
    RK_U8 operating_points_cnt_minus_1;

    AV1RawTimingInfo       timing_info;
    AV1RawDecoderModelInfo decoder_model_info;

    RK_U16 operating_point_idc[AV1_MAX_OPERATING_POINTS];
    RK_U8  seq_level_idx[AV1_MAX_OPERATING_POINTS];
    RK_U8  seq_tier[AV1_MAX_OPERATING_POINTS];
    RK_U8  decoder_model_present_for_this_op[AV1_MAX_OPERATING_POINTS];
    RK_U32 decoder_buffer_delay[AV1_MAX_OPERATING_POINTS];
    RK_U32 encoder_buffer_delay[AV1_MAX_OPERATING_POINTS];
    RK_U8  low_delay_mode_flag[AV1_MAX_OPERATING_POINTS];
    RK_U8  initial_display_delay_present_for_this_op[AV1_MAX_OPERATING_POINTS];
    RK_U8  initial_display_delay_minus_1[AV1_MAX_OPERATING_POINTS];

    RK_U8  frame_width_bits_minus_1;
    RK_U8  frame_height_bits_minus_1;
    RK_U16 max_frame_width_minus_1;
    RK_U16 max_frame_height_minus_1;

    RK_U8 frame_id_numbers_present_flag;
    RK_U8 delta_frame_id_length_minus_2;
    RK_U8 additional_frame_id_length_minus_1;

    RK_U8 use_128x128_superblock;
    RK_U8 enable_filter_intra;
    RK_U8 enable_intra_edge_filter;
    RK_U8 enable_interintra_compound;
    RK_U8 enable_masked_compound;
    RK_U8 enable_warped_motion;
    RK_U8 enable_dual_filter;

    RK_U8 enable_order_hint;
    RK_U8 enable_jnt_comp;
    RK_U8 enable_ref_frame_mvs;

    RK_U8 seq_choose_screen_content_tools;
    RK_U8 seq_force_screen_content_tools;
    RK_U8 seq_choose_integer_mv;
    RK_U8 seq_force_integer_mv;

    RK_U8 order_hint_bits_minus_1;

    RK_U8 enable_superres;
    RK_U8 enable_cdef;
    RK_U8 enable_restoration;

    AV1RawColorConfig color_config;

    RK_U8 film_grain_params_present;
} AV1RawSequenceHeader;

typedef struct AV1RawFilmGrainParams {
    RK_U8  apply_grain;
    RK_U16 grain_seed;
    RK_U8  update_grain;
    RK_U8  film_grain_params_ref_idx;
    RK_U8  num_y_points;
    RK_U8  point_y_value[14];
    RK_U8  point_y_scaling[14];
    RK_U8  chroma_scaling_from_luma;
    RK_U8  num_cb_points;
    RK_U8  point_cb_value[10];
    RK_U8  point_cb_scaling[10];
    RK_U8  num_cr_points;
    RK_U8  point_cr_value[10];
    RK_U8  point_cr_scaling[10];
    RK_U8  grain_scaling_minus_8;
    RK_U8  ar_coeff_lag;
    RK_U8  ar_coeffs_y_plus_128[24];
    RK_U8  ar_coeffs_cb_plus_128[25];
    RK_U8  ar_coeffs_cr_plus_128[25];
    RK_U8  ar_coeff_shift_minus_6;
    RK_U8  grain_scale_shift;
    RK_U8  cb_mult;
    RK_U8  cb_luma_mult;
    RK_U16 cb_offset;
    RK_U8  cr_mult;
    RK_U8  cr_luma_mult;
    RK_U16 cr_offset;
    RK_U8  overlap_flag;
    RK_U8  clip_to_restricted_range;
} AV1RawFilmGrainParams;

typedef struct AV1RawFrameHeader {
    RK_U8  show_existing_frame;
    RK_U8  frame_to_show_map_idx;
    RK_U32 frame_presentation_time;
    RK_U32 display_frame_id;

    RK_U8 frame_type;
    RK_U8 show_frame;
    RK_U8 showable_frame;

    RK_U8 error_resilient_mode;
    RK_U8 disable_cdf_update;
    RK_U8 allow_screen_content_tools;
    RK_U8 force_integer_mv;

    RK_S32 current_frame_id;
    RK_U8  frame_size_override_flag;
    RK_U8  order_hint;

    RK_U8  buffer_removal_time_present_flag;
    RK_U32 buffer_removal_time[AV1_MAX_OPERATING_POINTS];

    RK_U8  primary_ref_frame;
    RK_U16 frame_width_minus_1;
    RK_U16 frame_height_minus_1;
    RK_U8  use_superres;
    RK_U8  coded_denom;
    RK_U8  render_and_frame_size_different;
    RK_U16 render_width_minus_1;
    RK_U16 render_height_minus_1;

    RK_U8 found_ref[AV1_REFS_PER_FRAME];

    RK_U8 refresh_frame_flags;
    RK_U8 allow_intrabc;
    RK_U8 ref_order_hint[AV1_NUM_REF_FRAMES];
    RK_U8 frame_refs_short_signaling;
    RK_U8 last_frame_idx;
    RK_U8 golden_frame_idx;
    RK_S8  ref_frame_idx[AV1_REFS_PER_FRAME];
    RK_U32 delta_frame_id_minus1[AV1_REFS_PER_FRAME];

    RK_U8 allow_high_precision_mv;
    RK_U8 is_filter_switchable;
    RK_U8 interpolation_filter;
    RK_U8 is_motion_mode_switchable;
    RK_U8 use_ref_frame_mvs;

    RK_U8 disable_frame_end_update_cdf;

    RK_U8 uniform_tile_spacing_flag;
    RK_U8 tile_cols_log2;
    RK_U8 tile_rows_log2;
    RK_U8 width_in_sbs_minus_1[AV1_MAX_TILE_COLS];
    RK_U8 height_in_sbs_minus_1[AV1_MAX_TILE_ROWS];
    RK_U16 context_update_tile_id;
    RK_U8 tile_size_bytes_minus1;

    // These are derived values, but it's very unhelpful to have to
    // recalculate them all the time so we store them here.
    RK_U16 tile_cols;
    RK_U16 tile_rows;

    RK_U8 base_q_idx;
    RK_S8  delta_q_y_dc;
    RK_U8 diff_uv_delta;
    RK_S8  delta_q_u_dc;
    RK_S8  delta_q_u_ac;
    RK_S8  delta_q_v_dc;
    RK_S8  delta_q_v_ac;
    RK_U8 using_qmatrix;
    RK_U8 qm_y;
    RK_U8 qm_u;
    RK_U8 qm_v;

    RK_U8 segmentation_enabled;
    RK_U8 segmentation_update_map;
    RK_U8 segmentation_temporal_update;
    RK_U8 segmentation_update_data;
    RK_U8 feature_enabled[AV1_MAX_SEGMENTS][AV1_SEG_LVL_MAX];
    RK_S16 feature_value[AV1_MAX_SEGMENTS][AV1_SEG_LVL_MAX];

    RK_U8 delta_q_present;
    RK_U8 delta_q_res;
    RK_U8 delta_lf_present;
    RK_U8 delta_lf_res;
    RK_U8 delta_lf_multi;

    RK_U8 loop_filter_level[4];
    RK_U8 loop_filter_sharpness;
    RK_U8 loop_filter_delta_enabled;
    RK_U8 loop_filter_delta_update;
    RK_U8 update_ref_delta[AV1_TOTAL_REFS_PER_FRAME];
    RK_S8  loop_filter_ref_deltas[AV1_TOTAL_REFS_PER_FRAME];
    RK_U8 update_mode_delta[2];
    RK_S8  loop_filter_mode_deltas[2];

    RK_U8 cdef_damping_minus_3;
    RK_U8 cdef_bits;
    RK_U8 cdef_y_pri_strength[8];
    RK_U8 cdef_y_sec_strength[8];
    RK_U8 cdef_uv_pri_strength[8];
    RK_U8 cdef_uv_sec_strength[8];

    RK_U8 lr_type[3];
    RK_U8 lr_unit_shift;
    RK_U8 lr_uv_shift;

    RK_U8 tx_mode;
    RK_U8 reference_select;
    RK_U8 skip_mode_present;

    RK_U8 allow_warped_motion;
    RK_U8 reduced_tx_set;

    RK_U8 is_global[AV1_TOTAL_REFS_PER_FRAME];
    RK_U8 is_rot_zoom[AV1_TOTAL_REFS_PER_FRAME];
    RK_U8 is_translation[AV1_TOTAL_REFS_PER_FRAME];
    //AV1RawSubexp gm_params[AV1_TOTAL_REFS_PER_FRAME][6];
    RK_U32 gm_params[AV1_TOTAL_REFS_PER_FRAME][6];

    AV1RawFilmGrainParams film_grain;
} AV1RawFrameHeader;

typedef struct AV1RawTileData {
    RK_U8     *data;
    size_t       data_size;
} AV1RawTileData;

typedef struct AV1RawTileGroup {
    RK_U8  tile_start_and_end_present_flag;
    RK_U8 tg_start;
    RK_U8 tg_end;

    AV1RawTileData tile_data;
} AV1RawTileGroup;

typedef struct AV1RawFrame {
    AV1RawFrameHeader header;
    AV1RawTileGroup   tile_group;
} AV1RawFrame;

typedef struct AV1RawTileList {
    RK_U8 output_frame_width_in_tiles_minus_1;
    RK_U8 output_frame_height_in_tiles_minus_1;
    RK_U8 tile_count_minus_1;

    AV1RawTileData tile_data;
} AV1RawTileList;

typedef struct AV1RawMetadataHDRCLL {
    RK_U8 max_cll;
    RK_U8 max_fall;
} AV1RawMetadataHDRCLL;

typedef struct AV1RawMetadataHDRMDCV {
    RK_U8 primary_chromaticity_x[3];
    RK_U8 primary_chromaticity_y[3];
    RK_U8 white_point_chromaticity_x;
    RK_U8 white_point_chromaticity_y;
    RK_U32 luminance_max;
    RK_U32 luminance_min;
} AV1RawMetadataHDRMDCV;

typedef struct AV1RawMetadataScalability {
    RK_U8 scalability_mode_idc;
    RK_U8 spatial_layers_cnt_minus_1;
    RK_U8 spatial_layer_dimensions_present_flag;
    RK_U8 spatial_layer_description_present_flag;
    RK_U8 temporal_group_description_present_flag;
    RK_U8 scalability_structure_reserved_3bits;
    RK_U8 spatial_layer_max_width[4];
    RK_U8 spatial_layer_max_height[4];
    RK_U8 spatial_layer_ref_id[4];
    RK_U8 temporal_group_size;
    RK_U8 temporal_group_temporal_id[255];
    RK_U8 temporal_group_temporal_switching_up_point_flag[255];
    RK_U8 temporal_group_spatial_switching_up_point_flag[255];
    RK_U8 temporal_group_ref_cnt[255];
    RK_U8 temporal_group_ref_pic_diff[255][7];
} AV1RawMetadataScalability;

typedef struct AV1RawMetadataITUTT35 {
    RK_U8 itu_t_t35_country_code;
    RK_U8 itu_t_t35_country_code_extension_byte;

    RK_U8     *payload;
    size_t     payload_size;
} AV1RawMetadataITUTT35;

typedef struct AV1RawMetadataTimecode {
    RK_U8  counting_type;
    RK_U8  full_timestamp_flag;
    RK_U8  discontinuity_flag;
    RK_U8  cnt_dropped_flag;
    RK_U8  n_frames;
    RK_U8  seconds_value;
    RK_U8  minutes_value;
    RK_U8  hours_value;
    RK_U8  seconds_flag;
    RK_U8  minutes_flag;
    RK_U8  hours_flag;
    RK_U8  time_offset_length;
    RK_U32 time_offset_value;
} AV1RawMetadataTimecode;

typedef struct AV1RawMetadata {
    RK_U64 metadata_type;
    union {
        AV1RawMetadataHDRCLL      hdr_cll;
        AV1RawMetadataHDRMDCV     hdr_mdcv;
        AV1RawMetadataScalability scalability;
        AV1RawMetadataITUTT35     itut_t35;
        AV1RawMetadataTimecode    timecode;
    } metadata;
} AV1RawMetadata;

typedef struct AV1RawPadding {
    RK_U8     *payload;
    size_t       payload_size;
} AV1RawPadding;


typedef struct AV1RawOBU {
    AV1RawOBUHeader header;

    size_t obu_size;

    union {
        AV1RawSequenceHeader sequence_header;
        AV1RawFrameHeader    frame_header;
        AV1RawFrame          frame;
        AV1RawTileGroup      tile_group;
        AV1RawTileList       tile_list;
        AV1RawMetadata       metadata;
        AV1RawPadding        padding;
    } obu;
} AV1RawOBU;

typedef struct AV1ReferenceFrameState {
    RK_S32 valid;          // RefValid
    RK_S32 frame_id;       // RefFrameId
    RK_S32 upscaled_width; // RefUpscaledWidth
    RK_S32 frame_width;    // RefFrameWidth
    RK_S32 frame_height;   // RefFrameHeight
    RK_S32 render_width;   // RefRenderWidth
    RK_S32 render_height;  // RefRenderHeight
    RK_S32 frame_type;     // RefFrameType
    RK_S32 subsampling_x;  // RefSubsamplingX
    RK_S32 subsampling_y;  // RefSubsamplingY
    RK_S32 bit_depth;      // RefBitDepth
    RK_S32 order_hint;     // RefOrderHint

    RK_S8  loop_filter_ref_deltas[AV1_TOTAL_REFS_PER_FRAME];
    RK_S8  loop_filter_mode_deltas[2];
    RK_U8 feature_enabled[AV1_MAX_SEGMENTS][AV1_SEG_LVL_MAX];
    RK_S16 feature_value[AV1_MAX_SEGMENTS][AV1_SEG_LVL_MAX];
} AV1ReferenceFrameState;


typedef RK_U32 Av1UnitType;

typedef struct Av1ObuUnit_t {
    /**
     * Codec-specific type of this unit.
     */
    Av1UnitType type;

    /**
     * Pointer to the directly-parsable bitstream form of this unit.
     *
     * May be NULL if the unit currently only exists in decomposed form.
     */
    RK_U8 *data;

    size_t   data_size;

    void     *content;
} Av1ObuUnit;

typedef struct Av1UnitFragment_t {
    RK_U8    *data;
    size_t     data_size;
    int        nb_units;
    int        nb_units_allocated;
    Av1ObuUnit *units;
} Av1UnitFragment;

#endif //__AV1D_CBS_H__
