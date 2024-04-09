/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H265E_VEPU510_REG_H__
#define __HAL_H265E_VEPU510_REG_H__

#include "rk_type.h"
#include "vepu510_common.h"

typedef struct PreCstPar_t {
    struct {
        RK_U32 madi_thd0    : 7;
        RK_U32 reserved     : 1;
        RK_U32 madi_thd1    : 7;
        RK_U32 reserved1    : 1;
        RK_U32 madi_thd2    : 7;
        RK_U32 reserved2    : 1;
        RK_U32 madi_thd3    : 7;
        RK_U32 reserved3    : 1;
    } cst_madi_thd0;

    /* 0x000020c4 reg2097 */
    struct {
        RK_U32 madi_thd4    : 7;
        RK_U32 reserved     : 1;
        RK_U32 madi_thd5    : 7;
        RK_U32 reserved1    : 1;
        RK_U32 madi_thd6    : 7;
        RK_U32 reserved2    : 1;
        RK_U32 madi_thd7    : 7;
        RK_U32 reserved3    : 1;
    } cst_madi_thd1;

    /* 0x000020c8 reg2098 */
    struct {
        RK_U32 madi_thd8    : 7;
        RK_U32 reserved     : 1;
        RK_U32 madi_thd9    : 7;
        RK_U32 reserved1    : 1;
        RK_U32 madi_thd10   : 7;
        RK_U32 reserved2    : 1;
        RK_U32 madi_thd11   : 7;
        RK_U32 reserved3    : 1;
    } cst_madi_thd2;

    /* 0x000020cc reg2099 */
    struct {
        RK_U32 madi_thd12   : 7;
        RK_U32 reserved     : 1;
        RK_U32 madi_thd13   : 7;
        RK_U32 reserved1    : 1;
        RK_U32 mode_th      : 3;
        RK_U32 reserved2    : 1;
        RK_U32 qp_thd       : 6;
        RK_U32 reserved3    : 6;
    } cst_madi_thd3;

    /* 0x000020d0 reg2100 */
    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } cst_wgt0;

    /* 0x000020d4 reg2101 */
    struct {
        RK_U32 wgt4    : 8;
        RK_U32 wgt5    : 8;
        RK_U32 wgt6    : 8;
        RK_U32 wgt7    : 8;
    } cst_wgt1;

    /* 0x000020d8 reg2102 */
    struct {
        RK_U32 wgt8     : 8;
        RK_U32 wgt9     : 8;
        RK_U32 wgt10    : 8;
        RK_U32 wgt11    : 8;
    } cst_wgt2;

    /* 0x000020dc reg2103 */
    struct {
        RK_U32 wgt12            : 8;
        RK_U32 wgt13            : 8;
        RK_U32 wgt14            : 8;
        RK_U32 lambda_mv_bit_0  : 3;
        RK_U32 reserved         : 1;
        RK_U32 lambda_mv_bit_1  : 3;
        RK_U32 anti_strp_e      : 1;
    } cst_wgt3;
} pre_cst_par;

