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

#ifndef __H264E_API_H__
#define __H264E_API_H__

#include "encoder_codec_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern const ControlApi api_h264e_controller;

MPP_RET h264e_init(void *ctx, ControllerCfg *ctrlCfg);
MPP_RET h264e_deinit(void *ctx);
MPP_RET h264e_encode(void *ctx, HalEncTask *task);
MPP_RET h264e_reset(void *ctx);
MPP_RET h264e_flush(void *ctx);
MPP_RET h264e_config(void *ctx, RK_S32 cmd, void *param);
MPP_RET h264e_callback(void *ctx, void *feedback);

#ifdef  __cplusplus
}
#endif

#endif /*__H264E_API_H__*/
