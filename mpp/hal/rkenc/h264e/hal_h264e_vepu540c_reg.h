/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H264E_VEPU540C_REG_H__
#define __HAL_H264E_VEPU540C_REG_H__

#include "rk_type.h"
#include "vepu540c_common.h"

/* class: control/link */
/* 0x00000000 reg0 - 0x00000120 reg72 */
typedef struct Vepu540cControlCfg_t {
    /* 0x00000000 reg0 */
    struct {
        RK_U32 sub_ver      : 8;
        RK_U32 cap     : 1;
        RK_U32 hevc_cap     : 1;
        RK_U32 reserved     : 2;
        RK_U32 res_cap      : 4;
        RK_U32 osd_cap      : 2;
        RK_U32 filtr_cap    : 2;
        RK_U32 bfrm_cap     : 1;
        RK_U32 fbc_cap      : 2;
        RK_U32 reserved1    : 1;
        RK_U32 ip_id        : 8;
    } version;

    /* 0x4 - 0xc */
    RK_U32 reserved1_3[3];

    /* 0x00000010 reg4 */
    struct {
        RK_U32 lkt_num     : 8;
        RK_U32 vepu_cmd    : 2;
        RK_U32 reserved    : 22;
    } enc_strt;

    /* 0x00000014 reg5 */
    struct {
        RK_U32 safe_clr     : 1;
        RK_U32 force_clr    : 1;
        RK_U32 reserved     : 30;
    } enc_clr;

    /* 0x00000018 reg6 */
    struct {
        RK_U32 vswm_lcnt_soft    : 14;
        RK_U32 vswm_fcnt_soft    : 8;
        RK_U32 reserved          : 2;
        RK_U32 dvbm_ack_soft     : 1;
        RK_U32 dvbm_ack_sel      : 1;
        RK_U32 dvbm_inf_sel      : 1;
        RK_U32 reserved1         : 5;
    } vs_ldly;

    /* 0x1c */
    RK_U32 reserved_7;

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
    } int_en;

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
    } int_msk;

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
    } int_clr;

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
    } int_sta;

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
    } dtrns_map;

    /* 0x00000034 reg13 */
    struct {
        RK_U32 reserved        : 16;
        RK_U32 axi_brsp_cke    : 10;
        RK_U32 reserved1       : 6;
    } dtrns_cfg;

    /* 0x00000038 reg14 */
    struct {
        RK_U32 vs_load_thd     : 24;
        RK_U32 rfp_load_thd    : 8;
    } enc_wdg;

    /* 0x0000003c reg15 */
    struct {
        RK_U32 hurry_en      : 1;
        RK_U32 hurry_low     : 3;
        RK_U32 hurry_mid     : 3;
        RK_U32 hurry_high    : 3;
        RK_U32 reserved      : 22;
    } qos_cfg;

    /* 0x00000040 reg16 */
    struct {
        RK_U32 qos_period    : 16;
        RK_U32 reserved      : 16;
    } qos_perd;

    /* 0x00000044 reg17 */
    RK_U32 hurry_thd_low;

    /* 0x00000048 reg18 */
    RK_U32 hurry_thd_mid;

    /* 0x0000004c reg19 */
    RK_U32 hurry_thd_high;

    /* 0x00000050 reg20 */
    struct {
        RK_U32 idle_en_core    : 1;
        RK_U32 idle_en_axi     : 1;
        RK_U32 idle_en_ahb     : 1;
        RK_U32 reserved        : 29;
    } enc_idle_en;

    /* 0x00000054 reg21 */
    struct {
        RK_U32 cke                 : 1;
        RK_U32 resetn_hw_en        : 1;
        RK_U32 enc_done_tmvp_en    : 1;
        RK_U32 sram_ckg_en         : 1;
        RK_U32 link_err_stop       : 1;
        RK_U32 reserved            : 27;
    } func_en;

    /* 0x00000058 reg22 */
    struct {
        RK_U32 tq8_ckg           : 1;
        RK_U32 tq4_ckg           : 1;
        RK_U32 bits_ckg_8x8      : 1;
        RK_U32 bits_ckg_4x4_1    : 1;
        RK_U32 bits_ckg_4x4_0    : 1;
        RK_U32 inter_mode_ckg    : 1;
        RK_U32 inter_ctrl_ckg    : 1;
        RK_U32 inter_pred_ckg    : 1;
        RK_U32 intra8_ckg        : 1;
        RK_U32 intra4_ckg        : 1;
        RK_U32 reserved          : 22;
    } rdo_ckg;

    /* 0x0000005c reg23 */
    struct {
        RK_U32 core_id     : 2;
        RK_U32 reserved    : 30;
    } enc_id;
    /* 0x00000060 reg24 */
    struct {
        RK_U32 dvbm_en            : 1;
        RK_U32 src_badr_sel       : 1;
        RK_U32 vinf_frm_match     : 1;
        RK_U32 reserved           : 1;
        RK_U32 vrsp_half_cycle    : 4;
        RK_U32 reserved1          : 24;
    } dvbm_cfg;
} Vepu540cControlCfg;

