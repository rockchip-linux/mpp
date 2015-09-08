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



#define MODULE_TAG "h264d_api"



static MPP_RET free_input_ctx(H264dInputCtx_t *p_Inp)
{
	FunctionIn(p_Inp->p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Inp->p_Dec->logctx->parr[RUN_PARSE]);
	return MPP_OK;
}
static MPP_RET init_input_ctx(H264dInputCtx_t *p_Inp)
{
	FunctionIn(p_Inp->p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Inp->p_Dec->logctx->parr[RUN_PARSE]);

	return MPP_OK;
}


static MPP_RET free_cur_ctx(H264dCurCtx_t *p_Cur)
{
	FunctionIn(p_Cur->p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Cur->p_Dec->logctx->parr[RUN_PARSE]);

	return MPP_OK;
}
static MPP_RET init_cur_ctx(H264dCurCtx_t *p_Cur)
{
	FunctionIn(p_Cur->p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Cur->p_Dec->logctx->parr[RUN_PARSE]);
	return MPP_OK;
}


static MPP_RET free_vid_ctx(H264dVideoCtx_t *p_Vid)
{
	FunctionIn(p_Vid->p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Vid->p_Dec->logctx->parr[RUN_PARSE]);

	return MPP_OK;
}
static MPP_RET init_vid_ctx(H264dVideoCtx_t *p_Vid)
{
	FunctionIn(p_Vid->p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Vid->p_Dec->logctx->parr[RUN_PARSE]);

	return MPP_OK;
}



static MPP_RET free_dec_ctx(H264_DecCtx_t *p_Dec)
{
	FunctionIn(p_Dec->logctx->parr[RUN_PARSE]);


	FunctionOut(p_Dec->logctx->parr[RUN_PARSE]);

	return MPP_OK;
}
static MPP_RET init_dec_ctx(H264_DecCtx_t *p_Dec)
{
	FunctionIn(p_Dec->logctx->parr[RUN_PARSE]);


	

	//!< set task max size
	p_Dec->task_max_size = 2;
	FunctionOut(p_Dec->logctx->parr[RUN_PARSE]);

	return MPP_OK;
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
	RET_CHECK(!p_Dec);
	// init logctx
	MEM_CHECK(p_Dec->logctx = mpp_calloc(H264dLogCtx_t, 1));
	FUN_CHECK(ret = h264d_log_init(p_Dec->logctx));
	FunctionIn(p_Dec->logctx->parr[RUN_PARSE]);
	MEM_CHECK(p_Dec->p_Inp = mpp_calloc(H264dInputCtx_t, 1));
	MEM_CHECK(p_Dec->p_Cur = mpp_calloc(H264dCurCtx_t, 1));
	MEM_CHECK(p_Dec->p_Vid = mpp_calloc(H264dVideoCtx_t, 1));
	p_Dec->p_Inp->p_Dec = p_Dec;
	p_Dec->p_Inp->p_Cur = p_Dec->p_Cur;
	p_Dec->p_Inp->p_Vid = p_Dec->p_Vid;
	FUN_CHECK(ret = init_input_ctx(p_Dec->p_Inp));
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
	init->task_count = p_Dec->task_max_size; // return 

	FunctionOut(p_Dec->logctx->parr[RUN_PARSE]);
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
MPP_RET  h264d_deinit(void *decoder)
{	
	H264_DecCtx_t *p_Dec = (H264_DecCtx_t *)decoder;
		
	FunctionIn(p_Dec->logctx->parr[RUN_PARSE]);

	RET_CHECK(!p_Dec);
	free_input_ctx(p_Dec->p_Inp);
	mpp_free(p_Dec->p_Inp);
	free_cur_ctx(p_Dec->p_Cur);
	mpp_free(p_Dec->p_Cur);
	free_vid_ctx(p_Dec->p_Vid);
	mpp_free(p_Dec->p_Vid);		
	free_dec_ctx(p_Dec);		
	h264d_log_deinit(p_Dec->logctx);
	mpp_free(p_Dec->logctx);		

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
    

	(void)decoder;

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
    

	(void)decoder;

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

	MppPacketImpl *pkt = (MppPacketImpl *)in_pkt;



	pkt->size = (pkt->size >= 500) ? (pkt->size - 500) : 0;

	in_task->valid = pkt->size ? 0 : 1;

	(void)decoder;
	(void)in_task;

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
