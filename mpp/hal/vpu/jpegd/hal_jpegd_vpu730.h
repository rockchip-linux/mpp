/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_JPEGD_VPU730_H
#define HAL_JPEGD_VPU730_H

#include "rk_type.h"

#include "mpp_hal.h"

MPP_RET hal_jpegd_vpu730_init(void *hal, MppHalCfg *cfg);
MPP_RET hal_jpegd_vpu730_deinit(void *hal);
MPP_RET hal_jpegd_vpu730_gen_regs(void *hal,  HalTaskInfo *syn);
MPP_RET hal_jpegd_vpu730_start(void *hal, HalTaskInfo *task);
MPP_RET hal_jpegd_vpu730_wait(void *hal, HalTaskInfo *task);
MPP_RET hal_jpegd_vpu730_control(void *hal, MpiCmd cmd_type, void *param);

#endif /* HAL_JPEGD_VPU730_H */