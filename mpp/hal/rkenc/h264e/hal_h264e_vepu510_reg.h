/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H264E_VEPU510_REG_H__
#define __HAL_H264E_VEPU510_REG_H__

#include "rk_type.h"
#include "vepu510_common.h"

/* class: control/link */
/* 0x00000000 reg0 - 0x00000120 reg72 */
typedef struct Vepu510ControlCfg_t {
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
    } version;

    /* 0x00000004 - 0x0000000c */
    RK_U32 reserved1_3[3];

    /* 0x00000010 reg4 */
    struct {
        RK_U32 lkt_num     : 8;
        RK_U32 vepu_cmd    : 3;
        RK_U32 reserved    : 21;
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

    /* 0x0000001c */
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
        RK_U32 vsrc_err_en          : 1;
        RK_U32 wdg_en               : 1;
        RK_U32 lkt_err_int_en       : 1;
        RK_U32 lkt_err_stop_en      : 1;
        RK_U32 lkt_force_stop_en    : 1;
        RK_U32 jslc_done_en         : 1;
        RK_U32 jbsf_oflw_en         : 1;
        RK_U32 jbuf_lens_en         : 1;
        RK_U32 dvbm_err_en          : 1;
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
        RK_U32 vsrc_err_msk          : 1;
        RK_U32 wdg_msk               : 1;
        RK_U32 lkt_err_int_msk       : 1;
        RK_U32 lkt_err_stop_msk      : 1;
        RK_U32 lkt_force_stop_msk    : 1;
        RK_U32 jslc_done_msk         : 1;
        RK_U32 jbsf_oflw_msk         : 1;
        RK_U32 jbuf_lens_msk         : 1;
        RK_U32 dvbm_err_msk          : 1;
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
        RK_U32 vsrc_err_clr          : 1;
        RK_U32 wdg_clr               : 1;
        RK_U32 lkt_err_int_clr       : 1;
        RK_U32 lkt_err_stop_clr      : 1;
        RK_U32 lkt_force_stop_clr    : 1;
        RK_U32 jslc_done_clr         : 1;
        RK_U32 jbsf_oflw_clr         : 1;
        RK_U32 jbuf_lens_clr         : 1;
        RK_U32 dvbm_err_clr          : 1;
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
        RK_U32 vsrc_err_sta          : 1;
        RK_U32 wdg_sta               : 1;
        RK_U32 lkt_err_int_sta       : 1;
        RK_U32 lkt_err_stop_sta      : 1;
        RK_U32 lkt_force_stop_sta    : 1;
        RK_U32 jslc_done_sta         : 1;
        RK_U32 jbsf_oflw_sta         : 1;
        RK_U32 jbuf_lens_sta         : 1;
        RK_U32 dvbm_err_sta          : 1;
        RK_U32 reserved              : 16;
    } int_sta;

    /* 0x00000030 reg12 */
    struct {
        RK_U32 jpeg_bus_edin        : 4;
        RK_U32 src_bus_edin         : 4;
        RK_U32 meiw_bus_edin        : 4;
        RK_U32 bsw_bus_edin         : 4;
        RK_U32 reserved             : 8;
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
        RK_U32 vs_load_thd    : 24;
        RK_U32 reserved       : 8;
    } enc_wdg;

    /* 0x0000003c - 0x0000004c */
    RK_U32 reserved15_19[5];

    /* 0x00000050 reg20 */
    struct {
        RK_U32 idle_en_core    : 1;
        RK_U32 idle_en_axi     : 1;
        RK_U32 idle_en_ahb     : 1;
        RK_U32 reserved        : 29;
    } enc_idle_en;

    /* 0x00000054 reg21 */
    struct {
        RK_U32 cke              : 1;
        RK_U32 resetn_hw_en     : 1;
        RK_U32 rfpr_err_e       : 1;
        RK_U32 sram_ckg_en      : 1;
        RK_U32 link_err_stop    : 1;
        RK_U32 reserved         : 27;
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
    } core_id;

    /* 0x60 - 0x6c */
    RK_U32 reserved24_27[4];

    /* 0x00000070 reg28 */
    struct {
        RK_U32 reserved    : 4;
        RK_U32 lkt_addr    : 28;
    } lkt_base_addr;

    /* 0x00000074 reg29 */
    struct {
        RK_U32 dvbm_en           : 1;
        RK_U32 src_badr_sel      : 1;
        RK_U32 vinf_frm_match    : 1;
        RK_U32 src_oflw_drop     : 1;
        RK_U32 dvbm_isp_cnct     : 1;
        RK_U32 dvbm_vepu_cnct    : 1;
        RK_U32 vepu_expt_type    : 2;
        RK_U32 vinf_dly_cycle    : 8;
        RK_U32 ybuf_full_mgn     : 8;
        RK_U32 ybuf_oflw_mgn     : 8;
    } dvbm_cfg;

    /* 0x00000078 reg30 */
    struct {
        RK_U32 reserved         : 4;
        RK_U32 src_y_adr_str    : 28;
    } dvbm_y_sadr;

    /* 0x0000007c reg31 */
    struct {
        RK_U32 reserved         : 4;
        RK_U32 src_c_adr_str    : 28;
    } dvbm_c_sadr;

    /* 0x00000080 reg32 */
    struct {
        RK_U32 dvbm_y_line_strd    : 21;
        RK_U32 reserved            : 11;
    } dvbm_y_lstd;

    /* 0x84 */
    RK_U32 reserved_33;

    /* 0x00000088 reg34 */
    struct {
        RK_U32 reserved           : 4;
        RK_U32 dvbm_y_frm_strd    : 28;
    } dvbm_y_fstd;

    /* 0x0000008c reg35 */
    struct {
        RK_U32 reserved           : 4;
        RK_U32 dvbm_c_frm_strd    : 28;
    } dvbm_c_fstd;

    /* 0x00000090 reg36 */
    struct {
        RK_U32 reserved      : 4;
        RK_U32 dvbm_y_top    : 28;
    } dvbm_y_top;

    /* 0x00000094 reg37 */
    struct {
        RK_U32 reserved      : 4;
        RK_U32 dvbm_c_top    : 28;
    } dvbm_c_top;

    /* 0x00000098 reg38 */
    struct {
        RK_U32 reserved       : 4;
        RK_U32 dvbm_y_botm    : 28;
    } dvbm_y_botm;

    /* 0x0000009c reg39 */
    struct {
        RK_U32 reserved       : 4;
        RK_U32 dvbm_c_botm    : 28;
    } dvbm_c_botm;

    /* 0xa0 - 0xfc */
    RK_U32 reserved40_63[24];

    /* 0x00000100 reg64 */
    struct {
        RK_U32 node_core_id    : 2;
        RK_U32 node_int        : 1;
        RK_U32 reserved        : 1;
        RK_U32 task_id         : 12;
        RK_U32 reserved1       : 16;
    } lkt_node_cfg;

    /* 0x00000104 reg65 */
    struct {
        RK_U32 pcfg_rd_en       : 1;
        RK_U32 reserved         : 3;
        RK_U32 lkt_addr_pcfg    : 28;
    } lkt_addr_pcfg;

    /* 0x00000108 reg66 */
    struct {
        RK_U32 rc_cfg_rd_en       : 1;
        RK_U32 reserved           : 3;
        RK_U32 lkt_addr_rc_cfg    : 28;
    } lkt_addr_rc_cfg;

    /* 0x0000010c reg67 */
    struct {
        RK_U32 par_cfg_rd_en       : 1;
        RK_U32 reserved            : 3;
        RK_U32 lkt_addr_par_cfg    : 28;
    } lkt_addr_par_cfg;

    /* 0x00000110 reg68 */
    struct {
        RK_U32 sqi_cfg_rd_en       : 1;
        RK_U32 reserved            : 3;
        RK_U32 lkt_addr_sqi_cfg    : 28;
    } lkt_addr_sqi_cfg;

    /* 0x00000114 reg69 */
    struct {
        RK_U32 scal_cfg_rd_en       : 1;
        RK_U32 reserved             : 3;
        RK_U32 lkt_addr_scal_cfg    : 28;
    } lkt_addr_scal_cfg;

    /* 0x00000118 reg70 */
    struct {
        RK_U32 pp_cfg_rd_en       : 1;
        RK_U32 reserved           : 3;
        RK_U32 lkt_addr_pp_cfg    : 28;
    } lkt_addr_osd_cfg;

    /* 0x0000011c reg71 */
    struct {
        RK_U32 st_out_en      : 1;
        RK_U32 reserved       : 3;
        RK_U32 lkt_addr_st    : 28;
    } lkt_addr_st;

    /* 0x00000120 reg72 */
    struct {
        RK_U32 nxt_node_vld    : 1;
        RK_U32 reserved        : 3;
        RK_U32 lkt_addr_nxt    : 28;
    } lkt_addr_nxt;
} Vepu510ControlCfg;

