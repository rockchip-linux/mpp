/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPP3_H
#define VDPP3_H

#include "vdpp3_reg.h"
#include "vdpp2.h"

typedef struct PyrParams_t {
    RK_U32  pyr_en;
    RK_U32  nb_layers;                           // [1, 3], default: 3
    VdppImg layer_imgs[VDPP_PYR_MAX_LAYERS];     // fd buffer of layer 1
} PyrParams;

typedef struct BbdParams_t {
    RK_U32 bbd_en;          //[i] support VEP & DCI_HIST path
    RK_U32 bbd_det_blcklmt; //[i] luma threshold for black bar detection, always considered as full-range
    RK_U32 bbd_size_top;    //[o] output black bar size on top
    RK_U32 bbd_size_bottom; //[o] output black bar size on bottom
    RK_U32 bbd_size_left;   //[o] output black bar size on left
    RK_U32 bbd_size_right;  //[o] output black bar size on right
} BbdParams;

typedef struct vdpp3_params {
    RK_U32 src_fmt;          // see MppFrameFormat, {0-nv12, 15-nv24}
    RK_U32 src_yuv_swap;     // {0-no_swap(UV), 1-swap(VU)}
    RK_U32 src_range;        // {0-limited, 1-full}
    RK_U32 src_width;        // real width,  unit: pixel
    RK_U32 src_height;       // real height, unit: pixel
    RK_U32 src_width_vir;    // aligned width,  unit: pixel
    RK_U32 src_height_vir;   // aligned height, unit: pixel
    RK_U32 dst_fmt;          // see VDPP_FMT, {0-yuv444sp, 3-yuv420sp}
    RK_U32 dst_yuv_swap;     // {0-no_swap(UV), 1-swap(VU)}
    RK_U32 dst_width;        // unit: pixel
    RK_U32 dst_height;       // unit: pixel
    RK_U32 dst_width_vir;    // unit: pixel
    RK_U32 dst_height_vir;   // unit: pixel
    RK_U32 yuv_out_diff;     // {0:single_buf_out, 1-two_separate_bufs_out}
    RK_U32 dst_c_width;      // unit: pixel, valid when yuv_out_diff=1
    RK_U32 dst_c_height;     // unit: pixel, valid when yuv_out_diff=1
    RK_U32 dst_c_width_vir;  // unit: pixel, valid when yuv_out_diff=1
    RK_U32 dst_c_height_vir; // unit: pixel, valid when yuv_out_diff=1
    RK_U32 working_mode;     // 1-IEP2, 2-VEP, 3-DCI_HIST

    VdppAddr    src;         // src frame
    VdppAddr    dst;         // dst frame
    VdppAddr    dst_c;       // dst chroma frame, valid when yuv_out_diff=1

    /* vdpp features */
    DmsrParams  dmsr_params;
    ZmeParams   zme_params;

    /* vdpp2 features */
    HistParams  hist_params;
    EsParams    es_params;
    ShpParams   shp_params;

    /* vdpp3 features */
    PyrParams   pyr_params;
    BbdParams   bbd_params;
} Vdpp3Params;

typedef struct vdpp3_api_ctx {
    RK_S32 fd;
    Vdpp3Params params;
    Vdpp3Regs reg;
} Vdpp3ApiCtx;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpp3_init(VdppCtx ictx);
MPP_RET vdpp3_deinit(VdppCtx ictx);
MPP_RET vdpp3_control(VdppCtx ictx, VdppCmd cmd, void *iparam);
RK_S32  vdpp3_check_cap(VdppCtx ictx);

#ifdef __cplusplus
}
#endif

#endif /* VDPP3_H */
