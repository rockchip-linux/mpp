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

#ifndef __HAL_H265E_VEPU540C_REG_H__
#define __HAL_H265E_VEPU540C_REG_H__

#include "rk_type.h"
#include "vepu540c_common.h"
/* class: control/link */
/* 0x00000000 reg0 - 0x00000120 reg72 */
typedef struct HevcVepu540cControlCfg_t {
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
        RK_U32 vepu_cmd    : 3;
        RK_U32 reserved    : 21;
    } reg0004_enc_strt;

    /* 0x00000014 reg5 */
    struct {
        RK_U32 safe_clr     : 1;
        RK_U32 force_clr    : 1;
        RK_U32 reserved     : 30;
    } reg0005_enc_clr;

    /* 0x18 */
    struct {
        RK_U32 vswm_lcnt_soft    : 14;
        RK_U32 vswm_fcnt_soft    : 8;
        RK_U32 reserved          : 2;
        RK_U32 dvbm_ack_soft     : 1;
        RK_U32 dvbm_ack_sel      : 1;
        RK_U32 dvbm_inf_sel      : 1;
        RK_U32 reserved1         : 5;
    } reg0006_vs_ldly;

    /* 0x1c */
    RK_U32 reserved007;

    /* 0x00000020 reg8 */
    struct {
        RK_U32 enc_done_en          : 1;
        RK_U32 lkt_node_done_en     : 1;
        RK_U32 sclr_done_en         : 1;
        RK_U32 vslc_done_en         : 1;
        RK_U32 vbsf_oflw_en         : 1;
        RK_U32 vbuf_lens_en         : 1;
        RK_U32 enc_err_en           : 1;
        RK_U32 dvbm_fcfg_en         : 1;
        RK_U32 wdg_en               : 1;
        RK_U32 lkt_err_int_en       : 1;
        RK_U32 lkt_err_stop_en      : 1;
        RK_U32 lkt_force_stop_en    : 1;
        RK_U32 jslc_done_en         : 1;
        RK_U32 jbsf_oflw_en         : 1;
        RK_U32 jbuf_lens_en         : 1;
        RK_U32 dvbm_dcnt_en         : 1;
        RK_U32 reserved             : 16;
    } reg0008_int_en;

    /* 0x00000024 reg9 */
    struct {
        RK_U32 enc_done_msk          : 1;
        RK_U32 lkt_node_done_msk     : 1;
        RK_U32 sclr_done_msk         : 1;
        RK_U32 vslc_done_msk         : 1;
        RK_U32 vbsf_oflw_msk         : 1;
        RK_U32 vbuf_lens_msk         : 1;
        RK_U32 enc_err_msk           : 1;
        RK_U32 dvbm_fcfg_msk         : 1;
        RK_U32 wdg_msk               : 1;
        RK_U32 lkt_err_int_msk       : 1;
        RK_U32 lkt_err_stop_msk      : 1;
        RK_U32 lkt_force_stop_msk    : 1;
        RK_U32 jslc_done_msk         : 1;
        RK_U32 jbsf_oflw_msk         : 1;
        RK_U32 jbuf_lens_msk         : 1;
        RK_U32 dvbm_dcnt_msk         : 1;
        RK_U32 reserved              : 16;
    } reg0009_int_msk;

    /* 0x00000028 reg10 */
    struct {
        RK_U32 enc_done_clr          : 1;
        RK_U32 lkt_node_done_clr     : 1;
        RK_U32 sclr_done_clr         : 1;
        RK_U32 vslc_done_clr         : 1;
        RK_U32 vbsf_oflw_clr         : 1;
        RK_U32 vbuf_lens_clr         : 1;
        RK_U32 enc_err_clr           : 1;
        RK_U32 dvbm_fcfg_clr         : 1;
        RK_U32 wdg_clr               : 1;
        RK_U32 lkt_err_int_clr       : 1;
        RK_U32 lkt_err_stop_clr      : 1;
        RK_U32 lkt_force_stop_clr    : 1;
        RK_U32 jslc_done_clr         : 1;
        RK_U32 jbsf_oflw_clr         : 1;
        RK_U32 jbuf_lens_clr         : 1;
        RK_U32 dvbm_dcnt_clr         : 1;
        RK_U32 reserved              : 16;
    } reg0010_int_clr;

    /* 0x0000002c reg11 */
    struct {
        RK_U32 enc_done_sta          : 1;
        RK_U32 lkt_node_done_sta     : 1;
        RK_U32 sclr_done_sta         : 1;
        RK_U32 vslc_done_sta         : 1;
        RK_U32 vbsf_oflw_sta         : 1;
        RK_U32 vbuf_lens_sta         : 1;
        RK_U32 enc_err_sta           : 1;
        RK_U32 dvbm_fcfg_sta         : 1;
        RK_U32 wdg_sta               : 1;
        RK_U32 lkt_err_int_sta       : 1;
        RK_U32 lkt_err_stop_sta      : 1;
        RK_U32 lkt_force_stop_sta    : 1;
        RK_U32 jslc_done_sta         : 1;
        RK_U32 jbsf_oflw_sta         : 1;
        RK_U32 jbuf_lens_sta         : 1;
        RK_U32 dvbm_dcnt_sta         : 1;
        RK_U32 reserved              : 16;
    } reg0011_int_sta;

    /* 0x00000030 reg12 */
    struct {
        RK_U32 jpeg_bus_edin        : 4;
        RK_U32 src_bus_edin         : 4;
        RK_U32 meiw_bus_edin        : 4;
        RK_U32 bsw_bus_edin         : 4;
        RK_U32 lktr_bus_edin        : 4;
        RK_U32 roir_bus_edin        : 4;
        RK_U32 lktw_bus_edin        : 4;
        RK_U32 rec_nfbc_bus_edin    : 4;
    } reg0012_dtrns_map;

    /* 0x00000034 reg13 */
    struct {
        RK_U32 reserved        : 16;
        RK_U32 axi_brsp_cke    : 10;
        RK_U32 reserved1       : 6;
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
        RK_U32 link_err_stop       : 1;
        RK_U32 reserved            : 27;
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
    } reg0022_rdo_ckg;

    /* 0x0000005c reg23 */
    struct {
        RK_U32 core_id     : 2;
        RK_U32 reserved    : 30;
    } reg0023_enc_id;


    /* 0x00000060 reg24 */
    struct {
        RK_U32 dvbm_en            : 1;
        RK_U32 src_badr_sel       : 1;
        RK_U32 vinf_frm_match     : 1;
        RK_U32 reserved           : 1;
        RK_U32 vrsp_half_cycle    : 4;
        RK_U32 reserved1          : 24;
    } reg0024_dvbm_cfg;

    /* 0x00000064 - 0x6c*/
    RK_U32 reg025_027[3];

    /* 0x00000070*/
    struct {
        RK_U32 reserved    : 4;
        RK_U32 lkt_addr    : 28;
    } reg0028_lkt_base_addr;

    /* 0x74 - 0xfc */
    RK_U32 reserved29_63[35];

    struct {
        RK_U32 node_core_id    : 2;
        RK_U32 node_int        : 1;
        RK_U32 reserved        : 1;
        RK_U32 task_id         : 12;
        RK_U32 reserved1       : 16;
    } reg0064_lkt_node_cfg;

    /* 0x00000104 reg65 */
    struct {
        RK_U32 pcfg_rd_en       : 1;
        RK_U32 reserved         : 3;
        RK_U32 lkt_addr_pcfg    : 28;
    } reg0065_lkt_addr_pcfg;

    /* 0x00000108 reg66 */
    struct {
        RK_U32 rc_cfg_rd_en       : 1;
        RK_U32 reserved           : 3;
        RK_U32 lkt_addr_rc_cfg    : 28;
    } reg0066_lkt_addr_rc_cfg;

    /* 0x0000010c reg67 */
    struct {
        RK_U32 par_cfg_rd_en       : 1;
        RK_U32 reserved            : 3;
        RK_U32 lkt_addr_par_cfg    : 28;
    } reg0067_lkt_addr_par_cfg;

    /* 0x00000110 reg68 */
    struct {
        RK_U32 sqi_cfg_rd_en       : 1;
        RK_U32 reserved            : 3;
        RK_U32 lkt_addr_sqi_cfg    : 28;
    } reg0068_lkt_addr_sqi_cfg;

    /* 0x00000114 reg69 */
    struct {
        RK_U32 scal_cfg_rd_en       : 1;
        RK_U32 reserved             : 3;
        RK_U32 lkt_addr_scal_cfg    : 28;
    } reg0069_lkt_addr_scal_cfg;

    /* 0x00000118 reg70 */
    struct {
        RK_U32 pp_cfg_rd_en       : 1;
        RK_U32 reserved           : 3;
        RK_U32 lkt_addr_pp_cfg    : 28;
    } reg0070_lkt_addr_osd_cfg;

    /* 0x0000011c reg71 */
    struct {
        RK_U32 st_out_en      : 1;
        RK_U32 reserved       : 3;
        RK_U32 lkt_addr_st    : 28;
    } reg0071_lkt_addr_st;

    /* 0x00000120 reg72 */
    struct {
        RK_U32 nxt_node_vld    : 1;
        RK_U32 reserved        : 3;
        RK_U32 lkt_addr_nxt    : 28;
    } reg0072_lkt_addr_nxt;
} hevc_vepu540c_control_cfg;

