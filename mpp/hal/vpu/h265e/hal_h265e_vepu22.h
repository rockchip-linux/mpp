/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#include "rk_type.h"

#ifndef __HAL_H265E_VEPU22_H__
#define __HAL_H265E_VEPU22_H__

#ifdef __cplusplus
extern "C"
{
#endif

MPP_RET hal_h265e_vepu22_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_h265e_vepu22_deinit  (void *hal);
MPP_RET hal_h265e_vepu22_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_h265e_vepu22_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_h265e_vepu22_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_h265e_vepu22_reset   (void *hal);
MPP_RET hal_h265e_vepu22_flush   (void *hal);
MPP_RET hal_h265e_vepu22_control (void *hal, RK_S32 cmd_type, void *param);

#ifdef __cplusplus
}
#endif
#endif