/* class: buffer/video syntax */
/* 0x00000270 reg156 - 0x000003f4 reg253*/
typedef struct H265eVepu510Frame_t {
    Vepu510FrmCommon    common;

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 ltm_col                        : 1;
        RK_U32 ltm_idx0l0                     : 1;
        RK_U32 chrm_spcl                      : 1;
        RK_U32 cu_inter_e                     : 12;
        RK_U32 reserved                       : 8;
        RK_U32 ccwa_e                         : 1;
        RK_U32 scl_lst_sel                    : 2;
        RK_U32 lambda_qp_use_avg_cu16_flag    : 1;
        RK_U32 yuvskip_calc_en                : 1;
        RK_U32 atf_e                          : 1;
        RK_U32 atr_e                          : 1;
        RK_U32 reserved1                      : 2;
    } rdo_cfg;

    /* 0x000003a4 reg233 */
    struct {
        RK_U32 rdo_mark_mode    : 9;
        RK_U32 reserved         : 23;
    } iprd_csts;

    /* 0x3a8 - 0x3ac */
    RK_U32 reserved234_235[2];

    /* 0x000003b0 reg236 */
    struct {
        RK_U32 nal_unit_type    : 6;
        RK_U32 reserved         : 26;
    } synt_nal;

    /* 0x000003b4 reg237 */
    struct {
        RK_U32 smpl_adpt_ofst_e    : 1;
        RK_U32 num_st_ref_pic      : 7;
        RK_U32 lt_ref_pic_prsnt    : 1;
        RK_U32 num_lt_ref_pic      : 6;
        RK_U32 tmpl_mvp_e          : 1;
        RK_U32 log2_max_poc_lsb    : 4;
        RK_U32 strg_intra_smth     : 1;
        RK_U32 reserved            : 11;
    } synt_sps;

    /* 0x000003b8 reg238 */
    struct {
        RK_U32 dpdnt_sli_seg_en       : 1;
        RK_U32 out_flg_prsnt_flg      : 1;
        RK_U32 num_extr_sli_hdr       : 3;
        RK_U32 sgn_dat_hid_en         : 1;
        RK_U32 cbc_init_prsnt_flg     : 1;
        RK_U32 pic_init_qp            : 6;
        RK_U32 cu_qp_dlt_en           : 1;
        RK_U32 chrm_qp_ofst_prsn      : 1;
        RK_U32 lp_fltr_acrs_sli       : 1;
        RK_U32 dblk_fltr_ovrd_en      : 1;
        RK_U32 lst_mdfy_prsnt_flg     : 1;
        RK_U32 sli_seg_hdr_extn       : 1;
        RK_U32 cu_qp_dlt_depth        : 2;
        RK_U32 lpf_fltr_acrs_til      : 1;
        RK_U32 csip_flag              : 1;
        RK_U32 reserved               : 9;
    } synt_pps;

    /* 0x000003bc reg239 */
    struct {
        RK_U32 cbc_init_flg           : 1;
        RK_U32 mvd_l1_zero_flg        : 1;
        RK_U32 reserved               : 3;
        RK_U32 ref_pic_lst_mdf_l0     : 1;
        RK_U32 num_refidx_l1_act      : 2;
        RK_U32 num_refidx_l0_act      : 2;
        RK_U32 num_refidx_act_ovrd    : 1;
        RK_U32 sli_sao_chrm_flg       : 1;
        RK_U32 sli_sao_luma_flg       : 1;
        RK_U32 sli_tmprl_mvp_e        : 1;
        RK_U32 pic_out_flg            : 1;
        RK_U32 sli_type               : 2;
        RK_U32 sli_rsrv_flg           : 7;
        RK_U32 dpdnt_sli_seg_flg      : 1;
        RK_U32 sli_pps_id             : 6;
        RK_U32 no_out_pri_pic         : 1;
    } synt_sli0;

    /* 0x000003c0 reg240 */
    struct {
        RK_U32 sp_tc_ofst_div2         : 4;
        RK_U32 sp_beta_ofst_div2       : 4;
        RK_U32 sli_lp_fltr_acrs_sli    : 1;
        RK_U32 sp_dblk_fltr_dis        : 1;
        RK_U32 dblk_fltr_ovrd_flg      : 1;
        RK_U32 sli_cb_qp_ofst          : 5;
        RK_U32 sli_qp                  : 6;
        RK_U32 max_mrg_cnd             : 2;
        RK_U32 reserved                : 1;
        RK_U32 col_ref_idx             : 1;
        RK_U32 col_frm_l0_flg          : 1;
        RK_U32 lst_entry_l0            : 4;
        RK_U32 reserved1               : 1;
    } synt_sli1;

    /* 0x000003c4 reg241 */
    struct {
        RK_U32 sli_poc_lsb        : 16;
        RK_U32 sli_hdr_ext_len    : 9;
        RK_U32 reserved           : 7;
    } synt_sli2;

    /* 0x000003c8 reg242 */
    struct {
        RK_U32 st_ref_pic_flg    : 1;
        RK_U32 poc_lsb_lt0       : 16;
        RK_U32 lt_idx_sps        : 5;
        RK_U32 num_lt_pic        : 2;
        RK_U32 st_ref_pic_idx    : 6;
        RK_U32 num_lt_sps        : 2;
    } synt_refm0;

    /* 0x000003cc reg243 */
    struct {
        RK_U32 used_by_s0_flg        : 4;
        RK_U32 num_pos_pic           : 1;
        RK_U32 num_negative_pics     : 5;
        RK_U32 dlt_poc_msb_cycl0     : 16;
        RK_U32 dlt_poc_msb_prsnt0    : 1;
        RK_U32 dlt_poc_msb_prsnt1    : 1;
        RK_U32 dlt_poc_msb_prsnt2    : 1;
        RK_U32 used_by_lt_flg0       : 1;
        RK_U32 used_by_lt_flg1       : 1;
        RK_U32 used_by_lt_flg2       : 1;
    } synt_refm1;

    /* 0x000003d0 reg244 */
    struct {
        RK_U32 dlt_poc_s0_m10    : 16;
        RK_U32 dlt_poc_s0_m11    : 16;
    } synt_refm2;

    /* 0x000003d4 reg245 */
    struct {
        RK_U32 dlt_poc_s0_m12    : 16;
        RK_U32 dlt_poc_s0_m13    : 16;
    } synt_refm3;

    /* 0x000003d8 reg246 */
    struct {
        RK_U32 poc_lsb_lt1    : 16;
        RK_U32 poc_lsb_lt2    : 16;
    } synt_long_refm0;

    /* 0x000003dc reg247 */
    struct {
        RK_U32 dlt_poc_msb_cycl1    : 16;
        RK_U32 dlt_poc_msb_cycl2    : 16;
    } synt_long_refm1;

    struct {
        RK_U32 sao_lambda_multi    : 3;
        RK_U32 reserved            : 29;
    } sao_cfg;

    /* 0x3e4 - 0x3ec */
    RK_U32 reserved249_251[3];

    /* 0x000003f0 reg252 */
    struct {
        RK_U32 tile_w_m1    : 9;
        RK_U32 reserved     : 7;
        RK_U32 tile_h_m1    : 9;
        RK_U32 reserved1    : 6;
        RK_U32 tile_en      : 1;
    } tile_cfg;

    /* 0x000003f4 reg253 */
    struct {
        RK_U32 tile_x       : 9;
        RK_U32 reserved     : 7;
        RK_U32 tile_y       : 9;
        RK_U32 reserved1    : 7;
    } tile_pos_hevc;
} H265eVepu510Frame;

