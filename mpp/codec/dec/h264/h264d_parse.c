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

#define   MODULE_TAG "h264d_parse"

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

static MPP_RET realloc_curstrearm_buffer(H264dCurStream_t *p_strm)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    p_strm->buf = mpp_realloc(p_strm->buf, RK_U8, p_strm->max_size + NALU_BUF_ADD_SIZE);
    MEM_CHECK(ret, p_strm->buf);
    p_strm->max_size += NALU_BUF_ADD_SIZE;

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void reset_nalu(H264_Nalu_t*p_nal, H264dCurStream_t *p_strm)
{
    if (p_strm->endcode_found) {
        p_strm->startcode_found = p_strm->endcode_found;
        memset(p_nal, 0, sizeof(H264_Nalu_t));
        p_strm->endcode_found = 0;
    }
    p_nal->sodb_buf = p_strm->buf;
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

static MPP_RET read_nalu(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8 *p_curdata = NULL;
    H264dLogCtx_t   *logctx = currSlice->logctx;
    H264_DecCtx_t   *p_Dec  = currSlice->p_Dec;
    H264dCurCtx_t   *p_Cur  = currSlice->p_Cur;
    H264dInputCtx_t *p_Inp  = currSlice->p_Inp;

    FunctionIn(logctx->parr[RUN_PARSE]);
    reset_nalu(&p_Cur->nalu, &p_Cur->strm);
    while ((*p_Inp->in_size) > 0) {
        p_curdata = &p_Inp->in_buf[p_Cur->strm.offset++];
        (*p_Inp->in_size) -= 1;

        if (p_Cur->strm.startcode_found) {
            if (p_Cur->nalu.sodb_len >= p_Cur->strm.max_size) {
                FUN_CHECK(ret = realloc_curstrearm_buffer(&p_Cur->strm));
                p_Cur->nalu.sodb_buf = p_Cur->strm.buf;
            }
            p_Cur->nalu.sodb_buf[p_Cur->nalu.sodb_len++] = *p_curdata;
        }
        find_prefix_code(p_curdata, &p_Cur->strm);
        if (p_Cur->strm.endcode_found) {
            p_Cur->nalu.sodb_len -= START_PREFIX_3BYTE;
            while (p_Cur->nalu.sodb_buf[p_Cur->nalu.sodb_len - 1] == 0x00) { //!< find non-zeros byte
                p_Cur->nalu.sodb_len--;
            }
            p_Dec->nalu_ret = EndOfNalu;
            break;
        }
    }
    if (!(*p_Inp->in_size)) { //!< check input
        p_Cur->strm.offset = 0;
        p_Cur->strm.endcode_found = (p_Inp->is_eos && p_Cur->nalu.sodb_len) ? 1 : p_Cur->strm.endcode_found;
        p_Dec->nalu_ret = p_Inp->is_eos ? (p_Cur->nalu.sodb_len ? EndOfNalu : EndofStream) : HaveNoStream;
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
    set_bitread_ctx(p_bitctx, cur_nal->sodb_buf, cur_nal->sodb_len);
    set_bitread_logctx(p_bitctx, logctx->parr[LOG_READ_NALU]);
    WRITE_LOG(p_bitctx, "================== NAL begin ===================");
    READ_BITS(ret, p_bitctx, 1, &cur_nal->forbidden_bit, "forbidden_bit");
    ASSERT(cur_nal->forbidden_bit == 0);
    READ_BITS(ret, p_bitctx, 2, (RK_S32 *)&cur_nal->nal_reference_idc, "nal_ref_idc");
    READ_BITS(ret, p_bitctx, 5, (RK_S32 *)&cur_nal->nal_unit_type, "nal_unit_type");
    if (g_nalu_cnt == 321) {
        g_nalu_cnt = g_nalu_cnt;
    }
    cur_nal->ualu_header_bytes = 1;
    currSlice->svc_extension_flag = -1; //!< initialize to -1
    if ((cur_nal->nal_unit_type == NALU_TYPE_PREFIX) || (cur_nal->nal_unit_type == NALU_TYPE_SLC_EXT)) {
        READ_ONEBIT(ret, p_bitctx, &currSlice->svc_extension_flag, "svc_extension_flag");
        if (currSlice->svc_extension_flag) {
            FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
            currSlice->mvcExt.valid = 0;
            LogInfo(logctx->parr[RUN_PARSE], "svc_extension is not supported.");
            goto __FAILED;
        } else { //!< MVC
            currSlice->mvcExt.valid = 1;
            READ_ONEBIT(ret, p_bitctx,     &currSlice->mvcExt.non_idr_flag,     "nalu_type");
            READ_BITS(ret, p_bitctx,    6, &currSlice->mvcExt.priority_id,      "priority_id");
            READ_BITS(ret, p_bitctx,   10, &currSlice->mvcExt.view_id,          "view_id");
            READ_BITS(ret, p_bitctx,    3, &currSlice->mvcExt.temporal_id,      "temporal_id");
            READ_ONEBIT(ret, p_bitctx,     &currSlice->mvcExt.anchor_pic_flag,  "anchor_pic_flag");
            READ_ONEBIT(ret, p_bitctx,     &currSlice->mvcExt.inter_view_flag,  "inter_view_flag");
            READ_ONEBIT(ret, p_bitctx,     &currSlice->mvcExt.reserved_one_bit, "reserved_one_bit");
            ASSERT(currSlice->mvcExt.reserved_one_bit == 1);
            currSlice->mvcExt.iPrefixNALU = (cur_nal->nal_unit_type == NALU_TYPE_PREFIX) ? 1 : 0;
            if (cur_nal->nal_unit_type == NALU_TYPE_SLC_EXT) { //!< combine NALU_TYPE_SLC_EXT into NALU_TYPE_SLICE
                cur_nal->nal_unit_type = NALU_TYPE_SLICE;
            }
            FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
        }
        cur_nal->ualu_header_bytes += 3;
    } else {
        FPRINT(logctx->parr[LOG_READ_NALU]->fp, "g_nalu_cnt=%d, nalu_type=%d, len=%d \n", g_nalu_cnt++, cur_nal->nal_unit_type, cur_nal->sodb_len);
    }
    set_bitread_ctx(p_bitctx, (cur_nal->sodb_buf + cur_nal->ualu_header_bytes), (cur_nal->sodb_len - cur_nal->ualu_header_bytes)); // reset

    FunctionOut(logctx->parr[RUN_PARSE]);

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET parser_nalu(H264_SLICE_t *currSlice)
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
        if (currSlice->is_new_picture_flag) {
            currSlice->p_Dec->nalu_ret = StartOfPicture;
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

/*!
***********************************************************************
* \brief
*    loop function for parser
***********************************************************************
*/
MPP_RET parse_loop(H264_DecCtx_t *p_Dec)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 while_loop_flag = 1;


    FunctionIn(p_Dec->logctx.parr[RUN_PARSE]);
    //!< ====  parse loop ====
    while (while_loop_flag) {
        switch (p_Dec->next_state) {
        case SliceSTATE_ResetSlice:
            reset_slice(p_Dec->p_Vid);
            p_Dec->next_state = SliceSTATE_ReadNalu;
            break;
        case SliceSTATE_ReadNalu:
            (ret = read_nalu(&p_Dec->p_Cur->slice));
            if (p_Dec->nalu_ret == EndOfNalu) {
                p_Dec->next_state = SliceSTATE_ParseNalu;
            } else if (p_Dec->nalu_ret == EndofStream) {
                p_Dec->next_state = SliceSTATE_RegisterOneFrame;
            } else if (p_Dec->nalu_ret == HaveNoStream) {
                while_loop_flag = 0;
            }
            break;
        case SliceSTATE_ParseNalu:
            (ret = parser_nalu(&p_Dec->p_Cur->slice));
            if (p_Dec->nalu_ret == StartOfSlice) {
                p_Dec->next_state = SliceSTATE_GetSliceData;
            } else if (p_Dec->nalu_ret == StartOfPicture) {
                if (p_Dec->first_frame_flag) {
                    p_Dec->next_state = SliceSTATE_InitPicture;
                    p_Dec->first_frame_flag = 0;
                } else {
                    p_Dec->next_state = SliceSTATE_RegisterOneFrame;
                }
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
            p_Dec->parser_end_flag = 1;
            p_Dec->next_state = SliceSTATE_InitPicture;
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


