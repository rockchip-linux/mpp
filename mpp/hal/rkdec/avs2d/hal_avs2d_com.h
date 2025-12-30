/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_AVS2D_COM_H
#define HAL_AVS2D_COM_H

#include "vdpu38x_com.h"

MPP_RET hal_avs2d_vdpu_deinit(void *hal);
MPP_RET hal_avs2d_vdpu38x_prepare_header(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len);
MPP_RET hal_avs2d_vdpu38x_prepare_scalist(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len);
RK_S32 hal_avs2d_get_frame_fd(Avs2dHalCtx_t *p_hal, RK_S32 idx);
RK_S32 hal_avs2d_get_packet_fd(Avs2dHalCtx_t *p_hal, RK_S32 idx);
MPP_RET hal_avs2d_set_up_colmv_buf(void *hal);
RK_U8 hal_avs2d_fetch_data(RK_U32 fmt, RK_U8 *line, RK_U32 num);
MPP_RET hal_avs2d_vdpu_dump_yuv(void *hal, HalTaskInfo *task);
void vdpu38x_avs2d_rcb_setup(void *hal, HalTaskInfo *task,
                             Vdpu38xRcbRegSet *rcb_regs, Vdpu38xRcbCalc_f func);

#endif /* HAL_AVS2D_COM_H */
