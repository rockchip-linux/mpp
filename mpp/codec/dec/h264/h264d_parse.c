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
#include "mpp_packet_impl.h"
#include "hal_task.h"

#include "h264d_global.h"
#include "h264d_parse.h"
#include "h264d_slice.h"
#include "h264d_sps.h"
#include "h264d_pps.h"
#include "h264d_sei.h"
#include "h264d_init.h"
#include "h264d_fill.h"

#define  HEAD_SYNTAX_MAX_SIZE        (12800)
#define NALU_TYPE_NORMAL_LENGTH      (1)
#define NALU_TYPE_EXT_LENGTH         (5)

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

    memset(currSlice, 0, sizeof(H264_SLICE_t));
    //-- re-init parameters
    currSlice->view_id = -1;
    currSlice->p_Vid   = p_Vid;
    currSlice->p_Dec   = p_Vid->p_Dec;
    currSlice->p_Cur   = p_Vid->p_Cur;
    currSlice->p_Inp   = p_Vid->p_Inp;
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
    reset_cur_slice(p_Vid->p_Cur, currSlice);
}

static MPP_RET realloc_buffer(RK_U8 **buf, RK_U32 *max_size, RK_U32 add_size)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    if ((*buf) == NULL) {
        H264D_ERR("[realloc_buffer] pointer is null, %p \n", (*buf));
        ret = MPP_ERR_MALLOC;
        goto __FAILED;
    }

    add_size = MPP_ALIGN(add_size, 16);

    (*buf) = mpp_realloc((*buf), RK_U8, ((*max_size) + add_size));
    if ((*buf) == NULL) {
        H264D_ERR("[realloc_buffer] ERROR: max_size=%d, add_size=%d \n", (*max_size), add_size);
        ret = MPP_ERR_MALLOC;
        goto __FAILED;
    }
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
    (void)p_data;

    if ((p_strm->prefixdata & 0x00FFFFFF) == 0x00000001) {
        if (p_strm->startcode_found) {
            p_strm->endcode_found = 1;
        } else {
            p_strm->startcode_found = 1;
        }
    }
}

static MPP_RET parser_nalu_header(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dCurCtx_t   *p_Cur    = currSlice->p_Cur;
    BitReadCtx_t    *p_bitctx = &p_Cur->bitctx;
    H264_Nalu_t     *cur_nal  = &p_Cur->nalu;

    mpp_set_bitread_ctx(p_bitctx, cur_nal->sodb_buf, cur_nal->sodb_len);
    mpp_set_pre_detection(p_bitctx);

    READ_BITS(p_bitctx, 1, &cur_nal->forbidden_bit);
    ASSERT(cur_nal->forbidden_bit == 0);
    {
        RK_S32  *ptmp = NULL;
        ptmp = (RK_S32 *)&cur_nal->nal_reference_idc;
        READ_BITS(p_bitctx, 2, ptmp); //!<  nal_ref_idc
        ptmp = (RK_S32 *)&cur_nal->nalu_type;
        READ_BITS(p_bitctx, 5, ptmp); //!< nalu_type
    }
    cur_nal->ualu_header_bytes = 1;
    currSlice->svc_extension_flag = -1; //!< initialize to -1
    if (cur_nal->nalu_type == NALU_TYPE_PREFIX
        || cur_nal->nalu_type == NALU_TYPE_SLC_EXT) {
        READ_ONEBIT(p_bitctx, &currSlice->svc_extension_flag);
        if (currSlice->svc_extension_flag) {
            currSlice->mvcExt.valid = 0;
            //H264D_WARNNING("svc_extension is not supported.");
            //goto __FAILED;
        } else { //!< MVC
            currSlice->mvcExt.valid = 1;
            p_Cur->p_Dec->mvc_valid = 1;
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.non_idr_flag);
            READ_BITS(p_bitctx,    6, &currSlice->mvcExt.priority_id);
            READ_BITS(p_bitctx,   10, &currSlice->mvcExt.view_id);
            READ_BITS(p_bitctx,    3, &currSlice->mvcExt.temporal_id);
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.anchor_pic_flag);
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.inter_view_flag);
            READ_ONEBIT(p_bitctx,     &currSlice->mvcExt.reserved_one_bit);
            ASSERT(currSlice->mvcExt.reserved_one_bit == 1);
            currSlice->mvcExt.iPrefixNALU = (cur_nal->nalu_type == NALU_TYPE_PREFIX) ? 1 : 0;
            //!< combine NALU_TYPE_SLC_EXT into NALU_TYPE_SLICE
            if (cur_nal->nalu_type == NALU_TYPE_SLC_EXT) {
                cur_nal->nalu_type = NALU_TYPE_SLICE;
            }
        }
        cur_nal->ualu_header_bytes += 3;
    }
    /* Move forward the bitread offset */
    mpp_set_bitread_ctx(p_bitctx,
                        cur_nal->sodb_buf + cur_nal->ualu_header_bytes,
                        cur_nal->sodb_len - cur_nal->ualu_header_bytes);
    mpp_set_pre_detection(p_bitctx);
    p_Cur->p_Dec->nalu_ret = StartofNalu;

    return ret = MPP_OK;
