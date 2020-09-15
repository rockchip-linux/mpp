/*
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

#define MODULE_TAG "h265e_cabac"

#include "mpp_mem.h"
#include "mpp_common.h"

#include "h265e_slice.h"
#include "h265e_codec.h"
#include "h265e_enctropy.h"

#ifdef __GNUC__                         /* GCCs builtin atomics */
#define CLZ32(id, x)                        id = (unsigned long)__builtin_clz(x) ^ 31
#elif defined(_MSC_VER)                 /* Windows atomic intrinsics */
#define CLZ32(id, x)                        _BitScanReverse(&id, x)
#endif

const RK_U8 g_lpsTable[64][4] = {
    { 128, 176, 208, 240 },
    { 128, 167, 197, 227 },
    { 128, 158, 187, 216 },
    { 123, 150, 178, 205 },
    { 116, 142, 169, 195 },
    { 111, 135, 160, 185 },
    { 105, 128, 152, 175 },
    { 100, 122, 144, 166 },
    {  95, 116, 137, 158 },
    {  90, 110, 130, 150 },
    {  85, 104, 123, 142 },
    {  81,  99, 117, 135 },
    {  77,  94, 111, 128 },
    {  73,  89, 105, 122 },
    {  69,  85, 100, 116 },
    {  66,  80,  95, 110 },
    {  62,  76,  90, 104 },
    {  59,  72,  86,  99 },
    {  56,  69,  81,  94 },
    {  53,  65,  77,  89 },
    {  51,  62,  73,  85 },
    {  48,  59,  69,  80 },
    {  46,  56,  66,  76 },
    {  43,  53,  63,  72 },
    {  41,  50,  59,  69 },
    {  39,  48,  56,  65 },
    {  37,  45,  54,  62 },
    {  35,  43,  51,  59 },
    {  33,  41,  48,  56 },
    {  32,  39,  46,  53 },
    {  30,  37,  43,  50 },
    {  29,  35,  41,  48 },
    {  27,  33,  39,  45 },
    {  26,  31,  37,  43 },
    {  24,  30,  35,  41 },
    {  23,  28,  33,  39 },
    {  22,  27,  32,  37 },
    {  21,  26,  30,  35 },
    {  20,  24,  29,  33 },
    {  19,  23,  27,  31 },
    {  18,  22,  26,  30 },
    {  17,  21,  25,  28 },
    {  16,  20,  23,  27 },
    {  15,  19,  22,  25 },
    {  14,  18,  21,  24 },
    {  14,  17,  20,  23 },
    {  13,  16,  19,  22 },
    {  12,  15,  18,  21 },
    {  12,  14,  17,  20 },
    {  11,  14,  16,  19 },
    {  11,  13,  15,  18 },
    {  10,  12,  15,  17 },
    {  10,  12,  14,  16 },
    {   9,  11,  13,  15 },
    {   9,  11,  12,  14 },
    {   8,  10,  12,  14 },
    {   8,   9,  11,  13 },
    {   7,   9,  11,  12 },
    {   7,   9,  10,  12 },
    {   7,   8,  10,  11 },
    {   6,   8,   9,  11 },
    {   6,   7,   9,  10 },
    {   6,   7,   8,   9 },
    {   2,   2,   2,   2 }
};

