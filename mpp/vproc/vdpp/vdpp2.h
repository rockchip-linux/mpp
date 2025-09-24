/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPP2_H
#define VDPP2_H

#include "vdpp2_reg.h"
#include "vdpp.h"

typedef enum VdppDciFmt_e {
    VDPP_DCI_FMT_RGB888,        // 0, MSB order: [23:0]=[R8:G8:B8]
    VDPP_DCI_FMT_ARGB8888,      // 1, MSB order: [31:0]=[A8:R8:G8:B8]
    VDPP_DCI_FMT_Y_8bit_SP = 4, // 4
    VDPP_DCI_FMT_Y_10bit_SP,    // 5, [15:0]=[X6:Y10]
    VDPP_DCI_FMT_BUTT,
} VdppDciFmt; // for reg-sw_dci_data_format

typedef struct EsParams_t {
    RK_U32 es_bEnabledES;
    RK_U32 es_iAngleDelta;
    RK_U32 es_iAngleDeltaExtra;
    RK_U32 es_iGradNoDirTh;
    RK_U32 es_iGradFlatTh;
    RK_U32 es_iWgtGain; // should not be 0
    RK_U32 es_iWgtDecay;
    RK_U32 es_iLowConfTh;
    RK_U32 es_iLowConfRatio;
    RK_U32 es_iConfCntTh;
    RK_U32 es_iWgtLocalTh;
    RK_U32 es_iK1;
    RK_U32 es_iK2;
    RK_U32 es_iDeltaLimit;
    RK_U32 es_iDiff2conf_lut_x[9];
    RK_U32 es_iDiff2conf_lut_y[9];
    RK_U32 es_bEndpointCheckEnable;
    RK_U32 es_tan_hi_th;
    RK_U32 es_tan_lo_th;
} EsParams;

typedef struct ShpParams_t {
    RK_S32 sharp_enable;
    RK_S32 sharp_coloradj_bypass_en;

    RK_S32 lti_h_enable;
    RK_S32 lti_h_radius;
    RK_S32 lti_h_slope;
    RK_S32 lti_h_thresold;
    RK_S32 lti_h_gain;
    RK_S32 lti_h_noise_thr_pos;
    RK_S32 lti_h_noise_thr_neg;

    RK_S32 lti_v_enable;
    RK_S32 lti_v_radius;
    RK_S32 lti_v_slope;
    RK_S32 lti_v_thresold;
    RK_S32 lti_v_gain;
    RK_S32 lti_v_noise_thr_pos;
    RK_S32 lti_v_noise_thr_neg;

    RK_S32 cti_h_enable;
    RK_S32 cti_h_radius;
    RK_S32 cti_h_slope;
    RK_S32 cti_h_thresold;
    RK_S32 cti_h_gain;
    RK_S32 cti_h_noise_thr_pos;
    RK_S32 cti_h_noise_thr_neg;

    RK_S32 peaking_enable;
    RK_S32 peaking_gain;

    RK_S32 peaking_coring_enable;
    RK_S32 peaking_coring_zero[8];
    RK_S32 peaking_coring_thr[8];
    RK_S32 peaking_coring_ratio[8];

    RK_S32 peaking_gain_enable;
    RK_S32 peaking_gain_pos[8];
    RK_S32 peaking_gain_neg[8];

    RK_S32 peaking_limit_ctrl_enable;
    RK_S32 peaking_limit_ctrl_pos0[8];
    RK_S32 peaking_limit_ctrl_pos1[8];
    RK_S32 peaking_limit_ctrl_neg0[8];
    RK_S32 peaking_limit_ctrl_neg1[8];
    RK_S32 peaking_limit_ctrl_ratio[8];
    RK_S32 peaking_limit_ctrl_bnd_pos[8];
    RK_S32 peaking_limit_ctrl_bnd_neg[8];

    RK_S32 peaking_edge_ctrl_enable;
    RK_S32 peaking_edge_ctrl_non_dir_thr;
    RK_S32 peaking_edge_ctrl_dir_cmp_ratio;
    RK_S32 peaking_edge_ctrl_non_dir_wgt_offset;
    RK_S32 peaking_edge_ctrl_non_dir_wgt_ratio;
    RK_S32 peaking_edge_ctrl_dir_cnt_thr;
    RK_S32 peaking_edge_ctrl_dir_cnt_avg;
    RK_S32 peaking_edge_ctrl_dir_cnt_offset;
    RK_S32 peaking_edge_ctrl_diag_dir_thr;
    RK_S32 peaking_edge_ctrl_diag_adj_gain_tab[8];

    RK_S32 peaking_estc_enable;
    RK_S32 peaking_estc_delta_offset_h;
    RK_S32 peaking_estc_alpha_over_h;
    RK_S32 peaking_estc_alpha_under_h;
    RK_S32 peaking_estc_alpha_over_unlimit_h;
    RK_S32 peaking_estc_alpha_under_unlimit_h;
    RK_S32 peaking_estc_delta_offset_v;
    RK_S32 peaking_estc_alpha_over_v;
    RK_S32 peaking_estc_alpha_under_v;
    RK_S32 peaking_estc_alpha_over_unlimit_v;
    RK_S32 peaking_estc_alpha_under_unlimit_v;
    RK_S32 peaking_estc_delta_offset_d0;
    RK_S32 peaking_estc_alpha_over_d0;
    RK_S32 peaking_estc_alpha_under_d0;
    RK_S32 peaking_estc_alpha_over_unlimit_d0;
    RK_S32 peaking_estc_alpha_under_unlimit_d0;
    RK_S32 peaking_estc_delta_offset_d1;
    RK_S32 peaking_estc_alpha_over_d1;
    RK_S32 peaking_estc_alpha_under_d1;
    RK_S32 peaking_estc_alpha_over_unlimit_d1;
    RK_S32 peaking_estc_alpha_under_unlimit_d1;
    RK_S32 peaking_estc_delta_offset_non;
    RK_S32 peaking_estc_alpha_over_non;
    RK_S32 peaking_estc_alpha_under_non;
    RK_S32 peaking_estc_alpha_over_unlimit_non;
    RK_S32 peaking_estc_alpha_under_unlimit_non;
    RK_S32 peaking_filter_cfg_diag_enh_coef;

    RK_S32 peaking_filt_core_H0[6];
    RK_S32 peaking_filt_core_H1[6];
    RK_S32 peaking_filt_core_H2[6];
    RK_S32 peaking_filt_core_H3[6];
    RK_S32 peaking_filt_core_V0[3];
    RK_S32 peaking_filt_core_V1[3];
    RK_S32 peaking_filt_core_V2[3];
    RK_S32 peaking_filt_core_USM[3];

    RK_S32 shootctrl_enable;
    RK_S32 shootctrl_filter_radius;
    RK_S32 shootctrl_delta_offset;
    RK_S32 shootctrl_alpha_over;
    RK_S32 shootctrl_alpha_under;
    RK_S32 shootctrl_alpha_over_unlimit;
    RK_S32 shootctrl_alpha_under_unlimit;

    RK_S32 global_gain_enable;
    RK_S32 global_gain_lum_mode;
    RK_S32 global_gain_lum_grd[6];
    RK_S32 global_gain_lum_val[6];
    RK_S32 global_gain_adp_grd[6];
    RK_S32 global_gain_adp_val[6];
    RK_S32 global_gain_var_grd[6];
    RK_S32 global_gain_var_val[6];

    RK_S32 color_ctrl_enable;

    RK_S32 color_ctrl_p0_scaling_coef;
    RK_S32 color_ctrl_p0_point_u;
    RK_S32 color_ctrl_p0_point_v;
    RK_S32 color_ctrl_p0_roll_tab[16];

    RK_S32 color_ctrl_p1_scaling_coef;
    RK_S32 color_ctrl_p1_point_u;
    RK_S32 color_ctrl_p1_point_v;
    RK_S32 color_ctrl_p1_roll_tab[16];

    RK_S32 color_ctrl_p2_scaling_coef;
    RK_S32 color_ctrl_p2_point_u;
    RK_S32 color_ctrl_p2_point_v;
    RK_S32 color_ctrl_p2_roll_tab[16];

    RK_S32 color_ctrl_p3_scaling_coef;
    RK_S32 color_ctrl_p3_point_u;
    RK_S32 color_ctrl_p3_point_v;
    RK_S32 color_ctrl_p3_roll_tab[16];

    RK_S32 tex_adj_enable;
    RK_S32 tex_adj_y_mode_select;
    RK_S32 tex_adj_mode_select;
    RK_S32 tex_adj_grd[6];
    RK_S32 tex_adj_val[6];
} ShpParams;

