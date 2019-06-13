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

#define MODULE_TAG "h264d_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_packet_impl.h"

#include "h264d_api.h"
#include "h264d_global.h"
#include "h264d_parse.h"
#include "h264d_sps.h"
#include "h264d_slice.h"
#include "h264d_dpb.h"
#include "h264d_init.h"

RK_U32 rkv_h264d_parse_debug = 0;

static MPP_RET free_input_ctx(H264dInputCtx_t *p_Inp)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Inp);
    close_stream_file(p_Inp);
    MPP_FREE(p_Inp->spspps_buf);

__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_input_ctx(H264dInputCtx_t *p_Inp, ParserCfg *init)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Inp && !init);

    p_Inp->init = *init;
    mpp_env_get_u32("rkv_h264d_mvc_disable", &p_Inp->mvc_disable, 1);
    open_stream_file(p_Inp, "/sdcard");
    if (rkv_h264d_parse_debug & H264D_DBG_WRITE_ES_EN) {
        p_Inp->spspps_size = HEAD_BUF_MAX_SIZE;
        p_Inp->spspps_buf = mpp_malloc_size(RK_U8, p_Inp->spspps_size);
        MEM_CHECK(ret, p_Inp->spspps_buf);
    }

__RETURN:
    return ret = MPP_OK;
__FAILED:
    free_input_ctx(p_Inp);
    return ret;
}


static MPP_RET free_cur_ctx(H264dCurCtx_t *p_Cur)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Cur);

    if (p_Cur) {
        for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
            MPP_FREE(p_Cur->listP[i]);
            MPP_FREE(p_Cur->listB[i]);
        }
        MPP_FREE(p_Cur->strm.nalu_buf);
        MPP_FREE(p_Cur->strm.head_buf);
    }

__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_cur_ctx(H264dCurCtx_t *p_Cur)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dCurStream_t *p_strm = NULL;

    INP_CHECK(ret, !p_Cur);

    p_strm = &p_Cur->strm;
    p_strm->nalu_max_size = NALU_BUF_MAX_SIZE;
    p_strm->nalu_buf = mpp_malloc_size(RK_U8, p_strm->nalu_max_size);
    p_strm->head_max_size = HEAD_BUF_MAX_SIZE;
    p_strm->head_buf = mpp_malloc_size(RK_U8, p_strm->head_max_size);
    MEM_CHECK(ret, p_strm->nalu_buf && p_strm->head_buf);
    p_strm->prefixdata = 0xffffffff;
    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
        p_Cur->listP[i] = mpp_malloc_size(H264_StorePic_t*, MAX_LIST_SIZE * sizeof(H264_StorePic_t*));
        p_Cur->listB[i] = mpp_malloc_size(H264_StorePic_t*, MAX_LIST_SIZE * sizeof(H264_StorePic_t*));
        MEM_CHECK(ret, p_Cur->listP[i] && p_Cur->listB[i]); // +1 for reordering
    }
    reset_cur_slice(p_Cur, &p_Cur->slice);

__RETURN:
    return ret = MPP_OK;
__FAILED:
    free_cur_ctx(p_Cur);

    return ret;
}


static MPP_RET free_vid_ctx(H264dVideoCtx_t *p_Vid)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Vid);

    for (i = 0; i < MAXSPS; i++) {
        recycle_subsps(&p_Vid->subspsSet[i]);
    }
    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
        free_dpb(p_Vid->p_Dpb_layer[i]);
        MPP_FREE(p_Vid->p_Dpb_layer[i]);
    }
    free_storable_picture(p_Vid->p_Dec, p_Vid->dec_pic);
    //free_frame_store(p_Dec, &p_Vid->out_buffer);

