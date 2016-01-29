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

static RK_U64 mpp_get_bits(RK_U64 src, RK_S32 size, RK_S32 offset)
{
    RK_S32 i;
    RK_U64 temp = 0;

    mpp_assert(size + offset <= 64);// 64 is the FIFO_BIT_WIDTH

    for (i = 0; i < size ; i++) {
        temp <<= 1;
        temp |= 1;
    }
    temp &= (src >> offset);
    temp <<= offset;
    return temp;
}

RK_S32 mpp_set_bitput_ctx(BitputCtx_t *bp, RK_U64 *data, RK_U32 len)
{
    memset(bp, 0, sizeof(BitputCtx_t));
    bp->bit_buf = data;
    bp->total_len = len;
    return 0;
}

void mpp_put_bits(BitputCtx_t *bp, RK_U64 data, RK_S32 size)
{
    // h265h_dbg(H265H_DBG_RPS , "_count = %d value = %d", _count++, (RK_U32)data);
    if (bp->p_uint_index >= bp->total_len) return;
    if (size + bp->p_bit_offset >= 64) { // 64 is the FIFO_BIT_WIDTH
        RK_S32 len = 64 - bp->p_bit_offset;
        bp->bit_buf[bp->p_uint_index] = (mpp_get_bits(data, len, 0) << bp->p_bit_offset) |
                                        mpp_get_bits( bp->bit_buf[bp->p_uint_index],  bp->p_bit_offset, 0);
        bp->p_uint_index++;
        if (bp->p_uint_index > bp->total_len) bp->p_uint_index = 0;

        bp->bit_buf[bp->p_uint_index] = (mpp_get_bits(data, size - len, len) >> len);
        bp->p_bit_offset = size + bp->p_bit_offset - 64; // 64 is the FIFO_BIT_WIDTH

    } else {
        bp->bit_buf[bp->p_uint_index]  = ((data <<  (64 - size)) >> (64 - size -  bp->p_bit_offset)) |
                                         mpp_get_bits( bp->bit_buf[bp->p_uint_index], bp->p_bit_offset, 0);
        bp->p_bit_offset += size;
    }
    bp->p_bit_len += size;
}

void mpp_align(BitputCtx_t *bp, RK_S32 align_width, int flag)
{
    RK_S32 len = 0;
    RK_S32 word_offset = 0;

    if (bp->p_uint_index >= bp->total_len) return;

    word_offset = align_width >= 64 ? (bp->p_uint_index & (((align_width & 0xfe0) >> 6) - 1)) * 64 : 0;
    len = (align_width - (word_offset + (bp->p_bit_offset % align_width ))) % align_width ;

    while (len > 0) { //.len > 0)
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
