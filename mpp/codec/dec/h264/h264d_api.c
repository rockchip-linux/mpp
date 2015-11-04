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

#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"

#include "h264d_log.h"
#include "h264d_api.h"
#include "h264d_global.h"
#include "h264d_parse.h"
#include "h264d_sps.h"
#include "h264d_slice.h"
#include "h264d_dpb.h"



static void close_log_files(LogEnv_t *env)
{
    FCLOSE(env->fp_syn_parse);
    FCLOSE(env->fp_run_parse);
}

static MPP_RET open_log_files(LogEnv_t *env, LogFlag_t *pflag)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    char fname[128] = { 0 };

    INP_CHECK(ret, !pflag->write_en);
    set_log_outpath(env);
    //!< runlog file
    if (GetBitVal(env->ctrl, LOG_DEBUG)) {
        sprintf(fname, "%s/h264d_parse_runlog.dat", env->outpath);
        FLE_CHECK(ret, env->fp_run_parse = fopen(fname, "wb"));
    }
    //!< read syntax
    if (   GetBitVal(env->ctrl, LOG_READ_NALU  )
           || GetBitVal(env->ctrl, LOG_READ_SPS   )
           || GetBitVal(env->ctrl, LOG_READ_SUBSPS)
           || GetBitVal(env->ctrl, LOG_READ_PPS   )
           || GetBitVal(env->ctrl, LOG_READ_SLICE ) ) {
        sprintf(fname, "%s/h264d_read_syntax.dat", env->outpath);
        FLE_CHECK(ret, env->fp_syn_parse = fopen(fname, "wb"));
    }
__RETURN:
    return MPP_OK;

__FAILED:
    return ret;
}

static MPP_RET logctx_deinit(H264dLogCtx_t *logctx)
{
    close_log_files(&logctx->env);

    return MPP_OK;
}

static MPP_RET logctx_init(H264dLogCtx_t *logctx, LogCtx_t *logbuf)
{
    RK_U8 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    LogCtx_t *pcur = NULL;

    FUN_CHECK(ret = get_logenv(&logctx->env));
    if (logctx->env.help) {
        print_env_help(&logctx->env);
    }
    if (logctx->env.show) {
        show_env_flags(&logctx->env);
    }
    FUN_CHECK(ret = explain_ctrl_flag(logctx->env.ctrl, &logctx->log_flag));
    if ( !logctx->log_flag.debug_en
         && !logctx->log_flag.print_en && !logctx->log_flag.write_en ) {
        logctx->log_flag.debug_en = 0;
        goto __RETURN;
    }
    logctx->log_flag.level = (1 << logctx->env.level) - 1;
    //!< open file
    FUN_CHECK(ret = open_log_files(&logctx->env, &logctx->log_flag));
    //!< set logctx
    while (i < LOG_MAX) {
        if (GetBitVal(logctx->env.ctrl, i)) {
            pcur = logctx->parr[i] = &logbuf[i];
            pcur->tag = logctrl_name[i];
            pcur->flag = &logctx->log_flag;

            switch (i) {
            case RUN_PARSE:
                pcur->fp = logctx->env.fp_run_parse;
                break;
            case LOG_READ_NALU:
            case LOG_READ_SPS:
            case LOG_READ_SUBSPS:
            case LOG_READ_PPS:
            case LOG_READ_SLICE:
                pcur->fp = logctx->env.fp_syn_parse;
                break;
            default:
                break;
            }
        }
        i++;
    }

__RETURN:
    return ret = MPP_OK;
__FAILED:
    logctx->log_flag.debug_en = 0;
    logctx_deinit(logctx);

    return ret;
}

static MPP_RET free_input_ctx(H264dInputCtx_t *p_Inp)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Inp);
    FunctionIn(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);

    (void)p_Inp;

    FunctionOut(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_input_ctx(H264dInputCtx_t *p_Inp, ParserCfg *init)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Inp && !init);
    FunctionIn(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);

    p_Inp->init = *init;

    FunctionOut(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}


