/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_AV1D_COM_H
#define HAL_AV1D_COM_H

#include "rk_mpi_cmd.h"
#include "av1d_syntax.h"
#include "hal_av1d_common.h"
#include "vdpu38x_com.h"

#define NON_COEF_CDF_SIZE (434 * 16) // byte
#define COEF_CDF_SIZE (354 * 16) // byte
#define ALL_CDF_SIZE (NON_COEF_CDF_SIZE + COEF_CDF_SIZE * 4)

extern const RK_U32 g_av1d_default_prob[7400];

MPP_RET vdpu38x_av1d_deinit(void *hal);
MPP_RET vdpu38x_av1d_reset(void *hal);
MPP_RET vdpu38x_av1d_flush(void *hal);
MPP_RET vdpu38x_av1d_control(void *hal, MpiCmd cmd_type, void *param);
MPP_RET vdpu38x_av1d_uncomp_hdr(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva,
                                RK_U64 *data, RK_U32 len);
MPP_RET vdpu38x_av1d_cdf_setup(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva);
void vdpu38x_av1d_set_cdf_segid(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva,
                                RK_U32 *coef_rd_base, RK_U32 *coef_wr_base,
                                RK_U32 *segid_last_base, RK_U32 *segid_cur_base,
                                RK_U32 *noncoef_rd_base, RK_U32 *noncoef_wr_base);
MPP_RET vdpu38x_av1d_colmv_setup(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva);
void vdpu38x_av1d_rcb_setup(Av1dHalCtx *p_hal, HalTaskInfo *task,
                            DXVA_PicParams_AV1 *dxva, Vdpu38xRcbRegSet *rcb_regs,
                            Vdpu38xRcbCalc_f func);

#endif /* HAL_AV1D_COM_H */
