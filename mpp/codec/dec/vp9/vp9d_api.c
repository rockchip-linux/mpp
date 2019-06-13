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

#define MODULE_TAG "vp9d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_packet_impl.h"

#include "vp9d_codec.h"
#include "vp9d_parser.h"
#include "vp9d_api.h"

/*!
***********************************************************************
* \brief
*   alloc all buffer
***********************************************************************
*/

MPP_RET vp9d_init(void *ctx, ParserCfg *init)
{
    MPP_RET ret = MPP_OK;
    RK_U8 *buf = NULL;
    RK_S32 size = SZ_512K;
    Vp9CodecContext *vp9_ctx = (Vp9CodecContext *)ctx;

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
*   free all buffer
***********************************************************************
*/
MPP_RET vp9d_deinit(void *ctx)
{
    RK_U8 *buf = NULL;
    Vp9CodecContext *vp9_ctx = (Vp9CodecContext *)ctx;

    if (vp9_ctx) {
        vp9d_parser_deinit(vp9_ctx);
        vp9d_split_deinit(vp9_ctx);
        if (vp9_ctx->pkt) {
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
*   reset
***********************************************************************
*/
MPP_RET  vp9d_reset(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    Vp9CodecContext *vp9_ctx = (Vp9CodecContext *)ctx;
    vp9d_paser_reset(vp9_ctx);
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET  vp9d_flush(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)ctx;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET  vp9d_control(void *ctx, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)ctx;
    (void)cmd_type;
    (void)param;

    return ret = MPP_OK;
}


/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/
MPP_RET vp9d_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    Vp9CodecContext *vp9_ctx = (Vp9CodecContext *)ctx;
    SplitContext_t *ps = (SplitContext_t *)vp9_ctx->priv_data2;
    RK_S64 pts = -1;
    RK_S64 dts = -1;
    RK_U8 *buf = NULL;
    RK_S32 length = 0;
    RK_U8 *out_data = NULL;
    RK_S32 out_size = -1;
    RK_S32 consumed = 0;
    RK_U8 *pos = NULL;
    task->valid = -1;

    pts = mpp_packet_get_pts(pkt);
    dts = mpp_packet_get_dts(pkt);
    vp9_ctx->eos = mpp_packet_get_eos(pkt);
    buf = pos = mpp_packet_get_pos(pkt);
    length = (RK_S32)mpp_packet_get_length(pkt);

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
MPP_RET vp9d_parse(void *ctx, HalDecTask *in_task)
{
    Vp9CodecContext *vp9_ctx = (Vp9CodecContext *)ctx;
    MPP_RET ret = MPP_OK;
    vp9_parser_frame(vp9_ctx, in_task);

    return ret;
}
/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
MPP_RET vp9d_callback(void *decoder, void *info)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Vp9CodecContext *vp9_ctx = (Vp9CodecContext *)decoder;
    vp9_parser_update(vp9_ctx, info);

    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/

const ParserApi api_vp9d_parser = {
    .name = "vp9d_parse",
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(Vp9CodecContext),
    .flag = 0,
    .init = vp9d_init,
    .deinit = vp9d_deinit,
    .prepare = vp9d_prepare,
    .parse = vp9d_parse,
    .reset = vp9d_reset,
    .flush = vp9d_flush,
    .control = vp9d_control,
    .callback = vp9d_callback,
};

