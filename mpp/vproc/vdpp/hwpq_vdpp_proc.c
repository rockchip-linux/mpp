/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_buffer.h"
#include "mpp_soc.h"

#include "vdpp_api.h"
#include "hwpq_vdpp_proc_api.h"
#include "hwpq_debug.h"

RK_U32 hwpq_vdpp_debug = 0;

#define HWPQ_VDPP_MAX_FILE_NAME_LEN         (256)
#define HWPQ_VDPP_DEBUG_CFG_PROP            "vendor.vdpp.debug_cfg"

static const char hwpq_vdpp_in_path[] = "/data/vendor/rkalgo/hwpq_vdpp_in.bin";
static const char hwpq_vdpp_out_path[] = "/data/vendor/rkalgo/hwpq_vdpp_out.bin";

typedef struct VdppCtxImpl_e {
    vdpp_com_ctx* vdpp;

    MppBufferGroup memGroup;
    MppBuffer histbuf;
} VdppCtxImpl;

static void set_dmsr_default_config(struct vdpp_api_params* p_api_params)
{
    p_api_params->ptype = VDPP_PARAM_TYPE_DMSR;
    p_api_params->param.dmsr.enable = 1;
    p_api_params->param.dmsr.str_pri_y = 10;
    p_api_params->param.dmsr.str_sec_y = 4;
    p_api_params->param.dmsr.dumping_y = 6;
    p_api_params->param.dmsr.wgt_pri_gain_even_1 = 12;
    p_api_params->param.dmsr.wgt_pri_gain_even_2 = 12;
    p_api_params->param.dmsr.wgt_pri_gain_odd_1 = 8;
    p_api_params->param.dmsr.wgt_pri_gain_odd_2 = 16;
    p_api_params->param.dmsr.wgt_sec_gain = 5;
    p_api_params->param.dmsr.blk_flat_th = 20;
    p_api_params->param.dmsr.contrast_to_conf_map_x0 = 1680;
    p_api_params->param.dmsr.contrast_to_conf_map_x1 = 6720;
    p_api_params->param.dmsr.contrast_to_conf_map_y0 = 0;
    p_api_params->param.dmsr.contrast_to_conf_map_y1 = 65535;
    p_api_params->param.dmsr.diff_core_th0 = 1;
    p_api_params->param.dmsr.diff_core_th1 = 5;
    p_api_params->param.dmsr.diff_core_wgt0 = 16;
    p_api_params->param.dmsr.diff_core_wgt1 = 16;
    p_api_params->param.dmsr.diff_core_wgt2 = 16;
    p_api_params->param.dmsr.edge_th_low_arr[0] = 30;
    p_api_params->param.dmsr.edge_th_low_arr[1] = 10;
    p_api_params->param.dmsr.edge_th_low_arr[2] = 0;
    p_api_params->param.dmsr.edge_th_low_arr[3] = 0;
    p_api_params->param.dmsr.edge_th_low_arr[4] = 0;
    p_api_params->param.dmsr.edge_th_low_arr[5] = 0;
    p_api_params->param.dmsr.edge_th_low_arr[6] = 0;
    p_api_params->param.dmsr.edge_th_high_arr[0] = 60;
    p_api_params->param.dmsr.edge_th_high_arr[1] = 40;
    p_api_params->param.dmsr.edge_th_high_arr[2] = 20;
    p_api_params->param.dmsr.edge_th_high_arr[3] = 10;
    p_api_params->param.dmsr.edge_th_high_arr[4] = 10;
    p_api_params->param.dmsr.edge_th_high_arr[5] = 10;
    p_api_params->param.dmsr.edge_th_high_arr[6] = 10;
}

static void set_es_default_config(struct vdpp_api_params* p_api_params)
{
    static RK_S32 diff2conf_lut_x_tmp[9] = {
        0, 1024, 2048, 3072, 4096, 6144, 8192, 12288, 65535,
    };
    static RK_S32 diff2conf_lut_y_tmp[9] = {
        0, 84, 141, 179, 204, 233, 246, 253, 255,
    };

    p_api_params->ptype = VDPP_PARAM_TYPE_ES;
    p_api_params->param.es.es_bEnabledES          = 0;
    p_api_params->param.es.es_iAngleDelta         = 17;
    p_api_params->param.es.es_iAngleDeltaExtra    = 5;
    p_api_params->param.es.es_iGradNoDirTh        = 37;
    p_api_params->param.es.es_iGradFlatTh         = 75;
    p_api_params->param.es.es_iWgtGain            = 128;
    p_api_params->param.es.es_iWgtDecay           = 128;
    p_api_params->param.es.es_iLowConfTh          = 96;
    p_api_params->param.es.es_iLowConfRatio       = 32;
    p_api_params->param.es.es_iConfCntTh          = 4;
    p_api_params->param.es.es_iWgtLocalTh         = 64;
    p_api_params->param.es.es_iK1                 = 4096;
    p_api_params->param.es.es_iK2                 = 7168;
    p_api_params->param.es.es_iDeltaLimit         = 65280;
    memcpy(&p_api_params->param.es.es_iDiff2conf_lut_x[0], &diff2conf_lut_x_tmp[0],
           sizeof(diff2conf_lut_x_tmp));
    memcpy(&p_api_params->param.es.es_iDiff2conf_lut_y[0], &diff2conf_lut_y_tmp[0],
           sizeof(diff2conf_lut_y_tmp));
    p_api_params->param.es.es_bEndpointCheckEnable = 1;
}

