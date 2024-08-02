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

#ifndef __VEPU540C_COMMON_H__
#define __VEPU540C_COMMON_H__

#include "rk_venc_cmd.h"
#include "mpp_device.h"

#define VEPU540C_CTL_OFFSET          (0 * sizeof(RK_U32))
#define VEPU540C_BASE_OFFSET         (156 * sizeof(RK_U32))
#define VEPU540C_RCROI_OFFSET        (1024 * sizeof(RK_U32))
#define VEPU540C_WEG_OFFSET          (1472 * sizeof(RK_U32))
#define VEPU540C_RDOCFG_OFFSET       (2048 * sizeof(RK_U32))
#define VEPU540C_SCLCFG_OFFSET       (2168 * sizeof(RK_U32))
#define VEPU540C_JPEGTAB_OFFSET      (2848 * sizeof(RK_U32))

#define VEPU540C_OSD_OFFSET          (3072 * sizeof(RK_U32))
#define VEPU540C_STATUS_OFFSET       (4096 * sizeof(RK_U32))
#define VEPU540C_DEBUG_OFFSET        (5120 * sizeof(RK_U32))
#define VEPU540C_REG_BASE_HW_STATUS  0x2c

#define VEPU540C_MAX_ROI_NUM         8
#define VEPU540C_SLICE_FIFO_LEN      8

typedef struct Vepu540cOnline_t {
    /* 0x00000270 reg156 */
    RK_U32 reg0156_adr_vsy_t;
    /* 0x00000274 reg157 */
    RK_U32 reg0157_adr_vsc_t;
    /* 0x00000278 reg158 */
    RK_U32 reg0158_adr_vsy_b;
    /* 0x0000027c reg159 */
    RK_U32 reg0159_adr_vsc_b;
} vepu540c_online;

typedef struct IpRaw8_t {
    RK_U8 intra_val[8];
    RK_U8 inter_val[8];
} ip_raw8;

typedef struct IpRaw32_16_t {
    RK_U16 q_intra16[4];
    RK_U16 q_intra32[2];
    RK_U8  iq_intra16[4];
} ip_raw32_16;

/* 0x000021e0 reg2168 - 0x00002c7c reg2847*/
typedef struct Vepu540cSclCfg_t {
    /* 0x000021e0 reg2168 */
    RK_U32  q_dc_y16;
    RK_U32  q_dc_u16;
    RK_U32  q_dc_v16;
    RK_U32  q_dc_y32;
    RK_U32  iq_dc_0;
    RK_U32  iq_dc_1;

    /* 0x000021f8 reg2174 */
    struct {
        RK_U32 scal_clk_sel    : 1;
        RK_U32 reserved        : 31;
    } scal_cfg;
    /* 0x21fc */
    RK_U32 reserved_2175;
    /* 0x00002200 reg2176 - 0x0000227c reg2207*/
    RK_U16 q_intra_y8[64];
    /* 0x00002280 reg2208 - 0x000022fc reg2239*/
    RK_U16 q_intra_u8[64];
    /* 0x00002300 reg2240 - 0x0000237c reg2271*/
    RK_U16 q_intra_v8[64];

    /* 0x00002380 reg2272 - 0x000023fc reg2303*/
    RK_U16 q_inter_y8[64];
    /* 0x00002400 reg2304 - 0x0000247c reg2335*/
    RK_U16 q_inter_u8[64];
    /* 0x00002480 reg2336 - 0x000024fc reg2367*/
    RK_U16 q_inter_v8[64];

    /* 0x00002500 reg2368 - 0x000025fc reg2431*/
    ip_raw32_16 q_intra_y[16];
    /* 0x00002600 reg2432 - 0x000026fc reg2495*/
    ip_raw32_16 q_intra_u[16];
    /* 0x00002700 reg2496 - 0x000027fc reg2559 */
    ip_raw32_16 q_intra_v[16];

    /*0x00002800 reg2560 - 0x000028fc reg2623*/
    ip_raw32_16 q_inter_y[16];
    /*0x00002900 reg2624  - 0x000029fc reg2687*/
    ip_raw32_16 q_inter_u[16];
    /*0x00002a00 reg2688 - 0x00002afc reg2751 */
    ip_raw32_16 q_inter_v[16];

    /* 0x00002b00 reg2752 - 0x00002b7c reg2783 */
    ip_raw8  ip_y8[8];
    /* 0x00002b80 reg2784 - 0x00002bfc reg2815 */
    ip_raw8  ip_u8[8];
    /* 0x00002c00 reg2816 - 0x00002c7c reg2847 */
    ip_raw8  ip_v8[8];
} vepu540c_scl_cfg;

typedef struct PreCstPar_t {
    struct {
        RK_U32 madi_thd0    : 8;
        RK_U32 madi_thd1    : 8;
        RK_U32 madi_thd2    : 8;
        RK_U32 madi_thd3    : 8;
    } cst_madi_thd0;

    /* 0x000020c4 reg2097 */
    struct {
        RK_U32 madi_thd4    : 8;
        RK_U32 madi_thd5    : 8;
        RK_U32 reserved     : 16;
    } cst_madi_thd1;

    /* 0x000020c8 reg2098 */
    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } cst_wgt0;

    /* 0x000020cc reg2099 */
    struct {
        RK_U32 wgt4    : 8;
        RK_U32 wgt5    : 8;
        RK_U32 wgt6    : 8;
        RK_U32 wgt7    : 8;
    } cst_wgt1;

    /* 0x000020d0 reg2100 */
    struct {
        RK_U32 wgt8        : 8;
        RK_U32 wgt9        : 8;
        RK_U32 mode_th     : 3;
        RK_U32 reserved    : 13;
    } cst_wgt2;
} pre_cst_par;

typedef struct RdoSkipPar_t {
    struct {
        RK_U32 madp_thd0    : 12;
        RK_U32 reserved     : 4;
        RK_U32 madp_thd1    : 12;
        RK_U32 reserved1    : 4;
    } atf_thd0;

    /* 0x00002064 reg2073 */
    struct {
        RK_U32 madp_thd2    : 12;
        RK_U32 reserved     : 4;
        RK_U32 madp_thd3    : 12;
        RK_U32 reserved1    : 4;
    } atf_thd1;

    /* 0x00002068 reg2074 */
    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } atf_wgt0;

    /* 0x0000206c reg2075 */
    struct {
        RK_U32 wgt4    : 8;
        RK_U32 reserved                  : 24;
    } atf_wgt1;
} rdo_skip_par;

