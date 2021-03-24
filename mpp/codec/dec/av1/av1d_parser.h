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

#ifndef __AV1D_PARSER_H__
#define __AV1D_PARSER_H__

#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_bitread.h"

#include "parser_api.h"

#include "av1.h"
#include "av1d_codec.h"
#include "av1d_cbs.h"
#include "av1d_syntax.h"
#include "av1d_common.h"
#include "av1_entropymode.h"

extern RK_U32 av1d_debug;

#define AV1D_DBG_FUNCTION (0x00000001)
#define AV1D_DBG_HEADER   (0x00000002)
#define AV1D_DBG_REF      (0x00000004)
#define AV1D_DBG_STRMIN   (0x00000008)

#define av1d_dbg(flag, fmt, ...) _mpp_dbg(av1d_debug, flag, fmt, ##__VA_ARGS__)
#define av1d_dbg_func(fmt, ...)  av1d_dbg(AV1D_DBG_FUNCTION, fmt, ## __VA_ARGS__)

typedef struct RefInfo {
    RK_S32 ref_count;
    RK_U32 invisible;
    RK_U32 is_output;
    RK_U32 lst_frame_offset;
    RK_U32 lst2_frame_offset;
    RK_U32 lst3_frame_offset;
    RK_U32 gld_frame_offset;
    RK_U32 bwd_frame_offset;
    RK_U32 alt2_frame_offset;
    RK_U32 alt_frame_offset;
    RK_U32 is_intra_frame;
    RK_U32 intra_only;
} RefInfo;


typedef struct AV1Frame {
    MppFrame f;
    RK_S32 slot_index;
    AV1RawFrameHeader *raw_frame_header;
    RK_S32 temporal_id;
    RK_S32 spatial_id;
    RK_U8  order_hint;
    RK_U8  gm_type[AV1_NUM_REF_FRAMES];
    RK_S32 gm_params[AV1_NUM_REF_FRAMES][6];
    RK_U8  skip_mode_frame_idx[2];
    AV1RawFilmGrainParams film_grain;
    RK_U8 coded_lossless;
    RefInfo *ref;
} AV1Frame;


typedef struct AV1Context_t {
    BitReadCtx_t gb;

    AV1RawSequenceHeader *sequence_header;
    AV1RawSequenceHeader *seq_ref;
    AV1RawFrameHeader *raw_frame_header;
    Av1UnitFragment  current_obu;

    RK_S32 seen_frame_header;
    RK_U8  *frame_header;
    size_t frame_header_size;

    AV1Frame ref[AV1_NUM_REF_FRAMES];
    AV1Frame cur_frame;

    RK_S32 temporal_id;
    RK_S32 spatial_id;
    RK_S32 operating_point_idc;

    RK_S32 bit_depth;
    RK_S32 order_hint;
    RK_S32 frame_width;
    RK_S32 frame_height;
    RK_S32 upscaled_width;
    RK_S32 render_width;
    RK_S32 render_height;

    RK_S32 num_planes;
    RK_S32 coded_lossless;
    RK_S32 all_lossless;
    RK_S32 tile_cols;
    RK_S32 tile_rows;
    RK_S32 tile_num;
    RK_S32 operating_point;
    RK_S32 extra_has_frame;
    RK_U32 frame_tag_size;
    RK_U32 obu_len;

    AV1CDFs *cdfs;
    MvCDFs  *cdfs_ndvc;
    AV1CDFs default_cdfs;
    MvCDFs  default_cdfs_ndvc;
    AV1CDFs cdfs_last[NUM_REF_FRAMES];
    MvCDFs  cdfs_last_ndvc[NUM_REF_FRAMES];
    RK_U8 disable_frame_end_update_cdf;
    RK_U8 frame_is_intra;
    RK_U8 refresh_frame_flags;

    const Av1UnitType *unit_types;
    RK_S32 nb_unit_types;

    RK_U32 tile_offset_start[AV1_MAX_TILES];
    RK_U32 tile_offset_end[AV1_MAX_TILES];

    AV1ReferenceFrameState ref_s[AV1_NUM_REF_FRAMES];

    MppBufSlots slots;
    MppBufSlots packet_slots;
    RK_U8 skip_ref0;
    RK_U8 skip_ref1;
    MppDecCfgSet *cfg;
    HalDecTask *task;
    RK_S32 eos;       ///< current packet contains an EOS/EOB NAL
    RK_S64 pts;
} AV1Context;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET av1d_parser_init(Av1CodecContext *ctx, ParserCfg *init);

MPP_RET av1d_parser_deinit(Av1CodecContext *ctx);

RK_S32 av1d_parser_frame(Av1CodecContext *ctx, HalDecTask *in_task);

void av1d_parser_update(Av1CodecContext *ctx, void *info);

MPP_RET av1d_paser_reset(Av1CodecContext *ctx);

RK_S32 av1d_split_frame(SplitContext_t *ctx,
                        RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 size);

MPP_RET av1d_get_frame_stream(Av1CodecContext *ctx, RK_U8 *buf, RK_S32 length);

MPP_RET av1d_split_deinit(Av1CodecContext *ctx);

MPP_RET av1d_split_init(Av1CodecContext *ctx);

RK_S32 av1d_parser2_syntax(Av1CodecContext *ctx);

RK_S32 mpp_av1_split_fragment(AV1Context *ctx, Av1UnitFragment *frag, RK_S32 header_flag);
RK_S32 mpp_av1_read_fragment_content(AV1Context *ctx, Av1UnitFragment *frag);
RK_S32 mpp_av1_set_context_with_sequence(Av1CodecContext *ctx,
                                         const AV1RawSequenceHeader *seq);
void mpp_av1_fragment_reset(Av1UnitFragment *frag);
RK_S32 mpp_av1_assemble_fragment(AV1Context *ctx, Av1UnitFragment *frag);
void mpp_av1_flush(AV1Context *ctx);
void mpp_av1_close(AV1Context *ctx);
void mpp_av1_free_metadata(void *unit, RK_U8 *content);

void Av1GetCDFs(AV1Context *ctx, RK_U32 ref_idx);
void Av1StoreCDFs(AV1Context *ctx, RK_U32 refresh_frame_flags);

#ifdef  __cplusplus
}
#endif

#endif // __AV1D_PARSER_H__