static MPP_RET free_cur_ctx(H264dCurCtx_t *p_Cur)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Cur);
    FunctionIn(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);
    if (p_Cur) {
        recycle_slice(&p_Cur->slice);
        for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
            MPP_FREE(p_Cur->listP[i]);
            MPP_FREE(p_Cur->listB[i]);
        }
        MPP_FREE(p_Cur->strm.nalu_buf);
        MPP_FREE(p_Cur->strm.head_buf);
    }
    FunctionOut(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_cur_ctx(H264dCurCtx_t *p_Cur)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dCurStream_t *p_strm = NULL;

    INP_CHECK(ret, !p_Cur);
    FunctionIn(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);

    p_strm = &p_Cur->strm;
    p_strm->nalu_max_size = NALU_BUF_MAX_SIZE;
    p_strm->nalu_buf = mpp_malloc_size(RK_U8, p_strm->nalu_max_size);
    p_strm->head_max_size = HEAD_BUF_MAX_SIZE;
    p_strm->head_buf = mpp_malloc_size(RK_U8, p_strm->head_max_size);
    MEM_CHECK(ret, p_strm->nalu_buf && p_strm->head_buf);

    p_strm->prefixdata[0] = 0xff;
    p_strm->prefixdata[1] = 0xff;
    p_strm->prefixdata[2] = 0xff;
    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
        p_Cur->listP[i] = mpp_malloc_size(H264_StorePic_t*, MAX_LIST_SIZE * sizeof(H264_StorePic_t*));
        p_Cur->listB[i] = mpp_malloc_size(H264_StorePic_t*, MAX_LIST_SIZE * sizeof(H264_StorePic_t*));
        MEM_CHECK(ret, p_Cur->listP[i] && p_Cur->listB[i]); // +1 for reordering
    }
    FunctionOut(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);

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
    FunctionIn(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);

    for (i = 0; i < MAXSPS; i++) {
        recycle_subsps(&p_Vid->subspsSet[i]);
    }
    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
        free_dpb(p_Vid->p_Dpb_layer[i]);
        MPP_FREE(p_Vid->p_Dpb_layer[i]);
    }
    free_storable_picture(p_Vid->p_Dec, p_Vid->dec_pic);
    //free_frame_store(p_Dec, &p_Vid->out_buffer);

    FunctionOut(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_vid_ctx(H264dVideoCtx_t *p_Vid)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Vid);
    FunctionIn(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
        p_Vid->p_Dpb_layer[i] = mpp_calloc(H264_DpbBuf_t, 1);
        MEM_CHECK(ret, p_Vid->p_Dpb_layer[i]);
        p_Vid->p_Dpb_layer[i]->layer_id  = i;
        p_Vid->p_Dpb_layer[i]->p_Vid     = p_Vid;
        p_Vid->p_Dpb_layer[i]->init_done = 0;
    }
    //!< init video pars
    for (i = 0; i < MAXSPS; i++) {
        p_Vid->spsSet[i].seq_parameter_set_id = -1;
        p_Vid->subspsSet[i].sps.seq_parameter_set_id = -1;
    }
    for (i = 0; i < MAXPPS; i++) {
        p_Vid->ppsSet[i].pic_parameter_set_id = -1;
        p_Vid->ppsSet[i].seq_parameter_set_id = -1;
    }
    //!< init active_sps
    p_Vid->active_sps       = NULL;
    p_Vid->active_subsps    = NULL;
    p_Vid->active_sps_id[0] = -1;
    p_Vid->active_sps_id[1] = -1;
    //!< init subspsSet
    for (i = 0; i < MAXSPS; i++) {
        p_Vid->subspsSet[i].sps.seq_parameter_set_id = -1;
        p_Vid->subspsSet[i].num_views_minus1 = -1;
        p_Vid->subspsSet[i].num_level_values_signalled_minus1 = -1;
    }
    FunctionOut(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
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
    FunctionIn(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);

    MPP_FREE(p_dxva->slice_long);
    MPP_FREE(p_dxva->bitstream);
    MPP_FREE(p_dxva->syn.buf);

    FunctionOut(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}

static MPP_RET init_dxva_ctx(H264dDxvaCtx_t *p_dxva)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_dxva);
    FunctionIn(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);

    p_dxva->max_slice_size = MAX_SLICE_SIZE;
    p_dxva->max_strm_size  = FRAME_BUF_MAX_SIZE;
    p_dxva->slice_long  = mpp_calloc(DXVA_Slice_H264_Long,  p_dxva->max_slice_size);
    MEM_CHECK(ret, p_dxva->slice_long);
    p_dxva->bitstream   = mpp_malloc(RK_U8, p_dxva->max_strm_size);
    p_dxva->syn.buf     = mpp_calloc(DXVA2_DecodeBufferDesc, SYNTAX_BUF_SIZE);
    MEM_CHECK(ret, p_dxva->bitstream && p_dxva->syn.buf);
    FunctionOut(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;

__FAILED:
    return ret;
}

