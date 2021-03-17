/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H265E_VEPU541_REG_H__
#define __HAL_H265E_VEPU541_REG_H__

#include "mpp_buffer.h"
#include "mpp_enc_hal.h"
#include "hal_task.h"
#include "rkv_enc_def.h"
#include "h265_syntax.h"

/* OSD position */
typedef struct {
    RK_U32    lt_pos_x : 8; /* left-top */
    RK_U32    lt_pos_y : 8;
    RK_U32    rd_pos_x : 8; /* right-bottom */
    RK_U32    rd_pos_y : 8;
} OsdPos;

/* OSD palette */
typedef struct {
    RK_U32     y : 8;
    RK_U32     u : 8;
    RK_U32     v : 8;
    RK_U32     alpha : 8;
} OsdPlt;
typedef struct {
    RK_U32    axi_brsp_cke : 7;
    RK_U32    cime_dspw_orsd : 1;
    RK_U32    reserve : 24;
} v541_dtrns_cfg;

typedef struct {
    RK_U32    Reserve0 : 7;
    RK_U32    cime_dspw_orsd : 1;
    RK_U32    reserve1 : 8;
    RK_U32    axi_brsp_cke : 8;
    RK_U32    reserve2 : 8;
} v540_dtrns_cfg;

typedef struct H265eV541RegSet_t {

    /* reg[000] */
    /* 0x0 - VERSION, swreg01 */
    struct {
        RK_U32    rkvenc_ver : 32;        //default : 0x0000_0001
    } version;

    /* 0x4 - swreg02, ENC_STRT */
    struct {
        RK_U32    lkt_num : 8;
        RK_U32    rkvenc_cmd : 2;
        RK_U32    reserve : 6;
        RK_U32    enc_cke : 1;
        RK_U32    resetn_hw_en : 1;
        RK_U32    enc_done_tmvp_en : 1;
        RK_U32    Reserve : 13;
    } enc_strt;

    /* 0x8 - ENC_CLR */
    struct {
        RK_U32    safe_clr  : 1;
        RK_U32    force_clr : 1;
        RK_U32    reserve   : 30;
    } enc_clr;//swreg03

    /* 0xc - swreg04, LKT_ADDR */
    struct {
        RK_U32    lkt_addr : 32;
    } lkt_addr;

    /* 0x10 - swreg05, INT_EN */
    struct {
        RK_U32    enc_done_en  : 1;
        RK_U32    lkt_done_en  : 1;
        RK_U32    sclr_done_en : 1;
        RK_U32    slc_done_en  : 1;
        RK_U32    bsf_ovflw_en : 1;
        RK_U32    brsp_ostd_en : 1;
        RK_U32    wbus_err_en  : 1;
        RK_U32    rbus_err_en  : 1;
        RK_U32    wdg_en       : 1;
        RK_U32    reserve : 23;
    } int_en;

    struct {
        RK_U32    enc_done_msk  : 1;
        RK_U32    lkt_done_msk  : 1;
        RK_U32    sclr_done_msk : 1;
        RK_U32    slc_done_msk  : 1;
        RK_U32    bsf_folw_msk  : 1;
        RK_U32    brsp_otsd_msk : 1;
        RK_U32    wbus_err_msk  : 1;
        RK_U32    rbus_err_msk  : 1;
        RK_U32    wdg_msk       : 1;
        RK_U32    reserved      : 23;;
    } int_msk;  //swreg06, INT_MSK

    struct {
        RK_U32    enc_done_clr  : 1;
        RK_U32    lkt_done_clr  : 1;
        RK_U32    sclr_done_clr : 1;
        RK_U32    slc_done_clr  : 1;
        RK_U32    bsf_folw_clr  : 1;
        RK_U32    brsp_otsd_clr : 1;
        RK_U32    wbus_err_clr : 1;
        RK_U32    rbus_err_clr : 1;
        RK_U32    wdg_msk  : 1;
        RK_U32    reserved     : 23;
    } int_clr;  //swreg07, INT_CLR

    /* 0x1C - swreg08, INT_STA */
    struct {
        RK_U32    reserve : 32;
    } int_stus;

    RK_U32 reserved_0x20_0x2c[4];

    /* 0x30 - swreg09, ENC_RSL */
    struct {
        RK_U32    pic_wd8_m1 : 9;
        RK_U32    reserve0 : 1;
        RK_U32    pic_wfill : 6;
        RK_U32    pic_hd8_m1 : 9;
        RK_U32    reserve1 : 1;
        RK_U32    pic_hfill : 6;
    } enc_rsl;

    /* 0x34 - ENC_PIC */
    struct {
        RK_U32    enc_stnd     : 1;
        RK_U32    roi_en       : 1;
        RK_U32    cur_frm_ref  : 1;
        RK_U32    mei_stor     : 1;
        RK_U32    bs_scp       : 1;
        RK_U32    rdo_wgt_sel  : 1;
        RK_U32    reserved     : 2;
        RK_U32    pic_qp       : 6;
        RK_U32    tot_poc_num  : 5;
        RK_U32    log2_ctu_num : 4;
        RK_U32    atr_thd_sel  : 1;
        RK_U32    dchs_rxid    : 2;
        RK_U32    dchs_txid    : 2;
        RK_U32    dchs_rxe     : 1;
        RK_U32    satd_byps_en : 1;
        RK_U32    slen_fifo    : 1;
        RK_U32    node_int     : 1;
    } enc_pic; //swreg10

    struct {
        RK_U32    vs_load_thd : 24;
        RK_U32    rfp_load_thd : 8;
    } enc_wdg; //swreg11, ENC_WDG

    /* 0x3c - DTRNS_MAP */
    struct {
        RK_U32    lpfw_bus_ordr : 1; /* vepu540 used */
        RK_U32    cmvw_bus_ordr : 1;
        RK_U32    dspw_bus_ordr : 1;
        RK_U32    rfpw_bus_ordr : 1;
        RK_U32    src_bus_edin  : 4;
        RK_U32    meiw_bus_edin : 4;
        RK_U32    bsw_bus_edin  : 3;
        RK_U32    lktr_bus_edin : 4;
        RK_U32    roir_bus_edin : 4;
        RK_U32    lktw_bus_edin : 4;
        RK_U32    afbc_bsize : 1;
        RK_U32    reserved      : 4;
    } dtrns_map; //swreg12

    union {
        v541_dtrns_cfg dtrns_cfg_541;
        v540_dtrns_cfg dtrns_cfg_540;
    };

    /* 0x44 - SRC_FMT */
    struct {
        RK_U32    alpha_swap : 1;
        RK_U32    rbuv_swap : 1;
        RK_U32    src_cfmt : 4;
        RK_U32    src_range : 1;
        RK_U32    out_fmt_cfg : 1;  //vepu540
        RK_U32    reserve : 24;
    } src_fmt;

    /* 0x48 - SRC_UDFY */
    struct {
        RK_S32    wght_b2y : 9;
        RK_S32    wght_g2y : 9;
        RK_S32    wght_r2y : 9;
        RK_S32    reserved : 5;
    } src_udfy;

    /* 0x4c - SRC_UDFU */
    struct {
        RK_S32    wght_b2u : 9;
        RK_S32    wght_g2u : 9;
        RK_S32    wght_r2u : 9;
        RK_S32    reserved : 5;
    } src_udfu;

    /* 0x50 - SRC_UDFV */
    struct {
        RK_S32    wght_b2v : 9;
        RK_S32    wght_g2v : 9;
        RK_S32    wght_r2v : 9;
        RK_S32    reserved : 5;
    } src_udfv;

    /* 0x54 - SRC_UDFO */
    struct {
        RK_U32    ofst_v : 8;
        RK_U32    ofst_u : 8;
        RK_U32    ofst_y : 5;
        RK_U32    reserve : 11;
    } src_udfo;

    /* 0x58 - SRC_PROC */
    struct {
        RK_U32    reserved0 : 26;
        RK_U32    src_mirr  : 1;
        RK_U32    src_rot   : 2;
        RK_U32    txa_en    : 1;
        RK_U32    afbcd_en  : 1;
        RK_U32    reserved1 : 1;
    } src_proc;

    /* 0x5c - MMU0_DTE_ADDR */
    struct {
        RK_U32    tile_width_m1  : 6;
        RK_U32    reserved0      : 10;
        RK_U32    tile_height_m1 : 6;
        RK_U32    reserved1      : 9;
        RK_U32    tile_en        : 1;
    } tile_cfg;

    /* 0x60 - MMU1_DTE_ADDR */
    struct {
        RK_U32    tile_x    : 6;
        RK_U32    reserved0 : 10;
        RK_U32    tile_y    : 6;
        RK_U32    reserved1 : 10;
    } tile_pos;

    /* 0x64 - KLUT_OFST */
    struct {
        RK_U32 chrm_kult_ofst : 3;
        RK_U32 reserved       : 29;
    } klut_ofst;

    /* 0x68-0xc4 - KLUT_WGT */
    struct {
        RK_U32 chrm_klut_wgt0 : 18;
        RK_U32 reserved       : 5;
        RK_U32 chrm_klut_wgt1 : 9;
    } klut_wgt0;

    struct {
        RK_U32 chrm_klut_wgt1 : 9;
        RK_U32 reserved       : 5;
        RK_U32 chrm_klut_wgt2 : 18;
    } klut_wgt1;

    struct {
        RK_U32 chrm_klut_wgt3 : 18;
        RK_U32 reserved       : 5;
        RK_U32 chrm_klut_wgt4 : 9;
    } klut_wgt2;

    struct {
        RK_U32 chrm_klut_wgt4 : 9;
        RK_U32 reserved       : 5;
        RK_U32 chrm_klut_wgt5 : 18;
    } klut_wgt3;

    struct {
        RK_U32 chrm_klut_wgt6 : 18;
        RK_U32 reserved       : 5;
        RK_U32 chrm_klut_wgt7 : 9;
    } klut_wgt4;

    struct {
        RK_U32 chrm_klut_wgt7 : 9;
        RK_U32 reserved       : 5;
        RK_U32 chrm_klut_wgt8 : 18;
    } klut_wgt5;

    struct {
        RK_U32 chrm_klut_wgt9  : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt10 : 9;
    } klut_wgt6;

    struct {
        RK_U32 chrm_klut_wgt10 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt11 : 18;
    } klut_wgt7;

    struct {
        RK_U32 chrm_klut_wgt12 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt13 : 9;
    } klut_wgt8;

    struct {
        RK_U32 chrm_klut_wgt13 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt14 : 18;
    } klut_wgt9;

    struct {
        RK_U32 chrm_klut_wgt15 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt16 : 9;
    } klut_wgt10;

    struct {
        RK_U32 chrm_klut_wgt16 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt17 : 18;
    } klut_wgt11;

    struct {
        RK_U32 chrm_klut_wgt18 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt19 : 9;
    } klut_wgt12;

    struct {
        RK_U32 chrm_klut_wgt19 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt20 : 18;
    } klut_wgt13;

    struct {
        RK_U32 chrm_klut_wgt21 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt22 : 9;
    } klut_wgt14;

    struct {
        RK_U32 chrm_klut_wgt22 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt23 : 18;
    } klut_wgt15;

    struct {
        RK_U32 chrm_klut_wgt24 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt25 : 9;
    } klut_wgt16;

    struct {
        RK_U32 chrm_klut_wgt25 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt26 : 18;
    } klut_wgt17;

    struct {
        RK_U32 chrm_klut_wgt27 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt28 : 9;
    } klut_wgt18;

    struct {
        RK_U32 chrm_klut_wgt28 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt29 : 18;
    } klut_wgt19;

    struct {
        RK_U32 chrm_klut_wgt30 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt31 : 9;
    } klut_wgt20;

    struct {
        RK_U32 chrm_klut_wgt31 : 9;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt32 : 18;
    } klut_wgt21;

    struct {
        RK_U32 chrm_klut_wgt33 : 18;
        RK_U32 reserved        : 5;
        RK_U32 chrm_klut_wgt34 : 9;
    } klut_wgt22;

    /* 0xc4 - klut_wgt23 */
    struct {
        RK_U32 chrm_klut_wgt34 : 9;
        RK_U32 reserved        : 23;
    } klut_wgt23;

    /* 0xc8 - RC_CFG */
    struct {
        RK_U32    rc_en      : 1;
        RK_U32    aqmode_en  : 1;
        RK_U32    qp_mode    : 1;
        RK_U32    reserved   : 13;
        RK_U32    rc_ctu_num : 16;
    } rc_cfg;

    /* 0xcc - RC_QP */
    struct {
        RK_U32    reserved    : 16;
        RK_U32    rc_qp_range : 4;
        RK_U32    rc_max_qp   : 6;
        RK_U32    rc_min_qp   : 6;
    } rc_qp;

    /* 0xd0 - swreg55, RC_TGT */
    struct {
        RK_U32    ctu_ebits : 20;
        RK_U32    reserve : 12;
    } rc_tgt;

    /* 0xd4 - RC_ADJ0 */
    struct {
        RK_S32    qp_adjust0 : 5;
        RK_S32    qp_adjust1 : 5;
        RK_S32    qp_adjust2 : 5;
        RK_S32    qp_adjust3 : 5;
        RK_S32    qp_adjust4 : 5;
        RK_S32    reserved   : 7;
    } rc_adj0;

    /* 0xd8 - RCADJ1 */
    struct {
        RK_S32    qp_adjust5 : 5;
        RK_S32    qp_adjust6 : 5;
        RK_S32    qp_adjust7 : 5;
        RK_S32    qp_adjust8 : 5;
        RK_S32    reserved   : 12;
    } rc_adj1;

    /* 0xdc-0xfc swreg47, */
    struct {
        RK_S32    bits_thd0;
    } rc_erp0; //RC_ERP0

    struct {
        RK_S32    bits_thd1;
    } rc_erp1; //swreg48, RC_ERP1

    struct {
        RK_S32    bits_thd2;
    } rc_erp2; //swreg49, RC_ERP2

    struct {
        RK_S32    bits_thd3;
    } rc_erp3; //swreg50, RC_ERP3

    struct {
        RK_S32    bits_thd4;
    } rc_erp4; //swreg51, RC_ERP4

    /* 0xec-0xf8 */
    struct {
        RK_S32 bits_thd5;
    } rc_erp5;

    struct {
        RK_S32 bits_thd6;
    } rc_erp6;

    struct {
        RK_S32 bits_thd7;
    } rc_erp7;

    /* 0xfc - RC_ERP8 */
    struct {
        RK_S32 bits_thd8;
    } rc_erp8;

    /* 0x100 - QPMAP0 */
    struct {
        RK_U32 qpmin_area0 : 6;
        RK_U32 qpmax_area0 : 6;
        RK_U32 qpmin_area1 : 6;
        RK_U32 qpmax_area1 : 6;
        RK_U32 qpmin_area2 : 6;
        RK_U32 reserved    : 2;
    } qpmap0;

    /* 0x104 - QPMAP1 */
    struct {
        RK_U32 qpmax_area2 : 6;
        RK_U32 qpmin_area3 : 6;
        RK_U32 qpmax_area3 : 6;
        RK_U32 qpmin_area4 : 6;
        RK_U32 qpmax_area4 : 6;
        RK_U32 reserved    : 2;
    } qpmap1;

    /* 0x108 - QPMAP2 */
    struct {
        RK_U32 qpmin_area5 : 6;
        RK_U32 qpmax_area5 : 6;
        RK_U32 qpmin_area6 : 6;
        RK_U32 qpmax_area6 : 6;
        RK_U32 qpmin_area7 : 6;
        RK_U32 reserved    : 2;
    } qpmap2;

    /* 0x10c - QPMAP3 */
    struct {
        RK_U32 qpmax_area7 : 6;
        RK_U32 reserved    : 24;
        RK_U32 qpmap_mode  : 2;
    } qpmap3;

    /* 0x110 - PIC_OFST */
    struct {
        RK_U32 pic_ofst_y : 13;
        RK_U32 reserved0  : 3;
        RK_U32 pic_ofst_x : 13;
        RK_U32 reserved1  : 3;
    } pic_ofst;

    /* 0x114 - swreg23, SRC_STRID */
    struct {
        RK_U32 src_ystrid : 16;
        RK_U32 src_cstrid : 16;
    } src_strid;

    RK_U32 adr_srcy_hevc; /* 0x118 */
    RK_U32 adr_srcu_hevc;
    RK_U32 adr_srcv_hevc;
    RK_U32 roi_addr_hevc;
    RK_U32 rfpw_h_addr_hevc;
    RK_U32 rfpw_b_addr_hevc;
    RK_U32 rfpr_h_addr_hevc;
    RK_U32 rfpr_b_addr_hevc;
    RK_U32 cmvw_addr_hevc;
    RK_U32 cmvr_addr_hevc;
    RK_U32 dspw_addr_hevc;
    RK_U32 dspr_addr_hevc;
    RK_U32 meiw_addr_hevc;
    RK_U32 bsbt_addr_hevc;
    RK_U32 bsbb_addr_hevc;
    RK_U32 bsbr_addr_hevc;
    RK_U32 bsbw_addr_hevc; /* 0x158 */

    /* 0x15c - swreg41, SLI_SPL */
    struct {
        RK_U32    sli_splt : 1;
        RK_U32    sli_splt_mode : 1;
        RK_U32    sli_splt_cpst : 1;
        RK_U32    sli_max_num_m1 : 10;
        RK_U32    sli_flsh : 1;
        RK_U32    reserve : 2;
        RK_U32    sli_splt_cnum_m1 : 16;
    } sli_spl;

    /* 0x160 - swreg42, SLI_SPL_BYTE */
    struct {
        RK_U32    sli_splt_byte : 18;
        RK_U32    reserve : 14;
    } sli_spl_byte;

    /* 0x164 - swreg43, ME_RNGE */
    struct {
        RK_U32    cime_srch_h : 4;
        RK_U32    cime_srch_v : 4;
        RK_U32    rime_srch_h : 3;
        RK_U32    rime_srch_v : 3;
        RK_U32    reserved : 2;
        RK_U32    dlt_frm_num : 16;
    } me_rnge;

    /* 0x168 - swreg44, ME_CNST */
    struct {
        RK_U32 pmv_mdst_h  : 8;
        RK_U32 pmv_mdst_v  : 8;
        RK_U32 mv_limit    : 2;
        RK_U32 mv_num      : 2;
        RK_U32 colmv_store : 1;
        RK_U32 colmv_load  : 1;
        RK_U32 rime_dis_en : 5; /* used for rtl debug */
        RK_U32 fme_dis_en  : 5; /* used for rtl debug */
    } me_cnst;

    /* 0x16c - ME_RAM */
    struct {
        RK_U32    cime_rama_max : 11;
        RK_U32    cime_rama_h   : 5;
        RK_U32    cach_l2_tag   : 2;
        RK_U32    cime_linebuf_w: 8;  /*only used for 540*/
        RK_U32    reserved      : 6;
    } me_ram;

    /*0x170 - SYNT_REF_MARK4*/
    struct {
        RK_U32 poc_lsb_lt1 : 16;
        RK_U32 poc_lsb_lt2 : 16;
    } synt_ref_mark4;

    /*0x174 SYNT_REF_MARK5*/
    struct {
        RK_U32 dlt_poc_msb_cycl1 : 16;
        RK_U32 dlt_poc_msb_cycl2 : 16;
    } synt_ref_mark5;

    struct {
        RK_U32 osd_ch_inv_en : 8;
        RK_U32 osd_itype : 8;
        RK_U32 osd_lu_inv_msk : 8;
        RK_U32 osd_ch_inv_msk : 8;
    } osd_inv_cfg;
    RK_U32 lpfw_addr_hevc;
    RK_U32 lpfr_addr_hevc;
    RK_U32 reserved_0x184_0x190[4];

    /* 0x194 - REG_THD, reserved */
    RK_U32 reg_thd;

    /* 0x198 - swreg56, RDO_CFG */
    struct {
        RK_U32    ltm_col      : 1;
        RK_U32    ltm_idx0l0   : 1;
        RK_U32    chrm_special : 1;
        RK_U32    rdoq_en      : 1; /* may be used in the future */
        RK_U32    reserved0    : 2;
        RK_U32    cu_inter_en  : 4;
        RK_U32    reserved1    : 9;
        RK_U32    cu_intra_en  : 4;
        RK_U32    chrm_klut_en : 1;
        RK_U32    seq_scaling_matrix_present_flg : 2;
        RK_U32    reserved2    : 5;
        RK_U32    stad_byps_flg : 1;  /*only for 540*/
    } rdo_cfg;

    /* 0x19c - swreg57, SYNT_NAL */
    struct {
        RK_U32    nal_unit_type : 6;
        RK_U32    reserve : 26;
    } synt_nal;

    /* 0x1a0 - swreg58, SYNT_SPS */
    struct {
        RK_U32    smpl_adpt_ofst_en : 1;
        RK_U32    num_st_ref_pic : 7;
        RK_U32    lt_ref_pic_prsnt : 1;
        RK_U32    num_lt_ref_pic : 6;
        RK_U32    tmpl_mvp_en : 1;
        RK_U32    log2_max_poc_lsb : 4;
        RK_U32    strg_intra_smth : 1;
        RK_U32    reserved : 11;
    } synt_sps;

    /* 0x1a4 - swreg59, SYNT_PPS */
    struct {
        RK_U32    dpdnt_sli_seg_en : 1;
        RK_U32    out_flg_prsnt_flg : 1;
        RK_U32    num_extr_sli_hdr : 3;
        RK_U32    sgn_dat_hid_en : 1;
        RK_U32    cbc_init_prsnt_flg : 1;
        RK_U32    pic_init_qp : 6;
        RK_U32    cu_qp_dlt_en : 1;
        RK_U32    chrm_qp_ofst_prsn : 1;
        RK_U32    lp_fltr_acrs_sli : 1;
        RK_U32    dblk_fltr_ovrd_en : 1;
        RK_U32    lst_mdfy_prsnt_flg : 1;
        RK_U32    sli_seg_hdr_extn : 1;
        RK_U32    cu_qp_dlt_depth  : 2;
        RK_U32    lpf_fltr_acrs_til : 1; /*only for 540*/
        RK_U32    reserved : 10;
    } synt_pps;

    /* 0x1a8 - swreg60, SYNT_SLI0 */
    struct {
        RK_U32    cbc_init_flg : 1;
        RK_U32    mvd_l1_zero_flg : 1;
        RK_U32    merge_up_flag : 1;
        RK_U32    merge_left_flag : 1;
        RK_U32    reserved : 1;
        RK_U32    ref_pic_lst_mdf_l0 : 1;
        RK_U32    num_refidx_l1_act : 2;
        RK_U32    num_refidx_l0_act : 2;
        RK_U32    num_refidx_act_ovrd : 1;
        RK_U32    sli_sao_chrm_flg : 1;
        RK_U32    sli_sao_luma_flg : 1;
        RK_U32    sli_tmprl_mvp_en : 1;
        RK_U32    pic_out_flg : 1;
        RK_U32    sli_type : 2;
        RK_U32    sli_rsrv_flg : 7;
        RK_U32    dpdnt_sli_seg_flg : 1;
        RK_U32    sli_pps_id : 6;
        RK_U32    no_out_pri_pic : 1;
    } synt_sli0;

    /* 0x1ac - swreg61, SYNT_SLI1 */
    struct {
        RK_S32    sli_tc_ofst_div2            : 4;
        RK_S32    sli_beta_ofst_div2        : 4;
        RK_U32    sli_lp_fltr_acrs_sli         : 1;
        RK_U32    sli_dblk_fltr_dis           : 1;
        RK_U32    dblk_fltr_ovrd_flg        : 1;
        RK_S32    sli_cb_qp_ofst             : 5;
        RK_U32    sli_qp                         : 6;
        RK_U32    max_mrg_cnd                : 3;
        RK_U32    col_ref_idx                 : 1;
        RK_U32    col_frm_l0_flg            : 1;
        RK_U32    lst_entry_l0                : 4;
        RK_U32    reserved                    : 1;
    } synt_sli1;

    /* 0x1b0 - swreg62, SYNT_SLI2_RODR */
    struct {
        RK_U32    sli_poc_lsb : 16;
        RK_U32    sli_hdr_ext_len : 9;
        RK_U32    reserve : 7;
    } synt_sli2_rodr;

    /* 0x1b4 - swreg63, SYNT_REF_MARK0 */
    struct {
        RK_U32    st_ref_pic_flg : 1;
        RK_U32    poc_lsb_lt0 : 16;
        RK_U32    lt_idx_sps : 5;
        RK_U32    num_lt_pic : 2;
        RK_U32    st_ref_pic_idx : 6;
        RK_U32    num_lt_sps : 2;
    } synt_ref_mark0;

    /* 0x1b8 - swreg64, SYNT_REF_MARK1 */
    struct {
        RK_U32    used_by_s0_flg : 4;
        RK_U32    num_pos_pic : 1;
        RK_U32    num_neg_pic : 5;
        RK_U32    dlt_poc_msb_cycl0 : 16;
        RK_U32    dlt_poc_msb_prsnt0 : 1;
        RK_U32    dlt_poc_msb_prsnt1 : 1;
        RK_U32    dlt_poc_msb_prsnt2 : 1;
        RK_U32    used_by_lt_flg0 : 1;
        RK_U32    used_by_lt_flg1 : 1;
        RK_U32    used_by_lt_flg2 : 1;
    } synt_ref_mark1;

    RK_U32 reserved_0x1bc; /* not used for a long time */

    /* 0x1c0 - OSD_CFG */
    struct {
        RK_U32    osd_en       : 8;
        RK_U32    osd_inv      : 8;
        RK_U32    osd_clk_sel  : 1;
        RK_U32    osd_plt_type : 1;
        RK_U32    reserved     : 14;
    } osd_cfg;

    /* 0x1c4 - OSD_INV */
    struct {
        RK_U32    osd_inv_r0 : 4;
        RK_U32    osd_inv_r1 : 4;
        RK_U32    osd_inv_r2 : 4;
        RK_U32    osd_inv_r3 : 4;
        RK_U32    osd_inv_r4 : 4;
        RK_U32    osd_inv_r5 : 4;
        RK_U32    osd_inv_r6 : 4;
        RK_U32    osd_inv_r7 : 4;
    } osd_inv;

    /* 0x1c8 - SYNT_REF_MARK2 */
    struct {
        RK_U32    dlt_poc_s0_m10 : 16;
        RK_U32    dlt_poc_s0_m11 : 16;
    } synt_ref_mark2;

    /* 0x1cc - SYNT_REF_MARK3 */
    struct {
        RK_U32    dlt_poc_s0_m12 : 16;
        RK_U32    dlt_poc_s0_m13 : 16;
    } synt_ref_mark3;

    /* 0x1d0-0x1ec - OSD_POS0-OSD_POS7 */
    OsdPos osd_pos[8];

    /* 0x1f0-0x20c - OSD_ADDR0-OSD_ADDR7 */
    RK_U32    osd_addr[8];

    /* 0x210 - ST_BSL */
    struct {
        RK_U32    bs_lgth : 32;
    } st_bsl;

    /* 0x214 - ST_SSE_L32 */
    struct {
        RK_U32    sse_l32 : 32;
    } st_sse_l32;

    /* 0x218 - ST_SSE_QP */
    struct {
        RK_U32    qp_sum : 24;  /* sum of valid CU8x8s' QP */
        RK_U32    sse_h8 : 8;
    } st_sse_qp;

    /* 0x21c - ST_SAO */
    struct {
        RK_U32    slice_scnum : 12;
        RK_U32    slice_slnum : 12;
        RK_U32    reserve : 8;
    } st_sao;

    /* 0x220 - MMU0_STA, used by hardware?? */
    RK_U32 mmu0_sta;
    RK_U32 mmu1_sta;

    /* 0x228 - ST_ENC */
    struct {
        RK_U32    st_enc : 2;
        RK_U32    axiw_cln : 2;
        RK_U32    axir_cln : 2;
        RK_U32    reserve : 26;
    } st_enc;

    /* 0x22c - ST_LKT */
    struct {
        RK_U32    fnum_enc : 8;
        RK_U32    fnum_cfg : 8;
        RK_U32    fnum_int : 8;
        RK_U32    reserve : 8;
    } st_lkt;

    /* 0x230 - ST_NOD */
    struct {
        RK_U32    node_addr : 32;
    } st_nod;

    /* 0x234 - ST_BSB */
    struct {
        RK_U32    Bsbw_ovfl : 1;
        RK_U32    reserve : 2;
        RK_U32    bsbw_addr : 29;
    } st_bsb;

    /* 0x238 - ST_DTRNS */
    struct {
        RK_U32    axib_idl : 7;
        RK_U32    axib_ful : 7;
        RK_U32    axib_err : 7;
        RK_U32    axir_err : 6;
        RK_U32    reserve : 5;
    } st_dtrns;

    /* 0x23c - ST_SNUM */
    struct {
        RK_U32    slice_num : 6;
        RK_U32    reserve : 26;
    } st_snum;

    /* 0x240 - ST_SLEN */
    struct {
        RK_U32    slice_len : 23;
        RK_U32    reserve : 9;
    } st_slen;

    /* 0x244-0x340 - debug registers
     * ST_LVL64_INTER_NUM etc.
     */
    RK_U32 st_lvl64_inter_num;
    RK_U32 st_lvl32_inter_num;
    RK_U32 st_lvl16_inter_num;
    RK_U32 st_lvl8_inter_num;
    RK_U32 st_lvl32_intra_num;
    RK_U32 st_lvl16_intra_num;
    RK_U32 st_lvl8_intra_num;
    RK_U32 st_lvl4_intra_num;
    RK_U32 st_cu_num_qp[52];
    RK_U32 st_madp;
    RK_U32 st_ctu_num; /* used for MADP calculation */
    RK_U32 st_madi;
    RK_U32 st_mb_num; /* used for MADI calculation */

} H265eV541RegSet;

