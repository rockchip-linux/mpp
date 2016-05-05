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

#include "mpp_time.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer_impl.h"


#include "avsd_api.h"
#include "avsd_parse.h"


RK_U32 avsd_parse_debug = 0;


static MPP_RET free_input_ctx(AvsdInputCtx_t *p_inp)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p_inp);
	AVSD_PARSE_TRACE("In.");

	MPP_FCLOSE(p_inp->fp_log);
__RETURN:
	AVSD_PARSE_TRACE("Out.");
	return ret = MPP_OK;
}
static MPP_RET init_input_ctx(AvsdInputCtx_t *p_inp, ParserCfg *init)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p_inp && !init);
	AVSD_PARSE_TRACE("In.");

	p_inp->init = *init;
#if defined(_WIN32)
	if ((p_inp->fp_log = fopen("F:/avs_log/avs_runlog.txt", "wb")) == 0) {
		mpp_log("Wanning, open file, %s(%d)", __FUNCTION__, __LINE__);
	}
#endif
__RETURN:
	AVSD_PARSE_TRACE("Out.");
	return ret = MPP_OK;
}


static MPP_RET free_cur_ctx(AvsdCurCtx_t *p_cur)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p_cur);
	AVSD_PARSE_TRACE("In.");


__RETURN:
	AVSD_PARSE_TRACE("Out.");

	return ret = MPP_OK;
}
static MPP_RET init_cur_ctx(AvsdCurCtx_t *p_cur)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	AvsdCurStream_t *p_strm = NULL;
	
	INP_CHECK(ret, !p_cur);
	AVSD_PARSE_TRACE("In.");

	p_strm = &p_cur->m_strm;
	p_strm->prefixdata = 0xffffffff;

__RETURN:
	AVSD_PARSE_TRACE("Out.");
	return ret = MPP_OK;

}


static MPP_RET free_vid_ctx(AvsdVideoCtx_t *p_vid)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p_vid);
	AVSD_PARSE_TRACE("In.");


__RETURN:
	AVSD_PARSE_TRACE("Out.");

	return ret = MPP_OK;
}
static MPP_RET init_vid_ctx(AvsdVideoCtx_t *p_vid)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p_vid);
	AVSD_PARSE_TRACE("In.");

__RETURN:
	AVSD_PARSE_TRACE("Out.");
	return ret = MPP_OK;
}

static MPP_RET free_bitstream(AvsdBitstream_t *p)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p);
	AVSD_PARSE_TRACE("In.");

	MPP_FREE(p->pbuf);

__RETURN:
	AVSD_PARSE_TRACE("Out.");

	return ret = MPP_OK;
}

static MPP_RET init_bitstream(AvsdBitstream_t *p)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, !p);
	AVSD_PARSE_TRACE("In.");
	p->size = MAX_BITSTREAM_SIZE;
	p->pbuf = mpp_malloc(RK_U8, MAX_BITSTREAM_SIZE);
	MEM_CHECK(ret, p->pbuf);

__RETURN:
	AVSD_PARSE_TRACE("Out.");
	return ret = MPP_OK;
__FAILED:
	return ret;
}


static MPP_RET free_dec_ctx(Avs_DecCtx_t *p_dec)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	INP_CHECK(ret, NULL == p_dec);
	AVSD_PARSE_TRACE("In.");
	lib_avsd_free(p_dec->libdec);
	mpp_packet_deinit(&p_dec->task_pkt);
	free_bitstream(p_dec->bitstream);
	MPP_FREE(p_dec->mem);	

__RETURN:
	AVSD_PARSE_TRACE("Out.");

	return ret = MPP_OK;
}



static MPP_RET init_dec_ctx(Avs_DecCtx_t *p_dec)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	
	INP_CHECK(ret, !p_dec);
	AVSD_PARSE_TRACE("In.");	

	mpp_buf_slot_setup(p_dec->frame_slots, 4);
	p_dec->mem = mpp_calloc(AvsdMemory_t, 1);
	MEM_CHECK(ret, p_dec->mem);
	p_dec->bitstream = &p_dec->mem->bitstream;
	FUN_CHECK(ret = init_bitstream(p_dec->bitstream));
	//!< malloc mpp packet
	mpp_packet_init(&p_dec->task_pkt, p_dec->bitstream->pbuf, p_dec->bitstream->size);
	mpp_packet_set_length(p_dec->task_pkt, 0);
	MEM_CHECK(ret, p_dec->task_pkt);
	//!< malloc libavsd.so
	p_dec->libdec = lib_avsd_malloc(p_dec);
	MEM_CHECK(ret, p_dec->libdec);
