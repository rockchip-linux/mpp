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

#ifndef __HAL_H264E_VEPU580_REG_H__
#define __HAL_H264E_VEPU580_REG_H__

#include "rk_type.h"

#define VEPU580_CONTROL_CFG_OFFSET  (0 * sizeof(RK_U32))
#define VEPU580_BASE_CFG_OFFSET     (160 * sizeof(RK_U32))
#define VEPU580_RC_KLUT_CFG_OFFSET  (1024 * sizeof(RK_U32))
#define VEPU580_SECTION_3_OFFSET    (1472 * sizeof(RK_U32))
#define VEPU580_RDO_CFG_OFFSET      (2048 * sizeof(RK_U32))
#define VEPU580_SCL_CFG_OFFSET      (2176 * sizeof(RK_U32))
#define VEPU580_OSD_OFFSET          (3072 * sizeof(RK_U32))
#define VEPU580_STATUS_OFFSET       (4096 * sizeof(RK_U32))
#define VEPU580_DBG_OFFSET          (5120 * sizeof(RK_U32))
#define VEPU580_REG_BASE_HW_STATUS  0x2c

typedef struct {
    RK_U32 lt_pos_x : 10;
    RK_U32 reserved0 : 6;
    RK_U32 lt_pos_y : 10;
    RK_U32 reserved1 : 6;
} OsdLtPos;

typedef struct {
    RK_U32 rb_pos_x : 10;
    RK_U32 reserved0 : 6;
    RK_U32 rb_pos_y : 10;
    RK_U32 reserved1 : 6;
} OsdRbPos;

typedef struct OSD_POS_NEW {
    OsdLtPos lt; /* left-top */
    OsdRbPos rb; /* right-bottom */
} OSD_POS_NEW;

/* class: control/link */
/* 0x00000000 reg0 - 0x00000120 reg72 */
typedef struct Vepu580ControlCfg_t {
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
    } int_en;

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
    } int_msk;

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
    } int_clr;

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
    } int_sta;

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
    } dtrns_map;

    /* 0x00000034 reg13 */
    struct {
        RK_U32 reserved        : 7;
        RK_U32 dspr_otsd       : 1;
        RK_U32 reserved1       : 8;
        RK_U32 axi_brsp_cke    : 8;
        RK_U32 reserved2       : 8;
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
        RK_U32 reserved            : 28;
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
} Vepu580ControlCfg;

