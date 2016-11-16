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

#ifndef __AVSD_API_H__
#define __AVSD_API_H__

#include "parser_api.h"


#ifdef  __cplusplus
extern "C" {
#endif
extern const ParserApi api_avsd_parser;

MPP_RET  avsd_init   (void *decoder, ParserCfg *cfg);
MPP_RET  avsd_deinit (void *decoder);
MPP_RET  avsd_reset  (void *decoder);
MPP_RET  avsd_flush  (void *decoder);
MPP_RET  avsd_control(void *decoder, RK_S32 cmd_type, void *param);
MPP_RET  avsd_prepare(void *decoder, MppPacket pkt, HalDecTask *task);
MPP_RET  avsd_parse  (void *decoder, HalDecTask *task);
MPP_RET  avsd_callback(void *decoder, void *err_info);

#ifdef  __cplusplus
}
#endif

#endif /*__AVSD_API_H__*/
