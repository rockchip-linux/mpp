/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPP_H__
#define __VDPP_H__

#include "vdpp_reg.h"
#include "vdpp_common.h"

/* vdpp log marco */
#define VDPP_DBG_TRACE             (0x00000001)
#define VDPP_DBG_INT               (0x00000002)

extern RK_U32 vdpp_debug;

#define VDPP_DBG(level, fmt, ...)\
    do {\
        if (level & vdpp_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
    } while (0)

struct vdpp_params {
    RK_U32 src_yuv_swap;
    RK_U32 dst_fmt;
    RK_U32 dst_yuv_swap;
    RK_U32 src_width;
    RK_U32 src_height;
    RK_U32 dst_width;
    RK_U32 dst_height;

    struct vdpp_addr src; // src frame
    struct vdpp_addr dst; // dst frame

    struct dmsr_params dmsr_params;
    struct zme_params zme_params;
};

struct vdpp_api_ctx {
    RK_S32 fd;
    struct vdpp_params params;
    struct vdpp_reg reg;
    struct dmsr_reg dmsr;
    struct zme_reg zme;
};

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpp_init(VdppCtx *ictx);
MPP_RET vdpp_deinit(VdppCtx ictx);
MPP_RET vdpp_control(VdppCtx ictx, VdppCmd cmd, void *iparam);

#ifdef __cplusplus
}
#endif

#endif