/* class: buffer/video syntax */
/* 0x00000280 reg160 - 0x000003f4 reg253*/
typedef struct Vepu580BaseCfg_t {
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
    RK_U32 bsbr_addr;

    /* 0x000002bc reg175 */
    RK_U32 adr_bsbs;

    /* 0x000002c0 reg176 */
    RK_U32 lpfw_addr;

    /* 0x000002c4 reg177 */
    RK_U32 lpfr_addr;

    /* 0x000002c8 reg178 */
    RK_U32 roi_addr;

    /* 0x000002cc reg179 */
    RK_U32 roi_qp_addr;

    /* 0x000002d0 reg180 */
    RK_U32 qoi_amv_addr;

    /* 0x000002d4 reg181 */
    RK_U32 qoi_mv_addr;

    /* 0x000002d8 reg182 */
    RK_U32 ebuft_addr;

    /* 0x000002dc reg183 */
    RK_U32 ebufb_addr;

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
    } enc_pic;

    /* 0x00000304 reg193 */
    struct {
        RK_U32 dchs_txid    : 2;
        RK_U32 dchs_rxid    : 2;
        RK_U32 dchs_txe     : 1;
        RK_U32 dchs_rxe     : 1;
        RK_U32 reserved     : 10;
        RK_U32 dchs_ofst    : 11;
        RK_U32 reserved1    : 5;
    } dual_core;

    /* 0x308 - 0x30c */
    RK_U32 reserved194_195[2];

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
        RK_U32 alpha_swap    : 1;
        RK_U32 rbuv_swap     : 1;
        RK_U32 src_cfmt      : 4;
        RK_U32 src_range     : 1;
        RK_U32 out_fmt       : 1;
        RK_U32 reserved      : 24;
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
        RK_U32 reserved0   : 26;
        RK_U32 src_mirr    : 1;
        RK_U32 src_rot     : 2;
        RK_U32 txa_en      : 1;
        RK_U32 afbcd_en    : 1;
        RK_U32 reserved1   : 1;
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

    /* 0x33c - 0x34c */
    RK_U32 reserved207_211[5];

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
    RK_U32 reserved_219;

    /* 0x00000370 reg220 */
    struct {
        RK_U32 cme_srch_h     : 4;
        RK_U32 cme_srch_v     : 4;
        RK_U32 rme_srch_h     : 3;
        RK_U32 rme_srch_v     : 3;
        RK_U32 reserved       : 2;
        RK_U32 dlt_frm_num    : 16;
    } me_rnge;

    /* 0x00000374 reg221 */
    struct {
        RK_U32 pmv_mdst_h      : 8;
        RK_U32 pmv_mdst_v      : 8;
        RK_U32 mv_limit        : 2;
        RK_U32 pmv_num         : 2;
        RK_U32 colmv_stor      : 1;
        RK_U32 colmv_load      : 1;
        RK_U32 rme_dis         : 3;
        RK_U32 reserved        : 2;
        RK_U32 fme_dis         : 3;
        RK_U32 reserved1       : 1;
        RK_U32 lvl4_ovrd_en    : 1;
    } me_cfg;

    /* 0x00000378 reg222 */
    struct {
        RK_U32 cme_rama_max      : 11;
        RK_U32 cme_rama_h        : 5;
        RK_U32 cach_l2_tag       : 2;
        RK_U32 cme_linebuf_w     : 9;
        RK_U32 reserved          : 5;
    } me_cach;

    /* 0x37c */
    RK_U32 reserved_223;

    /* 0x00000380 reg224 */
    struct {
        RK_U32 gmv_x        : 13;
        RK_U32 reserved     : 3;
        RK_U32 gmv_y        : 13;
        RK_U32 reserved1    : 3;
    } gmv;

    /* 0x384 - 0x38c */
    RK_U32 reserved225_227[3];

    /* 0x00000390 reg228 */
    struct {
        RK_U32 roi_qp_en     : 1;
        RK_U32 roi_amv_en    : 1;
        RK_U32 roi_mv_en     : 1;
        RK_U32 reserved      : 29;
    } roi_en;

    /* 0x394 - 0x39c */
    RK_U32 reserved229_231[3];

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 rect_size      : 1;
        RK_U32 inter_4x4      : 1;
        RK_U32 arb_sel        : 1;
        RK_U32 vlc_lmt        : 1;
        RK_U32 chrm_spcl      : 1;
        RK_U32 rdo_mask       : 8;
        RK_U32 ccwa_e         : 1;
        RK_U32 reserved       : 1;
        RK_U32 atr_e          : 1;
        RK_U32 reserved1      : 3;
        RK_U32 atf_intra_e    : 1;
        RK_U32 scl_lst_sel    : 2;
        RK_U32 reserved2      : 10;
    } rdo_cfg;

    /* 0x000003a4 reg233 */
    struct {
        RK_U32 vthd_y       : 12;
        RK_U32 reserved     : 4;
        RK_U32 vthd_c       : 12;
        RK_U32 reserved1    : 4;
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
        RK_U32 wght_pred       : 1;
        RK_U32 dbf_cp_flg      : 1;
        RK_U32 reserved        : 7;
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
    } synt_long_refm0;

    /* 0x000003dc reg247 */
    struct {
        RK_U32 dlt_poc_msb_cycl1    : 16;
        RK_U32 dlt_poc_msb_cycl2    : 16;
    } synt_long_refm1;

    /* 0x3e0 - 0x3ec */
    RK_U32 reserved248_251[4];

    /* 0x000003f0 reg252 */
    struct {
        RK_U32 mv_v_lmt_thd    : 14;
        RK_U32 reserved        : 1;
        RK_U32 mv_v_lmt_en     : 1;
        RK_U32 reserved1       : 15;
        RK_U32 sli_crs_en      : 1;
    } sli_cfg;

    /* 0x000003f4 reg253 */
    struct {
        RK_U32 tile_x       : 8;
        RK_U32 reserved     : 8;
        RK_U32 tile_y       : 8;
        RK_U32 reserved1    : 8;
    } tile_pos;
} Vepu580BaseCfg;

/* class: rc/roi/aq/klut */
/* 0x00001000 reg1024 - 0x000010e0 reg1080 */
typedef struct Vepu580RcKlutCfg_t {
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
} Vepu580RcKlutCfg;

