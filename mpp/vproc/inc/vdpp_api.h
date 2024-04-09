/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPP_API_H__
#define __VDPP_API_H__

#include <stdbool.h>

#include "mpp_debug.h"
#include "mpp_frame.h"

#define CEIL(a)                 (int)( (double)(a) > (int)(a) ? (int)((a)+1) : (int)(a) )
#define FLOOR(a)                (int)( (double)(a) < (int)(a) ? (int)((a)-1) : (int)(a) )
#define ROUND(a)                (int)( (a) > 0 ? ((double) (a) + 0.5) : ((double) (a) - 0.5) )

#define RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_BITS    (8)
#define RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_NUMS    (1 << (RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_BITS))
#define RKVOP_PQ_PREPROCESS_HIST_BITS_VERI          (4)
#define RKVOP_PQ_PREPROCESS_HIST_BITS_HORI          (4)
#define RKVOP_PQ_PREPROCESS_HIST_SIZE_VERI          (1 << RKVOP_PQ_PREPROCESS_HIST_BITS_VERI) /* 16 */
#define RKVOP_PQ_PREPROCESS_HIST_SIZE_HORI          (1 << RKVOP_PQ_PREPROCESS_HIST_BITS_HORI) /* 16 */
#define RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_BITS     (4)
#define RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_NUMS     (1 << (RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_BITS))

#define VDPP_COLOR_SPACE_LIMIT_RANGE    (0)
#define VDPP_COLOR_SPACE_FULL_RANGE     (1)

#define VDPP_CAP_UNSUPPORTED      (0)
#define VDPP_CAP_VEP              (1 << 0)
#define VDPP_CAP_HIST             (1 << 1)

#define VDPP_WORK_MODE_VEP        (2)
#define VDPP_WORK_MODE_DCI        (3) /* hist mode */

#define VDPP_DMSR_EN            (4)
#define VDPP_ES_EN              (2)
#define VDPP_SHARP_EN           (1)

/* DCI horizontal scale down mode select */
#define VDPP_DCI_HSD_DISABLE    (0)
#define VDPP_DCI_HSD_MODE_1     (1)
/* DCI vertical scale down mode select */
#define VDPP_DCI_VSD_DISABLE    (0)
#define VDPP_DCI_VSD_MODE_1     (1)
#define VDPP_DCI_VSD_MODE_2     (2)

enum VDPP_FMT {
    VDPP_FMT_YUV444 = 0,
    VDPP_FMT_YUV420 = 3,
};

enum VDPP_YUV_SWAP {
    VDPP_YUV_SWAP_SP_UV,
    VDPP_YUV_SWAP_SP_VU,
};

enum VDPP_PARAM_TYPE {
    VDPP_PARAM_TYPE_COM,
    VDPP_PARAM_TYPE_DMSR,
    VDPP_PARAM_TYPE_ZME_COM,
    VDPP_PARAM_TYPE_ZME_COEFF,
    VDPP_PARAM_TYPE_COM2 = 0x10,
    VDPP_PARAM_TYPE_ES,
    VDPP_PARAM_TYPE_HIST,
    VDPP_PARAM_TYPE_SHARP,
};

typedef enum VdppCmd_e {
    VDPP_CMD_INIT,                            /* reset msg to all zero */
    VDPP_CMD_SET_SRC,                         /* config source image info */
    VDPP_CMD_SET_DST,                         /* config destination image info */
    VDPP_CMD_SET_COM_CFG,
    /* DMSR command */
    VDPP_CMD_SET_DMSR_CFG         = 0x0100,   /* config DMSR configure */
    /* ZME command */
    VDPP_CMD_SET_ZME_COM_CFG      = 0x0200,   /* config ZME COM configure */
    VDPP_CMD_SET_ZME_COEFF_CFG,               /* config ZME COEFF configure */
    /* hardware trigger command */
    VDPP_CMD_RUN_SYNC             = 0x1000,   /* start sync mode process */
    VDPP_CMD_SET_COM2_CFG         = 0x2000,   /* config common params for RK3576 */
    VDPP_CMD_SET_DST_C,                       /* config destination chroma info */
    VDPP_CMD_SET_HIST_FD,                     /* config dci hist fd */
    VDPP_CMD_SET_ES,                          /* config ES configure */
    VDPP_CMD_SET_DCI_HIST,                    /* config dci hist configure */
    VDPP_CMD_SET_SHARP,                       /* config sharp configure */
} VdppCmd;

