/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "av1d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_packet_impl.h"

#include "mpp_parser.h"
#include "av1d_codec.h"
#include "av1d_parser.h"

/*!
 ***********************************************************************
 * \brief
 *   free all buffer
 ***********************************************************************
 */
MPP_RET av1d_deinit(void *ctx)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)ctx;

    if (av1_ctx) {
        av1d_parser_deinit(av1_ctx);
        if (av1_ctx->pkt) {
            RK_U8 *buf;

            buf = mpp_packet_get_data(av1_ctx->pkt);
            MPP_FREE(buf);
            mpp_packet_deinit(&av1_ctx->pkt);
        }
    }

    return MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   alloc all buffer
 ***********************************************************************
 */
MPP_RET av1d_init(void *ctx, ParserCfg *init)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)ctx;
    RK_U8 *buf = NULL;
    RK_S32 size = SZ_512K;
    MPP_RET ret = MPP_OK;

    if (!av1_ctx || !init) {
        mpp_err("av1d init fail");
        return MPP_ERR_NULL_PTR;
    }

    av1_ctx->pix_fmt = MPP_FMT_BUTT;
    av1_ctx->usr_set_fmt = MPP_FMT_BUTT;

    if ((ret = av1d_parser_init(av1_ctx, init)) != MPP_OK)
        goto _err_exit;

    buf = mpp_malloc(RK_U8, size);
    if (!buf) {
        mpp_err("av1d init malloc stream buffer fail");
        ret = MPP_ERR_NOMEM;
        goto _err_exit;
    }

    if ((ret = mpp_packet_init(&av1_ctx->pkt, (void *)buf, size)) != MPP_OK)
        goto _err_exit;

    av1_ctx->stream_data = buf;
    av1_ctx->stream_size = size;
    mpp_packet_set_size(av1_ctx->pkt, size);
    mpp_packet_set_length(av1_ctx->pkt, 0);

    return ret;

_err_exit:
    av1d_deinit(av1_ctx);
    return ret;
}

/*!
 ***********************************************************************
 * \brief
 *   reset
 ***********************************************************************
 */
MPP_RET av1d_reset(void *ctx)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)ctx;

    return av1d_paser_reset(av1_ctx);
}

/*!
 ***********************************************************************
 * \brief
 *   flush
 ***********************************************************************
 */
MPP_RET av1d_flush(void *ctx)
{
    (void)ctx;

    return MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   prepare
 ***********************************************************************
 */
MPP_RET av1d_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)ctx;
    Av1Codec *s = (Av1Codec *)av1_ctx->priv_data;
    RK_S64 pts = mpp_packet_get_pts(pkt);
    RK_S64 dts = mpp_packet_get_dts(pkt);
    RK_U8 *buf = NULL;
    RK_S32 length = (RK_S32)mpp_packet_get_length(pkt);
    RK_U8 *out_data = NULL;
    RK_S32 out_size = -1;
    RK_S32 consumed = 0;
    RK_U8 *pos = NULL;
    RK_U32 need_split = av1_ctx->cfg->base.split_parse;
    MPP_RET ret = MPP_OK;

    need_split = 1;
    task->valid = 0;
    av1_ctx->is_new_frame = 0;

    buf = pos = mpp_packet_get_pos(pkt);
    if (mpp_packet_get_flag(pkt)& MPP_PACKET_FLAG_EXTRA_DATA) {
        s->extra_has_frame = 0;
        task = NULL;
        s->current_obu.data = buf;
        s->current_obu.data_size = length;
        ret = av1d_parser_fragment_header(&s->current_obu);
        if (ret < 0) {
            return ret;
        }
        ret = av1d_get_fragment_units(s, &s->current_obu);
        if (ret < 0) {
            return ret;
        }
        ret = av1d_read_fragment_uints(s, &s->current_obu);
        if (ret < 0) {
            return ret;
        }
        if (!s->seq_header) {
            goto end;
        }
        ret = av1d_set_context_with_seq(av1_ctx, s->seq_header);
    end:
        pos = buf + length;
        mpp_packet_set_pos(pkt, pos);
        av1d_fragment_reset(&s->current_obu);
        return ret;
    }

    if (need_split) {
        consumed = av1d_split_frame(av1_ctx, &out_data, &out_size, buf, length);
    } else {
        out_size = consumed = length;
        av1_ctx->is_new_frame = 1;
    }

    pos += (consumed >= 0) ? consumed : length;

    mpp_packet_set_pos(pkt, pos);
    mpp_packet_set_length(pkt, length - consumed);
    if (!mpp_packet_get_length(pkt))
        av1_ctx->eos = mpp_packet_get_eos(pkt);
    av1d_dbg(AV1D_DBG_STRMIN, "pkt_len=%d, pts=%lld , out_size %d consumed %d new frame %d eos %d\n",
             length, pts, out_size, consumed, av1_ctx->is_new_frame, av1_ctx->eos);
    if (out_size > 0) {
        av1d_get_frame_stream(av1_ctx, buf, consumed);
        task->input_packet = av1_ctx->pkt;
        mpp_packet_set_pts(av1_ctx->pkt, pts);
        mpp_packet_set_dts(av1_ctx->pkt, dts);
    } else {
        task->valid = 0;
        if (av1_ctx->eos)
            task->input_packet = av1_ctx->pkt;
    }
    if (av1_ctx->eos && !mpp_packet_get_length(pkt))
        task->flags.eos = av1_ctx->eos;

    if (av1_ctx->is_new_frame || (task->flags.eos)) {
        if (av1_ctx->stream_offset > 0)
            task->valid = 1;
        av1_ctx->stream_offset = 0;
    }

    return ret = MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   parser
 ***********************************************************************
 */
MPP_RET av1d_parser(void *ctx, HalDecTask *in_task)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)ctx;

    av1d_parser_frame(av1_ctx, in_task);
    return MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   control
 ***********************************************************************
 */
MPP_RET av1d_control(void *ctx, MpiCmd cmd, void *param)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)ctx;
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    switch (cmd) {
    case MPP_DEC_SET_OUTPUT_FORMAT : {
        av1_ctx->usr_set_fmt = param ? *((MppFrameFormat *)param) : MPP_FMT_YUV420SP;
    } break;
    default : {
    } break;
    }

    return ret;
}

/*!
 ***********************************************************************
 * \brief
 *   callback
 ***********************************************************************
 */
MPP_RET av1d_callback(void *decoder, void *info)
{
    Av1DecCtx *av1_ctx = (Av1DecCtx *)decoder;

    av1d_parser_update(av1_ctx, info);

    return MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   api struct interface
 ***********************************************************************
 */
static const ParserApi mpp_av1d = {
    .name = "av1d_parse",
    .coding = MPP_VIDEO_CodingAV1,
    .ctx_size = sizeof(Av1DecCtx),
    .flag = 0,
    .init = av1d_init,
    .deinit = av1d_deinit,
    .prepare = av1d_prepare,
    .parse = av1d_parser,
    .reset = av1d_reset,
    .flush = av1d_flush,
    .control = av1d_control,
    .callback = av1d_callback,
};

MPP_PARSER_API_REGISTER(mpp_av1d);
