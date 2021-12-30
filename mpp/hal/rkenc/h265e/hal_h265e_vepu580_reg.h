/*
 * Copyright 2021 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __HAL_H265E_VEPU580_REG_H__
#define __HAL_H265E_VEPU580_REG_H__

#include "rk_type.h"

#define VEPU580_CTL_OFFSET          (0 * sizeof(RK_U32))
#define VEPU580_BASE_OFFSET         (160 * sizeof(RK_U32))
#define VEPU580_RCKULT_OFFSET       (1024 * sizeof(RK_U32))
#define VEPU580_WEG_OFFSET          (1472 * sizeof(RK_U32))
#define VEPU580_RDOCFG_OFFSET       (2048 * sizeof(RK_U32))
#define VEPU580_OSD_OFFSET          (3072 * sizeof(RK_U32))
#define VEPU580_STATUS_OFFSET       (4096 * sizeof(RK_U32))
#define VEPU580_DEBUG_OFFSET        (5120 * sizeof(RK_U32))
#define VEPU580_REG_BASE_HW_STATUS  0x2c

/* class: control/link */
/* 0x00000000 reg0 - 0x00000120 reg72 */
typedef struct HevcVepu580ControlCfg_t {
    /* 0x00000000 reg0 */
    struct {
        RK_U32 sub_ver      : 8;
        RK_U32 h264_cap     : 1;
        RK_U32 hevc_cap     : 1;
        RK_U32 reserved     : 2;
        RK_U32 res_cap      : 4;
        RK_U32 osd_cap      : 2;
        RK_U32 filtr_cap    : 2;
        RK_U32 bfrm_cap     : 1;
        RK_U32 fbc_cap      : 2;
        RK_U32 reserved1    : 1;
        RK_U32 ip_id        : 8;
    } reg0001_version;

    /* 0x4 - 0xc */
    RK_U32 reserved1_3[3];

    /* 0x00000010 reg4 */
    struct {
        RK_U32 lkt_num     : 8;
        RK_U32 vepu_cmd    : 2;
        RK_U32 reserved    : 22;
    } reg0004_enc_strt;

    /* 0x00000014 reg5 */
    struct {
        RK_U32 safe_clr     : 1;
        RK_U32 force_clr    : 1;
        RK_U32 reserved     : 30;
    } reg0005_enc_clr;

    /* 0x18 - 0x1c */
    RK_U32 reserved6_7[2];

    /* 0x00000020 reg8 */
    struct {
        RK_U32 enc_done_en         : 1;
        RK_U32 lkt_node_done_en    : 1;
        RK_U32 sclr_done_en        : 1;
        RK_U32 slc_done_en         : 1;
        RK_U32 bsf_oflw_en         : 1;
        RK_U32 brsp_otsd_en        : 1;
        RK_U32 wbus_err_en         : 1;
        RK_U32 rbus_err_en         : 1;
        RK_U32 wdg_en              : 1;
        RK_U32 lkt_err_int_en      : 1;
        RK_U32 reserved            : 22;
    } reg0008_int_en;

    /* 0x00000024 reg9 */
    struct {
        RK_U32 enc_done_msk         : 1;
        RK_U32 lkt_node_done_msk    : 1;
        RK_U32 sclr_done_msk        : 1;
        RK_U32 slc_done_msk         : 1;
        RK_U32 bsf_oflw_msk         : 1;
        RK_U32 brsp_otsd_msk        : 1;
        RK_U32 wbus_err_msk         : 1;
        RK_U32 rbus_err_msk         : 1;
        RK_U32 wdg_msk              : 1;
        RK_U32 lkt_err_msk          : 1;
        RK_U32 reserved             : 22;
    } reg0009_int_msk;

    /* 0x00000028 reg10 */
    struct {
        RK_U32 enc_done_clr         : 1;
        RK_U32 lkt_node_done_clr    : 1;
        RK_U32 sclr_done_clr        : 1;
        RK_U32 slc_done_clr         : 1;
        RK_U32 bsf_oflw_clr         : 1;
        RK_U32 brsp_otsd_clr        : 1;
        RK_U32 wbus_err_clr         : 1;
        RK_U32 rbus_err_clr         : 1;
        RK_U32 wdg_clr              : 1;
        RK_U32 lkt_err_clr          : 1;
        RK_U32 reserved             : 22;
    } reg0010_int_clr;

    /* 0x0000002c reg11 */
    struct {
        RK_U32 enc_done_sta         : 1;
        RK_U32 lkt_node_done_sta    : 1;
        RK_U32 sclr_done_sta        : 1;
        RK_U32 slc_done_sta         : 1;
        RK_U32 bsf_oflw_sta         : 1;
        RK_U32 brsp_otsd_sta        : 1;
        RK_U32 wbus_err_sta         : 1;
        RK_U32 rbus_err_sta         : 1;
        RK_U32 wdg_sta              : 1;
        RK_U32 lkt_err_sta          : 1;
        RK_U32 reserved             : 22;
    } reg0011_int_sta;

    /* 0x00000030 reg12 */
    struct {
        RK_U32 lpfw_bus_ordr        : 1;
        RK_U32 cmvw_bus_ordr        : 1;
        RK_U32 dspw_bus_ordr        : 1;
        RK_U32 rfpw_bus_ordr        : 1;
        RK_U32 src_bus_edin         : 4;
        RK_U32 meiw_bus_edin        : 4;
        RK_U32 bsw_bus_edin         : 3;
        RK_U32 lktr_bus_edin        : 4;
        RK_U32 roir_bus_edin        : 4;
        RK_U32 lktw_bus_edin        : 4;
        RK_U32 afbc_bsize           : 1;
        RK_U32 ebufw_bus_ordr       : 1;
        RK_U32 rec_nfbc_bus_edin    : 3;
    } reg0012_dtrns_map;

    /* 0x00000034 reg13 */
    struct {
        RK_U32 reserved        : 7;
        RK_U32 dspr_otsd       : 1;
        RK_U32 reserved1       : 8;
        RK_U32 axi_brsp_cke    : 8;
        RK_U32 reserved2       : 8;
    } reg0013_dtrns_cfg;

    /* 0x00000038 reg14 */
    struct {
        RK_U32 vs_load_thd     : 24;
        RK_U32 rfp_load_thd    : 8;
    } reg0014_enc_wdg;

    /* 0x0000003c reg15 */
    struct {
        RK_U32 hurry_en      : 1;
        RK_U32 hurry_low     : 3;
        RK_U32 hurry_mid     : 3;
        RK_U32 hurry_high    : 3;
        RK_U32 reserved      : 22;
    } reg0015_qos_cfg;

    /* 0x00000040 reg16 */
    struct {
        RK_U32 qos_period    : 16;
        RK_U32 reserved      : 16;
    } reg0016_qos_perd;

    /* 0x00000044 reg17 */
    RK_U32 reg0017_hurry_thd_low;

    /* 0x00000048 reg18 */
    RK_U32 reg0018_hurry_thd_mid;

    /* 0x0000004c reg19 */
    RK_U32 reg0019_hurry_thd_high;

    /* 0x00000050 reg20 */
    struct {
        RK_U32 idle_en_core    : 1;
        RK_U32 idle_en_axi     : 1;
        RK_U32 idle_en_ahb     : 1;
        RK_U32 reserved        : 29;
    } reg0020_enc_idle_en;

    /* 0x00000054 reg21 */
    struct {
        RK_U32 cke                 : 1;
        RK_U32 resetn_hw_en        : 1;
        RK_U32 enc_done_tmvp_en    : 1;
        RK_U32 sram_ckg_en         : 1;
        RK_U32 reserved            : 28;
    } reg0021_func_en;

    /* 0x00000058 reg22 */
    struct {
        RK_U32 recon32_ckg       : 1;
        RK_U32 iqit32_ckg        : 1;
        RK_U32 q32_ckg           : 1;
        RK_U32 t32_ckg           : 1;
        RK_U32 cabac32_ckg       : 1;
        RK_U32 recon16_ckg       : 1;
        RK_U32 iqit16_ckg        : 1;
        RK_U32 q16_ckg           : 1;
        RK_U32 t16_ckg           : 1;
        RK_U32 cabac16_ckg       : 1;
        RK_U32 recon8_ckg        : 1;
        RK_U32 iqit8_ckg         : 1;
        RK_U32 q8_ckg            : 1;
        RK_U32 t8_ckg            : 1;
        RK_U32 cabac8_ckg        : 1;
        RK_U32 recon4_ckg        : 1;
        RK_U32 iqit4_ckg         : 1;
        RK_U32 q4_ckg            : 1;
        RK_U32 t4_ckg            : 1;
        RK_U32 cabac4_ckg        : 1;
        RK_U32 intra32_ckg       : 1;
        RK_U32 intra16_ckg       : 1;
        RK_U32 intra8_ckg        : 1;
        RK_U32 intra4_ckg        : 1;
        RK_U32 inter_pred_ckg    : 1;
        RK_U32 reserved          : 7;
    } reg0022_rdo_ckg_hevc;

    /* 0x0000005c reg23 */
    struct {
        RK_U32 core_id     : 2;
        RK_U32 reserved    : 30;
    } reg0023_enc_id;
} hevc_vepu580_control_cfg;

