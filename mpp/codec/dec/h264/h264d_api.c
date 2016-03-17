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
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"

#include "vpu_api.h"

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
	mpp_env_get_u32("rkv_h264d_mvc_disable", &p_Inp->mvc_disable, 1);
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
		free_ref_pic_list_reordering_buffer(&p_Cur->slice);
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
        memset(&p_Vid->outlist[i], 0, sizeof(p_Vid->outlist[i]));
        p_Vid->outlist[i].max_size = MAX_MARK_SIZE;
        p_Vid->last_outputpoc[i] = -1;
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
    p_Vid->iframe_cnt = 0;
	//!< memset error context

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
    p_dxva->slice_count    = 0;
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
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, NULL == p_Dec);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    if (p_Dec->mem) {
        free_dxva_ctx(&p_Dec->mem->dxva_ctx);
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
    p_Dec->dpb_mark         = p_Dec->mem->dpb_mark;           //!< for write out, MAX_DPB_SIZE
    p_Dec->dpb_info         = p_Dec->mem->dpb_info;           //!< 16
    p_Dec->dpb_old[0]       = p_Dec->mem->dpb_old[0];
    p_Dec->dpb_old[1]       = p_Dec->mem->dpb_old[1];
    p_Dec->refpic_info_p    = p_Dec->mem->refpic_info_p;      //!< 32
    p_Dec->refpic_info_b[0] = p_Dec->mem->refpic_info_b[0];   //!< [2][32]
    p_Dec->refpic_info_b[1] = p_Dec->mem->refpic_info_b[1];   //!< [2][32]
    //!< init dxva memory
    p_Dec->mem->dxva_ctx.p_Dec = p_Dec;
    FUN_CHECK(ret = init_dxva_ctx(&p_Dec->mem->dxva_ctx));
    p_Dec->dxva_ctx = &p_Dec->mem->dxva_ctx;
    //!< init Dpb_memory Mark, for fpga check
    for (i = 0; i < MAX_MARK_SIZE; i++) {
        p_Dec->dpb_mark[i].top_used = 0;
        p_Dec->dpb_mark[i].bot_used = 0;
        p_Dec->dpb_mark[i].mark_idx = i;
        p_Dec->dpb_mark[i].slot_idx = -1;
        p_Dec->dpb_mark[i].pic      = NULL;       
    }
    mpp_buf_slot_setup(p_Dec->frame_slots, MAX_MARK_SIZE);
    //!< malloc mpp packet
    mpp_packet_init(&p_Dec->task_pkt, p_Dec->dxva_ctx->bitstream, p_Dec->dxva_ctx->max_strm_size);
    MEM_CHECK(ret, p_Dec->task_pkt);
    //!< set Dec support decoder method
    p_Dec->spt_decode_mtds = MPP_DEC_BY_FRAME;
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->is_first_frame = 1;
    p_Dec->last_frame_slot_idx = -1;
	memset(&p_Dec->errctx, 0, sizeof(H264dErrCtx_t));
__RETURN:
    return ret = MPP_OK;

__FAILED:
    free_dec_ctx(p_Dec);

    return ret;
}

