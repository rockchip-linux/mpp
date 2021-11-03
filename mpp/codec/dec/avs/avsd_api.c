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

#define MODULE_TAG "avsd_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_packet_impl.h"

#include "avsd_syntax.h"
#include "avsd_api.h"
#include "avsd_parse.h"
#include "mpp_dec_cb_param.h"

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
    MPP_FREE(p_dec->streambuf);
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
    mpp_buf_slot_setup(p_dec->frame_slots, 12);
    p_dec->mem = mpp_calloc(AvsdMemory_t, 1);
    MEM_CHECK(ret, p_dec->mem);

    p_dec->syn = &p_dec->mem->syntax;
    p_dec->stream_size = MAX_STREAM_SIZE;
    p_dec->streambuf = mpp_malloc(RK_U8, p_dec->stream_size);
    MEM_CHECK(ret, p_dec->streambuf);
    mpp_packet_init(&p_dec->task_pkt, p_dec->streambuf, p_dec->stream_size);

    mpp_packet_set_length(p_dec->task_pkt, 0);
    MEM_CHECK(ret, p_dec->task_pkt);
    for (i = 0; i < 3; i++) {
        AvsdFrame_t *frm = &p_dec->mem->save[i];

        memset(frm, 0, sizeof(*frm));
        frm->idx = i;
        frm->slot_idx = -1;
    }
    p_dec->bx = &p_dec->mem->bitctx;
    p_dec->need_split = 1;

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
    p_dec->got_vsh = 0;
    p_dec->got_ph = 0;
    p_dec->got_keyframe = 0;
    p_dec->vec_flag = 0;
    p_dec->got_eos = 0;
    p_dec->left_length = 0;
    p_dec->state = 0xFFFFFFFF;
    p_dec->vop_header_found = 0;

    mpp_packet_set_length(p_dec->task_pkt, 0);
    mpp_packet_set_flag(p_dec->task_pkt, 0);

    AVSD_PARSE_TRACE("Out.");

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
    p_dec->got_eos = 0;
    p_dec->left_length = 0;
    p_dec->state = 0xFFFFFFFF;
    p_dec->vop_header_found = 0;

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
MPP_RET avsd_control(void *decoder, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;

    AVSD_PARSE_TRACE("In.");

    switch (cmd_type) {
    case MPP_DEC_SET_DISABLE_ERROR: {
        p_dec->disable_error = *((RK_U32 *)param);
    } break;
    default : {
    } break;
    }

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

    AVSD_PARSE_TRACE("In.");
    INP_CHECK(ret, !decoder && !pkt && !task);

    AvsdCtx_t *p_dec = (AvsdCtx_t *)decoder;
    RK_U8 *pos = mpp_packet_get_pos(pkt);
    size_t length = mpp_packet_get_length(pkt);
    RK_U32 eos = mpp_packet_get_eos(pkt);

    task->valid = 0;
    if (p_dec->got_eos) {
        AVSD_DBG(AVSD_DBG_INPUT, "got eos packet.\n");
        mpp_packet_set_length(pkt, 0);
        goto __RETURN;
    }
    AVSD_DBG(AVSD_DBG_INPUT, "[pkt_in] pts=%lld, eos=%d, len=%d, pkt_no=%d\n",
             mpp_packet_get_pts(pkt), eos, (RK_U32)length, (RK_U32)p_dec->pkt_no);
    p_dec->pkt_no++;

    if (mpp_packet_get_eos(pkt)) {
        if (mpp_packet_get_length(pkt) < 4) {
            avsd_flush(decoder);
        }
        p_dec->got_eos = 1;
        task->flags.eos = p_dec->got_eos;
        goto __RETURN;
    }
    if (mpp_packet_get_length(pkt) > MAX_STREAM_SIZE) {
        AVSD_DBG(AVSD_DBG_ERROR, "[pkt_in_timeUs] input error, stream too large");
        mpp_packet_set_length(pkt, 0);
        ret = MPP_NOK;
        goto __RETURN;
    }

    mpp_packet_set_length(p_dec->task_pkt, p_dec->left_length);

    RK_U32 total_length = MPP_ALIGN(p_dec->left_length + length, 16) + 64;
    if (total_length > p_dec->stream_size) {
        do {
            p_dec->stream_size <<= 1;
        } while (total_length > p_dec->stream_size);

        RK_U8 *dst = mpp_malloc_size(RK_U8, p_dec->stream_size);
        mpp_assert(dst);

        if (p_dec->left_length > 0) {
            memcpy(dst, p_dec->streambuf, p_dec->left_length);
        }
        mpp_free(p_dec->streambuf);
        p_dec->streambuf = dst;

        mpp_packet_set_data(p_dec->task_pkt, p_dec->streambuf);
        mpp_packet_set_size(p_dec->task_pkt, p_dec->stream_size);
    }

    if (!p_dec->need_split) {
        p_dec->got_eos = eos;
        // empty eos packet
        if (eos && (length < 4)) {
            avsd_flush(decoder);
            goto __RETURN;
        }
        // copy packet direct
        memcpy(p_dec->streambuf, pos, length);
        mpp_packet_set_data(p_dec->task_pkt, p_dec->streambuf);
        mpp_packet_set_length(p_dec->task_pkt, length);
        mpp_packet_set_pts(p_dec->task_pkt, mpp_packet_get_pts(pkt));
        // set input packet length to 0 here
        mpp_packet_set_length(pkt, 0);
        /* this step will enable the task and goto parse stage */
        task->valid = 1;
    } else {
        /* Split packet mode */
        if (MPP_OK == avsd_parser_split(p_dec, p_dec->task_pkt, pkt)) {
            p_dec->left_length = 0;
            task->valid = 1;
        } else {
            task->valid = 0;
            p_dec->left_length = mpp_packet_get_length(p_dec->task_pkt);
        }
        p_dec->got_eos = mpp_packet_get_eos(p_dec->task_pkt);
    }
    task->input_packet = p_dec->task_pkt;
    task->flags.eos = p_dec->got_eos;

__RETURN:
    AVSD_PARSE_TRACE("Out.");
    return ret = MPP_OK;
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
    if (p_dec->disable_error) {
        task->flags.ref_err = 0;
        task->flags.parse_err = 0;
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
    DecCbHalDone *ctx = (DecCbHalDone *)info;

    AVSD_PARSE_TRACE("In.");
    if (!p_dec->disable_error) {
        MppFrame mframe = NULL;
        HalDecTask *task_dec = (HalDecTask *)ctx->task;

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
    .coding = MPP_VIDEO_CodingAVS,
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

const ParserApi api_avsd_plus_parser = {
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
