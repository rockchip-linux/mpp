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
#include "mpp_bitput.h"

RK_S32 mpp_set_bitput_ctx(BitputCtx_t *bp, RK_U64 *data, RK_U32 len)
{
    memset(bp, 0, sizeof(BitputCtx_t));
    bp->index  = 0;
    bp->bitpos = 0;
    bp->bvalue = 0;
    bp->size   = len;
    bp->buflen = len;  // align 64bit
    bp->pbuf   = data;
    return 0;
}

void mpp_put_bits(BitputCtx_t *bp, RK_U64 invalue, RK_S32 lbits)
{
    RK_U8 hbits = 0;

    if (!lbits) return;

    if (bp->index >= bp->buflen) return;

    hbits = 64 - lbits;
    invalue = (invalue << hbits) >> hbits;
    bp->bvalue |= invalue << bp->bitpos;  // high bits value
    if ((bp->bitpos + lbits) >= 64) {
        bp->pbuf[bp->index] = bp->bvalue;
        bp->bvalue = invalue >> (64 - bp->bitpos);  // low bits value
        bp->index++;
    }
    bp->pbuf[bp->index] = bp->bvalue;
    bp->bitpos = (bp->bitpos + lbits) & 63;
    // mpp_log("bp->index = %d bp->bitpos = %d lbits = %d invalue 0x%x bp->hvalue 0x%x  bp->lvalue 0x%x",bp->index,bp->bitpos,lbits, (RK_U32)invalue,(RK_U32)(bp->bvalue >> 32),(RK_U32)bp->bvalue);
}

void mpp_put_align(BitputCtx_t *bp, RK_S32 align_bits, int flag)
{
    RK_U32 word_offset = 0,  len = 0;

    word_offset = (align_bits >= 64) ? ((bp->index & (((align_bits & 0xfe0) >> 6) - 1)) << 6) : 0;
    len = (align_bits - (word_offset + (bp->bitpos % align_bits))) % align_bits;
    while (len > 0) {
        if (len >= 8) {
            if (flag == 0)
                mpp_put_bits(bp, ((RK_U64)0 << (64 - 8)) >> (64 - 8), 8);
            else
                mpp_put_bits(bp, (0xffffffffffffffff << (64 - 8)) >> (64 - 8), 8);
            len -= 8;
        } else {
            if (flag == 0)
                mpp_put_bits(bp, ((RK_U64)0 << (64 - len)) >> (64 - len), len);
            else
                mpp_put_bits(bp, (0xffffffffffffffff << (64 - len)) >> (64 - len), len);
            len -= len;
        }
    }
}

