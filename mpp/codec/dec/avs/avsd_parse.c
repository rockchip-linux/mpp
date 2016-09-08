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

#define MODULE_TAG "avsd_parse"

#include <string.h>
#include <stdlib.h>

#include "vpu_api.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "hal_task.h"

#include "avsd_api.h"
#include "avsd_parse.h"
#include "avsd_impl.h"

#if 0
#define  START_PREFIX_3BYTE   (3)

static void reset_curstream(AvsdCurStream_t *p_strm)
{
    p_strm->p_start = NULL;
    p_strm->len = 0;
    p_strm->got_frame_flag = 0;
    p_strm->got_nalu_flag = 0;
}

static MPP_RET add_nalu_header(AvsdCurCtx_t *p_cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    RK_U32 add_size = sizeof(AvsdNalu_t);
    AvsdBitstream_t *p_bitstream = p_cur->p_dec->bitstream;

    if ((p_bitstream->offset + add_size) > p_bitstream->size) {
        AVSD_DBG(AVSD_DBG_ERROR, "error, head_offset is larger than %d", p_bitstream->size);
        goto __FAILED;
    }
    p_cur->cur_nalu = (AvsdNalu_t *)&p_bitstream->pbuf[p_bitstream->offset];
    p_cur->cur_nalu->eof       = 1;
    p_cur->cur_nalu->header    = 0;
    p_cur->cur_nalu->pdata     = NULL;
    p_cur->cur_nalu->length    = 0;
    p_cur->cur_nalu->start_pos = 0;
    p_bitstream->offset += add_size;

    (void)p_cur;
    return ret = MPP_OK;
__FAILED:
    return ret;
}



static MPP_RET store_cur_nalu(AvsdCurCtx_t *p_cur, AvsdCurStream_t *p_strm)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AvsdNalu_t *p_nalu = p_cur->cur_nalu;
    AvsdBitstream_t *p_bitstream = p_cur->p_dec->bitstream;

    //!< fill bitstream buffer
    RK_U32 add_size = p_strm->len;

    if ((p_bitstream->offset + add_size) > p_bitstream->size) {
        AVSD_DBG(AVSD_DBG_ERROR, "error, head_offset is larger than %d", p_bitstream->size);
        goto __FAILED;
    }
    p_nalu->length += p_strm->len;
    p_nalu->pdata = &p_bitstream->pbuf[p_bitstream->offset];
    memcpy(p_nalu->pdata, p_strm->p_start, p_strm->len);
    p_bitstream->offset += add_size;

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


MPP_RET avsd_parse_prepare(AvsdInputCtx_t *p_inp, AvsdCurCtx_t *p_cur)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8 *p_curdata = NULL;

    HalDecTask *in_task = p_inp->in_task;
    MppPacket  *in_pkt  = p_inp->in_pkt;
    MppPacketImpl *pkt_impl = (MppPacketImpl *)p_inp->in_pkt;
    AvsdCurStream_t *p_strm = &p_cur->m_strm;
    AvsdBitstream_t *p_bitstream = p_cur->p_dec->bitstream;

    AVSD_PARSE_TRACE("In.");
    //!< check input
    if (!mpp_packet_get_length(in_pkt)) {
        AVSD_DBG(AVSD_DBG_WARNNING, "input have no stream.");
        goto __RETURN;
    }

    in_task->valid = 0;
    in_task->flags.eos = 0;

    //!< check eos
    if (mpp_packet_get_eos(in_pkt)) {
        FUN_CHECK(ret = add_nalu_header(p_cur));
        in_task->valid = 1;
        in_task->flags.eos = 1;
        AVSD_DBG(AVSD_DBG_LOG, "end of stream.");
        goto __RETURN;
    }

    p_strm->p_start = p_curdata = (RK_U8 *)mpp_packet_get_pos(p_inp->in_pkt);
    while (pkt_impl->length > 0) {
        //!< found next picture start code
        if (((*p_curdata) == I_PICTURE_START_CODE)
            || ((*p_curdata) == PB_PICTURE_START_CODE)) {
            if (p_strm->got_frame_flag) {
                FUN_CHECK(ret = add_nalu_header(p_cur));
                in_task->valid = 1;
                pkt_impl->length += START_PREFIX_3BYTE;
                p_bitstream->len = p_bitstream->offset;
                //!< reset value
                p_bitstream->offset = 0;
                reset_curstream(p_strm);
                break;
            }
            p_strm->got_frame_flag = 1;
        }
        //!<  found next nalu start code
        p_strm->prefixdata = (p_strm->prefixdata << 8) | (*p_curdata);
        if ((p_strm->prefixdata & 0xFFFFFF00) == 0x00000100) {
            if (p_strm->got_nalu_flag)  {
                p_strm->len = (RK_U32)(p_curdata - p_strm->p_start) - START_PREFIX_3BYTE;
                FUN_CHECK(ret = store_cur_nalu(p_cur, p_strm));
                FPRINT(p_cur->p_inp->fp_log, "g_nalu_no=%d, length=%d, header=0x%02x \n", g_nalu_no++, p_cur->cur_nalu->length, p_cur->cur_nalu->header);

            }
            FUN_CHECK(ret = add_nalu_header(p_cur));
            p_cur->cur_nalu->header = (*p_curdata);
            p_cur->cur_nalu->eof = 0;
            p_cur->cur_nalu->start_pos = START_PREFIX_3BYTE;

            p_strm->p_start = p_curdata - START_PREFIX_3BYTE;
            p_strm->got_nalu_flag = 1;
        }
        p_curdata++;
        pkt_impl->length--;
    }
    if (!pkt_impl->length) {
        p_strm->len = (RK_U32)(p_curdata - p_strm->p_start) - 1;
        FUN_CHECK(ret = store_cur_nalu(p_cur, p_strm));
    }
__RETURN:
    AVSD_PARSE_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    return ret;
}
#else

MPP_RET avsd_parse_prepare(AvsdInputCtx_t *p_inp, AvsdCurCtx_t *p_cur)
{
    RK_S32 ret_val = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    Avs_DecCtx_t *p_dec = p_inp->p_dec;
    HalDecTask *in_task = p_inp->in_task;

    AVSD_PARSE_TRACE("In.");

    in_task->input_packet = p_dec->task_pkt;
    ret_val = lib_avsd_prepare(p_dec->libdec,
                               (RK_U8 *)mpp_packet_get_pos(p_inp->in_pkt), (RK_S32)mpp_packet_get_length(p_inp->in_pkt));
    if (ret_val < 0) {
        goto __FAILED;
    }
    in_task->valid = 1;
    mpp_packet_set_length(p_inp->in_pkt, 0);

    AVSD_PARSE_TRACE("Out.");
    (void)p_cur;
    return ret = MPP_OK;
__FAILED:
    return ret;
}
#endif


