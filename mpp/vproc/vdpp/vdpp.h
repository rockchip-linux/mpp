/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPP_H
#define VDPP_H

#include "vdpp_reg.h"
#include "vdpp_common.h"

typedef enum ZME_FMT {
    FMT_YCbCr420_888 = 4,
    FMT_YCbCr444_888 = 6,
} VdppZmeFmt;

typedef enum {
    SCL_NEI = 0,
    SCL_BIL = 1,
    SCL_BIC = 2,
    SCL_MPH = 3,
} VdppZmeScaleMode;

typedef struct scl_info {
    RK_U16 act_width;
    RK_U16 dsp_width;

    RK_U16 act_height;
    RK_U16 dsp_height;

    RK_U8  dering_en;

    RK_U8  xsd_en;
    RK_U8  xsu_en;
    RK_U8  xsd_bypass;
    RK_U8  xsu_bypass;
    RK_U8  xscl_mode;
    RK_U16 xscl_factor;
    RK_U8  xscl_offset;

    RK_U8  ysd_en;
    RK_U8  ysu_en;
    RK_U8  ys_bypass;
    RK_U8  yscl_mode;
    RK_U16 yscl_factor;
    RK_U8  yscl_offset;

    RK_U8  xavg_en;
    RK_U8  xgt_en;
    RK_U8  xgt_mode;

    RK_U8  yavg_en;
    RK_U8  ygt_en;
    RK_U8  ygt_mode;

    const RK_S16 (*xscl_zme_coe)[8];
    const RK_S16 (*yscl_zme_coe)[8];

    /* used after vdpp3 */
    RK_U8 xscl_coe_idx; // [0, 9], the coef index to use in x-axis
    RK_U8 yscl_coe_idx; // [0, 9], the coef index to use in y-axis
} VdppZmeScaleInfo;

typedef struct dmsr_params {
    bool   dmsr_enable;
    RK_U32 dmsr_str_pri_y;
    RK_U32 dmsr_str_sec_y;
    RK_U32 dmsr_dumping_y;
    RK_U32 dmsr_wgt_pri_gain_even_1;
    RK_U32 dmsr_wgt_pri_gain_even_2;
    RK_U32 dmsr_wgt_pri_gain_odd_1;
    RK_U32 dmsr_wgt_pri_gain_odd_2;
    RK_U32 dmsr_wgt_sec_gain;
    RK_U32 dmsr_blk_flat_th;
    RK_U32 dmsr_contrast_to_conf_map_x0;
    RK_U32 dmsr_contrast_to_conf_map_x1;
    RK_U32 dmsr_contrast_to_conf_map_y0;
    RK_U32 dmsr_contrast_to_conf_map_y1;
    RK_U32 dmsr_diff_core_th0;
    RK_U32 dmsr_diff_core_th1;
    RK_U32 dmsr_diff_core_wgt0;
    RK_U32 dmsr_diff_core_wgt1;
    RK_U32 dmsr_diff_core_wgt2;
    RK_U32 dmsr_edge_th_low_arr[7];
    RK_U32 dmsr_edge_th_high_arr[7];
} DmsrParams;

typedef struct zme_params {
    RK_U32 zme_bypass_en;
    RK_U32 zme_dering_enable;
    RK_U32 zme_dering_sen_0;
    RK_U32 zme_dering_sen_1;
    RK_U32 zme_dering_blend_alpha;
    RK_U32 zme_dering_blend_beta;
    const RK_S16 (*zme_tap8_coeff)[17][8]; // 11x17x8
    const RK_S16 (*zme_tap6_coeff)[17][8]; // 11x17x8

    /* for scale info */
    RK_U32 src_width;
    RK_U32 src_height;
    RK_U32 dst_width;
    RK_U32 dst_height;
    RK_U32 dst_fmt;

    /* 3576(vdpp2) feature */
    RK_U32 yuv_out_diff;
    RK_U32 dst_c_width;
    RK_U32 dst_c_height;
} ZmeParams;

typedef struct vdpp_params {
    RK_U32 src_yuv_swap;
    RK_U32 dst_fmt;
    RK_U32 dst_yuv_swap;
    RK_U32 src_width;
    RK_U32 src_height;
    RK_U32 dst_width;
    RK_U32 dst_height;

    VdppAddr    src; // src frame
    VdppAddr    dst; // dst frame

    DmsrParams  dmsr_params;
    ZmeParams   zme_params;
} Vdpp1Params;

typedef struct vdpp_api_ctx {
    RK_S32 fd; // device fd, like '/dev/mpp_service'
    Vdpp1Params params;
    Vdpp1Regs reg;
} Vdpp1ApiCtx;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpp_init(VdppCtx ictx);
MPP_RET vdpp_deinit(VdppCtx ictx);
MPP_RET vdpp_control(VdppCtx ictx, VdppCmd cmd, void *iparam);
RK_S32  vdpp_check_cap(VdppCtx ictx);

#ifdef __cplusplus
}
#endif

#endif /* VDPP_H */
