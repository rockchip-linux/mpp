/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_H264D_CTX_H
#define HAL_H264D_CTX_H

#include "mpp_common.h"
#include "vdpu_com.h"

typedef struct H264dVdpu3xxBuf_t {
    RK_U32              valid;
    void                *regs;
} H264dVdpu3xxBuf;

typedef struct H264dVdpu382Buf_t {
    RK_U32              valid;
    void                *regs;
    DXVA_PicEntry_H264  RefFrameList[16];
    DXVA_PicEntry_H264  RefPicList[3][32];
} H264dVdpu382Buf;

typedef struct Vdpu3xxH264dRegCtx_t {
    RK_U8               *spspps;
    RK_U8               *rps;
    RK_U8               *sclst;

    MppBuffer           bufs;
    RK_S32              bufs_fd;
    void                *bufs_ptr;
    RK_U32              offset_cabac;
    RK_U32              offset_errinfo;
    RK_U32              offset_spspps[VDPU_FAST_REG_SET_CNT];
    RK_U32              offset_rps[VDPU_FAST_REG_SET_CNT];
    RK_U32              offset_sclst[VDPU_FAST_REG_SET_CNT];

    union {
        H264dVdpu3xxBuf reg_buf[VDPU_FAST_REG_SET_CNT];
        H264dVdpu382Buf reg_382_buf[VDPU_FAST_REG_SET_CNT];
    };

    RK_U32              spspps_offset;
    RK_U32              rps_offset;
    RK_U32              sclst_offset;

    RK_S32              width;
    RK_S32              height;
    /* rcb buffers info */
    RK_U32              bit_depth;
    RK_U32              mbaff;
    RK_U32              chroma_format_idc;

    void                *rcb_ctx;
    RK_U32              rcb_buf_size;
    VdpuRcbInfo         rcb_info[RCB_BUF_CNT];
    MppBuffer           rcb_buf[VDPU_FAST_REG_SET_CNT];

    void                *regs;
    HalBufs             origin_bufs;

    RK_U32              err_ref_hack;
} Vdpu3xxH264dRegCtx;

#endif /* HAL_H264D_CTX_H */