const RK_U8 g_nextState[128][2] = {
    { 2, 1 }, { 0, 3 }, { 4, 0 }, { 1, 5 }, { 6, 2 }, { 3, 7 }, { 8, 4 }, { 5, 9 },
    { 10, 4 }, { 5, 11 }, { 12, 8 }, { 9, 13 }, { 14, 8 }, { 9, 15 }, { 16, 10 }, { 11, 17 },
    { 18, 12 }, { 13, 19 }, { 20, 14 }, { 15, 21 }, { 22, 16 }, { 17, 23 }, { 24, 18 }, { 19, 25 },
    { 26, 18 }, { 19, 27 }, { 28, 22 }, { 23, 29 }, { 30, 22 }, { 23, 31 }, { 32, 24 }, { 25, 33 },
    { 34, 26 }, { 27, 35 }, { 36, 26 }, { 27, 37 }, { 38, 30 }, { 31, 39 }, { 40, 30 }, { 31, 41 },
    { 42, 32 }, { 33, 43 }, { 44, 32 }, { 33, 45 }, { 46, 36 }, { 37, 47 }, { 48, 36 }, { 37, 49 },
    { 50, 38 }, { 39, 51 }, { 52, 38 }, { 39, 53 }, { 54, 42 }, { 43, 55 }, { 56, 42 }, { 43, 57 },
    { 58, 44 }, { 45, 59 }, { 60, 44 }, { 45, 61 }, { 62, 46 }, { 47, 63 }, { 64, 48 }, { 49, 65 },
    { 66, 48 }, { 49, 67 }, { 68, 50 }, { 51, 69 }, { 70, 52 }, { 53, 71 }, { 72, 52 }, { 53, 73 },
    { 74, 54 }, { 55, 75 }, { 76, 54 }, { 55, 77 }, { 78, 56 }, { 57, 79 }, { 80, 58 }, { 59, 81 },
    { 82, 58 }, { 59, 83 }, { 84, 60 }, { 61, 85 }, { 86, 60 }, { 61, 87 }, { 88, 60 }, { 61, 89 },
    { 90, 62 }, { 63, 91 }, { 92, 64 }, { 65, 93 }, { 94, 64 }, { 65, 95 }, { 96, 66 }, { 67, 97 },
    { 98, 66 }, { 67, 99 }, { 100, 66 }, { 67, 101 }, { 102, 68 }, { 69, 103 }, { 104, 68 }, { 69, 105 },
    { 106, 70 }, { 71, 107 }, { 108, 70 }, { 71, 109 }, { 110, 70 }, { 71, 111 }, { 112, 72 }, { 73, 113 },
    { 114, 72 }, { 73, 115 }, { 116, 72 }, { 73, 117 }, { 118, 74 }, { 75, 119 }, { 120, 74 }, { 75, 121 },
    { 122, 74 }, { 75, 123 }, { 124, 76 }, { 77, 125 }, { 124, 76 }, { 77, 125 }, { 126, 126 }, { 127, 127 }
};

RK_U8 sbacInit(RK_S32 qp, RK_S32 initValue)
{
    RK_S32  slope      = (initValue >> 4) * 5 - 45;
    RK_S32  offset     = ((initValue & 15) << 3) - 16;
    RK_S32  initState  =  MPP_MIN(MPP_MAX(1, (((slope * qp) >> 4) + offset)), 126);
    RK_U32  mpState = (initState >= 64);
    RK_U8   m_state = ((mpState ? (initState - 64) : (63 - initState)) << 1) + mpState;

    return m_state;
}

static void initBuffer(H265eContextModel_t* contextModel, SliceType sliceType, RK_U8 cabacIntFlag, RK_S32 qp, RK_U8* ctxModel, RK_S32 size)
{
    RK_S8 initType = 0;
    RK_S32 n = 0;
    h265e_dbg_skip("sliceType = %d", sliceType);
    if (sliceType == I_SLICE)
        initType = 0;
    else if (sliceType == P_SLICE)
        initType = cabacIntFlag ? 2 : 1;
    else
        initType = cabacIntFlag ? 1 : 2;

    ctxModel += (2 - initType) * size;

    for (n = 0; n < size; n++) {
        contextModel[n].m_state = sbacInit(qp, ctxModel[n]);
        //mpp_log("contextModel[%d].m_state = %d", n, contextModel[n].m_state);
        contextModel[n].bBinsCoded = 0;
    }
}

