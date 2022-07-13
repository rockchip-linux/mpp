/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __AVS2D_PARSE_H__
#define __AVS2D_PARSE_H__

#include "avs2d_global.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET avs2d_reset_parser(Avs2dCtx_t *p_dec);

MPP_RET avs2d_commit_syntaxs(Avs2dSyntax_t *syntax, HalDecTask *task);
MPP_RET avs2d_fill_parameters(Avs2dCtx_t *p_dec, Avs2dSyntax_t *syntax);

MPP_RET avs2d_parse_prepare_fast(Avs2dCtx_t *p_dec, MppPacket *pkt, HalDecTask *task);
MPP_RET avs2d_parse_prepare_split(Avs2dCtx_t *p_dec, MppPacket *pkt, HalDecTask *task);
MPP_RET avs2d_parse_stream(Avs2dCtx_t *p_dec, HalDecTask *task);
RK_S32 avs2d_split_frame(void *sc, const RK_U8 **poutbuf, RK_S32 *poutbuf_size,
                         const RK_U8 *buf, RK_U32 buf_size, RK_S64 pts, RK_S64 dts);

#ifdef  __cplusplus
}
#endif

#endif /*__AVS2D_PARSE_H__*/