static MPP_RET free_dec_ctx(H264_DecCtx_t *p_Dec)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, NULL == p_Dec);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    if (p_Dec->mem) {
        free_dxva_ctx(&p_Dec->mem->dxva_ctx);
        for (i = 0; i < MAX_MARK_SIZE; i++) {
            mpp_frame_deinit(&p_Dec->dpb_mark[i].frame);
        }
        MPP_FREE(p_Dec->mem);
    }
    //!< free mpp packet
    mpp_packet_deinit(&p_Dec->task_pkt);

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}
static MPP_RET init_dec_ctx(H264_DecCtx_t *p_Dec)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Dec);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    p_Dec->mem = mpp_calloc(H264_DecMem_t, 1);
    MEM_CHECK(ret, p_Dec->mem);
    p_Dec->dpb_mark       = p_Dec->mem->dpb_mark;         //!< for write out, MAX_DPB_SIZE
    p_Dec->dpb_info       = p_Dec->mem->dpb_info;         //!< 16
    p_Dec->refpic_info_p  = p_Dec->mem->refpic_info_p;    //!< 32
    p_Dec->refpic_info_b[0] = p_Dec->mem->refpic_info_b[0];   //!< [2][32]
    p_Dec->refpic_info_b[1] = p_Dec->mem->refpic_info_b[1];   //!< [2][32]
    //!< init dxva memory
    p_Dec->mem->dxva_ctx.p_Dec = p_Dec;
    FUN_CHECK(ret = init_dxva_ctx(&p_Dec->mem->dxva_ctx));
    p_Dec->dxva_ctx = &p_Dec->mem->dxva_ctx;
    //!< init frame slots, store frame buffer size
    mpp_buf_slot_setup(p_Dec->frame_slots, MAX_MARK_SIZE);
    //!< init Dpb_memory Mark, for fpga check
    for (i = 0; i < MAX_MARK_SIZE; i++) {
        p_Dec->dpb_mark[i].top_used = 0;
        p_Dec->dpb_mark[i].bot_used = 0;
        p_Dec->dpb_mark[i].mark_idx = i;
        p_Dec->dpb_mark[i].slot_idx = -1;
        p_Dec->dpb_mark[i].pic      = NULL;
        mpp_frame_init(&p_Dec->dpb_mark[i].frame);
    }
    //!< malloc mpp packet
    mpp_packet_init(&p_Dec->task_pkt, p_Dec->dxva_ctx->bitstream, p_Dec->dxva_ctx->max_strm_size);
    MEM_CHECK(ret, p_Dec->task_pkt);
    //!< set Dec support decoder method
    p_Dec->spt_decode_mtds = MPP_DEC_BY_FRAME | MPP_DEC_BY_SLICE;
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->is_first_frame = 1;

__RETURN:
    return ret = MPP_OK;

