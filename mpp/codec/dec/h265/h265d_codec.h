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
 * @file       h265d_codec.h
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#ifndef __MPP_CODEC_H__
#define __MPP_CODEC_H__

#include "mpp_dec.h"

enum MppColorSpace {
    MPPCOL_SPC_RGB          = 0,
    MPPCOL_SPC_BT709        = 1,    ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    MPPCOL_SPC_UNSPECIFIED  = 2,
    MPPCOL_SPC_FCC          = 4,
    MPPCOL_SPC_BT470BG      = 5,    ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    MPPCOL_SPC_SMPTE170M    = 6,    ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC / functionally identical to above
    MPPCOL_SPC_SMPTE240M    = 7,
    MPPCOL_SPC_YCOCG        = 8,    ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    MPPCOL_SPC_BT2020_NCL   = 9,    ///< ITU-R BT2020 non-constant luminance system
    MPPCOL_SPC_BT2020_CL    = 10,   ///< ITU-R BT2020 constant luminance system
    MPPCOL_SPC_NB             ,     ///< Not part of ABI
};

enum MppColorRange {
    MPPCOL_RANGE_UNSPECIFIED = 0,
    MPPCOL_RANGE_MPEG        = 1, ///< the normal 219*2^(n-8) "MPEG" YUV ranges
    MPPCOL_RANGE_JPEG        = 2, ///< the normal     2^n-1   "JPEG" YUV ranges
    MPPCOL_RANGE_NB             , ///< Not part of ABI
};

typedef struct MppRational {
    RK_S32 num; ///< numerator
    RK_S32 den; ///< denominator
} MppRational_t;

enum MppPictureStructure {
    MPP_PICTURE_STRUCTURE_UNKNOWN,      //< unknown
    MPP_PICTURE_STRUCTURE_TOP_FIELD,    //< coded as top field
    MPP_PICTURE_STRUCTURE_BOTTOM_FIELD, //< coded as bottom field
    MPP_PICTURE_STRUCTURE_FRAME,        //< coded as frame
};

#define END_NOT_FOUND (-100)
#define MPP_PARSER_PTS_NB 4

typedef struct SplitContext {
    RK_U8 *buffer;
    RK_U32 buffer_size;
    RK_S32 index;
    RK_S32 last_index;
    RK_U32 state;             ///< contains the last few bytes in MSB order
    RK_S32 frame_start_found;
    RK_S32 overread;               ///< the number of bytes which where irreversibly read from the next frame
    RK_S32 overread_index;         ///< the index into ParseContext.buffer of the overread bytes
    RK_U64 state64;           ///< contains the last 8 bytes in MSB order
    RK_S64 pts;     /* pts of the current frame */
    RK_S64 dts;     /* dts of the current frame */
    RK_S64 frame_offset; /* offset of the current frame */
    RK_S64 cur_offset; /* current offset
                           (incremented by each av_parser_parse()) */
    RK_S64 next_frame_offset; /* offset of the next frame */
    /* private data */
    RK_S64 last_pts;
    RK_S64 last_dts;
    RK_S32 fetch_timestamp;

    RK_S32 cur_frame_start_index;
    RK_S64 cur_frame_offset[MPP_PARSER_PTS_NB];
    RK_S64 cur_frame_pts[MPP_PARSER_PTS_NB];
    RK_S64 cur_frame_dts[MPP_PARSER_PTS_NB];

    RK_S64 offset;      ///< byte offset from starting packet start
    RK_S64 cur_frame_end[MPP_PARSER_PTS_NB];
    /**
     * Set by parser to 1 for key frames and 0 for non-key frames.
     * It is initialized to -1, so if the parser doesn't set this flag,
     * old-style fallback using AV_PICTURE_TYPE_I picture type as key frames
     * will be used.
     */
    RK_S32 key_frame;
    RK_S32 eos;
} SplitContext_t;

typedef struct H265dContext {

    void *priv_data;

    void *split_cxt;

    /**
    * for rk log printf
    **/
    // RK_LOG_CONTEX_t *log_ctx;
    /**
     * display width & height
    **/
    RK_S32 width, height;

    /**
     *codec decoder width & height
    **/
    RK_S32 coded_width, coded_height;

    RK_U8 *extradata;

    RK_U32 extradata_size;

    /**
     * Pixel format
    **/
    RK_U32 pix_fmt;

    RK_U32 nBitDepth;
    RK_U32 err_recognition;

    /**
         * sample aspect ratio (0 if unknown)
         * That is the width of a pixel divided by the height of the pixel.
         * Numerator and denominator must be relatively prime and smaller than 256 for some video standards.
         * - decoding: Set by rkcodec.
        */
    MppRational_t sample_aspect_ratio;

    /**
     * YUV colorspace type.
     * - decoding: Set by rkcodec
     */
    enum MppColorSpace colorspace;

    /**
     * MPEG vs JPEG YUV range.
     * - decoding: Set by rkcodec
     */
    enum MppColorRange color_range;

    void *compare_info;

    RK_U32 need_split;
    RK_U32 disable_error;
} H265dContext_t;
#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 h265d_parser2_syntax(void *ctx);
RK_S32 h265d_syntax_fill_slice(void *ctx, RK_S32 input_index);

#ifdef  __cplusplus
}
#endif

#endif /* __MPP_CODEC_H__ */
