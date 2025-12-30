/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPU_COM_H
#define VDPU_COM_H

#include <math.h>

#include "rk_type.h"

#define VDPU_FAST_REG_SET_CNT  3

#define RCB_ALLINE_SIZE        (64)
#define MPP_RCB_BYTES(bits)    ((RK_U32)(MPP_ALIGN(((RK_U32)ceilf(bits) + 7) / 8, RCB_ALLINE_SIZE)))

#define VDPU38X_REG_MAX_REF_CNT   8

typedef enum VdpuRcbSetMode_e {
    RCB_SET_BY_SIZE_SORT_MODE,
    RCB_SET_BY_PRIORITY_MODE,
} VdpuRcbSetMode;

/* IN/ON_TILE */
typedef enum Vdpu38xRcbType_e {
    RCB_STRMD_IN_ROW,
    RCB_STRMD_ON_ROW,
    RCB_INTER_IN_ROW,
    RCB_INTER_ON_ROW,
    RCB_INTRA_IN_ROW,
    RCB_INTRA_ON_ROW,
    RCB_FLTD_IN_ROW,
    RCB_FLTD_PROT_IN_ROW,
    RCB_FLTD_ON_ROW,
    RCB_FLTD_ON_COL,
    RCB_FLTD_UPSC_ON_COL,
    RCB_BUF_CNT,
} Vdpu38xRcbType;

typedef struct VdpuRcbInfo_t {
    RK_U32              reg_idx;
    RK_S32              size;
    RK_S32              offset;
} VdpuRcbInfo;

RK_S32 vdpu_compare_rcb_size(const void *a, const void *b);

#endif /* VDPU_COM_H */
