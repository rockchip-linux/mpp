/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_H265E_VEPU511_REG_H
#define HAL_H265E_VEPU511_REG_H

#include "rk_type.h"
#include "vepu511_common.h"

/* class: buffer/video syntax */
/* 0x00000270 reg156 -0x000003fc reg0255 */
typedef struct HevcVepu511Frame_t {
    /* 0x00000270 reg156 - 0x0000027c reg159 */
    vepu511_online online_addr;

    /* 0x00000280 reg160 */
    RK_U32 reg0160_adr_src0;

    /* 0x00000284 reg161 */
    RK_U32 reg0161_adr_src1;

    /* 0x00000288 reg162 */
    RK_U32 reg0162_adr_src2;

    /* 0x0000028c reg163 */
    RK_U32 reg0163_rfpw_h_addr;

    /* 0x00000290 reg164 */
    RK_U32 reg0164_rfpw_b_addr;

    /* 0x00000294 reg165 */
    RK_U32 reg0165_rfpr_h_addr;

    /* 0x00000298 reg166 */
    RK_U32 reg0166_rfpr_b_addr;

    /* 0x0000029c reg167 */
    RK_U32 reg0167_cmvw_addr;

    /* 0x000002a0 reg168 */
    RK_U32 reg0168_cmvr_addr;

    /* 0x000002a4 reg169 */
    RK_U32 reg0169_dspw_addr;

    /* 0x000002a8 reg170 */
    RK_U32 reg0170_dspr_addr;

    /* 0x000002ac reg171 */
    RK_U32 reg0171_meiw_addr;

    /* 0x000002b0 reg172 */
    RK_U32 reg0172_bsbt_addr;

    /* 0x000002b4 reg173 */
    RK_U32 reg0173_bsbb_addr;

    /* 0x000002b8 reg174 */
    RK_U32 reg0174_bsbs_addr;

    /* 0x000002bc reg175 */
    RK_U32 reg0175_bsbr_addr;

    /* 0x000002c0 reg176 */
    RK_U32 reg0176_lpfw_addr;

    /* 0x000002c4 reg177 */
    RK_U32 reg0177_lpfr_addr;

    /* 0x000002c8 reg178 */
    RK_U32 reg0178_adr_ebuft;

    /* 0x000002cc reg179 */
    RK_U32 reg0179_adr_ebufb;

    /* 0x000002d0 reg180 */
    RK_U32 reg0180_adr_rfpt_h;

    /* 0x000002d4 reg181 */
    RK_U32 reg0181_adr_rfpb_h;

    /* 0x000002d8 reg182 */
    RK_U32 reg0182_adr_rfpt_b;

    /* 0x000002dc reg183 */
    RK_U32 reg0183_adr_rfpb_b;

    /* 0x000002e0 reg184 */
    RK_U32 reg0184_adr_smr_rd;

    /* 0x000002e4 reg185 */
    RK_U32 reg0185_adr_smr_wr;

    /* 0x000002e8 reg186 */
    RK_U32 reg0186_adr_roir;

    /* 0x000002ec reg187 */
    RK_U32 reg0187_eslf_badr;

    /* 0x000002f0 reg188 */
    RK_U32 reg0188_rfp1r_h_addr;

    /* 0x000002f4 reg189 */
    RK_U32 reg0189_rfp1r_b_addr;

    /* 0x000002f8 reg190 */
    RK_U32 reg0190_dsp1r_addr;

    /* 0x000002fc reg191 */
    RK_U32 reserved191;

    /* 0x00000300 reg192 */
    struct {
        RK_U32 enc_stnd                : 2;
        RK_U32 cur_frm_ref             : 1;
        RK_U32 mei_stor                : 1;
        RK_U32 bs_scp                  : 1;
        RK_U32 meiw_mode               : 1;
        RK_U32 reserved                : 2;
        RK_U32 pic_qp                  : 6;
        RK_U32 num_pic_tot_cur_hevc    : 5;
        RK_U32 log2_ctu_num_hevc       : 5;
        RK_U32 rfpr_compress_mode      : 1;
        RK_U32 reserved1               : 2;
        RK_U32 eslf_out_e_jpeg         : 1;
        RK_U32 jpeg_slen_fifo          : 1;
        RK_U32 eslf_out_e              : 1;
        RK_U32 slen_fifo               : 1;
        RK_U32 rec_fbc_dis             : 1;
    } reg0192_enc_pic;

    /* 0x00000304 reg193 */
    struct {
        RK_U32 dchs_txid    : 2;
        RK_U32 dchs_rxid    : 2;
        RK_U32 dchs_txe     : 1;
        RK_U32 dchs_rxe     : 1;
        RK_U32 reserved     : 2;
        RK_U32 dchs_dly     : 8;
        RK_U32 dchs_ofst    : 10;
        RK_U32 reserved1    : 6;
    } reg0193_dual_core;

    /* 0x00000308 reg194 */
    struct {
        RK_U32 frame_id        : 8;
        RK_U32 frm_id_match    : 1;
        RK_U32 reserved        : 3;
        RK_U32 source_id       : 1;
        RK_U32 src_id_match    : 1;
        RK_U32 reserved1       : 2;
        RK_U32 ch_id           : 2;
        RK_U32 vrsp_rtn_en     : 1;
        RK_U32 vinf_req_en     : 1;
        RK_U32 reserved2       : 12;
    } reg0194_enc_id;

    /* 0x0000030c reg195 */
    RK_U32 bsp_size;

    /* 0x00000310 reg196 */
    struct {
        RK_U32 pic_wd8_m1    : 11;
        RK_U32 reserved      : 5;
        RK_U32 pic_hd8_m1    : 11;
        RK_U32 reserved1     : 5;
    } reg0196_enc_rsl;

    /* 0x00000314 reg197 */
    struct {
        RK_U32 pic_wfill    : 6;
        RK_U32 reserved     : 10;
        RK_U32 pic_hfill    : 6;
        RK_U32 reserved1    : 10;
    } reg0197_src_fill;

    /* 0x00000318 reg198 */
    struct {
        RK_U32 alpha_swap            : 1;
        RK_U32 rbuv_swap             : 1;
        RK_U32 src_cfmt              : 4;
        RK_U32 src_rcne              : 1;
        RK_U32 out_fmt               : 1;
        RK_U32 src_range_trns_en     : 1;
        RK_U32 src_range_trns_sel    : 1;
        RK_U32 chroma_ds_mode        : 1;
        RK_U32 reserved              : 21;
    }  reg0198_src_fmt;

    /* 0x0000031c reg199 */
    struct {
        RK_U32 csc_wgt_b2y    : 9;
        RK_U32 csc_wgt_g2y    : 9;
        RK_U32 csc_wgt_r2y    : 9;
        RK_U32 reserved       : 5;
    } reg0199_src_udfy;

    /* 0x00000320 reg200 */
    struct {
        RK_U32 csc_wgt_b2u    : 9;
        RK_U32 csc_wgt_g2u    : 9;
        RK_U32 csc_wgt_r2u    : 9;
        RK_U32 reserved       : 5;
    } reg0200_src_udfu;

    /* 0x00000324 reg201 */
    struct {
        RK_U32 csc_wgt_b2v    : 9;
        RK_U32 csc_wgt_g2v    : 9;
        RK_U32 csc_wgt_r2v    : 9;
        RK_U32 reserved       : 5;
    } reg0201_src_udfv;

    /* 0x00000328 reg202 */
    struct {
        RK_U32 csc_ofst_v    : 8;
        RK_U32 csc_ofst_u    : 8;
        RK_U32 csc_ofst_y    : 5;
        RK_U32 reserved      : 11;
    } reg0202_src_udfo;

    /* 0x0000032c reg203 */
    struct {
        RK_U32 cr_force_value     : 8;
        RK_U32 cb_force_value     : 8;
        RK_U32 chroma_force_en    : 1;
        RK_U32 reserved           : 9;
        RK_U32 src_mirr           : 1;
        RK_U32 src_rot            : 2;
        RK_U32 tile4x4_en         : 1;
        RK_U32 rkfbcd_en          : 1;
        RK_U32 reserved1          : 1;
    } reg0203_src_proc;

    /* 0x00000330 reg204 */
    struct {
        RK_U32 pic_ofst_x    : 14;
        RK_U32 reserved      : 2;
        RK_U32 pic_ofst_y    : 14;
        RK_U32 reserved1     : 2;
    } reg0204_pic_ofst;

    /* 0x00000334 reg205 */
    struct {
        RK_U32 src_strd0    : 21;
        RK_U32 reserved     : 11;
    } reg0205_src_strd0;

    /* 0x00000338 reg206 */
    struct {
        RK_U32 src_strd1    : 16;
        RK_U32 reserved     : 16;
    } reg0206_src_strd1;

    /* 0x0000033c reg207 */
    struct {
        RK_U32 pp_corner_filter_strength      : 2;
        RK_U32 reserved                       : 2;
        RK_U32 pp_edge_filter_strength        : 2;
        RK_U32 reserved1                      : 2;
        RK_U32 pp_internal_filter_strength    : 2;
        RK_U32 reserved2                      : 22;
    } reg0207_src_flt_cfg;

    /* 0x340 - 0x34c */
    RK_U32 reserved208_211[4];

    /* 0x00000350 reg212 */
    struct {
        RK_U32 rc_en         : 1;
        RK_U32 aq_en         : 1;
        RK_U32 reserved      : 10;
        RK_U32 rc_ctu_num    : 20;
    } reg212_rc_cfg;

    /* 0x00000354 reg213 */
    struct {
        RK_U32 reserved       : 16;
        RK_U32 rc_qp_range    : 4;
        RK_U32 rc_max_qp      : 6;
        RK_U32 rc_min_qp      : 6;
    } reg213_rc_qp;

    /* 0x00000358 reg214 */
    struct {
        RK_U32 ctu_ebit    : 20;
        RK_U32 reserved    : 12;
    } reg214_rc_tgt;

    /* 0x0000035c reg215 */
    struct {
        RK_U32 eslf_rptr    : 10;
        RK_U32 eslf_wptr    : 10;
        RK_U32 eslf_blen    : 10;
        RK_U32 eslf_updt    : 2;
    } eslf_buf;

    /* 0x00000360 reg216 */
    struct {
        RK_U32 sli_splt          : 1;
        RK_U32 sli_splt_mode     : 1;
        RK_U32 sli_splt_cpst     : 1;
        RK_U32 reserved          : 12;
        RK_U32 sli_flsh          : 1;
        RK_U32 sli_max_num_m1    : 15;
        RK_U32 reserved1         : 1;
    } reg0216_sli_splt;

    /* 0x00000364 reg217 */
    struct {
        RK_U32 sli_splt_byte    : 20;
        RK_U32 reserved         : 12;
    } reg0217_sli_byte;

    /* 0x00000368 reg218 */
    struct {
        RK_U32 sli_splt_cnum_m1    : 20;
        RK_U32 reserved            : 12;
    } reg0218_sli_cnum;

    /* 0x0000036c reg219 */
    struct {
        RK_U32 uvc_partition0_len    : 12;
        RK_U32 uvc_partition_len     : 12;
        RK_U32 uvc_skip_len          : 6;
        RK_U32 reserved              : 2;
    } vbs_pad;

    /* 0x00000370 reg220 */
    struct {
        RK_U32 cime_srch_dwnh    : 4;
        RK_U32 cime_srch_uph     : 4;
        RK_U32 cime_srch_rgtw    : 4;
        RK_U32 cime_srch_lftw    : 4;
        RK_U32 dlt_frm_num       : 16;
    } reg0220_me_rnge;

    /* 0x00000374 reg221 */
    struct {
        RK_U32 srgn_max_num      : 7;
        RK_U32 cime_dist_thre    : 13;
        RK_U32 rme_srch_h        : 2;
        RK_U32 rme_srch_v        : 2;
        RK_U32 rme_dis           : 3;
        RK_U32 reserved1         : 1;
        RK_U32 fme_dis           : 3;
        RK_U32 reserved2         : 1;
    } reg0221_me_cfg;

    /* 0x00000378 reg222 */
    struct {
        RK_U32 cime_zero_thre     : 13;
        RK_U32 reserved           : 15;
        RK_U32 fme_prefsu_en      : 2;
        RK_U32 colmv_stor         : 1;
        RK_U32 colmv_load         : 1;
    } reg0222_me_cach;

    /* 0x0000037c reg223 */
    struct {
        RK_U32 ref_num                   : 1;
        RK_U32 thre_zero_sad_dep0_cme    : 6;
        RK_U32 thre_zero_sad_dep1_cme    : 6;
        RK_U32 thre_zero_diff_dep1_cme   : 3;
        RK_U32 thre_zero_num_dep1_cme    : 3;
        RK_U32 thre_num_hit_dep1_cme     : 2;
        RK_U32 reserved                  : 7;
        RK_U32 rfpw_mode                 : 1;
        RK_U32 rfpr_mode                 : 1;
        RK_U32 rfp1r_mode                : 1;
        RK_U32 reserved1                 : 1;
    } reg0223_me_ref_comb;

    /* 0x380 - 0x39c */
    RK_U32 reserved224_231[8];

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
    }  reg0232_rdo_cfg;

    struct {
        RK_U32 intra_pu32_mode_num    : 2;
        RK_U32 intra_pu16_mode_num    : 2;
        RK_U32 intra_pu8_mode_num     : 2;
        RK_U32 intra_pu4_mode_num     : 2;
        RK_U32 reserved               : 24;
    } reg0233_rdo_intra_mode;

    /* 0x000003a8 reg234 */
    struct {
        RK_U32 rdoqx_pixel_e        : 1;
        RK_U32 rdoqx_cgzero_e       : 1;
        RK_U32 rdoqx_lastxy_e       : 1;
        RK_U32 rdoq4_rdoq_e         : 1;
        RK_U32 reserved1            : 4;
        RK_U32 rdoq_intra_r_coef    : 5;
        RK_U32 reserved2            : 19;
    } reg0234_rdoq_cfg;

    /* 0x3ac */
    RK_U32 reserved_235;

    /* 0x000003b0 reg236 */
    struct {
        RK_U32 nal_unit_type    : 6;
        RK_U32 reserved         : 26;
    } reg0236_synt_nal;

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
    } reg0237_synt_sps;

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
        RK_U32 lp_fltr_acrs_til       : 1;
        RK_U32 csip_flag              : 1;
        RK_U32 reserved               : 9;
    } reg0238_synt_pps;

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
    } reg0239_synt_sli0;

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
    } reg0240_synt_sli1;

    /* 0x000003c4 reg241 */
    struct {
        RK_U32 sli_poc_lsb        : 16;
        RK_U32 sli_hdr_ext_len    : 9;
        RK_U32 reserved           : 7;
    } reg0241_synt_sli2;

    /* 0x000003c8 reg242 */
    struct {
        RK_U32 st_ref_pic_flg    : 1;
        RK_U32 poc_lsb_lt0       : 16;
        RK_U32 lt_idx_sps        : 5;
        RK_U32 num_lt_pic        : 2;
        RK_U32 st_ref_pic_idx    : 6;
        RK_U32 num_lt_sps        : 2;
    } reg0242_synt_refm0;

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
    } reg0243_synt_refm1;

    /* 0x000003d0 reg244 */
    struct {
        RK_U32 dlt_poc_s0_m10    : 16;
        RK_U32 dlt_poc_s0_m11    : 16;
    } reg0244_synt_refm2;

    /* 0x000003d4 reg245 */
    struct {
        RK_U32 dlt_poc_s0_m12    : 16;
        RK_U32 dlt_poc_s0_m13    : 16;
    } reg0245_synt_refm3;

    /* 0x000003d8 reg246 */
    struct {
        RK_U32 poc_lsb_lt1    : 16;
        RK_U32 poc_lsb_lt2    : 16;
    } reg0246_synt_long_refm0;

    /* 0x000003dc reg247 */
    struct {
        RK_U32 dlt_poc_msb_cycl1    : 16;
        RK_U32 dlt_poc_msb_cycl2    : 16;
    } reg0247_synt_long_refm1;

    /* 0x000003e0 reg248 */
    struct {
        RK_U32 sao_lambda_multi    : 3;
        RK_U32 reserved            : 29;
    } reg0248_sao_cfg;

    /* 0x3e4 - 0x3ec */
    RK_U32 reserved249_251[3];

    /* 0x000003f0 reg252 */
    struct {
        RK_U32 tile_w_m1    : 9;
        RK_U32 reserved     : 7;
        RK_U32 tile_h_m1    : 9;
        RK_U32 reserved1    : 6;
        RK_U32 tile_en      : 1;
    } reg0252_tile_cfg;

    /* 0x000003f4 reg253 */
    struct {
        RK_U32 tile_x       : 9;
        RK_U32 reserved     : 7;
        RK_U32 tile_y       : 9;
        RK_U32 reserved1    : 7;
    } reg0253_tile_pos;

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
} HevcVepu511Frame;

