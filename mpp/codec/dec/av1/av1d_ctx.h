/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef AV1D_CODEC_H
#define AV1D_CODEC_H

#include "mpp_frame.h"

#include "mpp_bitread.h"
#include "av1d_syntax.h"

// Profiles
#define AV1_PROFILE_MAIN                0
#define AV1_PROFILE_HIGH                1
#define AV1_PROFILE_PROFESSIONAL        2

// Constants
#define AV1_MAX_OPERATING_POINTS        32
#define AV1_SELECT_SCREEN_CONTENT_TOOLS 2
#define AV1_SELECT_INTEGER_MV           2
#define AV1_SUPERRES_NUM                8
#define AV1_SUPERRES_DENOM_MIN          9

// Divisor lookup table
#define AV1_DIV_LUT_BITS                8
#define AV1_DIV_LUT_PREC_BITS           14
#define AV1_DIV_LUT_NUM                 257

// global motion params bits
#define AV1_GM_ABS_ALPHA_BITS           12
#define AV1_GM_ALPHA_PREC_BITS          15
#define AV1_GM_ABS_TRANS_ONLY_BITS      9
#define AV1_GM_TRANS_ONLY_PREC_BITS     3
#define AV1_GM_ABS_TRANS_BITS           12
#define AV1_GM_TRANS_PREC_BITS          6

// Bits of precision used for the model
#define AV1_WARPEDMODEL_PREC_BITS       16

// OBU types
enum {
    // 0 reserved.
    AV1_OBU_SEQUENCE_HEADER        = 1,
    AV1_OBU_TEMPORAL_DELIMITER     = 2,
    AV1_OBU_FRAME_HEADER           = 3,
    AV1_OBU_TILE_GROUP             = 4,
    AV1_OBU_METADATA               = 5,
    AV1_OBU_FRAME                  = 6,
    AV1_OBU_REDUNDANT_FRAME_HEADER = 7,
    AV1_OBU_TILE_LIST              = 8,
    // 9-14 reserved.
    AV1_OBU_PADDING                = 15,
} ;

// Metadata types
enum {
    AV1_METADATA_TYPE_HDR_CLL     = 1,
    AV1_METADATA_TYPE_HDR_MDCV    = 2,
    AV1_METADATA_TYPE_SCALABILITY = 3,
    AV1_METADATA_TYPE_ITUT_T35    = 4,
    AV1_METADATA_TYPE_TIMECODE    = 5,
};

// Warp models
enum {
    AV1_WARP_MODEL_IDENTITY       = 0, // identity transformation, 0-parameter
    AV1_WARP_MODEL_TRANSLATION    = 1, // translational motion 2-parameter
    AV1_WARP_MODEL_ROTZOOM        = 2, // simplified affine with rotation + zoom only, 4-parameter
    AV1_WARP_MODEL_AFFINE         = 3, // affine, 6-parameter
    AV1_WARP_PARAM_REDUCE_BITS    = 6,
};

// Chroma sample position
enum {
    AV1_CSP_UNKNOWN   = 0,
    AV1_CSP_VERTICAL  = 1,
    AV1_CSP_COLOCATED = 2,
};

