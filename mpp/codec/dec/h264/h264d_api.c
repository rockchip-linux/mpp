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

#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"
#include "h264d_api.h"
#include "h264d_log.h"
#include "h264d_global.h"
#include "h264d_parse.h"


#define MODULE_TAG "h264d_api"



static MPP_RET free_input_ctx(H264dInputCtx_t *p_Inp)
{
	INP_CHECK(!p_Inp);
    FunctionIn(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);

    (void)p_Inp;

    FunctionOut(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
}
static MPP_RET init_input_ctx(H264dInputCtx_t *p_Inp, MppParserInitCfg *init)
{
	INP_CHECK(!p_Inp && !init);
    FunctionIn(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);

    p_Inp->init = *init;

    FunctionOut(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
}


static MPP_RET free_cur_ctx(H264dCurCtx_t *p_Cur)
{
	RK_U32 i = 0;
	INP_CHECK(!p_Cur);
    FunctionIn(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);
	if (p_Cur)
	{
		//rkv_h264d_recycle_slice(p_Cur->slice);
		for (i = 0; i < 2; i++)
		{
			mpp_free(p_Cur->listP[i]);
			mpp_free(p_Cur->listB[i]);
		}
		mpp_free(p_Cur->strm.buf);
	}
    FunctionOut(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
}
static MPP_RET init_cur_ctx(H264dCurCtx_t *p_Cur)
{
	RK_U32 i = 0;
	INP_CHECK(!p_Cur);
    FunctionIn(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);

	MEM_CHECK(p_Cur->strm.buf = mpp_malloc_size(RK_U8, NALU_BUF_MAX_SIZE));
	p_Cur->strm.max_size = NALU_BUF_MAX_SIZE;
	p_Cur->strm.prefixdata[0] = 0xff;
	p_Cur->strm.prefixdata[1] = 0xff;
	p_Cur->strm.prefixdata[2] = 0xff;
	for (i = 0; i < 2; i++)
	{
		MEM_CHECK(p_Cur->listP[i] = mpp_malloc_size(H264_StorePic_t*, MAX_LIST_SIZE*sizeof(H264_StorePic_t*))); // +1 for reordering
		MEM_CHECK(p_Cur->listB[i] = mpp_malloc_size(H264_StorePic_t*, MAX_LIST_SIZE*sizeof(H264_StorePic_t*))); // +1 for reordering
	}
    FunctionOut(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
__FAILED:
	free_cur_ctx(p_Cur);

	return MPP_NOK;
}


static MPP_RET free_vid_ctx(H264dVideoCtx_t *p_Vid)
{
	RK_U32 i = 0;
	INP_CHECK(!p_Vid);
    FunctionIn(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
	for (i = 0; i < MAX_NUM_DPB_LAYERS; i++)
	{
		mpp_free(p_Vid->p_Dpb_layer[i]);
	}
    FunctionOut(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
}
static MPP_RET init_vid_ctx(H264dVideoCtx_t *p_Vid)
{
	RK_U32 i = 0;
	INP_CHECK(!p_Vid);
    FunctionIn(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
	for (i = 0; i < MAX_NUM_DPB_LAYERS; i++)
	{
		MEM_CHECK(p_Vid->p_Dpb_layer[i]  = mpp_calloc(H264_DpbBuf_t, 1));
		p_Vid->p_Dpb_layer[i]->layer_id  = i;
		p_Vid->p_Dpb_layer[i]->p_Vid     = p_Vid;
		p_Vid->p_Dpb_layer[i]->init_done = 0;
	}
	//!< init video pars
	for (i = 0; i < MAXSPS; i++)
	{
		p_Vid->spsSet[i].seq_parameter_set_id = -1;
		p_Vid->subspsSet[i].sps.seq_parameter_set_id = -1;
	}
	for (i = 0; i < MAXPPS; i++)
	{
		p_Vid->ppsSet[i].pic_parameter_set_id = -1;
		p_Vid->ppsSet[i].seq_parameter_set_id = -1;
	}
	//!< init active_sps
	p_Vid->active_sps       = NULL;
	p_Vid->active_subsps    = NULL;
	p_Vid->active_sps_id[0] = -1;
	p_Vid->active_sps_id[1] = -1;
	//!< init subspsSet
	for (i = 0; i < MAXSPS; i++)
	{
		p_Vid->subspsSet[i].sps.seq_parameter_set_id = -1;
		p_Vid->subspsSet[i].num_views_minus1 = -1;
		p_Vid->subspsSet[i].num_level_values_signalled_minus1 = -1;
	}
    FunctionOut(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
__FAILED:
	free_vid_ctx(p_Vid);

	return MPP_NOK;
}

static MPP_RET free_dxva_ctx(H264dDxvaCtx_t *p_dxva)
{		
	INP_CHECK(!p_dxva);
	FunctionIn(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);

	mpp_free(p_dxva->slice_short);
	mpp_free(p_dxva->slice_long);
	mpp_free(p_dxva->bitstream);
	mpp_free(p_dxva->syn.buf);

	FunctionOut(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
	return MPP_OK;
}

static MPP_RET init_dxva_ctx(H264dDxvaCtx_t *p_dxva)
{
	INP_CHECK(!p_dxva);
	FunctionIn(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);

	p_dxva->max_slice_size = MAX_SLICE_SIZE;
	MEM_CHECK(p_dxva->slice_short = mpp_calloc(DXVA_Slice_H264_Short, p_dxva->max_slice_size));
	MEM_CHECK(p_dxva->slice_long  = mpp_calloc(DXVA_Slice_H264_Long,  p_dxva->max_slice_size));
	p_dxva->max_strm_size = FRAME_BUF_MAX_SIZE;
	MEM_CHECK(p_dxva->bitstream = mpp_malloc(RK_U8,p_dxva->max_strm_size));
	MEM_CHECK(p_dxva->syn.buf   = mpp_calloc(DXVA2_DecodeBufferDesc, SYNTAX_BUF_SIZE));

	FunctionOut(p_dxva->p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
	return MPP_OK;

__FAILED:
	return MPP_NOK;
}

static MPP_RET free_dec_ctx(H264_DecCtx_t *p_Dec)
{
	RK_U32 i = 0;
	INP_CHECK(!p_Dec);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
	for (i = 0; i < MAX_TASK_SIZE; i++)
	{
		free_dxva_ctx(&p_Dec->dxva_ctx[i]);
	}
	mpp_free(p_Dec->mem);

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
    return MPP_OK;
}
static MPP_RET init_dec_ctx(H264_DecCtx_t *p_Dec)
{	
	RK_U32 i = 0;
	INP_CHECK(p_Dec);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

	MEM_CHECK(p_Dec->mem = mpp_calloc(H264_DecMem_t, 1));

	p_Dec->dpb_mark       = p_Dec->mem->dpb_mark;         //!< for write out, MAX_DPB_SIZE
	p_Dec->dpb_info       = p_Dec->mem->dpb_info;         //!< 16
	p_Dec->refpic_info_p  = p_Dec->mem->refpic_info_p;    //!< 32
	p_Dec->refpic_info[0] = p_Dec->mem->refpic_info[0];   //!< [2][32]
	p_Dec->refpic_info[1] = p_Dec->mem->refpic_info[1];   //!< [2][32]
	p_Dec->dxva_ctx       = p_Dec->mem->dxva_ctx;         //!< MAX_TASK_SIZE

	for (i = 0; i < MAX_TASK_SIZE; i++)
	{
		p_Dec->dxva_ctx[i].p_Dec = p_Dec;
		FUN_CHECK(init_dxva_ctx(&p_Dec->dxva_ctx[i]));
	}	
	//!< init Dpb_memory Mark
	for (i = 0; i < MAX_DPB_SIZE; i++)
	{
		p_Dec->dpb_mark[i].index = i;
	}
	//!< set Dec support decoder method
	p_Dec->spt_decode_mtds = MPP_DEC_BY_FRAME | MPP_DEC_BY_SLICE;
	p_Dec->next_state = SliceSTATE_ResetSlice;
	p_Dec->nalu_ret = NALU_NULL;
	p_Dec->first_frame_flag = 1;
__RETURN:
    return MPP_OK;

__FAILED:
	free_dec_ctx(p_Dec);

	return MPP_NOK;
}




/*!
***********************************************************************
* \brief
*   alloc all buffer
***********************************************************************
*/

MPP_RET h264d_init(void *decoder, MppParserInitCfg *init)
{
    MPP_RET ret = MPP_NOK;
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
    INP_CHECK(!p_Dec);
    // init logctx
    FUN_CHECK(ret = h264d_log_init(&p_Dec->logctx, p_Dec->logctxbuf));
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    MEM_CHECK(p_Dec->p_Inp = mpp_calloc(H264dInputCtx_t, 1));
    MEM_CHECK(p_Dec->p_Cur = mpp_calloc(H264dCurCtx_t, 1));
    MEM_CHECK(p_Dec->p_Vid = mpp_calloc(H264dVideoCtx_t, 1));
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

    //!< return max task size
    init->task_count = MAX_TASK_SIZE; // return
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);

__RETURN:
    return MPP_OK;
__FAILED:
    h264d_deinit(decoder);

    return MPP_NOK;
}
/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET h264d_deinit(void *decoder)
{
    H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;

	INP_CHECK(decoder);
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);

    free_input_ctx(p_Dec->p_Inp);
    mpp_free(p_Dec->p_Inp);
    free_cur_ctx(p_Dec->p_Cur);
    mpp_free(p_Dec->p_Cur);
    free_vid_ctx(p_Dec->p_Vid);
    mpp_free(p_Dec->p_Vid);
    free_dec_ctx(p_Dec);

    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
    h264d_log_deinit(&p_Dec->logctx);
__RETURN:
    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET  h264d_reset(void *decoder)
{
	INP_CHECK(decoder);

    (void)decoder;
__RETURN:
    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET  h264d_flush(void *decoder)
{
	INP_CHECK(decoder);

    (void)decoder;
__RETURN:
    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET  h264d_control(void *decoder, RK_S32 cmd_type, void *param)
{



    (void)decoder;
    (void)cmd_type;
    (void)param;


    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   parser
***********************************************************************
*/
MPP_RET h264d_parse(void *decoder, MppPacket in_pkt, HalDecTask *in_task)
{
	H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
	MppPacketImpl *pkt = (MppPacketImpl *)in_pkt;	
	INP_CHECK(!decoder && !in_pkt && !in_task);
	FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);







	pkt->size = (pkt->size >= 500) ? (pkt->size - 500) : 0;

	in_task->valid = pkt->size ? 0 : 1;





	FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
__RETURN:
	return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/

const MppDecParser api_h264d_parser = {
    "h264d_parse",
    MPP_VIDEO_CodingAVC,
    sizeof(H264_DecCtx_t),
    0,
    h264d_init,
    h264d_deinit,
    h264d_parse,
    h264d_reset,
    h264d_flush,
    h264d_control,
};
