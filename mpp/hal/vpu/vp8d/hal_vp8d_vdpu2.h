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

#ifndef __HAL_VP8D_VDPU2_H__
#define __HAL_VP8D_VDPU2_H__
#include "hal_vp8d_base.h"

MPP_RET hal_vp8d_vdpu2_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_vp8d_vdpu2_deinit  (void *hal);
MPP_RET hal_vp8d_vdpu2_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_vp8d_vdpu2_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_vp8d_vdpu2_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_vp8d_vdpu2_reset   (void *hal);
MPP_RET hal_vp8d_vdpu2_flush   (void *hal);
MPP_RET hal_vp8d_vdpu2_control (void *hal, RK_S32 cmd_type, void *param);

#endif