// Scalability modes
enum {
    AV1_SCALABILITY_L1T2 = 0,
    AV1_SCALABILITY_L1T3 = 1,
    AV1_SCALABILITY_L2T1 = 2,
    AV1_SCALABILITY_L2T2 = 3,
    AV1_SCALABILITY_L2T3 = 4,
    AV1_SCALABILITY_S2T1 = 5,
    AV1_SCALABILITY_S2T2 = 6,
    AV1_SCALABILITY_S2T3 = 7,
    AV1_SCALABILITY_L2T1h = 8,
    AV1_SCALABILITY_L2T2h = 9,
    AV1_SCALABILITY_L2T3h = 10,
    AV1_SCALABILITY_S2T1h = 11,
    AV1_SCALABILITY_S2T2h = 12,
    AV1_SCALABILITY_S2T3h = 13,
    AV1_SCALABILITY_SS = 14,
    AV1_SCALABILITY_L3T1 = 15,
    AV1_SCALABILITY_L3T2 = 16,
    AV1_SCALABILITY_L3T3 = 17,
    AV1_SCALABILITY_S3T1 = 18,
    AV1_SCALABILITY_S3T2 = 19,
    AV1_SCALABILITY_S3T3 = 20,
    AV1_SCALABILITY_L3T2_KEY = 21,
    AV1_SCALABILITY_L3T3_KEY = 22,
    AV1_SCALABILITY_L4T5_KEY = 23,
    AV1_SCALABILITY_L4T7_KEY = 24,
    AV1_SCALABILITY_L3T2_KEY_SHIFT = 25,
    AV1_SCALABILITY_L3T3_KEY_SHIFT = 26,
    AV1_SCALABILITY_L4T5_KEY_SHIFT = 27,
    AV1_SCALABILITY_L4T7_KEY_SHIFT = 28,
};

typedef struct AV1ColorConfig_t {
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
} AV1ColorConfig;

typedef struct AV1TimingInfo_t {
    RK_U32 num_units_in_display_tick;
    RK_U32 time_scale;

    RK_U8  equal_picture_interval;
    RK_U32 num_ticks_per_picture_minus_1;
} AV1TimingInfo;

typedef struct AV1DecoderModelInfo_t {
    RK_U8  buffer_delay_length_minus_1;
    RK_U32 num_units_in_decoding_tick;
    RK_U8  buffer_removal_time_length_minus_1;
    RK_U8  frame_presentation_time_length_minus_1;
} AV1DecoderModelInfo;

typedef struct AV1SequenceHeader_t {
    RK_U8 seq_profile;
    RK_U8 still_picture;
    RK_U8 reduced_still_picture_header;

    RK_U8 timing_info_present_flag;
    RK_U8 decoder_model_info_present_flag;
    RK_U8 initial_display_delay_present_flag;
    RK_U8 operating_points_cnt_minus_1;

    AV1TimingInfo       timing_info;
    AV1DecoderModelInfo decoder_model_info;

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

    AV1ColorConfig color_config;

    RK_U8 film_grain_params_present;
} AV1SeqHeader;

typedef struct AV1FilmGrainParams_t {
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
} AV1FilmGrainParams;

typedef struct AV1FrameHeader_t {
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
    RK_U8 ref_frame_valued;
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

    RK_U16 tile_cols;
    RK_U16 tile_rows;

    RK_U8 base_q_idx;
    RK_S8 delta_q_y_dc;
    RK_U8 diff_uv_delta;
    RK_S8 delta_q_u_dc;
    RK_S8 delta_q_u_ac;
    RK_S8 delta_q_v_dc;
    RK_S8 delta_q_v_ac;
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
    RK_U8 segmentation_id_last_active;
    RK_U8 segmentation_id_preskip;

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
    RK_S8 loop_filter_ref_deltas[AV1_TOTAL_REFS_PER_FRAME];
    RK_U8 update_mode_delta[2];
    RK_S8 loop_filter_mode_deltas[2];

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

    RK_U32 gm_params[AV1_TOTAL_REFS_PER_FRAME][6];

    AV1FilmGrainParams film_grain;
} AV1FrameHeader;

typedef struct AV1TileData_t {
    RK_U8   *data;
    size_t  offset;
    size_t  data_size;
} AV1TileData;

typedef struct AV1TileGroup_t {
    RK_U8  tile_start_and_end_present_flag;
    RK_U8 tg_start;
    RK_U8 tg_end;

    AV1TileData tile_data;
} AV1TileGroup;

typedef struct AV1Frame_t {
    AV1FrameHeader header;
    AV1TileGroup   tile_group;
} AV1Frame;

typedef struct AV1TileList_t {
    RK_U8 output_frame_width_in_tiles_minus_1;
    RK_U8 output_frame_height_in_tiles_minus_1;
    RK_U8 tile_count_minus_1;

    AV1TileData tile_data;
} AV1TileList;

