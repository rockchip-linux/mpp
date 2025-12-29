/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vp9d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_packet_impl.h"

#include "mpp_parser.h"
#include "vp9d_codec.h"
#include "vp9d_parser.h"

/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET vp9d_deinit(void *ctx)
{
    Vp9DecCtx *vp9_ctx = (Vp9DecCtx *)ctx;

    if (vp9_ctx) {
        vp9d_parser_deinit(vp9_ctx);
        vp9d_split_deinit(vp9_ctx);
        if (vp9_ctx->pkt) {
            RK_U8 *buf;

            buf = mpp_packet_get_data(vp9_ctx->pkt);
            MPP_FREE(buf);
            mpp_packet_deinit(&vp9_ctx->pkt);
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
MPP_RET vp9d_init(void *ctx, ParserCfg *init)
{
    Vp9DecCtx *vp9_ctx = (Vp9DecCtx *)ctx;
    RK_U8 *buf = NULL;
    RK_S32 size = SZ_512K;
    MPP_RET ret = MPP_OK;

    if (!vp9_ctx || !init) {
        mpp_err("vp9d init fail");
        return MPP_ERR_NULL_PTR;
    }

    if ((ret = vp9d_parser_init(vp9_ctx, init)) != MPP_OK)
        goto _err_exit;

    if ((ret = vp9d_split_init(vp9_ctx)) != MPP_OK)
        goto _err_exit;

    buf = mpp_malloc(RK_U8, size);
    if (!buf) {
        mpp_err("vp9 init malloc stream buffer fail");
        ret = MPP_ERR_NOMEM;
        goto _err_exit;
    }

    if ((ret = mpp_packet_init(&vp9_ctx->pkt, (void *)buf, size)) != MPP_OK)
        goto _err_exit;

    return ret;

_err_exit:
    vp9d_deinit(vp9_ctx);
    return ret;
}

/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET vp9d_reset(void *ctx)
{
    Vp9DecCtx *vp9_ctx = (Vp9DecCtx *)ctx;

    return vp9d_paser_reset(vp9_ctx);
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET vp9d_flush(void *ctx)
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
MPP_RET vp9d_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    Vp9DecCtx *vp9_ctx = (Vp9DecCtx *)ctx;
    VP9SplitCtx *ps = (VP9SplitCtx *)vp9_ctx->priv_data2;
    RK_S64 pts = mpp_packet_get_pts(pkt);
    RK_S64 dts = mpp_packet_get_dts(pkt);
    RK_U8 *buf = NULL;
    RK_S32 length = (RK_S32)mpp_packet_get_length(pkt);
    RK_U8 *out_data = NULL;
    RK_S32 out_size = -1;
    RK_S32 consumed = 0;
    RK_U8 *pos = NULL;

    task->valid = -1;

    vp9_ctx->eos = mpp_packet_get_eos(pkt);
    buf = pos = mpp_packet_get_pos(pkt);
    consumed = vp9d_split_frame(ps, &out_data, &out_size, buf, length);
    pos += (consumed >= 0) ? consumed : length;

    mpp_packet_set_pos(pkt, pos);
    vp9d_dbg(VP9D_DBG_STRMIN, "pkt_len=%d, pts=%lld\n", length, pts);
    if (out_size > 0) {
        vp9d_get_frame_stream(vp9_ctx, out_data, out_size);
        task->input_packet = vp9_ctx->pkt;
        task->valid = 1;
        mpp_packet_set_pts(vp9_ctx->pkt, pts);
        mpp_packet_set_dts(vp9_ctx->pkt, dts);
        task->flags.eos = vp9_ctx->eos;
    } else {
        task->valid = 0;
        task->flags.eos = vp9_ctx->eos;
    }

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   parser
***********************************************************************
*/
MPP_RET vp9d_parse(void *ctx, HalDecTask *in_task)
{
    Vp9DecCtx *vp9_ctx = (Vp9DecCtx *)ctx;

    vp9_parser_frame(vp9_ctx, in_task);

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
MPP_RET vp9d_callback(void *decoder, void *info)
{
    Vp9DecCtx *vp9_ctx = (Vp9DecCtx *)decoder;

    vp9_parser_update(vp9_ctx, info);

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/
const ParserApi mpp_vp9d = {
    .name = "vp9d_parse",
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(Vp9DecCtx),
    .flag = 0,
    .init = vp9d_init,
    .deinit = vp9d_deinit,
    .prepare = vp9d_prepare,
    .parse = vp9d_parse,
    .reset = vp9d_reset,
    .flush = vp9d_flush,
    .control = NULL,
    .callback = vp9d_callback,
};

MPP_PARSER_API_REGISTER(mpp_vp9d);