void h265e_reset_enctropy(void *slice_ctx)
{
    H265eSlice *slice = (H265eSlice *)slice_ctx;
    RK_U8 cabacInitFlag = slice->m_cabacInitFlag;
    RK_S32  qp              = slice->m_sliceQp;
    SliceType sliceType  = slice->m_sliceType;

    h265e_dbg_func("enter\n");
    initBuffer(&slice->m_contextModels[OFF_SPLIT_FLAG_CTX], sliceType, cabacInitFlag, qp, (RK_U8*)INIT_SPLIT_FLAG, NUM_SPLIT_FLAG_CTX);
    initBuffer(&slice->m_contextModels[OFF_SKIP_FLAG_CTX], sliceType, cabacInitFlag, qp, (RK_U8*)INIT_SKIP_FLAG, NUM_SKIP_FLAG_CTX);
    initBuffer(&slice->m_contextModels[OFF_MERGE_FLAG_EXT_CTX], sliceType, cabacInitFlag, qp, (RK_U8*)INIT_MERGE_FLAG_EXT, NUM_MERGE_FLAG_EXT_CTX);
    initBuffer(&slice->m_contextModels[OFF_MERGE_IDX_EXT_CTX], sliceType, cabacInitFlag, qp, (uint8_t*)INIT_MERGE_IDX_EXT, NUM_MERGE_IDX_EXT_CTX);

    h265e_dbg_func("leave\n");
}

void h265e_cabac_resetBits(H265eCabacCtx *cabac_ctx)
{
    h265e_dbg_func("enter\n");

    cabac_ctx->m_low              = 0;
    cabac_ctx->m_bitsLeft         = -12;
    cabac_ctx->m_numBufferedBytes = 0;
    cabac_ctx->m_bufferedByte     = 0xff;
    cabac_ctx->m_fracBits           = 0;

    h265e_dbg_func("leave\n");
}

void h265e_cabac_start(H265eCabacCtx *cabac_ctx)
{
    h265e_dbg_func("enter\n");
    cabac_ctx->m_low              = 0;
    cabac_ctx->m_range            = 510;
    cabac_ctx->m_bitsLeft         = -12;
    cabac_ctx->m_numBufferedBytes = 0;
    cabac_ctx->m_bufferedByte     = 0xff;
}

void h265e_cabac_init(H265eCabacCtx *cabac_ctx, MppWriteCtx * bitIf)
{
    h265e_dbg_func("enter\n");
    cabac_ctx->m_bitIf = bitIf;
    h265e_cabac_start(cabac_ctx);
    h265e_dbg_func("leave\n");
}

void h265e_cabac_writeOut(H265eCabacCtx *cabac_ctx)
{
    MppWriteCtx* s = cabac_ctx->m_bitIf;
    RK_U32 leadByte = cabac_ctx->m_low >> (13 + cabac_ctx->m_bitsLeft);
    RK_U32 low_mask = (RK_U32)(~0) >> (11 + 8 - cabac_ctx->m_bitsLeft);
    h265e_dbg_func("enter\n");

    cabac_ctx->m_bitsLeft -= 8;
    cabac_ctx->m_low &= low_mask;

    if (leadByte == 0xff) {
        cabac_ctx->m_numBufferedBytes++;
    } else {
        RK_U32 numBufferedBytes = cabac_ctx->m_numBufferedBytes;
        if (numBufferedBytes > 0) {
            RK_U32 carry = leadByte >> 8;
            RK_U32 byteTowrite = cabac_ctx->m_bufferedByte + carry;
            mpp_writer_put_bits(s, byteTowrite, 8);
            h265e_dbg_skip("byteTowrite = %x", byteTowrite);
            byteTowrite = (0xff + carry) & 0xff;
            while (numBufferedBytes > 1) {
                h265e_dbg_skip("byteTowrite = %x", byteTowrite);
                mpp_writer_put_bits(s, byteTowrite, 8);
                numBufferedBytes--;
            }
        }
        cabac_ctx->m_numBufferedBytes = 1;
        cabac_ctx->m_bufferedByte = (uint8_t)leadByte;
    }

    h265e_dbg_func("leave\n");
}

/**
 * \brief Encode bin
 *
 * \param binValue   bin value
 * \param rcCtxModel context model
 */
