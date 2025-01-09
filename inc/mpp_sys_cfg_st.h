/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_SYS_CFG_ST_H__
#define __MPP_SYS_CFG_ST_H__

#include "rk_type.h"
#include "mpp_err.h"

#include "mpp_frame.h"

typedef struct MppSysCfgStHStrd_t {
    /* input args start */
    MppCodingType  type;
    RK_U32         fmt_fbc;
    RK_U32         width;
    RK_U32         h_stride_by_byte;

    /* output args start */
    RK_U32 h_stride_by_pixel;
} MppSysCfgStHStrd;

typedef struct MppSysCfgStHByteStrd_t {
    /* input args start */
    MppCodingType  type;
    MppFrameFormat fmt_codec;
    RK_U32         fmt_fbc;
    RK_U32         width;

    /* in/output args start */
    RK_U32 h_stride_by_byte;
} MppSysCfgStHByteStrd;

typedef struct MppSysCfgStVStrd_t {
    /* input args start */
    MppCodingType  type;
    RK_U32         fmt_fbc;
    RK_U32         height;

    /* in/output args start */
    RK_U32 v_stride;
} MppSysCfgStVStrd;

typedef struct MppSysCfgStSize_t {
    /* input args start */
    MppCodingType  type;
    MppFrameFormat fmt_codec;
    RK_U32         fmt_fbc;
    RK_U32         width;
    RK_U32         height;

    /* in/output args start */
    RK_U32 h_stride_by_byte;
    RK_U32 v_stride;

    /* output args start */
    RK_U32 h_stride_by_pixel;
    RK_U32 size_total;
    RK_U32 size_fbc_hdr;
    RK_U32 size_fbc_bdy;
} MppSysCfgStSize;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_sys_cfg_st_get_h_stride(MppSysCfgStHStrd *h_stride_cfg);
MPP_RET mpp_sys_cfg_st_get_byte_stride(MppSysCfgStHByteStrd *byte_stride_cfg);
MPP_RET mpp_sys_cfg_st_get_v_stride(MppSysCfgStVStrd *v_stride_cfg);
MPP_RET mpp_sys_cfg_st_get_size(MppSysCfgStSize *size_cfg);

#ifdef __cplusplus
}
#endif

#endif /* MPP_SYS_CFGI_H__ */
