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
#ifndef __MPP_BITPUT_H__
#define __MPP_BITPUT_H__
#include <stdio.h>
#include <assert.h>
#include "rk_type.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_err.h"

typedef struct bitput_ctx_t {
    // Pointer to the start of put address
    RK_U64 *bit_buf;

    // Pointer to current wite address offset (8 Byte as uint)
    RK_S32 p_uint_index;

    // Pointer to bit alreay write in uint
    RK_S32 p_bit_offset;

    // total bit have write
    RK_S32 p_bit_len;

    // total len of the region to write
    RK_S32 total_len;
} BitputCtx_t;
#ifdef  __cplusplus
extern "C" {
#endif
RK_S32 mpp_set_bitput_ctx(BitputCtx_t *bp, RK_U64 *data, RK_U32 len);
void mpp_put_bits(BitputCtx_t *bp, RK_U64 data, RK_S32 size);
void mpp_align(BitputCtx_t *bp, RK_S32 align_width, int flag);
#ifdef  __cplusplus
}
#endif

#endif