/* class: buffer/video syntax */
/* 0x00000280 reg160 - 0x000003f4 reg253*/
typedef struct HevcVepu540cBase_t {
    vepu540c_online online_addr;
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
    RK_U32 reg0174_adr_bsbs;

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

    /* 0x2ec - 0x2fc */
    RK_U32 reserved187_191[5];

    /* 0x00000300 reg192 */
    struct {
        RK_U32 enc_stnd                : 2;
        RK_U32 cur_frm_ref             : 1;
        RK_U32 mei_stor                : 1;
        RK_U32 bs_scp                  : 1;
        RK_U32 reserved                : 3;
        RK_U32 pic_qp                  : 6;
        RK_U32 num_pic_tot_cur         : 5;
        RK_U32 log2_ctu_num            : 5;
        RK_U32 reserved1               : 6;
        RK_U32 slen_fifo               : 1;
        RK_U32 rec_fbc_dis             : 1;
    } reg0192_enc_pic;


    /* 0x304 */
    RK_U32 reserved_193;

    /* 0x00000308 reg194 */
    struct {
        RK_U32 frame_id       : 8;
        RK_U32 reserved       : 8;
        RK_U32 ch_id          : 2;
        RK_U32 vrsp_rtn_en    : 1;
        RK_U32 reserved1      : 13;
    } reg0194_dvbm_id;

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
        RK_U32 reserved     : 26;
        RK_U32 src_mirr     : 1;
        RK_U32 src_rot      : 2;
        RK_U32 reserved1    : 3;
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

    /* 0x0000036c reg219 */
    struct {
        RK_U32 uvc_partition0_len    : 12;
        RK_U32 uvc_partition_len     : 12;
        RK_U32 uvc_skip_len          : 6;
        RK_U32 reserved              : 2;
    } reg0218_uvc_cfg;

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
        RK_U32 cime_size_rama     : 10;
        RK_U32 reserved           : 1;
        RK_U32 cime_hgt_rama      : 5;
        RK_U32 reserved1          : 2;
        RK_U32 cme_linebuf_w      : 10;
        RK_U32 fme_prefsu_en      : 2;
        RK_U32 colmv_stor         : 1;
        RK_U32 colmv_load         : 1;
    } reg0222_me_cach;


    /* 0x37c - 0x39c */
    RK_U32 reserved223_231[9];

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 ltm_col                        : 1;
        RK_U32 ltm_idx0l0                     : 1;
        RK_U32 chrm_spcl                      : 1;
        RK_U32 cu_inter_e                     : 12;
        RK_U32 reserved                       : 4;
        RK_U32 cu_intra_e                     : 4;
        RK_U32 ccwa_e                         : 1;
        RK_U32 scl_lst_sel                    : 2;
        RK_U32 lambda_qp_use_avg_cu16_flag    : 1;
        RK_U32 yuvskip_calc_en                : 1;
        RK_U32 atf_e                          : 1;
        RK_U32 atr_e                          : 1;
        RK_U32 reserved1                      : 2;
    }  reg0232_rdo_cfg;

    /* 0x000003a4 reg233 */
    struct {
        RK_U32 rdo_mark_mode    : 9;
        RK_U32 reserved         : 23;
    }  reg0233_iprd_csts;

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

    struct {
        RK_U32 sao_lambda_multi    : 3;
        RK_U32 reserved            : 29;
    } reg0248_sao_cfg;

    /* 0x3e4 - 0x3ec */
    RK_U32 reserved249_251[3];

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

    /* 0x3f8 - 0x3fc */
    RK_U32 reserved254_255[2];

    /* 0x00000400 reg256 - 0x00000480 reg288 */
    Vepu540cJpegReg jpegReg;

} hevc_vepu540c_base;

