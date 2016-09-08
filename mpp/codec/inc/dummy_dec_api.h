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

#ifndef __DUMMY_DEC_API_H__
#define __DUMMY_DEC_API_H__

#include "parser_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi dummy_dec_parser;

MPP_RET dummy_dec_init   (void *dec, ParserCfg *cfg);
MPP_RET dummy_dec_deinit (void *dec);
MPP_RET dummy_dec_reset  (void *dec);
MPP_RET dummy_dec_flush  (void *dec);
MPP_RET dummy_dec_control(void *dec, RK_S32 cmd_type, void *param);
MPP_RET dummy_dec_prepare(void *dec, MppPacket pkt, HalDecTask *task);
MPP_RET dummy_dec_parse  (void *dec, HalDecTask *task);
MPP_RET dummy_dec_callback(void *dec, void *err_info);

#ifdef  __cplusplus
}
#endif

#endif /*__DUMMY_DEC_API_H__*/