/* class: rc/roi/aq/klut */
/* 0x00001000 reg1024 - 0x00001160 reg1112 */
typedef struct HevcVepu511RcRoi_t {
    /* 0x00001000 reg1024 */
    struct {
        RK_U32 qp_adj0     : 5;
        RK_U32 qp_adj1     : 5;
        RK_U32 qp_adj2     : 5;
        RK_U32 qp_adj3     : 5;
        RK_U32 qp_adj4     : 5;
        RK_U32 reserved    : 7;
    } rc_adj0;

    /* 0x00001004 reg1025 */
    struct {
        RK_U32 qp_adj5     : 5;
        RK_U32 qp_adj6     : 5;
        RK_U32 qp_adj7     : 5;
        RK_U32 qp_adj8     : 5;
        RK_U32 reserved    : 12;
    } rc_adj1;

    /* 0x00001008 reg1026 - 0x00001028 reg1034 */
    RK_U32 rc_dthd_0_8[9];

    /* 0x102c */
    RK_U32 reserved_1035;

    /* 0x00001030 reg1036 */
    struct {
        RK_U32 qpmin_area0    : 6;
        RK_U32 qpmax_area0    : 6;
        RK_U32 qpmin_area1    : 6;
        RK_U32 qpmax_area1    : 6;
        RK_U32 qpmin_area2    : 6;
        RK_U32 reserved       : 2;
    } roi_qthd0;

    /* 0x00001034 reg1037 */
    struct {
        RK_U32 qpmax_area2    : 6;
        RK_U32 qpmin_area3    : 6;
        RK_U32 qpmax_area3    : 6;
        RK_U32 qpmin_area4    : 6;
        RK_U32 qpmax_area4    : 6;
        RK_U32 reserved       : 2;
    } roi_qthd1;

    /* 0x00001038 reg1038 */
    struct {
        RK_U32 qpmin_area5    : 6;
        RK_U32 qpmax_area5    : 6;
        RK_U32 qpmin_area6    : 6;
        RK_U32 qpmax_area6    : 6;
        RK_U32 qpmin_area7    : 6;
        RK_U32 reserved       : 2;
    } roi_qthd2;

    /* 0x0000103c reg1039 */
    struct {
        RK_U32 qpmax_area7    : 6;
        RK_U32 reserved       : 26;
    } roi_qthd3;

    /* 0x00001040 reg1040 */
    RK_U32 reserved_1040;

    /* 0x00001044 reg1041 */
    struct {
        RK_U32 aq_tthd0    : 8;
        RK_U32 aq_tthd1    : 8;
        RK_U32 aq_tthd2    : 8;
        RK_U32 aq_tthd3    : 8;
    } aq_tthd0;

    /* 0x00001048 reg1042 */
    struct {
        RK_U32 aq_tthd4    : 8;
        RK_U32 aq_tthd5    : 8;
        RK_U32 aq_tthd6    : 8;
        RK_U32 aq_tthd7    : 8;
    } aq_tthd1;

    /* 0x0000104c reg1043 */
    struct {
        RK_U32 aq_tthd8     : 8;
        RK_U32 aq_tthd9     : 8;
        RK_U32 aq_tthd10    : 8;
        RK_U32 aq_tthd11    : 8;
    } aq_tthd2;

    /* 0x00001050 reg1044 */
    struct {
        RK_U32 aq_tthd12    : 8;
        RK_U32 aq_tthd13    : 8;
        RK_U32 aq_tthd14    : 8;
        RK_U32 aq_tthd15    : 8;
    } aq_tthd3;

    /* 0x00001054 reg1045 */
    struct {
        RK_U32 aq_stp_s0     : 5;
        RK_U32 aq_stp_0t1    : 5;
        RK_U32 aq_stp_1t2    : 5;
        RK_U32 aq_stp_2t3    : 5;
        RK_U32 aq_stp_3t4    : 5;
        RK_U32 aq_stp_4t5    : 5;
        RK_U32 reserved      : 2;
    } aq_stp0;

    /* 0x00001058 reg1046 */
    struct {
        RK_U32 aq_stp_5t6      : 5;
        RK_U32 aq_stp_6t7      : 5;
        RK_U32 aq_stp_7t8      : 5;
        RK_U32 aq_stp_8t9      : 5;
        RK_U32 aq_stp_9t10     : 5;
        RK_U32 aq_stp_10t11    : 5;
        RK_U32 reserved        : 2;
    } aq_stp1;

    /* 0x0000105c reg1047 */
    struct {
        RK_U32 aq_stp_11t12    : 5;
        RK_U32 aq_stp_12t13    : 5;
        RK_U32 aq_stp_13t14    : 5;
        RK_U32 aq_stp_14t15    : 5;
        RK_U32 aq_stp_b15      : 5;
        RK_U32 reserved        : 7;
    } aq_stp2;

    /* 0x00001060 reg1048 */
    struct {
        RK_U32 aq16_rnge         : 4;
        RK_U32 aq32_rnge         : 4;
        RK_U32 aq8_rnge          : 5;
        RK_U32 aq16_dif0         : 5;
        RK_U32 aq16_dif1         : 5;
        RK_U32 reserved          : 1;
        RK_U32 aq_cme_en         : 1;
        RK_U32 aq_subj_cme_en    : 1;
        RK_U32 aq_rme_en         : 1;
        RK_U32 aq_subj_rme_en    : 1;
        RK_U32 reserved1         : 4;
    } aq_clip;

    /* 0x00001064 reg1049 */
    struct {
        RK_U32 madi_th0    : 8;
        RK_U32 madi_th1    : 8;
        RK_U32 madi_th2    : 8;
        RK_U32 reserved    : 8;
    } madi_st_thd;

    /* 0x00001068 reg1050 */
    struct {
        RK_U32 madp_th0     : 12;
        RK_U32 reserved     : 4;
        RK_U32 madp_th1     : 12;
        RK_U32 reserved1    : 4;
    } madp_st_thd0;

    /* 0x0000106c reg1051 */
    struct {
        RK_U32 madp_th2    : 12;
        RK_U32 reserved    : 20;
    } madp_st_thd1;

    /* 0x1070 - 0x1078 */
    RK_U32 reserved1070_1078[3];

    /* 0x0000107c reg1055 */
    struct {
        RK_U32 chrm_klut_ofst    : 4;
        RK_U32 reserved          : 28;
    } klut_ofst;

    /* 0x00001080 reg1056 */
    struct {
        RK_U32 fmdc_adju_intra16         : 4;
        RK_U32 fmdc_adju_inter16         : 4;
        RK_U32 fmdc_adju_split16         : 4;
        RK_U32 fmdc_adju_res_intra16     : 4;
        RK_U32 fmdc_adju_res_inter16     : 4;
        RK_U32 fmdc_adju_res_zeromv16    : 4;
        RK_U32 fmdc_adju_pri             : 5;
        RK_U32 reserved                  : 3;
    } fmdc_adj0;

    /* 0x00001084 reg1057 */
    struct {
        RK_U32 fmdc_adju_intra32         : 4;
        RK_U32 fmdc_adju_inter32         : 4;
        RK_U32 fmdc_adju_split32         : 4;
        RK_U32 fmdc_adju_res_intra32     : 4;
        RK_U32 fmdc_adju_res_inter32     : 4;
        RK_U32 fmdc_adju_res_zeromv32    : 4;
        RK_U32 fmdc_adju_split8          : 4;
        RK_U32 fmdc_adju_lt_ref32        : 4;
    } fmdc_adj1;

    /* 0x1088 */
    RK_U32 reserved_1058;

    /* 0x0000108c reg1059 */
    struct {
        RK_U32 bmap_en               : 1;
        RK_U32 bmap_pri              : 5;
        RK_U32 bmap_qpmin            : 6;
        RK_U32 bmap_qpmax            : 6;
        RK_U32 bmap_mdc_dpth         : 1;
        RK_U32 reserved              : 13;
    } bmap_cfg;

    /* 0x00001090 reg1060 - 0x0000112c reg1099 */
    Vepu511RoiCfg roi_cfg;

    /* 0x00001130 reg1100 */
    struct {
        RK_U32 base_thre_rough_mad32_intra           : 4;
        RK_U32 delta0_thre_rough_mad32_intra         : 4;
        RK_U32 delta1_thre_rough_mad32_intra         : 6;
        RK_U32 delta2_thre_rough_mad32_intra         : 6;
        RK_U32 delta3_thre_rough_mad32_intra         : 7;
        RK_U32 delta4_thre_rough_mad32_intra_low5    : 5;
    } cudecis_thd0;

    /* 0x00001134 reg1101 */
    struct {
        RK_U32 delta4_thre_rough_mad32_intra_high2    : 2;
        RK_U32 delta5_thre_rough_mad32_intra          : 7;
        RK_U32 delta6_thre_rough_mad32_intra          : 7;
        RK_U32 base_thre_fine_mad32_intra             : 4;
        RK_U32 delta0_thre_fine_mad32_intra           : 4;
        RK_U32 delta1_thre_fine_mad32_intra           : 5;
        RK_U32 delta2_thre_fine_mad32_intra_low3      : 3;
    } cudecis_thd1;

    /* 0x00001138 reg1102 */
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

    /* 0x0000113c reg1103 */
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

    /* 0x00001140 reg1104 */
    struct {
        RK_U32 delta1_thre_mad16_intra          : 3;
        RK_U32 delta2_thre_mad16_intra          : 4;
        RK_U32 delta3_thre_mad16_intra          : 5;
        RK_U32 delta4_thre_mad16_intra          : 5;
        RK_U32 delta5_thre_mad16_intra          : 6;
        RK_U32 delta6_thre_mad16_intra          : 6;
        RK_U32 delta0_thre_mad16_ratio_intra    : 3;
    } cudecis_thd4;

    /* 0x00001144 reg1105 */
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

    /* 0x00001148 reg1106 */
    struct {
        RK_U32 delta2_thre_rough_bgrad32_intra_high2    : 2;
        RK_U32 delta3_thre_rough_bgrad32_intra          : 10;
        RK_U32 delta4_thre_rough_bgrad32_intra          : 10;
        RK_U32 delta5_thre_rough_bgrad32_intra_low10    : 10;
    } cudecis_thd6;

    /* 0x0000114c reg1107 */
    struct {
        RK_U32 delta5_thre_rough_bgrad32_intra_high1    : 1;
        RK_U32 delta6_thre_rough_bgrad32_intra          : 12;
        RK_U32 delta7_thre_rough_bgrad32_intra          : 13;
        RK_U32 delta0_thre_bgrad16_ratio_intra          : 4;
        RK_U32 delta1_thre_bgrad16_ratio_intra_low2     : 2;
    } cudecis_thd7;

    /* 0x00001150 reg1108 */
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
    } cudecis_thd8;

    /* 0x00001154 reg1109 */
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

    /* 0x00001158 reg1110 */
    struct {
        RK_U32 delta3_thre_fme32_inter    : 5;
        RK_U32 delta4_thre_fme32_inter    : 6;
        RK_U32 delta5_thre_fme32_inter    : 7;
        RK_U32 delta6_thre_fme32_inter    : 8;
        RK_U32 thre_cme32_inter           : 6;
    } cudecis_thd10;

    /* 0x0000115c reg1111 */
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

    /* 0x00001160 reg1112 */
    struct {
        RK_U32 delta0_thre_cme32_static_inter    : 4;
        RK_U32 delta1_thre_cme32_static_inter    : 4;
        RK_U32 delta2_thre_cme32_static_inter    : 4;
        RK_U32 delta3_thre_cme32_static_inter    : 4;
        RK_U32 delta4_thre_cme32_static_inter    : 4;
        RK_U32 delta5_thre_cme32_static_inter    : 4;
        RK_U32 delta6_thre_cme32_static_inter    : 4;
        RK_U32 delta7_thre_cme32_static_inter    : 4;
    } cudecis_thd12;
} HevcVepu511RcRoi;

