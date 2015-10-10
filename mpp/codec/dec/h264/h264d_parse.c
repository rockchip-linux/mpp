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
    RK_U16  nal_unit_type;
    RK_U32  sodb_len;
} H264dNaluHead_t;

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
    *buf = mpp_realloc(*buf, RK_U8, *max_size + add_size);
    MEM_CHECK(ret, *buf);
    *max_size += add_size;

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void reset_nalu(H264dCurStream_t *p_strm)
{
    if (p_strm->endcode_found) {
        p_strm->startcode_found = p_strm->endcode_found;
        p_strm->nalu_len = 0;
        p_strm->nal_unit_type = NALU_TYPE_NULL;
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
    }
    if (!p_Inp->in_length) { //!< check input
        p_strm->nalu_offset = 0;
        p_Dec->nalu_ret = HaveNoStream;
    }

    FunctionIn(logctx->parr[RUN_PARSE]);

    return ret = MPP_OK;
__FAILED:

    return ret;
}


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
    READ_BITS(p_bitctx, 5, (RK_S32 *)&cur_nal->nal_unit_type, "nal_unit_type");
    if (g_nalu_cnt0 == 2384) {
        g_nalu_cnt0 = g_nalu_cnt0;
    }
    //if (g_debug_file1 == NULL)
    //{
    //  g_debug_file1 = fopen("rk_debugfile_view1.txt", "wb");
    //}
    //FPRINT(g_debug_file1, "g_nalu_cnt = %d, nal_unit_type = %d, nalu_size = %d\n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
    /*  if((cur_nal->nal_unit_type == NALU_TYPE_SLICE)
    || (cur_nal->nal_unit_type == NALU_TYPE_IDR)
    || (cur_nal->nal_unit_type == NALU_TYPE_SPS)
    || (cur_nal->nal_unit_type == NALU_TYPE_PPS)
    || (cur_nal->nal_unit_type == NALU_TYPE_SUB_SPS)
    || (cur_nal->nal_unit_type == NALU_TYPE_SEI))*/{
        FPRINT(g_debug_file0, "g_nalu_cnt=%d, nal_type=%d, sodb_len=%d \n", g_nalu_cnt0++, cur_nal->nal_unit_type, cur_nal->sodb_len);
    }
    cur_nal->ualu_header_bytes = 1;
    currSlice->svc_extension_flag = -1; //!< initialize to -1
    if ((cur_nal->nal_unit_type == NALU_TYPE_PREFIX) || (cur_nal->nal_unit_type == NALU_TYPE_SLC_EXT)) {
        READ_ONEBIT(p_bitctx, &currSlice->svc_extension_flag, "svc_extension_flag");
        if (currSlice->svc_extension_flag) {
            //FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
            currSlice->mvcExt.valid = 0;
            LogInfo(logctx->parr[RUN_PARSE], "svc_extension is not supported.");
            goto __FAILED;
        } else { //!< MVC
            currSlice->mvcExt.valid = 1;
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.non_idr_flag,     "nalu_type");
            READ_BITS(p_bitctx,    6, &currSlice->mvcExt.priority_id,      "priority_id");
            READ_BITS(p_bitctx,   10, &currSlice->mvcExt.view_id,          "view_id");
            READ_BITS(p_bitctx,    3, &currSlice->mvcExt.temporal_id,      "temporal_id");
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.anchor_pic_flag,  "anchor_pic_flag");
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.inter_view_flag,  "inter_view_flag");
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.reserved_one_bit, "reserved_one_bit");
            ASSERT(currSlice->mvcExt.reserved_one_bit == 1);
            currSlice->mvcExt.iPrefixNALU = (cur_nal->nal_unit_type == NALU_TYPE_PREFIX) ? 1 : 0;
            //!< combine NALU_TYPE_SLC_EXT into NALU_TYPE_SLICE
            if (cur_nal->nal_unit_type == NALU_TYPE_SLC_EXT) {
                cur_nal->nal_unit_type = NALU_TYPE_SLICE;
            }
            //FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
        }
        cur_nal->ualu_header_bytes += 3;
    } else {
        //FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
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
    switch (currSlice->p_Cur->nalu.nal_unit_type) {
    case NALU_TYPE_SLICE:
    case NALU_TYPE_IDR:
        FUN_CHECK(ret = process_slice(currSlice));
        if (currSlice->is_new_picture) {
            currSlice->p_Dec->nalu_ret = StartOfPicture;
            FPRINT(g_debug_file0, "----- start of picture ---- \n");
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
        FUN_CHECK(ret = process_sei(currSlice));
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
    FunctionIn(runlog);

    return ret = MPP_OK;
__FAILED:

    return ret;
}

static MPP_RET find_next_frame(H264dCurCtx_t *p_Cur)
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
    mpp_set_bitread_ctx(p_bitctx, p_strm->nalu_buf, p_strm->nalu_len);
    set_bitread_logctx(p_bitctx, logctx->parr[LOG_READ_NALU]);
    READ_BITS(p_bitctx, 1, &forbidden_bit);
    ASSERT(forbidden_bit == 0);
    READ_BITS(p_bitctx, 2, &nal_reference_idc);
    READ_BITS(p_bitctx, 5, &p_strm->nal_unit_type);
    if (g_nalu_cnt1 == 29) {
        g_nalu_cnt1 = g_nalu_cnt1;
    }
    FPRINT(g_debug_file1, "g_nalu_cnt=%d, nal_type=%d, sodb_len=%d \n", g_nalu_cnt1++, p_strm->nal_unit_type, p_strm->nalu_len);

    nalu_header_bytes = 1;
    if ((p_strm->nal_unit_type == NALU_TYPE_PREFIX) || (p_strm->nal_unit_type == NALU_TYPE_SLC_EXT)) {
        READ_BITS(p_bitctx, 1, &svc_extension_flag);
        if (svc_extension_flag) {
            LogInfo(logctx->parr[RUN_PARSE], "svc_extension is not supported.");
            goto __FAILED;
        } else {
            if (p_strm->nal_unit_type == NALU_TYPE_SLC_EXT) {
                p_strm->nal_unit_type = NALU_TYPE_SLICE;
            }
        }
        nalu_header_bytes += 3;
    }
    //-- parse slice
    if ( p_strm->nal_unit_type == NALU_TYPE_SLICE || p_strm->nal_unit_type == NALU_TYPE_IDR) {
        mpp_set_bitread_ctx(p_bitctx, (p_strm->nalu_buf + nalu_header_bytes), 4); // reset
        READ_UE(p_bitctx, &first_mb_in_slice);

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

static MPP_RET add_empty_nalu(H264dCurCtx_t *p_Cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8  *p_des = NULL;
    H264dCurStream_t *p_strm = &p_Cur->strm;

    if (p_strm->head_offset >= p_strm->head_max_size) {

        FUN_CHECK(ret = realloc_buffer(&p_strm->head_buf, &p_strm->head_max_size, HEAD_BUF_ADD_SIZE));
    }
    p_des = &p_strm->head_buf[p_strm->head_offset];

    ((H264dNaluHead_t *)p_des)->is_frame_end  = 1;
    ((H264dNaluHead_t *)p_des)->nal_unit_type = 0;
    ((H264dNaluHead_t *)p_des)->sodb_len      = 0;
    p_strm->head_offset += sizeof(H264dNaluHead_t);

    return ret = MPP_OK;
__FAILED:
    return ret;
}


static MPP_RET store_cur_nalu(H264dCurCtx_t *p_Cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8 *p_des = NULL;
    RK_U32 add_size = 0;
    H264dDxvaCtx_t   *dxva_ctx = NULL;
    H264dCurStream_t *p_strm = &p_Cur->strm;

    FunctionIn(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);
    //!< fill head buffer
    if ((p_strm->nal_unit_type == NALU_TYPE_SLICE)
        || (p_strm->nal_unit_type == NALU_TYPE_IDR)
        || (p_strm->nal_unit_type == NALU_TYPE_SPS)
        || (p_strm->nal_unit_type == NALU_TYPE_PPS)
        || (p_strm->nal_unit_type == NALU_TYPE_SUB_SPS)
        || (p_strm->nal_unit_type == NALU_TYPE_SEI)
        || (p_strm->nal_unit_type == NALU_TYPE_PREFIX)
        || (p_strm->nal_unit_type == NALU_TYPE_SLC_EXT)) {
        if (p_strm->head_offset >= p_strm->head_max_size) {
            add_size = MPP_MAX(HEAD_BUF_ADD_SIZE, p_strm->nalu_len);
            FUN_CHECK(ret = realloc_buffer(&p_strm->head_buf, &p_strm->head_max_size, add_size));
        }
        p_des = &p_strm->head_buf[p_strm->head_offset];
        add_size = MPP_MIN(HEAD_MAX_SIZE, p_strm->nalu_len);
        ((H264dNaluHead_t *)p_des)->is_frame_end  = 0;
        ((H264dNaluHead_t *)p_des)->nal_unit_type = p_strm->nal_unit_type;
        ((H264dNaluHead_t *)p_des)->sodb_len      = add_size;
        memcpy(p_des + sizeof(H264dNaluHead_t), p_strm->nalu_buf, add_size);
        p_strm->head_offset += add_size + sizeof(H264dNaluHead_t);
    }
    //!< fill sodb buffer
    if ((p_strm->nal_unit_type == NALU_TYPE_SLICE)
        || (p_strm->nal_unit_type == NALU_TYPE_IDR)) {
        dxva_ctx = p_Cur->p_Dec->dxva_ctx;
        if (dxva_ctx->strm_offset >= dxva_ctx->max_strm_size) {
            add_size = MPP_MAX(SODB_BUF_ADD_SIZE, p_strm->nalu_len);
            realloc_buffer(&dxva_ctx->bitstream, &dxva_ctx->max_strm_size, add_size);
        }
        p_des = &dxva_ctx->bitstream[dxva_ctx->strm_offset];
        memcpy(p_des, g_start_precode, sizeof(g_start_precode));
        memcpy(p_des + sizeof(g_start_precode), p_strm->nalu_buf, p_strm->nalu_len);
        dxva_ctx->strm_offset += p_strm->nalu_len + sizeof(g_start_precode);
    }
    FunctionIn(p_Cur->p_Dec->logctx.parr[RUN_PARSE]);

    return ret = MPP_OK;
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
    FunctionIn(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);

    p_Inp->task_valid = 0;
    if (p_Cur->p_Inp->is_eos) {
        FUN_CHECK(ret = find_next_frame(p_Cur));
        FUN_CHECK(ret = store_cur_nalu(p_Cur));
        FUN_CHECK(ret = add_empty_nalu(p_Cur));
        p_Inp->task_valid = 1;
        FPRINT(g_debug_file1, "----- end of stream ---- \n");
        goto __RETURN;
    } else if (p_Cur->p_Dec->is_new_frame) {
        FUN_CHECK(ret = store_cur_nalu(p_Cur));
        p_Cur->p_Dec->is_new_frame = 0;
    }

    FUN_CHECK(ret = read_one_nalu(p_Inp, &p_Cur->strm));

    if (p_Inp->p_Dec->nalu_ret == EndOfNalu) {
        FUN_CHECK(ret = find_next_frame(p_Cur));
        if (p_Cur->p_Dec->is_new_frame) {
            //!< add an empty nalu to tell frame end
            FUN_CHECK(ret = add_empty_nalu(p_Cur));
            //!< reset curstream parameters
            p_Cur->strm.head_offset = 0;
            p_Inp->task_valid = 1;
            FPRINT(g_debug_file1, "----- start of picture ---- \n");
        } else {
            FUN_CHECK(ret = store_cur_nalu(p_Cur));
        }
    }

__RETURN:
    FunctionOut(p_Inp->p_Dec->logctx.parr[RUN_PARSE]);
    return ret = MPP_OK;
__FAILED:
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
            break;
        case SliceSTATE_InitPicture:
            (ret = init_picture(&p_Dec->p_Cur->slice));
            p_Dec->next_state = SliceSTATE_GetSliceData;
            break;
        case SliceSTATE_GetSliceData:
            (ret = fill_slice(&p_Dec->p_Cur->slice, p_Dec->dxva_ctx));
            p_Dec->p_Vid->iNumOfSlicesDecoded++;
            p_Dec->next_state = SliceSTATE_ResetSlice;
            break;
        case SliceSTATE_RegisterOneFrame:
            commit_buffer(p_Dec->dxva_ctx);
            while_loop_flag = 0;
            p_Dec->is_parser_end = 1;
            p_Dec->next_state = SliceSTATE_ReadNalu;
            break;
        default:
            ret = MPP_NOK;
            goto __FAILED;
        }
    }
    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    //__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}


