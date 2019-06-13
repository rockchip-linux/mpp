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

#ifndef __H264D_API_H__
#define __H264D_API_H__

#include "parser_api.h"

typedef enum mpp_decmtd_type {
    MPP_DEC_NULL     = 0,
    MPP_DEC_BY_SLICE = 0x1,
    MPP_DEC_BY_FRAME = 0x2,
    MPP_DEC_MAX,
} MppDecMtd_type;


#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi api_h264d_parser;

MPP_RET  h264d_init   (void *decoder, ParserCfg *cfg);
MPP_RET  h264d_deinit (void *decoder);
MPP_RET  h264d_reset  (void *decoder);
MPP_RET  h264d_flush  (void *decoder);
MPP_RET  h264d_control(void *decoder, RK_S32 cmd_type, void *param);
MPP_RET  h264d_prepare(void *decoder, MppPacket pkt, HalDecTask *task);
MPP_RET  h264d_parse  (void *decoder, HalDecTask *task);
MPP_RET  h264d_callback(void *decoder, void *err_info);

#ifdef  __cplusplus
}
#endif

#endif /*__H264D_API_H__*/
