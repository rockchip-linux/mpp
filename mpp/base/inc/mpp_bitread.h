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
#ifndef __MPP_BITREAD_H__
#define __MPP_BITREAD_H__

#include <stdio.h>
#include <assert.h>

#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_err.h"

#define   __BITREAD_ERR   __bitread_error

#define READ_ONEBIT(bitctx, out)\
    do {\
        RK_S32 _out; \
        bitctx->ret = mpp_read_bits(bitctx, 1, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

#define READ_BITS(bitctx, num_bits, out)\
    do {\
        RK_S32 _out; \
        bitctx->ret = mpp_read_bits(bitctx, num_bits, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

#define READ_BITS_LONG(bitctx, num_bits, out)\
    do {\
        RK_U32 _out; \
        bitctx->ret = mpp_read_longbits(bitctx, num_bits, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

#define SHOW_BITS(bitctx, num_bits, out)\
    do {\
        RK_S32 _out; \
        bitctx->ret = mpp_show_bits(bitctx, num_bits, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

#define SHOW_BITS_LONG(bitctx, num_bits, out)\
    do {\
        RK_U32 _out; \
        bitctx->ret = mpp_show_longbits(bitctx, num_bits, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

#define SKIP_BITS(bitctx, num_bits)\
    do {\
        bitctx->ret = mpp_skip_bits(bitctx, num_bits); \
        if (bitctx->ret) { goto __BITREAD_ERR; }\
    } while (0)

#define SKIP_BITS_LONG(bitctx, num_bits)\
    do {\
        bitctx->ret = mpp_skip_longbits(bitctx, num_bits); \
        if (bitctx->ret) { goto __BITREAD_ERR; }\
    } while (0)

#define READ_UE(bitctx, out)\
    do {\
        RK_U32 _out; \
        bitctx->ret = mpp_read_ue(bitctx, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

#define READ_SE(bitctx, out)\
    do {\
        RK_S32 _out; \
        bitctx->ret = mpp_read_se(bitctx, &_out); \
        if (!bitctx->ret) { *out = _out; }\
        else { goto __BITREAD_ERR; }\
    } while (0)

typedef struct bitread_ctx_t {
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
    // Number of emulation presentation bytes (0x000003) we met.
    RK_S64 emulation_prevention_bytes_;
    // count PPS SPS SEI read bits
    RK_S32 used_bits;
    RK_U8  *buf;
    RK_S32 buf_len;
    // ctx
    MPP_RET   ret;
    RK_S32    need_prevention_detection;
} BitReadCtx_t;


#ifdef  __cplusplus
extern "C" {
#endif

//!< set bit read context
void    mpp_set_bitread_ctx(BitReadCtx_t *bitctx, RK_U8 *data, RK_S32 size);

//!< Read bits (1-31)
MPP_RET mpp_read_bits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_S32 *out);

//!< Read bits (1-32)
MPP_RET mpp_read_longbits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_U32 *out);

//!< Show bits (1-31)
MPP_RET mpp_show_bits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_S32 *out);

//!< Show bits (1-32)
MPP_RET mpp_show_longbits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_U32 *out);

//!< skip bits(1-31)
MPP_RET mpp_skip_bits(BitReadCtx_t *bitctx, RK_S32 num_bits);

//!< skip bits(1-32)
MPP_RET mpp_skip_longbits(BitReadCtx_t *bitctx, RK_S32 num_bits);

//!< read ue(1-32)
MPP_RET mpp_read_ue(BitReadCtx_t *bitctx, RK_U32* val);

//!< read se(1-31)
MPP_RET mpp_read_se(BitReadCtx_t *bitctx, RK_S32* val);

//!< set whether detect 0x03 (used in h264 and h265)
void    mpp_set_pre_detection(BitReadCtx_t *bitctx);

//!< check whether has more rbsp data(used in h264)
RK_U32  mpp_has_more_rbsp_data(BitReadCtx_t * bitctx);

//!< align bits and get current pointer
RK_U8  *mpp_align_get_bits(BitReadCtx_t *bitctx);

#ifdef  __cplusplus
}
#endif


#endif /* __MPP_BITREAD_H__ */
