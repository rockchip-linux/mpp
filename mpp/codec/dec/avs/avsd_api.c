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

#define MODULE_TAG "avsd_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer_impl.h"

#include "avsd_syntax.h"
#include "avsd_api.h"
#include "avsd_parse.h"


RK_U32 avsd_parse_debug = 0;


/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET avsd_deinit(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;

    INP_CHECK(ret, !decoder);
    AVSD_PARSE_TRACE("In.");

    mpp_packet_deinit(&p_dec->task_pkt);
    MPP_FREE(p_dec->p_stream->pbuf);
    MPP_FREE(p_dec->p_header->pbuf);
    MPP_FREE(p_dec->mem);

__RETURN:
    (void)decoder;
    AVSD_PARSE_TRACE("Out.");
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   alloc all buffer
***********************************************************************
*/

MPP_RET avsd_init(void *decoder, ParserCfg *init)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_S32 i = 0;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;

    AVSD_PARSE_TRACE("In.");
    INP_CHECK(ret, !p_dec);

    memset(p_dec, 0, sizeof(AvsdCtx_t));
    mpp_env_get_u32("avsd_debug", &avsd_parse_debug, 0);
    //!< restore init parameters
    p_dec->init = *init;
    p_dec->frame_slots = init->frame_slots;
    p_dec->packet_slots = init->packet_slots;
    //!< decoder parameters
    mpp_buf_slot_setup(p_dec->frame_slots, 5);
    p_dec->mem = mpp_calloc(AvsdMemory_t, 1);
    MEM_CHECK(ret, p_dec->mem);
    p_dec->p_header = &p_dec->mem->headerbuf;
    p_dec->p_header->size = MAX_HEADER_SIZE;
    p_dec->p_header->pbuf = mpp_malloc(RK_U8, p_dec->p_header->size);
    MEM_CHECK(ret, p_dec->p_header->pbuf);

    p_dec->syn = &p_dec->mem->syntax;
    p_dec->p_stream = &p_dec->mem->streambuf;
    p_dec->p_stream->size = MAX_STREAM_SIZE;
    p_dec->p_stream->pbuf = mpp_malloc(RK_U8, p_dec->p_stream->size);
    MEM_CHECK(ret, p_dec->p_stream->pbuf);
    mpp_packet_init(&p_dec->task_pkt, p_dec->p_stream->pbuf, p_dec->p_stream->size);

    mpp_packet_set_length(p_dec->task_pkt, 0);
    MEM_CHECK(ret, p_dec->task_pkt);
    for (i = 0; i < 3; i++) {
        memset(&p_dec->mem->save[i], 0, sizeof(AvsdFrame_t));
        p_dec->mem->save[i].slot_idx = -1;
    }
__RETURN:
    AVSD_PARSE_TRACE("Out.");
    return ret = MPP_OK;
__FAILED:
    avsd_deinit(decoder);

    return ret;
}
/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET avsd_reset(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;

    AVSD_PARSE_TRACE("In.");

    avsd_reset_parameters(p_dec);
    p_dec->got_keyframe = 0;
    p_dec->vec_flag = 0;

    AVSD_PARSE_TRACE("Out.");
    (void)p_dec;
    (void)decoder;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET avsd_flush(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;
    AVSD_PARSE_TRACE("In.");

    avsd_reset_parameters(p_dec);
    p_dec->got_keyframe = 0;
    p_dec->vec_flag = 0;

    AVSD_PARSE_TRACE("Out.");
    (void)p_dec;
    (void)decoder;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET avsd_control(void *decoder, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");


    (void)decoder;
    (void)cmd_type;
    (void)param;
    AVSD_PARSE_TRACE("Out.");
    return ret = MPP_OK;
}


/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/
MPP_RET avsd_prepare(void *decoder, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t   *p_dec = (AvsdCtx_t *)decoder;

    AVSD_PARSE_TRACE("In.");
    INP_CHECK(ret, !decoder && !pkt && !task);

    if (p_dec->has_get_eos
        || (mpp_packet_get_flag(pkt) & MPP_PACKET_FLAG_EXTRA_DATA)) {
        mpp_packet_set_length(pkt, 0);
        goto __RETURN;
    }
    AVSD_DBG(AVSD_DBG_INPUT, "[pkt_in_timeUs] in_pts=%lld, pkt_eos=%d, len=%d, pkt_no=%d\n",
             mpp_packet_get_pts(pkt), (RK_U32)mpp_packet_get_eos(pkt), (RK_U32)mpp_packet_get_length(pkt), (RK_U32)p_dec->pkt_no);
    p_dec->pkt_no++;

    if (mpp_packet_get_eos(pkt)) {
        if (mpp_packet_get_length(pkt) < 4) {
            avsd_flush(decoder);
        }
        p_dec->has_get_eos = 1;
    }
    if (mpp_packet_get_length(pkt) > MAX_STREAM_SIZE) {
        AVSD_DBG(AVSD_DBG_ERROR, "[pkt_in_timeUs] input error, stream too large");
        mpp_packet_set_length(pkt, 0);
        ret = MPP_NOK;
        goto __FAILED;
    }

    task->valid = 0;
    do {
        (ret = avsd_parse_prepare(p_dec, pkt, task));

    } while (mpp_packet_get_length(pkt) && !task->valid);

    if (task->valid) {
        //!< bit stream
        RK_U32 align_len = MPP_ALIGN(p_dec->p_stream->len + 32, 16);

        mpp_assert(p_dec->p_stream->size > align_len);
        memset(p_dec->p_stream->pbuf + p_dec->p_stream->len,
               0, align_len - p_dec->p_stream->len);

        p_dec->syn->bitstream_size = align_len;
        p_dec->syn->bitstream = p_dec->p_stream->pbuf;

        mpp_packet_set_data(p_dec->task_pkt, p_dec->syn->bitstream);
        mpp_packet_set_length(p_dec->task_pkt, p_dec->syn->bitstream_size);
        mpp_packet_set_size(p_dec->task_pkt, p_dec->p_stream->size);

        mpp_packet_set_pts(p_dec->task_pkt, mpp_packet_get_pts(pkt));
        mpp_packet_set_dts(p_dec->task_pkt, mpp_packet_get_dts(pkt));
        task->input_packet = p_dec->task_pkt;

        p_dec->p_stream->len = 0;
        p_dec->p_header->len = 0;
    } else {
        task->input_packet = NULL;
    }

__RETURN:
    (void)decoder;
    (void)pkt;
    (void)task;
    AVSD_PARSE_TRACE("Out.");
    return ret = MPP_OK;
__FAILED:
    return ret;
}


/*!
***********************************************************************
* \brief
*   parser
***********************************************************************
*/
MPP_RET avsd_parse(void *decoder, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;
    AVSD_PARSE_TRACE("In.");

    task->valid = 0;
    memset(task->refer, -1, sizeof(task->refer));

    avsd_parse_stream(p_dec, task);
    if (task->valid) {
        avsd_fill_parameters(p_dec, p_dec->syn);
        avsd_set_dpb(p_dec, task);

        avsd_commit_syntaxs(p_dec->syn, task);
        avsd_update_dpb(p_dec);
    }

    AVSD_PARSE_TRACE("Out.");

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
MPP_RET avsd_callback(void *decoder, void *info)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;
    IOCallbackCtx *ctx = (IOCallbackCtx *)info;

    AVSD_PARSE_TRACE("In.");
    {
        MppFrame mframe = NULL;
        HalDecTask *task_dec = (HalDecTask *)ctx->task;
        AVSD_DBG(AVSD_DBG_CALLBACK, "reg[1]=%08x, ref=%d, dpberr=%d, harderr-%d\n",
                 ctx->regs[1], task_dec->flags.used_for_ref, task_dec->flags.ref_err, ctx->hard_err);

        mpp_buf_slot_get_prop(p_dec->frame_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);
        if (mframe) {
            if (ctx->hard_err || task_dec->flags.ref_err) {
                if (task_dec->flags.used_for_ref) {
                    mpp_frame_set_errinfo(mframe, MPP_FRAME_FLAG_PAIRED_FIELD);
                } else {
                    mpp_frame_set_discard(mframe, MPP_FRAME_FLAG_PAIRED_FIELD);
                }
            }
        }
    }
    AVSD_PARSE_TRACE("Out.");
    (void)p_dec;
    (void)ctx;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/
const ParserApi api_avsd_parser = {
    .name = "avsd_parse",
    .coding = MPP_VIDEO_CodingAVSPLUS,
    .ctx_size = sizeof(AvsdCtx_t),
    .flag = 0,
    .init = avsd_init,
    .deinit = avsd_deinit,
    .prepare = avsd_prepare,
    .parse = avsd_parse,
    .reset = avsd_reset,
    .flush = avsd_flush,
    .control = avsd_control,
    .callback = avsd_callback,
};

