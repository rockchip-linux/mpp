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

#define MODULE_TAG "h264d_parse"

#include <string.h>
#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "hal_task.h"

#include "h264d_parse.h"
#include "h264d_slice.h"
#include "h264d_sps.h"
#include "h264d_pps.h"
#include "h264d_sei.h"
#include "h264d_init.h"
#include "h264d_fill.h"

#define  HEAD_MAX_SIZE        12800

static const RK_U8 g_start_precode[3] = {0, 0, 1};

typedef struct h264d_nalu_head_t {
    RK_U16  is_frame_end;
    RK_U16  nalu_type;
    RK_U32  sodb_len;
} H264dNaluHead_t;


static RK_U16 U16_AT(const RK_U8 *ptr)
{
	return ptr[0] << 8 | ptr[1];
}

static RK_U32 U32_AT(const RK_U8 *ptr) 
{
	return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

static RK_U32 parse_nal_size(RK_U8 nal_size, RK_U8 *data) 
{
	switch (nal_size) {
	case 1:
		return *data;
	case 2:
		return U16_AT(data);
	case 3:
		return ((RK_U64)data[0] << 16) | U16_AT(&data[1]);
	case 4:		
		return U32_AT(data);
	}
	return 0;
}

static void reset_slice(H264dVideoCtx_t *p_Vid)
{
    RK_U32 i = 0, j = 0;
    H264_SLICE_t *currSlice = &p_Vid->p_Cur->slice;
    FunctionIn(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
    memset(currSlice, 0, sizeof(H264_SLICE_t));
    //-- re-init parameters
    currSlice->view_id = -1;
    currSlice->p_Vid   = p_Vid;
    currSlice->p_Dec   = p_Vid->p_Dec;
    currSlice->p_Cur   = p_Vid->p_Cur;
    currSlice->p_Inp   = p_Vid->p_Inp;
    currSlice->logctx  = &p_Vid->p_Dec->logctx;
    currSlice->active_sps = p_Vid->active_sps;
    currSlice->active_pps = p_Vid->active_pps;
    //--- reset listP listB
    for (i = 0; i < 2; i++) {
        currSlice->listP[i] = p_Vid->p_Cur->listP[i];
        currSlice->listB[i] = p_Vid->p_Cur->listB[i];
        for (j = 0; j < MAX_LIST_SIZE; j++) {
            currSlice->listP[i][j] = NULL;
            currSlice->listB[i][j] = NULL;
        }
        currSlice->listXsizeP[i] = 0;
        currSlice->listXsizeB[i] = 0;
    }
    FunctionOut(p_Vid->p_Dec->logctx.parr[RUN_PARSE]);
}

static MPP_RET realloc_buffer(RK_U8 **buf, RK_U32 *max_size, RK_U32 add_size)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
	add_size = MPP_ALIGN(add_size, 16);
	//mpp_log("xxxxxxxx max_size=%d, add_size=%d \n",(*max_size), add_size);
    (*buf) = mpp_realloc((*buf), RK_U8, ((*max_size) + add_size));
    MEM_CHECK(ret, (*buf));
    (*max_size) += add_size;

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void reset_nalu(H264dCurStream_t *p_strm)
{
    if (p_strm->endcode_found) {
        p_strm->startcode_found = p_strm->endcode_found;
        p_strm->nalu_len = 0;
        p_strm->nalu_type = NALU_TYPE_NULL;
        p_strm->endcode_found = 0;		
    }
}

static void find_prefix_code(RK_U8 *p_data, H264dCurStream_t *p_strm)
{
    p_strm->prefixdata[0] = p_strm->prefixdata[1];
    p_strm->prefixdata[1] = p_strm->prefixdata[2];
    p_strm->prefixdata[2] = *p_data;

    if (p_strm->prefixdata[0] == 0x00
        && p_strm->prefixdata[1] == 0x00
        && p_strm->prefixdata[2] == 0x01) {
        if (p_strm->startcode_found) {
            p_strm->endcode_found = 1;
        } else {
            p_strm->startcode_found = 1;
        }
    }
}
#if 0
static MPP_RET read_one_nalu(H264dInputCtx_t *p_Inp, H264dCurStream_t *p_strm)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dLogCtx_t   *logctx = &p_Inp->p_Dec->logctx;
    H264_DecCtx_t   *p_Dec  = p_Inp->p_Dec;

    FunctionIn(logctx->parr[RUN_PARSE]);
	reset_nalu(p_strm);
    while (p_Inp->in_length > 0) {
        p_strm->curdata = &p_Inp->in_buf[p_strm->nalu_offset++];
        (*p_Inp->in_size) -= 1;
        p_Inp->in_length--;
        if (p_strm->startcode_found) {

            if (p_strm->nalu_len >= p_strm->nalu_max_size) {
                FUN_CHECK(ret = realloc_buffer(&p_strm->nalu_buf, &p_strm->nalu_max_size, NALU_BUF_ADD_SIZE));
            }
            p_strm->nalu_buf[p_strm->nalu_len++] = *p_strm->curdata;
        }
        find_prefix_code(p_strm->curdata, p_strm);
        if (p_strm->endcode_found) {
            p_strm->nalu_len -= START_PREFIX_3BYTE;
            while (p_strm->nalu_buf[p_strm->nalu_len - 1] == 0x00) { //!< find non-zeros byte
                p_strm->nalu_len--;
            }
            p_Dec->nalu_ret = EndOfNalu;			
            break;
        }
		p_Dec->nalu_ret = MidOfNalu;
    }
    if (!p_Inp->in_length) { //!< check input
        p_strm->nalu_offset = 0;
        p_Dec->nalu_ret = HaveNoStream;
    }

    FunctionOut(logctx->parr[RUN_PARSE]);

    return ret = MPP_OK;
__FAILED:

    return ret;
}
#endif

static MPP_RET parser_nalu_header(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dLogCtx_t   *logctx   = currSlice->logctx;
    H264dCurCtx_t   *p_Cur    = currSlice->p_Cur;
    BitReadCtx_t    *p_bitctx = &p_Cur->bitctx;
    H264_Nalu_t     *cur_nal  = &p_Cur->nalu;

    FunctionIn(logctx->parr[RUN_PARSE]);
    mpp_set_bitread_ctx(p_bitctx, cur_nal->sodb_buf, cur_nal->sodb_len);

    set_bitread_logctx(p_bitctx, logctx->parr[LOG_READ_NALU]);
    LogInfo(p_bitctx->ctx, "================== NAL begin ===================");
    READ_BITS(p_bitctx, 1, &cur_nal->forbidden_bit, "forbidden_bit");
    ASSERT(cur_nal->forbidden_bit == 0);
    READ_BITS(p_bitctx, 2, (RK_S32 *)&cur_nal->nal_reference_idc, "nal_ref_idc");
    READ_BITS(p_bitctx, 5, (RK_S32 *)&cur_nal->nalu_type, "nalu_type");
    if (g_nalu_cnt0 == 2384) {
        g_nalu_cnt0 = g_nalu_cnt0;
    }
    //if (g_debug_file1 == NULL)
    //{
    //  g_debug_file1 = fopen("rk_debugfile_view1.txt", "wb");
    //}
    //FPRINT(g_debug_file1, "g_nalu_cnt = %d, nalu_type = %d, nalu_size = %d\n", g_nalu_cnt++, cur_nal->nalu_type, cur_nal->sodb_len);
	//if((cur_nal->nalu_type == NALU_TYPE_SLICE)
	//	|| (cur_nal->nalu_type == NALU_TYPE_IDR)
	//	|| (cur_nal->nalu_type == NALU_TYPE_SPS)
	//	|| (cur_nal->nalu_type == NALU_TYPE_PPS)
	//	|| (cur_nal->nalu_type == NALU_TYPE_SUB_SPS)
	//	|| (cur_nal->nalu_type == NALU_TYPE_SEI)){
 //       FPRINT(g_debug_file0, "g_nalu_cnt=%d, nal_type=%d, sodb_len=%d \n", g_nalu_cnt0++, cur_nal->nalu_type, cur_nal->sodb_len);
 //   }
    cur_nal->ualu_header_bytes = 1;
    currSlice->svc_extension_flag = -1; //!< initialize to -1
    if ((cur_nal->nalu_type == NALU_TYPE_PREFIX) || (cur_nal->nalu_type == NALU_TYPE_SLC_EXT)) {
        READ_ONEBIT(p_bitctx, &currSlice->svc_extension_flag, "svc_extension_flag");
        if (currSlice->svc_extension_flag) {
            //FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nalu_type, cur_nal->sodb_len);
            currSlice->mvcExt.valid = 0;
            LogInfo(logctx->parr[RUN_PARSE], "svc_extension is not supported.");
            goto __FAILED;
        } else { //!< MVC
            currSlice->mvcExt.valid = 1;
			p_Cur->p_Dec->mvc_valid = 1;
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.non_idr_flag,     "nalu_type");
            READ_BITS(p_bitctx,    6, &currSlice->mvcExt.priority_id,      "priority_id");
            READ_BITS(p_bitctx,   10, &currSlice->mvcExt.view_id,          "view_id");
            READ_BITS(p_bitctx,    3, &currSlice->mvcExt.temporal_id,      "temporal_id");
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.anchor_pic_flag,  "anchor_pic_flag");
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.inter_view_flag,  "inter_view_flag");
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.reserved_one_bit, "reserved_one_bit");
            ASSERT(currSlice->mvcExt.reserved_one_bit == 1);
            currSlice->mvcExt.iPrefixNALU = (cur_nal->nalu_type == NALU_TYPE_PREFIX) ? 1 : 0;
            //!< combine NALU_TYPE_SLC_EXT into NALU_TYPE_SLICE
            if (cur_nal->nalu_type == NALU_TYPE_SLC_EXT) {
                cur_nal->nalu_type = NALU_TYPE_SLICE;
            }
            //FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nalu_type, cur_nal->sodb_len);
        }
        cur_nal->ualu_header_bytes += 3;
    } else {
        //FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nalu_type, cur_nal->sodb_len);
    }
    mpp_set_bitread_ctx(p_bitctx, (cur_nal->sodb_buf + cur_nal->ualu_header_bytes), (cur_nal->sodb_len - cur_nal->ualu_header_bytes)); // reset

    p_Cur->p_Dec->nalu_ret = StartofNalu;
    FunctionOut(logctx->parr[RUN_PARSE]);

    return ret = MPP_OK;
