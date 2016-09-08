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


#ifndef __H264D_SYNTAX_H__
#define __H264D_SYNTAX_H__

#include "h264_syntax.h"
#include "dxva_syntax.h"

/* H.264/AVC-specific structures */

/* H.264/AVC picture entry data structure */
typedef struct _DXVA_PicEntry_H264 {
    union {
        struct {
            RK_U8  Index7Bits : 7;
            RK_U8  AssociatedFlag : 1;
        };
        RK_U8  bPicEntry;
    };
} DXVA_PicEntry_H264, *LPDXVA_PicEntry_H264;  /* 1 byte */

/* H.264/AVC picture parameters structure */
typedef struct _DXVA_PicParams_H264 {
    RK_U16  wFrameWidthInMbsMinus1;
    RK_U16  wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
    RK_U8   num_ref_frames;

    union {
        struct {
            RK_U16  field_pic_flag : 1;
            RK_U16  MbaffFrameFlag : 1;
            RK_U16  residual_colour_transform_flag : 1;
            RK_U16  sp_for_switch_flag : 1;
            RK_U16  chroma_format_idc : 2;
            RK_U16  RefPicFlag : 1;
            RK_U16  constrained_intra_pred_flag : 1;

            RK_U16  weighted_pred_flag : 1;
            RK_U16  weighted_bipred_idc : 2;
            RK_U16  MbsConsecutiveFlag : 1;
            RK_U16  frame_mbs_only_flag : 1;
            RK_U16  transform_8x8_mode_flag : 1;
            RK_U16  MinLumaBipredSize8x8Flag : 1;
            RK_U16  IntraPicFlag : 1;
        };
        RK_U16  wBitFields;
    };
    RK_U8   bit_depth_luma_minus8;
    RK_U8   bit_depth_chroma_minus8;

    RK_U16  Reserved16Bits;
    RK_U32  StatusReportFeedbackNumber;

    DXVA_PicEntry_H264  RefFrameList[16]; /* flag LT */
    RK_S32  CurrFieldOrderCnt[2];
    RK_S32  FieldOrderCntList[16][2];

    RK_S8   pic_init_qs_minus26;
    RK_S8   chroma_qp_index_offset;   /* also used for QScb */
    RK_S8   second_chroma_qp_index_offset; /* also for QScr */
    RK_U8   ContinuationFlag;

    /* remainder for parsing */
    RK_S8   pic_init_qp_minus26;
    RK_U8   num_ref_idx_l0_active_minus1;
    RK_U8   num_ref_idx_l1_active_minus1;
    RK_U8   Reserved8BitsA;

    RK_U16  FrameNumList[16];
    RK_U32  UsedForReferenceFlags;
    RK_U16  NonExistingFrameFlags;
    RK_U16  frame_num;

    RK_U8   log2_max_frame_num_minus4;
    RK_U8   pic_order_cnt_type;
    RK_U8   log2_max_pic_order_cnt_lsb_minus4;
    RK_U8   delta_pic_order_always_zero_flag;

    RK_U8   direct_8x8_inference_flag;
    RK_U8   entropy_coding_mode_flag;
    RK_U8   pic_order_present_flag;
    RK_U8   num_slice_groups_minus1;

    RK_U8   slice_group_map_type;
    RK_U8   deblocking_filter_control_present_flag;
    RK_U8   redundant_pic_cnt_present_flag;
    RK_U8   Reserved8BitsB;

    RK_U16  slice_group_change_rate_minus1;

    //RK_U8   SliceGroupMap[810]; /* 4b/sgmu, Size BT.601 */

} DXVA_PicParams_H264, *LPDXVA_PicParams_H264;

/* H.264/AVC quantization weighting matrix data structure */
typedef struct _DXVA_Qmatrix_H264 {
    RK_U8   bScalingLists4x4[6][16];
    RK_U8   bScalingLists8x8[6][64];

} DXVA_Qmatrix_H264, *LPDXVA_Qmatrix_H264;

/* H.264/AVC slice control data structure - short form */
typedef struct _DXVA_Slice_H264_Short {
    RK_U32  BSNALunitDataLocation; /* type 1..5 */
    RK_U32  SliceBytesInBuffer; /* for off-host parse */
    RK_U16  wBadSliceChopping;  /* for off-host parse */
} DXVA_Slice_H264_Short, *LPDXVA_Slice_H264_Short;

