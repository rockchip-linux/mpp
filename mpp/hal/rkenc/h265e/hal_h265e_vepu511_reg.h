/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H265E_VEPU511_REG_H__
#define __HAL_H265E_VEPU511_REG_H__

#include "rk_type.h"
#include "vepu511_common.h"

typedef struct PreCstPar_t {
    /* 0x00002120 reg2120 - 0x00002140 reg2128 */
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

    /* 0x00002124 reg2121 */
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

    /* 0x00002128 reg2122 */
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

    /* 0x0000212c reg2123 */
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

    /* 0x00002130 reg2124 */
    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } cst_wgt0;

    /* 0x00002134 reg2125 */
    struct {
        RK_U32 wgt4    : 8;
        RK_U32 wgt5    : 8;
        RK_U32 wgt6    : 8;
        RK_U32 wgt7    : 8;
    } cst_wgt1;

    /* 0x00002138 reg2126 */
    struct {
        RK_U32 wgt8     : 8;
        RK_U32 wgt9     : 8;
        RK_U32 wgt10    : 8;
        RK_U32 wgt11    : 8;
    } cst_wgt2;

    /* 0x0000213c reg2127 */
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
/* 0x00000270 reg156 - 0x00000538 reg334 */
typedef struct H265eVepu511Frame_t {
    /* 0x00000270 reg156  - 0x0000039c reg231 */
    Vepu511FrmCommon common;

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 ltm_col                     : 1;
        RK_U32 ltm_idx0l0                  : 1;
        RK_U32 chrm_spcl                   : 1;
        RK_U32 cu_inter_e                  : 12;
        RK_U32 reserved                    : 1;
        RK_U32 cu32_split_lambda_qp_sel    : 3;
        RK_U32 reserved1                   : 4;
        RK_U32 ccwa_e                      : 1;
        RK_U32 scl_lst_sel                 : 2;
        RK_U32 reserved2                   : 2;
        RK_U32 atf_e                       : 1;
        RK_U32 atr_e                       : 1;
        RK_U32 reserved3                   : 2;
    } rdo_cfg;

    struct {
        RK_U32 intra_pu32_mode_num    : 2;
        RK_U32 intra_pu16_mode_num    : 2;
        RK_U32 intra_pu8_mode_num     : 2;
        RK_U32 intra_pu4_mode_num     : 2;
        RK_U32 reserved               : 24;
    } rdo_intra_mode;

    /* 0x000003a8 reg234 */
    struct {
        RK_U32 rdoqx_pixel_e        : 1;
        RK_U32 rdoqx_cgzero_e       : 1;
        RK_U32 rdoqx_lastxy_e       : 1;
        RK_U32 rdoq4_rdoq_e         : 1;
        RK_U32 reserved             : 4;
        RK_U32 rdoq_intra_r_coef    : 5;
        RK_U32 reserved1            : 19;
    } rdoq_cfg_hevc;

    /* 0x3ac */
    RK_U32 reserved_235;

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
        RK_U32 mrg_up_flg             : 1;
        RK_U32 mrg_lft_flg            : 1;
        RK_U32 reserved               : 1;
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

    /* 0x000003e0 reg248 */
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

    /* 0x000003f8 reg254 */
    struct {
        RK_U32 slice_sta_x      : 9;
        RK_U32 reserved1        : 7;
        RK_U32 slice_sta_y      : 10;
        RK_U32 reserved2        : 5;
        RK_U32 slice_enc_ena    : 1;
    } slice_enc_cfg0;

    /* 0x000003fc reg255 */
    struct {
        RK_U32 slice_end_x    : 9;
        RK_U32 reserved       : 7;
        RK_U32 slice_end_y    : 10;
        RK_U32 reserved1      : 6;
    } slice_enc_cfg1;

    /* 0x00000400 reg256 */
    struct {
        RK_U32 reserved          : 8;
        RK_U32 bsbt_addr_jpeg    : 24;
    } adr_bsbt_jpeg;

    /* 0x00000404 reg257 */
    struct {
        RK_U32 reserved          : 8;
        RK_U32 bsbb_addr_jpeg    : 24;
    } adr_bsbb_jpeg;

    /* 0x00000408 reg258 */
    RK_U32 adr_bsbs_jpeg;

    /* 0x0000040c reg259 */
    struct {
        RK_U32 bsadr_msk_jpeg    : 4;
        RK_U32 reserved          : 4;
        RK_U32 bsbr_addr_jpeg    : 24;
    } adr_bsbr_jpeg;

    /* 0x00000410 reg260 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsy_b_jpeg    : 28;
    } adr_vsy_b_jpeg;

    /* 0x00000414 reg261 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsc_b_jpeg    : 28;
    } adr_vsc_b_jpeg;

    /* 0x00000418 reg262 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsy_t_jpeg    : 28;
    } adr_vsy_t_jpeg;

    /* 0x0000041c reg263 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsc_t_jpeg    : 28;
    } adr_vsc_t_jpeg;

    /* 0x00000420 reg264 */
    RK_U32 adr_src0_jpeg;

    /* 0x00000424 reg265 */
    RK_U32 adr_src1_jpeg;

    /* 0x00000428 reg266 */
    RK_U32 adr_src2_jpeg;

    /* 0x0000042c reg267 */
    RK_U32 bsp_size_jpeg;

    /* 0x430 - 0x43c */
    RK_U32 reserved268_271[4];

    /* 0x00000440 reg272 */
    struct {
        RK_U32 pic_wd8_m1    : 11;
        RK_U32 reserved      : 1;
        RK_U32 pp0_vnum_m1   : 4;
        RK_U32 pic_hd8_m1    : 11;
        RK_U32 reserved1     : 1;
        RK_U32 pp0_jnum_m1   : 4;
    } enc_rsl_jpeg;

    /* 0x00000444 reg273 */
    struct {
        RK_U32 pic_wfill_jpeg    : 6;
        RK_U32 reserved          : 10;
        RK_U32 pic_hfill_jpeg    : 6;
        RK_U32 reserved1         : 10;
    } src_fill_jpeg;

    /* 0x00000448 reg274 */
    struct {
        RK_U32 alpha_swap_jpeg            : 1;
        RK_U32 rbuv_swap_jpeg             : 1;
        RK_U32 src_cfmt_jpeg              : 4;
        RK_U32 reserved                   : 2;
        RK_U32 src_range_trns_en_jpeg     : 1;
        RK_U32 src_range_trns_sel_jpeg    : 1;
        RK_U32 chroma_ds_mode_jpeg        : 1;
        RK_U32 reserved1                  : 21;
    } src_fmt_jpeg;

    /* 0x0000044c reg275 */
    struct {
        RK_U32 csc_wgt_b2y_jpeg    : 9;
        RK_U32 csc_wgt_g2y_jpeg    : 9;
        RK_U32 csc_wgt_r2y_jpeg    : 9;
        RK_U32 reserved            : 5;
    } src_udfy_jpeg;

    /* 0x00000450 reg276 */
    struct {
        RK_U32 csc_wgt_b2u_jpeg    : 9;
        RK_U32 csc_wgt_g2u_jpeg    : 9;
        RK_U32 csc_wgt_r2u_jpeg    : 9;
        RK_U32 reserved            : 5;
    } src_udfu_jpeg;

    /* 0x00000454 reg277 */
    struct {
        RK_U32 csc_wgt_b2v_jpeg    : 9;
        RK_U32 csc_wgt_g2v_jpeg    : 9;
        RK_U32 csc_wgt_r2v_jpeg    : 9;
        RK_U32 reserved            : 5;
    } src_udfv_jpeg;

    /* 0x00000458 reg278 */
    struct {
        RK_U32 csc_ofst_v_jpeg    : 8;
        RK_U32 csc_ofst_u_jpeg    : 8;
        RK_U32 csc_ofst_y_jpeg    : 5;
        RK_U32 reserved           : 11;
    } src_udfo_jpeg;

    /* 0x0000045c reg279 */
    struct {
        RK_U32 cr_force_value_jpeg     : 8;
        RK_U32 cb_force_value_jpeg     : 8;
        RK_U32 chroma_force_en_jpeg    : 1;
        RK_U32 reserved                : 9;
        RK_U32 src_mirr_jpeg           : 1;
        RK_U32 src_rot_jpeg            : 2;
        RK_U32 reserved1               : 1;
        RK_U32 rkfbcd_en_jpeg          : 1;
        RK_U32 reserved2               : 1;
    } src_proc_jpeg;

    /* 0x00000460 reg280 */
    struct {
        RK_U32 pic_ofst_x_jpeg    : 14;
        RK_U32 reserved           : 2;
        RK_U32 pic_ofst_y_jpeg    : 14;
        RK_U32 reserved1          : 2;
    } pic_ofst_jpeg;

    /* 0x00000464 reg281 */
    struct {
        RK_U32 src_strd0_jpeg    : 21;
        RK_U32 reserved          : 11;
    } src_strd0_jpeg;

    /* 0x00000468 reg282 */
    struct {
        RK_U32 src_strd1_jpeg    : 16;
        RK_U32 reserved          : 16;
    } src_strd1_jpeg;

    /* 0x0000046c reg283 */
    struct {
        RK_U32 pp_corner_filter_strength_jpeg      : 2;
        RK_U32 reserved                            : 2;
        RK_U32 pp_edge_filter_strength_jpeg        : 2;
        RK_U32 reserved1                           : 2;
        RK_U32 pp_internal_filter_strength_jpeg    : 2;
        RK_U32 reserved2                           : 22;
    } src_flt_cfg_jpeg;

    /* 0x00000470 reg284 */
    struct {
        RK_U32 jpeg_bias_y    : 15;
        RK_U32 reserved       : 17;
    } jpeg_y_cfg;

    /* 0x00000474 reg285 */
    struct {
        RK_U32 jpeg_bias_u    : 15;
        RK_U32 reserved       : 17;
    } jpeg_u_cfg;

    /* 0x00000478 reg286 */
    struct {
        RK_U32 jpeg_bias_v    : 15;
        RK_U32 reserved       : 17;
    } jpeg_v_cfg;

    /* 0x0000047c reg287 */
    struct {
        RK_U32 jpeg_ri              : 25;
        RK_U32 jpeg_out_mode        : 1;
        RK_U32 jpeg_start_rst_m     : 3;
        RK_U32 jpeg_pic_last_ecs    : 1;
        RK_U32 reserved             : 1;
        RK_U32 jpeg_stnd            : 1;
    } jpeg_base_cfg;

    /* 0x00000480 reg288 */
    struct {
        RK_U32 uvc_partition0_len_jpeg    : 12;
        RK_U32 uvc_partition_len_jpeg     : 12;
        RK_U32 uvc_skip_len_jpeg          : 6;
        RK_U32 reserved                   : 2;
    } uvc_cfg_jpeg;

    /* 0x00000484 reg289 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 eslf_badr_jpeg    : 28;
    } adr_eslf_jpeg;

    /* 0x00000488 reg290 */
    struct {
        RK_U32 eslf_rptr_jpeg    : 10;
        RK_U32 eslf_wptr_jpeg    : 10;
        RK_U32 eslf_blen_jpeg    : 10;
        RK_U32 eslf_updt_jpeg    : 2;
    } eslf_buf_jpeg;

    /* 0x48c */
    RK_U32 reserved_291;

    /* 0x00000490 reg292 */
    struct {
        RK_U32 roi0_rdoq_start_x    : 11;
        RK_U32 roi0_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi0_rdoq_level      : 6;
        RK_U32 roi0_rdoq_en         : 1;
    } jpeg_roi0_cfg0;

    /* 0x00000494 reg293 */
    struct {
        RK_U32 roi0_rdoq_width_m1     : 11;
        RK_U32 roi0_rdoq_height_m1    : 11;
        RK_U32 reserved               : 3;
        RK_U32 frm_rdoq_level         : 6;
        RK_U32 frm_rdoq_en            : 1;
    } jpeg_roi0_cfg1;

    /* 0x00000498 reg294 */
    struct {
        RK_U32 roi1_rdoq_start_x    : 11;
        RK_U32 roi1_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi1_rdoq_level      : 6;
        RK_U32 roi1_rdoq_en         : 1;
    } jpeg_roi1_cfg0;

    /* 0x0000049c reg295 */
    struct {
        RK_U32 roi1_rdoq_width_m1     : 11;
        RK_U32 roi1_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi1_cfg1;

    /* 0x000004a0 reg296 */
    struct {
        RK_U32 roi2_rdoq_start_x    : 11;
        RK_U32 roi2_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi2_rdoq_level      : 6;
        RK_U32 roi2_rdoq_en         : 1;
    } jpeg_roi2_cfg0;

    /* 0x000004a4 reg297 */
    struct {
        RK_U32 roi2_rdoq_width_m1     : 11;
        RK_U32 roi2_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi2_cfg1;

    /* 0x000004a8 reg298 */
    struct {
        RK_U32 roi3_rdoq_start_x    : 11;
        RK_U32 roi3_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi3_rdoq_level      : 6;
        RK_U32 roi3_rdoq_en         : 1;
    } jpeg_roi3_cfg0;

    /* 0x000004ac reg299 */
    struct {
        RK_U32 roi3_rdoq_width_m1     : 11;
        RK_U32 roi3_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi3_cfg1;

    /* 0x000004b0 reg300 */
    struct {
        RK_U32 roi4_rdoq_start_x    : 11;
        RK_U32 roi4_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi4_rdoq_level      : 6;
        RK_U32 roi4_rdoq_en         : 1;
    } jpeg_roi4_cfg0;

    /* 0x000004b4 reg301 */
    struct {
        RK_U32 roi4_rdoq_width_m1     : 11;
        RK_U32 roi4_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi4_cfg1;

    /* 0x000004b8 reg302 */
    struct {
        RK_U32 roi5_rdoq_start_x    : 11;
        RK_U32 roi5_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi5_rdoq_level      : 6;
        RK_U32 roi5_rdoq_en         : 1;
    } jpeg_roi5_cfg0;

    /* 0x000004bc reg303 */
    struct {
        RK_U32 roi5_rdoq_width_m1     : 11;
        RK_U32 roi5_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi5_cfg1;

    /* 0x000004c0 reg304 */
    struct {
        RK_U32 roi6_rdoq_start_x    : 11;
        RK_U32 roi6_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi6_rdoq_level      : 6;
        RK_U32 roi6_rdoq_en         : 1;
    } jpeg_roi6_cfg0;

    /* 0x000004c4 reg305 */
    struct {
        RK_U32 roi6_rdoq_width_m1     : 11;
        RK_U32 roi6_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi6_cfg1;

    /* 0x000004c8 reg306 */
    struct {
        RK_U32 roi7_rdoq_start_x    : 11;
        RK_U32 roi7_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi7_rdoq_level      : 6;
        RK_U32 roi7_rdoq_en         : 1;
    } jpeg_roi7_cfg0;

    /* 0x000004cc reg307 */
    struct {
        RK_U32 roi7_rdoq_width_m1     : 11;
        RK_U32 roi7_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi7_cfg1;

    /* 0x000004d0 reg308 */
    struct {
        RK_U32 roi8_rdoq_start_x    : 11;
        RK_U32 roi8_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi8_rdoq_level      : 6;
        RK_U32 roi8_rdoq_en         : 1;
    } jpeg_roi8_cfg0;

    /* 0x000004d4 reg309 */
    struct {
        RK_U32 roi8_rdoq_width_m1     : 11;
        RK_U32 roi8_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi8_cfg1;

    /* 0x000004d8 reg310 */
    struct {
        RK_U32 roi9_rdoq_start_x    : 11;
        RK_U32 roi9_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi9_rdoq_level      : 6;
        RK_U32 roi9_rdoq_en         : 1;
    } jpeg_roi9_cfg0;

    /* 0x000004dc reg311 */
    struct {
        RK_U32 roi9_rdoq_width_m1     : 11;
        RK_U32 roi9_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi9_cfg1;

    /* 0x000004e0 reg312 */
    struct {
        RK_U32 roi10_rdoq_start_x    : 11;
        RK_U32 roi10_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi10_rdoq_level      : 6;
        RK_U32 roi10_rdoq_en         : 1;
    } jpeg_roi10_cfg0;

    /* 0x000004e4 reg313 */
    struct {
        RK_U32 roi10_rdoq_width_m1     : 11;
        RK_U32 roi10_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi10_cfg1;

    /* 0x000004e8 reg314 */
    struct {
        RK_U32 roi11_rdoq_start_x    : 11;
        RK_U32 roi11_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi11_rdoq_level      : 6;
        RK_U32 roi11_rdoq_en         : 1;
    } jpeg_roi11_cfg0;

    /* 0x000004ec reg315 */
    struct {
        RK_U32 roi11_rdoq_width_m1     : 11;
        RK_U32 roi11_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi11_cfg1;

    /* 0x000004f0 reg316 */
    struct {
        RK_U32 roi12_rdoq_start_x    : 11;
        RK_U32 roi12_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi12_rdoq_level      : 6;
        RK_U32 roi12_rdoq_en         : 1;
    } jpeg_roi12_cfg0;

    /* 0x000004f4 reg317 */
    struct {
        RK_U32 roi12_rdoq_width_m1     : 11;
        RK_U32 roi12_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi12_cfg1;

    /* 0x000004f8 reg318 */
    struct {
        RK_U32 roi13_rdoq_start_x    : 11;
        RK_U32 roi13_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi13_rdoq_level      : 6;
        RK_U32 roi13_rdoq_en         : 1;
    } jpeg_roi13_cfg0;

    /* 0x000004fc reg319 */
    struct {
        RK_U32 roi13_rdoq_width_m1     : 11;
        RK_U32 roi13_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi13_cfg1;

    /* 0x00000500 reg320 */
    struct {
        RK_U32 roi14_rdoq_start_x    : 11;
        RK_U32 roi14_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi14_rdoq_level      : 6;
        RK_U32 roi14_rdoq_en         : 1;
    } jpeg_roi14_cfg0;

    /* 0x00000504 reg321 */
    struct {
        RK_U32 roi14_rdoq_width_m1     : 11;
        RK_U32 roi14_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi14_cfg1;

    /* 0x00000508 reg322 */
    struct {
        RK_U32 roi15_rdoq_start_x    : 11;
        RK_U32 roi15_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi15_rdoq_level      : 6;
        RK_U32 roi15_rdoq_en         : 1;
    } jpeg_roi15_cfg0;

    /* 0x0000050c reg323 */
    struct {
        RK_U32 roi15_rdoq_width_m1     : 11;
        RK_U32 roi15_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi15_cfg1;

    /* 0x510 - 0x51c */
    RK_U32 reserved324_327[4];

    /* 0x00000520 reg328 */
    struct {
        RK_U32 reserved        : 4;
        RK_U32 base_addr_md    : 28;
    } adr_md_vpp;

    /* 0x00000524 reg329 */
    struct {
        RK_U32 reserved        : 4;
        RK_U32 base_addr_od    : 28;
    } adr_od_vpp;

    /* 0x00000528 reg330 */
    struct {
        RK_U32 reserved             : 4;
        RK_U32 base_addr_ref_mdw    : 28;
    } adr_ref_mdw;

    /* 0x0000052c reg331 */
    struct {
        RK_U32 reserved             : 4;
        RK_U32 base_addr_ref_mdr    : 28;
    } adr_ref_mdr;

    /* 0x00000530 reg332 */
    struct {
        RK_U32 sto_stride_md          : 8;
        RK_U32 sto_stride_od          : 8;
        RK_U32 cur_frm_en_md          : 1;
        RK_U32 ref_frm_en_md          : 1;
        RK_U32 switch_sad_md          : 2;
        RK_U32 night_mode_en_md       : 1;
        RK_U32 flycatkin_flt_en_md    : 1;
        RK_U32 en_od                  : 1;
        RK_U32 background_en_od       : 1;
        RK_U32 sad_comp_en_od         : 1;
        RK_U32 reserved               : 6;
        RK_U32 vepu_pp_en             : 1;
    } vpp_base_cfg;

    /* 0x00000534 reg333 */
    struct {
        RK_U32 thres_sad_md          : 12;
        RK_U32 thres_move_md         : 3;
        RK_U32 reserved              : 1;
        RK_U32 thres_dust_move_md    : 4;
        RK_U32 thres_dust_blk_md     : 3;
        RK_U32 reserved1             : 1;
        RK_U32 thres_dust_chng_md    : 8;
    } thd_md_vpp;

    /* 0x00000538 reg334 */
    struct {
        RK_U32 thres_complex_od        : 12;
        RK_U32 thres_complex_cnt_od    : 3;
        RK_U32 thres_sad_od            : 14;
        RK_U32 reserved                : 3;
    } thd_od_vpp;
} H265eVepu511Frame;

/* class: param */
/* 0x00001700 reg1472 - 0x000019cc reg1651*/
typedef struct H265eVepu511Param_t {
    /* 0x00001700 reg1472 - 0x0000172c reg1483*/
    RK_U32 reserved_1472_1483[12];

    /* 0x00001730 reg1484 */
    struct {
        RK_U32 bias_madi_th0    : 8;
        RK_U32 bias_madi_th1    : 8;
        RK_U32 bias_madi_th2    : 8;
        RK_U32 reserved         : 8;
    } bias_madi_thd_comb;

    /* 0x00001734 reg1485 */
    struct {
        RK_U32 bias_i_val0    : 10;
        RK_U32 bias_i_val1    : 10;
        RK_U32 bias_i_val2    : 10;
        RK_U32 reserved       : 2;
    } qnt0_i_bias_comb;

    /* 0x00001738 reg1486 */
    struct {
        RK_U32 bias_i_val3    : 10;
        RK_U32 reserved       : 22;
    } qnt1_i_bias_comb;

    /* 0x0000173c reg1487 */
    struct {
        RK_U32 bias_p_val0    : 10;
        RK_U32 bias_p_val1    : 10;
        RK_U32 bias_p_val2    : 10;
        RK_U32 reserved       : 2;
    } qnt0_p_bias_comb;

    /* 0x00001740 reg1488 */
    struct {
        RK_U32 bias_p_val3    : 10;
        RK_U32 reserved       : 22;
    } qnt1_p_bias_comb;

    /* 0x00001744 reg1489 */
    struct {
        RK_U32 light_change_en         : 1;
        RK_U32 light_ratio_mult1       : 5;
        RK_U32 light_ratio_mult2       : 4;
        RK_U32 light_thre_csu1_cnt     : 2;
        RK_U32 srch_rgn_en             : 1;
        RK_U32 reserved                : 3;
        RK_U32 light_thre_madp         : 8;
        RK_U32 light_thre_lightmadp    : 8;
    } light_cfg_hevc;

    /* 0x1748 - 0x175c */
    RK_U32 reserved1490_1495[6];

    /* 0x00001760 reg1496 */
    struct {
        RK_U32 cime_pmv_num      : 1;
        RK_U32 cime_fuse         : 1;
        RK_U32 reserved          : 2;
        RK_U32 move_lambda       : 4;
        RK_U32 rime_lvl_mrg      : 2;
        RK_U32 rime_prelvl_en    : 2;
        RK_U32 rime_prersu_en    : 3;
        RK_U32 fme_lvl_mrg       : 1;
        RK_U32 reserved1         : 16;
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
        RK_U32 cime_madp_th       : 12;
        RK_U32 ratio_consi_cfg    : 4;
        RK_U32 ratio_bmv_dist     : 4;
        RK_U32 reserved           : 12;
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
        RK_U32 lambda_luma_offset      : 5;
        RK_U32 lambda_chroma_offset    : 5;
        RK_U32 reserved                : 22;
    } prmd_intra_lamb_ofst;

    /* 0x18d4 - 0x18fc */
    RK_U32 reserved1589_1599[11];

    /* 0x00001900 reg1600 - 0x000019cc reg1651*/
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} H265eVepu511Param;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x00002160 reg2136 */
typedef struct H265eVepu511SqiCfg_t {
    /* 0x00002000 reg2048 */
    struct {
        RK_U32 subj_opt_en                     : 1;
        RK_U32 subj_opt_strength               : 3;
        RK_U32 aq_subj_en                      : 1;
        RK_U32 aq_subj_strength                : 3;
        RK_U32 bndry_cmplx_static_choose_en    : 2;
        RK_U32 feature_cal_en                  : 1;
        RK_U32 reserved                        : 1;
        RK_U32 thre_sum_grdn_point             : 20;
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
        RK_U32 cover_rdo_mode_intra_jcoef_d0    : 6;
        RK_U32 cover_rdo_mode_intra_jcoef_d1    : 6;
        RK_U32 cover_rmd_mode_intra_jcoef_d0    : 6;
        RK_U32 cover_rmd_mode_intra_jcoef_d1    : 6;
        RK_U32 cover_rdoq_rcoef_d0              : 4;
        RK_U32 cover_rdoq_rcoef_d1              : 4;
    } subj_opt_inrar_coef;

    /* 0x0000200c reg2051 */
    struct {
        RK_U32 cfc_rmd_mode_intra_jcoef_d0    : 6;
        RK_U32 cfc_rmd_mode_intra_jcoef_d1    : 6;
        RK_U32 cfc_rdo_mode_intra_jcoef_d0    : 6;
        RK_U32 cfc_rdo_mode_intra_jcoef_d1    : 6;
        RK_U32 cfc_rdoq_rcoef_d0              : 4;
        RK_U32 cfc_rdoq_rcoef_d1              : 4;
    } smear_opt_cfc_coef;

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
        RK_U32 ref1_subj_opt_en               : 1;
        RK_U32 smear_cfc_en                   : 1;
        RK_U32 reserved1                      : 6;
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
        RK_U32 confor_cime_gmv0      : 5;
        RK_U32 reserved              : 3;
        RK_U32 confor_cime_gmv1      : 5;
        RK_U32 reserved1             : 3;
        RK_U32 inconfor_cime_gmv0    : 6;
        RK_U32 reserved2             : 2;
        RK_U32 inconfor_cime_gmv1    : 6;
        RK_U32 reserved3             : 2;
    } smear_bmv_dist_thd0;

    /* 0x00002024 reg2057 */
    struct {
        RK_U32 inconfor_cime_gmv2    : 6;
        RK_U32 reserved              : 2;
        RK_U32 inconfor_cime_gmv3    : 6;
        RK_U32 reserved1             : 2;
        RK_U32 inconfor_cime_gmv4    : 6;
        RK_U32 reserved2             : 10;
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
        RK_U32 line_thre_min_cst_best_grdn_blk_dep1    : 8;
        RK_U32 line_thre_min_cst_best_grdn_blk_dep2    : 8;
        RK_U32 line_thre_ratio_best_grdn_blk_dep0      : 4;
        RK_U32 line_thre_ratio_best_grdn_blk_dep1      : 4;
        RK_U32 reserved                                : 2;
    } line_opt_cfg;

    /* 0x0000204c reg2067 */
    struct {
        RK_U32 line_thre_max_cst_best_grdn_blk_dep0    : 7;
        RK_U32 reserved                                : 1;
        RK_U32 line_thre_max_cst_best_grdn_blk_dep1    : 8;
        RK_U32 line_thre_max_cst_best_grdn_blk_dep2    : 8;
        RK_U32 reserved1                               : 8;
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
        RK_U32 skin_thre_qp                     : 6;
        RK_U32 smear_frame_thre_qp              : 6;
        RK_U32 bndry_rdo_mode_intra_jcoef_d0    : 6;
        RK_U32 bndry_rdo_mode_intra_jcoef_d1    : 6;
        RK_U32 skin_thre_madp                   : 8;
    } subj_opt_dqp1;

    /* 0x00002058 reg2070 */
    struct {
        RK_U32 line_rdo_split_rcoef_d0    : 5;
        RK_U32 line_rdo_split_rcoef_d1    : 5;
        RK_U32 choose_cu32_split_jcoef    : 6;
        RK_U32 choose_cu16_split_jcoef    : 5;
        RK_U32 reserved                   : 11;
    } subj_opt_rdo_split;

    /* 0x0000205c reg2071 */
    struct {
        RK_U32 lid_grdn_blk_cu16_th       : 8;
        RK_U32 lid_rmd_intra_jcoef_ang    : 5;
        RK_U32 lid_rdo_intra_rcoef_ang    : 5;
        RK_U32 lid_rmd_intra_jcoef_dp     : 6;
        RK_U32 lid_rdo_intra_rcoef_dp     : 6;
        RK_U32 lid_en                     : 1;
        RK_U32 reserved                   : 1;
    } line_intra_dir_cfg;

    /* 0x00002060 reg2072 -  0x0000206c reg 2076*/
    rdo_b32_skip_par rdo_b32_skip;

    /* 0x00002070 reg2076 - 0x0000207c reg2079*/
    rdo_skip_par rdo_b16_skip;

    /* 0x00002080 reg2080 - 0x00002088 reg2082 */
    rdo_b32_noskip_par rdo_b32_inter;

    /* 0x0000208c reg2083 - 0x00002094 reg2085 */
    rdo_noskip_par rdo_b16_inter;

    /* 0x00002098 reg2086 - 0x000020a0 reg2087 */
    rdo_noskip_par rdo_b32_intra;

    /* 0x000020a4 reg2088 - 0x000020ac reg2091 */
    rdo_noskip_par rdo_b16_intra;

    /* 0x000020b0 reg2092 */
    struct {
        RK_U32 ref1_rmd_mode_lr_jcoef_d0    : 5;
        RK_U32 ref1_rmd_mode_lr_jcoef_d1    : 5;
        RK_U32 ref1_rdo_mode_lr_jcoef_d0    : 5;
        RK_U32 ref1_rdo_mode_lr_jcoef_d1    : 5;
        RK_U32 ref1_rmd_mv_lr_jcoef_d0      : 5;
        RK_U32 ref1_rmd_mv_lr_jcoef_d1      : 5;
        RK_U32 reserved                     : 2;
    } smear_ref1_cfg0;

    /* 0x000020b4 reg2093 */
    struct {
        RK_U32 ref1_rdo_inter_tu_res_joef_d0    : 5;
        RK_U32 ref1_rdo_inter_tu_res_joef_d1    : 5;
        RK_U32 ref1_rdoq_zero_mv_rcoef_d0       : 5;
        RK_U32 ref1_rdoq_zero_mv_rcoef_d1       : 5;
        RK_U32 ref1_rmd_inter_lr_jcoef_d0       : 5;
        RK_U32 ref1_rmd_inter_lr_jcoef_d1       : 5;
        RK_U32 reserved                         : 2;
    } smear_ref1_cfg1;

    /* 0x000020b8 reg2094 - 0x000020bc reg2095*/
    RK_U32 reserved_2094_2095[2];

    /* 0x000020c0 reg2096 */
    struct {
        RK_U32 thre_max_luma_dark        : 8;
        RK_U32 thre_min_luma_bright      : 8;
        RK_U32 thre_ratio_dark_bright    : 6;
        RK_U32 reserved                  : 2;
        RK_U32 thre_qp_dark_bright       : 6;
        RK_U32 reserved1                 : 1;
        RK_U32 dark_bright_en            : 1;
    } dark_brgt_opt_cfg;

    /* 0x000020c4 reg2097 */
    struct {
        RK_U32 madp_th_dep0_dark_bright    : 8;
        RK_U32 madp_th_dep1_dark_bright    : 8;
        RK_U32 madi_th_dep0_dark_bright    : 6;
        RK_U32 reserved                    : 2;
        RK_U32 madi_th_dep1_dark_bright    : 6;
        RK_U32 reserved1                   : 2;
    } dark_brgt_madi_thd;

    /* 0x000020c8 reg2098 */
    struct {
        RK_U32 dark_bright_inter_res_j_coef_wgt_dep0    : 8;
        RK_U32 dark_bright_inter_res_j_coef_wgt_dep1    : 8;
        RK_U32 dark_bright_intra_j_coef_wgt_dep0        : 8;
        RK_U32 dark_bright_intra_j_coef_wgt_dep1        : 8;
    } dark_brgt_wgt0;

    /* 0x000020cc reg2099 */
    struct {
        RK_U32 dark_bright_split_rcoef_d0    : 6;
        RK_U32 reserved                      : 2;
        RK_U32 dark_bright_split_rcoef_d1    : 6;
        RK_U32 reserved1                     : 18;
    } dark_brgt_wgt1;

    /* 0x000020d0 reg2100 */
    struct {
        RK_U32 cmplx_static_en               : 1;
        RK_U32 cmplx_static_lgt_chng_en      : 1;
        RK_U32 thre_qp_cmplx_static          : 6;
        RK_U32 madp_th0_dep0_cmplx_static    : 6;
        RK_U32 madp_th1_dep0_cmplx_static    : 8;
        RK_U32 madp_th2_dep0_cmplx_static    : 10;
    } cmplx_statc_cfg;

    /* 0x000020d4 reg2101 */
    struct {
        RK_U32 num_grdn_point_th1_dep0_cmplx_static    : 8;
        RK_U32 num_grdn_point_th2_dep0_cmplx_static    : 8;
        RK_U32 madi_th1_dep0_cmplx_static              : 6;
        RK_U32 reserved                                : 2;
        RK_U32 madi_th2_dep0_cmplx_static              : 6;
        RK_U32 reserved1                               : 2;
    } cmplx_statc_thd0;

    /* 0x000020d8 reg2102 */
    struct {
        RK_U32 madp_th0_dep1_cmplx_static           : 6;
        RK_U32 madp_th1_dep1_cmplx_static           : 8;
        RK_U32 madp_th2_dep1_cmplx_static           : 10;
        RK_U32 static_num_thre_dep1_cmplx_static    : 2;
        RK_U32 srch_rgn_mv_th_cmplx_static          : 5;
        RK_U32 reserved                             : 1;
    } cmplx_statc_thd1;

    /* 0x000020dc reg2103 */
    struct {
        RK_U32 num_grdn_point_th1_dep1_cmplx_static    : 6;
        RK_U32 num_grdn_point_th2_dep1_cmplx_static    : 6;
        RK_U32 madi_th1_dep1_cmplx_static              : 6;
        RK_U32 madi_th2_dep1_cmplx_static              : 6;
        RK_U32 num_cu16_th                : 3;
        RK_U32 frame_qp_en                : 1;
        RK_U32 ratio_light_madp_th        : 2;
        RK_U32 reserved                   : 2;
    } cmplx_statc_thd2;

    /* 0x000020e0 reg2104 */
    struct {
        RK_U32 inter_res_j_coef_wgt1_dep0    : 8;
        RK_U32 inter_res_j_coef_wgt2_dep0    : 8;
        RK_U32 inter_res_j_coef_wgt1_dep1    : 8;
        RK_U32 inter_res_j_coef_wgt2_dep1    : 8;
    } cmplx_statc_wgt0;

    /* 0x000020e4 reg2105 */
    struct {
        RK_U32 intra_j_coef_wgt1_dep0    : 8;
        RK_U32 intra_j_coef_wgt2_dep0    : 8;
        RK_U32 intra_j_coef_wgt1_dep1    : 8;
        RK_U32 intra_j_coef_wgt2_dep1    : 8;
    } cmplx_statc_wgt1;

    /* 0x000020e8 reg2106 */
    struct {
        RK_U32 split_rcoef_w1d0    : 6;
        RK_U32 split_rcoef_w1d1    : 6;
        RK_U32 split_rcoef_w2d0    : 6;
        RK_U32 split_rcoef_w2d1    : 6;
        RK_U32 reserved            : 8;
    } cmplx_statc_wgt2;

    /* 0x20ec - 0x20fc */
    RK_U32 reserved2107_2111[5];

    /* 0x00002100 reg2112 */
    struct {
        RK_U32 blur_low_madi_thd     : 7;
        RK_U32 reserved              : 1;
        RK_U32 blur_high_madi_thd    : 7;
        RK_U32 reserved1             : 1;
        RK_U32 blur_low_cnt_thd      : 4;
        RK_U32 blur_hight_cnt_thd    : 4;
        RK_U32 blur_sum_cnt_thd      : 4;
        RK_U32 anti_blur_en          : 1;
        RK_U32 scene_mode            : 1;
        RK_U32 reserved2             : 2;
    } subj_anti_blur_thd;

    /* 0x00002104 reg2113 */
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

    /* 0x00002108 reg2114 */
    struct {
        RK_U32 notmerge_ofst_dist_eo_wgt0    : 8;
        RK_U32 notmerge_ofst_dist_bo_wgt0    : 8;
        RK_U32 notmerge_ofst_dist_eo_wgt1    : 8;
        RK_U32 notmerge_ofst_dist_bo_wgt1    : 8;
    } subj_anti_blur_wgt0;

    /* 0x0000210c reg2115 */
    struct {
        RK_U32 notmerge_ofst_lambda_eo_wgt0     : 8;
        RK_U32 notmerge_ofst_lambda_bo_wgt0     : 8;
        RK_U32 notmerge_compare_dist_eo_wgt0    : 8;
        RK_U32 notmerge_compare_dist_bo_wgt0    : 8;
    } subj_anti_blur_wgt1;

    /* 0x00002110 reg2116 */
    struct {
        RK_U32 notmerge_compare_dist_eo_wgt1    : 8;
        RK_U32 notmerge_compare_dist_bo_wgt1    : 8;
        RK_U32 notmerge_compare_rate_eo_wgt0    : 8;
        RK_U32 notmerge_compare_rate_bo_wgt0    : 8;
    } subj_anti_blur_wgt2;

    /* 0x00002114 reg2117 */
    struct {
        RK_U32 sao_mode_compare_dist_eo_wgt0    : 8;
        RK_U32 sao_mode_compare_dist_bo_wgt0    : 8;
        RK_U32 merge_cost_dist_eo_wgt0          : 8;
        RK_U32 merge_cost_dist_bo_wgt0          : 8;
    } subj_anti_blur_wgt3;

    /* 0x00002118 reg2118 */
    struct {
        RK_U32 merge_cost_dist_eo_wgt1    : 8;
        RK_U32 merge_cost_dist_bo_wgt1    : 8;
        RK_U32 merge_cost_bit_eo_wgt0     : 8;
        RK_U32 merge_cost_bit_bo_wgt0     : 8;
    } subj_anti_blur_wgt4;

    /* 0x211c */
    RK_U32 reserved_2119;

    /* 0x00002120 reg2120 - 0x0000213c reg2127 */
    pre_cst_par preintra32_cst;

    /* 0x00002140 reg2128 - 0x0000215c reg2135 */
    pre_cst_par preintra16_cst;

    /* 0x00002160 reg2136 */
    struct {
        RK_U32 offset_thd                      : 4;
        RK_U32 offset_diff_thd                 : 4;
        RK_U32 weak_texture_offset_thd         : 3;
        RK_U32 weak_texture_offset_diff_thd    : 3;
        RK_U32 reserved                        : 18;
    } atr_thd_hevc;
} H265eVepu511Sqi;

/* class: scaling list  */
/* 0x00002200 reg2176- 0x00002c9c reg2855*/
typedef struct H265eVepu511SclCfg_t {
    /* 0x2200 - 0x2584 iq_scal_y8_intra_ac0 ~ iq_scal_list_dc1 only HEVC*/
    RK_U32 tu8_intra_y[16];
    RK_U32 tu8_intra_u[16];
    RK_U32 tu8_intra_v[16];
    RK_U32 tu8_inter_y[16];
    RK_U32 tu8_inter_u[16];
    RK_U32 tu8_inter_v[16];
    RK_U32 tu16_intra_y_ac[16];
    RK_U32 tu16_intra_u_ac[16];
    RK_U32 tu16_intra_v_ac[16];
    RK_U32 tu16_inter_y_ac[16];
    RK_U32 tu16_inter_u_ac[16];
    RK_U32 tu16_inter_v_ac[16];
    RK_U32 tu32_intra_y_ac[16];
    RK_U32 tu32_inter_y_ac[16];

    /* 0x2580 */
    struct {
        RK_U32 tu16_intra_y_dc  : 8;
        RK_U32 tu16_intra_u_dc  : 8;
        RK_U32 tu16_intra_v_dc  : 8;
        RK_U32 tu16_inter_y_dc  : 8;
    } tu_dc0;

    /* 0x2584 reg 2401*/
    struct {
        RK_U32 tu16_inter_u_dc  : 8;
        RK_U32 tu16_inter_v_dc  : 8;
        RK_U32 tu32_intra_y_dc  : 8;
        RK_U32 tu32_inter_y_dc  : 8;
    } tu_dc1;

    /* 0x2588 reg 2402 - 0x258c reg 2403*/
    RK_U32 reserved2402_2403[2];

    /* 0x2590 reg 2404 - 0x2c9c reg 2855*/
    RK_U32 q_y8_intra[32];
    RK_U32 q_u8_intra[32];
    RK_U32 q_v8_intra[32];
    RK_U32 q_y8_inter[32];
    RK_U32 q_u8_inter[32];
    RK_U32 q_v8_inter[32];
    RK_U32 q_y16_intra[32];
    RK_U32 q_u16_intra[32];
    RK_U32 q_v16_intra[32];
    RK_U32 q_y16_inter[32];
    RK_U32 q_u16_inter[32];
    RK_U32 q_v16_inter[32];
    RK_U32 q_y32_intra[32];
    RK_U32 q_y32_inter[32];

    RK_U32 q_list[4];
} H265eVepu511SclCfg;

typedef struct H265eV511RegSet_t {
    Vepu511ControlCfg         reg_ctl;
    H265eVepu511Frame         reg_frm;
    Vepu511RcRoi              reg_rc_roi;
    H265eVepu511Param         reg_param;
    H265eVepu511Sqi           reg_sqi;
    H265eVepu511SclCfg        reg_scl;
    Vepu511OsdRegs            reg_osd;
    Vepu511Dbg                reg_dbg;
} H265eV511RegSet;

typedef struct H265eV511StatusElem_t {
    RK_U32 hw_status;
    Vepu511Status st;
} H265eV511StatusElem;

#endif
