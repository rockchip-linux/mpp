/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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

#include "mpp_log.h"
#include "mpp_mem.h"

#include "h264e_stream.h"

void h264e_swap_endian(RK_U32 *buf, RK_S32 size_bytes)
{
    RK_U32 i = 0;
    RK_S32 words = size_bytes / 4;
    RK_U32 val, val2, tmp, tmp2;

    mpp_assert((size_bytes % 8) == 0);

    while (words > 0) {
        val = buf[i];
        tmp = 0;

        tmp |= (val & 0xFF) << 24;
        tmp |= (val & 0xFF00) << 8;
        tmp |= (val & 0xFF0000) >> 8;
        tmp |= (val & 0xFF000000) >> 24;
        {
            val2 = buf[i + 1];
            tmp2 = 0;

            tmp2 |= (val2 & 0xFF) << 24;
            tmp2 |= (val2 & 0xFF00) << 8;
            tmp2 |= (val2 & 0xFF0000) >> 8;
            tmp2 |= (val2 & 0xFF000000) >> 24;

            buf[i] = tmp2;
            words--;
            i++;
        }
        buf[i] = tmp;
        words--;
        i++;
    }
}

MPP_RET h264e_stream_status(H264eStream *stream)
{
    if (stream->byte_cnt + 5 > stream->size) {
        stream->overflow = 1;
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET h264e_stream_reset(H264eStream *strmbuf)
{
    strmbuf->stream = strmbuf->buffer;
    strmbuf->byte_cnt = 0;
    strmbuf->overflow = 0;
    strmbuf->byte_buffer = 0;
    strmbuf->buffered_bits = 0;
    strmbuf->zero_bytes = 0;
    strmbuf->emul_cnt = 0;

    return MPP_OK;
}

MPP_RET h264e_stream_init(H264eStream *strmbuf, void *p, RK_S32 size)
{
    strmbuf->buffer = p;
    strmbuf->stream = p;
    strmbuf->size = size;
    strmbuf->byte_cnt = 0;
    strmbuf->overflow = 0;
    strmbuf->byte_buffer = 0;
    strmbuf->buffered_bits = 0;
    strmbuf->zero_bytes = 0;
    strmbuf->emul_cnt = 0;

    if (h264e_stream_status(strmbuf)) {
        mpp_err("stream buffer is overflow, while init");
        return MPP_NOK;
    }

    return MPP_OK;
}

void h264e_stream_put_bits(H264eStream *buffer, RK_S32 value, RK_S32 number,
                           const char *name)
{
    RK_S32 bits;
    RK_U32 byte_buffer = buffer->byte_buffer;
    RK_U8*stream = buffer->stream;
    (void)name;

    if (h264e_stream_status(buffer) != 0)
        return;

    mpp_assert(value < (1 << number)); //opposite to 'BUG_ON' in kernel
    mpp_assert(number < 25);

    bits = number + buffer->buffered_bits;
    value <<= (32 - bits);
    byte_buffer = byte_buffer | value;

    while (bits > 7) {
        *stream = (RK_U8)(byte_buffer >> 24);

        bits -= 8;
        byte_buffer <<= 8;
        stream++;
        buffer->byte_cnt++;
    }

    buffer->byte_buffer = byte_buffer;
    buffer->buffered_bits = (RK_U8)bits;
    buffer->stream = stream;

    return;
}

void h264e_stream_put_bits_with_detect(H264eStream * buffer,
                                       RK_S32 value, RK_S32 number,
                                       const char *name)
{
    RK_S32 bits;
    RK_U8 *stream = buffer->stream;
    RK_U32 byte_buffer = buffer->byte_buffer;
    (void)name;

    if (value) {
        mpp_assert(value < (1 << number));
        mpp_assert(number < 25);
    }
    bits = number + buffer->buffered_bits;

    byte_buffer = byte_buffer | ((RK_U32) value << (32 - bits));

    while (bits > 7) {
        RK_S32 zeroBytes = buffer->zero_bytes;
        RK_S32 byteCnt = buffer->byte_cnt;

        if (h264e_stream_status(buffer) != MPP_OK)
            return;

        *stream = (RK_U8) (byte_buffer >> 24);
        byteCnt++;

        if ((zeroBytes == 2) && (*stream < 4)) {
            *stream++ = 3;
            *stream = (RK_U8) (byte_buffer >> 24);
            byteCnt++;
            zeroBytes = 0;
            buffer->emul_cnt++;
        }

        if (*stream == 0)
            zeroBytes++;
        else
            zeroBytes = 0;

        bits -= 8;
        byte_buffer <<= 8;
        stream++;
        buffer->zero_bytes = zeroBytes;
        buffer->byte_cnt = byteCnt;
        buffer->stream = stream;
    }

    buffer->buffered_bits = (RK_U8) bits;
    buffer->byte_buffer = byte_buffer;
}

void h264e_stream_trailing_bits(H264eStream * stream)
{
    h264e_stream_put_bits_with_detect(stream, 1, 1,
                                      "rbsp_stop_one_bit");
    if (stream->buffered_bits > 0)
        h264e_stream_put_bits_with_detect(stream, 0,
                                          8 - stream->buffered_bits,
                                          "bsp_alignment_zero_bit(s)");
}

void h264e_stream_write_ue(H264eStream *fifo, RK_U32 val, const char *name)
{
    RK_U32 num_bits = 0;

    val++;
    while (val >> ++num_bits);

    if (num_bits > 12) {
        RK_U32 tmp;

        tmp = num_bits - 1;

        if (tmp > 24) {
            tmp -= 24;
            h264e_stream_put_bits_with_detect(fifo, 0, 24, name);
        }

        h264e_stream_put_bits_with_detect(fifo, 0, tmp, name);

        if (num_bits > 24) {
            num_bits -= 24;
            h264e_stream_put_bits_with_detect(fifo, val >> num_bits,
                                              24, name);
            val = val >> num_bits;
        }

        h264e_stream_put_bits_with_detect(fifo, val, num_bits, name);
    } else {
        h264e_stream_put_bits_with_detect(fifo, val,
                                          2 * num_bits - 1, name);
    }
}

void h264e_stream_write_se(H264eStream *fifo, RK_S32 val, const char *name)
{
    RK_U32 tmp;

    if (val > 0)
        tmp = (RK_U32)(2 * val - 1);
    else
        tmp = (RK_U32)(-2 * val);

    h264e_stream_write_ue(fifo, tmp, name);
}

RK_S32 exp_golomb_signed(RK_S32 val)
{
    RK_S32 tmp = 0;

    if (val > 0)
        val = 2 * val;
    else
        val = -2 * val + 1;

    while (val >> ++tmp)
        ;

    return tmp * 2 - 1;
}