__BITREAD_ERR:
    p_Cur->p_Dec->nalu_ret = ReadNaluError;
    return ret = p_bitctx->ret;
__FAILED:
    p_Cur->p_Dec->nalu_ret = StreamError;
    return ret;
}

static MPP_RET parser_one_nalu(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    LogCtx_t *runlog = currSlice->logctx->parr[RUN_PARSE];

    FunctionIn(runlog);
    FUN_CHECK(ret = parser_nalu_header(currSlice));
    //!< nalu_parse
    switch (currSlice->p_Cur->nalu.nalu_type) {
    case NALU_TYPE_SLICE:
    case NALU_TYPE_IDR:
        FUN_CHECK(ret = process_slice(currSlice));
        if (currSlice->is_new_picture) {
            currSlice->p_Dec->nalu_ret = StartOfPicture;
            //FPRINT(g_debug_file0, "----- start of picture ---- \n");
        } else {
            currSlice->p_Dec->nalu_ret = StartOfSlice;
        }
        LogTrace(runlog, "nalu_type=SLICE.");
        break;
    case NALU_TYPE_SPS:
        FUN_CHECK(ret = process_sps(currSlice));
        currSlice->p_Dec->nalu_ret = NALU_SPS;
        LogTrace(runlog, "nalu_type=SPS");
        break;
    case NALU_TYPE_PPS:
        FUN_CHECK(ret = process_pps(currSlice));
        currSlice->p_Dec->nalu_ret = NALU_PPS;
        LogTrace(runlog, "nalu_type=PPS");
        break;
    case NALU_TYPE_SUB_SPS:
        FUN_CHECK(ret = process_subsps(currSlice));
        currSlice->p_Dec->nalu_ret = NALU_SubSPS;
        LogTrace(runlog, "nalu_type=SUB_SPS");
        break;
    case NALU_TYPE_SEI:
        //FUN_CHECK(ret = process_sei(currSlice));
        LogTrace(runlog, "nalu_type=SEI");
        currSlice->p_Dec->nalu_ret = NALU_SEI;
        break;
    case NALU_TYPE_SLC_EXT:
        LogTrace(runlog, "Found NALU_TYPE_SLC_EXT.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_PREFIX:
        LogTrace(runlog, "Found NALU_TYPE_PREFIX.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_AUD:
        LogTrace(runlog, "Found NALU_TYPE_AUD.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_EOSEQ:
        LogTrace(runlog, "Found NALU_TYPE_EOSEQ.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_EOSTREAM:
        LogTrace(runlog, "Found NALU_TYPE_EOSTREAM.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_FILL:
        LogTrace(runlog, "Found NALU_TYPE_FILL.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_VDRD:
        LogTrace(runlog, "Found NALU_TYPE_VDRD.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_DPA:
    case NALU_TYPE_DPB:
    case NALU_TYPE_DPC:
        LogError(runlog, "Found NALU_TYPE_DPA DPB DPC, and not supported.");
        currSlice->p_Dec->nalu_ret = NaluNotSupport;
        break;
    default:
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    }
    FunctionOut(runlog);

    return ret = MPP_OK;
__FAILED:
	currSlice->p_Dec->nalu_ret = ReadNaluError;
    return ret;
}
#if 0
static MPP_RET analyze_cur_nalu(H264dCurCtx_t *p_Cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 nalu_header_bytes  = 0;
    RK_U32 first_mb_in_slice  = 0;
    H264dLogCtx_t   *logctx   = &p_Cur->p_Dec->logctx;
    BitReadCtx_t    *p_bitctx = &p_Cur->bitctx;
    H264dCurStream_t *p_strm  = &p_Cur->strm;
    RK_U32      forbidden_bit = -1;
    RK_U32  nal_reference_idc = -1;
    RK_U32 svc_extension_flag = -1;

    FunctionIn(logctx->parr[RUN_PARSE]);
	 memset(p_bitctx, 0, sizeof(BitReadCtx_t));
    mpp_set_bitread_ctx(p_bitctx, p_strm->nalu_buf, 4);
    set_bitread_logctx(p_bitctx, logctx->parr[LOG_READ_NALU]);
    READ_BITS(p_bitctx, 1, &forbidden_bit);
    ASSERT(forbidden_bit == 0);
    READ_BITS(p_bitctx, 2, &nal_reference_idc);
    READ_BITS(p_bitctx, 5, &p_strm->nalu_type);
    //if (g_nalu_cnt1 == 29) {
    //    g_nalu_cnt1 = g_nalu_cnt1;
    //}
    //FPRINT(g_debug_file0, "[Analyze_NALU] g_framecnt=%d, g_nalu_cnt=%d, nal_type=%d, sodb_len=%d \n", p_Cur->p_Vid->g_framecnt, g_nalu_cnt1++, p_strm->nalu_type, p_strm->nalu_len);

    nalu_header_bytes = 1;
    if ((p_strm->nalu_type == NALU_TYPE_PREFIX) || (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) {
        READ_BITS(p_bitctx, 1, &svc_extension_flag);
        if (svc_extension_flag) {
            LogInfo(logctx->parr[RUN_PARSE], "svc_extension is not supported.");
            goto __FAILED;
        } else {
            if (p_strm->nalu_type == NALU_TYPE_SLC_EXT) {
                p_strm->nalu_type = NALU_TYPE_SLICE;
            }
        }
        nalu_header_bytes += 3;
    }
    //-- parse slice
    if ( p_strm->nalu_type == NALU_TYPE_SLICE || p_strm->nalu_type == NALU_TYPE_IDR) {
        mpp_set_bitread_ctx(p_bitctx, (p_strm->nalu_buf + nalu_header_bytes), 4); // reset
        READ_UE(p_bitctx, &first_mb_in_slice);
		//if (first_mb_in_slice == 0)
		{
			//FPRINT(g_debug_file0, "[Analyze_NALU] first_mb_in_sile ==0 \n")
			first_mb_in_slice = first_mb_in_slice;
		}

        if (!p_Cur->p_Dec->is_first_frame && (first_mb_in_slice == 0)) {
            p_Cur->p_Dec->is_new_frame = 1;
        }
        p_Cur->p_Dec->is_first_frame = 0;
    }

    FunctionOut(logctx->parr[RUN_PARSE]);
    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = p_bitctx->ret;
__FAILED:
    return ret;
}
#endif
static MPP_RET add_empty_nalu(H264dCurStream_t *p_strm)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8  *p_des = NULL;
	RK_U32  add_size = 0;
	add_size = MPP_MAX(sizeof(H264dNaluHead_t), HEAD_BUF_ADD_SIZE);
    if ((p_strm->head_offset + add_size) >= p_strm->head_max_size) {
        FUN_CHECK(ret = realloc_buffer(&p_strm->head_buf, &p_strm->head_max_size, HEAD_BUF_ADD_SIZE));
    }
    p_des = &p_strm->head_buf[p_strm->head_offset];

    ((H264dNaluHead_t *)p_des)->is_frame_end  = 1;
    ((H264dNaluHead_t *)p_des)->nalu_type = 0;
    ((H264dNaluHead_t *)p_des)->sodb_len = 0;
    p_strm->head_offset += add_size;

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET store_cur_nalu(H264dCurStream_t *p_strm, H264dDxvaCtx_t *dxva_ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8 *p_des = NULL;
    RK_U32 add_size = 0;

    //!< fill head buffer
	//mpp_log("store_cur_nalu function In \n");
	if (   (p_strm->nalu_type == NALU_TYPE_SLICE)
		|| (p_strm->nalu_type == NALU_TYPE_IDR)
		|| (p_strm->nalu_type == NALU_TYPE_SPS)
		|| (p_strm->nalu_type == NALU_TYPE_PPS)
		|| (p_strm->nalu_type == NALU_TYPE_SUB_SPS)
		|| (p_strm->nalu_type == NALU_TYPE_SEI)
		|| (p_strm->nalu_type == NALU_TYPE_PREFIX)
		|| (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) {
		add_size = MPP_MAX(HEAD_BUF_ADD_SIZE, p_strm->nalu_len);
        if ((p_strm->head_offset + add_size) >= p_strm->head_max_size) {     
            FUN_CHECK(ret = realloc_buffer(&p_strm->head_buf, &p_strm->head_max_size, add_size));
        }
        p_des = &p_strm->head_buf[p_strm->head_offset];
        add_size = MPP_MIN(HEAD_MAX_SIZE, p_strm->nalu_len);
        ((H264dNaluHead_t *)p_des)->is_frame_end  = 0;
        ((H264dNaluHead_t *)p_des)->nalu_type = p_strm->nalu_type;
        ((H264dNaluHead_t *)p_des)->sodb_len = add_size;
		//mpp_log("memcpy begin,add_size=%d,offset=%d,max_size=%d,nalu_len=%d \n",
		//	add_size, p_strm->head_offset,p_strm->head_max_size, p_strm->nalu_len);
        memcpy(p_des + sizeof(H264dNaluHead_t), p_strm->nalu_buf, add_size);
		//mpp_log("memcpy over\n");
        p_strm->head_offset += add_size + sizeof(H264dNaluHead_t);
    }
	//mpp_log("fill head buffer over \n");
    //!< fill sodb buffer
    if ((p_strm->nalu_type == NALU_TYPE_SLICE)
        || (p_strm->nalu_type == NALU_TYPE_IDR)) {
		 add_size = MPP_MAX(SODB_BUF_ADD_SIZE, p_strm->nalu_len);
        if ((dxva_ctx->strm_offset + add_size) >= dxva_ctx->max_strm_size) {      
            realloc_buffer(&dxva_ctx->bitstream, &dxva_ctx->max_strm_size, add_size);
			//mpp_log("relloc_buffer over \n");
        }
        p_des = &dxva_ctx->bitstream[dxva_ctx->strm_offset];
		//mpp_log("memcpy begin, dxva_ctx->max_strm_size=%d, p_strm->nalu_len=%d \n", dxva_ctx->max_strm_size, p_strm->nalu_len);
        memcpy(p_des, g_start_precode, sizeof(g_start_precode));
        memcpy(p_des + sizeof(g_start_precode), p_strm->nalu_buf, p_strm->nalu_len);
		//mpp_log("memcpy end \n");
        dxva_ctx->strm_offset += p_strm->nalu_len + sizeof(g_start_precode);
    }
	//mpp_log("store_cur_nalu function Out \n");
    return ret = MPP_OK;
__FAILED:
	//mpp_log("store_cur_nalu function ERROR \n");
    return ret;
}
#if 0
static void insert_timestamp(H264dCurCtx_t *p_Cur, H264dCurStream_t *p_strm)
{
	RK_U8 i = 0;
	H264dTimeStamp_t *p_curr = NULL;
	RK_U64 used_bytes = p_Cur->strm.pkt_used_bytes;



	for (i = 0; i < MAX_PTS_NUM; i++) {
		p_curr = &p_strm->pkt_ts[i];
		if ( (used_bytes > p_curr->begin_off) 
			&& (used_bytes < p_curr->end_off) ) {
				p_Cur->curr_dts = p_curr->dts;
				p_Cur->curr_pts = p_curr->pts;
				mpp_err("[TimePkt] insert_pts=%lld, pkt_pts=%lld \n", p_curr->pts, p_Cur->p_Inp->in_pts);
				break;
		}
	}

	//p_Cur->dts = p_Cur->p_Inp->in_dts;
	//p_Cur->pts = p_Cur->p_Inp->in_pts;
}
#endif
static MPP_RET judge_is_new_frame(H264dCurCtx_t *p_Cur, H264dCurStream_t *p_strm)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	RK_U32 nalu_header_bytes  = 0;
	RK_U32 first_mb_in_slice  = 0;
	H264dLogCtx_t   *logctx   = &p_Cur->p_Dec->logctx;
	BitReadCtx_t    *p_bitctx = &p_Cur->bitctx;
	RK_U32      forbidden_bit = -1;
	RK_U32  nal_reference_idc = -1;
	RK_U32 svc_extension_flag = -1;

	FunctionIn(logctx->parr[RUN_PARSE]);
	memset(p_bitctx, 0, sizeof(BitReadCtx_t));
	mpp_set_bitread_ctx(p_bitctx, p_strm->nalu_buf, 4);
	set_bitread_logctx(p_bitctx, logctx->parr[LOG_READ_NALU]);
	READ_BITS(p_bitctx, 1, &forbidden_bit);
	ASSERT(forbidden_bit == 0);
	READ_BITS(p_bitctx, 2, &nal_reference_idc);
	READ_BITS(p_bitctx, 5, &p_strm->nalu_type);
	//if (g_nalu_cnt1 == 29) {
	//    g_nalu_cnt1 = g_nalu_cnt1;
	//}
	//FPRINT(g_debug_file0, "[Analyze_NALU] g_framecnt=%d, g_nalu_cnt=%d, nal_type=%d, sodb_len=%d \n", p_Cur->p_Vid->g_framecnt, g_nalu_cnt1++, p_strm->nalu_type, p_strm->nalu_len);

	nalu_header_bytes = 1;
	if ((p_strm->nalu_type == NALU_TYPE_PREFIX) || (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) {
		READ_BITS(p_bitctx, 1, &svc_extension_flag);
		if (svc_extension_flag) {
			LogInfo(logctx->parr[RUN_PARSE], "svc_extension is not supported.");
			goto __FAILED;
		} else {
			if (p_strm->nalu_type == NALU_TYPE_SLC_EXT) {
				p_strm->nalu_type = NALU_TYPE_SLICE;
			}
		}
		nalu_header_bytes += 3;
	}
	//-- parse slice
	if ( p_strm->nalu_type == NALU_TYPE_SLICE || p_strm->nalu_type == NALU_TYPE_IDR) {
		mpp_set_bitread_ctx(p_bitctx, (p_strm->nalu_buf + nalu_header_bytes), 4); // reset
		READ_UE(p_bitctx, &first_mb_in_slice);
		//!< get time stamp
		if (first_mb_in_slice == 0)	{
			p_Cur->last_dts = p_Cur->curr_dts;
			p_Cur->last_pts = p_Cur->curr_pts;
			//insert_timestamp(p_Cur, p_strm);
			//mpp_log("[init_pts]p_Cur->last_pts=%lld, p_Cur->cur_pts=%lld, inp_pts=%lld, g_framecnt=%d \n",
				//p_Cur->last_pts, p_Cur->curr_pts, p_Cur->p_Inp->in_pts, p_Cur->p_Vid->g_framecnt);
			p_Cur->curr_dts = p_Cur->p_Inp->in_dts;
			p_Cur->curr_pts = p_Cur->p_Inp->in_pts;

			if (!p_Cur->p_Dec->is_first_frame) {
				p_Cur->p_Dec->is_new_frame = 1;				

			}
			p_Cur->p_Dec->is_first_frame = 0;

		}
	}
	FunctionOut(logctx->parr[RUN_PARSE]);
	return ret = MPP_OK;
__BITREAD_ERR:
	return ret = p_bitctx->ret;
__FAILED:
	return ret;
}

/*!
***********************************************************************
* \brief
*    prepare function for parser
***********************************************************************
*/
MPP_RET parse_prepare(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	H264dLogCtx_t   *logctx  = &p_Inp->p_Dec->logctx;
	H264_DecCtx_t   *p_Dec   = p_Inp->p_Dec;
	H264dCurStream_t *p_strm = &p_Cur->strm;
	RK_U32 nalu_header_bytes = 0;

	FunctionIn(logctx->parr[RUN_PARSE]);
	p_Dec->nalu_ret = NALU_NULL;
	p_Inp->task_valid = 0;

	//!< check eos
	if (p_Inp->pkt_eos) {
		FUN_CHECK(ret = store_cur_nalu(&p_Cur->strm, p_Dec->dxva_ctx));
		FUN_CHECK(ret = add_empty_nalu(p_strm));
		p_Dec->p_Inp->task_valid = 1;
		p_Dec->p_Inp->task_eos = 1;
		LogInfo(p_Inp->p_Dec->logctx.parr[RUN_PARSE], "----- end of stream ----");
		//mpp_log("----- eos: end of stream ----\n");
		goto __RETURN;
	}
	//!< check input
	if (!p_Inp->in_length) {
		p_Dec->nalu_ret = HaveNoStream;
		//mpp_log("----- input have no stream ----\n");
		goto __RETURN;
	}
	while (p_Inp->in_length > 0) {
		p_strm->curdata = &p_Inp->in_buf[p_strm->nalu_offset++];
		(*p_Inp->in_size) -= 1;
		p_Inp->in_length--;
		p_strm->pkt_used_bytes++;
		if (p_strm->startcode_found) {
			if (p_strm->nalu_len >= p_strm->nalu_max_size) {
				FUN_CHECK(ret = realloc_buffer(&p_strm->nalu_buf, &p_strm->nalu_max_size, NALU_BUF_ADD_SIZE));
			}
			p_strm->nalu_buf[p_strm->nalu_len++] = *p_strm->curdata;
			if (p_strm->nalu_len == 1) {
				p_strm->nalu_type = p_strm->nalu_buf[0]&0x1F;
				nalu_header_bytes += 1; 
				if ((p_strm->nalu_type == NALU_TYPE_PREFIX) 
					|| (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) {
					nalu_header_bytes += 3;
				}
			}
			if (p_strm->nalu_len == (nalu_header_bytes + 4)) {
				FUN_CHECK(ret = judge_is_new_frame(p_Cur, p_strm));
				if (p_Cur->p_Dec->is_new_frame) {
					//!< add an empty nalu to tell frame end
					FUN_CHECK(ret = add_empty_nalu(&p_Cur->strm));
					//!< reset curstream parameters
					p_Cur->strm.head_offset = 0;
					p_Cur->p_Inp->task_valid = 1;				
					p_Cur->p_Dec->is_new_frame = 0;
					break;
				}
			}
		}
		find_prefix_code(p_strm->curdata, p_strm);

		if (p_strm->endcode_found) {
			p_strm->nalu_len -= START_PREFIX_3BYTE;
			//!< find non-zeros byte
			while (p_strm->nalu_buf[p_strm->nalu_len - 1] == 0x00) { 
				p_strm->nalu_len--;
				break;
			}			
			p_Dec->nalu_ret = EndOfNalu;
			FUN_CHECK(ret = store_cur_nalu(&p_Dec->p_Cur->strm, p_Dec->dxva_ctx));
			reset_nalu(p_strm);
			break;
		}
	}
	//!< check input
	if (!p_Inp->in_length) { 
		p_strm->nalu_offset = 0;
		p_Dec->nalu_ret = HaveNoStream;
	}

__RETURN:
	FunctionOut(logctx->parr[RUN_PARSE]);

	return ret = MPP_OK;
__FAILED:
	return ret;
}

/*!
***********************************************************************
* \brief
*    main function for parser extra header
***********************************************************************
*/
MPP_RET parse_prepare_extra_header(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur)
{
	RK_S32 i = 0;
	MPP_RET ret = MPP_ERR_UNKNOW;

	H264dLogCtx_t *logctx  = &p_Inp->p_Dec->logctx;
	H264dCurStream_t *p_strm = &p_Cur->strm;
	RK_U8 *pdata = p_Inp->in_buf;
	RK_U64 extrasize = p_Inp->in_length;

	FunctionIn(logctx->parr[RUN_PARSE]);	
	//!< free nalu_buffer
	MPP_FREE(p_strm->nalu_buf);
	if (p_Inp->in_length < 7) {
		H264D_ERR("avcC too short, len=%d \n", p_Inp->in_length);
		goto __FAILED;
	}
	if(pdata[0] != 1){
		goto __FAILED;
	}
	p_Inp->profile = pdata[1];
	p_Inp->level = pdata[3];
	p_Inp->nal_size = 1 + (pdata[4] & 3);
	p_Inp->sps_num = pdata[5] & 31;

	pdata += 6;
	extrasize -= 6;
	for (i=0; i < p_Inp->sps_num; ++i) {			
		p_strm->nalu_len = U16_AT(pdata);
		pdata += 2;
		extrasize -= 2;
		p_strm->nalu_type = NALU_TYPE_SPS;
		p_strm->nalu_buf = pdata;
		FUN_CHECK(ret = store_cur_nalu(p_strm, p_Cur->p_Dec->dxva_ctx));
		//mpp_log("[SPS] i=%d, nalu_len=%d, %02x, %02x, %02x, %02x, %02x, %02x \n", i, p_strm->nalu_len,
			//pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5]);
		pdata += p_strm->nalu_len;
		extrasize -= p_strm->nalu_len;
	}
	p_strm->nalu_buf = NULL;
	p_Inp->pps_num = *pdata;
	++pdata;
	--extrasize;
	for (i = 0; i < p_Inp->pps_num; ++i) {
		p_strm->nalu_len = U16_AT(pdata);
		pdata += 2;
		extrasize -= 2;
		p_strm->nalu_type = NALU_TYPE_PPS;
		p_strm->nalu_buf = pdata;
		FUN_CHECK(ret = store_cur_nalu(p_strm, p_Cur->p_Dec->dxva_ctx));
		//mpp_log("[PPS] i=%d, nalu_len=%d, %02x, %02x, %02x, %02x, %02x, %02x \n", i, p_strm->nalu_len,
		//	pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5]);
		pdata += p_strm->nalu_len;
		extrasize -= p_strm->nalu_len;
	}	
	(*p_Inp->in_size) = 0;
	p_strm->nalu_buf = NULL;
	p_strm->startcode_found = 1;
	//mpp_log("profile=%d, level=%d, nal_size=%d, sps_num=%d, pps_num=%d \n", p_Inp->profile, 
	//	p_Inp->level, p_Inp->nal_size, p_Inp->sps_num, p_Inp->pps_num);

//__RETURN:
	FunctionOut(logctx->parr[RUN_PARSE]);

	return ret = MPP_OK;
__FAILED:
	return ret;
}
/*!
***********************************************************************
* \brief
*    main function for parser extra data
***********************************************************************
*/
MPP_RET parse_prepare_extra_data(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur)
{	
	MPP_RET ret = MPP_ERR_UNKNOW;
	H264dLogCtx_t   *logctx  = &p_Inp->p_Dec->logctx;
	H264dCurStream_t *p_strm = &p_Cur->strm;

	FunctionIn(logctx->parr[RUN_PARSE]);
	p_Inp->task_valid = 0;
	if (p_Inp->pkt_eos) {
		LogInfo(logctx->parr[RUN_PARSE], "----- end of stream ----");
		//mpp_log("----- eos: end of stream ----\n");
		p_Inp->task_eos = 1;
		p_Inp->task_valid = 0;
		//goto __EOS_RETRUN;
		return h264d_flush((void *)p_Inp->p_Dec);
	}
	VAL_CHECK(ret, (p_Inp->nal_size > 0));
	//H264D_LOG("in_length=%d, %02x, %02x, %02x, %02x, %02x, %02x \n", p_Inp->in_length,
	//	pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5]);
	H264D_LOG("[extra_data] before: p_Inp_length=%d \n", p_Inp->in_length);
	
	p_strm->curdata = &p_Inp->in_buf[p_strm->nalu_offset];
	while(p_Inp->in_length > 0) {
		if (p_strm->startcode_found) {
			//RK_U8 *pdata = NULL;
			RK_U32 nalu_header_bytes = 0;

			p_strm->nalu_len = parse_nal_size(p_Inp->nal_size, p_strm->curdata);
			VAL_CHECK(ret, (p_strm->nalu_len > 0));
			H264D_LOG("[extra_data] nalu_len=%d, in_length=%d \n", p_strm->nalu_len, p_Inp->in_length);
			VAL_CHECK(ret, (RK_U32)(p_strm->nalu_len < p_Inp->in_length));

			p_strm->curdata += p_Inp->nal_size;
			p_strm->nalu_offset += p_Inp->nal_size;
			(*p_Inp->in_size) -= p_Inp->nal_size;
			p_Inp->in_length -= p_Inp->nal_size;

			p_strm->nalu_buf = p_strm->curdata;
			p_strm->nalu_type = p_strm->nalu_buf[0]&0x1F;
			nalu_header_bytes = ((p_strm->nalu_type == NALU_TYPE_PREFIX) || (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) ? 4 :1; 
			//mpp_log("[extra_data] after: nalu_type=%d, nalu_len=%d, in_len=%d \n", p_strm->nalu_type, p_strm->nalu_len, mpp_packet_get_length(p_Inp->in_pkt));
			//pdata = p_strm->nalu_buf;
			//mpp_log("[extra_data] [NALU_DATA] nalu_len=%d, max_len=%d, %02x, %02x, %02x, %02x, %02x, %02x \n", p_strm->nalu_len,p_strm->nalu_max_size, 
			//	pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5]);
			p_strm->startcode_found = 0;
			p_strm->endcode_found = 1;
			if (p_strm->nalu_len >= (nalu_header_bytes + 4)) {
				FUN_CHECK(ret = judge_is_new_frame(p_Cur, p_strm));
				H264D_LOG("[extra_data] nalu_type=%d, is_new_frame=%d \n", p_strm->nalu_type, p_Cur->p_Dec->is_new_frame);
				if (p_Cur->p_Dec->is_new_frame) {					
					break;
				}
			}
		}
		if (p_strm->endcode_found) {
			FUN_CHECK(ret = store_cur_nalu(p_strm, p_Cur->p_Dec->dxva_ctx));
			p_strm->curdata += p_strm->nalu_len;
			p_strm->nalu_offset += p_strm->nalu_len;
			(*p_Inp->in_size) -= p_strm->nalu_len;
			p_Inp->in_length -= p_strm->nalu_len;
			p_strm->startcode_found = 1;
			p_strm->endcode_found = 0;
			p_strm->nalu_len = 0;
		}
		if (p_Inp->in_length < p_Inp->nal_size) {
			p_Cur->p_Dec->is_new_frame = 1;
			p_Cur->p_Dec->is_first_frame = 1;
			(*p_Inp->in_size) = 0;
			p_Inp->in_length = 0;
			p_strm->nalu_len = 0;
			p_strm->nalu_offset = 0;
			p_strm->startcode_found = 1;
			p_strm->endcode_found = 0;
			p_strm->nalu_buf  = NULL;	
			break;
			H264D_LOG("[extra_data] clear pkt +++++++++++ \n");
		}
	}
	H264D_LOG("[extra_data] after: +++++++++++, in_size=%d, nal_size=%d \n", (*p_Inp->in_size), p_Inp->nal_size);

//__EOS_RETRUN:	
	//!< one frame end
	if (p_Cur->p_Dec->is_new_frame) {
		//!< add an empty nalu to tell frame end
		FUN_CHECK(ret = add_empty_nalu(&p_Cur->strm));
		//!< reset curstream parameters
		p_Cur->strm.head_offset = 0;
		p_Cur->p_Inp->task_valid = 1;				
		p_Cur->p_Dec->is_new_frame = 0;
	}
//__RETURN:
	FunctionOut(logctx->parr[RUN_PARSE]);
	ret = MPP_OK;	
__FAILED:
	//p_strm->nalu_len = 0;
	p_Cur->curr_dts  = p_Inp->in_dts;
	p_Cur->curr_pts  = p_Inp->in_pts;
	p_Cur->last_dts  = p_Cur->curr_dts;
	p_Cur->last_pts  = p_Cur->curr_pts;

	//p_strm->nalu_buf  = NULL;	
	//p_strm->head_offset = 0;
	//p_strm->nalu_offset = 0;
	p_Inp->p_Dec->nalu_ret = HaveNoStream;	

	return ret;
}



/*!
***********************************************************************
* \brief
*    main function for parser
***********************************************************************
*/
MPP_RET parse_loop(H264_DecCtx_t *p_Dec)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8 *p_curdata = NULL;
    RK_U8 while_loop_flag = 1;
    H264dNaluHead_t *p_head = NULL;
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    //!< ==== loop ====
    p_curdata = p_Dec->p_Cur->strm.head_buf;

    while (while_loop_flag) {
        switch (p_Dec->next_state) {
        case SliceSTATE_ResetSlice:
            reset_slice(p_Dec->p_Vid);
            p_Dec->next_state = SliceSTATE_ReadNalu;
			//H264D_LOG( "SliceSTATE_ResetSlice");
            break;
        case SliceSTATE_ReadNalu:
            p_head = (H264dNaluHead_t *)p_curdata;
            if (p_head->is_frame_end) {
                p_Dec->next_state = SliceSTATE_RegisterOneFrame;
                p_Dec->nalu_ret = HaveNoStream;
            } else {
                p_curdata += sizeof(H264dNaluHead_t);
                memset(&p_Dec->p_Cur->nalu, 0, sizeof(H264_Nalu_t));
                p_Dec->p_Cur->nalu.sodb_buf = p_curdata;
                p_Dec->p_Cur->nalu.sodb_len = p_head->sodb_len;
                p_curdata += p_head->sodb_len;
                p_Dec->nalu_ret = EndOfNalu;
                p_Dec->next_state = SliceSTATE_ParseNalu;
            }
			//H264D_LOG( "SliceSTATE_ReadNalu");
            break;
        case SliceSTATE_ParseNalu:
            (ret = parser_one_nalu(&p_Dec->p_Cur->slice));
            if (p_Dec->nalu_ret == StartOfSlice) {
                p_Dec->next_state = SliceSTATE_GetSliceData;
            } else if (p_Dec->nalu_ret == StartOfPicture) {
                p_Dec->next_state = SliceSTATE_InitPicture;
            } else {
                p_Dec->next_state = SliceSTATE_ReadNalu;
            }
			//H264D_LOG("SliceSTATE_ParseNalu");
            break;
        case SliceSTATE_InitPicture:
            (ret = init_picture(&p_Dec->p_Cur->slice));
            p_Dec->next_state = SliceSTATE_GetSliceData;
			//H264D_LOG("SliceSTATE_InitPicture");
            break;
        case SliceSTATE_GetSliceData:
			(ret = fill_slice_syntax(&p_Dec->p_Cur->slice, p_Dec->dxva_ctx));
            p_Dec->p_Vid->iNumOfSlicesDecoded++;
            p_Dec->next_state = SliceSTATE_ResetSlice;
			//H264D_LOG("SliceSTATE_GetSliceData");
            break;
        case SliceSTATE_RegisterOneFrame:
            commit_buffer(p_Dec->dxva_ctx);
            while_loop_flag = 0;
            p_Dec->is_parser_end = 1;
            p_Dec->next_state = SliceSTATE_ReadNalu;
			//H264D_LOG("SliceSTATE_RegisterOneFrame");
            break;
        default:
            ret = MPP_NOK;
            goto __FAILED;
        }
    }
    FunctionOut(p_Dec->logctx.parr[RUN_PARSE]);
    //__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}