/* class: rc/roi/aq/klut */
/* 0x00001000 reg1024 - 0x000010e0 reg1080 */
typedef struct HevcVepu540cRcRoi_t {
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
    RK_U32 reserved_1040;

    /* 0x00001044 reg1041 - 0x00001050 reg1044 */
    RK_U8 aq_tthd[16];

    /*
     * 0x00001054 reg1045 - 0x00001060 reg1048
     * only low 6 bits is valid for per step.
     */
    RK_U8 aq_step[16];

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

    /* 0x1078 - 0x107c */
    RK_U32 reserved1052_1054[3];

    /* 0x0000107c reg1055 */
    struct {
        RK_U32 chrm_klut_ofst                : 4;
        RK_U32 reserved                      : 4;
        RK_U32 inter_chrm_dist_multi         : 6;
        RK_U32 reserved1                     : 18;
    } klut_ofst;

    /*0x00001080 reg1056 - 0x0000110c reg1091 */
    Vepu540cRoiCfg roi_cfg;
} hevc_vepu540c_rc_roi;

/* class: iprd/iprd_wgt/rdo_wgta/prei_dif*/
/* 0x00001700 reg1472 - 0x00001cd4 reg1845 */
typedef struct HevcVepu540cWgt_t {
    /* 0x00001700 - 0x0000172c reg1472 */
    RK_U32 reserved1472_1483[12];

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
        RK_U32 cime_pmv_num      : 1;
        RK_U32 cime_fuse         : 1;
        RK_U32 itp_mode          : 1;
        RK_U32 reserved          : 1;
        RK_U32 move_lambda       : 4;
        RK_U32 rime_lvl_mrg      : 2;
        RK_U32 rime_prelvl_en    : 2;
        RK_U32 rime_prersu_en    : 3;
        RK_U32 reserved1         : 17;
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
        RK_U32 cime_madp_th    : 12;
        RK_U32 reserved        : 20;
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

    /* 0x00001800 reg1536 */
    struct {
        RK_U32 lambda_satd_offset    : 5;
        RK_U32 reserved              : 27;
    } iprd_lamb_satd_ofst;

    /* 0x1804 - 0x18fc */
    RK_U32 reserved1537_1599[63];;

    /* wgt_qp48_grpa */
    /* 0x00001900 reg1600 */
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} hevc_vepu540c_wgt;

typedef struct H265eV540cRegSet_t {
    hevc_vepu540c_control_cfg reg_ctl;
    hevc_vepu540c_base reg_base;
    hevc_vepu540c_rc_roi reg_rc_roi;
    hevc_vepu540c_wgt reg_wgt;
    vepu540c_rdo_cfg reg_rdo;
    vepu540c_scl_cfg   reg_scl;
    vepu540c_jpeg_tab jpeg_table;
    vepu540c_dbg reg_dbg;
} H265eV540cRegSet;

typedef struct H265eV540cStatusElem_t {
    RK_U32 hw_status;
    vepu540c_status st;
} H265eV540cStatusElem;

#endif