__BITREAD_ERR:
    p_Cur->p_Dec->nalu_ret = ReadNaluError;
    return ret = p_bitctx->ret;
//__FAILED:
//    p_Cur->p_Dec->nalu_ret = StreamError;
//    return ret;
}

static MPP_RET parser_one_nalu(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    FUN_CHECK(ret = parser_nalu_header(currSlice));
    //!< nalu_parse
    switch (currSlice->p_Cur->nalu.nalu_type) {
    case NALU_TYPE_SLICE:
    case NALU_TYPE_IDR:
        H264D_DBG(H264D_DBG_PARSE_NALU, "nalu_type=SLICE.");
        FUN_CHECK(ret = process_slice(currSlice));
        currSlice->p_Dec->nalu_ret = StartOfPicture;
        if (currSlice->layer_id && currSlice->p_Inp->mvc_disable)
            currSlice->p_Dec->nalu_ret = MvcDisAble;
        break;
    case NALU_TYPE_SPS:
        H264D_DBG(H264D_DBG_PARSE_NALU, "nalu_type=SPS");
        FUN_CHECK(ret = process_sps(currSlice));
        currSlice->p_Dec->nalu_ret = NALU_SPS;
        break;
    case NALU_TYPE_PPS:
        H264D_DBG(H264D_DBG_PARSE_NALU, "nalu_type=PPS");
        FUN_CHECK(ret = process_pps(currSlice));
        currSlice->p_Dec->nalu_ret = NALU_PPS;
        break;
    case NALU_TYPE_SUB_SPS:
        H264D_DBG(H264D_DBG_PARSE_NALU, "nalu_type=SUB_SPS");
        FUN_CHECK(ret = process_subsps(currSlice));
        currSlice->p_Dec->nalu_ret = NALU_SubSPS;
        break;
    case NALU_TYPE_SEI:
        H264D_DBG(H264D_DBG_PARSE_NALU, "nalu_type=SEI");
        ret = process_sei(currSlice);
        currSlice->p_Dec->nalu_ret = NALU_SEI;
        break;
    case NALU_TYPE_SLC_EXT:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_SLC_EXT.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_PREFIX:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_PREFIX.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_AUD:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_AUD.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_EOSEQ:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_EOSEQ.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_EOSTREAM:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_EOSTREAM.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_FILL:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_FILL.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_VDRD:
        H264D_DBG(H264D_DBG_PARSE_NALU, "Found NALU_TYPE_VDRD.");
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    case NALU_TYPE_DPA:
    case NALU_TYPE_DPB:
    case NALU_TYPE_DPC:
        H264D_DBG(H264D_DBG_PARSE_NALU,
                  "Found NALU_TYPE_DPA DPB DPC, and not supported.");
        currSlice->p_Dec->nalu_ret = NaluNotSupport;
        break;
    default:
        currSlice->p_Dec->nalu_ret = SkipNALU;
        break;
    }

    return ret = MPP_OK;
__FAILED:
    H264D_DBG(H264D_DBG_PARSE_NALU, "parser_one_nalu error.");
    currSlice->p_Dec->nalu_ret = ReadNaluError;
    return ret;
}