static void set_hist_cnt_default_config(struct vdpp_api_params* p_api_params)
{
    p_api_params->ptype = VDPP_PARAM_TYPE_HIST;

    p_api_params->param.hist.hist_cnt_en   = 1;
    p_api_params->param.hist.dci_hsd_mode  = 0;
    p_api_params->param.hist.dci_vsd_mode  = 0;
    p_api_params->param.hist.dci_yrgb_gather_num  = 0;
    p_api_params->param.hist.dci_yrgb_gather_en  = 0;
}

static void set_shp_default_config(struct vdpp_api_params* p_api_params)
{
    static RK_S32 coring_zero_tmp[7]        = {
        5, 5, 8, 5, 8, 5, 5,
    };
    static RK_S32 coring_thr_tmp[7]         = {
        40, 40, 40, 24, 26, 30, 26,
    };
    static RK_S32 coring_ratio_tmp[7]       = {
        1479, 1188, 1024, 1422, 1024, 1024, 1024,
    };
    static RK_S32 gain_pos_tmp[7]           = {
        128, 256, 512, 256, 512, 256, 256,
    };
    static RK_S32 gain_neg_tmp[7]           = {
        128, 256, 512, 256, 512, 256, 256,
    };
    static RK_S32 limit_ctrl_pos0_tmp[7]    = {
        64, 64, 64, 64, 64, 64, 64,
    };
    static RK_S32 limit_ctrl_pos1_tmp[7]    = {
        120, 120, 120, 120, 120, 120, 120,
    };
    static RK_S32 limit_ctrl_neg0_tmp[7]    = {
        64, 64, 64, 64, 64, 64, 64,
    };
    static RK_S32 limit_ctrl_neg1_tmp[7]    = {
        120, 120, 120, 120, 120, 120, 120,
    };
    static RK_S32 limit_ctrl_ratio_tmp[7]   = {
        128, 128, 128, 128, 128, 128, 128,
    };
    static RK_S32 limit_ctrl_bnd_pos_tmp[7] = {
        81, 131, 63, 81, 63, 63, 63,
    };
    static RK_S32 limit_ctrl_bnd_neg_tmp[7] = {
        81, 131, 63, 81, 63, 63, 63,
    };
    static RK_S32 lum_grd_tmp[6]            = {
        0, 200, 300, 860, 960, 102,
    };
    static RK_S32 lum_val_tmp[6]            = {
        64, 64, 64, 64, 64, 64,
    };
    static RK_S32 adp_grd_tmp[6]            = {
        0, 4, 60, 180, 300, 1023,
    };
    static RK_S32 adp_val_tmp[6]            = {
        64, 64, 64, 64, 64, 64,
    };
    static RK_S32 var_grd_tmp[6]            = {
        0, 39, 102, 209, 500, 1023,
    };
    static RK_S32 var_val_tmp[6]            = {
        64, 64, 64, 64, 64, 64,
    };
    static RK_S32 diag_adj_gain_tab_tmp[8]  = {
        6, 7, 8, 9, 10, 11, 12, 13,
    };
    static RK_S32 roll_tab_pattern0[16]     = {
        0, 0, 0, 1, 2, 3, 4, 6, 8, 10, 11, 12, 13, 14, 15, 15,
    };
    static RK_S32 roll_tab_pattern1[16]     = {
        31, 31, 30, 29, 28, 27, 25, 23, 21, 19, 18, 17, 16, 16, 15, 15,
    };
    static RK_S32 roll_tab_pattern2[16]     = {
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    };
    static RK_S32 tex_grd_tmp[6]            = {
        0, 128, 256, 400, 600, 1023,
    };
    static RK_S32 tex_val_tmp[6]            = {
        40, 60, 80, 100, 127, 127,
    };

    p_api_params->ptype = VDPP_PARAM_TYPE_SHARP;

    p_api_params->param.sharp.sharp_enable               = 1;
    p_api_params->param.sharp.sharp_coloradj_bypass_en   = 1;

    p_api_params->param.sharp.lti_h_enable           = 0;
    p_api_params->param.sharp.lti_h_radius           = 1;
    p_api_params->param.sharp.lti_h_slope            = 100;
    p_api_params->param.sharp.lti_h_thresold         = 21;
    p_api_params->param.sharp.lti_h_gain             = 8;
    p_api_params->param.sharp.lti_h_noise_thr_pos    = 1023;
    p_api_params->param.sharp.lti_h_noise_thr_neg    = 1023;

    p_api_params->param.sharp.lti_v_enable           = 0;
    p_api_params->param.sharp.lti_v_radius           = 1;
    p_api_params->param.sharp.lti_v_slope            = 100;
    p_api_params->param.sharp.lti_v_thresold         = 21;
    p_api_params->param.sharp.lti_v_gain             = 8;
    p_api_params->param.sharp.lti_v_noise_thr_pos    = 1023;
    p_api_params->param.sharp.lti_v_noise_thr_neg    = 1023;

    p_api_params->param.sharp.cti_h_enable           = 0;
    p_api_params->param.sharp.cti_h_radius           = 1;
    p_api_params->param.sharp.cti_h_slope            = 100;
    p_api_params->param.sharp.cti_h_thresold         = 21;
    p_api_params->param.sharp.cti_h_gain             = 8;
    p_api_params->param.sharp.cti_h_noise_thr_pos    = 1023;
    p_api_params->param.sharp.cti_h_noise_thr_neg    = 1023;

    p_api_params->param.sharp.peaking_enable         = 1;
    p_api_params->param.sharp.peaking_gain           = 196;

    p_api_params->param.sharp.peaking_coring_enable      = 1;
    p_api_params->param.sharp.peaking_limit_ctrl_enable  = 1;
    p_api_params->param.sharp.peaking_gain_enable        = 1;

    memcpy(p_api_params->param.sharp.peaking_coring_zero,    coring_zero_tmp,
           sizeof(coring_zero_tmp));
    memcpy(p_api_params->param.sharp.peaking_coring_thr,     coring_thr_tmp,
           sizeof(coring_thr_tmp));
    memcpy(p_api_params->param.sharp.peaking_coring_ratio,   coring_ratio_tmp,
           sizeof(coring_ratio_tmp));
    memcpy(p_api_params->param.sharp.peaking_gain_pos,   gain_pos_tmp,   sizeof(gain_pos_tmp));
    memcpy(p_api_params->param.sharp.peaking_gain_neg,   gain_neg_tmp,   sizeof(gain_neg_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_pos0,    limit_ctrl_pos0_tmp,
           sizeof(limit_ctrl_pos0_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_pos1,    limit_ctrl_pos1_tmp,
           sizeof(limit_ctrl_pos1_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_neg0,    limit_ctrl_neg0_tmp,
           sizeof(limit_ctrl_neg0_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_neg1,    limit_ctrl_neg1_tmp,
           sizeof(limit_ctrl_neg1_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_ratio,   limit_ctrl_ratio_tmp,
           sizeof(limit_ctrl_ratio_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_bnd_pos, limit_ctrl_bnd_pos_tmp,
           sizeof(limit_ctrl_bnd_pos_tmp));
    memcpy(p_api_params->param.sharp.peaking_limit_ctrl_bnd_neg, limit_ctrl_bnd_neg_tmp,
           sizeof(limit_ctrl_bnd_neg_tmp));

    p_api_params->param.sharp.peaking_edge_ctrl_enable               = 1;
    p_api_params->param.sharp.peaking_edge_ctrl_non_dir_thr          = 16;
    p_api_params->param.sharp.peaking_edge_ctrl_dir_cmp_ratio        = 4;
    p_api_params->param.sharp.peaking_edge_ctrl_non_dir_wgt_offset   = 64;
    p_api_params->param.sharp.peaking_edge_ctrl_non_dir_wgt_ratio    = 16;
    p_api_params->param.sharp.peaking_edge_ctrl_dir_cnt_thr          = 2;
    p_api_params->param.sharp.peaking_edge_ctrl_dir_cnt_avg          = 3;
    p_api_params->param.sharp.peaking_edge_ctrl_dir_cnt_offset       = 2;
    p_api_params->param.sharp.peaking_edge_ctrl_diag_dir_thr         = 16;

    memcpy(p_api_params->param.sharp.peaking_edge_ctrl_diag_adj_gain_tab,
           diag_adj_gain_tab_tmp, sizeof(diag_adj_gain_tab_tmp));

    p_api_params->param.sharp.peaking_estc_enable                    = 1;
    p_api_params->param.sharp.peaking_estc_delta_offset_h            = 4;
    p_api_params->param.sharp.peaking_estc_alpha_over_h              = 8;
    p_api_params->param.sharp.peaking_estc_alpha_under_h             = 16;
    p_api_params->param.sharp.peaking_estc_alpha_over_unlimit_h      = 64;
    p_api_params->param.sharp.peaking_estc_alpha_under_unlimit_h     = 112;
    p_api_params->param.sharp.peaking_estc_delta_offset_v            = 4;
    p_api_params->param.sharp.peaking_estc_alpha_over_v              = 8;
    p_api_params->param.sharp.peaking_estc_alpha_under_v             = 16;
    p_api_params->param.sharp.peaking_estc_alpha_over_unlimit_v      = 64;
    p_api_params->param.sharp.peaking_estc_alpha_under_unlimit_v     = 112;
    p_api_params->param.sharp.peaking_estc_delta_offset_d0           = 4;
    p_api_params->param.sharp.peaking_estc_alpha_over_d0             = 16;
    p_api_params->param.sharp.peaking_estc_alpha_under_d0            = 16;
    p_api_params->param.sharp.peaking_estc_alpha_over_unlimit_d0     = 96;
    p_api_params->param.sharp.peaking_estc_alpha_under_unlimit_d0    = 96;
    p_api_params->param.sharp.peaking_estc_delta_offset_d1           = 4;
    p_api_params->param.sharp.peaking_estc_alpha_over_d1             = 16;
    p_api_params->param.sharp.peaking_estc_alpha_under_d1            = 16;
    p_api_params->param.sharp.peaking_estc_alpha_over_unlimit_d1     = 96;
    p_api_params->param.sharp.peaking_estc_alpha_under_unlimit_d1    = 96;
    p_api_params->param.sharp.peaking_estc_delta_offset_non          = 4;
    p_api_params->param.sharp.peaking_estc_alpha_over_non            = 8;
    p_api_params->param.sharp.peaking_estc_alpha_under_non           = 8;
    p_api_params->param.sharp.peaking_estc_alpha_over_unlimit_non    = 112;
    p_api_params->param.sharp.peaking_estc_alpha_under_unlimit_non   = 112;
    p_api_params->param.sharp.peaking_filter_cfg_diag_enh_coef       = 6;

    p_api_params->param.sharp.peaking_filt_core_H0[0]                = 4;
    p_api_params->param.sharp.peaking_filt_core_H0[1]                = 16;
    p_api_params->param.sharp.peaking_filt_core_H0[2]                = 24;
    p_api_params->param.sharp.peaking_filt_core_H1[0]                = -16;
    p_api_params->param.sharp.peaking_filt_core_H1[1]                = 0;
    p_api_params->param.sharp.peaking_filt_core_H1[2]                = 32;
    p_api_params->param.sharp.peaking_filt_core_H2[0]                = 0;
    p_api_params->param.sharp.peaking_filt_core_H2[1]                = -16;
    p_api_params->param.sharp.peaking_filt_core_H2[2]                = 32;
    p_api_params->param.sharp.peaking_filt_core_V0[0]                = 1;
    p_api_params->param.sharp.peaking_filt_core_V0[1]                = 4;
    p_api_params->param.sharp.peaking_filt_core_V0[2]                = 6;
    p_api_params->param.sharp.peaking_filt_core_V1[0]                = -4;
    p_api_params->param.sharp.peaking_filt_core_V1[1]                = 0;
    p_api_params->param.sharp.peaking_filt_core_V1[2]                = 8;
    p_api_params->param.sharp.peaking_filt_core_V2[0]                = 0;
    p_api_params->param.sharp.peaking_filt_core_V2[1]                = -4;
    p_api_params->param.sharp.peaking_filt_core_V2[2]                = 8;
    p_api_params->param.sharp.peaking_filt_core_USM[0]               = 1;
    p_api_params->param.sharp.peaking_filt_core_USM[1]               = 4;
    p_api_params->param.sharp.peaking_filt_core_USM[2]               = 6;

    p_api_params->param.sharp.shootctrl_enable               = 1;
    p_api_params->param.sharp.shootctrl_filter_radius        = 1;
    p_api_params->param.sharp.shootctrl_delta_offset         = 16;
    p_api_params->param.sharp.shootctrl_alpha_over           = 8;
    p_api_params->param.sharp.shootctrl_alpha_under          = 8;
    p_api_params->param.sharp.shootctrl_alpha_over_unlimit   = 112;
    p_api_params->param.sharp.shootctrl_alpha_under_unlimit  = 112;

    p_api_params->param.sharp.global_gain_enable             = 0;
    p_api_params->param.sharp.global_gain_lum_mode           = 0;

    memcpy(p_api_params->param.sharp.global_gain_lum_grd, lum_grd_tmp, sizeof(lum_grd_tmp));
    memcpy(p_api_params->param.sharp.global_gain_lum_val, lum_val_tmp, sizeof(lum_val_tmp));
    memcpy(p_api_params->param.sharp.global_gain_adp_grd, adp_grd_tmp, sizeof(adp_grd_tmp));
    memcpy(p_api_params->param.sharp.global_gain_adp_val, adp_val_tmp, sizeof(adp_val_tmp));
    memcpy(p_api_params->param.sharp.global_gain_var_grd, var_grd_tmp, sizeof(var_grd_tmp));
    memcpy(p_api_params->param.sharp.global_gain_var_val, var_val_tmp, sizeof(var_val_tmp));

    p_api_params->param.sharp.color_ctrl_enable              = 0;

    p_api_params->param.sharp.color_ctrl_p0_scaling_coef     = 1;
    p_api_params->param.sharp.color_ctrl_p0_point_u          = 115;
    p_api_params->param.sharp.color_ctrl_p0_point_v          = 155;
    memcpy(p_api_params->param.sharp.color_ctrl_p0_roll_tab, roll_tab_pattern0,
           sizeof(roll_tab_pattern0));
    p_api_params->param.sharp.color_ctrl_p1_scaling_coef     = 1;
    p_api_params->param.sharp.color_ctrl_p1_point_u          = 90;
    p_api_params->param.sharp.color_ctrl_p1_point_v          = 120;
    memcpy(p_api_params->param.sharp.color_ctrl_p1_roll_tab, roll_tab_pattern1,
           sizeof(roll_tab_pattern1));
    p_api_params->param.sharp.color_ctrl_p2_scaling_coef     = 1;
    p_api_params->param.sharp.color_ctrl_p2_point_u          = 128;
    p_api_params->param.sharp.color_ctrl_p2_point_v          = 128;
    memcpy(p_api_params->param.sharp.color_ctrl_p2_roll_tab, roll_tab_pattern2,
           sizeof(roll_tab_pattern2));
    p_api_params->param.sharp.color_ctrl_p3_scaling_coef     = 1;
    p_api_params->param.sharp.color_ctrl_p3_point_u          = 128;
    p_api_params->param.sharp.color_ctrl_p3_point_v          = 128;
    memcpy(p_api_params->param.sharp.color_ctrl_p3_roll_tab, roll_tab_pattern2,
           sizeof(roll_tab_pattern2));

    p_api_params->param.sharp.tex_adj_enable                 = 0;
    p_api_params->param.sharp.tex_adj_y_mode_select          = 3;
    p_api_params->param.sharp.tex_adj_mode_select            = 0;

    memcpy(p_api_params->param.sharp.tex_adj_grd, tex_grd_tmp, sizeof(tex_grd_tmp));
    memcpy(p_api_params->param.sharp.tex_adj_val, tex_val_tmp, sizeof(tex_val_tmp));
}

static RK_S32 vdpp_set_user_cfg(vdpp_com_ctx* vdpp, vdpp_params* p_vdpp_params,
                                RK_U32 cfg_update_flag)
{
    struct vdpp_api_params params;
    RK_S32 ret = MPP_OK;

    if (cfg_update_flag == 0) {
        hwpq_vdpp_info("vdpp config not changed\n");
        return ret;
    }

    hwpq_vdpp_info("update vdpp config\n");

    set_dmsr_default_config(&params);
    params.param.dmsr.enable    = p_vdpp_params->dmsr_en;
    params.param.dmsr.str_pri_y = p_vdpp_params->str_pri_y;
    params.param.dmsr.str_sec_y = p_vdpp_params->str_sec_y;
    params.param.dmsr.dumping_y = p_vdpp_params->dumping_y;
    ret |= vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_DMSR_CFG, &params);

    set_es_default_config(&params);
    params.param.es.es_bEnabledES   = p_vdpp_params->es_en;
    params.param.es.es_iWgtGain     = p_vdpp_params->es_iWgtGain;
    ret |= vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_ES, &params);

    set_hist_cnt_default_config(&params);
    params.param.hist.hist_cnt_en   = p_vdpp_params->hist_cnt_en;
    params.param.hist.dci_csc_range = p_vdpp_params->hist_csc_range;
    ret |= vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_DCI_HIST, &params);

    set_shp_default_config(&params);
    params.param.sharp.sharp_enable                     = p_vdpp_params->shp_en;
    params.param.sharp.peaking_gain                     = p_vdpp_params->peaking_gain;
    params.param.sharp.shootctrl_enable                 = p_vdpp_params->shp_shoot_ctrl_en;
    params.param.sharp.shootctrl_alpha_over             = p_vdpp_params->shp_shoot_ctrl_over;
    params.param.sharp.shootctrl_alpha_over_unlimit     = p_vdpp_params->shp_shoot_ctrl_over;
    params.param.sharp.shootctrl_alpha_under            = p_vdpp_params->shp_shoot_ctrl_under;
    params.param.sharp.shootctrl_alpha_under_unlimit    = p_vdpp_params->shp_shoot_ctrl_under;
    ret |= vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_SHARP, &params);

    return ret;
}

static MPP_RET vdpp_set_img(vdpp_com_ctx *ctx, RK_S32 fd_yrgb, RK_S32 fd_cbcr,
                            RK_S32 cbcr_offset, VdppCmd cmd)
{
    VdppImg img;

    hwpq_vdpp_info("yrgb_fd=%d, cbcr_fd=%d, cbcr_offset=%d\n", fd_yrgb, fd_cbcr, cbcr_offset);
    img.mem_addr = fd_yrgb;
    img.uv_addr = fd_cbcr;
    img.uv_off = cbcr_offset;

    return ctx->ops->control(ctx->priv, cmd, &img);
}

static void* vdpp_map_buffer_with_fd(int fd, size_t bufSize)
{
    void* ptr = NULL;

    if (fd > 0) {
        ptr = mmap(NULL, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            mpp_err_f("failed to mmap buffer, fd=%d, size=%zu\n", fd, bufSize);
            return NULL;
        }
    } else {
        mpp_err_f("failed to mmap buffer with fd! invalid fd value %d input!\n", fd);
    }

    return ptr;
}

static inline void vdpp_unmap_buffer(void* ptr, size_t bufSize)
{
    if (ptr) {
        munmap(ptr, bufSize);
    }
}

static FILE *try_env_file(const char *env, const char *path, pid_t tid, int index)
{
    const char *fname = NULL;
    FILE *fp = NULL;
    char name[HWPQ_VDPP_MAX_FILE_NAME_LEN];

    mpp_env_get_str(env, &fname, path);
    if (fname == path) {
        snprintf(name, sizeof(name) - 1, "%s_%03d-%d", path, index, tid);
        fname = name;
    }

    fp = fopen(fname, "w+b");
    hwpq_vdpp_info("open %s %p for dump\n", fname, fp);

    return fp;
}

static void vdpp_dump(rk_vdpp_proc_params *p_proc_param, int index)
{
    FILE *fp_in = NULL;
    FILE *fp_out = NULL;
    pid_t tid = syscall(SYS_gettid);

    if (NULL == p_proc_param) {
        mpp_err_f("found NULL proc_param %p\n", p_proc_param);
        return;
    }

    if (hwpq_vdpp_debug & HWPQ_VDPP_DUMP_IN) {
        fp_in = try_env_file("hwpq_vdpp_dump_in", hwpq_vdpp_in_path, tid, index);
        if (NULL == fp_in) {
            mpp_err_f("failed to open file %p\n", fp_in);
        } else {
            int fd = p_proc_param->src_img_info.img_yrgb.fd;
            RK_U32 src_y_buf_len = p_proc_param->src_img_info.img_yrgb.w_vir *
                                   p_proc_param->src_img_info.img_yrgb.h_vir;
            RK_U8 *ptr = (RK_U8*)vdpp_map_buffer_with_fd(fd, src_y_buf_len);

            if (ptr == NULL) {
                mpp_err_f("vdpp dump fd(%d) map error!\n", fd);
            } else {
                fwrite(ptr, 1, src_y_buf_len, fp_in);
                fclose(fp_in);
            }

            vdpp_unmap_buffer(ptr, src_y_buf_len);
        }
    }

    if (hwpq_vdpp_debug & HWPQ_VDPP_DUMP_OUT) {
        fp_out = try_env_file("hwpq_vdpp_dump_out", hwpq_vdpp_out_path, tid, index);
        if (NULL == fp_out) {
            mpp_err_f("failed to open file %p\n", fp_out);
        } else {
            int fd = p_proc_param->src_img_info.img_yrgb.fd;
            RK_U32 dst_y_buf_len = p_proc_param->dst_img_info.img_yrgb.w_vir *
                                   p_proc_param->dst_img_info.img_yrgb.h_vir;
            RK_U8 *ptr = (RK_U8*)vdpp_map_buffer_with_fd(fd, dst_y_buf_len);

            if (ptr == NULL) {
                mpp_err_f("vdpp dump fd(%d) map error!\n", fd);
            } else {
                fwrite(ptr, 1, dst_y_buf_len, fp_out);
                fclose(fp_out);
            }

            vdpp_unmap_buffer(ptr, dst_y_buf_len);
        }
    }

    MPP_FCLOSE(fp_in);
    MPP_FCLOSE(fp_out);
}

static MppFrameFormat img_format_convert(vdpp_frame_format img_fmt_in)
{
    MppFrameFormat img_fmt_out = MPP_FMT_YUV420SP;

    switch (img_fmt_in) {
    case VDPP_FMT_NV24:
        img_fmt_out = MPP_FMT_YUV444SP;
        break;
    case VDPP_FMT_NV16:
        img_fmt_out = MPP_FMT_YUV422SP;
        break;
    case VDPP_FMT_NV12:
        img_fmt_out = MPP_FMT_YUV420SP;
        break;
    case VDPP_FMT_NV15:
        img_fmt_out = MPP_FMT_YUV420SP_10BIT;
        break;
    case VDPP_FMT_NV20:
        img_fmt_out = MPP_FMT_YUV420SP_10BIT;
        break;
    case VDPP_FMT_NV30:
        img_fmt_out = MPP_FMT_YUV420SP_10BIT;
        break;

    case VDPP_FMT_RGBA:
        img_fmt_out = MPP_FMT_RGBA8888;
        break;
    case VDPP_FMT_RG24:
        img_fmt_out = MPP_FMT_RGB888;
        break;
    case VDPP_FMT_BG24:
        img_fmt_out = MPP_FMT_BGR888;
        break;

    default:
        mpp_err_f("unsupport input format(%x), set NV12!", img_fmt_in);
        break;
    }

    return img_fmt_out;
}

static enum VDPP_YUV_SWAP get_img_format_swap(vdpp_frame_format img_fmt_in)
{
    enum VDPP_YUV_SWAP img_fmt_swap = VDPP_YUV_SWAP_SP_UV;

    switch (img_fmt_in) {
    case VDPP_FMT_NV24_VU:
    case VDPP_FMT_NV16_VU:
    case VDPP_FMT_NV12_VU:
        img_fmt_swap = VDPP_YUV_SWAP_SP_VU;
        break;

    default:
        img_fmt_swap = VDPP_YUV_SWAP_SP_UV;
        break;
    }

    return img_fmt_swap;
}

int hwpq_vdpp_deinit(rk_vdpp_context ctx)
{
    VdppCtxImpl *p = (VdppCtxImpl*)ctx;
    vdpp_com_ctx* vdpp = NULL;
    MPP_RET ret = MPP_NOK;

    hwpq_vdpp_enter();

    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx %p\n", ctx);
        ret = MPP_ERR_NULL_PTR;
        goto __RET;
    }

    vdpp = p->vdpp;
    if (NULL == vdpp || NULL == vdpp->ops) {
        mpp_err_f("found NULL vdpp\n");
        ret = MPP_ERR_NULL_PTR;
        goto __RET;
    }

    if (vdpp->ops->deinit) {
        ret = vdpp->ops->deinit(vdpp->priv);
        if (ret) {
            mpp_err_f("vdpp deinit failed! ret %d\n", ret);
        }
    }

    if (p->histbuf) {
        mpp_buffer_put(p->histbuf);
        p->histbuf = NULL;
    }

    if (p->memGroup) {
        mpp_buffer_group_put(p->memGroup);
        p->memGroup = NULL;
    }

    rockchip_vdpp_api_release_ctx(vdpp);
    MPP_FREE(p);
    hwpq_vdpp_leave();

__RET:
    return ret;
}

int hwpq_vdpp_init(rk_vdpp_context *p_ctx_ptr)
{
    VdppCtxImpl *p = NULL;
    vdpp_com_ctx *vdpp = NULL;
    MppBufferGroup memGroup = NULL;
    MppBuffer histbuf = NULL;
    MPP_RET ret = MPP_NOK;

    hwpq_vdpp_enter();

    if (NULL == p_ctx_ptr) {
        mpp_err("found NULL vdpp ctx pointer\n");
        ret = MPP_ERR_NULL_PTR;
        goto __ERR;
    }
    /* alloc vdpp ctx impl */
    p = mpp_malloc(VdppCtxImpl, 1);
    if (NULL == p) {
        mpp_err("alloc vdpp ctx failed!");
        ret = MPP_ERR_MALLOC;
        goto __ERR;
    }
    /* alloc vdpp */
    vdpp = rockchip_vdpp_api_alloc_ctx();
    if (NULL == vdpp || NULL == vdpp->ops) {
        mpp_err("alloc vdpp ctx failed!");
        ret = MPP_ERR_MALLOC;
        goto __ERR;
    }
    /* alloc buffer group */
    ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        mpp_err("memGroup mpp_buffer_group_get failed\n");
        ret = MPP_NOK;
        goto __ERR;
    }

    mpp_buffer_get(memGroup, &histbuf, VDPP_HIST_LENGTH);
    if (ret) {
        mpp_err("alloc histbuf failed\n");
        ret = MPP_NOK;
        goto __ERR;
    }

    /* setup env prop */
    mpp_env_get_u32(HWPQ_VDPP_DEBUG_CFG_PROP, &hwpq_vdpp_debug, 0);

    if (vdpp->ops->init) {
        ret = vdpp->ops->init(&vdpp->priv);
        if (ret) {
            mpp_err_f("vdpp init failed! ret %d\n", ret);
            goto __ERR;
        }
    }

    p->vdpp = vdpp;
    p->memGroup = memGroup;
    p->histbuf = histbuf;
    *p_ctx_ptr = (rk_vdpp_context)p;

    hwpq_vdpp_leave();
    return ret;

__ERR:
    if (histbuf) {
        mpp_buffer_put(histbuf);
        histbuf = NULL;
    }

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    rockchip_vdpp_api_release_ctx(vdpp);
    MPP_FREE(p);

    return ret;
}