/* class: buffer/video syntax */
/* 0x00000280 reg160 - 0x000003f4 reg253*/
typedef struct HevcVepu580Base_t {
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
    RK_U32 reg0174_bsbr_addr;

    /* 0x000002bc reg175 */
    RK_U32 reg0175_adr_bsbs;

    /* 0x000002c0 reg176 */
    RK_U32 reg0176_lpfw_addr;

    /* 0x000002c4 reg177 */
    RK_U32 reg0177_lpfr_addr;

    /* 0x000002c8 reg178 */
    RK_U32 reg0178_roi_addr;

    /* 0x000002cc reg179 */
    RK_U32 reg0179_roi_qp_addr;

    /* 0x000002d0 reg180 */
    RK_U32 reg0180_roi_amv_addr;

    /* 0x000002d4 reg181 */
    RK_U32 reg0181_roi_mv_addr;

    /* 0x000002d8 reg182 */
    RK_U32 reg0182_ebuft_addr;

    /* 0x000002dc reg183 */
    RK_U32 reg183_ebufb_addr;

    /* 0x2e0 - 0x2fc */
    RK_U32 reserved184_191[8];

    /* 0x00000300 reg192 */
    struct {
        RK_U32 enc_stnd           : 1;
        RK_U32 roi_en             : 1;
        RK_U32 cur_frm_ref        : 1;
        RK_U32 mei_stor           : 1;
        RK_U32 bs_scp             : 1;
        RK_U32 reserved           : 3;
        RK_U32 pic_qp             : 6;
        RK_U32 num_pic_tot_cur    : 5;
        RK_U32 log2_ctu_num       : 5;
        RK_U32 reserved1          : 6;
        RK_U32 slen_fifo          : 1;
        RK_U32 rec_fbc_dis        : 1;
    } reg0192_enc_pic;

    /* 0x00000304 reg193 */
    struct {
        RK_U32 dchs_txid    : 2;
        RK_U32 dchs_rxid    : 2;
        RK_U32 dchs_txe     : 1;
        RK_U32 dchs_rxe     : 1;
        RK_U32 reserved     : 10;
        RK_U32 dchs_ofst    : 11;
        RK_U32 reserved1    : 5;
    } reg0193_dual_core;

    /* 0x308 - 0x30c */
    RK_U32 reserved194_195[2];

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
        RK_U32 alpha_swap    : 1;
        RK_U32 rbuv_swap     : 1;
        RK_U32 src_cfmt      : 4;
        RK_U32 src_range     : 1;
        RK_U32 out_fmt       : 1;
        RK_U32 reserved      : 24;
    } reg0198_src_fmt;

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
        RK_U32 reserved0    : 26;
        RK_U32 src_mirr     : 1;
        RK_U32 src_rot      : 2;
        RK_U32 txa_en       : 1;
        RK_U32 afbcd_en     : 1;
        RK_U32 reserved1    : 1;
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
        RK_U32 src_strd0    : 17;
        RK_U32 reserved     : 15;
    } reg0205_src_strd0;

    /* 0x00000338 reg206 */
    struct {
        RK_U32 src_strd1    : 16;
        RK_U32 reserved     : 16;
    } reg0206_src_strd1;

    /* 0x33c - 0x34c */
    RK_U32 reserved207_211[5];

    /* 0x00000350 reg212 */
    struct {
        RK_U32 rc_en         : 1;
        RK_U32 aq_en         : 1;
        RK_U32 aq_mode       : 1;
        RK_U32 reserved      : 9;
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

    /* 0x35c */
    RK_U32 reserved_215;

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

    /* 0x36c */
    RK_U32 reserved_219;

    /* 0x00000370 reg220 */
    struct {
        RK_U32 cme_srch_h     : 4;
        RK_U32 cme_srch_v     : 4;
        RK_U32 rme_srch_h     : 3;
        RK_U32 rme_srch_v     : 3;
        RK_U32 reserved       : 2;
        RK_U32 dlt_frm_num    : 16;
    } reg0220_me_rnge;

    /* 0x00000374 reg221 */
    struct {
        RK_U32 pmv_mdst_h    : 8;
        RK_U32 pmv_mdst_v    : 8;
        RK_U32 mv_limit      : 2;
        RK_U32 pmv_num       : 2;
        RK_U32 colmv_stor    : 1;
        RK_U32 colmv_load    : 1;
        RK_U32 reserved1     : 1;
        RK_U32 rme_dis       : 4;
        RK_U32 reserved2     : 1;
        RK_U32 fme_dis       : 4;
    } reg0221_me_cfg;


    /* 0x00000378 reg222 */
    struct {
        RK_U32 cme_rama_max      : 11;
        RK_U32 cme_rama_h        : 5;
        RK_U32 cach_l2_tag       : 2;
        RK_U32 cme_linebuf_w     : 9;
        RK_U32 reserved          : 5;
    } reg0222_me_cach;

    /* 0x37c */
    RK_U32 reserved_223;

    /* 0x00000380 reg224 */
    struct {
        RK_U32 gmv_x        : 13;
        RK_U32 reserved     : 3;
        RK_U32 gmv_y        : 13;
        RK_U32 reserved1    : 3;
    } reg0224_gmv;

    /* 0x384 - 0x38c */
    RK_U32 reserved225_227[3];

    /* 0x00000390 reg228 */
    struct {
        RK_U32 roi_qp_en     : 1;
        RK_U32 roi_amv_en    : 1;
        RK_U32 roi_mv_en     : 1;
        RK_U32 reserved      : 29;
    } reg0228_roi_en;

    /* 0x394 - 0x39c */
    RK_U32 reserved229_231[3];

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 ltm_col          : 1;
        RK_U32 ltm_idx0l0       : 1;
        RK_U32 chrm_spcl        : 1;
        RK_U32 cu_inter_e       : 12;
        RK_U32 reserved         : 4;
        RK_U32 cu_intra_e       : 4;
        RK_U32 ccwa_e           : 1;
        RK_U32 scl_lst_sel      : 2;
        RK_U32 reserved1        : 2;
        RK_U32 satd_byps_flg    : 4;
    } reg0232_rdo_cfg;

    /* 0x000003a4 reg233 */
    struct {
        RK_U32 vthd_y       : 12;
        RK_U32 reserved     : 4;
        RK_U32 vthd_c       : 12;
        RK_U32 reserved1    : 4;
    } reg0233_iprd_csts;

    /* 0x3a8 - 0x3ac */
    RK_U32 reserved234_235[2];

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
        RK_U32 lpf_fltr_acrs_til      : 1;
        RK_U32 reserved               : 10;
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

    /* 0x3e0 - 0x3ec */
    RK_U32 reserved248_251[4];

    /* 0x000003f0 reg252 */

    struct {
        RK_U32 tile_w_m1    : 8;
        RK_U32 reserved     : 8;
        RK_U32 tile_h_m1    : 8;
        RK_U32 reserved1    : 7;
        RK_U32 tile_en      : 1;
    } reg0252_tile_cfg;
    /* 0x000003f4 reg253 */
    struct {
        RK_U32 tile_x       : 8;
        RK_U32 reserved     : 8;
        RK_U32 tile_y       : 8;
        RK_U32 reserved1    : 8;
    } reg0253_tile_pos;
} hevc_vepu580_base;

/* class: rc/roi/aq/klut */
/* 0x00001000 reg1024 - 0x000010e0 reg1080 */
typedef struct HevcVepu580RcKlut_t {
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
        RK_U32 reserved       : 24;
        RK_U32 qpmap_mode     : 2;
    } roi_qthd3;

    /* 0x00001040 reg1040 */
    struct {
        RK_U32 madi_mode    : 1;
        RK_U32 reserved     : 15;
        RK_U32 madi_thd     : 8;
        RK_U32 reserved1    : 8;
    } madi_cfg;

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
        RK_U32 aq_stp_s0     : 6;
        RK_U32 reserved      : 2;
        RK_U32 aq_stp_0t1    : 6;
        RK_U32 reserved1     : 2;
        RK_U32 aq_stp_1t2    : 6;
        RK_U32 reserved2     : 2;
        RK_U32 aq_stp_2t3    : 6;
        RK_U32 reserved3     : 2;
    } aq_stp0;

    /* 0x00001058 reg1046 */
    struct {
        RK_U32 aq_stp_3t4    : 6;
        RK_U32 reserved      : 2;
        RK_U32 aq_stp_4t5    : 6;
        RK_U32 reserved1     : 2;
        RK_U32 aq_stp_5t6    : 6;
        RK_U32 reserved2     : 2;
        RK_U32 aq_stp_6t7    : 6;
        RK_U32 reserved3     : 2;
    } aq_stp1;

    /* 0x0000105c reg1047 */
    struct {
        RK_U32 aq_stp_8t9      : 6;
        RK_U32 reserved        : 2;
        RK_U32 aq_stp_9t10     : 6;
        RK_U32 reserved1       : 2;
        RK_U32 aq_stp_10t11    : 6;
        RK_U32 reserved2       : 2;
        RK_U32 aq_stp_11t12    : 6;
        RK_U32 reserved3       : 2;
    } aq_stp2;

    /* 0x00001060 reg1048 */
    struct {
        RK_U32 aq_stp_12t13    : 6;
        RK_U32 reserved        : 2;
        RK_U32 aq_stp_13t14    : 6;
        RK_U32 reserved1       : 2;
        RK_U32 aq_stp_14t15    : 6;
        RK_U32 reserved2       : 2;
        RK_U32 aq_stp_b15      : 6;
        RK_U32 reserved3       : 2;
    } aq_stp3;

    /* 0x1064 - 0x106c */
    RK_U32 reserved1049_1051[3];

    /* 0x00001070 reg1052 */
    struct {
        RK_U32 md_sad_thd0    : 8;
        RK_U32 md_sad_thd1    : 8;
        RK_U32 md_sad_thd2    : 8;
        RK_U32 reserved       : 8;
    } md_sad_thd;

    /* 0x00001074 reg1053 */
    struct {
        RK_U32 madi_thd0    : 8;
        RK_U32 madi_thd1    : 8;
        RK_U32 madi_thd2    : 8;
        RK_U32 reserved     : 8;
    } madi_thd;

    /* 0x1078 - 0x107c */
    RK_U32 reserved1054_1055[2];

    /* 0x00001080 reg1056 */
    struct {
        RK_U32 chrm_klut_ofst    : 3;
        RK_U32 reserved          : 29;
    } klut_ofst;

    /* 0x00001084 reg1057 */
    struct {
        RK_U32 chrm_klut_wgt0       : 18;
        RK_U32 reserved             : 5;
        RK_U32 chrm_klut_wgt1_l9    : 9;
    } klut_wgt0;

    /* 0x00001088 reg1058 */
    struct {
        RK_U32 chrm_klut_wgt1_h9    : 9;
        RK_U32 reserved             : 5;
        RK_U32 chrm_klut_wgt2       : 18;
    } klut_wgt1;

    /* 0x0000108c reg1059 */
    struct {
        RK_U32 chrm_klut_wgt3       : 18;
        RK_U32 reserved             : 5;
        RK_U32 chrm_klut_wgt4_l9    : 9;
    } klut_wgt2;

    /* 0x00001090 reg1060 */
    struct {
        RK_U32 chrm_klut_wgt4_h9    : 9;
        RK_U32 reserved             : 5;
        RK_U32 chrm_klut_wgt5       : 18;
    } klut_wgt3;

    /* 0x00001094 reg1061 */
    struct {
        RK_U32 chrm_klut_wgt6       : 18;
        RK_U32 reserved             : 5;
        RK_U32 chrm_klut_wgt7_l9    : 9;
    } klut_wgt4;

    /* 0x00001098 reg1062 */
    struct {
        RK_U32 chrm_klut_wgt7_h9    : 9;
        RK_U32 reserved             : 5;
        RK_U32 chrm_klut_wgt8       : 18;
    } klut_wgt5;

    /* 0x0000109c reg1063 */
    struct {
        RK_U32 chrm_klut_wgt9        : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt10_l9    : 9;
    } klut_wgt6;

    /* 0x000010a0 reg1064 */
    struct {
        RK_U32 chrm_klut_wgt10_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt11       : 18;
    } klut_wgt7;

    /* 0x000010a4 reg1065 */
    struct {
        RK_U32 chrm_klut_wgt12       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt13_l9    : 9;
    } klut_wgt8;

    /* 0x000010a8 reg1066 */
    struct {
        RK_U32 chrm_klut_wgt13_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt14       : 18;
    } klut_wgt9;

    /* 0x000010ac reg1067 */
    struct {
        RK_U32 chrm_klut_wgt15       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt16_l9    : 9;
    } klut_wgt10;

    /* 0x000010b0 reg1068 */
    struct {
        RK_U32 chrm_klut_wgt16_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt17       : 18;
    } klut_wgt11;

    /* 0x000010b4 reg1069 */
    struct {
        RK_U32 chrm_klut_wgt18       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt19_l9    : 9;
    } klut_wgt12;

    /* 0x000010b8 reg1070 */
    struct {
        RK_U32 chrm_klut_wgt19_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt20       : 18;
    } klut_wgt13;

    /* 0x000010bc reg1071 */
    struct {
        RK_U32 chrm_klut_wgt21       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt22_l9    : 9;
    } klut_wgt14;

    /* 0x000010c0 reg1072 */
    struct {
        RK_U32 chrm_klut_wgt22_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt23       : 18;
    } klut_wgt15;

    /* 0x000010c4 reg1073 */
    struct {
        RK_U32 chrm_klut_wgt24       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt25_l9    : 9;
    } klut_wgt16;

    /* 0x000010c8 reg1074 */
    struct {
        RK_U32 chrm_klut_wgt25_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt26       : 18;
    } klut_wgt17;

    /* 0x000010cc reg1075 */
    struct {
        RK_U32 chrm_klut_wgt27       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt28_l9    : 9;
    } klut_wgt18;

    /* 0x000010d0 reg1076 */
    struct {
        RK_U32 chrm_klut_wgt28_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt29       : 18;
    } klut_wgt19;

    /* 0x000010d4 reg1077 */
    struct {
        RK_U32 chrm_klut_wgt30       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt31_l9    : 9;
    } klut_wgt20;

    /* 0x000010d8 reg1078 */
    struct {
        RK_U32 chrm_klut_wgt31_h9    : 9;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt32       : 18;
    } klut_wgt21;

    /* 0x000010dc reg1079 */
    struct {
        RK_U32 chrm_klut_wgt33       : 18;
        RK_U32 reserved              : 5;
        RK_U32 chrm_klut_wgt34_l9    : 9;
    } klut_wgt22;

    /* 0x000010e0 reg1080 */
    struct {
        RK_U32 chrm_klut_wgt34_h9    : 9;
        RK_U32 reserved              : 23;
    } klut_wgt23;
} hevc_vepu580_rc_klut;