/* class: buffer/video syntax */
/* 0x00000270 reg156 - 0x000003f4 reg253*/
typedef struct Vepu510FrameCfg_t {
    /* 0x00000270 reg156 - 0x0000027c reg159 */
    vepu510_online online_addr;

    /* 0x00000280 reg160 4*/
    RK_U32 adr_src0;

    /* 0x00000284 reg161 5*/
    RK_U32 adr_src1;

    /* 0x00000288 reg162 6*/
    RK_U32 adr_src2;

    /* 0x0000028c reg163 7*/
    RK_U32 rfpw_h_addr;

    /* 0x00000290 reg164 8*/
    RK_U32 rfpw_b_addr;

    /* 0x00000294 reg165 9*/
    RK_U32 rfpr_h_addr;

    /* 0x00000298 reg166 10*/
    RK_U32 rfpr_b_addr;

    /* 0x0000029c reg167 11*/
    RK_U32 colmvw_addr;

    /* 0x000002a0 reg168 12*/
    RK_U32 colmvr_addr;

    /* 0x000002a4 reg169 13*/
    RK_U32 dspw_addr;

    /* 0x000002a8 reg170 14*/
    RK_U32 dspr_addr;

    /* 0x000002ac reg171 15*/
    RK_U32 meiw_addr;

    /* 0x000002b0 reg172 16*/
    RK_U32 bsbt_addr;

    /* 0x000002b4 reg173 17*/
    RK_U32 bsbb_addr;

    /* 0x000002b8 reg174 18*/
    RK_U32 adr_bsbs;

    /* 0x000002bc reg175 19*/
    RK_U32 bsbr_addr;

    /* 0x000002c0 reg176 20*/
    RK_U32 lpfw_addr;

    /* 0x000002c4 reg177 21*/
    RK_U32 lpfr_addr;

    /* 0x000002c8 reg178 22*/
    RK_U32 ebuft_addr;

    /* 0x000002cc reg179 23*/
    RK_U32 ebufb_addr;

    /* 0x000002d0 reg180 */
    RK_U32 rfpt_h_addr;

    /* 0x000002d4 reg181 */
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
    RK_U32 adr_roir;

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
        RK_U32 num_pic_tot_cur_hevc    : 5;
        RK_U32 log2_ctu_num_hevc       : 5;
        RK_U32 reserved1               : 6;
        RK_U32 slen_fifo               : 1;
        RK_U32 rec_fbc_dis             : 1;
    } enc_pic;

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
    } dual_core;

    /* 0x00000308 reg194 */
    struct {
        RK_U32 frame_id        : 8;
        RK_U32 frm_id_match    : 1;
        RK_U32 reserved        : 7;
        RK_U32 ch_id           : 2;
        RK_U32 vrsp_rtn_en     : 1;
        RK_U32 vinf_req_en     : 1;
        RK_U32 reserved1       : 12;
    } enc_id;

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
        RK_U32 cr_force_value     : 8;
        RK_U32 cb_force_value     : 8;
        RK_U32 chroma_force_en    : 1;
        RK_U32 reserved           : 9;
        RK_U32 src_mirr           : 1;
        RK_U32 src_rot            : 2;
        RK_U32 tile4x4_en         : 1;
        RK_U32 reserved1          : 2;
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
        RK_U32 src_strd0    : 21;
        RK_U32 reserved     : 11;
    } src_strd0;

    /* 0x00000338 reg206 */
    struct {
        RK_U32 src_strd1    : 16;
        RK_U32 reserved     : 16;
    } src_strd1;

    /* 0x33c - 0x34c */
    RK_U32 reserved207_211[5];

    /* 0x00000350 reg212 */
    struct {
        RK_U32 rc_en         : 1;
        RK_U32 aq_en         : 1;
        RK_U32 reserved      : 10;
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
    } me_rnge;

    /* 0x00000374 reg221 */
    struct {
        RK_U32 srgn_max_num      : 7;
        RK_U32 cime_dist_thre    : 13;
        RK_U32 rme_srch_h        : 2;
        RK_U32 rme_srch_v        : 2;
        RK_U32 rme_dis           : 3;
        RK_U32 reserved          : 1;
        RK_U32 fme_dis           : 3;
        RK_U32 reserved1         : 1;
    } me_cfg;

    /* 0x00000378 reg222 */
    struct {
        RK_U32 cime_zero_thre     : 13;
        RK_U32 reserved           : 15;
        RK_U32 fme_prefsu_en      : 2;
        RK_U32 colmv_stor_hevc    : 1;
        RK_U32 colmv_load_hevc    : 1;
    } me_cach;

    /* 0x37c - 0x39c */
    RK_U32 reserved223_231[9];

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 rect_size         : 1;
        RK_U32 reserved          : 2;
        RK_U32 vlc_lmt           : 1;
        RK_U32 chrm_spcl         : 1;
        RK_U32 reserved1         : 8;
        RK_U32 ccwa_e            : 1;
        RK_U32 reserved2         : 1;
        RK_U32 atr_e             : 1;
        RK_U32 reserved3         : 4;
        RK_U32 scl_lst_sel       : 2;
        RK_U32 reserved4         : 6;
        RK_U32 atf_e             : 1;
        RK_U32 atr_mult_sel_e    : 1;
        RK_U32 reserved5         : 2;
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
        RK_U32 poc_type    : 2;
        RK_U32 reserved    : 21;
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

    /* 0x000003d4 reg245 */
    struct {
        RK_U32 dlt_poc_s0_m12    : 16;
        RK_U32 dlt_poc_s0_m13    : 16;
    } synt_refm3_hevc;

    /* 0x000003d8 reg246 */
    struct {
        RK_U32 poc_lsb_lt1    : 16;
        RK_U32 poc_lsb_lt2    : 16;
    } synt_long_refm0_hevc;

    /* 0x000003dc reg247 */
    struct {
        RK_U32 dlt_poc_msb_cycl1    : 16;
        RK_U32 dlt_poc_msb_cycl2    : 16;
    } synt_long_refm1_hevc;

    /* 0x000003e0 reg248 */
    struct {
        RK_U32 sao_lambda_multi    : 3;
        RK_U32 reserved            : 29;
    } sao_cfg_hevc;

    /* 0x3e4 - 0x3ec */
    RK_U32 reserved249_251[3];

    /* 0x000003f0 reg252 */
    struct {
        RK_U32 mv_v_lmt_thd    : 14;
        RK_U32 reserved        : 1;
        RK_U32 mv_v_lmt_en     : 1;
        RK_U32 reserved1       : 16;
    } sli_cfg;

    /* 0x000003f4 reg253 */
    struct {
        RK_U32 tile_x       : 9;
        RK_U32 reserved     : 7;
        RK_U32 tile_y       : 9;
        RK_U32 reserved1    : 7;
    } tile_pos;
} Vepu510FrameCfg;