static MPP_RET add_empty_nalu(H264dCurStream_t *p_strm)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8  *p_des = NULL;
    RK_U32 add_size = sizeof(H264dNaluHead_t);

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

static MPP_RET store_cur_nalu(H264dCurCtx_t *p_Cur, H264dCurStream_t *p_strm, H264dDxvaCtx_t *dxva_ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8 *p_des = NULL;

    //!< fill head buffer
    if (   (p_strm->nalu_type == NALU_TYPE_SLICE)
           || (p_strm->nalu_type == NALU_TYPE_IDR)
           || (p_strm->nalu_type == NALU_TYPE_SPS)
           || (p_strm->nalu_type == NALU_TYPE_PPS)
           || (p_strm->nalu_type == NALU_TYPE_SUB_SPS)
           || (p_strm->nalu_type == NALU_TYPE_SEI)
           || (p_strm->nalu_type == NALU_TYPE_PREFIX)
           || (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) {

        RK_U32 head_size = MPP_MIN(HEAD_SYNTAX_MAX_SIZE, p_strm->nalu_len);
        RK_U32 add_size = head_size + sizeof(H264dNaluHead_t);

        if ((p_strm->head_offset + add_size) >= p_strm->head_max_size) {
            FUN_CHECK(ret = realloc_buffer(&p_strm->head_buf, &p_strm->head_max_size, add_size));
        }
        p_des = &p_strm->head_buf[p_strm->head_offset];
        ((H264dNaluHead_t *)p_des)->is_frame_end  = 0;
        ((H264dNaluHead_t *)p_des)->nalu_type = p_strm->nalu_type;
        ((H264dNaluHead_t *)p_des)->sodb_len = head_size;
        memcpy(p_des + sizeof(H264dNaluHead_t), p_strm->nalu_buf, head_size);
        p_strm->head_offset += add_size;
    }    //!< fill sodb buffer
    if ((p_strm->nalu_type == NALU_TYPE_SLICE)
        || (p_strm->nalu_type == NALU_TYPE_IDR)) {

        RK_U32 add_size = p_strm->nalu_len + sizeof(g_start_precode);

        if ((dxva_ctx->strm_offset + add_size) >= dxva_ctx->max_strm_size) {
            FUN_CHECK(ret = realloc_buffer(&dxva_ctx->bitstream, &dxva_ctx->max_strm_size, add_size));
        }

        p_des = &dxva_ctx->bitstream[dxva_ctx->strm_offset];
        memcpy(p_des, g_start_precode, sizeof(g_start_precode));
        memcpy(p_des + sizeof(g_start_precode), p_strm->nalu_buf, p_strm->nalu_len);
        dxva_ctx->strm_offset += add_size;
    }
    if (rkv_h264d_parse_debug & H264D_DBG_WRITE_ES_EN) {
        H264dInputCtx_t *p_Inp = p_Cur->p_Inp;
        if ((p_strm->nalu_type == NALU_TYPE_SPS)
            || (p_strm->nalu_type == NALU_TYPE_PPS)) {
            if (p_Inp->spspps_update_flag) {
                p_des = &p_Inp->spspps_buf[p_Inp->spspps_offset];
                memcpy(p_des, g_start_precode, sizeof(g_start_precode));
                memcpy(p_des + sizeof(g_start_precode), p_strm->nalu_buf, p_strm->nalu_len);
                p_Inp->spspps_offset += p_strm->nalu_len + sizeof(g_start_precode);
                p_Inp->spspps_len = p_Inp->spspps_offset;
            }
        } else if ((p_strm->nalu_type == NALU_TYPE_SLICE)
                   || (p_strm->nalu_type == NALU_TYPE_IDR)) {
            p_Cur->p_Inp->spspps_update_flag = 1;
            p_Inp->spspps_offset = 0;
        }
    }

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET judge_is_new_frame(H264dCurCtx_t *p_Cur, H264dCurStream_t *p_strm)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 nalu_header_bytes  = 0;

    BitReadCtx_t    *p_bitctx = &p_Cur->bitctx;
    memset(p_bitctx, 0, sizeof(BitReadCtx_t));
    {
        RK_U32      forbidden_bit = -1;
        RK_U32  nal_reference_idc = -1;
        RK_U32 svc_extension_flag = -1;
        mpp_set_bitread_ctx(p_bitctx, p_strm->nalu_buf, 4);
        mpp_set_pre_detection(p_bitctx);

        READ_BITS(p_bitctx, 1, &forbidden_bit);
        ASSERT(forbidden_bit == 0);
        READ_BITS(p_bitctx, 2, &nal_reference_idc);
        READ_BITS(p_bitctx, 5, &p_strm->nalu_type);

        nalu_header_bytes = 1;
        if ((p_strm->nalu_type == NALU_TYPE_PREFIX) || (p_strm->nalu_type == NALU_TYPE_SLC_EXT)) {
            READ_BITS(p_bitctx, 1, &svc_extension_flag);
            if (svc_extension_flag) {
                H264D_WARNNING("svc_extension is not supported.");
                goto __FAILED;
            } else {
                if (p_strm->nalu_type == NALU_TYPE_SLC_EXT) {
                    p_strm->nalu_type = NALU_TYPE_SLICE;
                }
            }
            nalu_header_bytes += 3;
        }
    }
    if ((p_strm->nalu_len == 1)
        && (p_strm->nalu_type == NALU_TYPE_SEI
            || p_strm->nalu_type == NALU_TYPE_SPS
            || p_strm->nalu_type == NALU_TYPE_PPS
            || p_strm->nalu_type == NALU_TYPE_AUD)) {
        if (p_Cur->p_Dec->have_slice_data) {
            p_Cur->p_Dec->is_new_frame = 1;
        }
        p_Cur->p_Dec->have_slice_data = 0;
    } else if ((p_strm->nalu_len > 1)
               && (p_strm->nalu_type == NALU_TYPE_SLICE
                   || p_strm->nalu_type == NALU_TYPE_IDR)) {
        RK_U32 first_mb_in_slice  = 0;
        mpp_set_bitread_ctx(p_bitctx, (p_strm->nalu_buf + nalu_header_bytes), 4); // reset
        mpp_set_pre_detection(p_bitctx);
        READ_UE(p_bitctx, &first_mb_in_slice);
        if (first_mb_in_slice == 0) {
            p_Cur->last_dts = p_Cur->curr_dts;
            p_Cur->last_pts = p_Cur->curr_pts;
            p_Cur->curr_dts = p_Cur->p_Inp->in_dts;
            p_Cur->curr_pts = p_Cur->p_Inp->in_pts;
            if (p_Cur->p_Dec->have_slice_data) {
                p_Cur->p_Dec->is_new_frame = 1;
            }
            p_Cur->p_Dec->have_slice_data = 1;
        }
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = p_bitctx->ret;
__FAILED:
    return ret;
}

#define  MAX_ES_FILE_SIZE  (100*1024*1024)
static RK_U32 global_file_fid = 0;
static RK_U64 global_flie_size = 0;
/*!
***********************************************************************
* \brief
*    open stream file to store packet
***********************************************************************
*/
MPP_RET open_stream_file(H264dInputCtx_t *p_Inp, char *path)
{
    if (rkv_h264d_parse_debug & H264D_DBG_WRITE_ES_EN) {

        sprintf(p_Inp->fname[0], "%s/rkv_h264d_file_00.h264", path);
        sprintf(p_Inp->fname[1], "%s/rkv_h264d_file_01.h264", path);

        p_Inp->fp = fopen(p_Inp->fname[global_file_fid], "ab"); {
            if (p_Inp->fp == NULL) {
                H264D_WARNNING("[open_stream_file] can not open stream file, %s.", p_Inp->fname[global_file_fid]);
            }
        }
    }

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    write stream file
***********************************************************************
*/
MPP_RET fwrite_stream_to_file(H264dInputCtx_t *p_Inp, RK_U8 *pdata, RK_U32 len)
{
    if (p_Inp->fp && (rkv_h264d_parse_debug & H264D_DBG_WRITE_ES_EN)) {
        if (p_Inp->fp) {
            fwrite(pdata, sizeof(RK_U8), len, p_Inp->fp);
            fflush(p_Inp->fp);
        }
        global_flie_size += len;
        if (global_flie_size > MAX_ES_FILE_SIZE) {
            MPP_FCLOSE(p_Inp->fp);
            global_file_fid = 1 - global_file_fid;
            global_flie_size = 0;
            p_Inp->fp = fopen(p_Inp->fname[global_file_fid], "wb");
            if (p_Inp->fp == NULL) {
                H264D_WARNNING("[open_stream_file] can not open stream file, %s", p_Inp->fname[global_file_fid]);
            } else {
                fwrite(p_Inp->spspps_buf, sizeof(RK_U8), p_Inp->spspps_len, p_Inp->fp);
                fflush(p_Inp->fp);
            }
        }
    }

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    close stream file
***********************************************************************
*/
MPP_RET close_stream_file(H264dInputCtx_t *p_Inp)
{
    if (rkv_h264d_parse_debug & H264D_DBG_WRITE_ES_EN) {
        if (p_Inp->fp) {
            fflush(p_Inp->fp);
            MPP_FCLOSE(p_Inp->fp);
        }
    }

    return MPP_OK;
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

    H264_DecCtx_t   *p_Dec   = p_Inp->p_Dec;
    H264dCurStream_t *p_strm = &p_Cur->strm;
    MppPacketImpl *pkt_impl  = (MppPacketImpl *)p_Inp->in_pkt;

    p_Dec->nalu_ret = NALU_NULL;
    p_Inp->task_valid = 0;

    //!< check eos
    if (p_Inp->pkt_eos) {
        FUN_CHECK(ret = store_cur_nalu(p_Cur, &p_Cur->strm, p_Dec->dxva_ctx));
        FUN_CHECK(ret = add_empty_nalu(p_strm));
        p_Dec->p_Inp->task_valid = 1;
        p_Dec->p_Inp->task_eos = 1;
        H264D_LOG("----- end of stream ----");
        goto __RETURN;
    }
    //!< check input
    if (!p_Inp->in_length) {
        p_Dec->nalu_ret = HaveNoStream;
        goto __RETURN;
    }
    while (pkt_impl->length > 0) {
        p_strm->curdata = &p_Inp->in_buf[p_strm->nalu_offset++];
        pkt_impl->length--;
        p_strm->prefixdata = (p_strm->prefixdata << 8) | (*p_strm->curdata);
        if (p_strm->startcode_found) {
            if (p_strm->nalu_len >= p_strm->nalu_max_size) {
                FUN_CHECK(ret = realloc_buffer(&p_strm->nalu_buf, &p_strm->nalu_max_size, NALU_BUF_ADD_SIZE));
            }
            p_strm->nalu_buf[p_strm->nalu_len++] = *p_strm->curdata;
            if ((p_strm->nalu_len == NALU_TYPE_NORMAL_LENGTH)
                || (p_strm->nalu_len == NALU_TYPE_EXT_LENGTH)) {
                FUN_CHECK(ret = judge_is_new_frame(p_Cur, p_strm));
                if (p_Cur->p_Dec->is_new_frame) {
                    FUN_CHECK(ret = add_empty_nalu(&p_Cur->strm));
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
            if (p_strm->nalu_len > START_PREFIX_3BYTE) {
                while (p_strm->nalu_buf[p_strm->nalu_len - 1] == 0x00) {
                    p_strm->nalu_len--;
                }
            }
            p_Dec->nalu_ret = EndOfNalu;
            FUN_CHECK(ret = store_cur_nalu(p_Cur, &p_Dec->p_Cur->strm, p_Dec->dxva_ctx));
            reset_nalu(p_strm);
            break;
        }
    }
    p_Inp->in_length = pkt_impl->length;
    //!< check input
    if (!p_Inp->in_length) {
        p_strm->nalu_offset = 0;
        p_Dec->nalu_ret = HaveNoStream;
    }

__RETURN:

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
MPP_RET parse_prepare_fast(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264_DecCtx_t   *p_Dec   = p_Inp->p_Dec;
    H264dCurStream_t *p_strm = &p_Cur->strm;
    MppPacketImpl *pkt_impl  = (MppPacketImpl *)p_Inp->in_pkt;

    p_Dec->nalu_ret = NALU_NULL;
    p_Inp->task_valid = 0;

    while (pkt_impl->length > 0) {
        p_strm->curdata = &p_Inp->in_buf[p_strm->nalu_offset++];
        pkt_impl->length--;
        p_strm->prefixdata = (p_strm->prefixdata << 8) | (*p_strm->curdata);
        if (p_strm->startcode_found) {
            if (p_strm->nalu_len >= p_strm->nalu_max_size) {
                FUN_CHECK(ret = realloc_buffer(&p_strm->nalu_buf, &p_strm->nalu_max_size, NALU_BUF_ADD_SIZE));
            }
            p_strm->nalu_buf[p_strm->nalu_len++] = *p_strm->curdata;
            if (p_strm->nalu_len == 1) {
                p_strm->nalu_type = p_strm->nalu_buf[0] & 0x1F;

                if (p_strm->nalu_type == NALU_TYPE_SLICE
                    || p_strm->nalu_type == NALU_TYPE_IDR || p_strm->nalu_type == NALU_TYPE_SLC_EXT) {
                    p_strm->nalu_len += (RK_U32)pkt_impl->length;
                    if (p_strm->nalu_len >= p_strm->nalu_max_size) {
                        RK_U32 add_size =  pkt_impl->length + 1 - p_strm->nalu_max_size;
                        FUN_CHECK(ret = realloc_buffer(&p_strm->nalu_buf, &p_strm->nalu_max_size, MPP_MAX(NALU_BUF_ADD_SIZE, add_size)));
                    }
                    memcpy(&p_strm->nalu_buf[0], p_strm->curdata, pkt_impl->length + 1);
                    pkt_impl->length = 0;
                    p_Cur->p_Inp->task_valid = 1;
                    break;
                }
            }
        }

        find_prefix_code(p_strm->curdata, p_strm);

        if (p_strm->endcode_found) {
            p_strm->nalu_len -= START_PREFIX_3BYTE;
            while (p_strm->nalu_buf[p_strm->nalu_len - 1] == 0x00) {
                p_strm->nalu_len--;
            }
            p_Dec->nalu_ret = EndOfNalu;
            FUN_CHECK(ret = store_cur_nalu(p_Cur, &p_Dec->p_Cur->strm, p_Dec->dxva_ctx));
            reset_nalu(p_strm);
            break;
        }
    }
    if (p_Cur->p_Inp->task_valid) {
        FUN_CHECK(ret = store_cur_nalu(p_Cur, &p_Dec->p_Cur->strm, p_Dec->dxva_ctx));
        FUN_CHECK(ret = add_empty_nalu(&p_Cur->strm));
        p_Cur->strm.head_offset = 0;
        p_Cur->last_dts = p_Cur->p_Inp->in_dts;
        p_Cur->last_pts = p_Cur->p_Inp->in_pts;
    }
    p_Inp->in_length = pkt_impl->length;
    //!< check input
    if (!p_Inp->in_length) {
        if (!p_Cur->p_Inp->task_valid) {
            p_Dec->nalu_ret = EndOfNalu;
            FUN_CHECK(ret = store_cur_nalu(p_Cur, &p_Dec->p_Cur->strm, p_Dec->dxva_ctx));
        } else {
            p_Dec->nalu_ret = HaveNoStream;
        }
        p_strm->nalu_offset = 0;
        p_strm->endcode_found = 1;

        reset_nalu(p_strm);
        p_strm->startcode_found = 0;
    }

    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    main function for parser avcC header
***********************************************************************
*/
MPP_RET parse_prepare_avcC_header(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur)
{
    RK_S32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dCurStream_t *p_strm = &p_Cur->strm;
    RK_U8 *pdata = p_Inp->in_buf;
    RK_U64 extrasize = p_Inp->in_length;
    MppPacketImpl *pkt_impl  = (MppPacketImpl *)p_Inp->in_pkt;

    //!< free nalu_buffer
    MPP_FREE(p_strm->nalu_buf);
    if (p_Inp->in_length < 7) {
        H264D_ERR("avcC too short, len=%d \n", p_Inp->in_length);
        goto __FAILED;
    }
    if (pdata[0] != 1) {
        goto __FAILED;
    }
    p_Inp->profile = pdata[1];
    p_Inp->level = pdata[3];
    p_Inp->nal_size = 1 + (pdata[4] & 3);
    p_Inp->sps_num = pdata[5] & 31;

    pdata += 6;
    extrasize -= 6;
    for (i = 0; i < p_Inp->sps_num; ++i) {
        p_strm->nalu_len = U16_AT(pdata);
        pdata += 2;
        extrasize -= 2;
        p_strm->nalu_type = NALU_TYPE_SPS;
        p_strm->nalu_buf = pdata;
        FUN_CHECK(ret = store_cur_nalu(p_Cur, p_strm, p_Cur->p_Dec->dxva_ctx));
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
        FUN_CHECK(ret = store_cur_nalu(p_Cur, p_strm, p_Cur->p_Dec->dxva_ctx));
        pdata += p_strm->nalu_len;
        extrasize -= p_strm->nalu_len;
    }
    pkt_impl->length = 0;
    p_strm->nalu_buf = NULL;
    p_strm->startcode_found = 1;

    ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    main function for parser avcC data
***********************************************************************
*/
MPP_RET parse_prepare_avcC_data(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dCurStream_t *p_strm = &p_Cur->strm;
    MppPacketImpl *pkt_impl  = (MppPacketImpl *)p_Inp->in_pkt;

    p_strm->nalu_buf = NULL;
    p_Inp->task_valid = 0;
    if (p_Inp->pkt_eos) {
        p_Inp->task_eos = 1;
        p_Inp->task_valid = 1;
        return MPP_OK;
    }
    VAL_CHECK(ret, (p_Inp->nal_size > 0));
    p_strm->curdata = &p_Inp->in_buf[p_strm->nalu_offset];
    while (p_Inp->in_length > 0) {
        if (p_strm->startcode_found) {
            p_strm->nalu_len = parse_nal_size(p_Inp->nal_size, p_strm->curdata);
            if (p_strm->nalu_len <= 0 ||  p_strm->nalu_len >= p_Inp->in_length) {
                p_Cur->p_Dec->is_new_frame = 1;
                p_Cur->p_Dec->have_slice_data = 0;
                pkt_impl->length = 0;
                p_Inp->in_length = 0;
                p_strm->nalu_len = 0;
                p_strm->nalu_offset = 0;
                p_strm->startcode_found = 1;
                p_strm->endcode_found = 0;
                p_strm->nalu_buf  = NULL;
                goto __FAILED;
            }

            p_strm->nalu_buf = p_strm->curdata + p_Inp->nal_size;
            judge_is_new_frame(p_Cur, p_strm);
            if (p_Cur->p_Dec->is_new_frame) {
                p_Cur->p_Dec->have_slice_data = 0;
                p_strm->startcode_found = 1;
                p_strm->endcode_found = 0;
                break;
            }

            p_strm->curdata += p_Inp->nal_size;
            p_strm->nalu_offset += p_Inp->nal_size;
            pkt_impl->length -= p_Inp->nal_size;
            p_Inp->in_length -= p_Inp->nal_size;

            p_strm->nalu_buf = p_strm->curdata;
            p_strm->nalu_type = p_strm->nalu_buf[0] & 0x1F;
            p_strm->startcode_found = 0;
            p_strm->endcode_found = 1;

            FUN_CHECK(ret = store_cur_nalu(p_Cur, p_strm, p_Cur->p_Dec->dxva_ctx));
            p_strm->curdata += p_strm->nalu_len;
            p_strm->nalu_offset += p_strm->nalu_len;
            pkt_impl->length -= p_strm->nalu_len;
            p_Inp->in_length -= p_strm->nalu_len;
            p_strm->startcode_found = 1;
            p_strm->endcode_found = 0;
            p_strm->nalu_len = 0;
        }

        if (p_Inp->in_length < p_Inp->nal_size) {
            p_Cur->p_Dec->is_new_frame = 1;
            p_Cur->p_Dec->have_slice_data = 0;
            pkt_impl->length = 0;
            p_Inp->in_length = 0;
            p_strm->nalu_len = 0;
            p_strm->nalu_offset = 0;
            p_strm->startcode_found = 1;
            p_strm->endcode_found = 0;
            p_strm->nalu_buf  = NULL;
            break;
        }
    }
    //!< one frame end
    if (p_Cur->p_Dec->is_new_frame) {
        //!< add an empty nalu to tell frame end
        FUN_CHECK(ret = add_empty_nalu(&p_Cur->strm));
        //!< reset curstream parameters
        p_Cur->strm.head_offset = 0;
        p_Cur->p_Inp->task_valid = 1;
        p_Cur->p_Dec->is_new_frame = 0;

        p_Cur->last_dts = p_Inp->in_dts;
        p_Cur->last_pts = p_Inp->in_pts;
    }
    p_strm->nalu_buf = NULL;
    ret = MPP_OK;
__FAILED:
    //p_strm->nalu_len = 0;
    p_Cur->last_dts  = p_Inp->in_dts;
    p_Cur->last_dts  = p_Inp->in_pts;
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

    INP_CHECK(ret, !p_Dec);
    //!< ==== loop ====
    p_Dec->next_state = SliceSTATE_ResetSlice;
    p_curdata = p_Dec->p_Cur->strm.head_buf;

    while (while_loop_flag) {
        switch (p_Dec->next_state) {
        case SliceSTATE_ResetSlice:
            reset_slice(p_Dec->p_Vid);
            p_Dec->next_state = SliceSTATE_ReadNalu;
            H264D_DBG(H264D_DBG_LOOP_STATE, "SliceSTATE_ResetSlice");
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
            H264D_DBG(H264D_DBG_LOOP_STATE, "SliceSTATE_ReadNalu");
            break;
        case SliceSTATE_ParseNalu:
            FUN_CHECK(ret = parser_one_nalu(&p_Dec->p_Cur->slice));
            if (p_Dec->nalu_ret == StartOfSlice) {
                p_Dec->next_state = SliceSTATE_GetSliceData;
            } else if (p_Dec->nalu_ret == StartOfPicture) {
                p_Dec->next_state = SliceSTATE_InitPicture;
            }  else if (p_Dec->nalu_ret == MvcDisAble) {
                H264D_LOG("xxxxxxxx MVC disable");
                goto __FAILED;
            } else {
                p_Dec->next_state = SliceSTATE_ReadNalu;
            }
            H264D_DBG(H264D_DBG_LOOP_STATE, "SliceSTATE_ParseNalu");
            break;
        case SliceSTATE_InitPicture:
            if (!p_Dec->p_Vid->iNumOfSlicesDecoded) {
                FUN_CHECK(ret = init_picture(&p_Dec->p_Cur->slice));
                p_Dec->is_parser_end = 1;
            }
            p_Dec->next_state = SliceSTATE_GetSliceData;
            H264D_DBG(H264D_DBG_LOOP_STATE, "SliceSTATE_InitPicture");
            break;
        case SliceSTATE_GetSliceData:
            FUN_CHECK(ret = fill_slice_syntax(&p_Dec->p_Cur->slice, p_Dec->dxva_ctx));
            p_Dec->p_Vid->iNumOfSlicesDecoded++;
            if (p_Dec->is_parser_end) {
                p_Dec->next_state = SliceSTATE_RegisterOneFrame;
            } else {
                p_Dec->next_state = SliceSTATE_ResetSlice;
            }
            H264D_DBG(H264D_DBG_LOOP_STATE, "SliceSTATE_GetSliceData");
            break;
        case SliceSTATE_RegisterOneFrame:
            if (!p_Dec->is_parser_end) {
                ret = MPP_NOK;
                goto __FAILED;
            }
            commit_buffer(p_Dec->dxva_ctx);
            while_loop_flag = 0;
            p_Dec->next_state = SliceSTATE_ReadNalu;
            H264D_DBG(H264D_DBG_LOOP_STATE, "SliceSTATE_RegisterOneFrame");
            break;
        default:
            ret = MPP_NOK;
            break;
        }
    }

__RETURN:
    return ret = MPP_OK;
__FAILED:
    p_Dec->nalu_ret = NALU_NULL;
    p_Dec->dxva_ctx->slice_count = 0;
    p_Dec->dxva_ctx->strm_offset = 0;
    p_Dec->p_Vid->iNumOfSlicesDecoded = 0;
    p_Dec->p_Vid->exit_picture_flag   = 0;

    return ret;
}


