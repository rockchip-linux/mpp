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

#ifndef __H264D_BITREAD_H__
#define __H264D_BITREAD_H__
#include <stdio.h>
#include <assert.h>
#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_log.h"


#define WRITE_LOG(bitctx, name)\
    do {\
    LogInfo(bitctx->ctx, "%s", name);\
    } while (0)

#define SKIP_BITS(bitctx, num_bits)\
    do {\
    RK_S32 _out;\
    MPP_RET ret = MPP_NOK;\
    ret = read_bits(bitctx, num_bits, &_out);\
    LogInfo(bitctx->ctx, "%48s = %10d", "skip", _out);\
    if (ret) { ASSERT(0); goto __FAILED; }\
    } while (0)


#define READ_BITS(bitctx, num_bits, out, name)\
    do {\
    RK_S32 _out;\
    MPP_RET ret = MPP_NOK;\
    ret = read_bits(bitctx, num_bits, &_out);\
    LogInfo(bitctx->ctx, "%48s = %10d", name, _out);\
    if (ret) { ASSERT(0); goto __FAILED; }\
    else     { *out = _out; }\
    } while (0)


#define READ_ONEBIT(bitctx, out, name)\
    do {\
    RK_S32 _out;\
    MPP_RET ret = MPP_NOK;\
    ret = read_bits(bitctx, 1, &_out);\
    LogInfo(bitctx->ctx, "%48s = %10d", name, _out);\
    if (ret) { ASSERT(0); goto __FAILED; }\
    else     { *out = _out; }\
    } while (0)


#define READ_UE(bitctx, out, name)\
    do {\
    RK_U32 _out;\
    MPP_RET ret = MPP_NOK;\
    ret = read_ue(bitctx, &_out);\
    LogInfo(bitctx->ctx, "%48s = %10d", name, _out);\
    if (ret) { ASSERT(0); goto __FAILED; }\
    else     { *out = _out; }\
    } while (0)


#define READ_SE(bitctx, out, name)\
    do {\
    RK_S32 _out;\
    MPP_RET ret = MPP_NOK;\
    ret = read_se(bitctx, &_out);\
    LogInfo(bitctx->ctx, "%48s = %10d", name, _out);\
    if (ret) { ASSERT(0); goto __FAILED; }\
    else     { *out = _out; }\
    } while (0)




typedef struct getbit_ctx_t {
    // Pointer to the next unread (not in curr_byte_) byte in the stream.
    RK_U8 *data_;
    // Bytes left in the stream (without the curr_byte_).
    RK_U32 bytes_left_;
    // Contents of the current byte; first unread bit starting at position
    // 8 - num_remaining_bits_in_curr_byte_ from MSB.
    RK_S64 curr_byte_;
    // Number of bits remaining in curr_byte_
    RK_S32 num_remaining_bits_in_curr_byte_;
    // Used in emulation prevention three byte detection (see spec).
    // Initially set to 0xffff to accept all initial two-byte sequences.
    RK_S64 prev_two_bytes_;
    // Number of emulation preventation bytes (0x000003) we met.
    RK_S64 emulation_prevention_bytes_;
    // file to debug
    LogCtx_t *ctx;
    // count PPS SPS SEI read bits
    RK_S32 used_bits;
    RK_U8  *buf;
    RK_S32 buf_len;
} BitReadCtx_t;


#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET   read_bits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_S32 *out);
MPP_RET   read_one_bit(BitReadCtx_t *bitctx, RK_S32 *out);
MPP_RET   read_ue(BitReadCtx_t *bitctx, RK_U32* val);
MPP_RET   read_se(BitReadCtx_t *bitctx, RK_S32* val);
void      set_bitread_ctx(BitReadCtx_t *bitctx, RK_U8 *data, RK_S32 size);
RET_tpye  has_more_rbsp_data(BitReadCtx_t * bitctx);
void      set_bitread_logctx(BitReadCtx_t *bitctx, LogCtx_t *p_ctx);

#ifdef  __cplusplus
}
#endif


#endif /* __H264D_BITREAD_H__ */