typedef struct AV1MetadataHDRCLL_t {
    RK_U16 max_cll;
    RK_U16 max_fall;
} AV1MetadataHDRCLL;

typedef struct AV1MetadataHDRMDCV_t {
    RK_U16 primary_chromaticity_x[3];
    RK_U16 primary_chromaticity_y[3];
    RK_U16 white_point_chromaticity_x;
    RK_U16 white_point_chromaticity_y;
    RK_U32 luminance_max;
    RK_U32 luminance_min;
} AV1MetadataHDRMDCV;

typedef struct AV1MetadataScalability_t {
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
} AV1MetadataScalability;

typedef struct AV1MetadataITUTT35_t {
    RK_U8 itu_t_t35_country_code;
    RK_U8 itu_t_t35_country_code_extension_byte;
    RK_U32 itu_t_t35_terminal_provider_code;
    RK_U32 itu_t_t35_terminal_provider_oriented_code;

    RK_U8  *payload;
    size_t  payload_size;
} AV1MetadataITUTT35;

typedef struct AV1MetadataTimecode_t {
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
} AV1MetadataTimecode;

typedef struct AV1Metadata_t {
    RK_U64 metadata_type;
    union {
        AV1MetadataHDRCLL      hdr_cll;
        AV1MetadataHDRMDCV     hdr_mdcv;
        AV1MetadataScalability scalability;
        AV1MetadataITUTT35     itut_t35;
        AV1MetadataTimecode    timecode;
    } metadata;
} AV1Metadata;

typedef struct AV1Padding_t {
    RK_U8     *payload;
    size_t    payload_size;
} AV1Padding;


typedef struct AV1OBUHeader_t {
    RK_U8 obu_forbidden_bit;
    RK_U8 obu_type;
    RK_U8 obu_extension_flag;
    RK_U8 obu_has_size_field;
    RK_U8 obu_reserved_1bit;

    RK_U8 temporal_id;
    RK_U8 spatial_id;
    RK_U8 extension_header_reserved_3bits;

    RK_U64 obu_filed_size; // when obu_has_size_field=1
} AV1OBUHeader;

typedef struct AV1OBU_t {
    AV1OBUHeader header;

    size_t obu_size;
    union {
        AV1SeqHeader      seq_header;
        AV1FrameHeader    frame_header;
        AV1Frame          frame;
        AV1TileGroup      tile_group;
        AV1TileList       tile_list;
        AV1Metadata       metadata;
        AV1Padding        padding;
    } payload;
} AV1OBU;

typedef struct AV1RefFrameState_t {
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
    RK_U8  feature_enabled[AV1_MAX_SEGMENTS][AV1_SEG_LVL_MAX];
    RK_S16 feature_value[AV1_MAX_SEGMENTS][AV1_SEG_LVL_MAX];
} AV1RefFrameState;

typedef struct Av1ObuUnit_t {
    RK_U32  type;
    RK_U8   *data;
    size_t  data_size;
    AV1OBU  obu;
} Av1ObuUnit;

typedef struct Av1UnitFragment_t {
    RK_U8      *data;
    size_t     data_size;
    RK_S32     nb_units;
    RK_S32     nb_units_allocated;
    Av1ObuUnit *units;
} Av1UnitFragment;

typedef struct AV1DRefInfo_t {
    RK_S32 ref_count;
    RK_U32 invisible;
    RK_U32 is_output;
    RK_U32 lst_frame_offset;
    RK_U32 lst2_frame_offset;
    RK_U32 lst3_frame_offset;
    RK_U32 gld_frame_offset;
    RK_U32 bwd_frame_offset;
    RK_U32 alt2_frame_offset;
    RK_U32 alt_frame_offset;
    RK_U32 is_intra_frame;
    RK_U32 intra_only;
} Av1RefInfo;

typedef struct AV1WarpedMotion_t {
    RK_U32 wmtype;
    RK_S32 wmmat[6];
    RK_S32 wmmat_val[6];
    RK_S32 alpha, beta, gamma, delta;
} AV1WarpedMotion;

