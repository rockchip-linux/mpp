/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef VP9D_SYNTAX_H
#define VP9D_SYNTAX_H

#include "rk_type.h"

typedef struct DXVA_PicEntry_VPx_t {
    union {
        struct {
            RK_U8 Index7Bits     : 7;
            RK_U8 AssociatedFlag : 1;
        };
        RK_U8 bPicEntry;
    };
} DXVA_PicEntry_VPx, *LPDXVA_PicEntry_Vpx;

typedef struct DXVA_segmentation_VP9_t {
    union {
        struct {
            RK_U8 enabled : 1;
            RK_U8 update_map : 1;
            RK_U8 temporal_update : 1;
            RK_U8 abs_delta : 1;
            RK_U8 ReservedSegmentFlags4Bits : 4;
        };
        RK_U8 wSegmentInfoFlags;
    };
    RK_U8 tree_probs[7];
    RK_U8 pred_probs[3];
    RK_S16 feature_data[8][4];
    RK_U8 feature_mask[8];
} DXVA_segmentation_VP9;

typedef struct {
    RK_U8 y_mode[4][9];
    RK_U8 uv_mode[10][9];
    RK_U8 filter[4][2];
    RK_U8 mv_mode[7][3];
    RK_U8 intra[4];
    RK_U8 comp[5];
    RK_U8 single_ref[5][2];
    RK_U8 comp_ref[5];
    RK_U8 tx32p[2][3];
    RK_U8 tx16p[2][2];
    RK_U8 tx8p[2];
    RK_U8 skip[3];
    RK_U8 mv_joint[3];
    struct {
        RK_U8 sign;
        RK_U8 classes[10];
        RK_U8 class0;
        RK_U8 bits[10];
        RK_U8 class0_fp[2][3];
        RK_U8 fp[3];
        RK_U8 class0_hp;
        RK_U8 hp;
    } mv_comp[2];
    RK_U8 partition[4][4][3];
} DXVA_prob_vp9;

typedef struct DXVA_PicParams_VP9_t {
    DXVA_PicEntry_VPx CurrPic;
    RK_U8 profile;
    union {
        struct {
            RK_U16 frame_type : 1;
            RK_U16 show_frame : 1;
            RK_U16 error_resilient_mode : 1;
            RK_U16 subsampling_x : 1;
            RK_U16 subsampling_y : 1;
            RK_U16 extra_plane : 1;
            RK_U16 refresh_frame_context : 1;
            RK_U16 intra_only : 1;
            RK_U16 frame_context_idx : 2;
            RK_U16 reset_frame_context : 2;
            RK_U16 allow_high_precision_mv : 1;
            RK_U16 parallelmode            : 1;
            RK_U16 show_existing_frame : 1;
        };
        RK_U16 wFormatAndPictureInfoFlags;
    };
    RK_U32 width;
    RK_U32 height;
    RK_U8 BitDepthMinus8Luma;
    RK_U8 BitDepthMinus8Chroma;
    RK_U8 interp_filter;
    RK_U8 Reserved8Bits;
    DXVA_PicEntry_VPx ref_frame_map[8];
    RK_U32 ref_frame_coded_width[8];
    RK_U32 ref_frame_coded_height[8];
    DXVA_PicEntry_VPx frame_refs[3];
    RK_S8 ref_frame_sign_bias[4];
    RK_S8 filter_level;
    RK_S8 sharpness_level;
    union {
        struct {
            RK_U8 mode_ref_delta_enabled : 1;
            RK_U8 mode_ref_delta_update : 1;
            RK_U8 use_prev_in_find_mv_refs : 1;
            RK_U8 ReservedControlInfo5Bits : 5;
        };
        RK_U8 wControlInfoFlags;
    };
    RK_S8 ref_deltas[4];
    RK_S8 mode_deltas[2];
    RK_S16 base_qindex;
    RK_S8 y_dc_delta_q;
    RK_S8 uv_dc_delta_q;
    RK_S8 uv_ac_delta_q;
    DXVA_segmentation_VP9 stVP9Segments;
    RK_U8 log2_tile_cols;
    RK_U8 log2_tile_rows;
    RK_U16 uncompressed_header_size_byte_aligned;
    RK_U16 first_partition_size;
    RK_U16 Reserved16Bits;
    RK_U16 Reserved32Bits;
    RK_U32 StatusReportFeedbackNumber;
    struct {
        RK_U8 y_mode[4][9];
        RK_U8 uv_mode[10][9];
        RK_U8 filter[4][2];
        RK_U8 mv_mode[7][3];
        RK_U8 intra[4];
        RK_U8 comp[5];
        RK_U8 single_ref[5][2];
        RK_U8 comp_ref[5];
        RK_U8 tx32p[2][3];
        RK_U8 tx16p[2][2];
        RK_U8 tx8p[2];
        RK_U8 skip[3];
        RK_U8 mv_joint[3];
        struct {
            RK_U8 sign;
            RK_U8 classes[10];
            RK_U8 class0;
            RK_U8 bits[10];
            RK_U8 class0_fp[2][3];
            RK_U8 fp[3];
            RK_U8 class0_hp;
            RK_U8 hp;
        } mv_comp[2];
        RK_U8 partition[4][4][3];
        RK_U8 coef[4][2][2][6][6][3];
    } prob;
    struct {
        RK_U32 partition[4][4][4];
        RK_U32 skip[3][2];
        RK_U32 intra[4][2];
        RK_U32 tx32p[2][4];
        RK_U32 tx16p[2][4];
        RK_U32 tx8p[2][2];
        RK_U32 y_mode[4][10];
        RK_U32 uv_mode[10][10];
        RK_U32 comp[5][2];
        RK_U32 comp_ref[5][2];
        RK_U32 single_ref[5][2][2];
        RK_U32 mv_mode[7][4];
        RK_U32 filter[4][3];
        RK_U32 mv_joint[4];
        RK_U32 sign[2][2];
        RK_U32 classes[2][12]; // orign classes[12]
        RK_U32 class0[2][2];
        RK_U32 bits[2][10][2];
        RK_U32 class0_fp[2][2][4];
        RK_U32 fp[2][4];
        RK_U32 class0_hp[2][2];
        RK_U32 hp[2][2];
        RK_U32 coef[4][2][2][6][6][3];
        RK_U32 eob[4][2][2][6][6][2];
    } counts;
    struct {
        DXVA_prob_vp9 p_flag;
        DXVA_prob_vp9 p_delta;
        RK_U8 coef_flag[4][2][2][6][6][3];
        RK_U8 coef_delta[4][2][2][6][6][3];
    } prob_flag_delta;
    RK_U16 mvscale[3][2];
    RK_S8 txmode;
    RK_S8 refmode;
} DXVA_PicParams_VP9, *LPDXVA_PicParams_VP9;

typedef struct DXVA_Slice_VPx_Short_t {
    RK_U32 BSNALunitDataLocation;
    RK_U32 SliceByteInBuffer;
    RK_U16 wBadSliceChopping;
} DXVA_Slice_VPx_Short, *LPDXVA_Slice_VPx_Short;

#endif