__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_vid_ctx(H264dVideoCtx_t *p_Vid)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Vid);

    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
        p_Vid->p_Dpb_layer[i] = mpp_calloc(H264_DpbBuf_t, 1);
        MEM_CHECK(ret, p_Vid->p_Dpb_layer[i]);
        p_Vid->p_Dpb_layer[i]->layer_id  = i;
        p_Vid->p_Dpb_layer[i]->p_Vid     = p_Vid;
        p_Vid->p_Dpb_layer[i]->init_done = 0;
        p_Vid->p_Dpb_layer[i]->poc_interval = 2;
    }
    //!< init video pars
    for (i = 0; i < MAXSPS; i++) {
        p_Vid->spsSet[i].seq_parameter_set_id = 0;
        p_Vid->subspsSet[i].sps.seq_parameter_set_id = 0;
    }
    for (i = 0; i < MAXPPS; i++) {
        p_Vid->ppsSet[i].pic_parameter_set_id = 0;
        p_Vid->ppsSet[i].seq_parameter_set_id = 0;
    }
    //!< init active_sps
    p_Vid->active_sps       = NULL;
    p_Vid->active_subsps    = NULL;
    p_Vid->active_sps_id[0] = -1;
    p_Vid->active_sps_id[1] = -1;
    //!< init subspsSet
    for (i = 0; i < MAXSPS; i++) {
        p_Vid->subspsSet[i].sps.seq_parameter_set_id = 0;
        p_Vid->subspsSet[i].num_views_minus1 = -1;
        p_Vid->subspsSet[i].num_level_values_signalled_minus1 = -1;
    }
__RETURN:
    return ret = MPP_OK;
__FAILED:
    free_vid_ctx(p_Vid);

    return ret;
}

static MPP_RET free_dxva_ctx(H264dDxvaCtx_t *p_dxva)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, NULL == p_dxva);

    MPP_FREE(p_dxva->slice_long);
    MPP_FREE(p_dxva->bitstream);
    MPP_FREE(p_dxva->syn.buf);

__RETURN:
    return ret = MPP_OK;
}

static MPP_RET init_dxva_ctx(H264dDxvaCtx_t *p_dxva)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_dxva);

    p_dxva->slice_count    = 0;
    p_dxva->max_slice_size = MAX_SLICE_NUM;
    p_dxva->max_strm_size  = BITSTREAM_MAX_SIZE;
    p_dxva->slice_long  = mpp_calloc(DXVA_Slice_H264_Long,  p_dxva->max_slice_size);
    MEM_CHECK(ret, p_dxva->slice_long);
    p_dxva->bitstream   = mpp_malloc(RK_U8, p_dxva->max_strm_size);
    p_dxva->syn.buf     = mpp_calloc(DXVA2_DecodeBufferDesc, SYNTAX_BUF_SIZE);
    MEM_CHECK(ret, p_dxva->bitstream && p_dxva->syn.buf);

__RETURN:
    return ret = MPP_OK;

__FAILED:
    return ret;
}

static MPP_RET free_dec_ctx(H264_DecCtx_t *p_Dec)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, NULL == p_Dec);

    if (p_Dec->mem) {
        free_dxva_ctx(&p_Dec->mem->dxva_ctx);
        MPP_FREE(p_Dec->mem);
    }
    //!< free mpp packet
    mpp_packet_deinit(&p_Dec->task_pkt);