static void flush_dpb_buf_slot(H264_DecCtx_t *p_Dec)
{
	RK_U32 i = 0;
	H264_DpbMark_t *p_mark = NULL;

	for(i = 0; i < MAX_MARK_SIZE; i++) {
		p_mark = &p_Dec->dpb_mark[i];
		if (p_mark->out_flag && (p_mark->slot_idx >= 0)) {	
			MppFrame mframe = NULL;
			mpp_buf_slot_get_prop(p_Dec->frame_slots, p_mark->slot_idx, SLOT_FRAME_PTR, &mframe);
			if (mframe)	{
				H264D_DBG(H264D_DBG_SLOT_FLUSH, "[DPB_BUF_FLUSH] p_mark->slot_idx=%d", p_mark->slot_idx);
				mpp_buf_slot_set_flag(p_Dec->frame_slots, p_mark->slot_idx, SLOT_QUEUE_USE);
				mpp_buf_slot_enqueue(p_Dec->frame_slots, p_mark->slot_idx, QUEUE_DISPLAY);
				mpp_buf_slot_clr_flag(p_Dec->frame_slots, p_mark->slot_idx, SLOT_CODEC_USE);
			//	mpp_frame_set_errinfo(mframe, VPU_FRAME_ERR_UNKNOW);
			//	p_Dec->last_frame_slot_idx = p_mark->slot_idx;
			}			
		}
		p_mark->out_flag = 0;
	}
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
	mpp_env_get_u32("rkv_h264d_debug", &rkv_h264d_parse_debug, H264D_DBG_ERROR);
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
    h264d_flush(decoder);
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
    H264dCurStream_t *p_strm = NULL;

    INP_CHECK(ret, !decoder);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
	FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[0], 1));
	FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[0], 1));
	if (p_Dec->mvc_valid) 
	{ // layer_id == 1
		FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[1], 1));
		FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[1], 2));
		flush_muti_view_output(p_Dec->frame_slots, p_Dec->p_Vid->outlist, p_Dec->p_Vid);
	}
	flush_dpb_buf_slot(p_Dec);
    //!< reset input parameter
    p_Dec->p_Inp->in_buf        = NULL;
    p_Dec->p_Inp->pkt_eos       = 0;
    p_Dec->p_Inp->task_eos      = 0;
    p_Dec->p_Inp->in_pts        = 0;
    p_Dec->p_Inp->in_dts        = 0;
    p_Dec->p_Inp->out_buf       = NULL;
    p_Dec->p_Inp->out_length    = 0;
    p_Dec->p_Inp->has_get_eos   = 0;
    //!< reset video parameter
    p_Dec->p_Vid->g_framecnt    = 0;
    p_Dec->p_Vid->last_outputpoc[0] = -1;
    p_Dec->p_Vid->last_outputpoc[1] = -1;
    p_Dec->p_Vid->iframe_cnt    = 0;
	memset(&p_Dec->errctx, 0, sizeof(H264dErrCtx_t));

    //!< reset current time stamp
    p_Dec->p_Cur->last_dts  = 0;
    p_Dec->p_Cur->last_pts  = 0;
    p_Dec->p_Cur->curr_dts  = 0;
    p_Dec->p_Cur->curr_pts  = 0;
    //!< reset current stream
    p_strm = &p_Dec->p_Cur->strm;
    p_strm->prefixdata[0]   = 0xff;
    p_strm->prefixdata[1]   = 0xff;
    p_strm->prefixdata[2]   = 0xff;
    p_strm->nalu_offset     = 0;
    p_strm->nalu_len        = 0;
    p_strm->head_offset     = 0;
    p_strm->startcode_found = 0;
    p_strm->endcode_found   = 0;
    p_strm->pkt_ts_idx      = 0;
    p_strm->pkt_used_bytes  = 0;
    p_strm->startcode_found = p_Dec->p_Inp->is_nalff;
    memset(p_strm->pkt_ts, 0, MAX_PTS_NUM * sizeof(H264dTimeStamp_t));
    //!< reset decoder parameter
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->is_first_frame = 1;
    p_Dec->is_new_frame   = 0;
    p_Dec->is_parser_end  = 0;
    p_Dec->dxva_ctx->strm_offset = 0;
    p_Dec->dxva_ctx->slice_count = 0;

    memset(p_Dec->dpb_old[0], 0, MAX_DPB_SIZE * sizeof(H264_DpbInfo_t));
    memset(p_Dec->dpb_old[1], 0, MAX_DPB_SIZE * sizeof(H264_DpbInfo_t));

    //!< reset dpb
        // layer_id == 1
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
	
	if (p_Dec->last_frame_slot_idx < 0) {
		IOInterruptCB *cb = &p_Dec->p_Inp->init.notify_cb;
		if (cb->callBack) {
			cb->callBack(cb->opaque, NULL);
		}
		goto __RETURN;
	} 	
    FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[0], 1));
    FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[0], 1));
    //free_dpb(p_Dec->p_Vid->p_Dpb_layer[0]);
    if (p_Dec->mvc_valid) {
        // layer_id == 1
        FUN_CHECK(ret = flush_dpb(p_Dec->p_Vid->p_Dpb_layer[1], 2));
        FUN_CHECK(ret = init_dpb(p_Dec->p_Vid, p_Dec->p_Vid->p_Dpb_layer[1], 2));
        //free_dpb(p_Dec->p_Vid->p_Dpb_layer[1]);
        flush_muti_view_output(p_Dec->frame_slots, p_Dec->p_Vid->outlist, p_Dec->p_Vid);
    }  
	flush_dpb_buf_slot(p_Dec);
	p_Dec->p_Inp->task_eos = 1;
	mpp_buf_slot_set_prop(p_Dec->frame_slots, p_Dec->last_frame_slot_idx, SLOT_EOS, &p_Dec->p_Inp->task_eos);
