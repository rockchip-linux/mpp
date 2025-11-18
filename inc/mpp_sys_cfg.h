/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_SYS_CFG_H
#define MPP_SYS_CFG_H

#include "mpp_frame.h"
#include "mpp_list.h"

typedef enum MppSysDecBufCkhCfgChange_e {
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_ENABLE           = MPP_BIT(0),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_TYPE             = MPP_BIT(1),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FMT_CODEC        = MPP_BIT(2),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FMT_FBC          = MPP_BIT(3),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FMT_HDR          = MPP_BIT(4),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_WIDTH            = MPP_BIT(5),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_HEIGHT           = MPP_BIT(6),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_TOP         = MPP_BIT(7),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_BOTTOM      = MPP_BIT(8),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_LEFT        = MPP_BIT(9),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_RIGHT       = MPP_BIT(10),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FLAG_METADATA    = MPP_BIT(11),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FLAG_THUMBNAIL   = MPP_BIT(12),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_H_STRIDE_BYTE    = MPP_BIT(13),
    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_V_STRIDE         = MPP_BIT(14),

    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_ALL              = (0xFFFFFFFFU),
} MppSysDecBufCkhChange;

typedef struct MppSysBaseCfg_t {
    RK_U64 change;

    RK_U32 enable;

    /* input args start */
    MppCodingType type;
    MppFrameFormat fmt_codec;
    RK_U32 fmt_fbc;
    RK_U32 fmt_hdr;

    /* video codec width and height */
    RK_U32 width;
    RK_U32 height;

    /* display crop info */
    RK_U32 crop_top;
    RK_U32 crop_bottom;
    RK_U32 crop_left;
    RK_U32 crop_right;

    /* bit mask for metadata and thumbnail config */
    RK_U32 has_metadata;
    RK_U32 has_thumbnail;

    /* extra protocol config */
    /* H.265 ctu size, VP9/Av1 super block size */
    RK_U32 unit_size;

    /* output args start */
    /* system support capability */
    RK_U32 cap_fbc;
    RK_U32 cap_tile;

    /* 2 horizontal stride for 2 planes like Y/UV */
    RK_U32 h_stride_by_pixel;
    RK_U32 h_stride_by_byte;
    RK_U32 v_stride;
    RK_U32 buf_total_size;

    /* fbc display offset config for some fbc version */
    RK_U32 offset_y;
    RK_U32 size_total;
    RK_U32 size_fbc_hdr;
    RK_U32 size_fbc_bdy;

    /* extra buffer size */
    RK_U32 size_metadata;
    RK_U32 size_thumbnail;
} MppSysDecBufChkCfg;

typedef struct MppSysCfgSet_t {
    RK_U32 change;
    MppSysDecBufChkCfg dec_buf_chk;
} MppSysCfgSet;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_sys_dec_buf_chk_proc(MppSysDecBufChkCfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* MPP_SYS_CFG_H */