static MPP_RET hwpq_vdpp_common_config(vdpp_com_ctx *vdpp, rk_vdpp_proc_params *p_proc_param)
{
    struct vdpp_api_params params;
    RK_U32 is_vdpp2 = (mpp_get_soc_type() == ROCKCHIP_SOC_RK3576);
    RK_U32 yuv_out_diff;
    MPP_RET ret = MPP_NOK;

    yuv_out_diff = (p_proc_param->yuv_diff_flag && is_vdpp2);
    hwpq_vdpp_info("is_vdpp2: %d, yuv_diff: %d\n", is_vdpp2, yuv_out_diff);

    if (is_vdpp2) {
        RK_U32 hist_mode_en = p_proc_param->hist_mode_en;

        params.ptype = VDPP_PARAM_TYPE_COM2;
        memset(&params.param, 0, sizeof(union vdpp_api_content));
        params.param.com2.sfmt = img_format_convert(p_proc_param->src_img_info.img_fmt);
        params.param.com2.src_width = p_proc_param->src_img_info.img_yrgb.w_vld;
        params.param.com2.src_height = p_proc_param->src_img_info.img_yrgb.h_vld;
        params.param.com2.src_width_vir = p_proc_param->src_img_info.img_yrgb.w_vir;
        params.param.com2.src_height_vir = p_proc_param->src_img_info.img_yrgb.h_vir;
        params.param.com2.sswap = get_img_format_swap(p_proc_param->src_img_info.img_fmt);
        params.param.com2.dfmt = VDPP_FMT_YUV444; // TODO
        params.param.com2.dst_width = p_proc_param->dst_img_info.img_yrgb.w_vld;
        params.param.com2.dst_height = p_proc_param->dst_img_info.img_yrgb.h_vld;
        params.param.com2.dst_width_vir = p_proc_param->dst_img_info.img_yrgb.w_vir;
        params.param.com2.dst_height_vir = p_proc_param->dst_img_info.img_yrgb.h_vir;
        if (yuv_out_diff) {
            params.param.com2.yuv_out_diff = yuv_out_diff;
            params.param.com2.dst_c_width = p_proc_param->dst_img_info.img_cbcr.w_vld;
            params.param.com2.dst_c_height = p_proc_param->dst_img_info.img_cbcr.h_vld;
            params.param.com2.dst_c_width_vir = p_proc_param->dst_img_info.img_cbcr.w_vir;
            params.param.com2.dst_c_height_vir = p_proc_param->dst_img_info.img_cbcr.h_vir;
        }
        params.param.com2.dswap = get_img_format_swap(p_proc_param->dst_img_info.img_fmt);
        params.param.com2.hist_mode_en = hist_mode_en;
        hwpq_vdpp_info("hist_mode: %d\n", params.param.com2.hist_mode_en);
        hwpq_vdpp_info("src-fmt: %d\n", p_proc_param->src_img_info.img_fmt);
        hwpq_vdpp_info("dst-fmt: %d\n", p_proc_param->dst_img_info.img_fmt);
        hwpq_vdpp_info("src-res: %d-%d  %d-%d\n", params.param.com2.src_width, params.param.com2.src_height,
                       params.param.com2.src_width_vir, params.param.com2.src_height_vir);
        hwpq_vdpp_info("dst-res: %d-%d  %d-%d\n", params.param.com2.dst_width, params.param.com2.dst_height,
                       params.param.com2.dst_width_vir, params.param.com2.dst_height_vir);
        ret = vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_COM2_CFG, &params);
    } else {
        params.ptype = VDPP_PARAM_TYPE_COM;
        memset(&params.param, 0, sizeof(union vdpp_api_content));
        params.param.com.src_width = p_proc_param->src_img_info.img_yrgb.w_vld;
        params.param.com.src_height = p_proc_param->src_img_info.img_yrgb.h_vld;
        params.param.com.sswap = get_img_format_swap(p_proc_param->src_img_info.img_fmt);
        params.param.com.dfmt = VDPP_FMT_YUV444; // TODO
        params.param.com.dst_width = p_proc_param->dst_img_info.img_yrgb.w_vld;
        params.param.com.dst_height = p_proc_param->dst_img_info.img_yrgb.h_vld;
        params.param.com.dswap = get_img_format_swap(p_proc_param->dst_img_info.img_fmt);
        ret = vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_COM_CFG, &params);
    }

    return ret;
}

