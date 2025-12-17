/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_H264D_COM_H
#define HAL_H264D_COM_H

#include "rk_mpi_cmd.h"

#define H264_CTU_SIZE      16

extern const RK_U32 rkv_cabac_table[928];

MPP_RET vdpu3xx_h264d_deinit(void *hal);
MPP_RET vdpu_h264d_reset(void *hal);
MPP_RET vdpu_h264d_flush(void *hal);
MPP_RET vdpu38x_h264d_control(void *hal, MpiCmd cmd_type, void *param);
MPP_RET vdpu38x_h264d_prepare_spspps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len);
MPP_RET vdpu38x_h264d_prepare_scanlist(H264dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len);

#endif /* HAL_H264D_COM_H */
