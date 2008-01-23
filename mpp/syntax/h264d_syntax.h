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

/*
* @file        rkv_h264d_syntax.h
* @brief
* @author      leo.ding(leo.ding@rock-chips.com)
* @version     1.0.0
* @history
*   2015.07.17 : Create
*/

#ifndef __H264D_SYNTAX_H__
#define __H264D_SYNTAX_H__

#include "dxva_syntax.h"

/* H.264/AVC-specific structures */

/* H.264/AVC picture entry data structure */
typedef struct _DXVA_PicEntry_H264 {
    union {
        struct {
            UCHAR  Index7Bits : 7;
            UCHAR  AssociatedFlag : 1;
        };
        UCHAR  bPicEntry;
    };
} DXVA_PicEntry_H264, *LPDXVA_PicEntry_H264;  /* 1 byte */

/* H.264/AVC picture parameters structure */
typedef struct _DXVA_PicParams_H264 {
    USHORT  wFrameWidthInMbsMinus1;
    USHORT  wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
    UCHAR   num_ref_frames;

    union {
        struct {
            USHORT  field_pic_flag : 1;
            USHORT  MbaffFrameFlag : 1;
            USHORT  residual_colour_transform_flag : 1;
            USHORT  sp_for_switch_flag : 1;
            USHORT  chroma_format_idc : 2;
            USHORT  RefPicFlag : 1;
            USHORT  constrained_intra_pred_flag : 1;

            USHORT  weighted_pred_flag : 1;
            USHORT  weighted_bipred_idc : 2;
            USHORT  MbsConsecutiveFlag : 1;
            USHORT  frame_mbs_only_flag : 1;
            USHORT  transform_8x8_mode_flag : 1;
            USHORT  MinLumaBipredSize8x8Flag : 1;
            USHORT  IntraPicFlag : 1;
        };
        USHORT  wBitFields;
    };
    UCHAR  bit_depth_luma_minus8;
    UCHAR  bit_depth_chroma_minus8;

    USHORT Reserved16Bits;
    UINT   StatusReportFeedbackNumber;

    DXVA_PicEntry_H264  RefFrameList[16]; /* flag LT */
    INT    CurrFieldOrderCnt[2];
    INT    FieldOrderCntList[16][2];

    CHAR   pic_init_qs_minus26;
    CHAR   chroma_qp_index_offset;   /* also used for QScb */
    CHAR   second_chroma_qp_index_offset; /* also for QScr */
    UCHAR  ContinuationFlag;

    /* remainder for parsing */
    CHAR   pic_init_qp_minus26;
    UCHAR  num_ref_idx_l0_active_minus1;
    UCHAR  num_ref_idx_l1_active_minus1;
    UCHAR  Reserved8BitsA;

    USHORT FrameNumList[16];
    UINT   UsedForReferenceFlags;
    USHORT NonExistingFrameFlags;
    USHORT frame_num;

    UCHAR  log2_max_frame_num_minus4;
    UCHAR  pic_order_cnt_type;
    UCHAR  log2_max_pic_order_cnt_lsb_minus4;
    UCHAR  delta_pic_order_always_zero_flag;

    UCHAR  direct_8x8_inference_flag;
    UCHAR  entropy_coding_mode_flag;
    UCHAR  pic_order_present_flag;
    UCHAR  num_slice_groups_minus1;

    UCHAR  slice_group_map_type;
    UCHAR  deblocking_filter_control_present_flag;
    UCHAR  redundant_pic_cnt_present_flag;
    UCHAR  Reserved8BitsB;

    USHORT slice_group_change_rate_minus1;

    UCHAR  SliceGroupMap[810]; /* 4b/sgmu, Size BT.601 */

} DXVA_PicParams_H264, *LPDXVA_PicParams_H264;

/* H.264/AVC quantization weighting matrix data structure */
typedef struct _DXVA_Qmatrix_H264 {
    UCHAR  bScalingLists4x4[6][16];
    UCHAR  bScalingLists8x8[2][64];

} DXVA_Qmatrix_H264, *LPDXVA_Qmatrix_H264;

/* H.264/AVC slice control data structure - short form */
typedef struct _DXVA_Slice_H264_Short {
    UINT   BSNALunitDataLocation; /* type 1..5 */
    UINT   SliceBytesInBuffer; /* for off-host parse */
    USHORT wBadSliceChopping;  /* for off-host parse */
} DXVA_Slice_H264_Short, *LPDXVA_Slice_H264_Short;

