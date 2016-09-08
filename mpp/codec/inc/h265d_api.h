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

#ifndef __H265D_API_H__
#define __H265D_API_H__
#include "parser_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi api_h265d_parser;

MPP_RET h265d_prepare(void *ctx, MppPacket pkt, HalDecTask *task);
MPP_RET h265d_init(void *ctx, ParserCfg *parser_cfg);
MPP_RET h265d_parse(void *ctx, HalDecTask *task);
MPP_RET h265d_deinit(void *ctx);
MPP_RET h265d_flush(void *ctx);
MPP_RET h265d_reset(void *ctx);
MPP_RET h265d_control(void *ctx, RK_S32 cmd, void *param);
MPP_RET h265d_callback(void *ctx, void *err_info);
RK_S32 mpp_hevc_split_frame(void *sc,
                            const RK_U8 **poutbuf, RK_S32 *poutbuf_size,
                            const RK_U8 *buf, RK_S32 buf_size);

MPP_RET h265d_get_stream(void *ctx, RK_U8 **buf, RK_S32 *size); // used for compare openhevc
MPP_RET h265d_set_compare_info(void *ctx, void *info);


#ifdef  __cplusplus
}
#endif

#endif /*__H265D_API_H__*/