int hwpq_vdpp_proc(rk_vdpp_context ctx, rk_vdpp_proc_params *p_proc_param)
{
    VdppCtxImpl *p = (VdppCtxImpl*)ctx;
    vdpp_com_ctx* vdpp = NULL;
    RK_U32 is_vdpp2 = (mpp_get_soc_type() == ROCKCHIP_SOC_RK3576);
    MppBuffer histbuf = NULL;
    MppBufferGroup memGroup = NULL;
    RK_S32 ret = MPP_OK;
    void* phist;
    RK_S32 fdhist;
    static int frame_idx = 0;

    hwpq_vdpp_enter();

    if (NULL == ctx || NULL == p_proc_param) {
        mpp_err_f("found NULL input ctx %p proc_param %p\n", ctx, p_proc_param);
        return MPP_ERR_NULL_PTR;
    }

    vdpp = p->vdpp;
    if (NULL == vdpp || NULL == vdpp->ops || NULL == vdpp->ops->control) {
        mpp_err_f("found NULL vdpp or vdpp ops\n");
        return MPP_ERR_NULL_PTR;
    }

    memGroup = p->memGroup;
    histbuf = p->histbuf;
    if (NULL == memGroup || NULL == histbuf) {
        mpp_err_f("found NULL memGroup %p or histbuf %p\n", memGroup, histbuf);
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32(HWPQ_VDPP_DEBUG_CFG_PROP, &hwpq_vdpp_debug, 0);

    hwpq_vdpp_info("proc frame_idx %d\n", p_proc_param->frame_idx);

    hwpq_vdpp_info("begin set image info\n");
    hwpq_vdpp_info("set src img_info\n");
    ret |= vdpp_set_img(vdpp, p_proc_param->src_img_info.img_yrgb.fd, p_proc_param->src_img_info.img_cbcr.fd,
                        p_proc_param->src_img_info.img_cbcr.offset, VDPP_CMD_SET_SRC);
    hwpq_vdpp_info("set dst img_info\n");
    ret |= vdpp_set_img(vdpp, p_proc_param->dst_img_info.img_yrgb.fd, p_proc_param->dst_img_info.img_cbcr.fd,
                        p_proc_param->dst_img_info.img_cbcr.offset, VDPP_CMD_SET_DST);
    ret |= vdpp_set_img(vdpp, p_proc_param->dst_img_info.img_yrgb.fd, p_proc_param->dst_img_info.img_cbcr.fd,
                        p_proc_param->dst_img_info.img_cbcr.offset, VDPP_CMD_SET_DST_C);

    ret |= hwpq_vdpp_common_config(vdpp, p_proc_param);
    if (ret) {
        mpp_err("vdpp common config failed\n");
        return MPP_NOK;
    }

    /* set params */
    if (vdpp_set_user_cfg(vdpp, &p_proc_param->vdpp_config, p_proc_param->vdpp_config_update_flag))
        mpp_err_f("warning: set user cfg failed");

    phist   = mpp_buffer_get_ptr(histbuf);
    fdhist  = mpp_buffer_get_fd(histbuf);

    if (is_vdpp2) {
        ret = vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_HIST_FD, &fdhist);
        if (ret) {
            mpp_err("set hist fd failed\n");
            return MPP_NOK;
        }
    }

    ret = vdpp->ops->control(vdpp->priv, VDPP_CMD_RUN_SYNC, NULL);
    if (ret) {
        mpp_err("run vdpp failed\n");
        return MPP_NOK;
    }

    vdpp_dump(p_proc_param, frame_idx);

    frame_idx++;

    if (is_vdpp2) {
        memcpy(p_proc_param->p_hist_buf, phist, VDPP_HIST_LENGTH);
    }

    p_proc_param->dci_vdpp_info.p_hist_addr     = p_proc_param->p_hist_buf;
    p_proc_param->dci_vdpp_info.hist_length     = VDPP_HIST_LENGTH;
    p_proc_param->dci_vdpp_info.vdpp_img_w_in   = p_proc_param->src_img_info.img_yrgb.w_vld;
    p_proc_param->dci_vdpp_info.vdpp_img_h_in   = p_proc_param->src_img_info.img_yrgb.h_vld;
    p_proc_param->dci_vdpp_info.vdpp_img_w_out  = p_proc_param->dst_img_info.img_yrgb.w_vld;
    p_proc_param->dci_vdpp_info.vdpp_img_h_out  = p_proc_param->dst_img_info.img_yrgb.h_vld;

    p_proc_param->dci_vdpp_info.vdpp_blk_size_h = p_proc_param->src_img_info.img_yrgb.w_vld / 16;
    p_proc_param->dci_vdpp_info.vdpp_blk_size_v = p_proc_param->src_img_info.img_yrgb.h_vld / 16;

    hwpq_vdpp_leave();

    return MPP_OK;
}

