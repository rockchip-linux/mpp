/*
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

#define MODULE_TAG "av1d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_packet_impl.h"

#include "av1d_codec.h"
#include "av1d_parser.h"

#include "av1d_api.h"

/*!
 ***********************************************************************
 * \brief
 *   alloc all buffer
 ***********************************************************************
 */

MPP_RET av1d_init(void *ctx, ParserCfg *init)
{
    MPP_RET ret = MPP_OK;
    RK_U8 *buf = NULL;
    RK_S32 size = SZ_512K;
    Av1CodecContext *av1_ctx = (Av1CodecContext *)ctx;

    if (!av1_ctx || !init) {
        mpp_err("av1d init fail");
        return MPP_ERR_NULL_PTR;
    }

    av1_ctx->pix_fmt = MPP_FMT_BUTT;
    av1_ctx->usr_set_fmt = MPP_FMT_BUTT;

    if ((ret = av1d_parser_init(av1_ctx, init)) != MPP_OK)
        goto _err_exit;

    if ((ret = av1d_split_init(av1_ctx)) != MPP_OK)
        goto _err_exit;

    buf = mpp_malloc(RK_U8, size);
    if (!buf) {
        mpp_err("av1d init malloc stream buffer fail");
        ret = MPP_ERR_NOMEM;
        goto _err_exit;
    }

    if ((ret = mpp_packet_init(&av1_ctx->pkt, (void *)buf, size)) != MPP_OK)
        goto _err_exit;

    av1_ctx->stream = buf;
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
 *   free all buffer
 ***********************************************************************
 */
MPP_RET av1d_deinit(void *ctx)
{
    RK_U8 *buf = NULL;
    Av1CodecContext *av1_ctx = (Av1CodecContext *)ctx;

    if (av1_ctx) {
        av1d_parser_deinit(av1_ctx);
        av1d_split_deinit(av1_ctx);
        if (av1_ctx->pkt) {
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
 *   reset
 ***********************************************************************
 */
MPP_RET  av1d_reset(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    Av1CodecContext *av1_ctx = (Av1CodecContext *)ctx;
    av1d_paser_reset(av1_ctx);
    return ret = MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   flush
 ***********************************************************************
 */
MPP_RET  av1d_flush(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)ctx;
    return ret = MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   prepare
 ***********************************************************************
 */
MPP_RET av1d_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    Av1CodecContext *av1_ctx = (Av1CodecContext *)ctx;
    AV1Context *s = (AV1Context *)av1_ctx->priv_data;
    RK_S64 pts = -1;
    RK_S64 dts = -1;
    RK_U8 *buf = NULL;
    RK_S32 length = 0;
    RK_U8 *out_data = NULL;
    RK_S32 out_size = -1;
    RK_S32 consumed = 0;
    RK_U8 *pos = NULL;
    RK_U32 need_split = s->cfg->base.split_parse;

    need_split = 1;
    task->valid = 0;
    av1_ctx->new_frame = 0;

    pts = mpp_packet_get_pts(pkt);
    dts = mpp_packet_get_dts(pkt);
    buf = pos = mpp_packet_get_pos(pkt);
    length = (RK_S32)mpp_packet_get_length(pkt);
    if (mpp_packet_get_flag(pkt)& MPP_PACKET_FLAG_EXTRA_DATA) {
        s->extra_has_frame = 0;
        task = NULL;
        s->current_obu.data = buf;
        s->current_obu.data_size = length;
        ret = mpp_av1_split_fragment(s, &s->current_obu, 1);
        if (ret < 0) {
            return ret;
        }
        ret = mpp_av1_read_fragment_content(s, &s->current_obu);
        if (ret < 0) {
            return ret;
        }
        if (!s->sequence_header) {
            goto end;
        }
        ret = mpp_av1_set_context_with_sequence(av1_ctx, s->sequence_header);
    end:
        pos = buf + length;
        mpp_packet_set_pos(pkt, pos);
        mpp_av1_fragment_reset(&s->current_obu);
        return ret;
    }

    if (need_split) {
        consumed = av1d_split_frame(av1_ctx, &out_data, &out_size, buf, length);
    } else {
        out_size = consumed = length;
        av1_ctx->new_frame = 1;
    }

    pos += (consumed >= 0) ? consumed : length;

    mpp_packet_set_pos(pkt, pos);
    mpp_packet_set_length(pkt, length - consumed);
    if (!mpp_packet_get_length(pkt))
        av1_ctx->eos = mpp_packet_get_eos(pkt);
    av1d_dbg(AV1D_DBG_STRMIN, "pkt_len=%d, pts=%lld , out_size %d consumed %d new frame %d eos %d\n",
             length, pts, out_size, consumed, av1_ctx->new_frame, av1_ctx->eos);
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

    if (av1_ctx->new_frame || (task->flags.eos)) {
        if (av1_ctx->stream_offset > 0)
            task->valid = 1;
        av1_ctx->stream_offset = 0;
    }

    (void)pts;
    (void)dts;
    (void)task;
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
    Av1CodecContext *av1_ctx = (Av1CodecContext *)ctx;
    MPP_RET ret = MPP_OK;
    av1d_parser_frame(av1_ctx, in_task);
    return ret;
}

/*!
 ***********************************************************************
 * \brief
 *   control
 ***********************************************************************
 */
MPP_RET av1d_control(void *ctx, MpiCmd cmd, void *param)
{
    Av1CodecContext *av1_ctx = (Av1CodecContext *)ctx;
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    switch (cmd) {
    case MPP_DEC_SET_OUTPUT_FORMAT : {
        av1_ctx->usr_set_fmt = param ? *((MppFrameFormat *)param) : MPP_FMT_YUV420SP;
    } break;
    default:
        break;
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
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1CodecContext *av1_ctx = (Av1CodecContext *)decoder;
    av1d_parser_update(av1_ctx, info);

    return ret = MPP_OK;
}

/*!
 ***********************************************************************
 * \brief
 *   api struct interface
 ***********************************************************************
 */

const ParserApi api_av1d_parser = {
    .name = "av1d_parse",
    .coding = MPP_VIDEO_CodingAV1,
    .ctx_size = sizeof(Av1CodecContext),
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
