/*
 *
 * copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_JPEGE_VEPU2_H
#define __HAL_JPEGE_VEPU2_H
#include "rk_type.h"

MPP_RET hal_jpege_vepu2_init(void *hal, MppHalCfg *cfg);
MPP_RET hal_jpege_vepu2_deinit(void *hal);
MPP_RET hal_jpege_vepu2_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_jpege_vepu2_start(void *hal, HalTaskInfo *task);
MPP_RET hal_jpege_vepu2_wait(void *hal, HalTaskInfo *task);
MPP_RET hal_jpege_vepu2_reset(void *hal);
MPP_RET hal_jpege_vepu2_flush(void *hal);
MPP_RET hal_jpege_vepu2_control(void *hal, RK_S32 cmd, void *param);

#endif