int hwpq_vdpp_check_work_mode(rk_vdpp_context ctx, rk_vdpp_proc_params *p_proc_param)
{
    RK_S32 cap_mode = VDPP_CAP_UNSUPPORTED;
    VdppCtxImpl *p = (VdppCtxImpl*)ctx;
    vdpp_com_ctx* vdpp = NULL;
    int run_mode = VDPP_RUN_MODE_UNSUPPORTED;
    MPP_RET ret = MPP_NOK;

    if (NULL == ctx || NULL == p_proc_param) {
        mpp_err_f("found NULL vdpp %p proc_param %p", ctx, p_proc_param);
        return VDPP_RUN_MODE_UNSUPPORTED;
    }

    vdpp = p->vdpp;
    if (NULL == vdpp || NULL == vdpp->ops) {
        mpp_err_f("found NULL vdpp or ops");
        return VDPP_RUN_MODE_UNSUPPORTED;
    }

    ret = hwpq_vdpp_common_config(vdpp, p_proc_param);
    if (ret) {
        mpp_err("vdpp common config failed\n");
        return VDPP_RUN_MODE_UNSUPPORTED;
    }

    if (vdpp->ops->check_cap)
        cap_mode = vdpp->ops->check_cap(vdpp->priv);

    hwpq_vdpp_info("vdpp cap_mode %d", cap_mode);
    /* vep first */
    if (VDPP_CAP_VEP & cap_mode)
        run_mode = VDPP_RUN_MODE_VEP;
    else if (VDPP_CAP_HIST & cap_mode)
        run_mode = VDPP_RUN_MODE_HIST;

    return run_mode;
}
