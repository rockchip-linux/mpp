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

#ifndef __MPP_2STR_H__
#define __MPP_2STR_H__

#include "rk_type.h"
#include "rk_venc_rc.h"

#ifdef  __cplusplus
extern "C" {
#endif

const char *strof_ctx_type(MppCtxType type);
const char *strof_coding_type(MppCodingType coding);
const char *strof_rc_mode(MppEncRcMode rc_mode);
const char *strof_profle(MppCodingType coding, RK_U32 profile);
const char *strof_gop_mode(MppEncRcGopMode gop_mode);

#ifdef  __cplusplus
}
#endif

#endif
