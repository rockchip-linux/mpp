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

#ifndef __AV1D_CODEC_H__
#define __AV1D_CODEC_H__

#include "mpp_frame.h"

#include "av1d_cbs.h"
#include "av1d_syntax.h"
#include "mpp_bitread.h"

typedef struct AV1ParseContext {
    Av1UnitFragment temporal_unit;
    int parsed_extradata;
} AV1ParseContext;

#define MPP_PARSER_PTS_NB 4
#define MAX_OBU_HEADER_SIZE (2 + 8)

typedef struct AV1OBU_T {
    /** Size of payload */
    int size;
    const uint8_t *data;

    /**
     * Size, in bits, of just the data, excluding the trailing_one_bit and
     * any trailing padding.
     */
    int size_bits;

    /** Size of entire OBU, including header */
    int raw_size;
    const uint8_t *raw_data;

    /** GetBitContext initialized to the start of the payload */
    BitReadCtx_t gb;

    int type;

    int temporal_id;
    int spatial_id;
} AV1OBU;

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

typedef struct Av1CodecContext_t {

    void *priv_data; /* Av1Context */
    void *priv_data2; /* SplitContext */

    MppFrameFormat pix_fmt;
    MppFrameColorSpace      colorspace;
    MppFrameColorRange      color_range;
    MppFrameColorPrimaries  color_primaries;
    MppFrameColorTransferCharacteristic  color_trc;
    MppFrameChromaLocation chroma_sample_location;

    RK_S32 width, height;

    RK_S32 profile, level;

    MppPacket pkt;

    DXVA_PicParams_AV1 pic_params;


    RK_S32 eos;
} Av1CodecContext;

#endif /*__AV1D_CODEC_H__*/
