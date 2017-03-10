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

#ifndef __HAL_JPEGE_HDR_H__
#define __HAL_JPEGE_HDR_H__

#include "jpege_syntax.h"

typedef void *JpegeBits;

#ifdef __cplusplus
extern "C" {
#endif

void jpege_bits_init(JpegeBits *ctx);
void jpege_bits_deinit(JpegeBits ctx);

void jpege_bits_setup(JpegeBits ctx, RK_U8 *buf, RK_S32 size);
void jpege_bits_put(JpegeBits ctx, RK_U32 val, RK_S32 len);
void jpege_bits_align_byte(JpegeBits ctx);
RK_U8 *jpege_bits_get_buf(JpegeBits ctx);
RK_S32 jpege_bits_get_bitpos(JpegeBits ctx);
RK_S32 jpege_bits_get_bytepos(JpegeBits ctx);

MPP_RET write_jpeg_header(JpegeBits *bits, JpegeSyntax *syntax,
                          const RK_U8 *qtable[2]);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_JPEGE_HDR_H__*/

