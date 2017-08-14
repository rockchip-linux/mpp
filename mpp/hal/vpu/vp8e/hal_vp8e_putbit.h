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

#ifndef __HAL_VP8E_PUT_BIT_H__
#define __HAL_VP8E_PUT_BIT_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef struct {
    RK_U8 *data;
    RK_U8 *p_data;
    RK_S32 size;
    RK_S32 byte_cnt;

    RK_S32 range;
    RK_S32 bottom;
    RK_S32 bits_left;
} Vp8ePutBitBuf;

typedef struct {
    RK_S32 value;
    RK_S32 number;
    RK_S32 index[9];
} Vp8eTree;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vp8e_put_lit(Vp8ePutBitBuf *bitbuf, RK_S32 value, RK_S32 number);
MPP_RET vp8e_buffer_gap(Vp8ePutBitBuf *bitbuf, RK_S32 gap);
MPP_RET vp8e_buffer_overflow(Vp8ePutBitBuf *bitbuf);
MPP_RET vp8e_put_byte(Vp8ePutBitBuf *bitbuf, RK_S32 byte);
MPP_RET vp8e_put_bool(Vp8ePutBitBuf *bitbuf, RK_S32 prob, RK_S32 boolValue);
MPP_RET vp8e_set_buffer(Vp8ePutBitBuf *bitbuf, RK_U8 *data, RK_S32 size);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_VP8E_PUTBIT_H__*/