__RETURN:
	AVSD_PARSE_TRACE("Out.");

	return ret = MPP_OK;
__FAILED:
	free_dec_ctx(p_dec);

	return ret;
}
/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET avsd_deinit(void *decoder)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	Avs_DecCtx_t *p_dec = (Avs_DecCtx_t *)decoder;

	INP_CHECK(ret, !decoder);
	AVSD_PARSE_TRACE("In.");
	avsd_flush(decoder);
	free_input_ctx(p_dec->p_inp);
	MPP_FREE(p_dec->p_inp);
	free_cur_ctx(p_dec->p_cur);
	MPP_FREE(p_dec->p_cur);
	free_vid_ctx(p_dec->p_vid);
	MPP_FREE(p_dec->p_vid);
	free_dec_ctx(p_dec);
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
	Avs_DecCtx_t *p_dec = (Avs_DecCtx_t *)decoder;

	AVSD_PARSE_TRACE("In.");
	INP_CHECK(ret, !p_dec);
	memset(p_dec, 0, sizeof(Avs_DecCtx_t));
	// init logctx
	mpp_env_get_u32("avsd_debug", &avsd_parse_debug, 0);
	//!< get init frame_slots and packet_slots
	p_dec->frame_slots = init->frame_slots;
	p_dec->packet_slots = init->packet_slots;
	//!< malloc decoder buffer
	p_dec->p_inp = mpp_calloc(AvsdInputCtx_t, 1);
	p_dec->p_cur = mpp_calloc(AvsdCurCtx_t, 1);
	p_dec->p_vid = mpp_calloc(AvsdVideoCtx_t, 1);
	MEM_CHECK(ret, p_dec->p_inp && p_dec->p_cur && p_dec->p_vid);

	p_dec->p_inp->p_dec = p_dec;
	p_dec->p_inp->p_cur = p_dec->p_cur;
	p_dec->p_inp->p_vid = p_dec->p_vid;
	FUN_CHECK(ret = init_input_ctx(p_dec->p_inp, init));
	p_dec->p_cur->p_dec = p_dec;
	p_dec->p_cur->p_inp = p_dec->p_inp;
	p_dec->p_cur->p_vid = p_dec->p_vid;
	FUN_CHECK(ret = init_cur_ctx(p_dec->p_cur));
	p_dec->p_vid->p_dec = p_dec;
	p_dec->p_vid->p_inp = p_dec->p_inp;
	p_dec->p_vid->p_cur = p_dec->p_cur;
	FUN_CHECK(ret = init_vid_ctx(p_dec->p_vid));
	FUN_CHECK(ret = init_dec_ctx(p_dec));
__RETURN:
	(void)decoder;
	(void)init;
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
	Avs_DecCtx_t *p_dec = (Avs_DecCtx_t *)decoder;

    AVSD_PARSE_TRACE("In.");
	lib_reset(p_dec->libdec);

	AVSD_PARSE_TRACE("Out.");
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
	Avs_DecCtx_t *p_dec = (Avs_DecCtx_t *)decoder;
    AVSD_PARSE_TRACE("In.");

	lib_flush(p_dec->libdec);

	AVSD_PARSE_TRACE("Out.");
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
	AvsdInputCtx_t *p_inp = NULL;
	Avs_DecCtx_t   *p_dec = (Avs_DecCtx_t *)decoder;

	AVSD_PARSE_TRACE("In.");
	INP_CHECK(ret, !decoder && !pkt && !task);
	p_inp = p_dec->p_inp;
	if (p_inp->has_get_eos) {
		mpp_packet_set_length(pkt, 0);
		goto __RETURN;
	}
	p_inp->in_pkt = pkt;
	p_inp->in_task = task;

	if (mpp_packet_get_eos(pkt)) {
		if (mpp_packet_get_length(pkt) < 4) {
			avsd_flush(decoder);
		}
		p_inp->has_get_eos = 1;
	}
	AVSD_DBG(AVSD_DBG_INPUT, "[pkt_in_timeUs] in_pts=%lld, pkt_eos=%d, len=%d, pkt_no=%d",
		mpp_packet_get_pts(pkt), mpp_packet_get_eos(pkt), mpp_packet_get_length(pkt), p_inp->pkt_no++);

	if (mpp_packet_get_length(pkt) > MAX_STREM_IN_SIZE) {
		AVSD_DBG(AVSD_DBG_ERROR, "[pkt_in_timeUs] input error, stream too large");
		mpp_packet_set_length(pkt, 0);
		ret = MPP_NOK;
		goto __FAILED;
	}
	p_inp->in_pts = mpp_packet_get_pts(pkt);
	p_inp->in_dts = mpp_packet_get_dts(pkt);

	memset(task, 0, sizeof(HalDecTask));
	do {
		(ret = avsd_parse_prepare(p_inp, p_dec->p_cur));

	} while (mpp_packet_get_length(pkt) && !task->valid);

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
	RK_S64 p_s = 0, p_e = 0, diff = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
	Avs_DecCtx_t *p_dec = (Avs_DecCtx_t *)decoder;
    AVSD_PARSE_TRACE("In.");

	p_s = mpp_time();
	task->valid = 0;
	memset(task->refer, -1, sizeof(task->refer));
	lib_parse_one_frame(p_dec->libdec, task);
	if (task->flags.eos) {
		avsd_flush(decoder);
		goto __RETURN;
	}
	if (task->valid) {
		lib_init_one_frame(p_dec->libdec, task);	
		p_dec->parse_no++;
		p_e = mpp_time();
		diff = (p_e - p_s) / 1000;
		p_dec->parse_time += diff;
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] parser stream consume time %lld, av=%lld ", diff, p_dec->parse_time / p_dec->parse_no);
	}
