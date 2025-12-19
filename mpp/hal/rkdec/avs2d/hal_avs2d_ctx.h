/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_AVS2D_CTX_H
#define HAL_AVS2D_CTX_H

#include "vdpu_com.h"

#define VDPU_TOTAL_REG_CNT        320

typedef struct Avs2dRkvBuf_t {
    RK_U32                  valid;
    RK_U32                  offset_shph;
    RK_U32                  offset_sclst;
    void                    *regs;
} Avs2dRkvBuf;

typedef struct Avs2dRkvRegCtx {
    Avs2dRkvBuf             reg_buf[VDPU_FAST_REG_SET_CNT];

    RK_U32                  shph_offset;
    RK_U32                  sclst_offset;

    void                    *regs;

    RK_U8                   *shph_dat;
    RK_U8                   *scalist_dat;

    MppBuffer               bufs;
    RK_S32                  bufs_fd;
    void                    *bufs_ptr;

    MppBuffer               rcb_buf[VDPU_FAST_REG_SET_CNT];
    void                    *rcb_ctx;
    RK_U32                  rcb_buf_size;
    VdpuRcbInfo             rcb_info[RCB_BUF_CNT];
    RK_U32                  reg_out[VDPU_TOTAL_REG_CNT];
} Avs2dRkvRegCtx;

#endif /* HAL_AVS2D_CTX_H */

