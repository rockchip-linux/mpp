/*
 *
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
#ifndef _VP9D_SYNTAX_H_
#define _VP9D_SYNTAX_H_

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

typedef struct _DXVA_PicEntry_VPx {
    union {
        struct {
            UCHAR Index7Bits     : 7;
            UCHAR AssociatedFlag : 1;
        };
        UCHAR bPicEntry;
    };
} DXVA_PicEntry_VPx, *LPDXVA_PicEntry_Vpx;

typedef struct _segmentation_VP9 {
    union {
        struct {
            UCHAR enabled : 1;
            UCHAR update_map : 1;
            UCHAR temporal_update : 1;
            UCHAR abs_delta : 1;
            UCHAR ReservedSegmentFlags4Bits : 4;
        };
        UCHAR wSegmentInfoFlags;
    };
    UCHAR tree_probs[7];
    UCHAR pred_probs[3];
    SHORT feature_data[8][4];
    UCHAR feature_mask[8];
} DXVA_segmentation_VP9;

typedef struct _DXVA_PicParams_VP9 {
    DXVA_PicEntry_VPx CurrPic;
    UCHAR profile;
    union {
        struct {
            USHORT frame_type : 1;
            USHORT show_frame : 1;
            USHORT error_resilient_mode : 1;
            USHORT subsampling_x : 1;
            USHORT subsampling_y : 1;
            USHORT extra_plane : 1;
            USHORT refresh_frame_context : 1;
            USHORT intra_only : 1;
            USHORT frame_context_idx : 2;
            USHORT reset_frame_context : 2;
            USHORT allow_high_precision_mv : 1;
            USHORT parallelmode            : 1;
            USHORT ReservedFormatInfo2Bits : 1;
        };
        USHORT wFormatAndPictureInfoFlags;
    };
    UINT width;
    UINT height;
    UCHAR BitDepthMinus8Luma;
    UCHAR BitDepthMinus8Chroma;
    UCHAR interp_filter;
    UCHAR Reserved8Bits;
    DXVA_PicEntry_VPx ref_frame_map[8];
    UINT ref_frame_coded_width[8];
    UINT ref_frame_coded_height[8];
    DXVA_PicEntry_VPx frame_refs[3];
    CHAR ref_frame_sign_bias[4];
    CHAR filter_level;
    CHAR sharpness_level;
    union {
        struct {
            UCHAR mode_ref_delta_enabled : 1;
            UCHAR mode_ref_delta_update : 1;
            UCHAR use_prev_in_find_mv_refs : 1;
            UCHAR ReservedControlInfo5Bits : 5;
        };
        UCHAR wControlInfoFlags;
    };
    CHAR ref_deltas[4];
    CHAR mode_deltas[2];
    SHORT base_qindex;
    CHAR y_dc_delta_q;
    CHAR uv_dc_delta_q;
    CHAR uv_ac_delta_q;
    DXVA_segmentation_VP9 stVP9Segments;
    UCHAR log2_tile_cols;
    UCHAR log2_tile_rows;
    USHORT uncompressed_header_size_byte_aligned;
    USHORT first_partition_size;
    USHORT Reserved16Bits;
    USHORT Reserved32Bits;
    UINT StatusReportFeedbackNumber;
    struct {
        UCHAR y_mode[4][9];
        UCHAR uv_mode[10][9];
        UCHAR filter[4][2];
        UCHAR mv_mode[7][3];
        UCHAR intra[4];
        UCHAR comp[5];
        UCHAR single_ref[5][2];
        UCHAR comp_ref[5];
        UCHAR tx32p[2][3];
        UCHAR tx16p[2][2];
        UCHAR tx8p[2];
        UCHAR skip[3];
        UCHAR mv_joint[3];
        struct {
            UCHAR sign;
            UCHAR classes[10];
            UCHAR class0;
            UCHAR bits[10];
            UCHAR class0_fp[2][3];
            UCHAR fp[3];
            UCHAR class0_hp;
            UCHAR hp;
        } mv_comp[2];
        UCHAR partition[4][4][3];
        UCHAR coef[4][2][2][6][6][11];
    } prob;
    struct {
        UINT partition[4][4][4];
        UINT skip[3][2];
        UINT intra[4][2];
        UINT tx32p[2][4];
        UINT tx16p[2][4];
        UINT tx8p[2][2];
        UINT y_mode[4][10];
        UINT uv_mode[10][10];
        UINT comp[5][2];
        UINT comp_ref[5][2];
        UINT single_ref[5][2][2];
        UINT mv_mode[7][4];
        UINT filter[4][3];
        UINT mv_joint[4];
        UINT sign[2][2];
        UINT classes[2][12]; // orign classes[12]
        UINT class0[2][2];
        UINT bits[2][10][2];
        UINT class0_fp[2][2][4];
        UINT fp[2][4];
        UINT class0_hp[2][2];
        UINT hp[2][2];
        UINT coef[4][2][2][6][6][3];
        UINT eob[4][2][2][6][6][2];
    } counts;
    USHORT mvscale[3][2];
    CHAR txmode;
    CHAR refmode;
} DXVA_PicParams_VP9, *LPDXVA_PicParams_VP9;

typedef struct _DXVA_Slice_VPx_Short {
    UINT BSNALunitDataLocation;
    UINT SliceByteInBuffer;
    USHORT wBadSliceChopping;
} DXVA_Slice_VPx_Short, *LPDXVA_Slice_VPx_Short;

#endif

