/*
 * *
 * * Copyright 2016 Rockchip Electronics Co. LTD
 * *
 * * Licensed under the Apache License, Version 2.0 (the "License");
 * * you may not use this file except in compliance with the License.
 * * You may obtain a copy of the License at
 * *
 * *      http://www.apache.org/licenses/LICENSE-2.0
 * *
 * * Unless required by applicable law or agreed to in writing, software
 * * distributed under the License is distributed on an "AS IS" BASIS,
 * * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * * See the License for the specific language governing permissions and
 * * limitations under the License.
 * */

/*
* @file       hal_vpu_mpg4d_api.h
* @brief
* @author      gzl(lance.gao@rock-chips.com)

* @version     1.0.0
* @history
*   2016.04.11 : Create
*/

#ifndef __HAL_MPEG4D_API_H__
#define __HAL_MPEG4D_API_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_hal.h"

#define MPG4D_HAL_DBG_REG_PUT       (0x00000001)
#define MPG4D_HAL_DBG_REG_GET       (0x00000002)

#ifdef __cplusplus
extern "C" {
#endif

extern RK_U32 mpg4d_hal_debug;

extern const MppHalApi hal_api_mpg4d;

MPP_RET hal_vpu_mpg4d_init(void *hal, MppHalCfg *cfg);
MPP_RET hal_vpu_mpg4d_gen_regs(void *hal,  HalTaskInfo *syn);
MPP_RET hal_vpu_mpg4d_deinit(void *hal);
MPP_RET hal_vpu_mpg4d_start(void *hal, HalTaskInfo *task);
MPP_RET hal_vpu_mpg4d_wait(void *hal, HalTaskInfo *task);
MPP_RET hal_vpu_mpg4d_reset(void *hal);
MPP_RET hal_vpu_mpg4d_flush(void *hal);
MPP_RET hal_vpu_mpg4d_control(void *hal, RK_S32 cmd_type, void *param);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_MPEG4D_API_H__*/