/* class: iprd/iprd_wgt/rdo_wgta/prei_dif*/
/* 0x00001700 reg1472 - 0x00001cd4 reg1845 */
typedef struct HevcVepu511Param_t {
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
    } light_cfg;

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
    } me_sqi_cfg;

    /* 0x00001764 reg1497 */
    struct {
        RK_U32 cime_mvd_th0    : 9;
        RK_U32 reserved        : 1;
        RK_U32 cime_mvd_th1    : 9;
        RK_U32 reserved1       : 1;
        RK_U32 cime_mvd_th2    : 9;
        RK_U32 reserved2       : 3;
    } cime_mvd_th;

    /* 0x00001768 reg1498 */
    struct {
        RK_U32 cime_madp_th       : 12;
        RK_U32 ratio_consi_cfg    : 4;
        RK_U32 ratio_bmv_dist     : 4;
        RK_U32 reserved           : 12;
    } cime_madp_th;

    /* 0x0000176c reg1499 */
    struct {
        RK_U32 cime_multi0    : 8;
        RK_U32 cime_multi1    : 8;
        RK_U32 cime_multi2    : 8;
        RK_U32 cime_multi3    : 8;
    } cime_multi;

    /* 0x00001770 reg1500 */
    struct {
        RK_U32 rime_mvd_th0    : 3;
        RK_U32 reserved        : 1;
        RK_U32 rime_mvd_th1    : 3;
        RK_U32 reserved1       : 9;
        RK_U32 fme_madp_th     : 12;
        RK_U32 reserved2       : 4;
    } rime_mvd_th;

    /* 0x00001774 reg1501 */
    struct {
        RK_U32 rime_madp_th0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 rime_madp_th1    : 12;
        RK_U32 reserved1        : 4;
    } rime_madp_th;

    /* 0x00001778 reg1502 */
    struct {
        RK_U32 rime_multi0    : 10;
        RK_U32 rime_multi1    : 10;
        RK_U32 rime_multi2    : 10;
        RK_U32 reserved       : 2;
    } rime_multi;

    /* 0x0000177c reg1503 */
    struct {
        RK_U32 cmv_th0     : 8;
        RK_U32 cmv_th1     : 8;
        RK_U32 cmv_th2     : 8;
        RK_U32 reserved    : 8;
    } cmv_st_th;

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
} HevcVepu511Param;