/* H.264/AVC picture entry data structure - long form */
typedef struct _DXVA_Slice_H264_Long {
    UINT   BSNALunitDataLocation; /* type 1..5 */
    UINT   SliceBytesInBuffer; /* for off-host parse */
    USHORT wBadSliceChopping;  /* for off-host parse */

    USHORT first_mb_in_slice;
    USHORT NumMbsForSlice;

    USHORT BitOffsetToSliceData; /* after CABAC alignment */

    UCHAR  slice_type;
    UCHAR  luma_log2_weight_denom;
    UCHAR  chroma_log2_weight_denom;
    UCHAR  num_ref_idx_l0_active_minus1;
    UCHAR  num_ref_idx_l1_active_minus1;
    CHAR   slice_alpha_c0_offset_div2;
    CHAR   slice_beta_offset_div2;
    UCHAR  Reserved8Bits;
    DXVA_PicEntry_H264 RefPicList[3][32]; /* L0 & L1 */
    SHORT  Weights[2][32][3][2]; /* L0 & L1; Y, Cb, Cr */
    CHAR   slice_qs_delta;
    /* rest off-host parse */
    CHAR   slice_qp_delta;
    UCHAR  redundant_pic_cnt;
    UCHAR  direct_spatial_mv_pred_flag;
    UCHAR  cabac_init_idc;
    UCHAR  disable_deblocking_filter_idc;
    USHORT slice_id;
} DXVA_Slice_H264_Long, *LPDXVA_Slice_H264_Long;

/* H.264/AVC macro block control command data structure */
typedef struct _DXVA_MBctrl_H264 {
    union {
        struct {
            UINT  bSliceID : 8;   /* 1 byte */
            UINT  MbType5Bits : 5;
            UINT  IntraMbFlag : 1;
            UINT  mb_field_decoding_flag : 1;
            UINT  transform_size_8x8_flag : 1;   /* 2 bytes */
            UINT  HostResidDiff : 1;
            UINT  DcBlockCodedCrFlag : 1;
            UINT  DcBlockCodedCbFlag : 1;
            UINT  DcBlockCodedYFlag : 1;
            UINT  FilterInternalEdgesFlag : 1;
            UINT  FilterLeftMbEdgeFlag : 1;
            UINT  FilterTopMbEdgeFlag : 1;
            UINT  ReservedBit : 1;
            UINT  bMvQuantity : 8;   /* 4 bytes */
        };
        UINT  dwMBtype;                    /* 4 bytes so far */
    };
    USHORT  CurrMbAddr;                  /* 6 bytes so far */
    USHORT  wPatternCode[3];/* YCbCr, 16 4x4 blks, 1b each */
    /* 12 bytes so far */
    UCHAR   bQpPrime[3];    /* Y, Cb, Cr, need just 7b QpY */
    UCHAR   bMBresidDataQuantity;
    ULONG   dwMBdataLocation;  /* offset into resid buffer */
    /* 20 bytes so far */
    union {
        struct {
            /* start here for Intra MB's  (9 useful bytes in branch) */
            USHORT LumaIntraPredModes[4];/* 16 blocks, 4b each */
            /* 28 bytes so far */
            union {
                struct {
                    UCHAR  intra_chroma_pred_mode : 2;
                    UCHAR  IntraPredAvailFlags : 5;
                    UCHAR  ReservedIntraBit : 1;
                };
                UCHAR  bMbIntraStruct;        /* 29 bytes so far */
            };
            UCHAR ReservedIntra24Bits[3];   /* 32 bytes total  */
        };
        struct {
            /* start here for non-Intra MB's (12 bytes in branch)    */
            UCHAR  bSubMbShapes;          /* 4 subMbs, 2b each */
            UCHAR  bSubMbPredModes;       /* 4 subMBs, 2b each */
            /* 22 bytes so far */
            USHORT wMvBuffOffset;     /* offset into MV buffer */
            UCHAR  bRefPicSelect[2][4];     /* 32 bytes total */
        };
    };
} DXVA_MBctrl_H264, *LPDXVA_MBctrl_H264;