/* class: buffer/video syntax */
/* 0x00000280 reg160 - 0x000003f4 reg253*/
typedef struct Vepu540cBaseCfg_t {
    vepu540c_online online_addr;
    /* 0x00000280 reg160 */
    RK_U32 adr_src0;

    /* 0x00000284 reg161 */
    RK_U32 adr_src1;

    /* 0x00000288 reg162 */
    RK_U32 adr_src2;

    /* 0x0000028c reg163 */
    RK_U32 rfpw_h_addr;

    /* 0x00000290 reg164 */
    RK_U32 rfpw_b_addr;

    /* 0x00000294 reg165 */
    RK_U32 rfpr_h_addr;

    /* 0x00000298 reg166 */
    RK_U32 rfpr_b_addr;

    /* 0x0000029c reg167 */
    RK_U32 cmvw_addr;

    /* 0x000002a0 reg168 */
    RK_U32 cmvr_addr;

    /* 0x000002a4 reg169 */
    RK_U32 dspw_addr;

    /* 0x000002a8 reg170 */
    RK_U32 dspr_addr;

    /* 0x000002ac reg171 */
    RK_U32 meiw_addr;

    /* 0x000002b0 reg172 */
    RK_U32 bsbt_addr;

    /* 0x000002b4 reg173 */
    RK_U32 bsbb_addr;

    /* 0x000002b8 reg174 */
    RK_U32 adr_bsbs;

    /* 0x000002bc reg175 */
    RK_U32 bsbr_addr;

    /* 0x000002c0 reg176 */
    RK_U32 lpfw_addr;

    /* 0x000002c4 reg177 */
    RK_U32 lpfr_addr;

    /* 0x000002c8 reg178 */
    RK_U32 ebuft_addr ;

    /* 0x000002cc reg179 */
    RK_U32 ebufb_addr;

    /* 0x000002d0 reg180 */
    RK_U32 rfpt_h_addr;

    /* 0x000002d4 reg180 */
    RK_U32 rfpb_h_addr;

    /* 0x000002d8 reg182 */
    RK_U32 rfpt_b_addr;

    /* 0x000002dc reg183 */
    RK_U32 adr_rfpb_b;

    /* 0x000002e0 reg184 */
    RK_U32 adr_smear_rd;

    /* 0x000002e4 reg185 */
    RK_U32 adr_smear_wr;

    /* 0x000002e8 reg186 */
    RK_U32 adr_roir ;

    /* 0x2ec - 0x2fc */
    RK_U32 reserved187_191[5];

    /* 0x00000300 reg192 */
    struct {
        RK_U32 enc_stnd           : 2;
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
    } enc_pic;

    /* 0x304 */
    RK_U32 reserved_193;

    /* 0x00000308 reg194 */
    struct {
        RK_U32 frame_id       : 8;
        RK_U32 reserved       : 8;
        RK_U32 ch_id          : 2;
        RK_U32 vrsp_rtn_en    : 1;
        RK_U32 reserved1      : 13;
    } dvbm_id;

    /* 0x0000030c reg195 */
    RK_U32 bsp_size;
    /* 0x00000310 reg196 */
    struct {
        RK_U32 pic_wd8_m1    : 11;
        RK_U32 reserved      : 5;
        RK_U32 pic_hd8_m1    : 11;
        RK_U32 reserved1     : 5;
    } enc_rsl;

    /* 0x00000314 reg197 */
    struct {
        RK_U32 pic_wfill    : 6;
        RK_U32 reserved     : 10;
        RK_U32 pic_hfill    : 6;
        RK_U32 reserved1    : 10;
    } src_fill;

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
    } src_fmt;

    /* 0x0000031c reg199 */
    struct {
        RK_U32 csc_wgt_b2y    : 9;
        RK_U32 csc_wgt_g2y    : 9;
        RK_U32 csc_wgt_r2y    : 9;
        RK_U32 reserved       : 5;
    } src_udfy;

    /* 0x00000320 reg200 */
    struct {
        RK_U32 csc_wgt_b2u    : 9;
        RK_U32 csc_wgt_g2u    : 9;
        RK_U32 csc_wgt_r2u    : 9;
        RK_U32 reserved       : 5;
    } src_udfu;

    /* 0x00000324 reg201 */
    struct {
        RK_U32 csc_wgt_b2v    : 9;
        RK_U32 csc_wgt_g2v    : 9;
        RK_U32 csc_wgt_r2v    : 9;
        RK_U32 reserved       : 5;
    } src_udfv;

    /* 0x00000328 reg202 */
    struct {
        RK_U32 csc_ofst_v    : 8;
        RK_U32 csc_ofst_u    : 8;
        RK_U32 csc_ofst_y    : 5;
        RK_U32 reserved      : 11;
    } src_udfo;

    /* 0x0000032c reg203 */
    struct {
        RK_U32 reserved     : 26;
        RK_U32 src_mirr     : 1;
        RK_U32 src_rot      : 2;
        RK_U32 reserved1    : 3;
    } src_proc;

    /* 0x00000330 reg204 */
    struct {
        RK_U32 pic_ofst_x    : 14;
        RK_U32 reserved      : 2;
        RK_U32 pic_ofst_y    : 14;
        RK_U32 reserved1     : 2;
    } pic_ofst;

    /* 0x00000334 reg205 */
    struct {
        RK_U32 src_strd0    : 17;
        RK_U32 reserved     : 15;
    } src_strd0;

    /* 0x00000338 reg206 */
    struct {
        RK_U32 src_strd1    : 16;
        RK_U32 reserved     : 16;
    } src_strd1;

    /* 0x0000033c reg207 */
    struct {
        RK_U32 pp_corner_filter_strength      : 2;
        RK_U32 reserved                       : 2;
        RK_U32 pp_edge_filter_strength        : 2;
        RK_U32 reserved1                      : 2;
        RK_U32 pp_internal_filter_strength    : 2;
        RK_U32 reserved2                      : 22;
    } src_flt_cfg;

    /* 0x340 - 0x34c */
    RK_U32 reserved208_211[4];

    /* 0x00000350 reg212 */
    struct {
        RK_U32 rc_en         : 1;
        RK_U32 aq_en         : 1;
        RK_U32 aq_mode       : 1;
        RK_U32 reserved      : 9;
        RK_U32 rc_ctu_num    : 20;
    } rc_cfg;

    /* 0x00000354 reg213 */
    struct {
        RK_U32 reserved       : 16;
        RK_U32 rc_qp_range    : 4;
        RK_U32 rc_max_qp      : 6;
        RK_U32 rc_min_qp      : 6;
    } rc_qp;

    /* 0x00000358 reg214 */
    struct {
        RK_U32 ctu_ebit    : 20;
        RK_U32 reserved    : 12;
    } rc_tgt;

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
    } sli_splt;

    /* 0x00000364 reg217 */
    struct {
        RK_U32 sli_splt_byte    : 20;
        RK_U32 reserved         : 12;
    } sli_byte;

    /* 0x00000368 reg218 */
    struct {
        RK_U32 sli_splt_cnum_m1    : 20;
        RK_U32 reserved            : 12;
    } sli_cnum;

    /* 0x36c */
    /* 0x0000036c reg219 */
    struct {
        RK_U32 uvc_partition0_len    : 12;
        RK_U32 uvc_partition_len     : 12;
        RK_U32 uvc_skip_len          : 6;
        RK_U32 reserved              : 2;
    } uvc_cfg;

    /* 0x00000370 reg220 */
    struct {
        RK_U32 cime_srch_dwnh    : 4;
        RK_U32 cime_srch_uph     : 4;
        RK_U32 cime_srch_rgtw    : 4;
        RK_U32 cime_srch_lftw    : 4;
        RK_U32 dlt_frm_num       : 16;
    } me_rnge;

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
    } me_cfg;

    /* 0x00000378 reg222 */
    struct {
        RK_U32 cime_size_rama     : 10;
        RK_U32 reserved           : 1;
        RK_U32 cime_hgt_rama      : 5;
        RK_U32 reserved1          : 2;
        RK_U32 cme_linebuf_w      : 10;
        RK_U32 fme_prefsu_en      : 2;
        RK_U32 colmv_stor_hevc    : 1;
        RK_U32 colmv_load_hevc    : 1;
    } me_cach;

    /* 0x37c - 0x39c */
    RK_U32 reserved223_231[9];
    /* 0x000003a0 reg232 */
    struct {
        RK_U32 rect_size      : 1;
        RK_U32 reserved       : 2;
        RK_U32 vlc_lmt        : 1;
        RK_U32 chrm_spcl      : 1;
        RK_U32 reserved1      : 8;
        RK_U32 ccwa_e         : 1;
        RK_U32 reserved2      : 1;
        RK_U32 intra_cost_e   : 1;
        RK_U32 reserved3      : 4;
        RK_U32 scl_lst_sel    : 2;
        RK_U32 reserved4      : 6;
        RK_U32 atf_e          : 1;
        RK_U32 atr_e          : 1;
        RK_U32 reserved5      : 2;
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
        RK_U32 nal_ref_idc      : 2;
        RK_U32 nal_unit_type    : 5;
        RK_U32 reserved         : 25;
    } synt_nal;

    /* 0x000003b4 reg237 */
    struct {
        RK_U32 max_fnum    : 4;
        RK_U32 drct_8x8    : 1;
        RK_U32 mpoc_lm4    : 4;
        RK_U32 reserved    : 23;
    } synt_sps;

    /* 0x000003b8 reg238 */
    struct {
        RK_U32 etpy_mode       : 1;
        RK_U32 trns_8x8        : 1;
        RK_U32 csip_flag       : 1;
        RK_U32 num_ref0_idx    : 2;
        RK_U32 num_ref1_idx    : 2;
        RK_U32 pic_init_qp     : 6;
        RK_U32 cb_ofst         : 5;
        RK_U32 cr_ofst         : 5;
        RK_U32 reserved        : 1;
        RK_U32 dbf_cp_flg      : 1;
        RK_U32 reserved1       : 7;
    } synt_pps;

    /* 0x000003bc reg239 */
    struct {
        RK_U32 sli_type        : 2;
        RK_U32 pps_id          : 8;
        RK_U32 drct_smvp       : 1;
        RK_U32 num_ref_ovrd    : 1;
        RK_U32 cbc_init_idc    : 2;
        RK_U32 reserved        : 2;
        RK_U32 frm_num         : 16;
    } synt_sli0;

    /* 0x000003c0 reg240 */
    struct {
        RK_U32 idr_pid    : 16;
        RK_U32 poc_lsb    : 16;
    } synt_sli1;

    /* 0x000003c4 reg241 */
    struct {
        RK_U32 rodr_pic_idx      : 2;
        RK_U32 ref_list0_rodr    : 1;
        RK_U32 sli_beta_ofst     : 4;
        RK_U32 sli_alph_ofst     : 4;
        RK_U32 dis_dblk_idc      : 2;
        RK_U32 reserved          : 3;
        RK_U32 rodr_pic_num      : 16;
    } synt_sli2;

    /* 0x000003c8 reg242 */
    struct {
        RK_U32 nopp_flg      : 1;
        RK_U32 ltrf_flg      : 1;
        RK_U32 arpm_flg      : 1;
        RK_U32 mmco4_pre     : 1;
        RK_U32 mmco_type0    : 3;
        RK_U32 mmco_parm0    : 16;
        RK_U32 mmco_type1    : 3;
        RK_U32 mmco_type2    : 3;
        RK_U32 reserved      : 3;
    } synt_refm0;

    /* 0x000003cc reg243 */
    struct {
        RK_U32 mmco_parm1    : 16;
        RK_U32 mmco_parm2    : 16;
    } synt_refm1;

    /* 0x000003d0 reg244 */
    struct {
        RK_U32 long_term_frame_idx0    : 4;
        RK_U32 long_term_frame_idx1    : 4;
        RK_U32 long_term_frame_idx2    : 4;
        RK_U32 reserved                : 20;
    } synt_refm2;

    /* 0x3d4 - 0x3ec */
    RK_U32 reserved248_251[7];

    /* 0x000003f0 reg252 */
    struct {
        RK_U32 mv_v_lmt_thd    : 14;
        RK_U32 reserved        : 1;
        RK_U32 mv_v_lmt_en     : 1;
        RK_U32 reserved1       : 16;
    } sli_cfg;

    /* 0x3f4 - 0x3fc */
    RK_U32 reserved253_255[3];

    /* 0x00000400 reg256 - 0x00000480 reg288 */
    Vepu540cJpegReg jpegReg;

} Vepu540cBaseCfg;

