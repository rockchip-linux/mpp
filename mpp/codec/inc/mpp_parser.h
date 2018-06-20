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

#ifndef __MPP_PARSER_H__
#define __MPP_PARSER_H__

#include "parser_api.h"

typedef void* Parser;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_parser_init(Parser *prs, ParserCfg *cfg);
MPP_RET mpp_parser_deinit(Parser prs);

MPP_RET mpp_parser_prepare(Parser prs, MppPacket pkt, HalDecTask *task);
MPP_RET mpp_parser_parse(Parser prs, HalDecTask *task);

MPP_RET mpp_parser_reset(Parser prs);
MPP_RET mpp_parser_flush(Parser prs);
MPP_RET mpp_parser_control(Parser prs, RK_S32 cmd, void *para);
MPP_RET mpp_hal_callback(void* prs, void *err_info);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PARSER_H__*/