typedef struct RdoSkipPar_t {
    struct {
        RK_U32 madp_thd0        : 12;
        RK_U32 reserved         : 4;
        RK_U32 madp_thd1        : 12;
        /* only b32 skip support */
        RK_U32 reserved1          : 1;
        RK_U32 flckr_frame_qp_en  : 1;
        RK_U32 flckr_lgt_chng_en  : 1;
        RK_U32 flckr_en           : 1;
    } atf_thd0;

    struct {
        RK_U32 madp_thd2    : 12;
        RK_U32 reserved     : 4;
        RK_U32 madp_thd3    : 12;
        RK_U32 reserved1    : 4;
    } atf_thd1;

    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } atf_wgt0;

    RK_U32 reserved;
} RdoSkipPar;

typedef struct RdoNoSkipPar_t {
    struct {
        RK_U32 madp_thd0    : 12;
        RK_U32 reserved     : 4;
        RK_U32 madp_thd1    : 12;
        RK_U32 reserved1    : 4;
    } atf_thd0;

    struct {
        RK_U32 madp_thd2            : 12;
        RK_U32 reserved             : 4;
        /* only rdo_b32_inter support */
        RK_U32 atf_bypass_pri_flag  : 1;
        RK_U32 reserved1            : 15;
    } atf_thd1;

    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } atf_wgt;
} RdoNoSkipPar;

