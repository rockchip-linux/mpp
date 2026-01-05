/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef AV1D_PARSER_H
#define AV1D_PARSER_H

#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_frame.h"
#include "parser_api.h"

#include "av1d_codec.h"
#include "av1d_syntax.h"
#include "av1d_common.h"

extern RK_U32 av1d_debug;

#define AV1D_DBG_FUNCTION (0x00000001)
#define AV1D_DBG_HEADER   (0x00000002)
#define AV1D_DBG_REF      (0x00000004)
#define AV1D_DBG_STRMIN   (0x00000008)
#define AV1D_DBG_DUMP_RPU (0x10000000)

#define av1d_dbg(flag, fmt, ...) mpp_dbg_f(av1d_debug, flag, fmt, ##__VA_ARGS__)
#define av1d_dbg_func(fmt, ...)  av1d_dbg(AV1D_DBG_FUNCTION, fmt, ## __VA_ARGS__)

//!< function return check
#define FUN_CHECK(val)\
do{\
if ((val) < 0) {\
        av1d_dbg_func("Function error(%d).\n", __LINE__); \
        return (val); \
}} while (0)

typedef struct Av1DecContext_t {
    void *priv_data; // Av1Codec

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

    // config info
    MppBufSlots slots;
    MppBufSlots packet_slots;
    MppDecCfgSet *cfg;
    HalDecTask *task;
    const MppDecHwCap *hw_info;

    // split info
    RK_U32 frame_header_cnt;
    RK_U32 is_new_frame;
    RK_U8 *stream_data;
    RK_U32 stream_size;
    RK_U32 stream_offset;

    RK_S32 eos;
    MppFrameFormat usr_set_fmt;
} Av1DecCtx;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET av1d_parser_init(Av1DecCtx *ctx, ParserCfg *init);
MPP_RET av1d_parser_deinit(Av1DecCtx *ctx);
MPP_RET av1d_parser_frame(Av1DecCtx *ctx, HalDecTask *in_task);
MPP_RET av1d_parser_update(Av1DecCtx *ctx, void *info);
MPP_RET av1d_paser_reset(Av1DecCtx *ctx);
RK_S32  av1d_split_frame(Av1DecCtx *ctx, RK_U8 **out_data, RK_S32 *out_size,
                         RK_U8 *data, RK_S32 size);
MPP_RET av1d_get_frame_stream(Av1DecCtx *ctx, RK_U8 *buf, RK_S32 length);
MPP_RET av1d_set_context_with_seq(Av1DecCtx *ctx, const AV1SeqHeader *seq);

#ifdef  __cplusplus
}
#endif

#endif // AV1D_PARSER_H
