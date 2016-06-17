/*
 *
 * Copyright 2010 Rockchip Electronics Co. LTD
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
 * @file        mpeg4d_syntax.h
 * @brief
 * @author      gzl(gzl@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2016.04.06 : Create
 */
#ifndef __MPG4D_SYNTAX__
#define __MPG4D_SYNTAX__

/* MPEG4PT2 Picture Parameter structure */
typedef struct _DXVA_PicParams_MPEG4_PART2 {
    RK_U8   short_video_header;
    RK_U8   vop_coding_type;
    RK_U8   vop_quant;
    RK_U16  wDecodedPictureIndex;
    RK_U16  wDeblockedPictureIndex;
    RK_U16  wForwardRefPictureIndex;
    RK_U16  wBackwardRefPictureIndex;
    RK_U16  vop_time_increment_resolution;
    RK_U32  TRB[2];
    RK_U32  TRD[2];

    union {
        struct {
            RK_U16  unPicPostProc                 : 2;
            RK_U16  interlaced                    : 1;
            RK_U16  quant_type                    : 1;
            RK_U16  quarter_sample                : 1;
            RK_U16  resync_marker_disable         : 1;
            RK_U16  data_partitioned              : 1;
            RK_U16  reversible_vlc                : 1;
            RK_U16  reduced_resolution_vop_enable : 1;
            RK_U16  vop_coded                     : 1;
            RK_U16  vop_rounding_type             : 1;
            RK_U16  intra_dc_vlc_thr              : 3;
            RK_U16  top_field_first               : 1;
            RK_U16  alternate_vertical_scan_flag  : 1;
        };
        RK_U16 wPicFlagBitFields;
    };
    RK_U8   profile_and_level_indication;
    RK_U8   video_object_layer_verid;
    RK_U16  vop_width;
    RK_U16  vop_height;
    union {
        struct {
            RK_U16  sprite_enable               : 2;
            RK_U16  no_of_sprite_warping_points : 6;
            RK_U16  sprite_warping_accuracy     : 2;
        };
        RK_U16 wSpriteBitFields;
    };
    RK_S16  warping_mv[4][2];
    union {
        struct {
            RK_U8  vop_fcode_forward   : 3;
            RK_U8  vop_fcode_backward  : 3;
        };
        RK_U8 wFcodeBitFields;
    };
    RK_U16  StatusReportFeedbackNumber;
    RK_U16  Reserved16BitsA;
    RK_U16  Reserved16BitsB;

    // add by lance 2016.04.15
    RK_U32  consumed_bits;
    RK_U32  buf_len;
    RK_U32  time_bp;
    RK_U32  time_pp;
    MppBuffer q_table_buffer;
    RK_S32  quarterpel;
    RK_U32  version;
    RK_U32  intra_dc_threshold;
} DXVA_PicParams_MPEG4_PART2, *LPDXVA_PicParams_MPEG4_PART2;

typedef struct _DXVA_QmatrixData {
    RK_U8   bNewQmatrix[4]; //intra Y, inter Y, intra chroma, inter chroma
    RK_U16  Qmatrix[4][64];
} DXVA_QmatrixData, *LPDXVA_QmatrixData;

typedef struct _DXVA_SliceInfo {
    RK_U16  wHorizontalPosition;
    RK_U16  wVerticalPosition;
    RK_U32  dwSliceBitsInBuffer;
    RK_U32  dwSliceDataLocation;
    RK_U8   bStartCodeBitOffset;
    RK_U8   bReservedBits;
    RK_U16  wMBbitOffset;
    RK_U16  wNumberMBsInSlice;
    RK_U16  wQuantizerScaleCode;
    RK_U16  wBadSliceChopping;
} DXVA_SliceInfo, *LPDXVA_SliceInfo;

typedef struct mpeg4d_dxva2_picture_context {
    DXVA_PicParams_MPEG4_PART2  pp;
    RK_U32                      frame_count;
    const RK_U8                 *bitstream;
    RK_U32                      bitstream_size;
} mpeg4d_dxva2_picture_context_t;

#endif /*__MPG4D_SYNTAX__*/