typedef void* VdppCtx;
typedef struct vdpp_com_ctx_t vdpp_com_ctx;

typedef struct VdppImg_t {
    RK_U32  mem_addr; /* base address fd */
    RK_U32  uv_addr;  /* chroma address fd + (offset << 10) */
    RK_U32  uv_off;
} VdppImg;

typedef struct vdpp_com_ops_t {
    MPP_RET (*init)(VdppCtx *ctx);
    MPP_RET (*deinit)(VdppCtx ctx);
    MPP_RET (*control)(VdppCtx ctx, VdppCmd cmd, void *param);
    void    (*release)(vdpp_com_ctx *ctx);
    RK_S32  (*check_cap)(VdppCtx ctx);
    MPP_RET (*reserve[3])(VdppCtx *ctx);
} vdpp_com_ops;

typedef struct vdpp_com_ctx_t {
    vdpp_com_ops *ops;
    VdppCtx priv;
    RK_S32 ver;
} vdpp_com_ctx;

union vdpp_api_content {
    struct {
        enum VDPP_YUV_SWAP sswap;
        enum VDPP_FMT dfmt;
        enum VDPP_YUV_SWAP dswap;
        RK_S32 src_width;
        RK_S32 src_height;
        RK_S32 dst_width;
        RK_S32 dst_height;
    } com;

    struct {
        bool enable;
        RK_U32 str_pri_y;
        RK_U32 str_sec_y;
        RK_U32 dumping_y;
        RK_U32 wgt_pri_gain_even_1;
        RK_U32 wgt_pri_gain_even_2;
        RK_U32 wgt_pri_gain_odd_1;
        RK_U32 wgt_pri_gain_odd_2;
        RK_U32 wgt_sec_gain;
        RK_U32 blk_flat_th;
        RK_U32 contrast_to_conf_map_x0;
        RK_U32 contrast_to_conf_map_x1;
        RK_U32 contrast_to_conf_map_y0;
        RK_U32 contrast_to_conf_map_y1;
        RK_U32 diff_core_th0;
        RK_U32 diff_core_th1;
        RK_U32 diff_core_wgt0;
        RK_U32 diff_core_wgt1;
        RK_U32 diff_core_wgt2;
        RK_U32 edge_th_low_arr[7];
        RK_U32 edge_th_high_arr[7];
    } dmsr;

    struct {
        bool bypass_enable;
        bool dering_enable;
        RK_U32 dering_sen_0;
        RK_U32 dering_sen_1;
        RK_U32 dering_blend_alpha;
        RK_U32 dering_blend_beta;
        RK_S16 (*tap8_coeff)[17][8];
        RK_S16 (*tap6_coeff)[17][8];
    } zme;

    struct {
        MppFrameFormat sfmt;
        enum VDPP_YUV_SWAP sswap;
        enum VDPP_FMT dfmt;
        enum VDPP_YUV_SWAP dswap;
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
        RK_U32 hist_mode_en;  /* 0 - vdpp, 1 - hist */
        /* high 16 bit: mask; low 3 bit: dmsr|es|sharp */
        RK_U32 cfg_set;
    } com2;

    struct {
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
        /* generated by es_iAngleDelta and es_iAngleDeltaExtra */
        RK_U32 es_tan_hi_th;
        RK_U32 es_tan_lo_th;
    } es;

    struct {
        RK_U32 hist_cnt_en;
        RK_U32 dci_hsd_mode;
        RK_U32 dci_vsd_mode;
        RK_U32 dci_yrgb_gather_num;
        RK_U32 dci_yrgb_gather_en;
        RK_U32 dci_csc_range;
    } hist;

    struct {
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
    } sharp;
};

struct vdpp_api_params {
    enum VDPP_PARAM_TYPE ptype;
    union vdpp_api_content param;
};

#ifdef __cplusplus
extern "C" {
#endif

vdpp_com_ctx* rockchip_vdpp_api_alloc_ctx(void);
void rockchip_vdpp_api_release_ctx(vdpp_com_ctx *com_ctx);
MPP_RET dci_hist_info_parser(RK_U8* p_pack_hist_addr, RK_U32* p_hist_local, RK_U32* p_hist_global);

#ifdef __cplusplus
}
#endif

#endif
