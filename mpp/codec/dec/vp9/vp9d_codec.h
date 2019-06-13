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
#ifndef __VP9D_CODEC_H__
#define __VP9D_CODEC_H__

#include <stdlib.h>

#include "hal_task.h"

#include "vp9d_syntax.h"

typedef struct VP9ParseContext {
    RK_S32 n_frames; // 1-8
    RK_S32 size[8];
    RK_S64 pts;
} VP9ParseContext;
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
    void  *priv_data;
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

typedef struct Vp9CodecContext {

    void *priv_data; /* VP9Context */
    void *priv_data2; /* AVParserContext */

    RK_S32 pix_fmt;

    RK_S32 profile;
#define FF_PROFILE_VP9_0                            0
#define FF_PROFILE_VP9_1                            1
#define FF_PROFILE_VP9_2                            2
#define FF_PROFILE_VP9_3                            3

    RK_S32 level;
#define FF_LEVEL_UNKNOWN -99

    MppFrameColorSpace colorspace;

    MppFrameColorRange color_range;

    RK_S32 width, height;

    MppPacket pkt;

    // DXVA_segmentation_VP9 segmentation;
    DXVA_PicParams_VP9 pic_params;
    // DXVA_Slice_VPx_Short slice_short;
    RK_S32 eos;
} Vp9CodecContext;

#endif /*__VP9D_CODEC_H__*/