__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_dec_ctx(H264_DecCtx_t *p_Dec)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Dec);

    p_Dec->mem = mpp_calloc(H264_DecMem_t, 1);
    MEM_CHECK(ret, p_Dec->mem);
    p_Dec->dpb_mark         = p_Dec->mem->dpb_mark;           //!< for write out, MAX_DPB_SIZE
    p_Dec->dpb_info         = p_Dec->mem->dpb_info;           //!< 16
    p_Dec->refpic_info_p    = p_Dec->mem->refpic_info_p;      //!< 32
    p_Dec->refpic_info_b[0] = p_Dec->mem->refpic_info_b[0];   //!< [2][32]
    p_Dec->refpic_info_b[1] = p_Dec->mem->refpic_info_b[1];   //!< [2][32]
    //!< init dxva memory
    p_Dec->mem->dxva_ctx.p_Dec = p_Dec;
    FUN_CHECK(ret = init_dxva_ctx(&p_Dec->mem->dxva_ctx));
    p_Dec->dxva_ctx = &p_Dec->mem->dxva_ctx;
    //!< init Dpb_memory Mark
    for (i = 0; i < MAX_MARK_SIZE; i++) {
        reset_dpb_mark(&p_Dec->dpb_mark[i]);
        p_Dec->dpb_mark[i].mark_idx = i;
    }
    mpp_buf_slot_setup(p_Dec->frame_slots, MAX_MARK_SIZE);
    //!< malloc mpp packet
    mpp_packet_init(&p_Dec->task_pkt, p_Dec->dxva_ctx->bitstream, p_Dec->dxva_ctx->max_strm_size);
    MEM_CHECK(ret, p_Dec->task_pkt);
    //!< set Dec support decoder method
    p_Dec->spt_decode_mtds = MPP_DEC_BY_FRAME;
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->have_slice_data = 0;
    p_Dec->last_frame_slot_idx = -1;
    memset(&p_Dec->errctx, 0, sizeof(H264dErrCtx_t));
__RETURN:
    return ret = MPP_OK;

__FAILED:
    free_dec_ctx(p_Dec);

    return ret;
}

static MPP_RET h264d_flush_dpb_eos(H264_DecCtx_t *p_Dec)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    INP_CHECK(ret, !p_Dec->p_Vid);

    FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[0], 1));
    FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[0], 1));
    if (p_Dec->mvc_valid) {
        // layer_id == 1
        FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[1], 1));
        FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[1], 2));
    }

    flush_dpb_buf_slot(p_Dec);

__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret = MPP_NOK;
}
/*!
***********************************************************************
* \brief
*   alloc all buffer
***********************************************************************
*/

MPP_RET h264d_init(void *decoder, ParserCfg *init)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    INP_CHECK(ret, !p_Dec);
    memset(p_Dec, 0, sizeof(H264_DecCtx_t));

    mpp_env_get_u32("rkv_h264d_debug", &rkv_h264d_parse_debug, H264D_DBG_ERROR);

    //!< get init frame_slots and packet_slots
    p_Dec->frame_slots  = init->frame_slots;
    p_Dec->packet_slots = init->packet_slots;
    //!< malloc decoder buffer
    p_Dec->p_Inp = mpp_calloc(H264dInputCtx_t, 1);
    p_Dec->p_Cur = mpp_calloc(H264dCurCtx_t, 1);
    p_Dec->p_Vid = mpp_calloc(H264dVideoCtx_t, 1);
    MEM_CHECK(ret, p_Dec->p_Inp && p_Dec->p_Cur && p_Dec->p_Vid);
    p_Dec->p_Inp->p_Dec = p_Dec;
    p_Dec->p_Inp->p_Cur = p_Dec->p_Cur;
    p_Dec->p_Inp->p_Vid = p_Dec->p_Vid;

    p_Dec->p_Cur->p_Dec = p_Dec;
    p_Dec->p_Cur->p_Inp = p_Dec->p_Inp;
    p_Dec->p_Cur->p_Vid = p_Dec->p_Vid;

    p_Dec->p_Vid->p_Dec = p_Dec;
    p_Dec->p_Vid->p_Inp = p_Dec->p_Inp;
    p_Dec->p_Vid->p_Cur = p_Dec->p_Cur;
    FUN_CHECK(ret = init_input_ctx(p_Dec->p_Inp, init));
    FUN_CHECK(ret = init_cur_ctx(p_Dec->p_Cur));
    FUN_CHECK(ret = init_vid_ctx(p_Dec->p_Vid));
    FUN_CHECK(ret = init_dec_ctx(p_Dec));

__RETURN:
    return ret = MPP_OK;
