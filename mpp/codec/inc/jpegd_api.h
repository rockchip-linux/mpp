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

#ifndef __JPEGD_API_H__
#define __JPEGD_API_H__
#include "parser_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi api_jpegd_parser;


MPP_RET jpegd_prepare(void *ctx, MppPacket pkt, HalDecTask *task);
MPP_RET jpegd_init(void *ctx, ParserCfg *parser_cfg);
MPP_RET jpegd_parse(void *ctx, HalDecTask *task);
MPP_RET jpegd_deinit(void *ctx);
MPP_RET jpegd_flush(void *ctx);
MPP_RET jpegd_reset(void *ctx);
MPP_RET jpegd_control(void *ctx, RK_S32 cmd, void *param);
MPP_RET jpegd_callback(void *ctx, void *err_info);

#ifdef  __cplusplus
}
#endif

#endif /*__JPEGD_API_H__*/