/* H.264/AVC picture entry data structure - long form */
typedef struct _DXVA_Slice_H264_Long {
    RK_U32  BSNALunitDataLocation; /* type 1..5 */
    RK_U32  SliceBytesInBuffer; /* for off-host parse */
    RK_U16  wBadSliceChopping;  /* for off-host parse */

    RK_U16  first_mb_in_slice;
    RK_U16  NumMbsForSlice;

    RK_U16  BitOffsetToSliceData; /* after CABAC alignment */

    RK_U8   slice_type;
    RK_U8   luma_log2_weight_denom;
    RK_U8   chroma_log2_weight_denom;
    RK_U8   num_ref_idx_l0_active_minus1;
    RK_U8   num_ref_idx_l1_active_minus1;
    RK_S8   slice_alpha_c0_offset_div2;
    RK_S8   slice_beta_offset_div2;
    RK_U8   Reserved8Bits;
    DXVA_PicEntry_H264 RefPicList[3][32]; /* L0 & L1 */
    RK_S16  Weights[2][32][3][2]; /* L0 & L1; Y, Cb, Cr */
    RK_S8   slice_qs_delta;
    /* rest off-host parse */
    RK_S8   slice_qp_delta;
    RK_U8   redundant_pic_cnt;
    RK_U8   direct_spatial_mv_pred_flag;
    RK_U8   cabac_init_idc;
    RK_U8   disable_deblocking_filter_idc;
    RK_U16  slice_id;
    /* add parameter for setting hardware */
    RK_U32  active_sps_id;
    RK_U32  active_pps_id;
    RK_U32  idr_pic_id;
    RK_U32  idr_flag;
    RK_U32  drpm_used_bitlen;
    RK_U32  poc_used_bitlen;
    RK_U32  nal_ref_idc;
    RK_U32  profileIdc;
} DXVA_Slice_H264_Long, *LPDXVA_Slice_H264_Long;

/* H.264/AVC macro block control command data structure */
typedef struct _DXVA_MBctrl_H264 {
    union {
        struct {
            RK_U32  bSliceID : 8;   /* 1 byte */
            RK_U32  MbType5Bits : 5;
            RK_U32  IntraMbFlag : 1;
            RK_U32  mb_field_decoding_flag : 1;
            RK_U32  transform_size_8x8_flag : 1;   /* 2 bytes */
            RK_U32  HostResidDiff : 1;
            RK_U32  DcBlockCodedCrFlag : 1;
            RK_U32  DcBlockCodedCbFlag : 1;
            RK_U32  DcBlockCodedYFlag : 1;
            RK_U32  FilterInternalEdgesFlag : 1;
            RK_U32  FilterLeftMbEdgeFlag : 1;
            RK_U32  FilterTopMbEdgeFlag : 1;
            RK_U32  ReservedBit : 1;
            RK_U32  bMvQuantity : 8;   /* 4 bytes */
        };
        RK_U32  dwMBtype;                    /* 4 bytes so far */
    };
    RK_U16  CurrMbAddr;                  /* 6 bytes so far */
    RK_U16  wPatternCode[3];/* YCbCr, 16 4x4 blks, 1b each */
    /* 12 bytes so far */
    RK_U8   bQpPrime[3];    /* Y, Cb, Cr, need just 7b QpY */
    RK_U8   bMBresidDataQuantity;
    RK_U32  dwMBdataLocation;  /* offset into resid buffer */
    /* 20 bytes so far */
    union {
        struct {
            /* start here for Intra MB's  (9 useful bytes in branch) */
            RK_U16 LumaIntraPredModes[4];/* 16 blocks, 4b each */
            /* 28 bytes so far */
            union {
                struct {
                    RK_U8  intra_chroma_pred_mode : 2;
                    RK_U8  IntraPredAvailFlags : 5;
                    RK_U8  ReservedIntraBit : 1;
                };
                RK_U8  bMbIntraStruct;        /* 29 bytes so far */
            };
            RK_U8 ReservedIntra24Bits[3];   /* 32 bytes total  */
        };
        struct {
            /* start here for non-Intra MB's (12 bytes in branch)    */
            RK_U8  bSubMbShapes;          /* 4 subMbs, 2b each */
            RK_U8  bSubMbPredModes;       /* 4 subMBs, 2b each */
            /* 22 bytes so far */
            RK_U16 wMvBuffOffset;     /* offset into MV buffer */
            RK_U8  bRefPicSelect[2][4];     /* 32 bytes total */
        };
    };
} DXVA_MBctrl_H264, *LPDXVA_MBctrl_H264;

