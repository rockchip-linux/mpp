/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_AVS2D_VDPU383_H__
#define __HAL_AVS2D_VDPU383_H__

#include "mpp_hal.h"
#include "vdpu383.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET hal_avs2d_vdpu383_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_avs2d_vdpu383_deinit  (void *hal);
MPP_RET hal_avs2d_vdpu383_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_vdpu383_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_vdpu383_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_vdpu383_control (void *hal, MpiCmd cmd_type, void *param);

#ifdef  __cplusplus
}
#endif

#endif /*__HAL_AVS2D_RKV_H__*/