/* class: rc/roi/aq/klut */
/* 0x00001000 reg1024 - 0x000010e0 reg1080 */
typedef struct Vepu540cRcROiCfg_t {
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

    /* 0x1040 */
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

    /* 0x1070 - 0x1078 */
    RK_U32 reserved1052_1054[3];

    /* 0x0000107c reg1055 */
    struct {
        RK_U32 chrm_klut_ofst                : 4;
        RK_U32 reserved                      : 4;
        RK_U32 inter_chrm_dist_multi_hevc    : 6;
        RK_U32 reserved1                     : 18;
    } klut_ofst;
    /*0x00001080 reg1056 - 0x0000110c reg1091 */
    Vepu540cRoiCfg roi_cfg;
} Vepu540cRcRoiCfg;

/* class: iprd/iprd_wgt/rdo_wgta/prei_dif/sobel */
/* 0x00001700 reg1472 - 0x00001cd4 reg1845 */
typedef struct Vepu540cSection3_t {
    /* 0x1700 */
    /* 0x00001700 reg1472 */
    struct {
        RK_U32 iprd_tthdy4_0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 iprd_tthdy4_1    : 12;
        RK_U32 reserved1        : 4;
    } iprd_tthdy4_0;

    /* 0x00001704 reg1473 */
    struct {
        RK_U32 iprd_tthdy4_2    : 12;
        RK_U32 reserved         : 4;
        RK_U32 iprd_tthdy4_3    : 12;
        RK_U32 reserved1        : 4;
    } iprd_tthdy4_1;

    /* 0x00001708 reg1474 */
    struct {
        RK_U32 iprd_tthdc8_0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 iprd_tthdc8_1    : 12;
        RK_U32 reserved1        : 4;
    } iprd_tthdc8_0;

    /* 0x0000170c reg1475 */
    struct {
        RK_U32 iprd_tthdc8_2    : 12;
        RK_U32 reserved         : 4;
        RK_U32 iprd_tthdc8_3    : 12;
        RK_U32 reserved1        : 4;
    } iprd_tthdc8_1;

    /* 0x00001710 reg1476 */
    struct {
        RK_U32 iprd_tthdy8_0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 iprd_tthdy8_1    : 12;
        RK_U32 reserved1        : 4;
    } iprd_tthdy8_0;

    /* 0x00001714 reg1477 */
    struct {
        RK_U32 iprd_tthdy8_2    : 12;
        RK_U32 reserved         : 4;
        RK_U32 iprd_tthdy8_3    : 12;
        RK_U32 reserved1        : 4;
    } iprd_tthdy8_1;

    /* 0x00001718 reg1478 */
    struct {
        RK_U32 iprd_tthd_ul    : 12;
        RK_U32 reserved        : 20;
    } iprd_tthd_ul;

    /* 0x0000171c reg1479 */
    struct {
        RK_U32 iprd_wgty8_0    : 8;
        RK_U32 iprd_wgty8_1    : 8;
        RK_U32 iprd_wgty8_2    : 8;
        RK_U32 iprd_wgty8_3    : 8;
    } iprd_wgty8;

    /* 0x00001720 reg1480 */
    struct {
        RK_U32 iprd_wgty4_0    : 8;
        RK_U32 iprd_wgty4_1    : 8;
        RK_U32 iprd_wgty4_2    : 8;
        RK_U32 iprd_wgty4_3    : 8;
    } iprd_wgty4;

    /* 0x00001724 reg1481 */
    struct {
        RK_U32 iprd_wgty16_0    : 8;
        RK_U32 iprd_wgty16_1    : 8;
        RK_U32 iprd_wgty16_2    : 8;
        RK_U32 iprd_wgty16_3    : 8;
    } iprd_wgty16;

    /* 0x00001728 reg1482 */
    struct {
        RK_U32 iprd_wgtc8_0    : 8;
        RK_U32 iprd_wgtc8_1    : 8;
        RK_U32 iprd_wgtc8_2    : 8;
        RK_U32 iprd_wgtc8_3    : 8;
    } iprd_wgtc8;

    /* 0x172c */
    RK_U32 reserved_1483;

    /* 0x00001730 reg1484 */
    struct {
        RK_U32    quant_f_bias_I : 10;
        RK_U32    quant_f_bias_P : 10;
        RK_U32    reserve : 12;
    } RDO_QUANT;

    /* 0x1734 - 0x173c */
    RK_U32 reserved1485_1487[3];

    /* 0x00001740 reg1488 */
    // atr
    struct {
        RK_U32    atr_thd0 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    atr_thd1 : 12;
        RK_U32    reserve1 : 4;
    } ATR_THD0; //       only 264

    /* 0x1744 */
    struct {
        RK_U32    atr_thd2 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    atr_thdqp : 6;
        RK_U32    reserve1 : 10;
    } ATR_THD1; //       only 264

    /* 0x1748 */
    struct {
        RK_U32    atr1_thd0 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    atr1_thd1 : 12;
        RK_U32    reserve1 : 4;
    } ATR_THD10; //       only 264

    /* 0x174c */
    struct {
        RK_U32    atr1_thd2 : 12;
        RK_U32    reserve1 : 20;
    } ATR_THD11; //       only 264

    // /* 0x1748 - 0x174c */
    // RK_U32 reserved1490_1491[2];

    /* 0x00001750 reg1492 */
    struct {
        RK_U32    lvl16_atr_wgt0 : 8;
        RK_U32    lvl16_atr_wgt1 : 8;
        RK_U32    lvl16_atr_wgt2 : 8;
        RK_U32    reserved       : 8;
    } Lvl16_ATR_WGT; //      only 264

    /* 0x1754 */
    struct {
        RK_U32    lvl8_atr_wgt0 : 8;
        RK_U32    lvl8_atr_wgt1 : 8;
        RK_U32    lvl8_atr_wgt2 : 8;
        RK_U32    reserved      : 8;
    } Lvl8_ATR_WGT; //      only 264

    /* 0x1758 */
    struct {
        RK_U32    lvl4_atr_wgt0 : 8;
        RK_U32    lvl4_atr_wgt1 : 8;
        RK_U32    lvl4_atr_wgt2 : 8;
        RK_U32    reserved      : 8;
    } Lvl4_ATR_WGT; //      only 264

    /* 0x175c */
    RK_U32 reserved_1495;

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
    } cime_sqi_cfg;

    /* 0x00001764 reg1497 */
    struct {
        RK_U32 cime_mvd_th0    : 9;
        RK_U32 reserved        : 1;
        RK_U32 cime_mvd_th1    : 9;
        RK_U32 reserved1       : 1;
        RK_U32 cime_mvd_th2    : 9;
        RK_U32 reserved2       : 3;
    }  cime_mvd_th;

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
    }  cmv_st_th;

    /* 0x1780 - 0x17fc */
    RK_U32 reserved1504_1535[32];

    /* 0x00001800 - 0x18fc */
    RK_U32 reserved1537_1599[64];

    /* wgt_qp48_grpa */
    /* 0x00001900 reg1600 - 0x19cc */
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} Vepu540cSection3;

/* class: mmu */
/* 0x0000f000 reg15360 - 0x0000f064 reg15385 */

typedef struct HalVepu540cReg_t {
    Vepu540cControlCfg   reg_ctl;
    Vepu540cBaseCfg      reg_base;
    Vepu540cRcRoiCfg     reg_rc_roi;
    Vepu540cSection3     reg_s3;
    vepu540c_rdo_cfg     reg_rdo;
    vepu540c_scl_cfg     reg_scl;
    vepu540c_jpeg_tab    jpeg_table;
    vepu540c_status      reg_st;
} HalVepu540cRegSet;

#endif
