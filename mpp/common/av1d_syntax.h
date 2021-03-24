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

#ifndef _AV1D_SYNTAX_H_
#define _AV1D_SYNTAX_H_

typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       ULONG;
typedef unsigned short      USHORT;
typedef unsigned char       UCHAR;
typedef unsigned int        UINT;
typedef unsigned int        UINT32;

typedef signed   int        BOOL;
typedef signed   int        INT;
typedef signed   char       CHAR;
typedef signed   short      SHORT;
typedef signed   long       LONG;
typedef void               *PVOID;

typedef struct _DXVA_PicEntry_AV1 {
    union {
        struct {
            UCHAR Index7Bits     : 7;
            UCHAR AssociatedFlag : 1;
        };
        UCHAR bPicEntry;
    };
} DXVA_PicEntry_AV1, *LPDXVA_PicEntry_AV1;


typedef struct _DXVA_PicParams_AV1 {
    DXVA_PicEntry_AV1 CurrPic;
    USHORT width               ;
    USHORT height              ;
    USHORT max_width           ;
    USHORT max_height          ;
    USHORT CurrPicTextureIndex ;
    USHORT superres_denom      ;
    USHORT bitdepth            ;
    USHORT seq_profile         ;
    union {
        struct {
            UINT32 use_128x128_superblock       : 1;
            UINT32 intra_edge_filter            : 1;
            UINT32 interintra_compound          : 1;
            UINT32 masked_compound              : 1;
            UINT32 warped_motion                : 1;
            UINT32 dual_filter                  : 1;
            UINT32 jnt_comp                     : 1;
            UINT32 screen_content_tools         : 1;
            UINT32 integer_mv                   : 2;
            UINT32 cdef_en                      : 1;
            UINT32 restoration                  : 1;
            UINT32 film_grain_en                : 1;
            UINT32 intrabc                      : 1;
            UINT32 high_precision_mv            : 1;
            UINT32 switchable_motion_mode       : 1;
            UINT32 filter_intra                 : 1;
            UINT32 disable_frame_end_update_cdf : 1;
            UINT32 disable_cdf_update           : 1;
            UINT32 reference_mode               : 1;
            UINT32 skip_mode                    : 1;
            UINT32 reduced_tx_set               : 1;
            UINT32 superres                     : 1;
            UINT32 tx_mode                      : 3;
            UINT32 use_ref_frame_mvs            : 1;
            UINT32 enable_ref_frame_mvs         : 1;
            UINT32 reference_frame_update       : 1;
            UINT32 error_resilient_mode         : 1;
        } coding;
    };

    struct {
        USHORT   cols;
        USHORT   rows;
        USHORT   context_update_id;
        USHORT   widths[64];
        USHORT   heights[64];
        UINT32   tile_offset_start[128];
        UINT32   tile_offset_end[128];
        UCHAR    tile_sz_mag;
    } tiles;

    struct {
        UCHAR frame_type    ;
        UCHAR show_frame    ;
        UCHAR showable_frame;
        UCHAR subsampling_x ;
        UCHAR subsampling_y ;
        UCHAR mono_chrome   ;
    } format;

    UCHAR primary_ref_frame;
    UCHAR order_hint;
    UCHAR order_hint_bits;

    struct {
        UCHAR filter_level[2]              ;
        UCHAR filter_level_u               ;
        UCHAR filter_level_v               ;
        UCHAR sharpness_level              ;
        UCHAR mode_ref_delta_enabled       ;
        UCHAR mode_ref_delta_update        ;
        UCHAR delta_lf_multi               ;
        UCHAR delta_lf_present             ;
        UCHAR delta_lf_res                 ;
        CHAR  ref_deltas[8]                ;
        CHAR  mode_deltas[2]               ;
        UCHAR frame_restoration_type[3]    ;
        UCHAR log2_restoration_unit_size[3];
    } loop_filter;

    struct {
        UCHAR delta_q_present;
        UCHAR delta_q_res    ;
        UCHAR base_qindex    ;
        CHAR  y_dc_delta_q   ;
        CHAR  u_dc_delta_q   ;
        CHAR  v_dc_delta_q   ;
        CHAR  u_ac_delta_q   ;
        CHAR  v_ac_delta_q   ;
        UCHAR qm_y           ;
        UCHAR qm_u           ;
        UCHAR qm_v           ;
    } quantization;

    struct {
        UCHAR damping;
        UCHAR bits;

        struct {
            UCHAR primary;
            UCHAR secondary;
        } y_strengths[8];
        struct {
            UCHAR primary;
            UCHAR secondary;
        } uv_strengths[8];
    } cdef;

    struct {
        UCHAR  enabled           ;
        UCHAR  update_map        ;
        UCHAR  update_data       ;
        UCHAR  temporal_update   ;
        UCHAR  feature_mask[8]   ;
        INT    feature_data[8][8];
    } segmentation;

    struct {
        UCHAR apply_grain              ;
        UCHAR scaling_shift_minus8     ;
        UCHAR chroma_scaling_from_luma ;
        UCHAR ar_coeff_lag             ;
        UCHAR ar_coeff_shift_minus6    ;
        UCHAR grain_scale_shift        ;
        UCHAR overlap_flag             ;
        UCHAR clip_to_restricted_range ;
        UCHAR matrix_coeff_is_identity ;
        UCHAR num_y_points             ;
        UCHAR num_cb_points            ;
        UCHAR num_cr_points            ;
        UCHAR scaling_points_y[14][2]  ;
        UCHAR scaling_points_cb[10][2] ;
        UCHAR scaling_points_cr[10][2] ;
        UCHAR ar_coeffs_y[24]          ;
        UCHAR ar_coeffs_cb[25]         ;
        UCHAR ar_coeffs_cr[25]         ;
        UCHAR cb_mult                  ;
        UCHAR cb_luma_mult             ;
        UCHAR cr_mult                  ;
        UCHAR cr_luma_mult             ;

        USHORT grain_seed              ;
        USHORT cb_offset               ;
        USHORT cr_offset               ;
    } film_grain;

    struct {
        UINT32  width;
        UINT32  height;
        UINT32  order_hint;
        UINT32  lst_frame_offset;
        UINT32  lst2_frame_offset;
        UINT32  lst3_frame_offset;
        UINT32  gld_frame_offset;
        UINT32  bwd_frame_offset;
        UINT32  alt2_frame_offset;
        UINT32  alt_frame_offset;
        UINT32  is_intra_frame;
        UINT32  intra_only;
        CHAR    Index;
        UCHAR   wminvalid;
        UCHAR   wmtype;
        RK_S32  wmmat[6];
        USHORT  alpha, beta, gamma, delta;
    } frame_refs[7];

    UCHAR coded_lossless;
    UCHAR interp_filter;
    UCHAR RefFrameMapTextureIndex[7];
    UINT32 upscaled_width;
    UINT32 frame_to_show_map_idx;
    UINT32 frame_tag_size;
    UINT32 offset_to_dct_parts;
    UCHAR  skip_ref0;
    UCHAR  skip_ref1;
    RK_U8 refresh_frame_flags;
    void         *cdfs;
    void          *cdfs_ndvc;
    RK_U8 tile_cols_log2;
} DXVA_PicParams_AV1, *LPDXVA_PicParams_AV1;

typedef struct _DXVA_Slice_AV1_Short {
    UINT BSNALunitDataLocation;
    UINT SliceByteInBuffer;
    USHORT wBadSliceChopping;
} DXVA_Slice_AV1_Short, *LPDXVA_Slice_AV1_Short;

#endif