/* H.264/AVC IndexA and IndexB data structure */
typedef struct _DXVA_DeblockIndexAB_H264 {
    RK_U8   bIndexAinternal; /* 6b - could get from MB CC */
    RK_U8   bIndexBinternal; /* 6b - could get from MB CC */

    RK_U8   bIndexAleft0;
    RK_U8   bIndexBleft0;

    RK_U8   bIndexAleft1;
    RK_U8   bIndexBleft1;

    RK_U8   bIndexAtop0;
    RK_U8   bIndexBtop0;

    RK_U8   bIndexAtop1;
    RK_U8   bIndexBtop1;
} DXVA_DeblockIndexAB_H264, *LPDXVA_DeblockIndexAB_H264;
/* 10 bytes in struct */

/* H.264/AVC deblocking filter control data structure */
typedef struct _DXVA_Deblock_H264 {
    RK_U16  CurrMbAddr; /* dup info */   /* 2 bytes so far */
    union {
        struct {
            RK_U8  ReservedBit : 1;
            RK_U8  FieldModeCurrentMbFlag : 1; /* dup info */
            RK_U8  FieldModeLeftMbFlag : 1;
            RK_U8  FieldModeAboveMbFlag : 1;
            RK_U8  FilterInternal8x8EdgesFlag : 1;
            RK_U8  FilterInternal4x4EdgesFlag : 1;
            RK_U8  FilterLeftMbEdgeFlag : 1;
            RK_U8  FilterTopMbEdgeFlag : 1;
        };
        RK_U8  FirstByte;
    };
    RK_U8   Reserved8Bits;      /* 4 bytes so far */

    RK_U8   bbSinternalLeftVert; /* 2 bits per bS */
    RK_U8   bbSinternalMidVert;

    RK_U8   bbSinternalRightVert;
    RK_U8   bbSinternalTopHorz;  /* 8 bytes so far */

    RK_U8   bbSinternalMidHorz;
    RK_U8   bbSinternalBotHorz;       /* 10 bytes so far */

    RK_U16  wbSLeft0; /* 4 bits per bS (1 wasted) */
    RK_U16  wbSLeft1; /* 4 bits per bS (1 wasted) */

    RK_U16  wbSTop0;  /* 4 bits per bS (1 wasted) */
    RK_U16  wbSTop1;  /* 4b (2 wasted)  18 bytes so far*/

    DXVA_DeblockIndexAB_H264  IndexAB[3]; /* Y, Cb, Cr */

} DXVA_Deblock_H264, *LPDXVA_Deblock_H264;/* 48 bytes */

/* H.264/AVC film grain characteristics data structure */
typedef struct _DXVA_FilmGrainCharacteristics {

    RK_U16  wFrameWidthInMbsMinus1;
    RK_U16  wFrameHeightInMbsMinus1;

    DXVA_PicEntry_H264  InPic; /* flag is bot field flag */
    DXVA_PicEntry_H264  OutPic; /* flag is field pic flag */

    RK_U16  PicOrderCnt_offset;
    RK_S32  CurrPicOrderCnt;
    RK_U32  StatusReportFeedbackNumber;

    RK_U8   model_id;
    RK_U8   separate_colour_description_present_flag;
    RK_U8   film_grain_bit_depth_luma_minus8;
    RK_U8   film_grain_bit_depth_chroma_minus8;

    RK_U8   film_grain_full_range_flag;
    RK_U8   film_grain_colour_primaries;
    RK_U8   film_grain_transfer_characteristics;
    RK_U8   film_grain_matrix_coefficients;

    RK_U8   blending_mode_id;
    RK_U8   log2_scale_factor;

    RK_U8   comp_model_present_flag[4];
    RK_U8   num_intensity_intervals_minus1[4];
    RK_U8   num_model_values_minus1[4];

    RK_U8   intensity_interval_lower_bound[3][16];
    RK_U8   intensity_interval_upper_bound[3][16];
    RK_S16  comp_model_value[3][16][8];
} DXVA_FilmGrainChar_H264, *LPDXVA_FilmGrainChar_H264;

/* H.264/AVC status reporting data structure */
typedef struct _DXVA_Status_H264 {
    RK_U32  StatusReportFeedbackNumber;
    DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
    RK_U8   field_pic_flag;
    RK_U8   bDXVA_Func;
    RK_U8   bBufType;
    RK_U8   bStatus;
    RK_U8   bReserved8Bits;
    RK_U16  wNumMbsAffected;
} DXVA_Status_H264, *LPDXVA_Status_H264;