typedef struct H265eV541IoctlExtraInfoElem_t {
    RK_U32 reg_idx;
    RK_U32 offset;
} H265eV541IoctlExtraInfoElem;

typedef struct H265eV541IoctlExtraInfo_t {
    RK_U32                          magic;
    RK_U32                          cnt;
    H265eV541IoctlExtraInfoElem     elem[20];
} H265eV541IoctlExtraInfo;

typedef struct H265eV541IoctlOutputElem_t {
    RK_U32 hw_status;

    struct {
        RK_U32    bs_lgth : 32;
    } st_bsl;

    /* 0x214 - ST_SSE_L32 */
    struct {
        RK_U32    sse_l32 : 32;
    } st_sse_l32;

    /* 0x218 - ST_SSE_QP */
    struct {
        RK_U32    qp_sum : 24;  /* sum of valid CU8x8s' QP */
        RK_U32    sse_h8 : 8;
    } st_sse_qp;

    /* 0x21c - ST_SAO */
    struct {
        RK_U32    slice_scnum : 12;
        RK_U32    slice_slnum : 12;
        RK_U32    reserve : 8;
    } st_sao;

    /* 0x220 - MMU0_STA, used by hardware?? */
    RK_U32 mmu0_sta;
    RK_U32 mmu1_sta;

    /* 0x228 - ST_ENC */
    struct {
        RK_U32    st_enc : 2;
        RK_U32    axiw_cln : 2;
        RK_U32    axir_cln : 2;
        RK_U32    reserve : 26;
    } st_enc;

    /* 0x22c - ST_LKT */
    struct {
        RK_U32    fnum_enc : 8;
        RK_U32    fnum_cfg : 8;
        RK_U32    fnum_int : 8;
        RK_U32    reserve : 8;
    } st_lkt;

    /* 0x230 - ST_NOD */
    struct {
        RK_U32    node_addr : 32;
    } st_nod;

    /* 0x234 - ST_BSB */
    struct {
        RK_U32    Bsbw_ovfl : 1;
        RK_U32    reserve : 2;
        RK_U32    bsbw_addr : 29;
    } st_bsb;

    /* 0x238 - ST_DTRNS */
    struct {
        RK_U32    axib_idl : 7;
        RK_U32    axib_ful : 7;
        RK_U32    axib_err : 7;
        RK_U32    axir_err : 6;
        RK_U32    reserve : 5;
    } st_dtrns;

    /* 0x23c - ST_SNUM */
    struct {
        RK_U32    slice_num : 6;
        RK_U32    reserve : 26;
    } st_snum;

    /* 0x240 - ST_SLEN */
    struct {
        RK_U32    slice_len : 23;
        RK_U32    reserve : 9;
    } st_slen;
    RK_U32 st_lvl64_inter_num;
    RK_U32 st_lvl32_inter_num;
    RK_U32 st_lvl16_inter_num;
    RK_U32 st_lvl8_inter_num;
    RK_U32 st_lvl32_intra_num;
    RK_U32 st_lvl16_intra_num;
    RK_U32 st_lvl8_intra_num;
    RK_U32 st_lvl4_intra_num;
    RK_U32 st_cu_num_qp[52];
    RK_U32 st_madp;
    RK_U32 st_ctu_num; /* used for MADP calculation */
    RK_U32 st_madi;
    RK_U32 st_mb_num; /* used for MADI calculation */
} H265eV541IoctlOutputElem;

#endif
