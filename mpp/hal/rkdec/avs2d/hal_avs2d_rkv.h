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

#ifndef __HAL_AVS2D_RKV_H__
#define __HAL_AVS2D_RKV_H__

#include "mpp_device.h"

#include "parser_api.h"
#include "hal_avs2d_api.h"
#include "hal_avs2d_global.h"
#include "avs2d_syntax.h"
#include "vdpu34x.h"

#define AVS2D_REGISTERS     (278)

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET hal_avs2d_rkv_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_avs2d_rkv_deinit  (void *hal);
MPP_RET hal_avs2d_rkv_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_rkv_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_rkv_wait    (void *hal, HalTaskInfo *task);

#ifdef  __cplusplus
}
#endif

#endif /*__HAL_AVS2D_RKV_H__*/
