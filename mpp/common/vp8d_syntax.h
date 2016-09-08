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

#ifndef __VP8D_SYNTAX_H__
#define __VP8D_SYNTAX_H__

#include "rk_type.h"

typedef struct _DXVA_PicEntry_VP8 {
    union {
        struct {
            RK_U8 Index7Bits     : 7;
            RK_U8 AssociatedFlag : 1;
        };
        RK_U8 bPicEntry;
    };
} DXVA_PicEntry_VP8;
typedef struct _segmentation_Vp8 {
    union {
        struct {
            RK_U8 segmentation_enabled         : 1;
            RK_U8 update_mb_segmentation_map   : 1;
            RK_U8 update_mb_segmentation_data  : 1;
            RK_U8 mb_segement_abs_delta        : 1;
            RK_U8 ReservedSegmentFlags4Bits    : 4;
        };
        RK_U8 wSegmentFlags;
    };
    RK_S8 segment_feature_data[2][4];
    RK_U8 mb_segment_tree_probs[3];
} DXVA_segmentation_VP8;

typedef struct VP8DDxvaParam_t {
    RK_U32 first_part_size;
    RK_U32 width;
    RK_U32 height;
    DXVA_PicEntry_VP8 CurrPic;
    union {
        struct {
            RK_U8 frame_type           : 1;
            RK_U8 version              : 3;
            RK_U8 show_frame           : 1;
            RK_U8 clamp_type           : 1;
            RK_U8 ReservedFrameTag2Bits: 2;
        };
        RK_U8 wFrameTagFlags;
    };
    DXVA_segmentation_VP8 stVP8Segments;
    RK_U8              filter_type;
    RK_U8              filter_level;
    RK_U8              sharpness;
    RK_U8              mode_ref_lf_delta_enabled;
    RK_U8              mode_ref_lf_delta_update;
    RK_S8              ref_lf_deltas[4];
    RK_S8              mode_lf_deltas[4];
    RK_U8              log2_nbr_of_dct_partitions;
    RK_U8              base_qindex;
    RK_S8              y1dc_delta_q;
    RK_S8              y1ac_delta_q;
    RK_S8              y2dc_delta_q;
    RK_S8              y2ac_delta_q;
    RK_S8              uvdc_delta_q;
    RK_S8              uvac_delta_q;

    DXVA_PicEntry_VP8  alt_fb_idx;
    DXVA_PicEntry_VP8  gld_fb_idx;
    DXVA_PicEntry_VP8  lst_fb_idx;

    RK_U8              ref_frame_sign_bias_golden;
    RK_U8              ref_frame_sign_bias_altref;
    RK_U8              refresh_entropy_probs;

    RK_U8              vp8_coef_update_probs[4][8][3][11];
    RK_U8              probe_skip_false;
    RK_U8              mb_no_coeff_skip;
    RK_U8              prob_intra;
    RK_U8              prob_last;
    RK_U8              prob_golden;
    RK_U8              intra_16x16_prob[4];
    RK_U8              intra_chroma_prob[3];
    RK_U8              vp8_mv_update_probs[2][19];

    RK_U32             decMode;
    RK_U32             bool_value;
    RK_U32             bool_range;
    RK_U32             stream_start_offset;
    RK_U32             dctPartitionOffsets[8];
    RK_U32             bitstream_length;
    RK_U32             stream_start_bit;
    RK_U32             frameTagSize;
    RK_U32             streamEndPos;
    RK_U32             offsetToDctParts;
} DXVA_PicParams_VP8;

#if 0
typedef struct VP8DDxvaParam_t {
    RK_U32 first_part_size;
    RK_U32 width;
    RK_U32 height;
    DXVA_PicEntry_VP8 CurrPic;
    union {
        struct {
            RK_U8 key_frame : 1;
            RK_U8 segmentationEnabled : 1;
            RK_U8 segmentationMapUpdate : 1;
            RK_U8 modeRefLfEnabled : 1;
            RK_U8 coeffSkipMode: 1;
            RK_U8 reservedFormatInfo: 3;
        };
        RK_U8 wFormatAndPictureInfoFlags;
    };
    RK_U32              decMode;
    RK_U32              loopFilterType;
    RK_U32              loopFilterSharpness;
    RK_U32              loopFilterLevel;
    RK_U32              segmentFeatureMode;
    RK_U32              vpVersion;
    RK_U32              bool_value;
    RK_U32              bool_range;
    RK_U32              stream_start_offset;
    RK_U32              stream_start_bit;
    RK_U32              frameTagSize;
    RK_U32              streamEndPos;
    RK_U32              nbrDctPartitions;
    RK_U32              offsetToDctParts;

    RK_S8              qpYAc;
    RK_S8              qpYDc;
    RK_S8              qpY2Ac;
    RK_S8              qpY2Dc;
    RK_S8              qpChAc;
    RK_S8              qpChDc;
    RK_U8              vp8_coef_update_probs[4][8][3][11];
    RK_U8              probe_skip_false;
    RK_U8              prob_intra;
    RK_U8              prob_last;
    RK_U8              prob_golden;
    RK_U8              intra_16x16_prob[4];
    RK_U8              intra_chroma_prob[3];
    RK_U8              vp8_mv_update_probs[2][19];
    RK_U8              vp8_segment_prob[3];

    RK_U32             refFrameSignBias[2];
    RK_S32             dcPred[2];
    RK_S32             dcMatch[2];
    RK_S32             segmentQp[4];
    RK_S32             mbRefLfDelta[4];
    RK_S32             mbModeLfDelta[4];
    RK_U32             vp7ScanOrder[16];
    RK_S32             segmentLoopfilter[4];
    RK_U32             dctPartitionOffsets[8];
    RK_U32              bitstream_length;
    DXVA_PicEntry_VP8   frame_refs[3];
} DXVA_PicParams_VP8;
#endif
#endif
