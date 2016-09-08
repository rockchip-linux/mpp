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

#include "dxva_syntax.h"

#define MPEG4_VIDOBJLAY_AR_SQUARE           1
#define MPEG4_VIDOBJLAY_AR_625TYPE_43       2
#define MPEG4_VIDOBJLAY_AR_525TYPE_43       3
#define MPEG4_VIDOBJLAY_AR_625TYPE_169      8
#define MPEG4_VIDOBJLAY_AR_525TYPE_169      9
#define MPEG4_VIDOBJLAY_AR_EXTPAR           15

#define MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR   0
#define MPEG4_VIDOBJLAY_SHAPE_BINARY        1
#define MPEG4_VIDOBJLAY_SHAPE_BINARY_ONLY   2
#define MPEG4_VIDOBJLAY_SHAPE_GRAYSCALE     3

/*video sprite specific*/
#define MPEG4_SPRITE_NONE                   0
#define MPEG4_SPRITE_STATIC                 1
#define MPEG4_SPRITE_GMC                    2

/*video vop specific*/
typedef enum {
    MPEG4_RESYNC_VOP    = -2,
    MPEG4_INVALID_VOP   = -1,
    MPEG4_I_VOP         = 0,
    MPEG4_P_VOP         = 1,
    MPEG4_B_VOP         = 2,
    MPEG4_S_VOP         = 3,
    MPEG4_N_VOP         = 4,
    MPEG4_D_VOP         = 5,                /*Drop Frame*/
} MPEG4VOPType;

#define MPEG4_HDR_ERR       -2
#define MPEG4_DEC_ERR       -4
#define MPEG4_VLD_ERR       -5
#define MPEG4_FORMAT_ERR    -6
#define MPEG4_DIVX_PBBI     -7
#define MPEG4_INFO_CHANGE   -10

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

    // FIXME: added for rockchip hardware information
    RK_U32  custorm_version;
    RK_U32  prev_coding_type;
    RK_U32  time_bp;
    RK_U32  time_pp;
    RK_U32  header_bits;
} DXVA_PicParams_MPEG4_PART2, *LPDXVA_PicParams_MPEG4_PART2;

typedef struct _DXVA_QmatrixData {
    RK_U8   bNewQmatrix[4]; // intra Y, inter Y, intra chroma, inter chroma
    RK_U8   Qmatrix[4][64]; // NOTE: here we change U16 to U8
} DXVA_QmatrixData, *LPDXVA_QmatrixData;

typedef struct mpeg4d_dxva2_picture_context {
    DXVA_PicParams_MPEG4_PART2  pp;
    DXVA_QmatrixData            qm;

    // pointer and storage for buffer descriptor
    DXVA2_DecodeBufferDesc      *data[3];
    DXVA2_DecodeBufferDesc      desc[3];

    RK_U32                      frame_count;
    const RK_U8                 *bitstream;
    RK_U32                      bitstream_size;
} mpeg4d_dxva2_picture_context_t;

#endif /*__MPG4D_SYNTAX__*/
