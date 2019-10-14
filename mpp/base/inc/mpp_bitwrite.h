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

#ifndef __MPP_BITWRITER_H__
#define __MPP_BITWRITER_H__

#include "rk_type.h"
#include "mpp_err.h"

/*
 * Mpp bitstream writer for H.264/H.265
 */
typedef struct MppWriteCtx_t {
    RK_U8 *buffer;          /* point to first byte of stream */
    RK_U8 *stream;          /* Pointer to next byte of stream */
    RK_U32 size;            /* Byte size of stream buffer */
    RK_U32 byte_cnt;        /* Byte counter */
    RK_U32 byte_buffer;     /* Byte buffer */
    RK_U32 buffered_bits;   /* Amount of bits in byte buffer, [0-7] */
    RK_U32 zero_bytes;      /* Amount of consecutive zero bytes */
    RK_S32 overflow;        /* This will signal a buffer overflow */
    RK_U32 emul_cnt;        /* Counter for emulation_3_byte, needed in SEI */
} MppWriteCtx;

MPP_RET mpp_writer_init(MppWriteCtx *ctx, void *p, RK_S32 size);
MPP_RET mpp_writer_reset(MppWriteCtx *ctx);

/* check overflow status */
MPP_RET mpp_writer_status(MppWriteCtx *ctx);

/* write raw bit without emulation prevention 0x03 byte */
void mpp_writer_put_raw_bits(MppWriteCtx *ctx, RK_S32 val, RK_S32 len);

/* write bit with emulation prevention 0x03 byte */
void mpp_writer_put_bits(MppWriteCtx *ctx, RK_S32 val, RK_S32 len);

/* insert zero bits until byte-aligned */
void mpp_writer_align_zero(MppWriteCtx *ctx);

/* insert one bits until byte-aligned */
void mpp_writer_align_one(MppWriteCtx *ctx);

/* insert one bit then pad to byte-align with zero */
void mpp_writer_trailing(MppWriteCtx * ctx);

void mpp_writer_put_ue(MppWriteCtx *ctx, RK_U32 val);
void mpp_writer_put_se(MppWriteCtx *ctx, RK_S32 val);

RK_S32 mpp_writer_bytes(MppWriteCtx *ctx);
RK_S32 mpp_writer_bits(MppWriteCtx *ctx);

RK_S32 mpp_exp_golomb_signed(RK_S32 val);

#endif /* __MPP_BITWRITER_H__ */
