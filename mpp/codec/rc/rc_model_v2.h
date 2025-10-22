/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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
#ifndef RC_MODEL_V2_H
#define RC_MODEL_V2_H

#include "mpp_rc_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET rc_model_v2_init(void *ctx, RcCfg *cfg);
MPP_RET rc_model_v2_deinit(void *ctx);

MPP_RET rc_model_v2_check_reenc(void *ctx, EncRcTask *task);

MPP_RET rc_model_v2_start(void *ctx, EncRcTask *task);
MPP_RET rc_model_v2_end(void *ctx, EncRcTask *task);

MPP_RET rc_model_v2_hal_start(void *ctx, EncRcTask *task);
MPP_RET rc_model_v2_hal_end(void *ctx, EncRcTask *task);

#ifdef  __cplusplus
}
#endif

#endif /* RC_MODEL_V2_H */