typedef struct AV1FrameInfo_t {
    MppFrame f;
    RK_S32 slot_index;
    RK_S32 temporal_id;
    RK_S32 spatial_id;
    RK_U8  order_hint;
    AV1WarpedMotion gm_params[AV1_NUM_REF_FRAMES];

    RK_U8 skip_mode_frame_idx[2];
    AV1FilmGrainParams film_grain;
    RK_U8 coded_lossless;
    Av1RefInfo *ref;
} AV1FrameIInfo;

typedef struct Av1Codec_t {
    BitReadCtx_t gb;

    AV1SeqHeader *seq_header;
    AV1SeqHeader *seq_ref;
    AV1FrameHeader *frame_header;
    Av1UnitFragment current_obu;

    RK_S32 seen_frame_header;
    RK_U8  *frame_header_data;
    size_t frame_header_size;

    AV1FrameIInfo ref[AV1_NUM_REF_FRAMES];
    AV1FrameIInfo cur_frame;

    MppFrameMasteringDisplayMetadata mastering_display;
    MppFrameContentLightMetadata content_light;
    MppFrameHdrDynamicMeta *hdr_dynamic_meta;
    RK_U32 hdr_dynamic;
    RK_U32 is_hdr;

    RK_S32 temporal_id;
    RK_S32 spatial_id;
    RK_S32 operating_point_idc;

    RK_S32 bit_depth;
    RK_S32 order_hint;
    RK_S32 frame_width;
    RK_S32 frame_height;
    RK_S32 upscaled_width;
    RK_S32 render_width;
    RK_S32 render_height;

    RK_S32 num_planes;
    RK_S32 coded_lossless;
    RK_S32 all_lossless;
    RK_S32 tile_cols;
    RK_S32 tile_rows;
    RK_S32 tile_num;
    RK_S32 operating_point;
    RK_S32 extra_has_frame;
    RK_U32 frame_tag_size;
    RK_U32 fist_tile_group;
    RK_U32 tile_offset;

    AV1CDFs *cdfs;
    Av1MvCDFs  *cdfs_ndvc;
    AV1CDFs default_cdfs;
    Av1MvCDFs  default_cdfs_ndvc;
    AV1CDFs cdfs_last[AV1_NUM_REF_FRAMES];
    Av1MvCDFs  cdfs_last_ndvc[AV1_NUM_REF_FRAMES];
    RK_U8 disable_frame_end_update_cdf;
    RK_U8 frame_is_intra;
    RK_U8 refresh_frame_flags;

    const RK_U32 *unit_types;
    RK_S32 nb_unit_types;

    RK_U32 tile_offset_start[AV1_MAX_TILES];
    RK_U32 tile_offset_end[AV1_MAX_TILES];

    AV1RefFrameState ref_s[AV1_NUM_REF_FRAMES];

    RK_U8 skip_ref0;
    RK_U8 skip_ref1;

    RK_S32 eos;
    RK_S64 pts;
    RK_S64 dts;
} Av1Codec;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32  av1d_get_relative_dist(const AV1SeqHeader *seq, RK_U32 a, RK_U32 b);
MPP_RET av1d_read_obu_header(BitReadCtx_t *gb, AV1OBUHeader *hdr);
MPP_RET av1d_parser_fragment_header(Av1UnitFragment *frag);
MPP_RET av1d_get_fragment_units(Av1UnitFragment *frag);
MPP_RET av1d_read_fragment_uints(Av1Codec *ctx, Av1UnitFragment *frag);
MPP_RET av1d_fragment_reset(Av1UnitFragment *frag);

MPP_RET av1d_get_cdfs(Av1Codec *ctx, RK_U32 ref_idx);
MPP_RET av1d_store_cdfs(Av1Codec *ctx, RK_U32 refresh_frame_flags);
MPP_RET av1d_set_default_cdfs(AV1CDFs *cdfs, Av1MvCDFs *cdfs_ndvc);
MPP_RET av1d_set_default_coeff_probs(RK_U32 base_qindex, AV1CDFs *cdfs);

#ifdef  __cplusplus
}
#endif

#endif // AV1D_CODEC_H
