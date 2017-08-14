/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_vp8e_putbit"

#include "hal_vp8e_base.h"
#include "hal_vp8e_putbit.h"

#define TRACE_BIT_STREAM(v,n)


MPP_RET vp8e_set_buffer(Vp8ePutBitBuf *bitbuf, RK_U8 *data, RK_S32 size)
{
    if ((bitbuf == NULL) || (data == NULL) || (size < 1))
        return MPP_NOK;

    bitbuf->data = data;
    bitbuf->p_data = data;
    bitbuf->size = size;

    bitbuf->range = 255;
    bitbuf->bottom = 0;
    bitbuf->bits_left = 24;

    bitbuf->byte_cnt = 0;

    return MPP_OK;
}

MPP_RET vp8e_put_bool(Vp8ePutBitBuf *bitbuf, RK_S32 prob, RK_S32 bool_value)
{
    RK_S32 bits = 0;
    RK_S32 length_bits = 0;

    RK_S32 split = 1 + ((bitbuf->range - 1) * prob >> 8);

    if (bool_value) {
        bitbuf->bottom += split;
        bitbuf->range -= split;
    } else {
        bitbuf->range = split;
    }

    while (bitbuf->range < 128) {
        if (bitbuf->bottom < 0) {
            RK_U8 *data = bitbuf->data;
            while (*--data == 255) {
                *data = 0;
            }
            (*data)++;
        }
        bitbuf->range <<= 1;
        bitbuf->bottom <<= 1;

        if (!--bitbuf->bits_left) {
            length_bits += 8;
            bits <<= 8;
            bits |= (bitbuf->bottom >> 24) & 0xff;
            TRACE_BIT_STREAM(bits & 0xff, 8);
            *bitbuf->data++ = (bitbuf->bottom >> 24) & 0xff;
            bitbuf->byte_cnt++;
            bitbuf->bottom &= 0xffffff;     /* Keep 3 bytes */
            bitbuf->bits_left = 8;
        }
    }
    return MPP_OK;
}

MPP_RET vp8e_put_lit(Vp8ePutBitBuf *bitbuf, RK_S32 value,
                     RK_S32 number)
{
    while (number--) {
        vp8e_put_bool(bitbuf, 128, (value >> number) & 0x1);
    }
    return MPP_OK;
}

MPP_RET vp8e_put_byte(Vp8ePutBitBuf *bitbuf, RK_S32 byte)
{
    *bitbuf->data++ = byte;
    bitbuf->byte_cnt++;
    return MPP_OK;
}

MPP_RET vp8e_buffer_gap(Vp8ePutBitBuf *bitbuf, RK_S32 gap)
{
    if ((bitbuf->data - bitbuf->p_data) + gap > bitbuf->size) {
        bitbuf->size = 0;
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET vp8e_buffer_overflow(Vp8ePutBitBuf *bitbuf)
{
    if (bitbuf->size > 0)
        return MPP_OK;

    return MPP_NOK;
}
