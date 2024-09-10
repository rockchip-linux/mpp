/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H264E_VEPU510_REG_H__
#define __HAL_H264E_VEPU510_REG_H__

#include "rk_type.h"
#include "vepu510_common.h"

/* class: buffer/video syntax */
/* 0x00000270 reg156 - 0x000003f4 reg253*/
typedef struct H264eVepu510Frame_t {

    Vepu510FrmCommon    common;

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
    } tile_pos_hevc;
} H264eVepu510Frame;

/* class: param */
/* 0x00001700 reg1472 - 0x000019cc reg1651 */
typedef struct H264eVepu510Param_t {
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
        RK_U32    qnt_f_bias_i  : 10;
        RK_U32    qnt_f_bias_p  : 10;
        RK_U32    reserve       : 12;
    } qnt_bias_comb;

    /* 0x1734 - 0x173c */
    RK_U32 reserved1485_1487[3];

    /* 0x00001740 reg1488 */
    struct {
        RK_U32    thd0      : 8;
        RK_U32    reserve0  : 8;
        RK_U32    thd1      : 8;
        RK_U32    reserve1  : 8;
    } atr_thd0;

    /* 0x00001744 reg1489 */
    struct {
        RK_U32    thd2      : 8;
        RK_U32    reserve0  : 8;
        RK_U32    thdqp     : 6;
        RK_U32    reserve1  : 10;
    } atr_thd1;

    /* 0x1748 - 0x174c */
    RK_U32 reserved1490_1491[2];

    /* 0x00001750 reg1492 */
    struct {
        RK_U32 atr_lv16_wgt0    : 8;
        RK_U32 atr_lv16_wgt1    : 8;
        RK_U32 atr_lv16_wgt2    : 8;
        RK_U32 reserved         : 8;
    } atr_wgt16;

    /* 0x00001754  reg1493*/
    struct {
        RK_U32 atr_lv8_wgt0    : 8;
        RK_U32 atr_lv8_wgt1    : 8;
        RK_U32 atr_lv8_wgt2    : 8;
        RK_U32 reserved        : 8;
    } atr_wgt8;

    /* 0x00001758 reg1494 */
    struct {
        RK_U32 atr_lv4_wgt0    : 8;
        RK_U32 atr_lv4_wgt1    : 8;
        RK_U32 atr_lv4_wgt2    : 8;
        RK_U32 reserved        : 8;
    } atr_wgt4;

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
} H264eVepu510Param;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x000020fc reg2111 */
typedef struct H264eVepu510SqiCfg_t {
    /* 0x00002000 reg2048 - 0x00002010 reg2052*/
    RK_U32 reserved_2048_2052[5];

    /* 0x00002014 reg2053 */
    struct {
        RK_U32 rdo_smear_lvl16_multi    : 8;
        RK_U32 rdo_smear_dlt_qp         : 4;
        RK_U32 reserved                 : 1;
        RK_U32 stated_mode              : 2;
        RK_U32 rdo_smear_en             : 1;
        RK_U32 reserved1                : 16;
    } smear_opt_cfg;

    /* 0x00002018 reg2054 */
    struct {
        RK_U32 madp_cur_thd0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 madp_cur_thd1    : 12;
        RK_U32 reserved1        : 4;
    } smear_madp_thd0;

    /* 0x0000201c reg2055 */
    struct {
        RK_U32 madp_cur_thd2    : 12;
        RK_U32 reserved         : 4;
        RK_U32 madp_cur_thd3    : 12;
        RK_U32 reserved1        : 4;
    } smear_madp_thd1;

    /* 0x00002020 reg2056 */
    struct {
        RK_U32 madp_around_thd0    : 12;
        RK_U32 reserved            : 4;
        RK_U32 madp_around_thd1    : 12;
        RK_U32 reserved1           : 4;
    } smear_madp_thd2;

    /* 0x00002024 reg2057 */
    struct {
        RK_U32 madp_around_thd2    : 12;
        RK_U32 reserved            : 4;
        RK_U32 madp_around_thd3    : 12;
        RK_U32 reserved1           : 4;
    } smear_madp_thd3;

    /* 0x00002028 reg2058 */
    struct {
        RK_U32 madp_around_thd4    : 12;
        RK_U32 reserved            : 4;
        RK_U32 madp_around_thd5    : 12;
        RK_U32 reserved1           : 4;
    } smear_madp_thd4;

    /* 0x0000202c reg2059 */
    struct {
        RK_U32 madp_ref_thd0    : 12;
        RK_U32 reserved         : 4;
        RK_U32 madp_ref_thd1    : 12;
        RK_U32 reserved1        : 4;
    } smear_madp_thd5;

    /* 0x00002030 reg2060 */
    struct {
        RK_U32 cnt_cur_thd0    : 4;
        RK_U32 reserved        : 4;
        RK_U32 cnt_cur_thd1    : 4;
        RK_U32 reserved1       : 4;
        RK_U32 cnt_cur_thd2    : 4;
        RK_U32 reserved2       : 4;
        RK_U32 cnt_cur_thd3    : 4;
        RK_U32 reserved3       : 4;
    } smear_cnt_thd0;

    /* 0x00002034 reg2061 */
    struct {
        RK_U32 cnt_around_thd0    : 4;
        RK_U32 reserved           : 4;
        RK_U32 cnt_around_thd1    : 4;
        RK_U32 reserved1          : 4;
        RK_U32 cnt_around_thd2    : 4;
        RK_U32 reserved2          : 4;
        RK_U32 cnt_around_thd3    : 4;
        RK_U32 reserved3          : 4;
    } smear_cnt_thd1;

    /* 0x00002038 reg2062 */
    struct {
        RK_U32 cnt_around_thd4    : 4;
        RK_U32 reserved           : 4;
        RK_U32 cnt_around_thd5    : 4;
        RK_U32 reserved1          : 4;
        RK_U32 cnt_around_thd6    : 4;
        RK_U32 reserved2          : 4;
        RK_U32 cnt_around_thd7    : 4;
        RK_U32 reserved3          : 4;
    } smear_cnt_thd2;

    /* 0x0000203c reg2063 */
    struct {
        RK_U32 cnt_ref_thd0    : 4;
        RK_U32 reserved        : 4;
        RK_U32 cnt_ref_thd1    : 4;
        RK_U32 reserved1       : 20;
    } smear_cnt_thd3;

    /* 0x00002040 reg2064 */
    struct {
        RK_U32 resi_small_cur_th0    : 6;
        RK_U32 reserved              : 2;
        RK_U32 resi_big_cur_th0      : 6;
        RK_U32 reserved1             : 2;
        RK_U32 resi_small_cur_th1    : 6;
        RK_U32 reserved2             : 2;
        RK_U32 resi_big_cur_th1      : 6;
        RK_U32 reserved3             : 2;
    } smear_resi_thd0;

    /* 0x00002044 reg2065 */
    struct {
        RK_U32 resi_small_around_th0    : 6;
        RK_U32 reserved                 : 2;
        RK_U32 resi_big_around_th0      : 6;
        RK_U32 reserved1                : 2;
        RK_U32 resi_small_around_th1    : 6;
        RK_U32 reserved2                : 2;
        RK_U32 resi_big_around_th1      : 6;
        RK_U32 reserved3                : 2;
    } smear_resi_thd1;

    /* 0x00002048 reg2066 */
    struct {
        RK_U32 resi_small_around_th2    : 6;
        RK_U32 reserved                 : 2;
        RK_U32 resi_big_around_th2      : 6;
        RK_U32 reserved1                : 2;
        RK_U32 resi_small_around_th3    : 6;
        RK_U32 reserved2                : 2;
        RK_U32 resi_big_around_th3      : 6;
        RK_U32 reserved3                : 2;
    } smear_resi_thd2;

    /* 0x0000204c reg2067 */
    struct {
        RK_U32 resi_small_ref_th0    : 6;
        RK_U32 reserved              : 2;
        RK_U32 resi_big_ref_th0      : 6;
        RK_U32 reserved1             : 18;
    } smear_resi_thd3;

    /* 0x00002050 reg2068 */
    struct {
        RK_U32 resi_th0    : 8;
        RK_U32 reserved    : 8;
        RK_U32 resi_th1    : 8;
        RK_U32 reserved1   : 8;
    } smear_resi_thd4;

    /* 0x00002054 reg2069 */
    struct {
        RK_U32 madp_cnt_th0    : 4;
        RK_U32 madp_cnt_th1    : 4;
        RK_U32 madp_cnt_th2    : 4;
        RK_U32 madp_cnt_th3    : 4;
        RK_U32 reserved        : 16;
    } smear_st_thd;

    /* 0x2058 - 0x206c */
    RK_U32 reserved2070_2075[6];

    /* 0x00002070 reg2076 - 0x0000207c reg2079*/
    rdo_skip_par rdo_b16_skip;

    /* 0x00002080 reg2080 - 0x00002088 reg2082 */
    RK_U32 reserved2080_2082[3];

    /* 0x0000208c reg2083 - 0x00002094 reg2085 */
    rdo_noskip_par rdo_b16_inter;

    /* 0x00002098 reg2086 - 0x000020a4 reg2088 */
    RK_U32 reserved2086_2088[3];

    /* 0x000020a8 reg2089 - 0x000020ac reg2091 */
    rdo_noskip_par rdo_b16_intra;

    /* 0x000020b0 reg2092 */
    RK_U32 reserved2092;

    /* 0x000020b4 reg2093 */
    struct {
        RK_U32 thd0         : 4;
        RK_U32 reserved     : 4;
        RK_U32 thd1         : 4;
        RK_U32 reserved1    : 4;
        RK_U32 thd2         : 4;
        RK_U32 reserved2    : 4;
        RK_U32 thd3         : 4;
        RK_U32 reserved3    : 4;
    } rdo_b16_intra_atf_cnt_thd;

    /* 0x000020b8 reg2094 */
    struct {
        RK_U32 big_th0      : 6;
        RK_U32 reserved     : 2;
        RK_U32 big_th1      : 6;
        RK_U32 reserved1    : 2;
        RK_U32 small_th0    : 6;
        RK_U32 reserved2    : 2;
        RK_U32 small_th1    : 6;
        RK_U32 reserved3    : 2;
    } rdo_atf_resi_thd;
} H264eVepu510Sqi;

/* class: mmu */
/* 0x0000f000 reg15360 - 0x0000f024 reg15369 */
typedef struct HalVepu510Reg_t {
    Vepu510ControlCfg       reg_ctl;
    H264eVepu510Frame       reg_frm;
    Vepu510RcRoi            reg_rc_roi;
    H264eVepu510Param       reg_param;
    H264eVepu510Sqi         reg_sqi;
    Vepu510SclCfg           reg_scl;
    Vepu510Status           reg_st;
    Vepu510Dbg              reg_dbg;
} HalVepu510RegSet;

#endif