typedef struct hist_params {
    RK_U32 hist_cnt_en;         // [0, 1], default: 0
    RK_U32 dci_hsd_mode;        // [0, 1], default: 0
    RK_U32 dci_vsd_mode;        // [0, 2], default: 0
    RK_U32 dci_yrgb_gather_num; // DCI AXI bus yrgb data gather transfer number {0:2, 1:4, 2:8}
    RK_U32 dci_yrgb_gather_en;  // default: 0
    RK_S32 dci_format;          // see VdppDciFmt: {0: rgb24, 1: argb32, 4: yuvsp-8bit, 5: yuvsp-10bit}
    RK_S32 dci_alpha_swap;      // [0, 1], default: 0
    RK_S32 dci_rbuv_swap;       // [0, 1], default: 0
    RK_S32 dci_csc_range;       // {0-limited, 1-full}, default: 0
    RK_S32 hist_addr;           // dci hist buffer fd

    /* for register checking */
    RK_U32 src_fmt;             // see MppFrameFormat, {0-nv12, 15-nv24}
    RK_U32 src_width;           // real width,  unit: pixel
    RK_U32 src_height;          // real height, unit: pixel
    RK_U32 src_width_vir;       // aligned width,  unit: pixel
    RK_U32 src_height_vir;      // aligned height, unit: pixel
    RK_U32 src_addr;            // src buffer fd
    RK_U32 working_mode;        // 1-IEP2, 2-VEP, 3-DCI_HIST
} HistParams;

typedef struct vdpp2_params {
    RK_U32 src_fmt;          // see MppFrameFormat, {0-nv12, 15-nv24}
    RK_U32 src_yuv_swap;     // {0-no_swap(UV), 1-swap(VU)}
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
} Vdpp2Params;

typedef struct vdpp2_api_ctx {
    RK_S32 fd;
    Vdpp2Params params;
    Vdpp2Regs reg;
} Vdpp2ApiCtx;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpp2_init(VdppCtx ictx);
MPP_RET vdpp2_deinit(VdppCtx ictx);
MPP_RET vdpp2_control(VdppCtx ictx, VdppCmd cmd, void *iparam);
RK_S32  vdpp2_check_cap(VdppCtx ictx);

#ifdef __cplusplus
}
#endif

#endif /* VDPP2_H */