/* H.264/AVC IndexA and IndexB data structure */
typedef struct _DXVA_DeblockIndexAB_H264 {
    UCHAR  bIndexAinternal; /* 6b - could get from MB CC */
    UCHAR  bIndexBinternal; /* 6b - could get from MB CC */

    UCHAR  bIndexAleft0;
    UCHAR  bIndexBleft0;

    UCHAR  bIndexAleft1;
    UCHAR  bIndexBleft1;

    UCHAR  bIndexAtop0;
    UCHAR  bIndexBtop0;

    UCHAR  bIndexAtop1;
    UCHAR  bIndexBtop1;
} DXVA_DeblockIndexAB_H264, *LPDXVA_DeblockIndexAB_H264;
/* 10 bytes in struct */

/* H.264/AVC deblocking filter control data structure */
typedef struct _DXVA_Deblock_H264 {
    USHORT  CurrMbAddr; /* dup info */   /* 2 bytes so far */
    union {
        struct {
            UCHAR  ReservedBit : 1;
            UCHAR  FieldModeCurrentMbFlag : 1; /* dup info */
            UCHAR  FieldModeLeftMbFlag : 1;
            UCHAR  FieldModeAboveMbFlag : 1;
            UCHAR  FilterInternal8x8EdgesFlag : 1;
            UCHAR  FilterInternal4x4EdgesFlag : 1;
            UCHAR  FilterLeftMbEdgeFlag : 1;
            UCHAR  FilterTopMbEdgeFlag : 1;
        };
        UCHAR  FirstByte;
    };
    UCHAR  Reserved8Bits;      /* 4 bytes so far */

    UCHAR  bbSinternalLeftVert; /* 2 bits per bS */
    UCHAR  bbSinternalMidVert;

    UCHAR  bbSinternalRightVert;
    UCHAR  bbSinternalTopHorz;  /* 8 bytes so far */

    UCHAR  bbSinternalMidHorz;
    UCHAR  bbSinternalBotHorz;       /* 10 bytes so far */

    USHORT wbSLeft0; /* 4 bits per bS (1 wasted) */
    USHORT wbSLeft1; /* 4 bits per bS (1 wasted) */

    USHORT wbSTop0;  /* 4 bits per bS (1 wasted) */
    USHORT wbSTop1;  /* 4b (2 wasted)  18 bytes so far*/

    DXVA_DeblockIndexAB_H264  IndexAB[3]; /* Y, Cb, Cr */

} DXVA_Deblock_H264, *LPDXVA_Deblock_H264;/* 48 bytes */

/* H.264/AVC film grain characteristics data structure */
typedef struct _DXVA_FilmGrainCharacteristics {

    USHORT  wFrameWidthInMbsMinus1;
    USHORT  wFrameHeightInMbsMinus1;

    DXVA_PicEntry_H264  InPic; /* flag is bot field flag */
    DXVA_PicEntry_H264  OutPic; /* flag is field pic flag */

    USHORT PicOrderCnt_offset;
    INT    CurrPicOrderCnt;
    UINT   StatusReportFeedbackNumber;

    UCHAR model_id;
    UCHAR separate_colour_description_present_flag;
    UCHAR film_grain_bit_depth_luma_minus8;
    UCHAR film_grain_bit_depth_chroma_minus8;

    UCHAR film_grain_full_range_flag;
    UCHAR film_grain_colour_primaries;
    UCHAR film_grain_transfer_characteristics;
    UCHAR film_grain_matrix_coefficients;

    UCHAR blending_mode_id;
    UCHAR log2_scale_factor;

    UCHAR comp_model_present_flag[4];
    UCHAR num_intensity_intervals_minus1[4];
    UCHAR num_model_values_minus1[4];

    UCHAR intensity_interval_lower_bound[3][16];
    UCHAR intensity_interval_upper_bound[3][16];
    SHORT comp_model_value[3][16][8];
} DXVA_FilmGrainChar_H264, *LPDXVA_FilmGrainChar_H264;

/* H.264/AVC status reporting data structure */
typedef struct _DXVA_Status_H264 {
    UINT   StatusReportFeedbackNumber;
    DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
    UCHAR  field_pic_flag;
    UCHAR  bDXVA_Func;
    UCHAR  bBufType;
    UCHAR  bStatus;
    UCHAR  bReserved8Bits;
    USHORT wNumMbsAffected;
} DXVA_Status_H264, *LPDXVA_Status_H264;

