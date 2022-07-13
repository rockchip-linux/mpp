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

#ifndef __AVS2D_PS_H__
#define __AVS2D_PS_H__

#include "avs2d_global.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET avs2d_parse_sequence_header(Avs2dCtx_t *p_dec);
MPP_RET avs2d_parse_picture_header(Avs2dCtx_t *p_dec, RK_U32 startcode);

#ifdef  __cplusplus
}
#endif

#endif /*__AVS2D_PS_H__*/