__FAILED:
    h264d_deinit(decoder);

    return ret;
}
/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET h264d_deinit(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;

    INP_CHECK(ret, !decoder);

    free_input_ctx(p_Dec->p_Inp);
    MPP_FREE(p_Dec->p_Inp);
    free_cur_ctx(p_Dec->p_Cur);
    MPP_FREE(p_Dec->p_Cur);
    free_vid_ctx(p_Dec->p_Vid);
    MPP_FREE(p_Dec->p_Vid);
    free_dec_ctx(p_Dec);

__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET h264d_reset(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    H264dCurStream_t *p_strm = NULL;

    INP_CHECK(ret, !decoder);

    FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[0], 1));
    FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[0], 1));
    if (p_Dec->mvc_valid) {
        // layer_id == 1
        FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[1], 1));
        FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[1], 2));
    }
    flush_dpb_buf_slot(p_Dec);
    //!< reset input parameter
    p_Dec->p_Inp->in_buf        = NULL;
    p_Dec->p_Inp->pkt_eos       = 0;
    p_Dec->p_Inp->task_eos      = 0;
    p_Dec->p_Inp->in_pts        = 0;
    p_Dec->p_Inp->in_dts        = 0;
    p_Dec->p_Inp->has_get_eos   = 0;
    //!< reset video parameter
    p_Dec->p_Vid->have_outpicture_flag = 0;
    p_Dec->p_Vid->exit_picture_flag    = 0;
    p_Dec->p_Vid->active_mvc_sps_flag  = 0;
    p_Dec->p_Vid->g_framecnt           = 0;
    p_Dec->p_Vid->dec_pic = NULL;
    p_Dec->p_Vid->last_pic = NULL;
    memset(&p_Dec->p_Vid->old_pic, 0, sizeof(H264_StorePic_t));
    memset(&p_Dec->errctx, 0, sizeof(H264dErrCtx_t));
    //!< reset current time stamp
    p_Dec->p_Cur->last_dts  = 0;
    p_Dec->p_Cur->last_pts  = 0;
    p_Dec->p_Cur->curr_dts  = 0;
    p_Dec->p_Cur->curr_pts  = 0;
    //!< reset current stream
    p_strm = &p_Dec->p_Cur->strm;
    p_strm->prefixdata      = 0xffffffff;
    p_strm->nalu_offset     = 0;
    p_strm->nalu_len        = 0;
    p_strm->head_offset     = 0;
    p_strm->startcode_found = 0;
    p_strm->endcode_found   = 0;
    p_strm->startcode_found = p_Dec->p_Inp->is_nalff;
    //!< reset decoder parameter
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->have_slice_data = 0;
    p_Dec->is_new_frame   = 0;
    p_Dec->is_parser_end  = 0;
    p_Dec->dxva_ctx->strm_offset = 0;
    p_Dec->dxva_ctx->slice_count = 0;
    p_Dec->last_frame_slot_idx   = -1;