/* H.264 MVC picture parameters structure */
typedef struct _DXVA_PicParams_H264_MVC {
    USHORT  wFrameWidthInMbsMinus1;
    USHORT  wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
    UCHAR   num_ref_frames;

    union {
        struct {
            USHORT  field_pic_flag : 1;
            USHORT  MbaffFrameFlag : 1;
            USHORT  residual_colour_transform_flag : 1;
            USHORT  sp_for_switch_flag : 1;
            USHORT  chroma_format_idc : 2;
            USHORT  RefPicFlag : 1;
            USHORT  constrained_intra_pred_flag : 1;

            USHORT  weighted_pred_flag : 1;
            USHORT  weighted_bipred_idc : 2;
            USHORT  MbsConsecutiveFlag : 1;
            USHORT  frame_mbs_only_flag : 1;
            USHORT  transform_8x8_mode_flag : 1;
            USHORT  MinLumaBipredSize8x8Flag : 1;
            USHORT  IntraPicFlag : 1;
        };
        USHORT  wBitFields;
    };
    UCHAR  bit_depth_luma_minus8;
    UCHAR  bit_depth_chroma_minus8;

    USHORT Reserved16Bits;
    UINT   StatusReportFeedbackNumber;

    DXVA_PicEntry_H264  RefFrameList[16]; /* flag LT */
    INT    CurrFieldOrderCnt[2];
    INT    FieldOrderCntList[16][2];

    CHAR   pic_init_qs_minus26;
    CHAR   chroma_qp_index_offset;   /* also used for QScb */
    CHAR   second_chroma_qp_index_offset; /* also for QScr */
    UCHAR  ContinuationFlag;

    /* remainder for parsing */
    CHAR   pic_init_qp_minus26;
    UCHAR  num_ref_idx_l0_active_minus1;
    UCHAR  num_ref_idx_l1_active_minus1;
    UCHAR  Reserved8BitsA;

    USHORT FrameNumList[16];
    UINT   UsedForReferenceFlags;
    USHORT NonExistingFrameFlags;
    USHORT frame_num;

    UCHAR  log2_max_frame_num_minus4;
    UCHAR  pic_order_cnt_type;
    UCHAR  log2_max_pic_order_cnt_lsb_minus4;
    UCHAR  delta_pic_order_always_zero_flag;

    UCHAR  direct_8x8_inference_flag;
    UCHAR  entropy_coding_mode_flag;
    UCHAR  pic_order_present_flag;
    UCHAR  num_slice_groups_minus1;

    UCHAR  slice_group_map_type;
    UCHAR  deblocking_filter_control_present_flag;
    UCHAR  redundant_pic_cnt_present_flag;
    UCHAR  Reserved8BitsB;
    /* SliceGroupMap is not needed for MVC, as MVC is for high profile only */
    USHORT slice_group_change_rate_minus1;
    /* Following are H.264 MVC Specific parameters */
    UCHAR   num_views_minus1;
    USHORT  view_id[16];
    UCHAR   num_anchor_refs_l0[16];
    USHORT  anchor_ref_l0[16][16];
    UCHAR   num_anchor_refs_l1[16];
    USHORT  anchor_ref_l1[16][16];
    UCHAR   num_non_anchor_refs_l0[16];
    USHORT  non_anchor_ref_l0[16][16];
    UCHAR   num_non_anchor_refs_l1[16];
    USHORT  non_anchor_ref_l1[16][16];

    USHORT curr_view_id;
    UCHAR  anchor_pic_flag;
    UCHAR  inter_view_flag;
    USHORT ViewIDList[16];
    //!< add in Rock-Chip RKVDEC IP
    USHORT RefPicColmvUsedFlags;
    USHORT RefPicFiledFlags;
    UCHAR  RefPicLayerIdList[16];
    UCHAR  scaleing_list_enable_flag;
    ////!< for fpga test
    //USHORT seq_parameter_set_id;
    //USHORT pps_seq_parameter_set_id;
    //USHORT profile_idc;
    //UCHAR  constraint_set3_flag;
    //UCHAR  qpprime_y_zero_transform_bypass_flag;
    //UCHAR  mvc_extension_enable;

} DXVA_PicParams_H264_MVC, *LPDXVA_PicParams_H264_MVC;

typedef struct h264d_syntax_t
{
    UINT                    num;
    DXVA2_DecodeBufferDesc *buf;
} H264D_Syntax_t;

#endif /* end of __H264D_SYNTAX_H__ */