/* class: iprd/iprd_wgt/rdo_wgta/prei_dif/sobel */
/* 0x00001700 reg1472 - 0x00001cd4 reg1845 */
typedef struct Vepu580Section3_t {
    /* 0x1700 */
    struct {
        RK_U32    lvl4_intra_cst_thd0 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    lvl4_intra_cst_thd1 : 12;
        RK_U32    reserve1 : 4;
    } lvl32_intra_CST_THD0;

    /* 0x1704 */
    struct {
        RK_U32    lvl4_intra_cst_thd2 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    lvl4_intra_cst_thd3 : 12;
        RK_U32    reserve1 : 4;
    } lvl32_intra_CST_THD1;

    /* 0x1708 */
    struct {
        RK_U32    lvl8_intra_chrm_cst_thd0 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    lvl8_intra_chrm_cst_thd1 : 12;
        RK_U32    reserve1 : 4;
    } lvl16_intra_CST_THD0;

    /* 0x170c */
    struct {
        RK_U32    lvl8_intra_chrm_cst_thd2 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    lvl8_intra_chrm_cst_thd3 : 12;
        RK_U32    reserve1 : 4;
    } lvl16_intra_CST_THD1;

    /* 0x1710 */
    struct {
        RK_U32    lvl8_intra_cst_thd0 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    lvl8_intra_cst_thd1 : 12;
        RK_U32    reserve1 : 4;
    } lvl8_intra_CST_THD0; //     only 264

    /* 0x1714 */
    struct {
        RK_U32    lvl8_intra_cst_thd2 : 12;
        RK_U32    reserve0 : 4;
        RK_U32    lvl8_intra_cst_thd3 : 12;
        RK_U32    reserve1 : 4;
    } lvl8_intra_CST_THD1; //     only 264

    /* 0x1718 */
    struct {
        RK_U32    lvl16_intra_ul_cst_thld : 12;
        RK_U32    reserve0 : 20;
    } lvl16_intra_UL_CST_THD; //      only 264

    /* 0x171c */
    struct {
        RK_U32    lvl8_intra_cst_wgt0 : 8;

        RK_U32    lvl8_intra_cst_wgt1 : 8;

        RK_U32    lvl8_intra_cst_wgt2 : 8;

        RK_U32    lvl8_intra_cst_wgt3 : 8;

    } lvl32_intra_CST_WGT0;

    /* 0x1720 */
    struct {
        RK_U32    lvl4_intra_cst_wgt0 : 8;

        RK_U32    lvl4_intra_cst_wgt1 : 8;

        RK_U32    lvl4_intra_cst_wgt2 : 8;

        RK_U32    lvl4_intra_cst_wgt3 : 8;

    } lvl32_intra_CST_WGT1; //

    /* 0x1724 */
    struct {
        RK_U32    lvl16_intra_cst_wgt0 : 8;

        RK_U32    lvl16_intra_cst_wgt1 : 8;

        RK_U32    lvl16_intra_cst_wgt2 : 8;

        RK_U32    lvl16_intra_cst_wgt3 : 8;

    } lvl16_intra_CST_WGT0; //  7.10

    /* 0x1728 */
    struct {
        RK_U32    lvl8_intra_chrm_cst_wgt0 : 8;

        RK_U32    lvl8_intra_chrm_cst_wgt1 : 8;

        RK_U32    lvl8_intra_chrm_cst_wgt2 : 8;

        RK_U32    lvl8_intra_chrm_cst_wgt3 : 8;

    } lvl16_intra_CST_WGT1; //

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
    /* 0x00001900 reg1600 - 0x19cc */
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} Vepu580Section3;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x00002c98 reg2854 */
typedef struct Vepu580RdoCfg_t {
    /* 0x00002000 reg2048 */
    struct {
        RK_U32 atf_pskip_en    : 1;
        RK_U32 reserved             : 31;
    } rdo_sqi_cfg;

    /* 0x00002004 reg2049 */
    struct {
        RK_U32 cu64_rdo_inter_cime_thd0    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu64_rdo_inter_cime_thd1    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b64_inter_cime_thd0;

    /* 0x00002008 reg2050 */
    struct {
        RK_U32 cu64_rdo_inter_cime_thd2    : 12;
        RK_U32 reserved                    : 20;
    } rdo_b64_inter_cime_thd1;

    /* 0x0000200c reg2051 */
    struct {
        RK_U32 cu64_rdo_inter_var_thd00    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu64_rdo_inter_var_thd01    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b64_inter_var_thd0;

    /* 0x00002010 reg2052 */
    struct {
        RK_U32 cu64_rdo_inter_var_thd10    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu64_rdo_inter_var_thd11    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b64_inter_var_thd1;

    /* 0x00002014 reg2053 */
    struct {
        RK_U32 cu64_rdo_inter_var_thd20    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu64_rdo_inter_var_thd21    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b64_inter_var_thd2;

    /* 0x00002018 reg2054 */
    struct {
        RK_U32 cu64_rdo_inter_var_thd30    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu64_rdo_inter_var_thd31    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b64_inter_var_thd3;

    /* 0x0000201c reg2055 */
    struct {
        RK_U32 cu64_rdo_inter_atf_wgt00    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt01    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt02    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b64_inter_atf_wgt0;

    /* 0x00002020 reg2056 */
    struct {
        RK_U32 cu64_rdo_inter_atf_wgt10    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt11    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt12    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b64_inter_atf_wgt1;

    /* 0x00002024 reg2057 */
    struct {
        RK_U32 cu64_rdo_inter_atf_wgt20    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt21    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt22    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b64_inter_atf_wgt2;

    /* 0x00002028 reg2058 */
    struct {
        RK_U32 cu64_rdo_inter_atf_wgt30    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt31    : 8;
        RK_U32 cu64_rdo_inter_atf_wgt32    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b64_inter_atf_wgt3;

    /* 0x0000202c reg2059 */
    struct {
        RK_U32 cu64_rdo_skip_cime_thd0    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu64_rdo_skip_cime_thd1    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b64_skip_cime_thd0;

    /* 0x00002030 reg2060 */
    struct {
        RK_U32 cu64_rdo_skip_cime_thd2    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu64_rdo_skip_cime_thd3    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b64_skip_cime_thd1;

    /* 0x00002034 reg2061 */
    struct {
        RK_U32 cu64_rdo_skip_var_thd10    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu64_rdo_skip_var_thd11    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b64_skip_var_thd0;

    /* 0x00002038 reg2062 */
    struct {
        RK_U32 cu64_rdo_skip_var_thd20    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu64_rdo_skip_var_thd21    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b64_skip_var_thd1;

    /* 0x0000203c reg2063 */
    struct {
        RK_U32 cu64_rdo_skip_var_thd30    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu64_rdo_skip_var_thd31    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b64_skip_var_thd2;

    /* 0x00002040 reg2064 */
    struct {
        RK_U32 cu64_rdo_skip_var_thd40    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu64_rdo_skip_var_thd41    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b64_skip_var_thd3;

    /* 0x00002044 reg2065 */
    struct {
        RK_U32 cu64_rdo_skip_atf_wgt00    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt10    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt11    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt12    : 8;
    } rdo_b64_skip_atf_wgt0;

    /* 0x00002048 reg2066 */
    struct {
        RK_U32 cu64_rdo_skip_atf_wgt20    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt21    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt22    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b64_skip_atf_wgt1;

    /* 0x0000204c reg2067 */
    struct {
        RK_U32 cu64_rdo_skip_atf_wgt30    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt31    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt32    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b64_skip_atf_wgt2;

    /* 0x00002050 reg2068 */
    struct {
        RK_U32 cu64_rdo_skip_atf_wgt40    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt41    : 8;
        RK_U32 cu64_rdo_skip_atf_wgt42    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b64_skip_atf_wgt3;

    /* 0x00002054 reg2069 */
    struct {
        RK_U32 cu32_rdo_intra_cime_thd0    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_intra_cime_thd1    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_intra_cime_thd0;

    /* 0x00002058 reg2070 */
    struct {
        RK_U32 cu32_rdo_intra_cime_thd2    : 12;
        RK_U32 reserved                    : 20;
    } rdo_b32_intra_cime_thd1;

    /* 0x0000205c reg2071 */
    struct {
        RK_U32 cu32_rdo_intra_var_thd00    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_intra_var_thd01    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_intra_var_thd0;

    /* 0x00002060 reg2072 */
    struct {
        RK_U32 cu32_rdo_intra_var_thd10    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_intra_var_thd11    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_intra_var_thd1;

    /* 0x00002064 reg2073 */
    struct {
        RK_U32 cu32_rdo_intra_var_thd20    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_intra_var_thd21    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_intra_var_thd2;

    /* 0x00002068 reg2074 */
    struct {
        RK_U32 cu32_rdo_intra_var_thd30    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_intra_var_thd31    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_intra_var_thd3;

    /* 0x0000206c reg2075 */
    struct {
        RK_U32 cu32_rdo_intra_atf_wgt00    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt01    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt02    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_intra_atf_wgt0;

    /* 0x00002070 reg2076 */
    struct {
        RK_U32 cu32_rdo_intra_atf_wgt10    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt11    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt12    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_intra_atf_wgt1;

    /* 0x00002074 reg2077 */
    struct {
        RK_U32 cu32_rdo_intra_atf_wgt20    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt21    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt22    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_intra_atf_wgt2;

    /* 0x00002078 reg2078 */
    struct {
        RK_U32 cu32_rdo_intra_atf_wgt30    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt31    : 8;
        RK_U32 cu32_rdo_intra_atf_wgt32    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_intra_atf_wgt3;

    /* 0x0000207c reg2079 */
    struct {
        RK_U32 cu32_rdo_inter_cime_thd0    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_inter_cime_thd1    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_inter_cime_thd0;

    /* 0x00002080 reg2080 */
    struct {
        RK_U32 cu32_rdo_inter_cime_thd2    : 12;
        RK_U32 reserved                    : 20;
    } rdo_b32_inter_cime_thd1;

    /* 0x00002084 reg2081 */
    struct {
        RK_U32 cu32_rdo_inter_var_thd00    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_inter_var_thd01    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_inter_var_thd0;

    /* 0x00002088 reg2082 */
    struct {
        RK_U32 cu32_rdo_inter_var_thd10    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_inter_var_thd11    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_inter_var_thd1;

    /* 0x0000208c reg2083 */
    struct {
        RK_U32 cu32_rdo_inter_var_thd20    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_inter_var_thd21    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_inter_var_thd2;

    /* 0x00002090 reg2084 */
    struct {
        RK_U32 cu32_rdo_inter_var_thd30    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu32_rdo_inter_var_thd31    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b32_inter_var_thd3;

    /* 0x00002094 reg2085 */
    struct {
        RK_U32 cu32_rdo_inter_atf_wgt00    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt01    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt02    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_inter_atf_wgt0;

    /* 0x00002098 reg2086 */
    struct {
        RK_U32 cu32_rdo_inter_atf_wgt10    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt11    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt12    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_inter_atf_wgt1;

    /* 0x0000209c reg2087 */
    struct {
        RK_U32 cu32_rdo_inter_atf_wgt20    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt21    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt22    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_inter_atf_wgt2;

    /* 0x000020a0 reg2088 */
    struct {
        RK_U32 cu32_rdo_inter_atf_wgt30    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt31    : 8;
        RK_U32 cu32_rdo_inter_atf_wgt32    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b32_inter_atf_wgt3;

    /* 0x000020a4 reg2089 */
    struct {
        RK_U32 cu32_rdo_skip_cime_thd0    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu32_rdo_skip_cime_thd1    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b32_skip_cime_thd0;

    /* 0x000020a8 reg2090 */
    struct {
        RK_U32 cu32_rdo_skip_cime_thd2    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu32_rdo_skip_cime_thd3    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b32_skip_cime_thd1;

    /* 0x000020ac reg2091 */
    struct {
        RK_U32 cu32_rdo_skip_var_thd10    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu32_rdo_skip_var_thd11    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b32_sskip_var_thd0;

    /* 0x000020b0 reg2092 */
    struct {
        RK_U32 cu32_rdo_skip_var_thd20    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu32_rdo_skip_var_thd21    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b32_sskip_var_thd1;

    /* 0x000020b4 reg2093 */
    struct {
        RK_U32 cu32_rdo_skip_var_thd30    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu32_rdo_skip_var_thd31    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b32_sskip_var_thd2;

    /* 0x000020b8 reg2094 */
    struct {
        RK_U32 cu32_rdo_skip_var_thd40    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu32_rdo_skip_var_thd41    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b32_sskip_var_thd3;

    /* 0x000020bc reg2095 */
    struct {
        RK_U32 cu32_rdo_skip_atf_wgt00    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt10    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt11    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt12    : 8;
    } rdo_b32_skip_atf_wgt0;

    /* 0x000020c0 reg2096 */
    struct {
        RK_U32 cu32_rdo_skip_atf_wgt20    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt21    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt22    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b32_skip_atf_wgt1;

    /* 0x000020c4 reg2097 */
    struct {
        RK_U32 cu32_rdo_skip_atf_wgt30    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt31    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt32    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b32_skip_atf_wgt2;

    /* 0x000020c8 reg2098 */
    struct {
        RK_U32 cu32_rdo_skip_atf_wgt40    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt41    : 8;
        RK_U32 cu32_rdo_skip_atf_wgt42    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b32_skip_atf_wgt3;

    /* 0x000020cc reg2099 */
    struct {
        RK_U32 atf_rdo_intra_cime_thd0    : 12;
        RK_U32 reserved                        : 4;
        RK_U32 atf_rdo_intra_cime_thd1    : 12;
        RK_U32 reserved1                       : 4;
    } rdo_intra_cime_thd0;

    /* 0x000020d0 reg2100 */
    struct {
        RK_U32 atf_rdo_intra_cime_thd2    : 12;
        RK_U32 reserved                        : 20;
    } rdo_intra_cime_thd1;

    /* 0x000020d4 reg2101 */
    struct {
        RK_U32 atf_rdo_intra_var_thd00    : 12;
        RK_U32 reserved                        : 4;
        RK_U32 atf_rdo_intra_var_thd01    : 12;
        RK_U32 reserved1                       : 4;
    } rdo_intra_var_thd0;

    /* 0x000020d8 reg2102 */
    struct {
        RK_U32 atf_rdo_intra_var_thd10    : 12;
        RK_U32 reserved                        : 4;
        RK_U32 atf_rdo_intra_var_thd11    : 12;
        RK_U32 reserved1                       : 4;
    } rdo_intra_var_thd1;

    /* 0x000020dc reg2103 */
    struct {
        RK_U32 atf_rdo_intra_var_thd20    : 12;
        RK_U32 reserved                        : 4;
        RK_U32 atf_rdo_intra_var_thd21    : 12;
        RK_U32 reserved1                       : 4;
    } rdo_intra_var_thd2;

    /* 0x000020e0 reg2104 */
    struct {
        RK_U32 atf_rdo_intra_var_thd30    : 12;
        RK_U32 reserved                        : 4;
        RK_U32 atf_rdo_intra_var_thd31    : 12;
        RK_U32 reserved1                       : 4;
    } rdo_intra_var_thd3;

    /* 0x000020e4 reg2105 */
    struct {
        RK_U32 atf_rdo_intra_wgt00    : 8;
        RK_U32 atf_rdo_intra_wgt01    : 8;
        RK_U32 atf_rdo_intra_wgt02    : 8;
        RK_U32 reserved                    : 8;
    } rdo_intra_atf_wgt0;

    /* 0x000020e8 reg2106 */
    struct {
        RK_U32 atf_rdo_intra_wgt10    : 8;
        RK_U32 atf_rdo_intra_wgt11    : 8;
        RK_U32 atf_rdo_intra_wgt12    : 8;
        RK_U32 reserved                    : 8;
    } rdo_intra_atf_wgt1;

    /* 0x000020ec reg2107 */
    struct {
        RK_U32 atf_rdo_intra_wgt20    : 8;
        RK_U32 atf_rdo_intra_wgt21    : 8;
        RK_U32 atf_rdo_intra_wgt22    : 8;
        RK_U32 reserved                    : 8;
    } rdo_intra_atf_wgt2;

    /* 0x000020f0 reg2108 */
    struct {
        RK_U32 atf_rdo_intra_wgt30    : 8;
        RK_U32 atf_rdo_intra_wgt31    : 8;
        RK_U32 atf_rdo_intra_wgt32    : 8;
        RK_U32 reserved                    : 8;
    } rdo_intra_atf_wgt3;

    /* 0x000020f4 reg2109 */
    struct {
        RK_U32 cu16_rdo_inter_cime_thd0    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu16_rdo_inter_cime_thd1    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b16_inter_cime_thd0;

    /* 0x000020f8 reg2110 */
    struct {
        RK_U32 cu16_rdo_inter_cime_thd2    : 12;
        RK_U32 reserved                    : 20;
    } rdo_b16_inter_cime_thd1;

    /* 0x000020fc reg2111 */
    struct {
        RK_U32 cu16_rdo_inter_var_thd00    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu16_rdo_inter_var_thd01    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b16_inter_var_thd0;

    /* 0x00002100 reg2112 */
    struct {
        RK_U32 cu16_rdo_inter_var_thd10    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu16_rdo_inter_var_thd11    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b16_inter_var_thd1;

    /* 0x00002104 reg2113 */
    struct {
        RK_U32 cu16_rdo_inter_var_thd20    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu16_rdo_inter_var_thd21    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b16_inter_var_thd2;

    /* 0x00002108 reg2114 */
    struct {
        RK_U32 cu16_rdo_inter_var_thd30    : 12;
        RK_U32 reserved                    : 4;
        RK_U32 cu16_rdo_inter_var_thd31    : 12;
        RK_U32 reserved1                   : 4;
    } rdo_b16_inter_var_thd3;

    /* 0x0000210c reg2115 */
    struct {
        RK_U32 cu16_rdo_inter_atf_wgt00    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt01    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt02    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b16_inter_atf_wgt0;

    /* 0x00002110 reg2116 */
    struct {
        RK_U32 cu16_rdo_inter_atf_wgt10    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt11    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt12    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b16_inter_atf_wgt1;

    /* 0x00002114 reg2117 */
    struct {
        RK_U32 cu16_rdo_inter_atf_wgt20    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt21    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt22    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b16_inter_atf_wgt2;

    /* 0x00002118 reg2118 */
    struct {
        RK_U32 cu16_rdo_inter_atf_wgt30    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt31    : 8;
        RK_U32 cu16_rdo_inter_atf_wgt32    : 8;
        RK_U32 reserved                    : 8;
    } rdo_b16_inter_atf_wgt3;

    /* 0x0000211c reg2119 */
    struct {
        RK_U32 atf_rdo_skip_cime_thd0    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 atf_rdo_skip_cime_thd1    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_skip_cime_thd0;

    /* 0x00002120 reg2120 */
    struct {
        RK_U32 atf_rdo_skip_cime_thd2    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 atf_rdo_skip_cime_thd3    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_skip_cime_thd1;

    /* 0x00002124 reg2121 */
    struct {
        RK_U32 atf_rdo_skip_var_thd10    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 atf_rdo_skip_var_thd11    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_skip_var_thd0;

    /* 0x00002128 reg2122 */
    struct {
        RK_U32 atf_rdo_skip_var_thd20    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 atf_rdo_skip_var_thd21    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_skip_var_thd1;

    /* 0x0000212c reg2123 */
    struct {
        RK_U32 atf_rdo_skip_var_thd30    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 atf_rdo_skip_var_thd31    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_skip_var_thd2;

    /* 0x00002130 reg2124 */
    struct {
        RK_U32 atf_rdo_skip_var_thd40    : 12;
        RK_U32 reserved                       : 4;
        RK_U32 atf_rdo_skip_var_thd41    : 12;
        RK_U32 reserved1                      : 4;
    } rdo_skip_var_thd3;

    /* 0x00002134 reg2125 */
    struct {
        RK_U32 atf_rdo_skip_atf_wgt00    : 8;
        RK_U32 atf_rdo_skip_atf_wgt10    : 8;
        RK_U32 atf_rdo_skip_atf_wgt11    : 8;
        RK_U32 atf_rdo_skip_atf_wgt12    : 8;
    } rdo_skip_atf_wgt0;

    /* 0x00002138 reg2126 */
    struct {
        RK_U32 atf_rdo_skip_atf_wgt20    : 8;
        RK_U32 atf_rdo_skip_atf_wgt21    : 8;
        RK_U32 atf_rdo_skip_atf_wgt22    : 8;
        RK_U32 reserved                       : 8;
    } rdo_skip_atf_wgt1;

    /* 0x0000213c reg2127 */
    struct {
        RK_U32 atf_rdo_skip_atf_wgt30    : 8;
        RK_U32 atf_rdo_skip_atf_wgt31    : 8;
        RK_U32 atf_rdo_skip_atf_wgt32    : 8;
        RK_U32 reserved                       : 8;
    } rdo_skip_atf_wgt2;

    /* 0x00002140 reg2128 */
    struct {
        RK_U32 atf_rdo_skip_atf_wgt40    : 8;
        RK_U32 atf_rdo_skip_atf_wgt41    : 8;
        RK_U32 atf_rdo_skip_atf_wgt42    : 8;
        RK_U32 reserved                       : 8;
    } rdo_skip_atf_wgt3;

    /* 0x00002144 reg2129 */
    struct {
        RK_U32 cu8_rdo_intra_cime_thd0    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_intra_cime_thd1    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_intra_cime_thd0;

    /* 0x00002148 reg2130 */
    struct {
        RK_U32 cu8_rdo_intra_cime_thd2    : 12;
        RK_U32 reserved                   : 20;
    } rdo_b8_intra_cime_thd1;

    /* 0x0000214c reg2131 */
    struct {
        RK_U32 cu8_rdo_intra_var_thd00    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_intra_var_thd01    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_intra_var_thd0;

    /* 0x00002150 reg2132 */
    struct {
        RK_U32 cu8_rdo_intra_var_thd10    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_intra_var_thd11    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_intra_var_thd1;

    /* 0x00002154 reg2133 */
    struct {
        RK_U32 cu8_rdo_intra_var_thd20    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_intra_var_thd21    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_intra_var_thd2;

    /* 0x00002158 reg2134 */
    struct {
        RK_U32 cu8_rdo_intra_var_thd30    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_intra_var_thd31    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_intra_var_thd3;

    /* 0x0000215c reg2135 */
    struct {
        RK_U32 cu8_rdo_intra_atf_wgt00    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt01    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt02    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_intra_atf_wgt0;

    /* 0x00002160 reg2136 */
    struct {
        RK_U32 cu8_rdo_intra_atf_wgt10    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt11    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt12    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_intra_atf_wgt1;

    /* 0x00002164 reg2137 */
    struct {
        RK_U32 cu8_rdo_intra_atf_wgt20    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt21    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt22    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_intra_atf_wgt2;

    /* 0x00002168 reg2138 */
    struct {
        RK_U32 cu8_rdo_intra_atf_wgt30    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt31    : 8;
        RK_U32 cu8_rdo_intra_atf_wgt32    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_intra_atf_wgt3;

    /* 0x0000216c reg2139 */
    struct {
        RK_U32 cu8_rdo_inter_cime_thd0    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_inter_cime_thd1    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_inter_cime_thd0;

    /* 0x00002170 reg2140 */
    struct {
        RK_U32 cu8_rdo_inter_cime_thd2    : 12;
        RK_U32 reserved                   : 20;
    } rdo_b8_inter_cime_thd1;

    /* 0x00002174 reg2141 */
    struct {
        RK_U32 cu8_rdo_inter_var_thd00    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_inter_var_thd01    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_inter_var_thd0;

    /* 0x00002178 reg2142 */
    struct {
        RK_U32 cu8_rdo_inter_var_thd10    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_inter_var_thd11    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_inter_var_thd1;

    /* 0x0000217c reg2143 */
    struct {
        RK_U32 cu8_rdo_inter_var_thd20    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_inter_var_thd21    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_inter_var_thd2;

    /* 0x00002180 reg2144 */
    struct {
        RK_U32 cu8_rdo_inter_var_thd30    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 cu8_rdo_inter_var_thd31    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_b8_inter_var_thd3;

    /* 0x00002184 reg2145 */
    struct {
        RK_U32 cu8_rdo_inter_atf_wgt00    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt01    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt02    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_inter_atf_wgt0;

    /* 0x00002188 reg2146 */
    struct {
        RK_U32 cu8_rdo_inter_atf_wgt10    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt11    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt12    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_inter_atf_wgt1;

    /* 0x0000218c reg2147 */
    struct {
        RK_U32 cu8_rdo_inter_atf_wgt20    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt21    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt22    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_inter_atf_wgt2;

    /* 0x00002190 reg2148 */
    struct {
        RK_U32 cu8_rdo_inter_atf_wgt30    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt31    : 8;
        RK_U32 cu8_rdo_inter_atf_wgt32    : 8;
        RK_U32 reserved                   : 8;
    } rdo_b8_inter_atf_wgt3;

    /* 0x00002194 reg2149 */
    struct {
        RK_U32 cu8_rdo_skip_cime_thd0    : 12;
        RK_U32 reserved                  : 4;
        RK_U32 cu8_rdo_skip_cime_thd1    : 12;
        RK_U32 reserved1                 : 4;
    } rdo_b8_skip_cime_thd0;

    /* 0x00002198 reg2150 */
    struct {
        RK_U32 cu8_rdo_skip_cime_thd2    : 12;
        RK_U32 reserved                  : 4;
        RK_U32 cu8_rdo_skip_cime_thd3    : 12;
        RK_U32 reserved1                 : 4;
    } rdo_b8_skip_cime_thd1;

    /* 0x0000219c reg2151 */
    struct {
        RK_U32 cu8_rdo_skip_var_thd10    : 12;
        RK_U32 reserved                  : 4;
        RK_U32 cu8_rdo_skip_var_thd11    : 12;
        RK_U32 reserved1                 : 4;
    } rdo_b8_skip_var_thd0;

    /* 0x000021a0 reg2152 */
    struct {
        RK_U32 cu8_rdo_skip_var_thd20    : 12;
        RK_U32 reserved                  : 4;
        RK_U32 cu8_rdo_skip_var_thd21    : 12;
        RK_U32 reserved1                 : 4;
    } rdo_b8_skip_var_thd1;

    /* 0x000021a4 reg2153 */
    struct {
        RK_U32 cu8_rdo_skip_var_thd30    : 12;
        RK_U32 reserved                  : 4;
        RK_U32 cu8_rdo_skip_var_thd31    : 12;
        RK_U32 reserved1                 : 4;
    } rdo_b8_skip_var_thd2;

    /* 0x000021a8 reg2154 */
    struct {
        RK_U32 cu8_rdo_skip_var_thd40    : 12;
        RK_U32 reserved                  : 4;
        RK_U32 cu8_rdo_skip_var_thd41    : 12;
        RK_U32 reserved1                 : 4;
    } rdo_b8_skip_var_thd3;

    /* 0x000021ac reg2155 */
    struct {
        RK_U32 cu8_rdo_skip_atf_wgt00    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt10    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt11    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt12    : 8;
    } rdo_b8_skip_atf_wgt0;

    /* 0x000021b0 reg2156 */
    struct {
        RK_U32 cu8_rdo_skip_atf_wgt20    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt21    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt22    : 8;
        RK_U32 reserved                  : 8;
    } rdo_b8_skip_atf_wgt1;

    /* 0x000021b4 reg2157 */
    struct {
        RK_U32 cu8_rdo_skip_atf_wgt30    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt31    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt32    : 8;
        RK_U32 reserved                  : 8;
    } rdo_b8_skip_atf_wgt2;

    /* 0x000021b8 reg2158 */
    struct {
        RK_U32 cu8_rdo_skip_atf_wgt40    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt41    : 8;
        RK_U32 cu8_rdo_skip_atf_wgt42    : 8;
        RK_U32 reserved                  : 8;
    } rdo_b8_skip_atf_wgt3;

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
} Vepu580RdoCfg;

/* class: scaling list  */
/* 0x00002200 reg2200 - 0x00003084 reg3105*/
typedef struct Vepu580SclCfg_t {
    /* 0x00002200 */
    RK_U16  intra8_y[64];
    RK_U16  intra8_u[64];
    RK_U16  intra8_v[64];
    RK_U16  inter8_y[64];
    RK_U16  inter8_u[64];
    RK_U16  inter8_v[64];
    /* 0x00002500 */
    RK_U32  q_iq_16_32[480];
    /* 0x00002c80 */
    RK_U32  q_dc_y16;
    RK_U32  q_dc_u16;
    RK_U32  q_dc_v16;
    RK_U32  q_dc_v32;
    /* 0x00002c90 */
    RK_U32  iq_dc_0;
    RK_U32  iq_dc_1;
    /* 0x00002c98 */
    RK_U32  scal_clk_sel;
} Vepu580SclCfg;

/* class: osd */
/* 0x00003000 reg3072 - 0x00003084 reg3105*/
typedef struct Vepu580Osd_t {
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

    /* 0x00003010 reg3076 - 0x0000304c reg3091*/
    OSD_POS_NEW osd_pos[8];

    /* 0x00003050 reg3092 - 0x0000306c reg3099 */
    RK_U32 osd_addr[8];

    /* 0x3070 - 0x307c */
    RK_U32 reserved3100_3103[4];

    /* 0x03080-0x347c reg3104-reg3360 */
    RK_U32 plt[256];
} Vepu580Osd;

// /* class: osd_plt 255 */
// /* 0x0000347c reg3359 */
// typedef struct Vepu580Section6_t {
//     /* 0x0000347c reg3359 */
//     struct {
//         RK_U32 y        : 8;
//         RK_U32 u        : 8;
//         RK_U32 v        : 8;
//         RK_U32 alpha    : 8;
//     } osd_plt255;
// } vepu580section6;

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

    /* 0x00004200 reg4224 - 0x000042cc reg4275 */
    // b8num_qp0-51
    /*
     * RK_U32 b8num_qp0    : 22;
     * RK_U32 reserved     : 10;
     */
    RK_U32 st_b8_qp[52];
} Vepu580Status;

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
        RK_U32 sh_st_cs    : 4;
        RK_U32 rsd_st_cs        : 4;
        RK_U32 sd_st_cs    : 5;
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
} Vepu580Dbg;

/* class: mmu */
/* 0x0000f000 reg15360 - 0x0000f064 reg15385 */

typedef struct HalVepu580Reg_t {
    Vepu580ControlCfg   reg_ctl;
    Vepu580BaseCfg      reg_base;
    Vepu580RcKlutCfg    reg_rc_klut;
    Vepu580Section3     reg_s3;
    Vepu580RdoCfg       reg_rdo;
    Vepu580SclCfg       reg_scl;
    Vepu580Osd          reg_osd;
    Vepu580Status       reg_st;
    Vepu580Dbg          reg_dbg;
} HalVepu580RegSet;

#endif
