/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef AV1D_SYNTAX_H
#define AV1D_SYNTAX_H

#include "av1d_common.h"

typedef struct DXVA_PicEntry_AV1_t {
    union {
        struct {
            RK_U8 Index7Bits     : 7;
            RK_U8 AssociatedFlag : 1;
        };
        RK_U8 bPicEntry;
    };
} DXVA_PicEntry_AV1, *LPDXVA_PicEntry_AV1;

typedef struct DXVA_PicParams_AV1_t {
    DXVA_PicEntry_AV1 CurrPic;
    RK_U16 width               ;
    RK_U16 height              ;
    RK_U16 max_width           ;
    RK_U16 max_height          ;
    RK_U16 CurrPicTextureIndex ;
    RK_U16 use_superres        ;
    RK_U16 superres_denom      ;
    RK_U16 bitdepth            ;
    RK_U16 seq_profile         ;
    RK_U16 frame_header_size   ;
    union {
        struct {
            RK_U32 current_operating_point      : 12;
            RK_U32 use_128x128_superblock       : 1;
            RK_U32 intra_edge_filter            : 1;
            RK_U32 interintra_compound          : 1;
            RK_U32 masked_compound              : 1;
            RK_U32 warped_motion                : 1;
            RK_U32 dual_filter                  : 1;
            RK_U32 jnt_comp                     : 1;
            RK_U32 screen_content_tools         : 1;
            RK_U32 integer_mv                   : 2;
            RK_U32 cdef_en                      : 1;
            RK_U32 restoration                  : 1;
            RK_U32 film_grain_en                : 1;
            RK_U32 intrabc                      : 1;
            RK_U32 high_precision_mv            : 1;
            RK_U32 switchable_motion_mode       : 1;
            RK_U32 filter_intra                 : 1;
            RK_U32 disable_frame_end_update_cdf : 1;
            RK_U32 disable_cdf_update           : 1;
            RK_U32 reference_mode               : 1;
            RK_U32 skip_mode                    : 1;
            RK_U32 reduced_tx_set               : 1;
            RK_U32 superres                     : 1;
            RK_U32 tx_mode                      : 3;
            RK_U32 use_ref_frame_mvs            : 1;
            RK_U32 enable_ref_frame_mvs         : 1;
            RK_U32 reference_frame_update       : 1;
            RK_U32 error_resilient_mode         : 1;
        } coding;
    };

    struct {
        RK_U16 cols;
        RK_U16 rows;
        RK_U16 context_update_id;
        RK_U16 widths[64];
        RK_U16 heights[64];
        RK_U32 tile_offset_start[128];
        RK_U32 tile_offset_end[128];
        RK_U8 tile_sz_mag;
    } tiles;

    struct {
        RK_U8 frame_type    ;
        RK_U8 show_frame    ;
        RK_U8 showable_frame;
        RK_U8 subsampling_x ;
        RK_U8 subsampling_y ;
        RK_U8 mono_chrome   ;
    } format;

    RK_U8 primary_ref_frame;
    RK_U8 enable_order_hint;
    RK_U8 order_hint;
    RK_U8 order_hint_bits;

    struct {
        RK_U8 filter_level[2]              ;
        RK_U8 filter_level_u               ;
        RK_U8 filter_level_v               ;
        RK_U8 sharpness_level              ;
        RK_U8 mode_ref_delta_enabled       ;
        RK_U8 mode_ref_delta_update        ;
        RK_U8 delta_lf_multi               ;
        RK_U8 delta_lf_present             ;
        RK_U8 delta_lf_res                 ;
        RK_S8 ref_deltas[8]                ;
        RK_S8 mode_deltas[2]               ;
        RK_U8 frame_restoration_type[3]    ;
        RK_U8 log2_restoration_unit_size[3];
    } loop_filter;

    struct {
        RK_U8 delta_q_present;
        RK_U8 delta_q_res    ;
        RK_U8 base_qindex    ;
        RK_S8 y_dc_delta_q   ;
        RK_S8 u_dc_delta_q   ;
        RK_S8 v_dc_delta_q   ;
        RK_S8 u_ac_delta_q   ;
        RK_S8 v_ac_delta_q   ;
        RK_S8 using_qmatrix  ;
        RK_U8 qm_y           ;
        RK_U8 qm_u           ;
        RK_U8 qm_v           ;
    } quantization;

    struct {
        RK_U8 damping;
        RK_U8 bits;

        struct {
            RK_U8 primary;
            RK_U8 secondary;
        } y_strengths[8];
        struct {
            RK_U8 primary;
            RK_U8 secondary;
        } uv_strengths[8];
    } cdef;

    struct {
        RK_U8 enabled                   ;
        RK_U8 update_map                ;
        RK_U8 update_data               ;
        RK_U8 temporal_update           ;
        RK_U8 feature_mask[8]           ;
        RK_S32 feature_data[8][8]       ;
        RK_U8 last_active               ;
        RK_U8 preskip                   ;
    } segmentation;

    struct {
        RK_U8 apply_grain               ;
        RK_U8 scaling_shift_minus8      ;
        RK_U8 chroma_scaling_from_luma  ;
        RK_U8 ar_coeff_lag              ;
        RK_U8 ar_coeff_shift_minus6     ;
        RK_U8 grain_scale_shift         ;
        RK_U8 overlap_flag              ;
        RK_U8 clip_to_restricted_range  ;
        RK_U8 matrix_coefficients       ;
        RK_U8 matrix_coeff_is_identity  ;
        RK_U8 num_y_points              ;
        RK_U8 num_cb_points             ;
        RK_U8 num_cr_points             ;
        RK_U8 scaling_points_y[14][2]   ;
        RK_U8 scaling_points_cb[10][2]  ;
        RK_U8 scaling_points_cr[10][2]  ;
        RK_U8 ar_coeffs_y[24]           ;
        RK_U8 ar_coeffs_cb[25]          ;
        RK_U8 ar_coeffs_cr[25]          ;
        RK_U8 cb_mult                   ;
        RK_U8 cb_luma_mult              ;
        RK_U8 cr_mult                   ;
        RK_U8 cr_luma_mult              ;

        RK_U16 grain_seed               ;
        RK_U16 update_grain             ;
        RK_U16 cb_offset                ;
        RK_U16 cr_offset                ;
    } film_grain;

    RK_U32 ref_frame_valued;
    RK_U32 ref_frame_idx[7];

    RK_U32 ref_order_hint[8];
    struct {
        RK_U32 width;
        RK_U32 height;
        RK_U32 order_hint;
        RK_U32 lst_frame_offset;
        RK_U32 lst2_frame_offset;
        RK_U32 lst3_frame_offset;
        RK_U32 gld_frame_offset;
        RK_U32 bwd_frame_offset;
        RK_U32 alt2_frame_offset;
        RK_U32 alt_frame_offset;
        RK_U32 is_intra_frame;
        RK_U32 intra_only;
        RK_S8 Index;
        RK_U8 wminvalid;
        RK_U8 wmtype;
        RK_S32 wmmat[6];
        RK_S32 wmmat_val[6];
        RK_U16 alpha, beta, gamma, delta;
    } frame_refs[8];

    struct {
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
    } frame_ref_state[8];

    RK_U8 ref_frame_sign_bias[8];

    RK_U8 coded_lossless;
    RK_S32 all_lossless;
    RK_U8 interp_filter;
    RK_U8 RefFrameMapTextureIndex[8];
    RK_U32 upscaled_width;
    RK_U32 frame_to_show_map_idx;
    RK_U32 show_existing_frame;
    RK_U32 frame_tag_size;
    RK_U32 offset_to_dct_parts;
    RK_U8 skip_ref0;
    RK_U8 skip_ref1;
    RK_U8 refresh_frame_flags;
    void *cdfs;
    void *cdfs_ndvc;
    RK_U8 tile_cols_log2;
    RK_U8 tile_rows_log2;
} DXVA_PicParams_AV1, *LPDXVA_PicParams_AV1;

typedef struct DXVA_Slice_AV1_Short_t {
    RK_U32 BSNALunitDataLocation;
    RK_U32 SliceByteInBuffer;
    RK_U16 wBadSliceChopping;
} DXVA_Slice_AV1_Short, *LPDXVA_Slice_AV1_Short;

#endif