/* class: param */
/* 0x00001700 reg1472 - 0x000019cc reg1651 */
typedef struct H265eVepu510Param_t {
    /* 0x00001700 reg1472 - 0x0000172c reg1483*/
    RK_U32 reserved_1472_1483[12];

    /* 0x00001730 reg1484 */
    struct {
        RK_U32    qnt_f_bias_i  : 10;
        RK_U32    qnt_f_bias_p  : 10;
        RK_U32    reserve       : 12;
    } qnt_bias_comb;

    /* 0x00001734 reg1485 - 0x0000175c reg1495*/
    RK_U32 reserved1485_1495[11];

    /* 0x00001760 reg1496 */
    struct {
        RK_U32 cime_pmv_num      : 1;
        RK_U32 cime_fuse         : 1;
        RK_U32 itp_mode          : 1;
        RK_U32 reserved          : 1;
        RK_U32 move_lambda       : 4;
        RK_U32 rime_lvl_mrg      : 2;
        RK_U32 rime_prelvl_en    : 2;
        RK_U32 rime_prersu_en    : 3;
        RK_U32 reserved1         : 17;
    } me_sqi_comb;

    /* 0x00001764 reg1497 */
    struct {
        RK_U32 cime_mvd_th0    : 9;
        RK_U32 reserved        : 1;
        RK_U32 cime_mvd_th1    : 9;
        RK_U32 reserved1       : 1;
        RK_U32 cime_mvd_th2    : 9;
        RK_U32 reserved2       : 3;
    }  cime_mvd_th_comb;

    /* 0x00001768 reg1498 */
    struct {
        RK_U32 cime_madp_th    : 12;
        RK_U32 reserved        : 20;
    } cime_madp_th_comb;

    /* 0x0000176c reg1499 */
    struct {
        RK_U32 cime_multi0    : 8;
        RK_U32 cime_multi1    : 8;
        RK_U32 cime_multi2    : 8;
        RK_U32 cime_multi3    : 8;
    } cime_multi_comb;

    /* 0x00001770 reg1500 */
    struct {
        RK_U32 rime_mvd_th0    : 3;
        RK_U32 reserved        : 1;
        RK_U32 rime_mvd_th1    : 3;
        RK_U32 reserved1       : 9;
        RK_U32 fme_madp_th     : 12;
        RK_U32 reserved2       : 4;
    } rime_mvd_th_comb;

    /* 0x00001774 reg1501 */
    struct {
        RK_U32 rime_madp_th0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 rime_madp_th1    : 12;
        RK_U32 reserved1        : 4;
    } rime_madp_th_comb;

    /* 0x00001778 reg1502 */
    struct {
        RK_U32 rime_multi0    : 10;
        RK_U32 rime_multi1    : 10;
        RK_U32 rime_multi2    : 10;
        RK_U32 reserved       : 2;
    } rime_multi_comb;

    /* 0x0000177c reg1503 */
    struct {
        RK_U32 cmv_th0     : 8;
        RK_U32 cmv_th1     : 8;
        RK_U32 cmv_th2     : 8;
        RK_U32 reserved    : 8;
    } cmv_st_th_comb;

    /* 0x1780 - 0x17fc */
    RK_U32 reserved1504_1535[32];

    /* 0x00001800 reg1536 - 0x000018cc reg1587*/
    RK_U32 pprd_lamb_satd_0_51[52];

    /* 0x000018d0 reg1588 */
    struct {
        RK_U32 lambda_satd_offset    : 5;
        RK_U32 reserved              : 27;
    } iprd_lamb_satd_ofst;

    /* 0x18d4 - 0x18fc */
    RK_U32 reserved1589_1599[11];

    /* 0x00001900 reg1600 - 0x000019cc reg1651*/
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} H265eVepu510Param;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x000020fc reg2111 */
typedef struct H265eVepu510SqiCfg_t {
    /* 0x00002000 reg2048 */
    struct {
        RK_U32 subj_opt_en            : 1;
        RK_U32 subj_opt_strength      : 3;
        RK_U32 aq_subj_en             : 1;
        RK_U32 aq_subj_strength       : 3;
        RK_U32 reserved               : 4;
        RK_U32 thre_sum_grdn_point    : 20;
    } subj_opt_cfg;

    /* 0x00002004 reg2049 */
    struct {
        RK_U32 common_thre_num_grdn_point_dep0    : 8;
        RK_U32 common_thre_num_grdn_point_dep1    : 8;
        RK_U32 common_thre_num_grdn_point_dep2    : 8;
        RK_U32 reserved                           : 8;
    } subj_opt_dpth_thd;

    /* 0x00002008 reg2050 */
    struct {
        RK_U32 common_rdo_cu_intra_r_coef_dep0    : 8;
        RK_U32 common_rdo_cu_intra_r_coef_dep1    : 8;
        RK_U32 reserved                           : 16;
    } subj_opt_inrar_coef;

    /* 0x200c */
    RK_U32 reserved_2051;

    /* 0x00002010 reg2052 */
    struct {
        RK_U32 anti_smear_en                  : 1;
        RK_U32 frm_static                     : 1;
        RK_U32 smear_stor_en                  : 1;
        RK_U32 smear_load_en                  : 1;
        RK_U32 smear_strength                 : 3;
        RK_U32 reserved                       : 1;
        RK_U32 thre_mv_inconfor_cime          : 4;
        RK_U32 thre_mv_confor_cime            : 2;
        RK_U32 thre_mv_confor_cime_gmv        : 2;
        RK_U32 thre_mv_inconfor_cime_gmv      : 4;
        RK_U32 thre_num_mv_confor_cime        : 2;
        RK_U32 thre_num_mv_confor_cime_gmv    : 2;
        RK_U32 reserved1                      : 8;
    } smear_opt_cfg0;

    /* 0x00002014 reg2053 */
    struct {
        RK_U32 dist0_frm_avg               : 14;
        RK_U32 thre_dsp_static             : 5;
        RK_U32 thre_dsp_mov                : 5;
        RK_U32 thre_dist_mv_confor_cime    : 7;
        RK_U32 reserved                    : 1;
    } smear_opt_cfg1;

    /* 0x00002018 reg2054 */
    struct {
        RK_U32 thre_madp_stc_dep0    : 4;
        RK_U32 thre_madp_stc_dep1    : 4;
        RK_U32 thre_madp_stc_dep2    : 4;
        RK_U32 thre_madp_mov_dep0    : 6;
        RK_U32 thre_madp_mov_dep1    : 6;
        RK_U32 thre_madp_mov_dep2    : 6;
        RK_U32 reserved              : 2;
    } smear_madp_thd;

    /* 0x0000201c reg2055 */
    struct {
        RK_U32 thre_num_pt_stc_dep0    : 6;
        RK_U32 thre_num_pt_stc_dep1    : 4;
        RK_U32 thre_num_pt_stc_dep2    : 2;
        RK_U32 reserved                : 4;
        RK_U32 thre_num_pt_mov_dep0    : 6;
        RK_U32 thre_num_pt_mov_dep1    : 4;
        RK_U32 thre_num_pt_mov_dep2    : 2;
        RK_U32 reserved1               : 4;
    } smear_stat_thd;

    /* 0x00002020 reg2056 */
    struct {
        RK_U32 thre_ratio_dist_mv_confor_cime_gmv0      : 5;
        RK_U32 reserved                                 : 3;
        RK_U32 thre_ratio_dist_mv_confor_cime_gmv1      : 5;
        RK_U32 reserved1                                : 3;
        RK_U32 thre_ratio_dist_mv_inconfor_cime_gmv0    : 6;
        RK_U32 reserved2                                : 2;
        RK_U32 thre_ratio_dist_mv_inconfor_cime_gmv1    : 6;
        RK_U32 reserved3                                : 2;
    } smear_bmv_dist_thd0;

    /* 0x00002024 reg2057 */
    struct {
        RK_U32 thre_ratio_dist_mv_inconfor_cime_gmv2    : 6;
        RK_U32 reserved                                 : 2;
        RK_U32 thre_ratio_dist_mv_inconfor_cime_gmv3    : 6;
        RK_U32 reserved1                                : 2;
        RK_U32 thre_ratio_dist_mv_inconfor_cime_gmv4    : 6;
        RK_U32 reserved2                                : 10;
    } smear_bmv_dist_thd1;

    /* 0x00002028 reg2058 */
    struct {
        RK_U32 thre_min_num_confor_csu0_bndry_cime_gmv      : 2;
        RK_U32 thre_max_num_confor_csu0_bndry_cime_gmv      : 2;
        RK_U32 thre_min_num_inconfor_csu0_bndry_cime_gmv    : 2;
        RK_U32 thre_max_num_inconfor_csu0_bndry_cime_gmv    : 2;
        RK_U32 thre_split_dep0                              : 2;
        RK_U32 thre_zero_srgn                               : 5;
        RK_U32 reserved                                     : 1;
        RK_U32 madi_thre_dep0                               : 8;
        RK_U32 madi_thre_dep1                               : 8;
    } smear_min_bndry_gmv;

    /* 0x0000202c reg2059 */
    struct {
        RK_U32 thre_madp_stc_cover0    : 6;
        RK_U32 thre_madp_stc_cover1    : 6;
        RK_U32 thre_madp_mov_cover0    : 5;
        RK_U32 thre_madp_mov_cover1    : 5;
        RK_U32 smear_qp_strength       : 4;
        RK_U32 smear_thre_qp           : 6;
    } smear_madp_cov_thd;

    /* 0x00002030 reg2060 */
    struct {
        RK_U32 skin_en                        : 1;
        RK_U32 skin_strength                  : 3;
        RK_U32 thre_uvsqr16_skin              : 8;
        RK_U32 skin_thre_cst_best_mad         : 10;
        RK_U32 skin_thre_cst_best_grdn_blk    : 7;
        RK_U32 reserved                       : 1;
        RK_U32 frame_skin_ratio               : 2;
    } skin_opt_cfg;

    /* 0x00002034 reg2061 */
    struct {
        RK_U32 thre_sum_mad_intra         : 2;
        RK_U32 thre_sum_grdn_blk_intra    : 2;
        RK_U32 vld_thre_skin_v            : 3;
        RK_U32 reserved                   : 1;
        RK_U32 thre_min_skin_u            : 8;
        RK_U32 thre_max_skin_u            : 8;
        RK_U32 thre_min_skin_v            : 8;
    } skin_chrm_thd;

    /* 0x00002038 reg2062 */
    struct {
        RK_U32 block_en                        : 1;
        RK_U32 reserved                        : 1;
        RK_U32 block_thre_cst_best_mad         : 10;
        RK_U32 reserved1                       : 4;
        RK_U32 block_thre_cst_best_grdn_blk    : 6;
        RK_U32 reserved2                       : 2;
        RK_U32 thre_num_grdnt_point_cmplx      : 2;
        RK_U32 block_delta_qp_flag             : 2;
        RK_U32 reserved3                       : 4;
    } block_opt_cfg;

    /* 0x0000203c reg2063 */
    struct {
        RK_U32 cmplx_thre_cst_best_mad_dep0    : 13;
        RK_U32 reserved                        : 3;
        RK_U32 cmplx_thre_cst_best_mad_dep1    : 13;
        RK_U32 reserved1                       : 2;
        RK_U32 cmplx_en                        : 1;
    } cmplx_opt_cfg;

    /* 0x00002040 reg2064 */
    struct {
        RK_U32 cmplx_thre_cst_best_mad_dep2         : 13;
        RK_U32 reserved                             : 3;
        RK_U32 cmplx_thre_cst_best_grdn_blk_dep0    : 11;
        RK_U32 reserved1                            : 5;
    } cmplx_bst_mad_thd;

    /* 0x00002044 reg2065 */
    struct {
        RK_U32 cmplx_thre_cst_best_grdn_blk_dep1    : 11;
        RK_U32 reserved                             : 5;
        RK_U32 cmplx_thre_cst_best_grdn_blk_dep2    : 11;
        RK_U32 reserved1                            : 5;
    } cmplx_bst_grdn_thd;

    /* 0x00002048 reg2066 */
    struct {
        RK_U32 line_en                                 : 1;
        RK_U32 line_thre_min_cst_best_grdn_blk_dep0    : 5;
        RK_U32 line_thre_min_cst_best_grdn_blk_dep1    : 5;
        RK_U32 line_thre_min_cst_best_grdn_blk_dep2    : 5;
        RK_U32 line_thre_ratio_best_grdn_blk_dep0      : 4;
        RK_U32 line_thre_ratio_best_grdn_blk_dep1      : 4;
        RK_U32 reserved                                : 8;
    } line_opt_cfg;

    /* 0x0000204c reg2067 */
    struct {
        RK_U32 line_thre_max_cst_best_grdn_blk_dep0    : 7;
        RK_U32 reserved                                : 1;
        RK_U32 line_thre_max_cst_best_grdn_blk_dep1    : 7;
        RK_U32 reserved1                               : 1;
        RK_U32 line_thre_max_cst_best_grdn_blk_dep2    : 7;
        RK_U32 reserved2                               : 9;
    } line_cst_bst_grdn;

    /* 0x00002050 reg2068 */
    struct {
        RK_U32 line_thre_qp               : 6;
        RK_U32 block_strength             : 3;
        RK_U32 block_thre_qp              : 6;
        RK_U32 cmplx_strength             : 3;
        RK_U32 cmplx_thre_qp              : 6;
        RK_U32 cmplx_thre_max_grdn_blk    : 6;
        RK_U32 reserved                   : 2;
    } subj_opt_dqp0;

    /* 0x00002054 reg2069 */
    struct {
        RK_U32 skin_thre_qp                      : 6;
        RK_U32 reserved                          : 2;
        RK_U32 bndry_rdo_cu_intra_r_coef_dep0    : 8;
        RK_U32 bndry_rdo_cu_intra_r_coef_dep1    : 8;
        RK_U32 reserved1                         : 8;
    } subj_opt_dqp1;

    /* 0x2058 - 0x205c */
    RK_U32 reserved2070_2071[2];

    /* 0x00002060 reg2072 - 0x0000206c reg2075 */
    rdo_skip_par rdo_b32_skip;

    /* 0x00002070 reg2076 - 0x0000207c reg2079*/
    rdo_skip_par rdo_b16_skip;

    /* 0x00002080 reg2080 - 0x00002088 reg2082 */
    rdo_noskip_par rdo_b32_inter;

    /* 0x0000208c reg2083 - 0x00002094 reg2085 */
    rdo_noskip_par rdo_b16_inter;

    /* 0x00002098 reg2086 - 0x000020a4 reg2088 */
    rdo_noskip_par rdo_b32_intra;

    /* 0x000020a8 reg2089 - 0x000020ac reg2091 */
    rdo_noskip_par rdo_b16_intra;

    /* 0x000020b0 reg2092 */
    struct {
        RK_U32 blur_low_madi_thd     : 7;
        RK_U32 reserved              : 1;
        RK_U32 blur_high_madi_thd    : 7;
        RK_U32 reserved1             : 1;
        RK_U32 blur_low_cnt_thd      : 4;
        RK_U32 blur_hight_cnt_thd    : 4;
        RK_U32 blur_sum_cnt_thd      : 4;
        RK_U32 anti_blur_en          : 1;
        RK_U32 reserved2             : 3;
    } subj_anti_blur_thd;

    /* 0x000020b4 reg2093 */
    struct {
        RK_U32 blur_motion_thd           : 12;
        RK_U32 sao_ofst_thd_eo_luma      : 3;
        RK_U32 reserved                  : 1;
        RK_U32 sao_ofst_thd_bo_luma      : 3;
        RK_U32 reserved1                 : 1;
        RK_U32 sao_ofst_thd_eo_chroma    : 3;
        RK_U32 reserved2                 : 1;
        RK_U32 sao_ofst_thd_bo_chroma    : 3;
        RK_U32 reserved3                 : 5;
    } subj_anti_blur_sao;

    /* 0x000020b8 reg2094 - 0x000020bc reg2095*/
    RK_U32 reserved_2094_2095[2];

    /* 0x000020c0 reg2096 - 0x000020dc reg2103 */
    pre_cst_par preintra32_cst;

    /* 0x000020e0 reg2104 - 0x000020fc reg2111 */
    pre_cst_par preintra16_cst;

    /* 0x00002100 reg2112 */
    struct {
        RK_U32 base_thre_rough_mad32_intra           : 4;
        RK_U32 delta0_thre_rough_mad32_intra         : 4;
        RK_U32 delta1_thre_rough_mad32_intra         : 6;
        RK_U32 delta2_thre_rough_mad32_intra         : 6;
        RK_U32 delta3_thre_rough_mad32_intra         : 7;
        RK_U32 delta4_thre_rough_mad32_intra_low5    : 5;
    } cudecis_thd0;

    /* 0x00002104 reg2113 */
    struct {
        RK_U32 delta4_thre_rough_mad32_intra_high2    : 2;
        RK_U32 delta5_thre_rough_mad32_intra          : 7;
        RK_U32 delta6_thre_rough_mad32_intra          : 7;
        RK_U32 base_thre_fine_mad32_intra             : 4;
        RK_U32 delta0_thre_fine_mad32_intra           : 4;
        RK_U32 delta1_thre_fine_mad32_intra           : 5;
        RK_U32 delta2_thre_fine_mad32_intra_low3      : 3;
    } cudecis_thd1;

    /* 0x00002108 reg2114 */
    struct {
        RK_U32 delta2_thre_fine_mad32_intra_high2    : 2;
        RK_U32 delta3_thre_fine_mad32_intra          : 5;
        RK_U32 delta4_thre_fine_mad32_intra          : 5;
        RK_U32 delta5_thre_fine_mad32_intra          : 6;
        RK_U32 delta6_thre_fine_mad32_intra          : 6;
        RK_U32 base_thre_str_edge_mad32_intra        : 3;
        RK_U32 delta0_thre_str_edge_mad32_intra      : 2;
        RK_U32 delta1_thre_str_edge_mad32_intra      : 3;
    } cudecis_thd2;

    /* 0x0000210c reg2115 */
    struct {
        RK_U32 delta2_thre_str_edge_mad32_intra      : 3;
        RK_U32 delta3_thre_str_edge_mad32_intra      : 4;
        RK_U32 base_thre_str_edge_bgrad32_intra      : 5;
        RK_U32 delta0_thre_str_edge_bgrad32_intra    : 2;
        RK_U32 delta1_thre_str_edge_bgrad32_intra    : 3;
        RK_U32 delta2_thre_str_edge_bgrad32_intra    : 4;
        RK_U32 delta3_thre_str_edge_bgrad32_intra    : 5;
        RK_U32 base_thre_mad16_intra                 : 3;
        RK_U32 delta0_thre_mad16_intra               : 3;
    } cudecis_thd3;

    /* 0x00002110 reg2116 */
    struct {
        RK_U32 delta1_thre_mad16_intra          : 3;
        RK_U32 delta2_thre_mad16_intra          : 4;
        RK_U32 delta3_thre_mad16_intra          : 5;
        RK_U32 delta4_thre_mad16_intra          : 5;
        RK_U32 delta5_thre_mad16_intra          : 6;
        RK_U32 delta6_thre_mad16_intra          : 6;
        RK_U32 delta0_thre_mad16_ratio_intra    : 3;
    } cudecis_thd4;

    /* 0x00002114 reg2117 */
    struct {
        RK_U32 delta1_thre_mad16_ratio_intra           : 3;
        RK_U32 delta2_thre_mad16_ratio_intra           : 3;
        RK_U32 delta3_thre_mad16_ratio_intra           : 3;
        RK_U32 delta4_thre_mad16_ratio_intra           : 3;
        RK_U32 delta5_thre_mad16_ratio_intra           : 3;
        RK_U32 delta6_thre_mad16_ratio_intra           : 3;
        RK_U32 delta7_thre_mad16_ratio_intra           : 3;
        RK_U32 delta0_thre_rough_bgrad32_intra         : 3;
        RK_U32 delta1_thre_rough_bgrad32_intra         : 4;
        RK_U32 delta2_thre_rough_bgrad32_intra_low4    : 4;
    } cudecis_thd5;

    /* 0x00002118 reg2118 */
    struct {
        RK_U32 delta2_thre_rough_bgrad32_intra_high2    : 2;
        RK_U32 delta3_thre_rough_bgrad32_intra          : 10;
        RK_U32 delta4_thre_rough_bgrad32_intra          : 10;
        RK_U32 delta5_thre_rough_bgrad32_intra_low10    : 10;
    } cudecis_thd6;

    /* 0x0000211c reg2119 */
    struct {
        RK_U32 delta5_thre_rough_bgrad32_intra_high1    : 1;
        RK_U32 delta6_thre_rough_bgrad32_intra          : 12;
        RK_U32 delta7_thre_rough_bgrad32_intra          : 13;
        RK_U32 delta0_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta1_thre_bgrad16_ratio_intra_low2     : 2;
    } cudecis_thd7;

    /* 0x00002120 reg2120 */
    struct {
        RK_U32 delta1_thre_bgrad16_ratio_intra_high2    : 2;
        RK_U32 delta2_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta3_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta4_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta5_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta6_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta7_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta0_thre_fme_ratio_inter              : 3;
        RK_U32 delta1_thre_fme_ratio_inter              : 3;
    } cudecis_thdt8;

    /* 0x00002124 reg2121 */
    struct {
        RK_U32 delta2_thre_fme_ratio_inter    : 3;
        RK_U32 delta3_thre_fme_ratio_inter    : 3;
        RK_U32 delta4_thre_fme_ratio_inter    : 3;
        RK_U32 delta5_thre_fme_ratio_inter    : 3;
        RK_U32 delta6_thre_fme_ratio_inter    : 3;
        RK_U32 delta7_thre_fme_ratio_inter    : 3;
        RK_U32 base_thre_fme32_inter          : 3;
        RK_U32 delta0_thre_fme32_inter        : 3;
        RK_U32 delta1_thre_fme32_inter        : 4;
        RK_U32 delta2_thre_fme32_inter        : 4;
    } cudecis_thd9;

    /* 0x00002128 reg2122 */
    struct {
        RK_U32 delta3_thre_fme32_inter    : 5;
        RK_U32 delta4_thre_fme32_inter    : 6;
        RK_U32 delta5_thre_fme32_inter    : 7;
        RK_U32 delta6_thre_fme32_inter    : 8;
        RK_U32 thre_cme32_inter           : 6;
    } cudecis_thd10;

    /* 0x0000212c reg2123 */
    struct {
        RK_U32 delta0_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta1_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta2_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta3_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta4_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta5_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta6_thre_mad_fme_ratio_inter    : 4;
        RK_U32 delta7_thre_mad_fme_ratio_inter    : 4;
    } cudecis_thd11;
} H265eVepu510Sqi;

typedef struct H265eV510RegSet_t {
    Vepu510ControlCfg         reg_ctl;
    H265eVepu510Frame         reg_frm;
    Vepu510RcRoi              reg_rc_roi;
    H265eVepu510Param         reg_param;
    H265eVepu510Sqi           reg_sqi;
    Vepu510Dbg                reg_dbg;
} H265eV510RegSet;

typedef struct H265eV510StatusElem_t {
    RK_U32 hw_status;
    Vepu510Status st;
} H265eV510StatusElem;

#endif
