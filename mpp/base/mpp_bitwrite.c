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

#include "mpp_bitwrite.h"

MPP_RET mpp_writer_status(MppWriteCtx *ctx)
{
    if (ctx->byte_cnt > ctx->size) {
        ctx->overflow = 1;
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_writer_reset(MppWriteCtx *ctx)
{
    ctx->stream = ctx->buffer;
    ctx->byte_cnt = 0;
    ctx->byte_buffer = 0;
    ctx->buffered_bits = 0;
    ctx->zero_bytes = 0;
    ctx->overflow = 0;
    ctx->emul_cnt = 0;

    return MPP_OK;
}

MPP_RET mpp_writer_init(MppWriteCtx *ctx, void *p, RK_S32 size)
{
    MPP_RET ret;

    ctx->buffer = p;
    ctx->stream = p;
    ctx->size = size;
    ctx->byte_cnt = 0;
    ctx->overflow = 0;
    ctx->byte_buffer = 0;
    ctx->buffered_bits = 0;
    ctx->zero_bytes = 0;
    ctx->emul_cnt = 0;

    ret = mpp_writer_status(ctx);
    if (ret)
        mpp_err_f("failed to init with overflow config\n");

    return ret;
}

void mpp_writer_put_raw_bits(MppWriteCtx *ctx, RK_S32 val, RK_S32 len)
{
    RK_S32 bits;
    RK_U32 byte_buffer = ctx->byte_buffer;
    RK_U8*stream = ctx->stream;

    if (mpp_writer_status(ctx))
        return;

    mpp_assert(val < (1 << len));
    mpp_assert(len < 25);

    bits = len + ctx->buffered_bits;
    val <<= (32 - bits);
    byte_buffer = byte_buffer | val;

    while (bits > 7) {
        *stream = (RK_U8)(byte_buffer >> 24);

        bits -= 8;
        byte_buffer <<= 8;
        stream++;
        ctx->byte_cnt++;
    }

    ctx->byte_buffer = byte_buffer;
    ctx->buffered_bits = (RK_U8)bits;
    ctx->stream = stream;

    return;
}

void mpp_writer_flush(MppWriteCtx *ctx)
{
    RK_U32 byte_buffer = ctx->byte_buffer;
    RK_U8*stream = ctx->stream;

    if (mpp_writer_status(ctx))
        return;

    if (ctx->buffered_bits)
        *stream = (RK_U8)(byte_buffer >> 24);

    return;
}

void mpp_writer_put_bits(MppWriteCtx * ctx, RK_S32 val, RK_S32 len)
{
    RK_S32 bits;
    RK_U8 *stream = ctx->stream;
    RK_U32 byte_buffer = ctx->byte_buffer;

    if (val) {
        mpp_assert(val < (1 << len));
        mpp_assert(len < 25);
    }
    bits = len + ctx->buffered_bits;

    byte_buffer = byte_buffer | ((RK_U32) val << (32 - bits));

    while (bits > 7) {
        RK_S32 zeroBytes = ctx->zero_bytes;
        RK_S32 byteCnt = ctx->byte_cnt;

        if (mpp_writer_status(ctx))
            return;

        *stream = (RK_U8) (byte_buffer >> 24);

        byteCnt++;
        if ((zeroBytes == 2) && (*stream < 4)) {
            *stream++ = 3;
            *stream = (RK_U8) (byte_buffer >> 24);
            byteCnt++;
            zeroBytes = 0;
            ctx->emul_cnt++;
        }

        if (*stream == 0)
            zeroBytes++;
        else
            zeroBytes = 0;

        bits -= 8;
        byte_buffer <<= 8;
        stream++;
        ctx->zero_bytes = zeroBytes;
        ctx->byte_cnt = byteCnt;
        ctx->stream = stream;
    }

    ctx->buffered_bits = (RK_U8) bits;
    ctx->byte_buffer = byte_buffer;
}

void mpp_writer_align_zero(MppWriteCtx *ctx)
{
    if (ctx->buffered_bits)
        mpp_writer_put_raw_bits(ctx, 0, 8 - ctx->buffered_bits);
}

void mpp_writer_align_one(MppWriteCtx *ctx)
{
    if (ctx->buffered_bits) {
        RK_S32 len = 8 - ctx->buffered_bits;

        mpp_writer_put_raw_bits(ctx, (1 << len) - 1, len);
    }
}

void mpp_writer_trailing(MppWriteCtx *ctx)
{
    mpp_writer_put_bits(ctx, 1, 1);
    if (ctx->buffered_bits)
        mpp_writer_put_bits(ctx, 0, 8 - ctx->buffered_bits);
}

void mpp_writer_put_ue(MppWriteCtx *ctx, RK_U32 val)
{
    RK_U32 num_bits = 0;

    val++;
    while (val >> ++num_bits);

    if (num_bits > 12) {
        RK_U32 tmp;

        tmp = num_bits - 1;

        if (tmp > 24) {
            tmp -= 24;
            mpp_writer_put_bits(ctx, 0, 24);
        }

        mpp_writer_put_bits(ctx, 0, tmp);

        if (num_bits > 24) {
            num_bits -= 24;
            mpp_writer_put_bits(ctx, val >> num_bits, 24);
            val = val >> num_bits;
        }

        mpp_writer_put_bits(ctx, val, num_bits);
    } else {
        mpp_writer_put_bits(ctx, val, 2 * num_bits - 1);
    }
}

void mpp_writer_put_se(MppWriteCtx *ctx, RK_S32 val)
{
    RK_U32 tmp;

    if (val > 0)
        tmp = (RK_U32)(2 * val - 1);
    else
        tmp = (RK_U32)(-2 * val);

    mpp_writer_put_ue(ctx, tmp);
}

RK_S32 mpp_writer_bytes(MppWriteCtx *ctx)
{
    return ctx->byte_cnt + (ctx->buffered_bits > 0);
}

RK_S32 mpp_writer_bits(MppWriteCtx *ctx)
{
    return ctx->byte_cnt * 8 + ctx->buffered_bits;
}

RK_S32 mpp_exp_golomb_signed(RK_S32 val)
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