__FAILED:
    free_dec_ctx(p_Dec);

    return ret;
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
    // init logctx
    FUN_CHECK(ret = logctx_init(&p_Dec->logctx, p_Dec->logctxbuf));
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
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
    FUN_CHECK(ret = init_input_ctx(p_Dec->p_Inp, init));
    p_Dec->p_Cur->p_Dec = p_Dec;
    p_Dec->p_Cur->p_Inp = p_Dec->p_Inp;
    p_Dec->p_Cur->p_Vid = p_Dec->p_Vid;
    FUN_CHECK(ret = init_cur_ctx(p_Dec->p_Cur));
    p_Dec->p_Vid->p_Dec = p_Dec;
    p_Dec->p_Vid->p_Inp = p_Dec->p_Inp;
    p_Dec->p_Vid->p_Cur = p_Dec->p_Cur;
    FUN_CHECK(ret = init_vid_ctx(p_Dec->p_Vid));
    FUN_CHECK(ret = init_dec_ctx(p_Dec));

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);

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
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

    free_input_ctx(p_Dec->p_Inp);
    MPP_FREE(p_Dec->p_Inp);
    free_cur_ctx(p_Dec->p_Cur);
    MPP_FREE(p_Dec->p_Cur);
    free_vid_ctx(p_Dec->p_Vid);
    MPP_FREE(p_Dec->p_Vid);
    free_dec_ctx(p_Dec);

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
    logctx_deinit(&p_Dec->logctx);
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
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;

    INP_CHECK(ret, !decoder);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    mpp_log_f("reset In,g_framecnt=%d ", p_Dec->p_Vid->g_framecnt);
    //!< reset input parameter
    p_Dec->p_Inp->in_buf        = NULL;
    p_Dec->p_Inp->in_size       = 0;
    p_Dec->p_Inp->is_eos        = 0;
    p_Dec->p_Inp->pts           = -1;
    p_Dec->p_Inp->dts           = -1;
    p_Dec->p_Inp->out_buf       = NULL;
    p_Dec->p_Inp->out_length    = 0;
    //!< reset video parameter
    p_Dec->p_Vid->g_framecnt    = 0;
    //!< reset current stream
    p_Dec->p_Cur->strm.prefixdata[0] = 0xff;
    p_Dec->p_Cur->strm.prefixdata[1] = 0xff;
    p_Dec->p_Cur->strm.prefixdata[2] = 0xff;
    p_Dec->p_Cur->strm.nalu_offset = 0;
    p_Dec->p_Cur->strm.nalu_len = 0;
    p_Dec->p_Cur->strm.head_offset = 0;
    p_Dec->p_Cur->strm.startcode_found = 0;
    p_Dec->p_Cur->strm.endcode_found = 0;
    //!< reset decoder parameter
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->is_first_frame = 1;
    p_Dec->is_new_frame = 0;
    p_Dec->is_parser_end = 0;
    p_Dec->dxva_ctx->strm_offset = 0;
    p_Dec->dxva_ctx->slice_count = 0;
    //!< reset dpb
    FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[0], 1));
    if (p_Dec->p_Vid->active_mvc_sps_flag) { // layer_id == 1
        FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[1], 2));
    }
    for (i = 0; i < MAX_MARK_SIZE; i++) {
        p_Dec->dpb_mark[i].top_used = 0;
        p_Dec->dpb_mark[i].bot_used = 0;
        p_Dec->dpb_mark[i].slot_idx = -1;
        p_Dec->dpb_mark[i].pic      = NULL;
    }

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);

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
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

    FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[0], 1));
    FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[0], 1));
    //free_dpb(p_Dec->p_Vid->p_Dpb_layer[0]);
    if (p_Dec->p_Vid->active_mvc_sps_flag) { // layer_id == 1
        FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[1], 2));
        FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[1], 2));
        //free_dpb(p_Dec->p_Vid->p_Dpb_layer[1]);
    }
    mpp_buf_slot_set_prop(p_Dec->frame_slots, p_Dec->last_frame_slot_idx, SLOT_EOS, &p_Dec->p_Inp->is_eos);

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
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

    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    INP_CHECK(ret, !decoder);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

    (void)cmd_type;
    (void)param;
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return ret = MPP_OK;
}


