/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H264D_VDPU383_H__
#define __HAL_H264D_VDPU383_H__

#include "mpp_hal.h"
#include "vdpu383.h"

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpu383_h264d_init    (void *hal, MppHalCfg *cfg);
MPP_RET vdpu383_h264d_deinit  (void *hal);
MPP_RET vdpu383_h264d_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET vdpu383_h264d_start   (void *hal, HalTaskInfo *task);
MPP_RET vdpu383_h264d_wait    (void *hal, HalTaskInfo *task);
MPP_RET vdpu383_h264d_reset   (void *hal);
MPP_RET vdpu383_h264d_flush   (void *hal);
MPP_RET vdpu383_h264d_control (void *hal, MpiCmd cmd_type, void *param);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_H264D_VDPU34X_H__ */