void h265e_cabac_encodeBin(H265eCabacCtx *cabac_ctx, H265eContextModel_t *ctxModel, RK_U32 binValue)
{
    RK_U32 mstate = ctxModel->m_state;

    h265e_dbg_func("enter\n");

    //  mpp_log("before m_low %d ctxModel.m_state  %d m_range %d \n", cabac_ctx->m_low, ctxModel->m_state,cabac_ctx->m_range);
    ctxModel->m_state = sbacNext(mstate, binValue);

    ctxModel->bBinsCoded = 1;
    RK_U32 range = cabac_ctx->m_range;
    RK_U32 state = sbacGetState(mstate);
    RK_U32 lps = g_lpsTable[state][((uint8_t)range >> 6)];
    range -= lps;

    //  mpp_log("ctxModel.m_state  %d binValue %d \n", ctxModel->m_state, binValue);
    //  assert(lps >= 2);

    RK_S32 numBits = (RK_U32)(range - 256) >> 31;
    RK_U32 low = cabac_ctx->m_low;

    // NOTE: MPS must be LOWEST bit in mstate
    // assert(((binValue ^ mstate) & 1) == (binValue != sbacGetMps(mstate)));
    if ((binValue ^ mstate) & 1) {
        unsigned long idx;
        CLZ32(idx, lps);
        // assert(state != 63 || idx == 1);

        numBits = 8 - idx;
        if (state >= 63)
            numBits = 6;
        // assert(numBits <= 6);

        low    += range;
        range   = lps;
    }
    cabac_ctx->m_low = (low << numBits);
    cabac_ctx->m_range = (range << numBits);
    cabac_ctx->m_bitsLeft += numBits;

    //  mpp_log("after m_low %d ctxModel.m_state  %d m_range %d \n", cabac_ctx->m_low, ctxModel->m_state,cabac_ctx->m_range);
    if (cabac_ctx->m_bitsLeft >= 0) {
        h265e_cabac_writeOut(cabac_ctx);
    }

    h265e_dbg_func("leave\n");
}

void h265e_cabac_encodeBinTrm(H265eCabacCtx *cabac_ctx, RK_U32 binValue)
{
    h265e_dbg_func("enter\n");

    // mpp_log("encodeBinTrm m_range %d binValue %d \n", cabac_ctx->m_range, binValue);
    cabac_ctx->m_range -= 2;
    if (binValue) {
        cabac_ctx->m_low  += cabac_ctx->m_range;
        cabac_ctx->m_low <<= 7;
        cabac_ctx->m_range = 2 << 7;
        cabac_ctx->m_bitsLeft += 7;
    } else if (cabac_ctx->m_range >= 256) {
        return;
    } else {
        cabac_ctx->m_low   <<= 1;
        cabac_ctx->m_range <<= 1;
        cabac_ctx->m_bitsLeft++;
    }

    if (cabac_ctx->m_bitsLeft >= 0) {
        h265e_cabac_writeOut(cabac_ctx);
    }

    h265e_dbg_func("leave\n");
}

void h265e_cabac_finish(H265eCabacCtx *cabac_ctx)
{
    MppWriteCtx* s = cabac_ctx->m_bitIf;

    h265e_dbg_func("enter\n");

    if (cabac_ctx->m_low >> (21 + cabac_ctx->m_bitsLeft)) {

        mpp_writer_put_bits(s, cabac_ctx->m_bufferedByte + 1, 8);
        while (cabac_ctx->m_numBufferedBytes > 1) {
            mpp_writer_put_bits(s, 0, 8);
            cabac_ctx->m_numBufferedBytes--;
        }

        cabac_ctx->m_low -= 1 << (21 + cabac_ctx->m_bitsLeft);
    } else {
        if (cabac_ctx->m_numBufferedBytes > 0) {

            mpp_writer_put_bits(s, cabac_ctx->m_bufferedByte , 8);
        }
        while (cabac_ctx->m_numBufferedBytes > 1) {
            mpp_writer_put_bits(s, 0xff , 8);
            cabac_ctx->m_numBufferedBytes--;
        }
    }
    mpp_writer_put_bits(s, cabac_ctx->m_low >> 8 , 13 + cabac_ctx->m_bitsLeft);

    h265e_dbg_func("leave\n");
}

void h265e_cabac_flush(H265eCabacCtx *cabac_ctx)
{
    MppWriteCtx* s = cabac_ctx->m_bitIf;

    h265e_dbg_func("enter\n");

    h265e_cabac_encodeBinTrm(cabac_ctx, 1);
    h265e_cabac_finish(cabac_ctx);
    mpp_writer_put_bits(s, 1, 1);
    mpp_writer_align_zero(s);
    h265e_cabac_start(cabac_ctx);

    h265e_dbg_func("leave\n");
}