typedef struct RdoNoSkipPar_t {
    /* 0x00002080 reg2080 */
    struct {
        RK_U32 madp_thd0    : 12;
        RK_U32 reserved     : 4;
        RK_U32 madp_thd1    : 12;
        RK_U32 reserved1    : 4;
    } ratf_thd0;

    /* 0x00002084 reg2081 */
    struct {
        RK_U32 madp_thd2    : 12;
        RK_U32 reserved     : 20;
    } ratf_thd1;

    /* 0x00002088 reg2082 */
    struct {
        RK_U32 wgt0    : 8;
        RK_U32 wgt1    : 8;
        RK_U32 wgt2    : 8;
        RK_U32 wgt3    : 8;
    } atf_wgt;
} rdo_noskip_par;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x00002c7c reg2847*/
typedef struct Vepu540cRdoCfg_t {

    /* 0x2000 - 0x200c */
    RK_U32 reserved2048_2051[4];

    /* 0x00002010 reg2052 */
    struct {
        RK_U32 rdo_segment_multi       : 8;
        RK_U32 rdo_segment_en          : 1;
        RK_U32 reserved                : 7;
        RK_U32 rdo_smear_lvl4_multi    : 8;
        RK_U32 rdo_smear_lvl8_multi    : 8;
    } rdo_segment_cfg;

    /* 0x00002014 reg2053 */
    struct {
        RK_U32 rdo_smear_lvl16_multi    : 8;
        RK_U32 rdo_smear_dlt_qp         : 4;
        RK_U32 rdo_smear_order_state    : 1;
        RK_U32 stated_mode              : 2;
        RK_U32 rdo_smear_en             : 1;
        RK_U32 online_en                : 1;
        RK_U32 reserved                 : 3;
        RK_U32 smear_stride             : 12;
    } rdo_smear_cfg_comb;

    /* 0x00002018 reg2054 */
    struct {
        RK_U32 rdo_smear_madp_cur_thd0    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 rdo_smear_madp_cur_thd1    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_smear_madp_thd0_comb;

    /* 0x0000201c reg2055 */
    struct {
        RK_U32 rdo_smear_madp_cur_thd2    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 rdo_smear_madp_cur_thd3    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_smear_madp_thd1_comb;

    /* 0x00002020 reg2056 */
    struct {
        RK_U32 rdo_smear_madp_around_thd0    : 12;
        RK_U32 reserved                      : 4;
        RK_U32 rdo_smear_madp_around_thd1    : 12;
        RK_U32 reserved1                     : 4;
    } rdo_smear_madp_thd2_comb;

    /* 0x00002024 reg2057 */
    struct {
        RK_U32 rdo_smear_madp_around_thd2    : 12;
        RK_U32 reserved                      : 4;
        RK_U32 rdo_smear_madp_around_thd3    : 12;
        RK_U32 reserved1                     : 4;
    } rdo_smear_madp_thd3_comb;

    /* 0x00002028 reg2058 */
    struct {
        RK_U32 rdo_smear_madp_around_thd4    : 12;
        RK_U32 reserved                      : 4;
        RK_U32 rdo_smear_madp_around_thd5    : 12;
        RK_U32 reserved1                     : 4;
    } rdo_smear_madp_thd4_comb;

    /* 0x0000202c reg2059 */
    struct {
        RK_U32 rdo_smear_madp_ref_thd0    : 12;
        RK_U32 reserved                   : 4;
        RK_U32 rdo_smear_madp_ref_thd1    : 12;
        RK_U32 reserved1                  : 4;
    } rdo_smear_madp_thd5_comb;

    /* 0x00002030 reg2060 */
    struct {
        RK_U32 rdo_smear_cnt_cur_thd0    : 4;
        RK_U32 reserved                  : 4;
        RK_U32 rdo_smear_cnt_cur_thd1    : 4;
        RK_U32 reserved1                 : 4;
        RK_U32 rdo_smear_cnt_cur_thd2    : 4;
        RK_U32 reserved2                 : 4;
        RK_U32 rdo_smear_cnt_cur_thd3    : 4;
        RK_U32 reserved3                 : 4;
    } rdo_smear_cnt_thd0_comb;

    /* 0x00002034 reg2061 */
    struct {
        RK_U32 rdo_smear_cnt_around_thd0    : 4;
        RK_U32 reserved                     : 4;
        RK_U32 rdo_smear_cnt_around_thd1    : 4;
        RK_U32 reserved1                    : 4;
        RK_U32 rdo_smear_cnt_around_thd2    : 4;
        RK_U32 reserved2                    : 4;
        RK_U32 rdo_smear_cnt_around_thd3    : 4;
        RK_U32 reserved3                    : 4;
    } rdo_smear_cnt_thd1_comb;

    /* 0x00002038 reg2062 */
    struct {
        RK_U32 rdo_smear_cnt_around_thd4    : 4;
        RK_U32 reserved                     : 4;
        RK_U32 rdo_smear_cnt_around_thd5    : 4;
        RK_U32 reserved1                    : 4;
        RK_U32 rdo_smear_cnt_around_thd6    : 4;
        RK_U32 reserved2                    : 4;
        RK_U32 rdo_smear_cnt_around_thd7    : 4;
        RK_U32 reserved3                    : 4;
    } rdo_smear_cnt_thd2_comb;

    /* 0x0000203c reg2063 */
    struct {
        RK_U32 rdo_smear_cnt_ref_thd0    : 4;
        RK_U32 reserved                  : 4;
        RK_U32 rdo_smear_cnt_ref_thd1    : 4;
        RK_U32 reserved1                 : 20;
    } rdo_smear_cnt_thd3_comb;

    /* 0x00002040 reg2064 */
    struct {
        RK_U32 rdo_smear_resi_small_cur_th0    : 6;
        RK_U32 reserved                        : 2;
        RK_U32 rdo_smear_resi_big_cur_th0      : 6;
        RK_U32 reserved1                       : 2;
        RK_U32 rdo_smear_resi_small_cur_th1    : 6;
        RK_U32 reserved2                       : 2;
        RK_U32 rdo_smear_resi_big_cur_th1      : 6;
        RK_U32 reserved3                       : 2;
    } rdo_smear_resi_thd0_comb;

    /* 0x00002044 reg2065 */
    struct {
        RK_U32 rdo_smear_resi_small_around_th0    : 6;
        RK_U32 reserved                           : 2;
        RK_U32 rdo_smear_resi_big_around_th0      : 6;
        RK_U32 reserved1                          : 2;
        RK_U32 rdo_smear_resi_small_around_th1    : 6;
        RK_U32 reserved2                          : 2;
        RK_U32 rdo_smear_resi_big_around_th1      : 6;
        RK_U32 reserved3                          : 2;
    } rdo_smear_resi_thd1_comb;

    /* 0x00002048 reg2066 */
    struct {
        RK_U32 rdo_smear_resi_small_around_th2    : 6;
        RK_U32 reserved                           : 2;
        RK_U32 rdo_smear_resi_big_around_th2      : 6;
        RK_U32 reserved1                          : 2;
        RK_U32 rdo_smear_resi_small_around_th3    : 6;
        RK_U32 reserved2                          : 2;
        RK_U32 rdo_smear_resi_big_around_th3      : 6;
        RK_U32 reserved3                          : 2;
    } rdo_smear_resi_thd2_comb;

    /* 0x0000204c reg2067 */
    struct {
        RK_U32 rdo_smear_resi_small_ref_th0    : 6;
        RK_U32 reserved                        : 2;
        RK_U32 rdo_smear_resi_big_ref_th0      : 6;
        RK_U32 reserved1                       : 18;
    } rdo_smear_resi_thd3_comb;

    /* 0x00002050 reg2068 */
    struct {
        RK_U32 rdo_smear_resi_th0    : 8;
        RK_U32 reserved              : 8;
        RK_U32 rdo_smear_resi_th1    : 8;
        RK_U32 reserved1             : 8;
    } rdo_smear_st_thd0_comb;

    /* 0x00002054 reg2069 */
    struct {
        RK_U32 rdo_smear_madp_cnt_th0    : 4;
        RK_U32 rdo_smear_madp_cnt_th1    : 4;
        RK_U32 rdo_smear_madp_cnt_th2    : 4;
        RK_U32 rdo_smear_madp_cnt_th3    : 4;
        RK_U32 rdo_smear_madp_cnt_th4    : 4;
        RK_U32 rdo_smear_madp_cnt_th5    : 4;
        RK_U32 reserved                  : 8;
    } rdo_smear_st_thd1_comb;

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

    /* 0x00002098 reg2086 - 0x000020a4 reg2089 */
    rdo_noskip_par rdo_b32_intra;

    /* 0x000020a8 reg2090 - 0x000020ac reg2091 */
    rdo_noskip_par rdo_b16_intra;

    /* 0x000020b0 reg2092 */
    struct {
        RK_U32 thd0    : 6;
        RK_U32 reserved                       : 2;
        RK_U32 thd1    : 6;
        RK_U32 reserved1                      : 2;
        RK_U32 thd2    : 6;
        RK_U32 reserved2                      : 2;
        RK_U32 thd3    : 6;
        RK_U32 reserved3                      : 2;
    } rdo_b32_intra_atf_cnt_thd;

    /* 0x000020b4 reg2093 */
    struct {
        RK_U32 thd0    : 4;
        RK_U32 reserved                       : 4;
        RK_U32 thd1    : 4;
        RK_U32 reserved1                      : 4;
        RK_U32 thd2    : 4;
        RK_U32 reserved2                      : 4;
        RK_U32 thd3    : 4;
        RK_U32 reserved3                      : 4;
    } rdo_b16_intra_atf_cnt_thd_comb;

    /* 0x000020b8 reg2094 */
    struct {
        RK_U32 big_th0      : 6;
        RK_U32 reserved                  : 2;
        RK_U32 big_th1      : 6;
        RK_U32 reserved1                 : 2;
        RK_U32 small_th0    : 6;
        RK_U32 reserved2                 : 2;
        RK_U32 small_th1    : 6;
        RK_U32 reserved3                 : 2;
    } rdo_atf_resi_thd_comb;

    /* 0x20bc */
    RK_U32 reserved_2095;

    /* 0x000020c0 reg2096 - 0x000020d0 reg2100 */
    pre_cst_par preintra32_cst;
    /* 0x000020d4 reg2101 - 0x000020e4 reg2105 */
    pre_cst_par preintra16_cst;

    /* 0x20e8 - 0x20ec */
    RK_U32 reserved2106_2107[2];

    /* 0x000020f0 reg2108 */
    struct {
        RK_U32 pre_intra_qp_thd             : 6;
        RK_U32 reserved                     : 2;
        RK_U32 pre_intra4_lambda_mv_bit     : 3;
        RK_U32 reserved1                    : 1;
        RK_U32 pre_intra8_lambda_mv_bit     : 3;
        RK_U32 reserved2                    : 1;
        RK_U32 pre_intra16_lambda_mv_bit    : 3;
        RK_U32 reserved3                    : 1;
        RK_U32 pre_intra32_lambda_mv_bit    : 3;
        RK_U32 reserved4                    : 9;
    } preintra_sqi_cfg;

    /* 0x000020f4 reg2109 */
    struct {
        RK_U32 i_cu32_madi_thd0    : 8;
        RK_U32 i_cu32_madi_thd1    : 8;
        RK_U32 i_cu32_madi_thd2    : 8;
        RK_U32 reserved            : 8;
    } rdo_atr_i_cu32_madi_cfg0;

    /* 0x000020f8 reg2110 */
    struct {
        RK_U32 i_cu32_madi_cnt_thd3      : 5;
        RK_U32 reserved                  : 3;
        RK_U32 i_cu32_madi_thd4          : 8;
        RK_U32 i_cu32_madi_cost_multi    : 8;
        RK_U32 reserved1                 : 8;
    } rdo_atr_i_cu32_madi_cfg1;

    /* 0x000020fc reg2111 */
    struct {
        RK_U32 i_cu16_madi_thd0          : 8;
        RK_U32 i_cu16_madi_thd1          : 8;
        RK_U32 i_cu16_madi_cost_multi    : 8;
        RK_U32 reserved                  : 8;
    } rdo_atr_i_cu16_madi_cfg0;
} vepu540c_rdo_cfg;

typedef struct Vepu540cJpegReg_t {

    RK_U32  reg0256_adr_bsbt;

    /* 0x00000404 reg257 */
    RK_U32  reg0257_adr_bsbb;

    /* 0x00000408 reg258 */
    RK_U32 reg0258_adr_bsbs;

    /* 0x0000040c reg259 */
    RK_U32 reg0259_adr_bsbr;

    /* 0x00000410 reg260 */
    RK_U32 reg0260_adr_vsy_b;

    /* 0x00000414 reg261 */
    RK_U32 reg0261_adr_vsc_b;

    /* 0x00000418 reg262 */
    RK_U32 reg0262_adr_vsy_t;

    /* 0x0000041c reg263 */
    RK_U32 reg0263_adr_vsc_t;

    /* 0x00000420 reg264 */
    RK_U32 reg0264_adr_src0;

    /* 0x00000424 reg265 */
    RK_U32 reg0265_adr_src1;

    /* 0x00000428 reg266 */
    RK_U32 reg0266_adr_src2;

    /* 0x0000042c reg267 */
    struct {
        RK_U32 bsp_size_jpeg    : 1;
        RK_U32 reserved         : 31;
    } bsp_size_jpeg;

    /* 0x430 - 0x43c */
    RK_U32 reserved268_271[4];

    /* 0x00000440 reg272 */
    struct {
        RK_U32 pic_wd8_m1         : 11;
        RK_U32 reserved           : 5;
        RK_U32 pic_hd8_m1         : 11;
        RK_U32 reserved1          : 5;
    } reg0272_enc_rsl;

    /* 0x00000444 reg273 */
    struct {
        RK_U32 pic_wfill         : 6;
        RK_U32 reserved          : 10;
        RK_U32 pic_hfill         : 6;
        RK_U32 reserved1         : 10;
    } reg0273_src_fill;

    /* 0x00000448 reg274 */
    struct {
        RK_U32 alpha_swap            : 1;
        RK_U32 rbuv_swap             : 1;
        RK_U32 src_cfmt              : 4;
        RK_U32 reserved              : 1;
        RK_U32 out_fmt               : 1;
        RK_U32 src_range_trns_en     : 1;
        RK_U32 src_range_trns_sel    : 1;
        RK_U32 chroma_ds_mode        : 1;
        RK_U32 reserved1             : 21;
    } reg0274_src_fmt;

    /* 0x0000044c reg275 */
    struct {
        RK_U32 csc_wgt_b2y    : 9;
        RK_U32 csc_wgt_g2y    : 9;
        RK_U32 csc_wgt_r2y    : 9;
        RK_U32 reserved            : 5;
    } reg0275_src_udfy;

    /* 0x00000450 reg276 */
    struct {
        RK_U32 csc_wgt_b2u    : 9;
        RK_U32 csc_wgt_g2u    : 9;
        RK_U32 csc_wgt_r2u    : 9;
        RK_U32 reserved            : 5;
    } reg0276_src_udfu;

    /* 0x00000454 reg277 */
    struct {
        RK_U32 csc_wgt_b2v    : 9;
        RK_U32 csc_wgt_g2v    : 9;
        RK_U32 csc_wgt_r2v    : 9;
        RK_U32 reserved            : 5;
    } reg0277_src_udfv;

    /* 0x00000458 reg278 */
    struct {
        RK_U32 csc_ofst_v    : 8;
        RK_U32 csc_ofst_u    : 8;
        RK_U32 csc_ofst_y    : 5;
        RK_U32 reserved           : 11;
    } reg0278_src_udfo;

    /* 0x0000045c reg279 */
    struct {
        RK_U32 reserved         : 26;
        RK_U32 src_mirr    : 1;
        RK_U32 src_rot     : 2;
        RK_U32 reserved1        : 3;
    } reg0279_src_proc;

    /* 0x00000460 reg280 */
    struct {
        RK_U32 pic_ofst_x    : 14;
        RK_U32 reserved           : 2;
        RK_U32 pic_ofst_y    : 14;
        RK_U32 reserved1          : 2;
    } reg0280_pic_ofst;

    /* 0x00000464 reg281 */
    struct {
        RK_U32 src_strd0    : 17;
        RK_U32 reserved          : 15;
    } reg0281_src_strd0;

    /* 0x00000468 reg282 */
    struct {
        RK_U32 src_strd1    : 16;
        RK_U32 reserved          : 16;
    } reg0282_src_strd1;

    /* 0x0000046c reg283 */
    struct {
        RK_U32 pp_corner_filter_strength      : 2;
        RK_U32 reserved                            : 2;
        RK_U32 pp_edge_filter_strength        : 2;
        RK_U32 reserved1                           : 2;
        RK_U32 pp_internal_filter_strength    : 2;
        RK_U32 reserved2                           : 22;
    } reg0283_src_flt;

    /* 0x00000470 reg284 */
    struct {
        RK_U32 bias_y    : 15;
        RK_U32 reserved       : 17;
    } reg0284_y_cfg;

    /* 0x00000474 reg285 */
    struct {
        RK_U32 bias_u    : 15;
        RK_U32 reserved       : 17;
    } reg0285_u_cfg;

    /* 0x00000478 reg286 */
    struct {
        RK_U32 bias_v    : 15;
        RK_U32 reserved       : 17;
    } reg0286_v_cfg;

    /* 0x0000047c reg287 */
    struct {
        RK_U32 jpeg_ri              : 25;
        RK_U32 jpeg_out_mode        : 1;
        RK_U32 jpeg_start_rst_m     : 3;
        RK_U32 jpeg_pic_last_ecs    : 1;
        RK_U32 jpeg_slen_fifo       : 1;
        RK_U32 jpeg_stnd            : 1;
    } reg0287_base_cfg;

    /* 0x00000480 reg288 */
    struct {
        RK_U32 uvc_partition0_len    : 12;
        RK_U32 uvc_partition_len     : 12;
        RK_U32 uvc_skip_len          : 6;
        RK_U32 reserved              : 2;
    } reg0288_uvc_cfg;
} Vepu540cJpegReg;

/* 0x00002c80 reg2848 - - 0x00002dfc reg2943*/
typedef struct Vepu540cJpegTab_t {
    /* 0x00002c80 reg2848 - 0x00002cfc reg2879 */
    RK_U16 qua_tab0[64];
    /*  0x00002d00 reg2880 -  0x00002d7c reg2911 */
    RK_U16 qua_tab1[64];
    /* 0x00002d80 reg2912 - 0x00002dfc reg2943 */
    RK_U16 qua_tab2[64];
} vepu540c_jpeg_tab;

typedef struct Vepu540cRoiRegion_t {

    struct {
        RK_U32 roi_lt_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 roi_lt_y    : 10;
        RK_U32 reserved1    : 6;
    } roi_pos_lt;

    struct {
        RK_U32 roi_rb_x    : 10;
        RK_U32 reserved     : 6;
        RK_U32 roi_rb_y    : 10;
        RK_U32 reserved1    : 6;
    } roi_pos_rb;

    struct {
        RK_U32 roi_qp_value       : 7;
        RK_U32 roi_qp_adj_mode    : 1;
        RK_U32 roi_pri            : 5;
        RK_U32 roi_en             : 1;
        RK_U32 reserved           : 18;
    } roi_base;
    struct {
        RK_U32 roi_mdc_inter16         : 4;
        RK_U32 roi_mdc_skip16          : 4;
        RK_U32 roi_mdc_intra16         : 4;
        RK_U32 roi0_mdc_inter32_hevc   : 4;
        RK_U32 roi0_mdc_skip32_hevc    : 4;
        RK_U32 roi0_mdc_intra32_hevc   : 4;
        RK_U32 roi0_mdc_dpth_hevc      : 1;
        RK_U32 reserved                : 7;
    } roi_mdc;
} Vepu540cRoiRegion;

/*
 * Vepu540cRoiCfg
 *
 */
typedef struct Vepu540cRoiCfg_t {
    struct {
        RK_U32 fmdc_adju_inter16         : 4;
        RK_U32 fmdc_adju_skip16          : 4;
        RK_U32 fmdc_adju_intra16         : 4;
        RK_U32 fmdc_adju_inter32         : 4;
        RK_U32 fmdc_adju_skip32          : 4;
        RK_U32 fmdc_adju_intra32         : 4;
        RK_U32 fmdc_adj_pri              : 5;
        RK_U32 reserved                  : 3;
    } fmdc_adj0;

    /* 0x00001084 reg1057 */
    struct {
        RK_U32 fmdc_adju_inter8         : 4;
        RK_U32 fmdc_adju_skip8          : 4;
        RK_U32 fmdc_adju_intra8         : 4;
        RK_U32 reserved                 : 20;
    } fmdc_adj1;

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

    Vepu540cRoiRegion regions[8];
} Vepu540cRoiCfg;

/* class: st */
/* 0x00004000 reg4096 - 0x000042cc reg4275 */
typedef struct Vepu540cStatus_t {
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
        RK_U32 sli_num_video     : 6;
        RK_U32 sli_num_jpeg      : 6;
        RK_U32 reserved          : 4;
        RK_U32 bpkt_num_video    : 7;
        RK_U32 bpkt_lst_video    : 1;
        RK_U32 bpkt_num_jpeg     : 7;
        RK_U32 bpkt_lst_jpeg     : 1;
    } st_snum;

    /* 0x00004038 reg4110 */
    RK_U32 sli_len;

    /* 0x403c - 0x40fc */
    struct {
        RK_U32 task_id_proc     : 12;
        RK_U32 task_id_done     : 12;
        RK_U32 task_done        : 1;
        RK_U32 task_lkt_err     : 3;
        RK_U32 dvbm_enc_err     : 1;
        RK_U32 dvbm_fmch_err    : 1;
        RK_U32 reserved         : 1;
        RK_U32 rfpr_ovrb_err    : 1;
    } st_link_task;

    /* 0x4040 - 0x405c */
    RK_U32 reserved4111_4119[8];

    RK_U32 sli_len_jpeg;

    /* 0x00004064 reg4121 */
    RK_U32 jpeg_head_bits_l32;

    /* 0x00004068 reg4122 */
    struct {
        RK_U32 jpeg_head_bits_h8    : 1;
        RK_U32 reserved             : 31;
    } st_bsl_h8_jpeg;

    /* 0x0000406c reg4123 */
    struct {
        RK_U32 jbsbw_ovfl    : 1;
        RK_U32 reserved      : 2;
        RK_U32 jbsbw_addr    : 28;
        RK_U32 reserved1     : 1;
    } st_jbsb;

    /* 0x4070 - 0x407c */
    RK_U32 reserved4124_4127[4];

    /* 0x00004080 reg4128 */
    struct {
        RK_U32 pnum_p64    : 17;
        RK_U32 reserved    : 15;
    } st_pnum_p64;

    /* 0x00004084 reg4129 */
    struct {
        RK_U32 pnum_p32    : 19;
        RK_U32 reserved    : 13;
    } st_pnum_p32;

    /* 0x00004088 reg4130 */
    struct {
        RK_U32 pnum_p16    : 21;
        RK_U32 reserved    : 11;
    } st_pnum_p16;

    /* 0x0000408c reg4131 */
    struct {
        RK_U32 pnum_p8     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_p8;

    /* 0x00004090 reg4132 */
    struct {
        RK_U32 pnum_i32    : 19;
        RK_U32 reserved    : 13;
    } st_pnum_i32;

    /* 0x00004094 reg4133 */
    struct {
        RK_U32 pnum_i16    : 21;
        RK_U32 reserved    : 11;
    } st_pnum_i16;

    /* 0x00004098 reg4134 */
    struct {
        RK_U32 pnum_i8     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_i8;

    /* 0x0000409c reg4135 */
    struct {
        RK_U32 pnum_i4     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_i4;

    /* 0x000040a0 reg4136 */
    struct {
        RK_U32 num_b16     : 23;
        RK_U32 reserved    : 9;
    } st_bnum_b16;

    /* 0x000040a4 reg4137 */
    struct {
        RK_U32 rdo_smear_cnt0    : 8;
        RK_U32 rdo_smear_cnt1    : 8;
        RK_U32 rdo_smear_cnt2    : 8;
        RK_U32 rdo_smear_cnt3    : 8;
    } st_smear_cnt;

    /* 0x000040a8 reg4138 */
    RK_U32 madi_sum;

    /* 0x40ac - 0x40bc */
    RK_U32 reserved4139_4143[5];

    /* 0x000040c0 reg4144 */
    struct {
        RK_U32 madi_th_lt_cnt0    : 16;
        RK_U32 madi_th_lt_cnt1    : 16;
    } st_madi_lt_num0;

    /* 0x000040c4 reg4145 */
    struct {
        RK_U32 madi_th_lt_cnt2    : 16;
        RK_U32 madi_th_lt_cnt3    : 16;
    } st_madi_lt_num1;

    /* 0x000040c8 reg4146 */
    struct {
        RK_U32 madi_th_rt_cnt0    : 16;
        RK_U32 madi_th_rt_cnt1    : 16;
    } st_madi_rt_num0;

    /* 0x000040cc reg4147 */
    struct {
        RK_U32 madi_th_rt_cnt2    : 16;
        RK_U32 madi_th_rt_cnt3    : 16;
    } st_madi_rt_num1;

    /* 0x000040d0 reg4148 */
    struct {
        RK_U32 madi_th_lb_cnt0    : 16;
        RK_U32 madi_th_lb_cnt1    : 16;
    } st_madi_lb_num0;

    /* 0x000040d4 reg4149 */
    struct {
        RK_U32 madi_th_lb_cnt2    : 16;
        RK_U32 madi_th_lb_cnt3    : 16;
    } st_madi_lb_num1;

    /* 0x000040d8 reg4150 */
    struct {
        RK_U32 madi_th_rb_cnt0    : 16;
        RK_U32 madi_th_rb_cnt1    : 16;
    } st_madi_rb_num0;

    /* 0x000040dc reg4151 */
    struct {
        RK_U32 madi_th_rb_cnt2    : 16;
        RK_U32 madi_th_rb_cnt3    : 16;
    } st_madi_rb_num1;

    /* 0x000040e0 reg4152 */
    struct {
        RK_U32 madp_th_lt_cnt0    : 16;
        RK_U32 madp_th_lt_cnt1    : 16;
    } st_madp_lt_num0;

    /* 0x000040e4 reg4153 */
    struct {
        RK_U32 madp_th_lt_cnt2    : 16;
        RK_U32 madp_th_lt_cnt3    : 16;
    } st_madp_lt_num1;

    /* 0x000040e8 reg4154 */
    struct {
        RK_U32 madp_th_rt_cnt0    : 16;
        RK_U32 madp_th_rt_cnt1    : 16;
    } st_madp_rt_num0;

    /* 0x000040ec reg4155 */
    struct {
        RK_U32 madp_th_rt_cnt2    : 16;
        RK_U32 madp_th_rt_cnt3    : 16;
    } st_madp_rt_num1;

    /* 0x000040f0 reg4156 */
    struct {
        RK_U32 madp_th_lb_cnt0    : 16;
        RK_U32 madp_th_lb_cnt1    : 16;
    } st_madp_lb_num0;

    /* 0x000040f4 reg4157 */
    struct {
        RK_U32 madp_th_lb_cnt2    : 16;
        RK_U32 madp_th_lb_cnt3    : 16;
    } st_madp_lb_num1;

    /* 0x000040f8 reg4158 */
    struct {
        RK_U32 madp_th_rb_cnt0    : 16;
        RK_U32 madp_th_rb_cnt1    : 16;
    } st_madp_rb_num0;

    /* 0x000040fc reg4159 */
    struct {
        RK_U32 madp_th_rb_cnt2    : 16;
        RK_U32 madp_th_rb_cnt3    : 16;
    } st_madp_rb_num1;

    /* 0x00004100 reg4160 */
    struct {
        RK_U32 cmv_th_lt_cnt0    : 16;
        RK_U32 cmv_th_lt_cnt1    : 16;
    } st_cmv_lt_num0;

    /* 0x00004104 reg4161 */
    struct {
        RK_U32 cmv_th_lt_cnt2    : 16;
        RK_U32 cmv_th_lt_cnt3    : 16;
    } st_cmv_lt_num1;

    /* 0x00004108 reg4162 */
    struct {
        RK_U32 cmv_th_rt_cnt0    : 16;
        RK_U32 cmv_th_rt_cnt1    : 16;
    } st_cmv_rt_num0;

    /* 0x0000410c reg4163 */
    struct {
        RK_U32 cmv_th_rt_cnt2    : 16;
        RK_U32 cmv_th_rt_cnt3    : 16;
    } st_cmv_rt_num1;

    /* 0x00004110 reg4164 */
    struct {
        RK_U32 cmv_th_lb_cnt0    : 16;
        RK_U32 cmv_th_lb_cnt1    : 16;
    } st_cmv_lb_num0;

    /* 0x00004114 reg4165 */
    struct {
        RK_U32 cmv_th_lb_cnt2    : 16;
        RK_U32 cmv_th_lb_cnt3    : 16;
    } st_cmv_lb_num1;

    /* 0x00004118 reg4166 */
    struct {
        RK_U32 cmv_th_rb_cnt0    : 16;
        RK_U32 cmv_th_rb_cnt1    : 16;
    } st_cmv_rb_num0;

    /* 0x0000411c reg4167 */
    struct {
        RK_U32 cmv_th_rb_cnt2    : 16;
        RK_U32 cmv_th_rb_cnt3    : 16;
    } st_cmv_rb_num1;

    /* 0x00004120 reg4168 */
    struct {
        RK_U32 org_y_r_max_value    : 8;
        RK_U32 org_y_r_min_value    : 8;
        RK_U32 org_u_g_max_value    : 8;
        RK_U32 org_u_g_min_value    : 8;
    } st_vsp_org_value0;

    /* 0x00004124 reg4169 */
    struct {
        RK_U32 org_v_b_max_value    : 8;
        RK_U32 org_v_b_min_value    : 8;
        RK_U32 reserved             : 16;
    } st_vsp_org_value1;

    /* 0x00004128 reg4170 */
    struct {
        RK_U32 jpeg_y_r_max_value    : 8;
        RK_U32 jpeg_y_r_min_value    : 8;
        RK_U32 jpeg_u_g_max_value    : 8;
        RK_U32 jpeg_u_g_min_value    : 8;
    } st_vsp_jpeg_value0;

    /* 0x0000412c reg4171 */
    struct {
        RK_U32 jpeg_v_b_max_value    : 8;
        RK_U32 jpeg_v_b_min_value    : 8;
        RK_U32 reserved              : 16;
    } st_vsp_jpeg_value1;

    /* 0x4130 - 0x417c */
    RK_U32 reserved4172_4191[20];

    /* 0x00004180 reg4192 - 0x0000424c reg4243*/
    RK_U32 st_b8_qp[52];

} vepu540c_status;

/* class: dbg/st/axipn */
/* 0x00005000 reg5120 - 0x00005354 reg5333*/
typedef struct Vepu540cDbg_t {
    struct {
        RK_U32 pp0_tout     : 1;
        RK_U32 pp1_out      : 1;
        RK_U32 cme_tout     : 1;
        RK_U32 swn_tout     : 1;
        RK_U32 rfme_tout    : 1;
        RK_U32 pren_tout    : 1;
        RK_U32 rdo_tout     : 1;
        RK_U32 lpf_tout     : 1;
        RK_U32 etpy_tout    : 1;
        RK_U32 jpeg_tout    : 1;
        RK_U32 frm_tout     : 1;
        RK_U32 reserved     : 21;
    } st_wdg;

    /* 0x00005004 reg5121 */
    struct {
        RK_U32 pp0_wrk     : 1;
        RK_U32 pp1_wrk     : 1;
        RK_U32 cme_wrk     : 1;
        RK_U32 swn_wrk     : 1;
        RK_U32 rfme_wrk    : 1;
        RK_U32 pren_wrk    : 1;
        RK_U32 rdo_wrk     : 1;
        RK_U32 lpf_wrk     : 1;
        RK_U32 etpy_wrk    : 1;
        RK_U32 jpeg_wrk    : 1;
        RK_U32 frm_wrk     : 1;
        RK_U32 reserved    : 21;
    } st_ppl;

    /* 0x00005008 reg5122 */
    struct {
        RK_U32 vsp0_pos_x    : 16;
        RK_U32 vsp0_pos_y    : 16;
    } st_ppl_pos_vsp0;

    /* 0x0000500c reg5123 */
    struct {
        RK_U32 vsp1_pos_x    : 16;
        RK_U32 vsp1_pos_y    : 16;
    } st_ppl_pos_vsp1;

    /* 0x00005010 reg5124 */
    struct {
        RK_U32 cme_pos_x    : 16;
        RK_U32 cme_pos_y    : 16;
    } st_ppl_pos_cme;

    /* 0x00005014 reg5125 */
    struct {
        RK_U32 swin_pos_x    : 16;
        RK_U32 swin_pos_y    : 16;
    } st_ppl_pos_swin;

    /* 0x00005018 reg5126 */
    struct {
        RK_U32 rfme_pos_x    : 16;
        RK_U32 rfme_pos_y    : 16;
    } st_ppl_pos_rfme;

    /* 0x0000501c reg5127 */
    struct {
        RK_U32 pren_pos_x    : 16;
        RK_U32 pren_pos_y    : 16;
    } st_ppl_pos_pren;

    /* 0x00005020 reg5128 */
    struct {
        RK_U32 rdo_pos_x    : 16;
        RK_U32 rdo_pos_y    : 16;
    } st_ppl_pos_rdo;

    /* 0x00005024 reg5129 */
    struct {
        RK_U32 lpf_pos_x    : 16;
        RK_U32 lpf_pos_y    : 16;
    } st_ppl_pos_lpf;

    /* 0x00005028 reg5130 */
    struct {
        RK_U32 etpy_pos_x    : 16;
        RK_U32 etpy_pos_y    : 16;
    } st_ppl_pos_etpy;

    /* 0x0000502c reg5131 */
    struct {
        RK_U32 vsp0_pos_x    : 16;
        RK_U32 vsp0_pos_y    : 16;
    } st_ppl_pos_jsp0;

    /* 0x00005030 reg5132 */
    struct {
        RK_U32 vsp1_pos_x    : 16;
        RK_U32 vsp1_pos_y    : 16;
    } st_ppl_pos_jsp1;

    /* 0x00005034 reg5133 */
    struct {
        RK_U32 jpeg_pos_x    : 16;
        RK_U32 jpeg_pos_y    : 16;
    } st_ppl_pos_jpeg;

    /* 0x5038 - 0x503c */
    RK_U32 reserved5134_5135[2];

    /* 0x00005040 reg5136 */
    struct {
        RK_U32 sli_num     : 15;
        RK_U32 reserved    : 17;
    } st_sli_num;

    /* 0x5044 - 0x50fc */
    RK_U32 reserved5137_5183[47];

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
        RK_U32 r_ena_lambd        : 1;
        RK_U32 r_fst_swinw_end    : 1;
        RK_U32 r_swinw_end        : 1;
        RK_U32 r_cnt_swinw        : 1;

        RK_U32 r_dspw_end         : 1;
        RK_U32 r_dspw_cnt         : 1;
        RK_U32 i_sjgen_work       : 1;
        RK_U32 r_end_rspgen       : 1;

        RK_U32 r_cost_gate        : 1;
        RK_U32 r_ds_gate          : 1;
        RK_U32 r_mvp_gate         : 1;
        RK_U32 i_smvp_arrdy       : 1;

        RK_U32 i_smvp_arvld       : 1;
        RK_U32 i_stptr_wrdy       : 1;
        RK_U32 i_stptr_wvld       : 1;
        RK_U32 i_rdy_atf          : 1;

        RK_U32 i_vld_atf          : 1;
        RK_U32 i_rdy_bmv16        : 1;
        RK_U32 i_vld_bmv16        : 1;
        RK_U32 i_wr_dsp           : 1;

        RK_U32 i_rdy_dsp          : 1;
        RK_U32 i_vld_dsp          : 1;
        RK_U32 r_rdy_org          : 1;
        RK_U32 i_vld_org          : 1;

        RK_U32 i_rdy_state        : 1;
        RK_U32 i_vld_state        : 1;
        RK_U32 i_rdy_madp         : 1;
        RK_U32 i_vld_madp         : 1;

        RK_U32 i_rdy_diff         : 1;
        RK_U32 i_vld_diff         : 1;
        RK_U32 reserved           : 2;
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
    } dbg_dma_ar;

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
    } dbg_dma_r;

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
    } dbg_dma_dbg0;

    /* 0x00005140 reg5200 */
    struct {
        RK_U32 cpip_st     : 2;
        RK_U32 mvp_st      : 3;
        RK_U32 qpd6_st     : 2;
        RK_U32 cmd_st      : 2;
        RK_U32 reserved    : 23;
    } dbg_dma_dbg1;

    /* 0x00005144 reg5201 */
    struct {
        RK_U32 cme_byps    : 3;
        RK_U32 reserved    : 29;
    } dbg_tctrl;

    /* 0x5148 */
    RK_U32 reserved_5202;

    /* 0x0000514c reg5203 */
    struct {
        RK_U32 lpf_work               : 1;
        RK_U32 rdo_par_nrdy           : 1;
        RK_U32 rdo_rcn_nrdy           : 1;
        RK_U32 lpf_rcn_rdy            : 1;
        RK_U32 dblk_work              : 1;
        RK_U32 sao_work               : 1;
        RK_U32 reserved               : 18;
        RK_U32 tile_bdry_read         : 1;
        RK_U32 tile_bdry_write        : 1;
        RK_U32 tile_bdry_rrdy         : 1;
        RK_U32 rdo_read_tile_bdry     : 1;
        RK_U32 rdo_write_tile_bdry    : 1;
        RK_U32 reserved1              : 3;
    } dbg_lpf;

    /* 0x00005150 reg5204 */
    RK_U32 dbg_topc_lpfr;

    /* 0x00005154 reg5205 */
    RK_U32 dbg0_cache;

    /* 0x00005158 reg5206 */
    RK_U32 dbg1_cache;

    /* 0x0000515c reg5207 */
    RK_U32 dbg2_cache;

    /* 0x00005160 reg5208 */
    struct {
        RK_U32 ebuf_diff_cmd    : 8;
        RK_U32 lbuf_lpf_ncnt    : 7;
        RK_U32 lbuf_lpf_cien    : 1;
        RK_U32 lbuf_rdo_ncnt    : 7;
        RK_U32 lbuf_rdo_cien    : 1;
        RK_U32 reserved         : 8;
    } dbg_lbuf0;

    /* 0x00005164 reg5209 */
    struct {
        RK_U32 rvld_ebfr          : 1;
        RK_U32 rrdy_ebfr          : 1;
        RK_U32 arvld_ebfr         : 1;
        RK_U32 arrdy_ebfr         : 1;
        RK_U32 wvld_ebfw          : 1;
        RK_U32 wrdy_ebfw          : 1;
        RK_U32 awvld_ebfw         : 1;
        RK_U32 awrdy_ebfw         : 1;
        RK_U32 lpf_lbuf_rvld      : 1;
        RK_U32 lpf_lbuf_rrdy      : 1;
        RK_U32 lpf_lbuf_wvld      : 1;
        RK_U32 lpf_lbuf_wrdy      : 1;
        RK_U32 rdo_lbuf_rvld      : 1;
        RK_U32 rdo_lbuf_rrdy      : 1;
        RK_U32 rdo_lbuf_wvld      : 1;
        RK_U32 rdo_lbuf_wrdy      : 1;
        RK_U32 fme_lbuf_rvld      : 1;
        RK_U32 fme_lbuf_rrdy      : 1;
        RK_U32 cme_lbuf_rvld      : 1;
        RK_U32 cme_lbuf_rrdy      : 1;
        RK_U32 smear_lbuf_rvld    : 1;
        RK_U32 smear_lbuf_rrdy    : 1;
        RK_U32 smear_lbuf_wvld    : 1;
        RK_U32 smear_lbuf_wrdy    : 1;
        RK_U32 rdo_lbufw_flag     : 1;
        RK_U32 rdo_lbufr_flag     : 1;
        RK_U32 cme_lbufr_flag     : 1;
        RK_U32 reserved           : 5;
    } dbg_lbuf1;

    /* 0x00005168 reg5210 */
    struct {
        RK_U32 vinf_lcnt_dvbm    : 14;
        RK_U32 vinf_fcnt_dvbm    : 8;
        RK_U32 vinf_rdy_dvbm     : 1;
        RK_U32 vinf_vld_dvbm     : 1;
        RK_U32 st_cur_vinf       : 3;
        RK_U32 st_cur_vrsp       : 2;
        RK_U32 vcnt_req_sync     : 1;
        RK_U32 vcnt_ack_dvbm     : 1;
        RK_U32 vcnt_req_dvbm     : 1;
    } dbg_dvbm0;

    /* 0x0000516c reg5211 */
    struct {
        RK_U32 vrsp_lcnt_dvbm    : 14;
        RK_U32 vrsp_fcnt_dvbm    : 8;
        RK_U32 vrsp_tgl_dvbm     : 1;
        RK_U32 reserved          : 9;
    } dbg_dvbm1;

    /* 0x00005170 reg5212 */
    struct {
        RK_U32 dvbm_src_lcnt     : 12;
        RK_U32 jbuf_dvbm_rdy     : 1;
        RK_U32 vbuf_dvbm_rdy     : 1;
        RK_U32 work_dvbm_rdy     : 1;
        RK_U32 fmch_dvbm_rdy     : 1;
        RK_U32 vrsp_lcnt_vsld    : 14;
        RK_U32 vrsp_rdy_vsld     : 1;
        RK_U32 vrsp_vld_vsld     : 1;
    } dbg_dvbm2;

    /* 0x00005174 reg5213 */
    struct {
        RK_U32 vsp_ctu_flag     : 4;
        RK_U32 reserved         : 4;
        RK_U32 cime_ctu_flag    : 8;
        RK_U32 swin_ctu_flag    : 2;
        RK_U32 rfme_ctu_flag    : 6;
        RK_U32 pnra_ctu_flag    : 1;
        RK_U32 rdo_ctu_flg0     : 7;
    } dbg_tctrl0;

    /* 0x00005178 reg5214 */
    struct {
        RK_U32 rdo_ctu_flg1     : 8;
        RK_U32 jpeg_ctu_flag    : 3;
        RK_U32 lpf_ctu_flag     : 1;
        RK_U32 reserved         : 4;
        RK_U32 dma_brsp_idle    : 1;
        RK_U32 jpeg_frm_done    : 1;
        RK_U32 rdo_frm_done     : 1;
        RK_U32 lpf_frm_done     : 1;
        RK_U32 ent_frm_done     : 1;
        RK_U32 ppl_ctrl_done    : 1;
        RK_U32 reserved1        : 10;
    } dbg_tctrl1;

    /* 0x0000517c reg5215 */
    struct {
        RK_U32 criw_frm_done     : 1;
        RK_U32 meiw_frm_done     : 1;
        RK_U32 smiw_frm_done     : 1;
        RK_U32 strg_rsrc_done    : 1;
        RK_U32 reserved          : 28;
    } dbg_tctrl2;

    /* 0x5180 - 0x51fc */
    RK_U32 reserved5216_5247[32];

    /* 0x00005200 reg5248 */
    RK_U32 frame_cyc;

    /* 0x00005204 reg5249 */
    RK_U32 pp0_fcyc;

    /* 0x00005208 reg5250 */
    RK_U32 pp1_fcyc;

    /* 0x0000520c reg5251 */
    RK_U32 cme_fcyc;

    /* 0x00005210 reg5252 */
    RK_U32 ldr_fcyc;

    /* 0x00005214 reg5253 */
    RK_U32 rfme_fcyc;

    /* 0x00005218 reg5254 */
    RK_U32 fme_fcyc;

    /* 0x0000521c reg5255 */
    RK_U32 rdo_fcyc;

    /* 0x00005220 reg5256 */
    RK_U32 lpf_fcyc;

    /* 0x00005224 reg5257 */
    RK_U32 etpy_fcyc;

    /* 0x00005228 reg5258 */
    RK_U32 jpeg_fcyc;

    /* 0x522c - 0x52fc */
    RK_U32 reserved5259_5311[53];

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
    RK_U32 axip_num_ltcy;

    /* 0x00005328 reg5322 */
    RK_U32 axip_sum_ltcy;

    /* 0x0000532c reg5323 */
    RK_U32 axip_rbyt;

    /* 0x00005330 reg5324 */
    RK_U32 axip_wbyt;

    /* 0x00005334 reg5325 */
    RK_U32 axip_wrk_cyc;

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
} vepu540c_dbg;

typedef struct Vepu540cJpegCfg_t {
    MppDev dev;
    void *jpeg_reg_base;
    void *reg_tab;
    void *enc_task;
    void *input_fmt;
} Vepu540cJpegCfg;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vepu540c_set_jpeg_reg(Vepu540cJpegCfg *cfg);
MPP_RET vepu540c_set_roi(void *roi_reg_base, MppEncROICfg * roi, RK_S32 w,
                         RK_S32 h);

#ifdef __cplusplus
}
#endif
#endif              /* __VEPU540C_COMMON_H__ */