__RETURN:
	H264D_DBG(H264D_DBG_DPB_DISPLAY, "[DPB display] flush end, task_eos=%d, last_slot_idx=%d", p_Dec->p_Inp->task_eos, p_Dec->last_frame_slot_idx);
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
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
	H264dInputCtx_t *p_Inp = NULL;
    H264_DecCtx_t   *p_Dec = (H264_DecCtx_t *)decoder;

    INP_CHECK(ret, !decoder && !pkt && !task);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

	p_Inp = p_Dec->p_Inp;
    if (p_Inp->has_get_eos) {
        ((MppPacketImpl *)pkt)->length = 0;
        goto __RETURN;
    }
    p_Inp->in_pkt = pkt;
    p_Inp->in_pts = mpp_packet_get_pts(pkt);
    p_Inp->in_dts = mpp_packet_get_dts(pkt);

    if (mpp_packet_get_eos(pkt)) {
        p_Inp->in_buf      = NULL;
        p_Inp->in_length   = 0;
        p_Inp->pkt_eos     = 1;
        p_Inp->has_get_eos = 1;
    } else {
        p_Inp->in_buf      = (RK_U8 *)mpp_packet_get_pos(pkt);
        p_Inp->in_length   = mpp_packet_get_length(pkt);
        p_Inp->pkt_eos     = 0;
    }
	//!< avcC stream
    if (mpp_packet_get_flag(pkt)& MPP_PACKET_FLAG_EXTRA_DATA) {
        RK_U8 *pdata = p_Inp->in_buf;
        p_Inp->is_nalff = (p_Inp->in_length > 3) && (pdata[0] && pdata[1]);
        if (p_Inp->is_nalff) {
            (ret = parse_prepare_extra_header(p_Inp, p_Dec->p_Cur));
            goto __RETURN;
        }
    }
	H264D_DBG(H264D_DBG_INPUT, "[pkt_in timeUs] is_avcC=%d, in_pts=%lld, pkt_eos=%d, len=%d, g_framecnt=%d \n",
              p_Inp->is_nalff, p_Inp->in_pts, p_Inp->pkt_eos, p_Inp->in_length, p_Dec->p_Vid->g_framecnt);

    if (p_Inp->is_nalff) {
        (ret = parse_prepare_extra_data(p_Inp, p_Dec->p_Cur));
        task->valid = p_Inp->task_valid;  //!< prepare valid flag
    } else  {
        do {
            (ret = parse_prepare_fast(p_Inp, p_Dec->p_Cur));
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
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);

    return ret = MPP_OK;
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

	p_Dec->errctx.parse_err_flag = 0;
	p_Dec->errctx.dpb_err_flag = 0;
	p_Dec->errctx.used_for_ref_flag = 0;
    FUN_CHECK(ret = parse_loop(p_Dec));

    if (p_Dec->is_parser_end) {		
        p_Dec->is_parser_end = 0;
		p_Dec->p_Vid->g_framecnt++;
		ret = update_dpb(p_Dec);
		if (in_task->flags.eos) {
			h264d_flush(decoder);
			goto __RETURN;
		}
		if (ret) {
			goto __FAILED;
		}
		in_task->valid = 1; // register valid flag
		in_task->syntax.number = p_Dec->dxva_ctx->syn.num;
		in_task->syntax.data   = (void *)p_Dec->dxva_ctx->syn.buf;		
		in_task->dpb_err_flag  = p_Dec->errctx.dpb_err_flag;
		in_task->used_for_ref_flag = p_Dec->errctx.used_for_ref_flag;
    }
__RETURN:
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
    return ret = MPP_OK;

__FAILED:
	{
		H264_StorePic_t *dec_pic = p_Dec->p_Vid->dec_pic;
		if (dec_pic && dec_pic->mem_mark->out_flag) {
			MppFrame mframe = NULL;
			mpp_buf_slot_get_prop(p_Dec->frame_slots, dec_pic->mem_mark->slot_idx, SLOT_FRAME_PTR, &mframe);
			mpp_buf_slot_set_flag(p_Dec->frame_slots, dec_pic->mem_mark->slot_idx, SLOT_QUEUE_USE);
			mpp_buf_slot_enqueue (p_Dec->frame_slots, dec_pic->mem_mark->slot_idx, QUEUE_DISPLAY);
			mpp_buf_slot_clr_flag(p_Dec->frame_slots, dec_pic->mem_mark->slot_idx, SLOT_CODEC_USE);
			dec_pic->mem_mark->out_flag = 0;
			p_Dec->p_Vid->dec_pic = NULL;
			mpp_frame_set_discard(mframe, 1);
		}
	}

	return ret;
}

/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
MPP_RET h264d_callback(void *decoder, void *errinfo)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	MppFrame mframe = NULL;
	RK_U32 *p_regs = NULL;
	RK_U32 out_slot_idx = 0;
	RK_U32 dpb_err_flag = 0;
	RK_U32 used_for_ref_flag = 0;
	H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;

	INP_CHECK(ret, !decoder);
	FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

	p_regs = (RK_U32*)errinfo;
	out_slot_idx = p_regs[78];
	dpb_err_flag = p_regs[79];
	used_for_ref_flag = p_regs[80];

	mpp_buf_slot_get_prop(p_Dec->frame_slots, out_slot_idx, SLOT_FRAME_PTR, &mframe);
	if (used_for_ref_flag) {
		mpp_frame_set_errinfo(mframe, VPU_FRAME_ERR_UNKNOW);
	}
	else {
		mpp_frame_set_discard(mframe, VPU_FRAME_ERR_UNKNOW);
	}
	H264D_DBG(H264D_DBG_CALLBACK, "g_frame_no=%d, out_slot_idx=%d, swreg[01]=%08x, swreg[45]=%08x, dpb_err=%d, used_for_ref=%d",
		p_Dec->p_Vid->g_framecnt, out_slot_idx, p_regs[1], p_regs[45], dpb_err_flag, used_for_ref_flag);

__RETURN:
	FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
	return ret = MPP_OK;
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
    h264d_callback,
};