/* class: rc/roi/aq/klut */
/* 0x00001000 reg1024 - 0x0000110c reg1091 */
typedef struct Vepu510RcRoiCfg_t {
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
    RK_U32 reserved1052_1054[3];

    /* 0x0000107c reg1055 */
    struct {
        RK_U32 chrm_klut_ofst                : 4;
        RK_U32 reserved                      : 4;
        RK_U32 inter_chrm_dist_multi_hevc    : 6;
        RK_U32 reserved1                     : 18;
    } klut_ofst;

    /* 0x00001080 reg1056 - - 0x0000110c reg1091 */
    Vepu510RoiCfg roi_cfg;
} Vepu510RcRoiCfg;

/* class: iprd/iprd_wgt/rdo_wgta/prei_dif/sobel */
/* 0x00001700 reg1472 -0x000019cc reg1651 */
typedef struct Vepu510Param_t {
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
    struct {
        RK_U32    atr_thd0 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    atr_thd1 : 12;
        RK_U32    reserve1 : 4;
    } ATR_THD0;
    /* 0x00001744 reg1489 */
    struct {
        RK_U32    atr_thd2 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    atr_thdqp : 6;
        RK_U32    reserve1 : 10;
    } ATR_THD1;

    /* 0x1748 - 0x174c */
    RK_U32 reserved1490_1491[2];

    /* 0x00001750 reg1492 */
    struct {
        RK_U32    lvl16_atr_wgt0 : 8;
        RK_U32    lvl16_atr_wgt1 : 8;
        RK_U32    lvl16_atr_wgt2 : 8;
        RK_U32    reserved       : 8;
    } Lvl16_ATR_WGT;

    /* 0x00001754  reg1493*/
    struct {
        RK_U32    lvl8_atr_wgt0 : 8;
        RK_U32    lvl8_atr_wgt1 : 8;
        RK_U32    lvl8_atr_wgt2 : 8;
        RK_U32    reserved      : 8;
    } Lvl8_ATR_WGT;

    /* 0x00001758 reg1494 */
    struct {
        RK_U32    lvl4_atr_wgt0 : 8;
        RK_U32    lvl4_atr_wgt1 : 8;
        RK_U32    lvl4_atr_wgt2 : 8;
        RK_U32    reserved      : 8;
    } Lvl4_ATR_WGT;

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
    } me_sqi_comb;

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
    } cmv_st_th;

    /* 0x1780 - 0x17fc */
    RK_U32 reserved1504_1535[32];

    /* 0x00001800 reg1536 - 0x000018cc reg1587*/
    RK_U32 pprd_lamb_satd_hevc_0_51[52];

    /* 0x000018d0 reg1588 */
    RK_U32 iprd_lamb_satd_ofst_hevc;

    /* 0x18d4 - 0x18fc */
    RK_U32 reserved1589_1599[11];

    // /* 0x00001900 reg1600 - 0x000019cc reg1651*/
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} Vepu510Param;

/* class: scaling list  */
/* 0x00002200 reg2176- 0x00002584 reg2401*/
typedef struct Vepu510SclCfg_t {
    RK_U32 q_scal_list_0_225[226];
} Vepu510SclCfg;

/* class: mmu */
/* 0x0000f000 reg15360 - 0x0000f024 reg15369 */
typedef struct HalVepu510Reg_t {
    Vepu510ControlCfg       reg_ctl;
    Vepu510FrameCfg         reg_frm;
    Vepu510RcRoiCfg         reg_rc_roi;
    Vepu510Param            reg_param;
    Vepu510Sqi              reg_sqi;
    Vepu510SclCfg           reg_scl;
    Vepu510Status           reg_st;
    Vepu510Dbg              reg_dbg;
} HalVepu510RegSet;

#endif