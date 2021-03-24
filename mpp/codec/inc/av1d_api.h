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

#ifndef __AV1D_API_H__
#define __AV1D_API_H__

#include "parser_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi api_av1d_parser;

MPP_RET  av1d_init   (void *decoder, ParserCfg *cfg);
MPP_RET  av1d_deinit (void *decoder);
MPP_RET  av1d_reset  (void *decoder);
MPP_RET  av1d_flush  (void *decoder);
MPP_RET  av1d_control(void *decoder, MpiCmd cmd_type, void *param);
MPP_RET  av1d_prepare(void *decoder, MppPacket pkt, HalDecTask *task);
MPP_RET  av1d_parse  (void *decoder, HalDecTask *task);
MPP_RET  av1d_callback(void *decoder, void *info);

#ifdef  __cplusplus
}
#endif

#endif /* __AV1D_API_H__*/
