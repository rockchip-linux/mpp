/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPP_API_H
#define VDPP_API_H

#include <stdbool.h>

#include "mpp_debug.h"
#include "mpp_frame.h"

#define RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_BITS    (8)
#define RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_NUMS    (1 << (RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_BITS))
#define RKVOP_PQ_PREPROCESS_HIST_BITS_VERI          (4)
#define RKVOP_PQ_PREPROCESS_HIST_BITS_HORI          (4)
#define RKVOP_PQ_PREPROCESS_HIST_SIZE_VERI          (1 << RKVOP_PQ_PREPROCESS_HIST_BITS_VERI) /* 16 */
#define RKVOP_PQ_PREPROCESS_HIST_SIZE_HORI          (1 << RKVOP_PQ_PREPROCESS_HIST_BITS_HORI) /* 16 */
#define RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_BITS     (4)
#define RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_NUMS     (1 << (RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_BITS))

/* for vdpp_api_content.hist.dci_csc_range */
#define VDPP_COLOR_SPACE_LIMIT_RANGE    (0)
#define VDPP_COLOR_SPACE_FULL_RANGE     (1)

#define VDPP_CAP_UNSUPPORTED      (0)
#define VDPP_CAP_VEP              (1 << 0)
#define VDPP_CAP_HIST             (1 << 1)

#define VDPP_WORK_MODE_VEP        (2)
#define VDPP_WORK_MODE_DCI        (3) /* hist mode */

/* for vdpp_api_content.com2.cfg_set */
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

/* Pyramid layer number */
#define VDPP_PYR_MAX_LAYERS     (8)

/* vdpp reg format definition */
typedef enum VdppFmt_e {
    VDPP_FMT_YUV444 = 0, // yuv444sp
    VDPP_FMT_YUV420 = 3, // yuv420sp
} VdppFmt;

typedef enum VdppYuvSwap_e {
    VDPP_YUV_SWAP_SP_UV,
    VDPP_YUV_SWAP_SP_VU,
} VdppYuvSwap;

typedef enum VdppParamType_e {
    VDPP_PARAM_TYPE_COM,
    VDPP_PARAM_TYPE_DMSR,
    VDPP_PARAM_TYPE_ZME_COM,
    VDPP_PARAM_TYPE_ZME_COEFF,
    VDPP_PARAM_TYPE_COM2 = 0x10,
    VDPP_PARAM_TYPE_ES,
    VDPP_PARAM_TYPE_HIST,
    VDPP_PARAM_TYPE_SHARP,
    VDPP_PARAM_TYPE_PYR = 0x20, // pyramid building
    VDPP_PARAM_TYPE_BBD,        // black bar detection
} VdppParamType;

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

    /* new after vdpp2 */
    VDPP_CMD_SET_COM2_CFG         = 0x2000,   /* config common params for RK3576 */
    VDPP_CMD_SET_DST_C,                       /* config destination chroma info */
    VDPP_CMD_SET_HIST_FD,                     /* config dci hist fd */
    VDPP_CMD_SET_ES,                          /* config ES configure */
    VDPP_CMD_SET_DCI_HIST,                    /* config dci hist configure */
    VDPP_CMD_SET_SHARP,                       /* config sharp configure */
    VDPP_CMD_GET_AVG_LUMA         = 0x2100,   /* get average luma value */

    /* new after vdpp3 */
    VDPP_CMD_SET_PYR              = 0x3100,   /* config pyramid building configure */
    VDPP_CMD_SET_BBD              = 0x3200,   /* config black bar detection configure */
    VDPP_CMD_GET_BBD,                         /* get black bar detection result to vdpp_results */
} VdppCmd;

/* forward declaration */
typedef void* VdppCtx;
typedef struct VdppComCtx_t VdppComCtx;

typedef struct VdppImg_t {
    /* memory info */
    RK_U32 mem_addr; /* base address fd */
    RK_U32 uv_addr;  /* chroma address fd */
    RK_U32 uv_off;   /* chroma offset, unit: byte */
    RK_U32 y_off;    /* luma offset, unit: byte */
    RK_U32 reserved1[4];

    /* image info */
    RK_U32 fmt;     /* see MppFrameFormat */
    RK_U32 wid;     /* real width, unit: pixel */
    RK_U32 hgt;     /* real width, unit: pixel */
    RK_U32 wid_vir; /* virtual width, unit: pixel */
    RK_U32 hgt_vir; /* virtual height, unit: pixel */
    RK_U32 reserved2[3];
} VdppImg;