/* class: iprd/iprd_wgt/rdo_wgta/prei_dif/sobel */
/* 0x00001700 reg1472 - 0x00001cd4 reg1845 */
typedef struct HevcVepu580Wgt_t {
    /* 0x00001700 reg1472 */
    struct {
        RK_U32 lvl32_intra_cst_thd0 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 lvl32_intra_cst_thd1 : 12;
        RK_U32 reserved1 : 4;
    } lvl32_intra_CST_THD0;

    /* 0x1704 */
    struct {
        RK_U32 lvl32_intra_cst_thd2 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 lvl32_intra_cst_thd3 : 12;
        RK_U32 reserved1 : 4;
    } lvl32_intra_CST_THD1;

    /* 0x1708 */
    struct {
        RK_U32 lvl16_intra_cst_thd0 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 lvl16_intra_cst_thd1 : 12;
        RK_U32 reserved1 : 4;
    } lvl16_intra_CST_THD0;

    /* 0x170c */
    struct {
        RK_U32 lvl16_intra_cst_thd2 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 lvl16_intra_cst_thd3 : 12;
        RK_U32 reserved1 : 4;
    } lvl16_intra_CST_THD1;

    /* 0x10-0x18 - reserved */
    RK_U32 reserved_0x1710_0x0x1718[3];

    /* 0x171c */
    struct {
        RK_U32 lvl32_intra_cst_wgt0 : 8;
        RK_U32 lvl32_intra_cst_wgt1 : 8;
        RK_U32 lvl32_intra_cst_wgt2 : 8;
        RK_U32 lvl32_intra_cst_wgt3 : 8;
    } lvl32_intra_CST_WGT0;

    /* 0x1720 */
    struct {
        RK_U32 lvl32_intra_cst_wgt4 : 8;
        RK_U32 lvl32_intra_cst_wgt5 : 8;
        RK_U32 lvl32_intra_cst_wgt6 : 8;
        RK_U32 reserved2 : 8;
    } lvl32_intra_CST_WGT1;

    /* 0x1724 */
    struct {
        RK_U32 lvl16_intra_cst_wgt0 : 8;
        RK_U32 lvl16_intra_cst_wgt1 : 8;
        RK_U32 lvl16_intra_cst_wgt2 : 8;
        RK_U32 lvl16_intra_cst_wgt3 : 8;
    } lvl16_intra_CST_WGT0;

    /* 0x1728 */
    struct {
        RK_U32 lvl16_intra_cst_wgt4 : 8;
        RK_U32 lvl16_intra_cst_wgt5 : 8;
        RK_U32 lvl16_intra_cst_wgt6 : 8;
        RK_U32 reserved2 : 8;
    } lvl16_intra_CST_WGT1;


    /* 0x172c */
    RK_U32 reserved_1483;

    /* 0x00001730 reg1484 */
    struct {
        RK_U32 qnt_bias_i    : 10;
        RK_U32 qnt_bias_p    : 10;
        RK_U32 reserved      : 12;
    } reg1484_qnt_bias_comb;

    /* 0x1734 - 0x175c */
    RK_U32 reserved1485_1495[11];

    /* 0x00001760 reg1496 */
    struct {
        RK_U32 cime_sad_mod_sel          : 1;
        RK_U32 cime_sad_use_big_block    : 1;
        RK_U32 cime_pmv_set_zero         : 1;
        RK_U32 reserved                  : 5;
        RK_U32 cime_pmv_num              : 2;
        RK_U32 reserved1                 : 22;
    } cime_sqi_cfg;

    /* 0x00001764 reg1497 */
    struct {
        RK_U32 cime_mvd_th0    : 9;
        RK_U32 reserved        : 1;
        RK_U32 cime_mvd_th1    : 9;
        RK_U32 reserved1       : 1;
        RK_U32 cime_mvd_th2    : 9;
        RK_U32 reserved2       : 3;
    } cime_sqi_thd;

    /* 0x00001768 reg1498 */
    struct {
        RK_U32 cime_multi0    : 10;
        RK_U32 reserved       : 6;
        RK_U32 cime_multi1    : 10;
        RK_U32 reserved1      : 6;
    } cime_sqi_multi0;

    /* 0x0000176c reg1499 */
    struct {
        RK_U32 cime_multi2    : 10;
        RK_U32 reserved       : 6;
        RK_U32 cime_multi3    : 10;
        RK_U32 reserved1      : 6;
    } cime_sqi_multi1;

    /* 0x00001770 reg1500 */
    struct {
        RK_U32 cime_sad_th0    : 12;
        RK_U32 reserved        : 4;
        RK_U32 rime_mvd_th0    : 4;
        RK_U32 reserved1       : 4;
        RK_U32 rime_mvd_th1    : 4;
        RK_U32 reserved2       : 4;
    } rime_sqi_thd;

    /* 0x00001774 reg1501 */
    struct {
        RK_U32 rime_multi0    : 10;
        RK_U32 rime_multi1    : 10;
        RK_U32 rime_multi2    : 10;
        RK_U32 reserved       : 2;
    } rime_sqi_multi;

    /* 0x00001778 reg1502 */
    struct {
        RK_U32 cime_sad_pu16_th    : 12;
        RK_U32 reserved            : 4;
        RK_U32 cime_sad_pu32_th    : 12;
        RK_U32 reserved1           : 4;
    } fme_sqi_thd0;

    /* 0x0000177c reg1503 */
    struct {
        RK_U32 cime_sad_pu64_th    : 12;
        RK_U32 reserved            : 4;
        RK_U32 move_lambda         : 4;
        RK_U32 reserved1           : 12;
    } fme_sqi_thd1;

    /* 0x1780 - 0x17fc */
    RK_U32 reserved1504_1535[32];

    /* 0x00001800 reg1536 - 0x000018cc reg1587  */
    // struct {
    //     RK_U32 wgt_qp0     : 20;
    //     RK_U32 reserved    : 12;
    // } iprd_wgt_qp_hevc_0_51[52];
    RK_U32 iprd_wgt_qp_hevc_0_51[52];

    /* 0x18d0 - 0x18fc */
    RK_U32 reserved1588_1599[12];

    /* wgt_qp48_grpa */
    /* 0x00001900 reg1600 */
    RK_U32 rdo_wgta_qp_grpa_0_51[52];

    /* 0x19d0 - 0x1afc */
    RK_U32 reserved1652_1727[76];
    // 0x1b00

    struct {
        RK_U32 pre_intra_cla0_m0 : 6;
        RK_U32 pre_intra_cla0_m1 : 6;
        RK_U32 pre_intra_cla0_m2 : 6;
        RK_U32 pre_intra_cla0_m3 : 6;
        RK_U32 pre_intra_cla0_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla0_B0;

    // 0x1b04
    struct {
        RK_U32 pre_intra_cla0_m5 : 6;
        RK_U32 pre_intra_cla0_m6 : 6;
        RK_U32 pre_intra_cla0_m7 : 6;
        RK_U32 pre_intra_cla0_m8 : 6;
        RK_U32 pre_intra_cla0_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla0_B1;

    // 0x1b08
    struct {
        RK_U32 pre_intra_cla1_m0 : 6;
        RK_U32 pre_intra_cla1_m1 : 6;
        RK_U32 pre_intra_cla1_m2 : 6;
        RK_U32 pre_intra_cla1_m3 : 6;
        RK_U32 pre_intra_cla1_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla1_B0;

    // 0x1b0c
    struct {
        RK_U32 pre_intra_cla1_m5 : 6;
        RK_U32 pre_intra_cla1_m6 : 6;
        RK_U32 pre_intra_cla1_m7 : 6;
        RK_U32 pre_intra_cla1_m8 : 6;
        RK_U32 pre_intra_cla1_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla1_B1;

    // 0x1b10
    // 0x0320
    struct {
        RK_U32 pre_intra_cla2_m0 : 6;
        RK_U32 pre_intra_cla2_m1 : 6;
        RK_U32 pre_intra_cla2_m2 : 6;
        RK_U32 pre_intra_cla2_m3 : 6;
        RK_U32 pre_intra_cla2_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla2_B0;

    // 0x1b14
    struct {
        RK_U32 pre_intra_cla2_m5 : 6;
        RK_U32 pre_intra_cla2_m6 : 6;
        RK_U32 pre_intra_cla2_m7 : 6;
        RK_U32 pre_intra_cla2_m8 : 6;
        RK_U32 pre_intra_cla2_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla2_B1;

    // 0x1b18
    struct {
        RK_U32 pre_intra_cla3_m0 : 6;
        RK_U32 pre_intra_cla3_m1 : 6;
        RK_U32 pre_intra_cla3_m2 : 6;
        RK_U32 pre_intra_cla3_m3 : 6;
        RK_U32 pre_intra_cla3_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla3_B0;

    // 0x1b1c
    struct {
        RK_U32 pre_intra_cla3_m5 : 6;
        RK_U32 pre_intra_cla3_m6 : 6;
        RK_U32 pre_intra_cla3_m7 : 6;
        RK_U32 pre_intra_cla3_m8 : 6;
        RK_U32 pre_intra_cla3_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla3_B1;

    // 0x1b20
    struct {
        RK_U32 pre_intra_cla4_m0 : 6;
        RK_U32 pre_intra_cla4_m1 : 6;
        RK_U32 pre_intra_cla4_m2 : 6;
        RK_U32 pre_intra_cla4_m3 : 6;
        RK_U32 pre_intra_cla4_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla4_B0;

    // 0x1b24
    struct {
        RK_U32 pre_intra_cla4_m5 : 6;
        RK_U32 pre_intra_cla4_m6 : 6;
        RK_U32 pre_intra_cla4_m7 : 6;
        RK_U32 pre_intra_cla4_m8 : 6;
        RK_U32 pre_intra_cla4_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla4_B1;

    // 0x1b28
    struct {
        RK_U32 pre_intra_cla5_m0 : 6;
        RK_U32 pre_intra_cla5_m1 : 6;
        RK_U32 pre_intra_cla5_m2 : 6;
        RK_U32 pre_intra_cla5_m3 : 6;
        RK_U32 pre_intra_cla5_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla5_B0;

    // 0x1b2c
    struct {
        RK_U32 pre_intra_cla5_m5 : 6;
        RK_U32 pre_intra_cla5_m6 : 6;
        RK_U32 pre_intra_cla5_m7 : 6;
        RK_U32 pre_intra_cla5_m8 : 6;
        RK_U32 pre_intra_cla5_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla5_B1;

    // 0x1b30
    struct {
        RK_U32 pre_intra_cla6_m0 : 6;
        RK_U32 pre_intra_cla6_m1 : 6;
        RK_U32 pre_intra_cla6_m2 : 6;
        RK_U32 pre_intra_cla6_m3 : 6;
        RK_U32 pre_intra_cla6_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla6_B0;

    // 0x1b34
    struct {
        RK_U32 pre_intra_cla6_m5 : 6;
        RK_U32 pre_intra_cla6_m6 : 6;
        RK_U32 pre_intra_cla6_m7 : 6;
        RK_U32 pre_intra_cla6_m8 : 6;
        RK_U32 pre_intra_cla6_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla6_B1;

    // 0x1b38
    struct {
        RK_U32 pre_intra_cla7_m0 : 6;
        RK_U32 pre_intra_cla7_m1 : 6;
        RK_U32 pre_intra_cla7_m2 : 6;
        RK_U32 pre_intra_cla7_m3 : 6;
        RK_U32 pre_intra_cla7_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla7_B0;

    // 0x1b3c
    struct {
        RK_U32 pre_intra_cla7_m5 : 6;
        RK_U32 pre_intra_cla7_m6 : 6;
        RK_U32 pre_intra_cla7_m7 : 6;
        RK_U32 pre_intra_cla7_m8 : 6;
        RK_U32 pre_intra_cla7_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla7_B1;

    // 0x1b40
    struct {
        RK_U32 pre_intra_cla8_m0 : 6;
        RK_U32 pre_intra_cla8_m1 : 6;
        RK_U32 pre_intra_cla8_m2 : 6;
        RK_U32 pre_intra_cla8_m3 : 6;
        RK_U32 pre_intra_cla8_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla8_B0;

    // 0x1b44
    struct {
        RK_U32 pre_intra_cla8_m5 : 6;
        RK_U32 pre_intra_cla8_m6 : 6;
        RK_U32 pre_intra_cla8_m7 : 6;
        RK_U32 pre_intra_cla8_m8 : 6;
        RK_U32 pre_intra_cla8_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla8_B1;

    // 0x1b48
    struct {
        RK_U32 pre_intra_cla9_m0 : 6;
        RK_U32 pre_intra_cla9_m1 : 6;
        RK_U32 pre_intra_cla9_m2 : 6;
        RK_U32 pre_intra_cla9_m3 : 6;
        RK_U32 pre_intra_cla9_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla9_B0;

    // 0x1b4c
    struct {
        RK_U32 pre_intra_cla9_m5 : 6;
        RK_U32 pre_intra_cla9_m6 : 6;
        RK_U32 pre_intra_cla9_m7 : 6;
        RK_U32 pre_intra_cla9_m8 : 6;
        RK_U32 pre_intra_cla9_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla9_B1;

    // 0x1b50
    struct {
        RK_U32 pre_intra_cla10_m0 : 6;
        RK_U32 pre_intra_cla10_m1 : 6;
        RK_U32 pre_intra_cla10_m2 : 6;
        RK_U32 pre_intra_cla10_m3 : 6;
        RK_U32 pre_intra_cla10_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla10_B0;

    // 0x1b54
    struct {
        RK_U32 pre_intra_cla10_m5 : 6;
        RK_U32 pre_intra_cla10_m6 : 6;
        RK_U32 pre_intra_cla10_m7 : 6;
        RK_U32 pre_intra_cla10_m8 : 6;
        RK_U32 pre_intra_cla10_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla10_B1;

    // 0x1b58
    struct {
        RK_U32 pre_intra_cla11_m0 : 6;
        RK_U32 pre_intra_cla11_m1 : 6;
        RK_U32 pre_intra_cla11_m2 : 6;
        RK_U32 pre_intra_cla11_m3 : 6;
        RK_U32 pre_intra_cla11_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla11_B0;

    // 0x1b5c
    struct {
        RK_U32 pre_intra_cla11_m5 : 6;
        RK_U32 pre_intra_cla11_m6 : 6;
        RK_U32 pre_intra_cla11_m7 : 6;
        RK_U32 pre_intra_cla11_m8 : 6;
        RK_U32 pre_intra_cla11_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla11_B1;

    // 0x1b60
    struct {
        RK_U32 pre_intra_cla12_m0 : 6;
        RK_U32 pre_intra_cla12_m1 : 6;
        RK_U32 pre_intra_cla12_m2 : 6;
        RK_U32 pre_intra_cla12_m3 : 6;
        RK_U32 pre_intra_cla12_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla12_B0;

    // 0x1b64
    struct {
        RK_U32 pre_intra_cla12_m5 : 6;
        RK_U32 pre_intra_cla12_m6 : 6;
        RK_U32 pre_intra_cla12_m7 : 6;
        RK_U32 pre_intra_cla12_m8 : 6;
        RK_U32 pre_intra_cla12_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla12_B1;

    // 0x1b68
    struct {
        RK_U32 pre_intra_cla13_m0 : 6;
        RK_U32 pre_intra_cla13_m1 : 6;
        RK_U32 pre_intra_cla13_m2 : 6;
        RK_U32 pre_intra_cla13_m3 : 6;
        RK_U32 pre_intra_cla13_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla13_B0;

    // 0x1b6c
    struct {
        RK_U32 pre_intra_cla13_m5 : 6;
        RK_U32 pre_intra_cla13_m6 : 6;
        RK_U32 pre_intra_cla13_m7 : 6;
        RK_U32 pre_intra_cla13_m8 : 6;
        RK_U32 pre_intra_cla13_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla13_B1;

    // 0x1b70
    struct {
        RK_U32 pre_intra_cla14_m0 : 6;
        RK_U32 pre_intra_cla14_m1 : 6;
        RK_U32 pre_intra_cla14_m2 : 6;
        RK_U32 pre_intra_cla14_m3 : 6;
        RK_U32 pre_intra_cla14_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla14_B0;

    // 0x1b74
    struct {
        RK_U32 pre_intra_cla14_m5 : 6;
        RK_U32 pre_intra_cla14_m6 : 6;
        RK_U32 pre_intra_cla14_m7 : 6;
        RK_U32 pre_intra_cla14_m8 : 6;
        RK_U32 pre_intra_cla14_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla14_B1;

    // 0x1b78
    struct {
        RK_U32 pre_intra_cla15_m0 : 6;
        RK_U32 pre_intra_cla15_m1 : 6;
        RK_U32 pre_intra_cla15_m2 : 6;
        RK_U32 pre_intra_cla15_m3 : 6;
        RK_U32 pre_intra_cla15_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla15_B0;

    // 0x1b7c
    struct {
        RK_U32 pre_intra_cla15_m5 : 6;
        RK_U32 pre_intra_cla15_m6 : 6;
        RK_U32 pre_intra_cla15_m7 : 6;
        RK_U32 pre_intra_cla15_m8 : 6;
        RK_U32 pre_intra_cla15_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla15_B1;

    // 0x1b80
    struct {
        RK_U32 pre_intra_cla16_m0 : 6;
        RK_U32 pre_intra_cla16_m1 : 6;
        RK_U32 pre_intra_cla16_m2 : 6;
        RK_U32 pre_intra_cla16_m3 : 6;
        RK_U32 pre_intra_cla16_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla16_B0;

    // 0x1b84
    struct {
        RK_U32 pre_intra_cla16_m5 : 6;
        RK_U32 pre_intra_cla16_m6 : 6;
        RK_U32 pre_intra_cla16_m7 : 6;
        RK_U32 pre_intra_cla16_m8 : 6;
        RK_U32 pre_intra_cla16_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla16_B1;

    /* 0x1b88 - 0x1bfc */
    RK_U32 reserved1762_1791[30];

    /* 0x00001c00 reg1792 */
    struct {
        RK_U32 intra_l16_sobel_t0    : 12;
        RK_U32 reserved              : 4;
        RK_U32 intra_l16_sobel_t1    : 12;
        RK_U32 reserved1             : 4;
    } i16_sobel_t;

    /* 0x00001c04 reg1793 */
    struct {
        RK_U32 intra_l16_sobel_a0_qp0    : 6;
        RK_U32 intra_l16_sobel_a0_qp1    : 6;
        RK_U32 intra_l16_sobel_a0_qp2    : 6;
        RK_U32 intra_l16_sobel_a0_qp3    : 6;
        RK_U32 intra_l16_sobel_a0_qp4    : 6;
        RK_U32 reserved                  : 2;
    } i16_sobel_a_00;

    /* 0x00001c08 reg1794 */
    struct {
        RK_U32 intra_l16_sobel_a0_qp5    : 6;
        RK_U32 intra_l16_sobel_a0_qp6    : 6;
        RK_U32 intra_l16_sobel_a0_qp7    : 6;
        RK_U32 intra_l16_sobel_a0_qp8    : 6;
        RK_U32 reserved                  : 8;
    } i16_sobel_a_01;

    /* 0x00001c0c reg1795 */
    struct {
        RK_U32 intra_l16_sobel_b0_qp0    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_b0_qp1    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_b_00;

    /* 0x00001c10 reg1796 */
    struct {
        RK_U32 intra_l16_sobel_b0_qp2    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_b0_qp3    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_b_01;

    /* 0x00001c14 reg1797 */
    struct {
        RK_U32 intra_l16_sobel_b0_qp4    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_b0_qp5    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_b_02;

    /* 0x00001c18 reg1798 */
    struct {
        RK_U32 intra_l16_sobel_b0_qp6    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_b0_qp7    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_b_03;

    /* 0x00001c1c reg1799 */
    struct {
        RK_U32 intra_l16_sobel_b0_qp8    : 15;
        RK_U32 reserved                  : 17;
    } i16_sobel_b_04;

    /* 0x00001c20 reg1800 */
    struct {
        RK_U32 intra_l16_sobel_c0_qp0    : 6;
        RK_U32 intra_l16_sobel_c0_qp1    : 6;
        RK_U32 intra_l16_sobel_c0_qp2    : 6;
        RK_U32 intra_l16_sobel_c0_qp3    : 6;
        RK_U32 intra_l16_sobel_c0_qp4    : 6;
        RK_U32 reserved                  : 2;
    } i16_sobel_c_00;

    /* 0x00001c24 reg1801 */
    struct {
        RK_U32 intra_l16_sobel_c0_qp5    : 6;
        RK_U32 intra_l16_sobel_c0_qp6    : 6;
        RK_U32 intra_l16_sobel_c0_qp7    : 6;
        RK_U32 intra_l16_sobel_c0_qp8    : 6;
        RK_U32 reserved                  : 8;
    } i16_sobel_c_01;

    /* 0x00001c28 reg1802 */
    struct {
        RK_U32 intra_l16_sobel_d0_qp0    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_d0_qp1    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_d_00;

    /* 0x00001c2c reg1803 */
    struct {
        RK_U32 intra_l16_sobel_d0_qp2    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_d0_qp3    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_d_01;

    /* 0x00001c30 reg1804 */
    struct {
        RK_U32 intra_l16_sobel_d0_qp4    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_d0_qp5    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_d_02;

    /* 0x00001c34 reg1805 */
    struct {
        RK_U32 intra_l16_sobel_d0_qp6    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l16_sobel_d0_qp7    : 15;
        RK_U32 reserved1                 : 1;
    } i16_sobel_d_03;

    /* 0x00001c38 reg1806 */
    struct {
        RK_U32 intra_l16_sobel_d0_qp8    : 15;
        RK_U32 reserved                  : 17;
    } i16_sobel_d_04;

    /* 0x00001c3c reg1807 */
    RK_U32 intra_l16_sobel_e0_qp0_low;

    /* 0x00001c40 reg1808 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp0_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_01;

    /* 0x00001c44 reg1809 */
    RK_U32 intra_l16_sobel_e0_qp1_low;

    /* 0x00001c48 reg1810 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp1_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_03;

    /* 0x00001c4c reg1811 */
    RK_U32 intra_l16_sobel_e0_qp2_low;

    /* 0x00001c50 reg1812 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp2_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_05;

    /* 0x00001c54 reg1813 */
    RK_U32 intra_l16_sobel_e0_qp3_low;

    /* 0x00001c58 reg1814 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp3_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_07;

    /* 0x00001c5c reg1815 */
    RK_U32 intra_l16_sobel_e0_qp4_low;

    /* 0x00001c60 reg1816 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp4_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_09;

    /* 0x00001c64 reg1817 */
    RK_U32 intra_l16_sobel_e0_qp5_low;

    /* 0x00001c68 reg1818 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp5_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_11;

    /* 0x00001c6c reg1819 */
    RK_U32 intra_l16_sobel_e0_qp6_low;

    /* 0x00001c70 reg1820 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp6_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_13;

    /* 0x00001c74 reg1821 */
    RK_U32 intra_l16_sobel_e0_qp7_low;

    /* 0x00001c78 reg1822 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp7_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_15;

    /* 0x00001c7c reg1823 */
    RK_U32 intra_l16_sobel_e0_qp8_low;

    /* 0x00001c80 reg1824 */
    struct {
        RK_U32 intra_l16_sobel_e0_qp8_high    : 2;
        RK_U32 reserved                       : 30;
    } i16_sobel_e_17;

    /* 0x00001c84 reg1825 */
    struct {
        RK_U32 intra_l32_sobel_t2    : 12;
        RK_U32 reserved              : 4;
        RK_U32 intra_l32_sobel_t3    : 12;
        RK_U32 reserved1             : 4;
    } i32_sobel_t_00;

    /* 0x00001c88 reg1826 */
    struct {
        RK_U32 intra_l32_sobel_t4    : 6;
        RK_U32 reserved              : 26;
    } i32_sobel_t_01;

    /* 0x00001c8c reg1827 */
    struct {
        RK_U32 intra_l32_sobel_t5    : 12;
        RK_U32 reserved              : 4;
        RK_U32 intra_l32_sobel_t6    : 12;
        RK_U32 reserved1             : 4;
    } i32_sobel_t_02;

    /* 0x00001c90 reg1828 */
    struct {
        RK_U32 intra_l32_sobel_a1_qp0    : 6;
        RK_U32 intra_l32_sobel_a1_qp1    : 6;
        RK_U32 intra_l32_sobel_a1_qp2    : 6;
        RK_U32 intra_l32_sobel_a1_qp3    : 6;
        RK_U32 intra_l32_sobel_a1_qp4    : 6;
        RK_U32 reserved                  : 2;
    } i32_sobel_a;

    /* 0x00001c94 reg1829 */
    struct {
        RK_U32 intra_l32_sobel_b1_qp0    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l32_sobel_b1_qp1    : 15;
        RK_U32 reserved1                 : 1;
    } i32_sobel_b_00;

    /* 0x00001c98 reg1830 */
    struct {
        RK_U32 intra_l32_sobel_b1_qp2    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l32_sobel_b1_qp3    : 15;
        RK_U32 reserved1                 : 1;
    } i32_sobel_b_01;

    /* 0x00001c9c reg1831 */
    struct {
        RK_U32 intra_l32_sobel_b1_qp4    : 15;
        RK_U32 reserved                  : 17;
    } i32_sobel_b_02;

    /* 0x00001ca0 reg1832 */
    struct {
        RK_U32 intra_l32_sobel_c1_qp0    : 6;
        RK_U32 intra_l32_sobel_c1_qp1    : 6;
        RK_U32 intra_l32_sobel_c1_qp2    : 6;
        RK_U32 intra_l32_sobel_c1_qp3    : 6;
        RK_U32 intra_l32_sobel_c1_qp4    : 6;
        RK_U32 reserved                  : 2;
    } i32_sobel_c;

    /* 0x00001ca4 reg1833 */
    struct {
        RK_U32 intra_l32_sobel_d1_qp0    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l32_sobel_d1_qp1    : 15;
        RK_U32 reserved1                 : 1;
    } i32_sobel_d_00;

    /* 0x00001ca8 reg1834 */
    struct {
        RK_U32 intra_l32_sobel_d1_qp2    : 15;
        RK_U32 reserved                  : 1;
        RK_U32 intra_l32_sobel_d1_qp3    : 15;
        RK_U32 reserved1                 : 1;
    } i32_sobel_d_01;

    /* 0x00001cac reg1835 */
    struct {
        RK_U32 intra_l32_sobel_d1_qp4    : 15;
        RK_U32 reserved                  : 17;
    } i32_sobel_d_02;

    /* 0x00001cb0 reg1836 */
    RK_U32 intra_l32_sobel_e1_qp0_low;

    /* 0x00001cb4 reg1837 */
    struct {
        RK_U32 intra_l32_sobel_e1_qp0_high    : 9;
        RK_U32 reserved                       : 23;
    } i32_sobel_e_01;

    /* 0x00001cb8 reg1838 */
    RK_U32 intra_l32_sobel_e1_qp1_low;

    /* 0x00001cbc reg1839 */
    struct {
        RK_U32 intra_l32_sobel_e1_qp1_high    : 9;
        RK_U32 reserved                       : 23;
    } i32_sobel_e_03;

    /* 0x00001cc0 reg1840 */
    RK_U32 intra_l32_sobel_e1_qp2_low;

    /* 0x00001cc4 reg1841 */
    struct {
        RK_U32 intra_l32_sobel_e1_qp2_high    : 9;
        RK_U32 reserved                       : 23;
    } i32_sobel_e_05;

    /* 0x00001cc8 reg1842 */
    RK_U32 intra_l32_sobel_e1_qp3_low;

    /* 0x00001ccc reg1843 */
    struct {
        RK_U32 intra_l32_sobel_e1_qp3_high    : 9;
        RK_U32 reserved                       : 23;
    } i32_sobel_e_07;

    /* 0x00001cd0 reg1844 */
    RK_U32 intra_l32_sobel_e1_qp4_low;

    /* 0x00001cd4 reg1845 */
    struct {
        RK_U32 intra_l32_sobel_e1_qp4_high    : 9;
        RK_U32 reserved                       : 23;
    } i32_sobel_e_09;
} hevc_vepu580_wgt;

typedef struct {
    struct {
        RK_U32 cu_rdo_cime_thd0 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_cime_thd1 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_cime_thd0;

    struct {
        RK_U32 cu_rdo_cime_thd2 : 12;
        RK_U32 reserved : 20;
    } rdo_b_cime_thd1;

    struct {
        RK_U32 cu_rdo_var_thd00 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd01 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd0;

    struct {
        RK_U32 cu_rdo_var_thd10 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd11 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd1;

    struct {
        RK_U32 cu_rdo_var_thd20 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd21 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd2;

    struct {
        RK_U32 cu_rdo_var_thd30 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd31 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd3;

    struct {
        RK_U32 cu_rdo_atf_wgt00 : 8;
        RK_U32 cu_rdo_atf_wgt01 : 8;
        RK_U32 cu_rdo_atf_wgt02 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt0;

    struct {
        RK_U32 cu_rdo_atf_wgt10 : 8;
        RK_U32 cu_rdo_atf_wgt11 : 8;
        RK_U32 cu_rdo_atf_wgt12 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt1;

    struct {
        RK_U32 cu_rdo_atf_wgt20 : 8;
        RK_U32 cu_rdo_atf_wgt21 : 8;
        RK_U32 cu_rdo_atf_wgt22 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt2;

    struct {
        RK_U32 cu_rdo_atf_wgt30 : 8;
        RK_U32 cu_rdo_atf_wgt31 : 8;
        RK_U32 cu_rdo_atf_wgt32 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt3;
} RdoAtfCfg;


typedef struct {
    struct {
        RK_U32 cu_rdo_cime_thd0 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_cime_thd1 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_cime_thd0;

    struct {
        RK_U32 cu_rdo_cime_thd2 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_cime_thd3 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_cime_thd1;

    struct {
        RK_U32 cu_rdo_var_thd10 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd11 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd0;

    struct {
        RK_U32 cu_rdo_var_thd20 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd21 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd1;

    struct {
        RK_U32 cu_rdo_var_thd30 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd31 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd2;

    struct {
        RK_U32 cu_rdo_var_thd40 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 cu_rdo_var_thd41 : 12;
        RK_U32 reserved1 : 4;
    } rdo_b_var_thd3;

    struct {
        RK_U32 cu_rdo_atf_wgt00 : 8;
        RK_U32 cu_rdo_atf_wgt10 : 8;
        RK_U32 cu_rdo_atf_wgt11 : 8;
        RK_U32 cu_rdo_atf_wgt12 : 8;
    } rdo_b_atf_wgt0;

    struct {
        RK_U32 cu_rdo_atf_wgt20 : 8;
        RK_U32 cu_rdo_atf_wgt21 : 8;
        RK_U32 cu_rdo_atf_wgt22 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt1;

    struct {
        RK_U32 cu_rdo_atf_wgt30 : 8;
        RK_U32 cu_rdo_atf_wgt31 : 8;
        RK_U32 cu_rdo_atf_wgt32 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt2;

    struct {
        RK_U32 cu_rdo_atf_wgt40 : 8;
        RK_U32 cu_rdo_atf_wgt41 : 8;
        RK_U32 cu_rdo_atf_wgt42 : 8;
        RK_U32 reserved : 8;
    } rdo_b_atf_wgt3;
} RdoAtfSkipCfg;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x00002c98 reg2854 */
typedef struct Vepu580RdoCfg_t {
    /* 0x00002000 reg2048 */
    struct {
        RK_U32 rdo_segment_en    : 1;
        RK_U32 rdo_smear_en      : 1;
        RK_U32 reserved          : 30;
    } rdo_sqi_cfg;
    // 0x2004  - 0x2028     start   VEPU_RDO_B64_INTER_CIME_THD0
    RdoAtfCfg rdo_b64_inter_atf;
    // 0x202c  - 0x2050
    RdoAtfSkipCfg rdo_b64_skip_atf;
    // 0x2054  - 0x2078
    RdoAtfCfg rdo_b32_intra_atf;
    // 0x207c  - 0x20a0
    RdoAtfCfg rdo_b32_inter_atf;
    // 0x20a4  - 0x20c8
    RdoAtfSkipCfg rdo_b32_skip_atf;
    // 0x20cc  - 0x20f0
    RdoAtfCfg rdo_b16_intra_atf;
    // 0x20f4  - 0x2118
    RdoAtfCfg rdo_b16_inter_atf;
    // 0x211c  - 0x2140
    RdoAtfSkipCfg rdo_b16_skip_atf;
    // 0x2144  - 0x2168
    RdoAtfCfg rdo_b8_intra_atf;
    // 0x216c  - 0x2190
    RdoAtfCfg rdo_b8_inter_atf;
    // 0x2194  - 0x21b8
    RdoAtfSkipCfg rdo_b8_skip_atf;

    /* 0x000021bc reg2159 */
    struct {
        RK_U32 rdo_segment_cu64_th0    : 12;
        RK_U32 reserved                : 4;
        RK_U32 rdo_segment_cu64_th1    : 12;
        RK_U32 reserved1               : 4;
    } rdo_segment_b64_thd0;

    /* 0x000021c0 reg2160 */
    struct {
        RK_U32 rdo_segment_cu64_th2           : 12;
        RK_U32 reserved                       : 4;
        RK_U32 rdo_segment_cu64_th3           : 4;
        RK_U32 rdo_segment_cu64_th4           : 4;
        RK_U32 rdo_segment_cu64_th5_minus1    : 4;
        RK_U32 rdo_segment_cu64_th6_minus1    : 4;
    } rdo_segment_b64_thd1;

    /* 0x000021c4 reg2161 */
    struct {
        RK_U32 rdo_segment_cu32_th0    : 12;
        RK_U32 reserved                : 4;
        RK_U32 rdo_segment_cu32_th1    : 12;
        RK_U32 reserved1               : 4;
    } rdo_segment_b32_thd0;

    /* 0x000021c8 reg2162 */
    struct {
        RK_U32 rdo_segment_cu32_th2           : 12;
        RK_U32 reserved                       : 4;
        RK_U32 rdo_segment_cu32_th3           : 2;
        RK_U32 reserved1                      : 2;
        RK_U32 rdo_segment_cu32_th4           : 2;
        RK_U32 reserved2                      : 2;
        RK_U32 rdo_segment_cu32_th5_minus1    : 2;
        RK_U32 reserved3                      : 2;
        RK_U32 rdo_segment_cu32_th6_minus1    : 2;
        RK_U32 reserved4                      : 2;
    } rdo_segment_b32_thd1;

    /* 0x000021cc reg2163 */
    struct {
        RK_U32 rdo_segment_cu64_multi    : 8;
        RK_U32 rdo_segment_cu32_multi    : 8;
        RK_U32 rdo_smear_cu16_multi      : 8;
        RK_U32 reserved                  : 8;
    } rdo_segment_multi;

    /* 0x000021d0 reg2164 */
    struct {
        RK_U32 rdo_smear_cu16_cime_sad_th0    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 rdo_smear_cu16_cime_sad_th1    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_b16_smear_thd0;

    /* 0x000021d4 reg2165 */
    struct {
        RK_U32 rdo_smear_cu16_cime_sad_th2    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 rdo_smear_cu16_cime_sad_th3    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_b16_smear_thd1;

    /* 0x000021d8 reg2166 */
    struct {
        RK_U32 pre_intra32_cst_var_th00    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 pre_intra32_cst_var_th01    : 12;
        RK_U32 reserved1                   : 1;
        RK_U32 pre_intra32_mode_th         : 3;
    } preintra_b32_cst_var_thd;

    /* 0x000021dc reg2167 */
    struct {
        RK_U32 pre_intra32_cst_wgt00    : 8;
        RK_U32 reserved                 : 8;
        RK_U32 pre_intra32_cst_wgt01    : 8;
        RK_U32 reserved1                : 8;
    } preintra_b32_cst_wgt;

    /* 0x000021e0 reg2168 */
    struct {
        RK_U32 pre_intra16_cst_var_th00    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 pre_intra16_cst_var_th01    : 12;
        RK_U32 reserved1                   : 1;
        RK_U32 pre_intra16_mode_th         : 3;
    } preintra_b16_cst_var_thd;

    /* 0x000021e4 reg2169 */
    struct {
        RK_U32 pre_intra16_cst_wgt00    : 8;
        RK_U32 reserved                 : 8;
        RK_U32 pre_intra16_cst_wgt01    : 8;
        RK_U32 reserved1                : 8;
    } preintra_b16_cst_wgt;
#if 0
    RK_U32 scaling_list_reg[678]; /* total number really: 678 */
    // 0x2c98
    RK_U32 scal_cfg_reg;
    /* 0x21e8 - 0x21fc */
    RK_U32 reserved2170_2175[6];
#endif
} vepu580_rdo_cfg;

/* class: osd */
/* 0x00003000 reg3072 - 0x00003084 reg3105*/
typedef struct Vepu580OsdCfg_t {
    /* 0x00003000 reg3072 */
    struct {
        RK_U32 osd_lu_inv_en     : 8;
        RK_U32 osd_ch_inv_en     : 8;
        RK_U32 osd_lu_inv_msk    : 8;
        RK_U32 osd_ch_inv_msk    : 8;
    } osd_inv_en;

    /* 0x00003004 reg3073 */
    struct {
        RK_U32 osd_ithd_r0    : 4;
        RK_U32 osd_ithd_r1    : 4;
        RK_U32 osd_ithd_r2    : 4;
        RK_U32 osd_ithd_r3    : 4;
        RK_U32 osd_ithd_r4    : 4;
        RK_U32 osd_ithd_r5    : 4;
        RK_U32 osd_ithd_r6    : 4;
        RK_U32 osd_ithd_r7    : 4;
    } osd_inv_thd;

    /* 0x00003008 reg3074 */
    struct {
        RK_U32 osd_en         : 8;
        RK_U32 osd_itype      : 8;
        RK_U32 osd_plt_cks    : 1;
        RK_U32 osd_plt_typ    : 1;
        RK_U32 reserved       : 14;
    } osd_cfg;

    /* 0x300c */
    RK_U32 reserved_3075;

    /* 0x00003010 reg3076 */
    struct {
        RK_U32 osd0_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd0_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd0_lt_pos;

    /* 0x00003014 reg3077 */
    struct {
        RK_U32 osd0_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd0_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd0_rb_pos;

    /* 0x00003018 reg3078 */
    struct {
        RK_U32 osd1_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd1_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd1_lt_pos;

    /* 0x0000301c reg3079 */
    struct {
        RK_U32 osd1_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd1_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd1_rb_pos;

    /* 0x00003020 reg3080 */
    struct {
        RK_U32 osd2_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd2_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd2_lt_pos;

    /* 0x00003024 reg3081 */
    struct {
        RK_U32 osd2_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd2_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd2_rb_pos;

    /* 0x00003028 reg3082 */
    struct {
        RK_U32 osd3_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd3_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd3_lt_pos;

    /* 0x0000302c reg3083 */
    struct {
        RK_U32 osd3_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd3_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd3_rb_pos;

    /* 0x00003030 reg3084 */
    struct {
        RK_U32 osd4_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd4_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd4_lt_pos;

    /* 0x00003034 reg3085 */
    struct {
        RK_U32 osd4_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd4_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd4_rb_pos;

    /* 0x00003038 reg3086 */
    struct {
        RK_U32 osd5_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd5_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd5_lt_pos;

    /* 0x0000303c reg3087 */
    struct {
        RK_U32 osd5_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd5_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd5_rb_pos;

    /* 0x00003040 reg3088 */
    struct {
        RK_U32 osd6_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd6_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd6_lt_pos;

    /* 0x00003044 reg3089 */
    struct {
        RK_U32 osd6_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd6_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd6_rb_pos;

    /* 0x00003048 reg3090 */
    struct {
        RK_U32 osd7_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd7_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } osd7_lt_pos;

    /* 0x0000304c reg3091 */
    struct {
        RK_U32 osd7_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 osd7_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } osd7_rb_pos;

    /* 0x00003050 reg3092 */
    RK_U32 osd0_addr;

    /* 0x00003054 reg3093 */
    RK_U32 osd1_addr;

    /* 0x00003058 reg3094 */
    RK_U32 osd2_addr;

    /* 0x0000305c reg3095 */
    RK_U32 osd3_addr;

    /* 0x00003060 reg3096 */
    RK_U32 osd4_addr;

    /* 0x00003064 reg3097 */
    RK_U32 osd5_addr;

    /* 0x00003068 reg3098 */
    RK_U32 osd6_addr;

    /* 0x0000306c reg3099 */
    RK_U32 osd7_addr;

    /* 0x3070 - 0x307c */
    RK_U32 reserved3100_3103[4];
    /* 0x03080-0x347c reg3104-reg3360 */
    RK_U32 plt[256];
} vepu580_osd_cfg;

/* class: st */
/* 0x00004000 reg4096 - 0x000042cc reg4275 */
typedef struct Vepu580Status_t {
    /* 0x00004000 reg4096 */
    RK_U32 bs_lgth_l32;

    /* 0x00004004 reg4097 */
    struct {
        RK_U32 bs_lgth_h8    : 8;
        RK_U32 reserved      : 8;
        RK_U32 sse_l16       : 16;
    } st_sse_bsl;

    /* 0x00004008 reg4098 */
    RK_U32 sse_h32;

    /* 0x0000400c reg4099 */
    RK_U32 qp_sum;

    /* 0x00004010 reg4100 */
    struct {
        RK_U32 sao_cnum    : 16;
        RK_U32 sao_ynum    : 16;
    } st_sao;

    /* 0x00004014 reg4101 */
    RK_U32 rdo_head_bits;

    /* 0x00004018 reg4102 */
    struct {
        RK_U32 rdo_head_bits_h8    : 8;
        RK_U32 reserved            : 8;
        RK_U32 rdo_res_bits_l16    : 16;
    } st_head_res_bl;

    /* 0x0000401c reg4103 */
    RK_U32 rdo_res_bits_h24;

    /* 0x00004020 reg4104 */
    struct {
        RK_U32 st_enc      : 2;
        RK_U32 st_sclr     : 1;
        RK_U32 reserved    : 29;
    } st_enc;

    /* 0x00004024 reg4105 */
    struct {
        RK_U32 fnum_cfg_done    : 8;
        RK_U32 fnum_cfg         : 8;
        RK_U32 fnum_int         : 8;
        RK_U32 fnum_enc_done    : 8;
    } st_lkt;

    /* 0x00004028 reg4106 */
    RK_U32 node_addr;

    /* 0x0000402c reg4107 */
    struct {
        RK_U32 bsbw_ovfl    : 1;
        RK_U32 reserved     : 2;
        RK_U32 bsbw_addr    : 28;
        RK_U32 reserved1    : 1;
    } st_bsb;

    /* 0x00004030 reg4108 */
    struct {
        RK_U32 axib_idl     : 8;
        RK_U32 axib_ovfl    : 8;
        RK_U32 axib_err     : 8;
        RK_U32 axir_err     : 7;
        RK_U32 reserved     : 1;
    } st_bus;

    /* 0x00004034 reg4109 */
    struct {
        RK_U32 sli_num     : 6;
        RK_U32 reserved    : 26;
    } st_snum;

    /* 0x00004038 reg4110 */
    struct {
        RK_U32 sli_len     : 25;
        RK_U32 reserved    : 7;
    } st_slen;

    /* 0x403c - 0x40fc */
    RK_U32 reserved4111_4159[49];

    /* 0x00004100 reg4160 */
    struct {
        RK_U32 pnum_p64    : 17;
        RK_U32 reserved    : 15;
    } st_pnum_p64;

    /* 0x00004104 reg4161 */
    struct {
        RK_U32 pnum_p32    : 19;
        RK_U32 reserved    : 13;
    } st_pnum_p32;

    /* 0x00004108 reg4162 */
    struct {
        RK_U32 pnum_p16    : 21;
        RK_U32 reserved    : 11;
    } st_pnum_p16;

    /* 0x0000410c reg4163 */
    struct {
        RK_U32 pnum_p8     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_p8;

    /* 0x00004110 reg4164 */
    struct {
        RK_U32 pnum_i32    : 19;
        RK_U32 reserved    : 13;
    } st_pnum_i32;

    /* 0x00004114 reg4165 */
    struct {
        RK_U32 pnum_i16    : 21;
        RK_U32 reserved    : 11;
    } st_pnum_i16;

    /* 0x00004118 reg4166 */
    struct {
        RK_U32 pnum_i8     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_i8;

    /* 0x0000411c reg4167 */
    struct {
        RK_U32 pnum_i4     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_i4;

    /* 0x00004120 reg4168 */
    RK_U32 madp;

    /* 0x00004124 reg4169 */
    struct {
        RK_U32 num_ctu     : 21;
        RK_U32 reserved    : 11;
    } st_bnum_cme;

    /* 0x00004128 reg4170 */
    RK_U32 madi;

    /* 0x0000412c reg4171 */
    struct {
        RK_U32 num_b16     : 23;
        RK_U32 reserved    : 9;
    } st_bnum_b16;

    /* 0x00004130 reg4172 */
    RK_U32 num_madi_max_b16;

    /* 0x00004134 reg4173 */
    RK_U32 md_sad_b16num0;

    /* 0x00004138 reg4174 */
    RK_U32 md_sad_b16num1;

    /* 0x0000413c reg4175 */
    RK_U32 md_sad_b16num2;

    /* 0x00004140 reg4176 */
    RK_U32 md_sad_b16num3;

    /* 0x00004144 reg4177 */
    RK_U32 madi_b16num0;

    /* 0x00004148 reg4178 */
    RK_U32 madi_b16num1;

    /* 0x0000414c reg4179 */
    RK_U32 madi_b16num2;

    /* 0x00004150 reg4180 */
    RK_U32 madi_b16num3;

    /* 0x4154 - 0x41fc */
    RK_U32 reserved4181_4223[43];

    /* 0x00004200 reg4224 */
    struct {
        RK_U32 b8num_qp0    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp0;

    /* 0x00004204 reg4225 */
    struct {
        RK_U32 b8num_qp1    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp1;

    /* 0x00004208 reg4226 */
    struct {
        RK_U32 b8num_qp2    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp2;

    /* 0x0000420c reg4227 */
    struct {
        RK_U32 b8num_qp3    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp3;

    /* 0x00004210 reg4228 */
    struct {
        RK_U32 b8num_qp4    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp4;

    /* 0x00004214 reg4229 */
    struct {
        RK_U32 b8num_qp5    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp5;

    /* 0x00004218 reg4230 */
    struct {
        RK_U32 b8num_qp6    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp6;

    /* 0x0000421c reg4231 */
    struct {
        RK_U32 b8num_qp7    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp7;

    /* 0x00004220 reg4232 */
    struct {
        RK_U32 b8num_qp8    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp8;

    /* 0x00004224 reg4233 */
    struct {
        RK_U32 b8num_qp9    : 22;
        RK_U32 reserved     : 10;
    } st_b8_qp9;

    /* 0x00004228 reg4234 */
    struct {
        RK_U32 b8num_qp10    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp10;

    /* 0x0000422c reg4235 */
    struct {
        RK_U32 b8num_qp11    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp11;

    /* 0x00004230 reg4236 */
    struct {
        RK_U32 b8num_qp12    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp12;

    /* 0x00004234 reg4237 */
    struct {
        RK_U32 b8num_qp13    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp13;

    /* 0x00004238 reg4238 */
    struct {
        RK_U32 b8num_qp14    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp14;

    /* 0x0000423c reg4239 */
    struct {
        RK_U32 b8num_qp15    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp15;

    /* 0x00004240 reg4240 */
    struct {
        RK_U32 b8num_qp16    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp16;

    /* 0x00004244 reg4241 */
    struct {
        RK_U32 b8num_qp17    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp17;

    /* 0x00004248 reg4242 */
    struct {
        RK_U32 b8num_qp18    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp18;

    /* 0x0000424c reg4243 */
    struct {
        RK_U32 b8num_qp19    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp19;

    /* 0x00004250 reg4244 */
    struct {
        RK_U32 b8num_qp20    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp20;

    /* 0x00004254 reg4245 */
    struct {
        RK_U32 b8num_qp21    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp21;

    /* 0x00004258 reg4246 */
    struct {
        RK_U32 b8num_qp22    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp22;

    /* 0x0000425c reg4247 */
    struct {
        RK_U32 b8num_qp23    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp23;

    /* 0x00004260 reg4248 */
    struct {
        RK_U32 b8num_qp24    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp24;

    /* 0x00004264 reg4249 */
    struct {
        RK_U32 b8num_qp25    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp25;

    /* 0x00004268 reg4250 */
    struct {
        RK_U32 b8num_qp26    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp26;

    /* 0x0000426c reg4251 */
    struct {
        RK_U32 b8num_qp27    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp27;

    /* 0x00004270 reg4252 */
    struct {
        RK_U32 b8num_qp28    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp28;

    /* 0x00004274 reg4253 */
    struct {
        RK_U32 b8num_qp29    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp29;

    /* 0x00004278 reg4254 */
    struct {
        RK_U32 b8num_qp30    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp30;

    /* 0x0000427c reg4255 */
    struct {
        RK_U32 b8num_qp31    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp31;

    /* 0x00004280 reg4256 */
    struct {
        RK_U32 b8num_qp32    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp32;

    /* 0x00004284 reg4257 */
    struct {
        RK_U32 b8num_qp33    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp33;

    /* 0x00004288 reg4258 */
    struct {
        RK_U32 b8num_qp34    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp34;

    /* 0x0000428c reg4259 */
    struct {
        RK_U32 b8num_qp35    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp35;

    /* 0x00004290 reg4260 */
    struct {
        RK_U32 b8num_qp36    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp36;

    /* 0x00004294 reg4261 */
    struct {
        RK_U32 b8num_qp37    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp37;

    /* 0x00004298 reg4262 */
    struct {
        RK_U32 b8num_qp38    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp38;

    /* 0x0000429c reg4263 */
    struct {
        RK_U32 b8num_qp39    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp39;

    /* 0x000042a0 reg4264 */
    struct {
        RK_U32 b8num_qp40    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp40;

    /* 0x000042a4 reg4265 */
    struct {
        RK_U32 b8num_qp41    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp41;

    /* 0x000042a8 reg4266 */
    struct {
        RK_U32 b8num_qp42    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp42;

    /* 0x000042ac reg4267 */
    struct {
        RK_U32 b8num_qp43    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp43;

    /* 0x000042b0 reg4268 */
    struct {
        RK_U32 b8num_qp44    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp44;

    /* 0x000042b4 reg4269 */
    struct {
        RK_U32 b8num_qp45    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp45;

    /* 0x000042b8 reg4270 */
    struct {
        RK_U32 b8num_qp46    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp46;

    /* 0x000042bc reg4271 */
    struct {
        RK_U32 b8num_qp47    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp47;

    /* 0x000042c0 reg4272 */
    struct {
        RK_U32 b8num_qp48    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp48;

    /* 0x000042c4 reg4273 */
    struct {
        RK_U32 b8num_qp49    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp49;

    /* 0x000042c8 reg4274 */
    struct {
        RK_U32 b8num_qp50    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp50;

    /* 0x000042cc reg4275 */
    struct {
        RK_U32 b8num_qp51    : 22;
        RK_U32 reserved      : 10;
    } st_b8_qp51;
} vepu580Status;

/* class: dbg/st/axipn */
/* 0x00005000 reg5120 - 0x00005354 reg5333*/
typedef struct Vepu580Dbg_t {
    /* 0x00005000 reg5120 */
    struct {
        RK_U32 pp_tout      : 1;
        RK_U32 cme_tout     : 1;
        RK_U32 swn_tout     : 1;
        RK_U32 rme_tout     : 1;
        RK_U32 fme_tout     : 1;
        RK_U32 rdo_tout     : 1;
        RK_U32 lpf_tout     : 1;
        RK_U32 etpy_tout    : 1;
        RK_U32 frm_tout     : 1;
        RK_U32 reserved     : 23;
    } st_wdg;

    /* 0x00005004 reg5121 */
    struct {
        RK_U32 pp_wrk      : 1;
        RK_U32 cme_wrk     : 1;
        RK_U32 swn_wrk     : 1;
        RK_U32 rme_wrk     : 1;
        RK_U32 fme_wrk     : 1;
        RK_U32 rdo_wrk     : 1;
        RK_U32 lpf_wrk     : 1;
        RK_U32 etpy_wrk    : 1;
        RK_U32 frm_wrk     : 1;
        RK_U32 reserved    : 23;
    } st_ppl;

    /* 0x00005008 reg5122 */
    struct {
        RK_U32 pp_pos_x    : 16;
        RK_U32 pp_pos_y    : 16;
    } st_ppl_pos_pp;

    /* 0x0000500c reg5123 */
    struct {
        RK_U32 cme_pos_x    : 16;
        RK_U32 cme_pos_y    : 16;
    } st_ppl_pos_cme;

    /* 0x00005010 reg5124 */
    struct {
        RK_U32 swin_pos_x    : 16;
        RK_U32 swin_pos_y    : 16;
    } st_ppl_pos_swin;

    /* 0x00005014 reg5125 */
    struct {
        RK_U32 rme_pos_x    : 16;
        RK_U32 rme_pos_y    : 16;
    } st_ppl_pos_rme;

    /* 0x00005018 reg5126 */
    struct {
        RK_U32 fme_pos_x    : 16;
        RK_U32 fme_pos_y    : 16;
    } st_ppl_pos_fme;

    /* 0x0000501c reg5127 */
    struct {
        RK_U32 rdo_pos_x    : 16;
        RK_U32 rdo_pos_y    : 16;
    } st_ppl_pos_rdo;

    /* 0x00005020 reg5128 */
    struct {
        RK_U32 lpf_pos_x    : 16;
        RK_U32 lpf_pos_y    : 16;
    } st_ppl_pos_lpf;

    /* 0x00005024 reg5129 */
    struct {
        RK_U32 etpy_pos_x    : 16;
        RK_U32 etpy_pos_y    : 16;
    } st_ppl_pos_etpy;

    /* 0x00005028 reg5130 */
    struct {
        RK_U32 sli_num     : 15;
        RK_U32 reserved    : 17;
    } st_sli_num;

    /* 0x0000502c reg5131 */
    struct {
        RK_U32 lkt_err     : 3;
        RK_U32 reserved    : 29;
    } st_lkt_err;

    /* 0x5030 - 0x50fc */
    RK_U32 reserved5132_5183[52];

    /* 0x00005100 reg5184 */
    struct {
        RK_U32 empty_oafifo        : 1;
        RK_U32 full_cmd_oafifo     : 1;
        RK_U32 full_data_oafifo    : 1;
        RK_U32 empty_iafifo        : 1;
        RK_U32 full_cmd_iafifo     : 1;
        RK_U32 full_info_iafifo    : 1;
        RK_U32 fbd_brq_st          : 4;
        RK_U32 fbd_hdr_vld         : 1;
        RK_U32 fbd_bmng_end        : 1;
        RK_U32 nfbd_req_st         : 4;
        RK_U32 acc_axi_cmd         : 8;
        RK_U32 reserved            : 8;
    } dbg_pp_st;

    /* 0x00005104 reg5185 */
    struct {
        RK_U32 cur_state_cime    : 2;
        RK_U32 cur_state_ds      : 3;
        RK_U32 cur_state_ref     : 2;
        RK_U32 cur_state_cst     : 2;
        RK_U32 reserved          : 23;
    } dbg_cime_st;

    /* 0x00005108 reg5186 */
    RK_U32 swin_dbg_inf;

    /* 0x0000510c reg5187 */
    struct {
        RK_U32 bbrq_cmps_left_len2    : 1;
        RK_U32 bbrq_cmps_left_len1    : 1;
        RK_U32 cmps_left_len0         : 1;
        RK_U32 bbrq_rdy2              : 1;
        RK_U32 dcps_vld2              : 1;
        RK_U32 bbrq_rdy1              : 1;
        RK_U32 dcps_vld1              : 1;
        RK_U32 bbrq_rdy0              : 1;
        RK_U32 dcps_vld0              : 1;
        RK_U32 hb_rdy2                : 1;
        RK_U32 bbrq_vld2              : 1;
        RK_U32 hb_rdy1                : 1;
        RK_U32 bbrq_vld1              : 1;
        RK_U32 hb_rdy0                : 1;
        RK_U32 bbrq_vld0              : 1;
        RK_U32 idle_msb2              : 1;
        RK_U32 idle_msb1              : 1;
        RK_U32 idle_msb0              : 1;
        RK_U32 cur_state_dcps         : 1;
        RK_U32 cur_state_bbrq         : 1;
        RK_U32 cur_state_hb           : 1;
        RK_U32 cke_bbrq_dcps          : 1;
        RK_U32 cke_dcps               : 1;
        RK_U32 cke_bbrq               : 1;
        RK_U32 rdy_lwcd_rsp           : 1;
        RK_U32 vld_lwcd_rsp           : 1;
        RK_U32 rdy_lwcd_req           : 1;
        RK_U32 vld_lwcd_req           : 1;
        RK_U32 rdy_lwrsp              : 1;
        RK_U32 vld_lwrsp              : 1;
        RK_U32 rdy_lwreq              : 1;
        RK_U32 vld_lwreq              : 1;
    } dbg_fbd_hhit0;

    /* 0x5110 */
    RK_U32 reserved_5188;

    /* 0x00005114 reg5189 */
    struct {
        RK_U32 mscnt_clr    : 1;
        RK_U32 reserved     : 31;
    } dbg_cach_clr;

    /* 0x00005118 reg5190 */
    RK_U32 l1_mis;

    /* 0x0000511c reg5191 */
    RK_U32 l2_mis;

    /* 0x00005120 reg5192 */
    RK_U32 rdo_st;

    /* 0x00005124 reg5193 */
    RK_U32 rdo_if;

    /* 0x00005128 reg5194 */
    struct {
        RK_U32 h264_sh_st_cs    : 4;
        RK_U32 rsd_st_cs        : 4;
        RK_U32 h264_sd_st_cs    : 5;
        RK_U32 etpy_rdy         : 1;
        RK_U32 reserved         : 18;
    } dbg_etpy;

    /* 0x0000512c reg5195 */
    struct {
        RK_U32 crdy_ppr    : 1;
        RK_U32 cvld_ppr    : 1;
        RK_U32 drdy_ppw    : 1;
        RK_U32 dvld_ppw    : 1;
        RK_U32 crdy_ppw    : 1;
        RK_U32 cvld_ppw    : 1;
        RK_U32 reserved    : 26;
    } dbg_dma_pp;

    /* 0x00005130 reg5196 */
    struct {
        RK_U32 axi_wrdy     : 8;
        RK_U32 axi_wvld     : 8;
        RK_U32 axi_awrdy    : 8;
        RK_U32 axi_awvld    : 8;
    } dbg_dma_w;

    /* 0x00005134 reg5197 */
    struct {
        RK_U32 axi_otsd_read    : 16;
        RK_U32 axi_arrdy        : 7;
        RK_U32 reserved         : 1;
        RK_U32 axi_arvld        : 7;
        RK_U32 reserved1        : 1;
    } dbg_dma_r;

    /* 0x00005138 reg5198 */
    struct {
        RK_U32 dfifo0_lvl    : 4;
        RK_U32 dfifo1_lvl    : 4;
        RK_U32 dfifo2_lvl    : 4;
        RK_U32 dfifo3_lvl    : 4;
        RK_U32 dfifo4_lvl    : 4;
        RK_U32 dfifo5_lvl    : 4;
        RK_U32 reserved      : 6;
        RK_U32 cmd_vld       : 1;
        RK_U32 reserved1     : 1;
    } dbg_dma_rfpr;

    /* 0x0000513c reg5199 */
    struct {
        RK_U32 meiw_busy    : 1;
        RK_U32 dspw_busy    : 1;
        RK_U32 bsw_rdy      : 1;
        RK_U32 bsw_flsh     : 1;
        RK_U32 bsw_busy     : 1;
        RK_U32 crpw_busy    : 1;
        RK_U32 lktw_busy    : 1;
        RK_U32 lpfw_busy    : 1;
        RK_U32 roir_busy    : 1;
        RK_U32 dspr_crdy    : 1;
        RK_U32 dspr_cvld    : 1;
        RK_U32 lktr_busy    : 1;
        RK_U32 lpfr_otsd    : 4;
        RK_U32 rfpr_otsd    : 12;
        RK_U32 dspr_otsd    : 4;
    } dbg_dma_ch_st;

    /* 0x00005140 reg5200 */
    struct {
        RK_U32 cpip_st     : 2;
        RK_U32 mvp_st      : 3;
        RK_U32 qpd6_st     : 2;
        RK_U32 cmd_st      : 2;
        RK_U32 reserved    : 23;
    } dbg_tctrl_cime_st;

    /* 0x00005144 reg5201 */
    struct {
        RK_U32 cme_byps      : 1;
        RK_U32 swin_byps     : 1;
        RK_U32 rme_byps      : 1;
        RK_U32 intra_byps    : 1;
        RK_U32 fme_byps      : 1;
        RK_U32 rdo_byps      : 1;
        RK_U32 lpf_byps      : 1;
        RK_U32 etpy_byps     : 1;
        RK_U32 reserved      : 24;
    } dbg_tctrl;

    /* 0x5148 */
    RK_U32 reserved_5202;

    /* 0x0000514c reg5203 */
    RK_U32 dbg_lpf_st;

    /* 0x00005150 reg5204 */
    RK_U32 dbg_topc_lpfr;

    /* 0x00005154 reg5205 */
    RK_U32 dbg0_cache;

    /* 0x00005158 reg5206 */
    RK_U32 dbg1_cache;

    /* 0x0000515c reg5207 */
    RK_U32 dbg2_cache;

    /* 0x5160 - 0x51fc */
    RK_U32 reserved5208_5247[40];

    /* 0x00005200 reg5248 */
    RK_U32 frame_cyc;

    /* 0x00005204 reg5249 */
    RK_U32 pp_fcyc;

    /* 0x00005208 reg5250 */
    RK_U32 cme_fcyc;

    /* 0x0000520c reg5251 */
    RK_U32 cme_dspr_fcyc;

    /* 0x00005210 reg5252 */
    RK_U32 ldr_fcyc;

    /* 0x00005214 reg5253 */
    RK_U32 rme_fcyc;

    /* 0x00005218 reg5254 */
    RK_U32 fme_fcyc;

    /* 0x0000521c reg5255 */
    RK_U32 rdo_fcyc;

    /* 0x00005220 reg5256 */
    RK_U32 lpf_fcyc;

    /* 0x00005224 reg5257 */
    RK_U32 etpy_fcyc;

    /* 0x5228 - 0x52fc */
    RK_U32 reserved5258_5311[54];

    /* 0x00005300 reg5312 */
    struct {
        RK_U32 axip_e      : 1;
        RK_U32 axip_clr    : 1;
        RK_U32 axip_mod    : 1;
        RK_U32 reserved    : 29;
    } axip0_cmd;

    /* 0x00005304 reg5313 */
    struct {
        RK_U32 axip_ltcy_id     : 4;
        RK_U32 axip_ltcy_thd    : 12;
        RK_U32 reserved         : 16;
    } axip0_ltcy;

    /* 0x00005308 reg5314 */
    struct {
        RK_U32 axip_cnt_typ    : 1;
        RK_U32 axip_cnt_ddr    : 2;
        RK_U32 axip_cnt_rid    : 5;
        RK_U32 axip_cnt_wid    : 5;
        RK_U32 reserved        : 19;
    } axip0_cnt;

    /* 0x530c */
    RK_U32 reserved_5315;

    /* 0x00005310 reg5316 */
    struct {
        RK_U32 axip_e      : 1;
        RK_U32 axip_clr    : 1;
        RK_U32 axip_mod    : 1;
        RK_U32 reserved    : 29;
    } axip1_cmd;

    /* 0x00005314 reg5317 */
    struct {
        RK_U32 axip_ltcy_id     : 4;
        RK_U32 axip_ltcy_thd    : 12;
        RK_U32 reserved         : 16;
    } axip1_ltcy;

    /* 0x00005318 reg5318 */
    struct {
        RK_U32 axip_cnt_typ    : 1;
        RK_U32 axip_cnt_ddr    : 2;
        RK_U32 axip_cnt_rid    : 5;
        RK_U32 axip_cnt_wid    : 5;
        RK_U32 reserved        : 19;
    } axip1_cnt;

    /* 0x531c */
    RK_U32 reserved_5319;

    /* 0x00005320 reg5320 */
    struct {
        RK_U32 axip_max_ltcy    : 16;
        RK_U32 reserved         : 16;
    } st_axip0_maxl;

    /* 0x00005324 reg5321 */
    RK_U32 axip0_num_ltcy;

    /* 0x00005328 reg5322 */
    RK_U32 axip0_sum_ltcy;

    /* 0x0000532c reg5323 */
    RK_U32 axip0_rbyt;

    /* 0x00005330 reg5324 */
    RK_U32 axip0_wbyt;

    /* 0x00005334 reg5325 */
    RK_U32 axip0_wrk_cyc;

    /* 0x5338 - 0x533c */
    RK_U32 reserved5326_5327[2];

    /* 0x00005340 reg5328 */
    struct {
        RK_U32 axip_max_ltcy    : 16;
        RK_U32 reserved         : 16;
    } st_axip1_maxl;

    /* 0x00005344 reg5329 */
    RK_U32 axip1_num_ltcy;

    /* 0x00005348 reg5330 */
    RK_U32 axip1_sum_ltcy;

    /* 0x0000534c reg5331 */
    RK_U32 axip1_rbyt;

    /* 0x00005350 reg5332 */
    RK_U32 axip1_wbyt;

    /* 0x00005354 reg5333 */
    RK_U32 axip1_wrk_cyc;
} Vepu580_dbg;

typedef struct H265eV580RegSet_t {
    hevc_vepu580_control_cfg reg_ctl;
    hevc_vepu580_base reg_base;
    hevc_vepu580_rc_klut reg_rc_klut;
    hevc_vepu580_wgt reg_wgt;
    vepu580_rdo_cfg reg_rdo;
    vepu580_osd_cfg reg_osd_cfg;
    Vepu580_dbg reg_dbg;
} H265eV580RegSet;

typedef struct H265eV580StatusElem_t {
    RK_U32 hw_status;
    vepu580Status st;
} H265eV580StatusElem;

#endif
