/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPP2_H__
#define __VDPP2_H__

#include "vdpp2_reg.h"
#include "vdpp_common.h"

/* vdpp log marco */
#define VDPP2_DBG_TRACE             (0x00000001)
#define VDPP2_DBG_INT               (0x00000002)
#define VDPP2_DBG_CHECK             (0x00000004)

extern RK_U32 vdpp2_debug;

#define VDPP2_DBG(level, fmt, ...)\
do {\
    if (level & vdpp2_debug)\
    { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

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

typedef struct EsParams_t {
    RK_U32 es_bEnabledES;
    RK_U32 es_iAngleDelta;
    RK_U32 es_iAngleDeltaExtra;
    RK_U32 es_iGradNoDirTh;
    RK_U32 es_iGradFlatTh;
    RK_U32 es_iWgtGain;
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

struct vdpp2_params {
    RK_U32 src_fmt;
    RK_U32 src_yuv_swap;
    RK_U32 dst_fmt;
    RK_U32 dst_yuv_swap;
    RK_U32 src_width;
    RK_U32 src_height;
    RK_U32 src_width_vir;
    RK_U32 src_height_vir;
    RK_U32 dst_width;
    RK_U32 dst_height;
    RK_U32 dst_width_vir;
    RK_U32 dst_height_vir;
    RK_U32 yuv_out_diff;
    RK_U32 dst_c_width;
    RK_U32 dst_c_height;
    RK_U32 dst_c_width_vir;
    RK_U32 dst_c_height_vir;
    RK_U32 working_mode;    // 2 - VDPP, 3 - DCI HIST

    struct vdpp_addr src;   // src frame
    struct vdpp_addr dst;   // dst frame
    struct vdpp_addr dst_c; // dst chroma

    RK_S32 hist;            // dci hist fd

    struct dmsr_params dmsr_params;
    struct zme_params zme_params;
    /* vdpp2 new feature */
    EsParams   es_params;
    ShpParams  shp_params;

    RK_U32 hist_cnt_en;
    RK_U32 dci_hsd_mode;
    RK_U32 dci_vsd_mode;
    RK_U32 dci_yrgb_gather_num;
    RK_U32 dci_yrgb_gather_en;
    RK_S32 dci_format;
    RK_S32 dci_alpha_swap;
    RK_S32 dci_rbuv_swap;
    RK_S32 dci_csc_range;
};

struct vdpp2_api_ctx {
    RK_S32 fd;
    struct vdpp2_params params;
    struct vdpp2_reg reg;
    struct dmsr_reg dmsr;
    struct zme_reg zme;
};

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpp2_init(VdppCtx *ictx);
MPP_RET vdpp2_deinit(VdppCtx ictx);
MPP_RET vdpp2_control(VdppCtx ictx, VdppCmd cmd, void *iparam);
RK_S32  vdpp2_check_cap(VdppCtx ictx);

#ifdef __cplusplus
}
#endif

#endif
