
/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define MODULE_TAG "hal_h264d_fifo"

#include <stdio.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"

#include "h264d_log.h"
#include "hal_h264d_fifo.h"

/*!
***********************************************************************
* \brief
*    write fifo data to file
***********************************************************************
*/
//extern "C"
void fifo_fwrite(FILE *fp, void *psrc, RK_U32 size)
{
    fwrite(psrc, 1, size, fp);
    fflush(fp);
}

/*!
***********************************************************************
* \brief
*    flush left bytes to fifo
***********************************************************************
*/
//extern "C"
void fifo_flush_bits(FifoCtx_t *pkt)
{
    if (pkt->bitpos) {
        fifo_write_bits(pkt, 0, (64 - pkt->bitpos), "flush");
        pkt->index++;
        pkt->bitpos = 0;
    }
}

//void fifo_fwrite_header(FifoCtx_t *pkt, RK_S32 pkt_size)
//{
//  if (pkt->fp_data)
//  {
//      pkt->size = pkt_size;
//      fifo_fwrite(pkt->fp_data, &pkt->header, sizeof(RK_U32));
//      fifo_fwrite(pkt->fp_data, &pkt->size, sizeof(RK_U32));
//  }
//}

/*!
***********************************************************************
* \brief
*    fwrite header and data
***********************************************************************
*/
//extern "C"
void fifo_fwrite_data(FifoCtx_t *pkt)
{
    RK_U32 pkt_size = 0;

    if (pkt->fp_data) {
        pkt_size = pkt->index * sizeof(RK_U64);
        fifo_fwrite(pkt->fp_data, &pkt->header, sizeof(RK_U32));
        fifo_fwrite(pkt->fp_data, &pkt_size,    sizeof(RK_U32));
        fifo_fwrite(pkt->fp_data, pkt->pbuf, pkt->index * sizeof(RK_U64));
    }
}

/*!
***********************************************************************
* \brief
*    write bits to fifo
***********************************************************************
*/
//extern "C"
void  fifo_write_bits(FifoCtx_t *pkt, RK_U64 invalue, RK_U8 lbits, const char *name)
{
    RK_U8 hbits = 0;

    if (!lbits) return;

    hbits = 64 - lbits;
    invalue = (invalue << hbits) >> hbits;
    LogInfo(pkt->logctx, "%48s = %10d  (bits=%d)", name, invalue, lbits);
    pkt->bvalue |= invalue << pkt->bitpos;  // high bits value
    if ((pkt->bitpos + lbits) >= 64) {
        pkt->pbuf[pkt->index] = pkt->bvalue;
        pkt->bvalue = invalue >> (64 - pkt->bitpos);  // low bits value
        pkt->index++;
        ASSERT(pkt->index <= pkt->buflen);
    }
    pkt->pbuf[pkt->index] = pkt->bvalue;
    pkt->bitpos = (pkt->bitpos + lbits) & 63;
}
/*!
***********************************************************************
* \brief
*    align fifo bits
***********************************************************************
*/
//extern "C"
void fifo_align_bits(FifoCtx_t *pkt, RK_U8 align_bits)
{
    RK_U32 word_offset = 0, bits_offset = 0, bitlen = 0;

    word_offset = (align_bits >= 64) ? ((pkt->index & (((align_bits & 0xfe0) >> 6) - 1)) << 6) : 0;
    bits_offset = (align_bits - (word_offset + (pkt->bitpos % align_bits))) % align_bits;
    while (bits_offset > 0) {
        bitlen = (bits_offset >= 8) ? 8 : bits_offset;
        fifo_write_bits(pkt, 0, bitlen, "align");
        bits_offset -= bitlen;
    }
}
/*!
***********************************************************************
* \brief
*    write bytes to fifo
***********************************************************************
*/
//extern "C"
void fifo_write_bytes(FifoCtx_t *pkt, void *psrc, RK_U32 size)
{
    RK_U8   hbits   = 0;
    RK_U32  bitslen = 0;
    RK_U8  *pdst    = NULL;

    hbits = 64 - pkt->bitpos;
    pkt->pbuf[pkt->index] = pkt->bitpos ? ((pkt->bvalue << hbits) >> hbits) : 0;
    if (size) {
        pdst = (RK_U8 *)&pkt->pbuf[pkt->index];
        pdst += pkt->bitpos / 8;

        memcpy(pdst, psrc, size);
        bitslen = size * 8 + pkt->bitpos;

        pkt->index += bitslen / 64;
        pkt->bitpos = bitslen & 63;

        hbits = 64 - pkt->bitpos;
        pkt->bvalue = pkt->bitpos ? ((pkt->pbuf[pkt->index] << hbits) >> hbits) : 0;
    }
}
/*!
***********************************************************************
* \brief
*    reset fifo packet
***********************************************************************
*/
//extern "C"
void fifo_packet_reset(FifoCtx_t *pkt)
{
    pkt->index  = 0;
    pkt->bitpos = 0;
    pkt->bvalue = 0;
    pkt->size   = 0;
}
/*!
***********************************************************************
* \brief
*    init fifo packet
***********************************************************************
*/
//extern "C"
void fifo_packet_init(FifoCtx_t *pkt, void *p_start, RK_S32 size)
{
    pkt->index  = 0;
    pkt->bitpos = 0;
    pkt->bvalue = 0;
    pkt->size   = size;
    pkt->buflen = (pkt->size + 7) / 8;  // align 64bit
    pkt->pbuf   = (RK_U64 *)p_start;
}


/*!
***********************************************************************
* \brief
*    alloc fifo packet
***********************************************************************
*/
//extern "C"
MPP_RET fifo_packet_alloc(FifoCtx_t *pkt, RK_S32 header, RK_S32 size)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    pkt->header = header;
    pkt->index  = 0;
    pkt->bitpos = 0;
    pkt->bvalue = 0;
    pkt->size   = size;
    pkt->buflen = (pkt->size + 7) / 8;  // align 64bit
    pkt->pbuf   = NULL;
    if (pkt->buflen) {
        pkt->pbuf = mpp_calloc(RK_U64, pkt->buflen);
        MEM_CHECK(ret, pkt->pbuf);
    }

    return MPP_OK;

__FAILED:
    return ret;
}