/* 0x00002000 reg2048 - 0x0000216c reg2139 */
typedef struct HevcVepu511Sqi_t {
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
    } smear_opt_rmd_intra;

    /* 0x0000200c reg2051 */
    struct {
        RK_U32 cfc_rmd_mode_intra_jcoef_d0    : 6;
        RK_U32 cfc_rmd_mode_intra_jcoef_d1    : 6;
        RK_U32 cfc_rdo_mode_intra_jcoef_d0    : 6;
        RK_U32 cfc_rdo_mode_intra_jcoef_d1    : 6;
        RK_U32 cfc_rdoq_rcoef_d0              : 4;
        RK_U32 cfc_rdoq_rcoef_d1              : 4;
    } smear_opt_cfg_coef;

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

    /* 0x00002060 reg2072 - 0x206c reg2075 */
    RdoSkipPar rdo_b32_skip;

    /* 0x00002070 reg2076 - 0x0000207c reg2079 */
    RdoSkipPar rdo_b16_skip;

    /* 0x00002080 reg2080 - 0x00002088 reg2082 */
    RdoNoSkipPar rdo_b32_inter;

    /* 0x0000208c reg2083 - 0x00002094 reg2085 */
    RdoNoSkipPar rdo_b16_inter;

    /* 0x00002098 reg2086 - 0x000020a0 reg2088 */
    RdoNoSkipPar rdo_b32_intra;

    /* 0x000020a4 reg2089 - 0x000020ac reg2091 */
    RdoNoSkipPar rdo_b16_intra;

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
        RK_U32 dark_bright_nsp_j_coef_wgt_dep0    : 6;
        RK_U32 reserved                           : 2;
        RK_U32 dark_bright_nsp_j_coef_wgt_dep1    : 6;
        RK_U32 reserved1                          : 18;
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
        RK_U32 cmplx_static_num_cu16_th                : 3;
        RK_U32 cmplx_static_frame_qp_en                : 1;
        RK_U32 cmplx_static_ratio_light_madp_th        : 2;
        RK_U32 reserved                                : 2;
    } cmplx_statc_thd2;

    /* 0x000020e0 reg2104 */
    struct {
        RK_U32 cmplx_static_inter_res_j_coef_wgt1_dep0    : 8;
        RK_U32 cmplx_static_inter_res_j_coef_wgt2_dep0    : 8;
        RK_U32 cmplx_static_inter_res_j_coef_wgt1_dep1    : 8;
        RK_U32 cmplx_static_inter_res_j_coef_wgt2_dep1    : 8;
    } cmplx_statc_wgt0;

    /* 0x000020e4 reg2105 */
    struct {
        RK_U32 cmplx_static_intra_j_coef_wgt1_dep0    : 8;
        RK_U32 cmplx_static_intra_j_coef_wgt2_dep0    : 8;
        RK_U32 cmplx_static_intra_j_coef_wgt1_dep1    : 8;
        RK_U32 cmplx_static_intra_j_coef_wgt2_dep1    : 8;
    } cmplx_statc_wgt1;

    /* 0x000020e8 reg2106 */
    struct {
        RK_U32 cmplx_static_split_rcoef_w1d0    : 6;
        RK_U32 cmplx_static_split_rcoef_w1d1    : 6;
        RK_U32 cmplx_static_split_rcoef_w2d0    : 6;
        RK_U32 cmplx_static_split_rcoef_w2d1    : 6;
        RK_U32 reserved                         : 8;
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

    union {
        RK_U8 pre_i32_cst_thd[14];

        struct {
            /* 0x00002120 reg2120 */
            struct {
                RK_U32 madi_thd0     : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd1     : 7;
                RK_U32 reserved1     : 1;
                RK_U32 madi_thd2     : 7;
                RK_U32 reserved2     : 1;
                RK_U32 madi_thd3     : 7;
                RK_U32 reserved3     : 1;
            } pre_i32_cst_thd0;

            /* 0x00002124 reg2121 */
            struct {
                RK_U32 madi_thd4     : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd5     : 7;
                RK_U32 reserved1     : 1;
                RK_U32 madi_thd6     : 7;
                RK_U32 reserved2     : 1;
                RK_U32 madi_thd7     : 7;
                RK_U32 reserved3     : 1;
            } pre_i32_cst_thd1;

            /* 0x00002128 reg2122 */
            struct {
                RK_U32 madi_thd8     : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd9     : 7;
                RK_U32 reserved1     : 1;
                RK_U32 madi_thd10    : 7;
                RK_U32 reserved2     : 1;
                RK_U32 madi_thd11    : 7;
                RK_U32 reserved3     : 1;
            } pre_i32_cst_thd2;

            /* 0x0000212c reg2123 */
            struct {
                RK_U32 madi_thd12    : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd13    : 7;
                RK_U32 reserved1     : 1;
                RK_U32 mode_th       : 3;
                RK_U32 reserved2     : 1;
                RK_U32 qp_thd        : 6;
                RK_U32 reserved3     : 6;
            } pre_i32_cst_thd3;
        };
    };

    union {
        RK_U8 pre_i32_cst_wgt[15];

        struct {
            /* 0x00002130 reg2124 */
            struct {
                RK_U32 wgt0                 : 8;
                RK_U32 wgt1                 : 8;
                RK_U32 wgt2                 : 8;
                RK_U32 wgt3                 : 8;
            } pre_i32_cst_wgt0;

            /* 0x00002134 reg2125 */
            struct {
                RK_U32 wgt4                 : 8;
                RK_U32 wgt5                 : 8;
                RK_U32 wgt6                 : 8;
                RK_U32 wgt7                 : 8;
            } pre_i32_cst_wgt1;

            /* 0x00002138 reg2126 */
            struct {
                RK_U32 wgt8                 : 8;
                RK_U32 wgt9                 : 8;
                RK_U32 wgt10                : 8;
                RK_U32 wgt11                : 8;
            } pre_i32_cst_wgt2;
            /* 0x0000213c reg2127 */
            struct {
                RK_U32 wgt12                : 8;
                RK_U32 wgt13                : 8;
                RK_U32 wgt14                : 8;
                RK_U32 i32_lambda_mv_bit    : 3;
                RK_U32 reserved             : 1;
                RK_U32 i16_lambda_mv_bit    : 3;
                RK_U32 anti_strp_e          : 1;
            } pre_i32_cst_wgt3;
        };
    };

    union {
        RK_U8 pre_i16_cst_thd[14];

        struct {
            /* 0x00002140 reg2128 */
            struct {
                RK_U32 madi_thd0     : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd1     : 7;
                RK_U32 reserved1     : 1;
                RK_U32 madi_thd2     : 7;
                RK_U32 reserved2     : 1;
                RK_U32 madi_thd3     : 7;
                RK_U32 reserved3     : 1;
            } pre_i16_cst_thd0;

            /* 0x00002144 reg2129 */
            struct {
                RK_U32 madi_thd4     : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd5     : 7;
                RK_U32 reserved1     : 1;
                RK_U32 madi_thd6     : 7;
                RK_U32 reserved2     : 1;
                RK_U32 madi_thd7     : 7;
                RK_U32 reserved3     : 1;
            } pre_i16_cst_thd1;

            /* 0x00002148 reg2130 */
            struct {
                RK_U32 madi_thd8     : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd9     : 7;
                RK_U32 reserved1     : 1;
                RK_U32 madi_thd10    : 7;
                RK_U32 reserved2     : 1;
                RK_U32 madi_thd11    : 7;
                RK_U32 reserved3     : 1;
            } pre_i16_cst_thd2;

            /* 0x0000214c reg2131 */
            struct {
                RK_U32 madi_thd12    : 7;
                RK_U32 reserved      : 1;
                RK_U32 madi_thd13    : 7;
                RK_U32 reserved1     : 1;
                RK_U32 mode_th       : 3;
                RK_U32 reserved2     : 13;
            } pre_i16_cst_thd3;
        };
    };

    union {
        RK_U8 pre_i16_cst_wgt[15];

        struct {
            /* 0x00002150 reg2132 */
            struct {
                RK_U32 wgt0                : 8;
                RK_U32 wgt1                : 8;
                RK_U32 wgt2                : 8;
                RK_U32 wgt3                : 8;
            } pre_i16_cst_wgt0;

            /* 0x00002154 reg2133 */
            struct {
                RK_U32 wgt4                : 8;
                RK_U32 wgt5                : 8;
                RK_U32 wgt6                : 8;
                RK_U32 wgt7                : 8;
            } pre_i16_cst_wgt1;

            /* 0x00002158 reg2134 */
            struct {
                RK_U32 wgt8                : 8;
                RK_U32 wgt9                : 8;
                RK_U32 wgt10               : 8;
                RK_U32 wgt11               : 8;
            } pre_i16_cst_wgt2;
            /* 0x0000215c reg2135 */
            struct {
                RK_U32 wgt12               : 8;
                RK_U32 wgt13               : 8;
                RK_U32 wgt14               : 8;
                RK_U32 i8_lambda_mv_bit    : 3;
                RK_U32 reserved            : 1;
                RK_U32 i4_lambda_mv_bit    : 3;
                RK_U32 reserved1           : 1;
            } pre_i16_cst_wgt3;
        };
    };

    /* 0x00002160 reg2136 */
    struct {
        RK_U32 offset_thd                      : 4;
        RK_U32 offset_diff_thd                 : 4;
        RK_U32 weak_texture_offset_thd         : 3;
        RK_U32 weak_texture_offset_diff_thd    : 3;
        RK_U32 reserved                        : 18;
    } atr_thd;

    /* 0x00002164 reg2137 - 0x0000216c reg2139 */
    RK_U32 reserved_2137_2139[3];
} HevcVepu511Sqi;

typedef struct H265eV511RegSet_t {
    Vepu511ControlCfg       reg_ctl;
    HevcVepu511Frame        reg_frm;
    HevcVepu511RcRoi        reg_rc_roi;
    HevcVepu511Param        reg_param;
    HevcVepu511Sqi          reg_sqi;
    Vepu511SclJpgTbl        reg_scl_jpgtbl;
    Vepu511OsdComb          reg_osd;
    Vepu511Status           reg_st;
    Vepu511Dbg              reg_dbg;
} H265eV511RegSet;

typedef struct H265eV511StatusElem_t {
    RK_U32 hw_status;
    Vepu511Status st;
} H265eV511StatusElem;

#endif
