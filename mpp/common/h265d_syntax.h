/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_SYNTAX_H
#define H265D_SYNTAX_H

#include "rk_type.h"

/* HEVC Picture Entry structure */
#define MAX_SLICES 600

typedef struct DXVA_PicEntry_HEVC_t {
    union {
        struct {
            RK_U8 Index7Bits : 7;
            RK_U8 AssociatedFlag : 1;
        };
        RK_U8 bPicEntry;
    };
} DXVA_PicEntry_HEVC, *LPDXVA_PicEntry_HEVC;

typedef struct Short_SPS_RPS_HEVC_t {
    RK_U8 num_negative_pics;
    RK_U8 num_positive_pics;
    RK_S16 delta_poc_s0[16];
    RK_U8 s0_used_flag[16];
    RK_S16 delta_poc_s1[16];
    RK_U8  s1_used_flag[16];
} Short_SPS_RPS_HEVC;

typedef struct LT_SPS_RPS_HEVC_t {
    RK_U16 lt_ref_pic_poc_lsb;
    RK_U8  used_by_curr_pic_lt_flag;
} LT_SPS_RPS_HEVC;

/* HEVC Picture Parameter structure */
typedef struct DXVA_PicParams_HEVC_t {
    RK_U16      PicWidthInMinCbsY;
    RK_U16      PicHeightInMinCbsY;
    union {
        struct {
            RK_U16  chroma_format_idc                       : 2;
            RK_U16  separate_colour_plane_flag              : 1;
            RK_U16  bit_depth_luma_minus8                   : 3;
            RK_U16  bit_depth_chroma_minus8                 : 3;
            RK_U16  log2_max_pic_order_cnt_lsb_minus4       : 4;
            RK_U16  NoPicReorderingFlag                     : 1;
            RK_U16  NoBiPredFlag                            : 1;
            RK_U16  ReservedBits1                           : 1;
        };
        RK_U16 wFormatAndSequenceInfoFlags;
    };
    DXVA_PicEntry_HEVC  CurrPic;
    RK_U8   sps_max_dec_pic_buffering_minus1;
    RK_U8   log2_min_luma_coding_block_size_minus3;
    RK_U8   log2_diff_max_min_luma_coding_block_size;
    RK_U8   log2_min_transform_block_size_minus2;
    RK_U8   log2_diff_max_min_transform_block_size;
    RK_U8   max_transform_hierarchy_depth_inter;
    RK_U8   max_transform_hierarchy_depth_intra;
    RK_U8   num_short_term_ref_pic_sets;
    RK_U8   num_long_term_ref_pics_sps;
    RK_U8   num_ref_idx_l0_default_active_minus1;
    RK_U8   num_ref_idx_l1_default_active_minus1;
    RK_S8    init_qp_minus26;
    RK_U8   ucNumDeltaPocsOfRefRpsIdx;
    RK_U16  wNumBitsForShortTermRPSInSlice;
    RK_U16  ReservedBits2;

    union {
        struct {
            RK_U32  scaling_list_enabled_flag                    : 1;
            RK_U32  amp_enabled_flag                             : 1;
            RK_U32  sample_adaptive_offset_enabled_flag          : 1;
            RK_U32  pcm_enabled_flag                             : 1;
            RK_U32  pcm_sample_bit_depth_luma_minus1             : 4;
            RK_U32  pcm_sample_bit_depth_chroma_minus1           : 4;
            RK_U32  log2_min_pcm_luma_coding_block_size_minus3   : 2;
            RK_U32  log2_diff_max_min_pcm_luma_coding_block_size : 2;
            RK_U32  pcm_loop_filter_disabled_flag                : 1;
            RK_U32  long_term_ref_pics_present_flag              : 1;
            RK_U32  sps_temporal_mvp_enabled_flag                : 1;
            RK_U32  strong_intra_smoothing_enabled_flag          : 1;
            RK_U32  dependent_slice_segments_enabled_flag        : 1;
            RK_U32  output_flag_present_flag                     : 1;
            RK_U32  num_extra_slice_header_bits                  : 3;
            RK_U32  sign_data_hiding_enabled_flag                : 1;
            RK_U32  cabac_init_present_flag                      : 1;
            RK_U32  ReservedBits3                                : 5;
        };
        RK_U32 dwCodingParamToolFlags;
    };

    union {
        struct {
            RK_U32  constrained_intra_pred_flag                 : 1;
            RK_U32  transform_skip_enabled_flag                 : 1;
            RK_U32  cu_qp_delta_enabled_flag                    : 1;
            RK_U32  pps_slice_chroma_qp_offsets_present_flag    : 1;
            RK_U32  weighted_pred_flag                          : 1;
            RK_U32  weighted_bipred_flag                        : 1;
            RK_U32  transquant_bypass_enabled_flag              : 1;
            RK_U32  tiles_enabled_flag                          : 1;
            RK_U32  entropy_coding_sync_enabled_flag            : 1;
            RK_U32  uniform_spacing_flag                        : 1;
            RK_U32  loop_filter_across_tiles_enabled_flag       : 1;
            RK_U32  pps_loop_filter_across_slices_enabled_flag  : 1;
            RK_U32  deblocking_filter_override_enabled_flag     : 1;
            RK_U32  pps_deblocking_filter_disabled_flag         : 1;
            RK_U32  lists_modification_present_flag             : 1;
            RK_U32  slice_segment_header_extension_present_flag : 1;
            RK_U32  IrapPicFlag                                 : 1;
            RK_U32  IdrPicFlag                                  : 1;
            RK_U32  IntraPicFlag                                : 1;
            // sps exension flags
            RK_U32  sps_extension_flag                          : 1;
            RK_U32  sps_range_extension_flag                    : 1;
            RK_U32  transform_skip_rotation_enabled_flag        : 1;
            RK_U32  transform_skip_context_enabled_flag         : 1;
            RK_U32  implicit_rdpcm_enabled_flag                 : 1;
            RK_U32  explicit_rdpcm_enabled_flag                 : 1;
            RK_U32  extended_precision_processing_flag          : 1;
            RK_U32  intra_smoothing_disabled_flag               : 1;
            RK_U32  high_precision_offsets_enabled_flag         : 1;
            RK_U32  persistent_rice_adaptation_enabled_flag     : 1;
            RK_U32  cabac_bypass_alignment_enabled_flag         : 1;
            // pps exension flags
            RK_U32  cross_component_prediction_enabled_flag     : 1;
            RK_U32  chroma_qp_offset_list_enabled_flag          : 1;
        };
        RK_U32 dwCodingSettingPicturePropertyFlags;
    };
    RK_S8       pps_cb_qp_offset;
    RK_S8       pps_cr_qp_offset;
    RK_U8       num_tile_columns_minus1;
    RK_U8       num_tile_rows_minus1;
    RK_U16      column_width_minus1[19];
    RK_U16      row_height_minus1[21];
    RK_U8       diff_cu_qp_delta_depth;
    RK_S8       pps_beta_offset_div2;
    RK_S8       pps_tc_offset_div2;
    RK_U8       log2_parallel_merge_level_minus2;
    RK_S32      CurrPicOrderCntVal;
    DXVA_PicEntry_HEVC RefPicList[16];
    RK_U8       ReservedBits5;
    RK_S32      PicOrderCntValList[16];
    RK_U8       RefPicSetStCurrBefore[8];
    RK_U8       RefPicSetStCurrAfter[8];
    RK_U8       RefPicSetLtCurr[8];
    RK_U16      ReservedBits6;
    RK_U16      ReservedBits7;
    RK_U32      StatusReportFeedbackNumber;
    RK_U32      vps_id;
    RK_U32      pps_id;
    RK_U32      sps_id;
    RK_S32      current_poc;

    Short_SPS_RPS_HEVC cur_st_rps;
    Short_SPS_RPS_HEVC sps_st_rps[64];
    LT_SPS_RPS_HEVC    sps_lt_rps[32];

    // PPS exentison
    RK_U8   log2_max_transform_skip_block_size;
    RK_U8   diff_cu_chroma_qp_offset_depth;
    RK_S8   cb_qp_offset_list[6];
    RK_S8   cr_qp_offset_list[6];
    RK_U8   chroma_qp_offset_list_len_minus1;

    RK_U8   scaling_list_data_present_flag;
    RK_U8   ps_update_flag;
    RK_U8   rps_update_flag;
} DXVA_PicParams_HEVC, *LPDXVA_PicParams_HEVC;