/* H.264 MVC picture parameters structure */
typedef struct _DXVA_PicParams_H264_MVC {
    RK_U16  wFrameWidthInMbsMinus1;
    RK_U16  wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
    RK_U8   num_ref_frames;

    union {
        struct {
            RK_U16  field_pic_flag : 1;
            RK_U16  MbaffFrameFlag : 1;
            RK_U16  residual_colour_transform_flag : 1;
            RK_U16  sp_for_switch_flag : 1;
            RK_U16  chroma_format_idc : 2;
            RK_U16  RefPicFlag : 1;
            RK_U16  constrained_intra_pred_flag : 1;

            RK_U16  weighted_pred_flag : 1;
            RK_U16  weighted_bipred_idc : 2;
            RK_U16  MbsConsecutiveFlag : 1;
            RK_U16  frame_mbs_only_flag : 1;
            RK_U16  transform_8x8_mode_flag : 1;
            RK_U16  MinLumaBipredSize8x8Flag : 1;
            RK_U16  IntraPicFlag : 1;
        };
        RK_U16  wBitFields;
    };
    RK_U8   bit_depth_luma_minus8;
    RK_U8   bit_depth_chroma_minus8;

    RK_U16  Reserved16Bits;
    RK_U32  StatusReportFeedbackNumber;

    DXVA_PicEntry_H264  RefFrameList[16]; /* flag LT */
    RK_S32  CurrFieldOrderCnt[2];
    RK_S32  FieldOrderCntList[16][2];

    RK_S8   pic_init_qs_minus26;
    RK_S8   chroma_qp_index_offset;   /* also used for QScb */
    RK_S8   second_chroma_qp_index_offset; /* also for QScr */
    RK_U8   ContinuationFlag;

    /* remainder for parsing */
    RK_S8   pic_init_qp_minus26;
    RK_U8   num_ref_idx_l0_active_minus1;
    RK_U8   num_ref_idx_l1_active_minus1;
    RK_U8   Reserved8BitsA;

    RK_U16  FrameNumList[16];
    RK_U16  LongTermPicNumList[16];
    RK_U32  UsedForReferenceFlags;
    RK_U16  NonExistingFrameFlags;
    RK_U16  frame_num;

    RK_U8   log2_max_frame_num_minus4;
    RK_U8   pic_order_cnt_type;
    RK_U8   log2_max_pic_order_cnt_lsb_minus4;
    RK_U8   delta_pic_order_always_zero_flag;

    RK_U8   direct_8x8_inference_flag;
    RK_U8   entropy_coding_mode_flag;
    RK_U8   pic_order_present_flag;
    RK_U8   num_slice_groups_minus1;

    RK_U8   slice_group_map_type;
    RK_U8   deblocking_filter_control_present_flag;
    RK_U8   redundant_pic_cnt_present_flag;
    RK_U8   Reserved8BitsB;
    /* SliceGroupMap is not needed for MVC, as MVC is for high profile only */
    RK_U16  slice_group_change_rate_minus1;
    /* Following are H.264 MVC Specific parameters */
    RK_U8   num_views_minus1;
    RK_U16  view_id[16];
    RK_U8   num_anchor_refs_l0[16];
    RK_U16  anchor_ref_l0[16][16];
    RK_U8   num_anchor_refs_l1[16];
    RK_U16  anchor_ref_l1[16][16];
    RK_U8   num_non_anchor_refs_l0[16];
    RK_U16  non_anchor_ref_l0[16][16];
    RK_U8   num_non_anchor_refs_l1[16];
    RK_U16  non_anchor_ref_l1[16][16];

    RK_U16  curr_view_id;
    RK_U8   anchor_pic_flag;
    RK_U8   inter_view_flag;
    RK_U16  ViewIDList[16];
    //!< add in Rock-Chip RKVDEC IP
    RK_U16  curr_layer_id;
    RK_U16  RefPicColmvUsedFlags;
    RK_U16  RefPicFiledFlags;
    RK_U8   RefPicLayerIdList[16];
    RK_U8   scaleing_list_enable_flag;
    RK_U16  UsedForInTerviewflags;

    ////!< for fpga test
    //USHORT seq_parameter_set_id;
    //USHORT pps_seq_parameter_set_id;
    //USHORT profile_idc;
    //UCHAR  constraint_set3_flag;
    //UCHAR  qpprime_y_zero_transform_bypass_flag;
    //UCHAR  mvc_extension_enable;

} DXVA_PicParams_H264_MVC, *LPDXVA_PicParams_H264_MVC;



typedef struct h264d_syntax_t {
    RK_U32                  num;
    DXVA2_DecodeBufferDesc *buf;
} H264dSyntax_t;

#endif /*__H264D_SYNTAX_H__*/

