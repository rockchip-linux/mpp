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

#define MODULE_TAG "avs2d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_debug.h"
#include "mpp_env.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer_impl.h"

#include "avs2d_syntax.h"
#include "avs2d_api.h"
#include "avs2d_parse.h"
#include "avs2d_dpb.h"
#include "mpp_dec_cb_param.h"

RK_U32 avs2d_parse_debug = 0;

MPP_RET avs2d_deinit(void *decoder)
{
    MPP_RET ret = MPP_OK;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;

    INP_CHECK(ret, !decoder);
    AVS2D_PARSE_TRACE("In.");

    MPP_FREE(p_dec->p_stream->pbuf);
    MPP_FREE(p_dec->p_header->pbuf);
    MPP_FREE(p_dec->mem);
    mpp_packet_deinit(&p_dec->task_pkt);
    avs2d_dpb_destroy(p_dec);

__RETURN:
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_init(void *decoder, ParserCfg *init)
{
    MPP_RET ret = MPP_OK;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;

    AVS2D_PARSE_TRACE("In.");
    INP_CHECK(ret, !p_dec);

    memset(p_dec, 0, sizeof(Avs2dCtx_t));
    mpp_env_get_u32("avs2d_debug", &avs2d_parse_debug, 0);
    //!< restore init parameters
    p_dec->init = *init;
    // p_dec->init.cfg->base.split_parse = 1;
    p_dec->frame_slots = init->frame_slots;
    p_dec->packet_slots = init->packet_slots;
    //!< decoder parameters
    mpp_buf_slot_setup(p_dec->frame_slots, AVS2_MAX_BUF_NUM);

    p_dec->mem = mpp_calloc(Avs2dMemory_t, 1);
    MEM_CHECK(ret, p_dec->mem);
    p_dec->p_header = &p_dec->mem->headerbuf;
    p_dec->p_header->size = MAX_HEADER_SIZE;
    p_dec->p_header->pbuf = mpp_malloc(RK_U8, p_dec->p_header->size);
    MEM_CHECK(ret, p_dec->p_header->pbuf);

    p_dec->p_stream = &p_dec->mem->streambuf;
    p_dec->p_stream->size = MAX_STREAM_SIZE;
    p_dec->p_stream->pbuf = mpp_malloc(RK_U8, p_dec->p_stream->size);
    MEM_CHECK(ret, p_dec->p_stream->pbuf);

    mpp_packet_init(&p_dec->task_pkt, p_dec->p_stream->pbuf, p_dec->p_stream->size);
    mpp_packet_set_length(p_dec->task_pkt, 0);
    MEM_CHECK(ret, p_dec->task_pkt);

    // avs2d_dpb_create(p_dec);

__RETURN:
    AVS2D_PARSE_TRACE("Out.");
    return ret;
__FAILED:
    avs2d_deinit(decoder);

    return ret;
}

MPP_RET avs2d_reset(void *decoder)
{
    MPP_RET ret = MPP_OK;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;

    AVS2D_PARSE_TRACE("In.");

    //!< flush dpb
    avs2d_dpb_flush(p_dec);

    //!< reset parser parameters
    avs2d_reset_parser(p_dec);

    //!< reset decoder parameters
    p_dec->pkt_no      = 0;
    p_dec->frame_no    = 0;
    p_dec->has_get_eos = 0;
    p_dec->nal         = NULL;

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_flush(void *decoder)
{
    MPP_RET ret = MPP_OK;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;
    AVS2D_PARSE_TRACE("In.");

    dpb_remove_unused_frame(p_dec);

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_control(void *decoder, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    AVS2D_PARSE_TRACE("In.");
    (void)decoder;
    (void)cmd_type;
    (void)param;
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}


MPP_RET avs2d_prepare(void *decoder, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;
    RK_U8 *buf = NULL;
    RK_S64 pts = -1;
    RK_S64 dts = -1;
    RK_U32 length = 0;
    RK_U32 pkt_eos = 0;

    AVS2D_PARSE_TRACE("In.");
    INP_CHECK(ret, !decoder && !pkt && !task);

    task->valid = 0;

    pkt_eos = mpp_packet_get_eos(pkt);

    buf = (RK_U8 *)mpp_packet_get_pos(pkt);
    pts = mpp_packet_get_pts(pkt);
    dts = mpp_packet_get_dts(pkt);
    length = (RK_U32)mpp_packet_get_length(pkt);

    AVS2D_DBG(AVS2D_DBG_INPUT, "[pkt_in_timeUs] in_pts=%lld, dts=%lld, len=%d, eos=%d, pkt_no=%lld\n",
              pts, dts, length, pkt_eos, p_dec->pkt_no);
    p_dec->pkt_no++;

    AVS2D_DBG(AVS2D_DBG_INPUT, "packet length %d, eos %d, buf[0-3]=%02x %02x %02x %02x\n",
              length, p_dec->has_get_eos, buf[0], buf[1], buf[2], buf[3]);

    if (pkt_eos) {
        p_dec->has_get_eos = 1;
        if (length <= 4) {
            // skip parsing video_sequence_end_code if it exists
            task->flags.eos = 1;
            avs2d_dpb_flush(p_dec);
            goto __RETURN;
        }
    }

    if (!p_dec->init.cfg->base.split_parse) {
        ret = avs2d_parse_prepare_fast(p_dec, pkt, task);
    } else {
        ret = avs2d_parse_prepare_split(p_dec, pkt, task);
    }

    if (task->valid) {
        //!< bit stream
        RK_U32 align_len = MPP_ALIGN(p_dec->p_stream->len + 32, 16);

        mpp_assert(p_dec->p_stream->size > align_len);
        memset(p_dec->p_stream->pbuf + p_dec->p_stream->len,
               0, align_len - p_dec->p_stream->len);

        p_dec->syntax.bitstream_size = align_len;
        p_dec->syntax.bitstream = p_dec->p_stream->pbuf;
        // TODO if bistream_size is larger than p_stream->size, realloc

        mpp_packet_set_data(p_dec->task_pkt, p_dec->syntax.bitstream);
        mpp_packet_set_length(p_dec->task_pkt, p_dec->syntax.bitstream_size);
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
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_parse(void *decoder, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;
    AVS2D_PARSE_TRACE("In.");

    task->valid = 0;

    avs2d_parse_stream(p_dec, task);
    if (task->valid) {
        AVS2D_PARSE_TRACE("-------- Frame %lld--------", p_dec->frame_no);
        avs2d_dpb_insert(p_dec, task);
        avs2d_fill_parameters(p_dec, &p_dec->syntax);
        avs2d_commit_syntaxs(&p_dec->syntax, task);
        AVS2D_PARSE_TRACE("--------------------------");
    } else {
        task->flags.parse_err = 1;
    }

    AVS2D_PARSE_TRACE("Out.");

    return ret;
}

MPP_RET avs2d_callback(void *decoder, void *info)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Avs2dCtx_t *p_dec = (Avs2dCtx_t *)decoder;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;
    DecCbHalDone *ctx = (DecCbHalDone *)info;
    HalDecTask *task_dec = (HalDecTask *)ctx->task;
    MppFrame mframe = NULL;
    MppFrame ref_frm = NULL;
    RK_U32 i = 0;
    RK_U32 error = 0;
    RK_U32 discard = 0;
    RK_U32 ref_used_flag = 0;

    AVS2D_PARSE_TRACE("In.");
    mpp_buf_slot_get_prop(p_dec->frame_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);

    if (!mframe) {
        ret = MPP_ERR_UNKNOW;
        AVS2D_DBG(AVS2D_DBG_CALLBACK, "[CALLBACK]: failed to get frame\n");
        goto __FAILED;
    }

    if (ctx->hard_err || task_dec->flags.ref_err) {
        if (task_dec->flags.used_for_ref) {
            error = 1;
        } else {
            discard = 1;
        }
    } else {
        if (task_dec->flags.ref_miss & task_dec->flags.ref_used) {
            discard = 1;
            AVS2D_DBG(AVS2D_DBG_CALLBACK, "[CALLBACK]: fake ref used, miss 0x%x used 0x%x\n",
                      task_dec->flags.ref_miss, task_dec->flags.ref_used);
        }
    }

    for (i = 0; i < AVS2_MAX_REFS; i++) {
        if (!mgr->refs[i] || !mgr->refs[i]->frame || mgr->refs[i]->slot_idx < 0)
            continue;

        mpp_buf_slot_get_prop(p_dec->frame_slots, mgr->refs[i]->slot_idx, SLOT_FRAME_PTR, &ref_frm);
        if (!ref_frm)
            continue;

        ref_used_flag = (task_dec->flags.ref_used >> i) & 1;
        //TODO: In fast mode, ref list isn't kept sync with task flag.ref_used
        AVS2D_DBG(AVS2D_DBG_CALLBACK, "[CALLBACK]: ref_frm poc %d, err %d, dis %d, ref_used %d\n",
                  mpp_frame_get_poc(ref_frm), mpp_frame_get_errinfo(ref_frm),
                  mpp_frame_get_discard(ref_frm), ref_used_flag);

        if (ref_used_flag) {
            discard |= mpp_frame_get_discard(ref_frm);
            error |= mpp_frame_get_errinfo(ref_frm);
        }
    }

    mpp_frame_set_errinfo(mframe, error);
    mpp_frame_set_discard(mframe, discard);

    AVS2D_DBG(AVS2D_DBG_CALLBACK, "[CALLBACK]: frame poc %d, ref=%d, dpberr=%d, harderr=%d, err:dis=%d:%d\n",
              mpp_frame_get_poc(mframe), task_dec->flags.used_for_ref, task_dec->flags.ref_err,
              ctx->hard_err, error, discard);

__FAILED:
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

const ParserApi api_avs2d_parser = {
    .name = "avs2d_parse",
    .coding = MPP_VIDEO_CodingAVS2,
    .ctx_size = sizeof(Avs2dCtx_t),
    .flag = 0,
    .init = avs2d_init,
    .deinit = avs2d_deinit,
    .prepare = avs2d_prepare,
    .parse = avs2d_parse,
    .reset = avs2d_reset,
    .flush = avs2d_flush,
    .control = avs2d_control,
    .callback = avs2d_callback,
};