/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/
MPP_RET h264d_prepare(void *decoder, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    LogCtx_t *logctx = NULL;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    INP_CHECK(ret, !decoder && !pkt && !task);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    logctx = p_Dec->logctx.parr[RUN_PARSE];
    //LogTrace(logctx, "Prepare In:len=%d, valid=%d ", mpp_packet_get_length(pkt), task->valid);

    p_Dec->p_Inp->in_buf    = (RK_U8 *)mpp_packet_get_pos(pkt);
    p_Dec->p_Inp->in_length = mpp_packet_get_length(pkt);
    p_Dec->p_Inp->is_eos    = mpp_packet_get_eos(pkt);
    p_Dec->p_Inp->in_size   = &((MppPacketImpl *)pkt)->length;

    p_Dec->p_Inp->pts = mpp_packet_get_pts(pkt);
    p_Dec->p_Inp->dts = mpp_packet_get_dts(pkt);

    task->flags.eos = p_Dec->p_Inp->is_eos;
    if (p_Dec->p_Vid->g_framecnt == 1477) {
        pkt = pkt;
    }
    LogTrace(logctx, "[Prepare_In] in_length=%d, eos=%d, g_framecnt=%d", mpp_packet_get_length(pkt), mpp_packet_get_eos(pkt), p_Dec->p_Vid->g_framecnt);
    do {
        (ret = parse_prepare(p_Dec->p_Inp, p_Dec->p_Cur));
        task->valid = p_Dec->p_Inp->task_valid;  //!< prepare valid flag
        LogTrace(logctx, "[Prepare_Ing] length=%d, valid=%d,strm_off=%d, g_framecnt=%d", mpp_packet_get_length(pkt), task->valid, p_Dec->dxva_ctx->strm_offset, p_Dec->p_Vid->g_framecnt);
    } while (mpp_packet_get_length(pkt) && !task->valid);
    //LogTrace(logctx, "Prepare Out: len=%d, valid=%d ", mpp_packet_get_length(pkt), task->valid);

    if (task->valid) {
        mpp_packet_set_data(p_Dec->task_pkt, p_Dec->dxva_ctx->bitstream);
        mpp_packet_set_length(p_Dec->task_pkt, p_Dec->dxva_ctx->strm_offset);
        mpp_packet_set_size(p_Dec->task_pkt, p_Dec->dxva_ctx->max_strm_size);
        LogTrace(logctx, "[Prepare_Out] ptr=%08x, stream_len=%d, max_size=%d", p_Dec->dxva_ctx->bitstream, p_Dec->dxva_ctx->strm_offset, p_Dec->dxva_ctx->max_strm_size);

        task->input_packet = p_Dec->task_pkt;
    } else {
        task->input_packet = NULL;
    }
__RETURN:
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);

    return ret = MPP_OK;
//__FAILED:
//    return ret;
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

    INP_CHECK(ret, !decoder && !in_task);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

    in_task->valid = 0; // prepare end flag
    p_Dec->in_task = in_task;

    mpp_packet_set_data(p_Dec->task_pkt, p_Dec->dxva_ctx->bitstream);
    mpp_packet_set_length(p_Dec->task_pkt, p_Dec->dxva_ctx->strm_offset);
    mpp_packet_set_size(p_Dec->task_pkt, p_Dec->dxva_ctx->max_strm_size);

    LogTrace(p_Dec->logctx.parr[RUN_PARSE], "[Parse_In] stream_len=%d, eos=%d, g_framecnt=%d",
             mpp_packet_get_length(p_Dec->task_pkt),
             in_task->flags.eos,
             p_Dec->p_Vid->g_framecnt);
    FUN_CHECK(ret = parse_loop(p_Dec));
    if (p_Dec->is_parser_end) {
        p_Dec->is_parser_end = 0;
        //LogTrace(p_Dec->logctx.parr[RUN_PARSE], "[Prarse loop end]");
        in_task->valid = 1; // register valid flag
        in_task->syntax.number = p_Dec->dxva_ctx->syn.num;
        in_task->syntax.data   = (void *)p_Dec->dxva_ctx->syn.buf;
        FUN_CHECK(ret = update_dpb(p_Dec));
        //LogTrace(p_Dec->logctx.parr[RUN_PARSE], "[Update dpb end]");
        //mpp_log_f("[PARSE_OUT] line=%d, g_framecnt=%d",__LINE__, p_Dec->p_Vid->g_framecnt++/*in_task->g_framecnt*/);
        LogTrace(p_Dec->logctx.parr[RUN_PARSE], "[PARSE_OUT] line=%d, g_framecnt=%d", __LINE__, p_Dec->p_Vid->g_framecnt++);
    }

__RETURN:
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
    return ret = MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/

const ParserApi api_h264d_parser = {
    "h264d_parse",
    MPP_VIDEO_CodingAVC,
    sizeof(H264_DecCtx_t),
    0,
    h264d_init,
    h264d_deinit,
    h264d_prepare,
    h264d_parse,
    h264d_reset,
    h264d_flush,
    h264d_control,
};

