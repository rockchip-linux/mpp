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

#ifndef __H264E_STREAM_H__
#define __H264E_STREAM_H__

#include "rk_type.h"
#include "mpp_err.h"

/* struct for assemble bitstream */
typedef struct H264eVpuStream_t {
    RK_U8 *buffer; /* point to first byte of stream */
    RK_U8 *stream; /* Pointer to next byte of stream */
    RK_U32 size;   /* Byte size of stream buffer */
    RK_U32 byte_cnt;    /* Byte counter */
    RK_U32 byte_buffer; /* Byte buffer */
    RK_U32 buffered_bits;   /* Amount of bits in byte buffer, [0-7] */
    RK_U32 zero_bytes; /* Amount of consecutive zero bytes */
    RK_S32 overflow;    /* This will signal a buffer overflow */
    RK_U32 emul_cnt; /* Counter for emulation_3_byte, needed in SEI */
} H264eStream;

void h264e_swap_endian(RK_U32 *buf, RK_S32 size_bytes);
MPP_RET h264e_stream_status(H264eStream *stream);
MPP_RET h264e_stream_reset(H264eStream *strmbuf);
MPP_RET h264e_stream_init(H264eStream *strmbuf, void *p, RK_S32 size);
void h264e_stream_put_bits(H264eStream *buffer, RK_S32 value, RK_S32 number,
                           const char *name);
void h264e_stream_put_bits_with_detect(H264eStream * buffer,
                                       RK_S32 value, RK_S32 number,
                                       const char *name);
void h264e_stream_trailing_bits(H264eStream * stream);
void h264e_stream_write_ue(H264eStream *fifo, RK_U32 val, const char *name);
void h264e_stream_write_se(H264eStream *fifo, RK_S32 val, const char *name);

RK_S32 exp_golomb_signed(RK_S32 val);

#endif /* __H264E_STREAM_H__ */