/* HEVC Quantizatiuon Matrix structure */
typedef struct DXVA_Qmatrix_HEVC_t {
    RK_U8   ucScalingLists0[6][16];
    RK_U8   ucScalingLists1[6][64];
    RK_U8   ucScalingLists2[6][64];
    RK_U8   ucScalingLists3[2][64];
    RK_U8   ucScalingListDCCoefSizeID2[6];
    RK_U8   ucScalingListDCCoefSizeID3[2];
} DXVA_Qmatrix_HEVC, *LPDXVA_Qmatrix_HEVC;

/* HEVC Slice Control Structure */
typedef struct DXVA_Slice_HEVC_Short_t {
    RK_U32  BSNALunitDataLocation;
    RK_U32  SliceBytesInBuffer;
    RK_U16  wBadSliceChopping;
} DXVA_Slice_HEVC_Short, *LPDXVA_Slice_HEVC_Short;

/* just use in the case of pps->slice_header_extension_present_flag is 1 */
typedef struct DXVA_Slice_HEVC_Cut_Param_t {
    RK_U32  start_bit;
    RK_U32  end_bit;
    RK_U16  is_enable;
} DXVA_Slice_HEVC_Cut_Param, *LPDXVA_Slice_HEVC_Cut_Param;

typedef struct h265d_dxva2_picture_context {
    DXVA_PicParams_HEVC         pp;
    DXVA_Qmatrix_HEVC           qm;
    RK_U32                      slice_count;
    DXVA_Slice_HEVC_Short       *slice_short;
    const RK_U8                 *bitstream;
    RK_U32                      bitstream_size;
    DXVA_Slice_HEVC_Cut_Param   *slice_cut_param;
    RK_S32                      max_slice_num;
} h265d_dxva2_picture_context_t;

#endif /* H265D_SYNTAX_H */