__RETURN:
    (void)decoder;
    (void)task;
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
	RK_S64 p_s = 0, p_e = 0, diff = 0;
	MPP_RET ret = MPP_ERR_UNKNOW;
	HalTaskInfo *task = (HalTaskInfo *)info;
	Avs_DecCtx_t *p_dec = (Avs_DecCtx_t *)decoder;
	AVSD_PARSE_TRACE("In.");
	AVSD_PARSE_TRACE("[avsd_parse_decode] frame_dec_no=%d", p_dec->dec_no);
	p_dec->dec_no++;
	{
		p_s = mpp_time();
		lib_decode_one_frame(p_dec->libdec, &task->dec);
		p_e = mpp_time();
		diff = (p_e - p_s) / 1000;
		p_dec->decode_frame_time += diff;
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] decode one frame total time=%lld, av=%lld ", diff, p_dec->decode_frame_time / p_dec->dec_no);

		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] @ decode init av=%lld ", p_dec->decode_init_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] @ decode data av=%lld ", p_dec->decode_data_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]     # read slice header   consume av=%lld ", p_dec->read_slice_header_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]     # init arodeco slice  consume av=%lld ", p_dec->init_arideco_slice_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]     # start  marco blocks consume av=%lld ", p_dec->start_marcoblock_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]     # read   marco blocks consume av=%lld ", p_dec->read_marcoblock_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]     # decode marco blocks consume av=%lld ", p_dec->decode_marcoblock_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]         $ get    marco blocks consume av=%lld ", p_dec->get_block_time          / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]         $ intra  pred  blocks consume av=%lld ", p_dec->intra_pred_block_time   / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]         $ inter  pred  blocks consume av=%lld ", p_dec->inter_pred_block_time   / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time]         $ idc_dequant  blocks consume av=%lld ", p_dec->idc_dequaut_time        / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] @ deblocking   consume av=%lld ", p_dec->deblock_time       / (1000 *p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] @ decode updte consume av=%lld ", p_dec->decode_update_time / (1000 * p_dec->dec_no));
		AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] @ frame post   consume av=%lld ", p_dec->frame_post_time    / (1000 * p_dec->dec_no));
	}
	{
		MppBuffer mbuffer = NULL;
		mpp_buf_slot_get_prop(p_dec->frame_slots, task->dec.output, SLOT_BUFFER, &mbuffer);

		if (mbuffer) {
			RK_U8 *p = (RK_U8 *)mpp_buffer_get_ptr(mbuffer);
			p_s = mpp_time();
			nv12_copy_buffer(p_dec->libdec, p);
			p_e = mpp_time();
			diff = (p_e - p_s) / 1000;
			p_dec->nvcopy_time += diff;
			AVSD_DBG(AVSD_DBG_TIME, "[avsd_time] copy one frame consume time=%lld, av=%lld ", diff, p_dec->nvcopy_time / p_dec->dec_no);
		}	
	}

	(void)task;
	(void)decoder;
	(void)info;
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
    "avsd_parse",
    MPP_VIDEO_CodingAVS,
    sizeof(Avs_DecCtx_t),
    0,
    avsd_init,
    avsd_deinit,
    avsd_prepare,
    avsd_parse,
    avsd_reset,
    avsd_flush,
    avsd_control,
    avsd_callback,
};

