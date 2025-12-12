/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpu_comm"

#include "vdpu_com.h"

RK_S32 vdpu_compare_rcb_size(const void *a, const void *b)
{
    VdpuRcbInfo *p0 = (VdpuRcbInfo *)a;
    VdpuRcbInfo *p1 = (VdpuRcbInfo *)b;

    return (p0->size > p1->size) ? -1 : 1;
}