typedef struct VdppComOps_t {
    MPP_RET (*init)(VdppCtx ctx);
    MPP_RET (*deinit)(VdppCtx ctx);
    MPP_RET (*control)(VdppCtx ctx, VdppCmd cmd, void *param);
    void    (*release)(VdppComCtx *ctx); /* RESERVED */
    RK_S32  (*check_cap)(VdppCtx ctx);
    MPP_RET (*reserve[3])(VdppCtx *ctx);
} VdppComOps;

typedef VdppComOps vdpp_com_ops; // for compatibility

typedef struct VdppComCtx_t {
    VdppComOps *ops;
    VdppCtx priv;
    RK_S32 ver; // vdpp version, range: {0x100, 0x200, 0x300}
} VdppComCtx;

typedef union VdppApiContent_u {
    struct {
        VdppYuvSwap sswap;
        VdppFmt dfmt; // 0-yuv444sp, 3-yuv420sp
        VdppYuvSwap dswap;
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
        const RK_S16 (*tap8_coeff)[17][8]; // 11x17x8
        const RK_S16 (*tap6_coeff)[17][8];
    } zme;

    /* new after vdpp2 */
    struct {
        MppFrameFormat sfmt;      //
        VdppYuvSwap sswap;        // {0-uv, 1-vu}, default: 0
        VdppFmt dfmt;             // {0-yuv444sp, 3-yuv420sp}
        VdppYuvSwap dswap;        // {0-uv, 1-vu}, default: 0
        RK_U32 src_width;         // unit: pixel
        RK_U32 src_height;        // unit: pixel
        RK_U32 src_width_vir;     // unit: pixel
        RK_U32 src_height_vir;    // unit: pixel
        RK_U32 dst_width;         // unit: pixel
        RK_U32 dst_height;        // unit: pixel
        RK_U32 dst_width_vir;     // unit: pixel
        RK_U32 dst_height_vir;    // unit: pixel
        RK_U32 yuv_out_diff;      // output luma/chroma buffer separated flag
        RK_U32 dst_c_width;       // unit: pixel
        RK_U32 dst_c_height;      // unit: pixel
        RK_U32 dst_c_width_vir;   // unit: pixel
        RK_U32 dst_c_height_vir;  // unit: pixel
        /* 0-vdpp(vep), 1-dci_hist, use to set the work mode */
        RK_U32 hist_mode_en;
        /* high 16 bit: mask; low 3 bit (msb->lsb): dmsr|es|sharp */
        RK_U32 cfg_set;
        RK_S32 src_range;         // {0-limited, 1-full}, default: 0
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
        RK_U32 dci_hsd_mode;        // [0, 1], default: 0
        RK_U32 dci_vsd_mode;        // [0, 2], default: 0
        RK_U32 dci_yrgb_gather_num; // DCI AXI bus yrgb data gather transfer number {0:2, 1:4, 2:8}
        RK_U32 dci_yrgb_gather_en;  // default: 0
        RK_U32 dci_csc_range;       // {0-limited, 1-full}, default: 0
        RK_U32 hist_addr;           // hist buffer fd
    } hist;

    struct {
        RK_S32 sharp_enable;             // default: 1
        RK_S32 sharp_coloradj_bypass_en; // default: 1, DO NOT modify this!

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

    /* new after vdpp3 */
    struct {
        RK_U32  pyr_en;
        RK_U32  nb_layers;                           // [1, 3], default: 3
        VdppImg layer_imgs[VDPP_PYR_MAX_LAYERS];     // fd buffer of layer 1
    } pyr;

    struct {
        RK_U32 bbd_en;
        RK_U32 bbd_det_blcklmt; // luma threshold for black bar detection, always considered as full-range
    } bbd;
} VdppApiContent;

typedef union VdppResults_u {
    struct {
        RK_S32 luma_avg;        // output mean luma value in U10 range, <0 means invalid value
    } hist;

    struct {
        RK_U32 bbd_size_top;    // output black bar size on top
        RK_U32 bbd_size_bottom; // output black bar size on bottom
        RK_U32 bbd_size_left;   // output black bar size on left
        RK_U32 bbd_size_right;  // output black bar size on right
    } bbd;
} VdppResults;

typedef struct VdppApiParams_t {
    VdppParamType ptype;
    VdppApiContent param;
} VdppApiParams;

#ifdef __cplusplus
extern "C" {
#endif

VdppComCtx *rockchip_vdpp_api_alloc_ctx(void);
void rockchip_vdpp_api_release_ctx(VdppComCtx *com_ctx);

MPP_RET dci_hist_info_parser(const RK_U8 *p_pack_hist_addr, RK_U32 *p_hist_local, RK_U32 *p_hist_global);

#ifdef __cplusplus
}
#endif

#endif /* VDPP_API_H */