__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret = MPP_NOK;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET  h264d_flush(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;

    INP_CHECK(ret, !decoder);
    INP_CHECK(ret, !p_Dec->p_Inp);
    INP_CHECK(ret, !p_Dec->p_Vid);

    FUN_CHECK(ret = output_dpb(p_Dec, p_Dec->p_Vid->p_Dpb_layer[0]));
    if (p_Dec->mvc_valid) {
        FUN_CHECK(ret = output_dpb(p_Dec, p_Dec->p_Vid->p_Dpb_layer[1]));
    }

__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret = MPP_NOK;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET  h264d_control(void *decoder, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t   *dec = (H264_DecCtx_t *)decoder;

    INP_CHECK(ret, !decoder);
    switch (cmd_type) {
    case MPP_DEC_SET_DISABLE_ERROR: {
        dec->disable_error = *((RK_U32 *)param);
    } break;
    case MPP_DEC_SET_IMMEDIATE_OUT: {
        dec->immediate_out = *((RK_U32 *)param);
    } break;
    default : {
    } break;
    }

__RETURN:
    return ret = MPP_OK;
}


/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/
#define MAX_STREM_IN_SIZE         (10*1024*1024)
MPP_RET h264d_prepare(void *decoder, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dInputCtx_t *p_Inp = NULL;
    H264_DecCtx_t   *p_Dec = (H264_DecCtx_t *)decoder;

    INP_CHECK(ret, !decoder && !pkt && !task);

    p_Inp = p_Dec->p_Inp;
    if (p_Inp->has_get_eos || p_Dec->errctx.un_spt_flag) {
        mpp_packet_set_length(pkt, 0);
        goto __RETURN;
    }
    p_Inp->in_pkt = pkt;
    p_Inp->in_pts = mpp_packet_get_pts(pkt);
    p_Inp->in_dts = mpp_packet_get_dts(pkt);
    p_Inp->in_length = mpp_packet_get_length(pkt);
    p_Inp->pkt_eos = mpp_packet_get_eos(pkt);
    p_Inp->in_buf = (RK_U8 *)mpp_packet_get_pos(pkt);

    if (p_Inp->pkt_eos) {
        p_Inp->has_get_eos = 1;
        if (p_Inp->in_length < 4) {
            p_Inp->in_buf      = NULL;
            p_Inp->in_length   = 0;
            task->flags.eos = p_Inp->pkt_eos;
            h264d_flush_dpb_eos(p_Dec);
            goto __RETURN;
        }
    }

    if (p_Inp->in_length > MAX_STREM_IN_SIZE) {
        H264D_ERR("[pkt_in_timeUs] input error, stream too large, pts=%lld, eos=%d, len=%d, pkt_no=%d",
                  p_Inp->in_pts, p_Inp->pkt_eos, p_Inp->in_length, p_Dec->p_Vid->g_framecnt);
        mpp_packet_set_length(pkt, 0);
        ret = MPP_NOK;
        goto __FAILED;
    }
    //!< avcC stream
    if (mpp_packet_get_flag(pkt) & MPP_PACKET_FLAG_EXTRA_DATA) {
        RK_U8 *pdata = p_Inp->in_buf;
        p_Inp->is_nalff = (p_Inp->in_length > 3) && (pdata[0] && pdata[1]);
        if (p_Inp->is_nalff) {
            (ret = parse_prepare_avcC_header(p_Inp, p_Dec->p_Cur));
            goto __RETURN;
        }
    }
    H264D_DBG(H264D_DBG_INPUT, "[pkt_in_timeUs] is_avcC=%d, in_pts=%lld, pkt_eos=%d, len=%d, pkt_no=%d",
              p_Inp->is_nalff, p_Inp->in_pts, p_Inp->pkt_eos, p_Inp->in_length, p_Dec->p_Vid->g_framecnt);
    if (p_Inp->is_nalff) {
        (ret = parse_prepare_avcC_data(p_Inp, p_Dec->p_Cur));
        task->valid = p_Inp->task_valid;  //!< prepare valid flag
    } else  {
        fwrite_stream_to_file(p_Inp, p_Inp->in_buf, (RK_U32)p_Inp->in_length);
        do {
            if (p_Inp->init.need_split) {
                ret = parse_prepare(p_Inp, p_Dec->p_Cur);
            } else {
                ret = parse_prepare_fast(p_Inp, p_Dec->p_Cur);
            }
            task->valid = p_Inp->task_valid;  //!< prepare valid flag
        } while (mpp_packet_get_length(pkt) && !task->valid);
    }
    task->flags.eos = p_Inp->pkt_eos;
    if (task->valid) {
        memset(p_Dec->dxva_ctx->bitstream + p_Dec->dxva_ctx->strm_offset, 0,
               MPP_ALIGN(p_Dec->dxva_ctx->strm_offset, 16) - p_Dec->dxva_ctx->strm_offset);
        mpp_packet_set_data(p_Dec->task_pkt, p_Dec->dxva_ctx->bitstream);
        mpp_packet_set_length(p_Dec->task_pkt, MPP_ALIGN(p_Dec->dxva_ctx->strm_offset, 16));
        mpp_packet_set_size(p_Dec->task_pkt, p_Dec->dxva_ctx->max_strm_size);
        task->input_packet = p_Dec->task_pkt;
    } else {
        task->input_packet = NULL;
    }
__RETURN:

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
MPP_RET h264d_parse(void *decoder, HalDecTask *in_task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    H264dErrCtx_t *p_err = &p_Dec->errctx;

    in_task->valid = 0;
    p_Dec->in_task = in_task;
    p_err->cur_err_flag  = 0;
    p_err->used_ref_flag = 0;
    p_Dec->is_parser_end = 0;
    memset(&p_Dec->p_Cur->sei, 0, sizeof(p_Dec->p_Cur->sei));

    ret = parse_loop(p_Dec);
    if (ret) {
        in_task->flags.parse_err = 1;
    }

    if (p_Dec->is_parser_end) {
        p_Dec->is_parser_end = 0;
        p_Dec->p_Vid->g_framecnt++;
        ret = update_dpb(p_Dec);
        if (ret) {
            in_task->flags.ref_err = 1;
        }
        if (in_task->flags.eos) {
            h264d_flush_dpb_eos(p_Dec);
        }
    }
    in_task->valid = 1;
    if (!in_task->flags.parse_err) {
        in_task->syntax.number = p_Dec->dxva_ctx->syn.num;
        in_task->syntax.data   = (void *)p_Dec->dxva_ctx->syn.buf;
        in_task->flags.used_for_ref = p_err->used_ref_flag;
        in_task->flags.ref_err |= (!p_Dec->disable_error
                                   && (p_err->dpb_err_flag | p_err->cur_err_flag)) ? 1 : 0;
    }

    return ret;
}

/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
typedef enum MppHalHardType_e {
    HAL_VDPU,           //!< vpu combined decoder
    HAL_VEPU,           //!< vpu combined encoder
    HAL_RKVDEC,         //!< rock-chip h264 h265 vp9 combined decoder
    HAL_DEVICE_BUTT,
} HalDeviceId;
MPP_RET h264d_callback(void *decoder, void *errinfo)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    IOCallbackCtx *ctx = (IOCallbackCtx *)errinfo;

    INP_CHECK(ret, !decoder);

    {
        MppFrame mframe = NULL;
        RK_U32 *p_regs = ctx->regs;
        HalDecTask *task_dec = (HalDecTask *)ctx->task;

        mpp_buf_slot_get_prop(p_Dec->frame_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);
        if (mframe) {
            RK_U32 task_err = task_dec->flags.parse_err || task_dec->flags.ref_err;
            if (ctx->hard_err || task_err) {
                if (task_dec->flags.used_for_ref) {
                    mpp_frame_set_errinfo(mframe, MPP_FRAME_ERR_UNKNOW);
                } else {
                    mpp_frame_set_discard(mframe, MPP_FRAME_ERR_UNKNOW);
                }
            }
            H264D_DBG(H264D_DBG_CALLBACK, "[CALLBACK] g_no=%d, out_idx=%d, dpberr=%d, harderr=%d, ref_flag=%d, errinfo=%d, discard=%d\n",
                      p_Dec->p_Vid->g_framecnt, task_dec->output, task_err, ctx->hard_err, task_dec->flags.used_for_ref,
                      mpp_frame_get_errinfo(mframe), mpp_frame_get_discard(mframe));

            if (ctx->device_id == HAL_RKVDEC) {
                H264D_DBG(H264D_DBG_CALLBACK, "[CALLBACK] sw[01]=%08x, sw[45]=%08x, sw[76]=%08x\n", p_regs[1], p_regs[45], p_regs[76]);
            } else if (ctx->device_id == HAL_VDPU) {

            }
        }
    }

__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/

const ParserApi api_h264d_parser = {
    .name = "h264d_parse",
    .coding = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(H264_DecCtx_t),
    .flag = 0,
    .init = h264d_init,
    .deinit = h264d_deinit,
    .prepare = h264d_prepare,
    .parse = h264d_parse,
    .reset = h264d_reset,
    .flush = h264d_flush,
    .control = h264d_control,
    .callback = h264d_callback,
};

