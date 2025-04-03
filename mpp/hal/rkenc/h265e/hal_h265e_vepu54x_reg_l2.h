/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H265E_VEPU54X_REG_L2_H__
#define __HAL_H265E_VEPU54X_REG_L2_H__

#include "rk_type.h"

typedef struct {
    /* 0x48 - ATF_THD0 */
    struct {
        RK_U32 atf_thd0_i32       : 6;
        RK_U32 reserved0          : 10;
        RK_U32 atf_thd1_i32       : 6;
        RK_U32 reserved1          : 10;
    } atf_thd0;

    /* 0x4c - ATF_THD1 */
    struct {
        RK_U32 atf_thd0_i16       : 6;
        RK_U32 reserved0          : 10;
        RK_U32 atf_thd1_i16       : 6;
        RK_U32 reserved1          : 10;
    } atf_thd1;

    /* 0x50 - ATF_SAD_THD0 */
    struct {
        RK_U32 atf_thd0_p64       : 6;
        RK_U32 reserved0          : 10;
        RK_U32 atf_thd1_p64       : 6;
        RK_U32 reserved1          : 10;
    } atf_sad_thd0;

    /* 0x54 - ATF_SAD_THD1 */
    struct {
        RK_U32 atf_thd0_p32       : 6;
        RK_U32 reserved0          : 10;
        RK_U32 atf_thd1_p32       : 6;
        RK_U32 reserved1          : 10;
    } atf_sad_thd1;

    /* 0x58 - ATF_SAD_WGT0 */
    struct {
        RK_U32 atf_thd0_p16       : 6;
        RK_U32 reserved0          : 10;
        RK_U32 atf_thd1_p16       : 6;
        RK_U32 reserved1          : 10;
    } atf_sad_wgt0;
} vepu541_intra_thd;

typedef struct {
    /* 0x48 - ATF_THD0 */
    struct {
        RK_U32 atf_thd0_i32       : 10;
        RK_U32 reserved0          : 6;
        RK_U32 atf_thd1_i32       : 10;
        RK_U32 reserved1          : 6;
    } atf_thd0;

    /* 0x4c - ATF_THD1 */
    struct {
        RK_U32 atf_thd0_i16       : 10;
        RK_U32 reserved0          : 6;
        RK_U32 atf_thd1_i16       : 10;
        RK_U32 reserved1          : 6;
    } atf_thd1;

    /* 0x50 - ATF_SAD_THD0 */
    struct {
        RK_U32 atf_thd0_p64       : 10;
        RK_U32 reserved0          : 6;
        RK_U32 atf_thd1_p64       : 10;
        RK_U32 reserved1          : 6;
    } atf_sad_thd0;

    /* 0x54 - ATF_SAD_THD1 */
    struct {
        RK_U32 atf_thd0_p32       : 10;
        RK_U32 reserved0          : 6;
        RK_U32 atf_thd1_p32       : 10;
        RK_U32 reserved1          : 6;
    } atf_sad_thd1;

    /* 0x58 - ATF_SAD_WGT0 */
    struct {
        RK_U32 atf_thd0_p16       : 10;
        RK_U32 reserved0          : 6;
        RK_U32 atf_thd1_p16       : 10;
        RK_U32 reserved1          : 6;
    } atf_sad_wgt0;
} vepu540_intra_thd;


typedef struct H265eV54xL2RegSet_t {
    /* L2 Register: 0x4 */
    struct {
        RK_U32 lvl32_intra_cst_thd0 : 12;
        RK_U32 reserved0            : 4;
        RK_U32 lvl32_intra_cst_thd1 : 12;
        RK_U32 reserved1            : 4;
    } lvl32_intra_CST_THD0;

    struct {
        RK_U32 lvl32_intra_cst_thd2 : 12;
        RK_U32 reserved0            : 4;
        RK_U32 lvl32_intra_cst_thd3 : 12;
        RK_U32 reserved1            : 4;
    } lvl32_intra_CST_THD1;

    struct {
        RK_U32 lvl16_intra_cst_thd0 : 12;
        RK_U32 reserved0            : 4;
        RK_U32 lvl16_intra_cst_thd1 : 12;
        RK_U32 reserved1            : 4;
    } lvl16_intra_CST_THD0;

    struct {
        RK_U32 lvl16_intra_cst_thd2 : 12;
        RK_U32 reserved0            : 4;
        RK_U32 lvl16_intra_cst_thd3 : 12;
        RK_U32 reserved1            : 4;
    } lvl16_intra_CST_THD1;

    /* 0x14-0x1c - reserved */
    RK_U32 lvl8_intra_CST_THD0;
    RK_U32 lvl8_intra_CST_THD1;
    RK_U32 lvl16_intra_UL_CST_THD;

    struct {
        RK_U32 lvl32_intra_cst_wgt0 : 8;
        RK_U32 lvl32_intra_cst_wgt1 : 8;
        RK_U32 lvl32_intra_cst_wgt2 : 8;
        RK_U32 lvl32_intra_cst_wgt3 : 8;
    } lvl32_intra_CST_WGT0;

    struct {
        RK_U32 lvl32_intra_cst_wgt4 : 8;
        RK_U32 lvl32_intra_cst_wgt5 : 8;
        RK_U32 lvl32_intra_cst_wgt6 : 8;
        RK_U32 reserved2            : 8;
    } lvl32_intra_CST_WGT1;

    struct {
        RK_U32 lvl16_intra_cst_wgt0 : 8;
        RK_U32 lvl16_intra_cst_wgt1 : 8;
        RK_U32 lvl16_intra_cst_wgt2 : 8;
        RK_U32 lvl16_intra_cst_wgt3 : 8;
    } lvl16_intra_CST_WGT0;

    struct {
        RK_U32 lvl16_intra_cst_wgt4 : 8;
        RK_U32 lvl16_intra_cst_wgt5 : 8;
        RK_U32 lvl16_intra_cst_wgt6 : 8;
        RK_U32 reserved2            : 8;
    } lvl16_intra_CST_WGT1;

    /* 0x30 - RDO_QUANT */
    struct {
        RK_U32 quant_f_bias_I       : 10;
        RK_U32 quant_f_bias_P       : 10;
        RK_U32 reserved             : 12;
    } rdo_quant;

    /* 0x34 - ATR_THD0, reserved */
    struct {
        RK_U32 atr_thd0         : 12;
        RK_U32 reserved0        : 4;
        RK_U32 atr_thd1         : 12;
        RK_U32 reserved1        : 4;
    } atr_thd0;

    /* 0x38 - ATR_THD1, reserved */
    struct {
        RK_U32 atr_thd2         : 12;
        RK_U32 reserved0        : 4;
        RK_U32 atr_thdqp        : 6;
        RK_U32 reserved1        : 10;
    } atr_thd1;

    /* 0x3c - Lvl16_ATR_WGT, reserved */
    struct {
        RK_U32 lvl16_atr_wgt0       : 8;
        RK_U32 lvl16_atr_wgt1       : 8;
        RK_U32 lvl16_atr_wgt2       : 8;
        RK_U32 reserved             : 8;
    } lvl16_atr_wgt;

    /* 0x40 - Lvl8_ATR_WGT, reserved */
    struct {
        RK_U32 lvl8_atr_wgt0       : 8;
        RK_U32 lvl8_atr_wgt1       : 8;
        RK_U32 lvl8_atr_wgt2       : 8;
        RK_U32 reserved            : 8;
    } lvl8_atr_wgt;

    /* 0x44 - Lvl4_ATR_WGT, reserved */
    struct {
        RK_U32 lvl4_atr_wgt0       : 8;
        RK_U32 lvl4_atr_wgt1       : 8;
        RK_U32 lvl4_atr_wgt2       : 8;
        RK_U32 reserved            : 8;
    } lvl4_atr_wgt;

    union {
        vepu541_intra_thd thd_541;
        vepu540_intra_thd thd_540;
    };

    /* 0x5c - ATF_SAD_WGT1 */
    struct {
        RK_U32 atf_wgt_i16       : 6;
        RK_U32 reserved0         : 10;
        RK_U32 atf_wgt_i32       : 6;
        RK_U32 reserved1         : 10;
    } atf_sad_wgt1;

    /* 0x60 - ATF_SAD_WGT2 */
    struct {
        RK_U32 atf_wgt_p32       : 6;
        RK_U32 reserved0         : 10;
        RK_U32 atf_wgt_p64       : 6;
        RK_U32 reserved1         : 10;
    } atf_sad_wgt2;

    /* 0x64 - ATF_SAD_OFST0 */
    struct {
        RK_U32 atf_wgt_p16         : 6;
        RK_U32 reserved            : 26;
    } atf_sad_ofst0;

    /* 0x68 - ATF_SAD_OFST1, reserved */
    struct {
        RK_U32 atf_sad_ofst12       : 14;
        RK_U32 reserved0            : 2;
        RK_U32 atf_sad_ofst20       : 14;
        RK_U32 reserved1            : 2;
    } atf_sad_ofst1;

    /* 0x6c - ATF_SAD_OFST2, reserved */
    struct {
        RK_U32 atf_sad_ofst21       : 14;
        RK_U32 reserved0            : 2;
        RK_U32 atf_sad_ofst30       : 14;
        RK_U32 reserved1            : 2;
    } atf_sad_ofst2;

    /* 0x70-0x13c - LAMD_SATD_qp */
    RK_U32 lamd_satd_qp[52];

    /* 0x140-0x20c - LAMD_MOD_qp, combo for I and P */
    RK_U32 lamd_moda_qp[52];
    /* 0x210-0x2dc */
    RK_U32 lamd_modb_qp[52];

    /* 0x2e0 - MADI_CFG */
    struct {
        RK_U32 reserved      : 16;
        RK_U32 madi_thd      : 8;
        RK_U32 reserved1     : 8;
    } madi_cfg;

    /* 0x2e4 - AQ_THD0 - 0x2f0 - AQ_THD3 */
    RK_U8 aq_tthd[16];

    /*
     * 0x2f4 - AQ_QP_DLT0 - 0x300 - AQ_QP_DLT3
     * only low 6 bits is valid for per step.
     */
    RK_U8 aq_step[16];

    /*0x304-0x30c*/
    RK_U32 reserve[3];
    /*pre_intra class mode */
    // 0x0310~0x394
    // 0x0310
    struct {
        RK_U32 pre_intra_cla0_m0 : 6;
        RK_U32 pre_intra_cla0_m1 : 6;
        RK_U32 pre_intra_cla0_m2 : 6;
        RK_U32 pre_intra_cla0_m3 : 6;
        RK_U32 pre_intra_cla0_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla0_B0;

    // 0x0314
    struct {
        RK_U32 pre_intra_cla0_m5 : 6;
        RK_U32 pre_intra_cla0_m6 : 6;
        RK_U32 pre_intra_cla0_m7 : 6;
        RK_U32 pre_intra_cla0_m8 : 6;
        RK_U32 pre_intra_cla0_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla0_B1;

    // 0x0318
    struct {
        RK_U32 pre_intra_cla1_m0 : 6;
        RK_U32 pre_intra_cla1_m1 : 6;
        RK_U32 pre_intra_cla1_m2 : 6;
        RK_U32 pre_intra_cla1_m3 : 6;
        RK_U32 pre_intra_cla1_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla1_B0;

    // 0x031c
    struct {
        RK_U32 pre_intra_cla1_m5 : 6;
        RK_U32 pre_intra_cla1_m6 : 6;
        RK_U32 pre_intra_cla1_m7 : 6;
        RK_U32 pre_intra_cla1_m8 : 6;
        RK_U32 pre_intra_cla1_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla1_B1;

    // 0x0320
    struct {
        RK_U32 pre_intra_cla2_m0 : 6;
        RK_U32 pre_intra_cla2_m1 : 6;
        RK_U32 pre_intra_cla2_m2 : 6;
        RK_U32 pre_intra_cla2_m3 : 6;
        RK_U32 pre_intra_cla2_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla2_B0;

    // 0x0324
    struct {
        RK_U32 pre_intra_cla2_m5 : 6;
        RK_U32 pre_intra_cla2_m6 : 6;
        RK_U32 pre_intra_cla2_m7 : 6;
        RK_U32 pre_intra_cla2_m8 : 6;
        RK_U32 pre_intra_cla2_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla2_B1;

    // 0x0328
    struct {
        RK_U32 pre_intra_cla3_m0 : 6;
        RK_U32 pre_intra_cla3_m1 : 6;
        RK_U32 pre_intra_cla3_m2 : 6;
        RK_U32 pre_intra_cla3_m3 : 6;
        RK_U32 pre_intra_cla3_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla3_B0;

    // 0x032c
    struct {
        RK_U32 pre_intra_cla3_m5 : 6;
        RK_U32 pre_intra_cla3_m6 : 6;
        RK_U32 pre_intra_cla3_m7 : 6;
        RK_U32 pre_intra_cla3_m8 : 6;
        RK_U32 pre_intra_cla3_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla3_B1;

    // 0x0330
    struct {
        RK_U32 pre_intra_cla4_m0 : 6;
        RK_U32 pre_intra_cla4_m1 : 6;
        RK_U32 pre_intra_cla4_m2 : 6;
        RK_U32 pre_intra_cla4_m3 : 6;
        RK_U32 pre_intra_cla4_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla4_B0;

    // 0x0334
    struct {
        RK_U32 pre_intra_cla4_m5 : 6;
        RK_U32 pre_intra_cla4_m6 : 6;
        RK_U32 pre_intra_cla4_m7 : 6;
        RK_U32 pre_intra_cla4_m8 : 6;
        RK_U32 pre_intra_cla4_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla4_B1;

    // 0x0338
    struct {
        RK_U32 pre_intra_cla5_m0 : 6;
        RK_U32 pre_intra_cla5_m1 : 6;
        RK_U32 pre_intra_cla5_m2 : 6;
        RK_U32 pre_intra_cla5_m3 : 6;
        RK_U32 pre_intra_cla5_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla5_B0;

    // 0x033c
    struct {
        RK_U32 pre_intra_cla5_m5 : 6;
        RK_U32 pre_intra_cla5_m6 : 6;
        RK_U32 pre_intra_cla5_m7 : 6;
        RK_U32 pre_intra_cla5_m8 : 6;
        RK_U32 pre_intra_cla5_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla5_B1;

    // 0x0340
    struct {
        RK_U32 pre_intra_cla6_m0 : 6;
        RK_U32 pre_intra_cla6_m1 : 6;
        RK_U32 pre_intra_cla6_m2 : 6;
        RK_U32 pre_intra_cla6_m3 : 6;
        RK_U32 pre_intra_cla6_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla6_B0;

    // 0x0344
    struct {
        RK_U32 pre_intra_cla6_m5 : 6;
        RK_U32 pre_intra_cla6_m6 : 6;
        RK_U32 pre_intra_cla6_m7 : 6;
        RK_U32 pre_intra_cla6_m8 : 6;
        RK_U32 pre_intra_cla6_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla6_B1;

    // 0x0348
    struct {
        RK_U32 pre_intra_cla7_m0 : 6;
        RK_U32 pre_intra_cla7_m1 : 6;
        RK_U32 pre_intra_cla7_m2 : 6;
        RK_U32 pre_intra_cla7_m3 : 6;
        RK_U32 pre_intra_cla7_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla7_B0;

    // 0x034c
    struct {
        RK_U32 pre_intra_cla7_m5 : 6;
        RK_U32 pre_intra_cla7_m6 : 6;
        RK_U32 pre_intra_cla7_m7 : 6;
        RK_U32 pre_intra_cla7_m8 : 6;
        RK_U32 pre_intra_cla7_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla7_B1;

    // 0x0350
    struct {
        RK_U32 pre_intra_cla8_m0 : 6;
        RK_U32 pre_intra_cla8_m1 : 6;
        RK_U32 pre_intra_cla8_m2 : 6;
        RK_U32 pre_intra_cla8_m3 : 6;
        RK_U32 pre_intra_cla8_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla8_B0;

    // 0x0354
    struct {
        RK_U32 pre_intra_cla8_m5 : 6;
        RK_U32 pre_intra_cla8_m6 : 6;
        RK_U32 pre_intra_cla8_m7 : 6;
        RK_U32 pre_intra_cla8_m8 : 6;
        RK_U32 pre_intra_cla8_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla8_B1;

    // 0x0358
    struct {
        RK_U32 pre_intra_cla9_m0 : 6;
        RK_U32 pre_intra_cla9_m1 : 6;
        RK_U32 pre_intra_cla9_m2 : 6;
        RK_U32 pre_intra_cla9_m3 : 6;
        RK_U32 pre_intra_cla9_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla9_B0;

    // 0x035c
    struct {
        RK_U32 pre_intra_cla9_m5 : 6;
        RK_U32 pre_intra_cla9_m6 : 6;
        RK_U32 pre_intra_cla9_m7 : 6;
        RK_U32 pre_intra_cla9_m8 : 6;
        RK_U32 pre_intra_cla9_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla9_B1;

    // 0x0360
    struct {
        RK_U32 pre_intra_cla10_m0 : 6;
        RK_U32 pre_intra_cla10_m1 : 6;
        RK_U32 pre_intra_cla10_m2 : 6;
        RK_U32 pre_intra_cla10_m3 : 6;
        RK_U32 pre_intra_cla10_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla10_B0;

    // 0x0364
    struct {
        RK_U32 pre_intra_cla10_m5 : 6;
        RK_U32 pre_intra_cla10_m6 : 6;
        RK_U32 pre_intra_cla10_m7 : 6;
        RK_U32 pre_intra_cla10_m8 : 6;
        RK_U32 pre_intra_cla10_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla10_B1;

    // 0x0368
    struct {
        RK_U32 pre_intra_cla11_m0 : 6;
        RK_U32 pre_intra_cla11_m1 : 6;
        RK_U32 pre_intra_cla11_m2 : 6;
        RK_U32 pre_intra_cla11_m3 : 6;
        RK_U32 pre_intra_cla11_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla11_B0;

    // 0x036c
    struct {
        RK_U32 pre_intra_cla11_m5 : 6;
        RK_U32 pre_intra_cla11_m6 : 6;
        RK_U32 pre_intra_cla11_m7 : 6;
        RK_U32 pre_intra_cla11_m8 : 6;
        RK_U32 pre_intra_cla11_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla11_B1;

    // 0x0370
    struct {
        RK_U32 pre_intra_cla12_m0 : 6;
        RK_U32 pre_intra_cla12_m1 : 6;
        RK_U32 pre_intra_cla12_m2 : 6;
        RK_U32 pre_intra_cla12_m3 : 6;
        RK_U32 pre_intra_cla12_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla12_B0;

    // 0x0374
    struct {
        RK_U32 pre_intra_cla12_m5 : 6;
        RK_U32 pre_intra_cla12_m6 : 6;
        RK_U32 pre_intra_cla12_m7 : 6;
        RK_U32 pre_intra_cla12_m8 : 6;
        RK_U32 pre_intra_cla12_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla12_B1;

    // 0x0378
    struct {
        RK_U32 pre_intra_cla13_m0 : 6;
        RK_U32 pre_intra_cla13_m1 : 6;
        RK_U32 pre_intra_cla13_m2 : 6;
        RK_U32 pre_intra_cla13_m3 : 6;
        RK_U32 pre_intra_cla13_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla13_B0;

    // 0x037c
    struct {
        RK_U32 pre_intra_cla13_m5 : 6;
        RK_U32 pre_intra_cla13_m6 : 6;
        RK_U32 pre_intra_cla13_m7 : 6;
        RK_U32 pre_intra_cla13_m8 : 6;
        RK_U32 pre_intra_cla13_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla13_B1;

    // 0x0380
    struct {
        RK_U32 pre_intra_cla14_m0 : 6;
        RK_U32 pre_intra_cla14_m1 : 6;
        RK_U32 pre_intra_cla14_m2 : 6;
        RK_U32 pre_intra_cla14_m3 : 6;
        RK_U32 pre_intra_cla14_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla14_B0;

    // 0x0384
    struct {
        RK_U32 pre_intra_cla14_m5 : 6;
        RK_U32 pre_intra_cla14_m6 : 6;
        RK_U32 pre_intra_cla14_m7 : 6;
        RK_U32 pre_intra_cla14_m8 : 6;
        RK_U32 pre_intra_cla14_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla14_B1;

    // 0x0388
    struct {
        RK_U32 pre_intra_cla15_m0 : 6;
        RK_U32 pre_intra_cla15_m1 : 6;
        RK_U32 pre_intra_cla15_m2 : 6;
        RK_U32 pre_intra_cla15_m3 : 6;
        RK_U32 pre_intra_cla15_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla15_B0;

    // 0x038c
    struct {
        RK_U32 pre_intra_cla15_m5 : 6;
        RK_U32 pre_intra_cla15_m6 : 6;
        RK_U32 pre_intra_cla15_m7 : 6;
        RK_U32 pre_intra_cla15_m8 : 6;
        RK_U32 pre_intra_cla15_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla15_B1;

    // 0x0390
    struct {
        RK_U32 pre_intra_cla16_m0 : 6;
        RK_U32 pre_intra_cla16_m1 : 6;
        RK_U32 pre_intra_cla16_m2 : 6;
        RK_U32 pre_intra_cla16_m3 : 6;
        RK_U32 pre_intra_cla16_m4 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla16_B0;

    // 0x0394
    struct {
        RK_U32 pre_intra_cla16_m5 : 6;
        RK_U32 pre_intra_cla16_m6 : 6;
        RK_U32 pre_intra_cla16_m7 : 6;
        RK_U32 pre_intra_cla16_m8 : 6;
        RK_U32 pre_intra_cla16_m9 : 6;
        RK_U32 reserved : 2;
    } pre_intra_cla16_B1;

    // 0x0398 0x03fC
    RK_U32 reg_L2reserved1[26];

    // 0x400
    RK_U32 reg_rdo_ckg_hevc;

    // 0x404~0x40c
    RK_U32 reg_L2reserved2[3];

    // 0x410
    struct {
        RK_U32 intra_l16_sobel_t0 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 intra_l16_sobel_t1 : 12;
        RK_U32 reserved1 : 4;
    } i16_sobel_t_hevc;

    struct {
        RK_U32 intra_l16_sobel_a0_qp0 : 6;
        RK_U32 intra_l16_sobel_a0_qp1 : 6;
        RK_U32 intra_l16_sobel_a0_qp2 : 6;
        RK_U32 intra_l16_sobel_a0_qp3 : 6;
        RK_U32 intra_l16_sobel_a0_qp4 : 6;
        RK_U32 reserved0 : 2;
    } i16_sobel_a_00_hevc;

    struct {
        RK_U32 intra_l16_sobel_a0_qp5 : 6;
        RK_U32 intra_l16_sobel_a0_qp6 : 6;
        RK_U32 intra_l16_sobel_a0_qp7 : 6;
        RK_U32 intra_l16_sobel_a0_qp8 : 6;
        RK_U32 reserved0 : 8;
    } i16_sobel_a_01_hevc;

    struct {
        RK_U32 intra_l16_sobel_b0_qp0 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_b0_qp1 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_b_00_hevc;

    struct {
        RK_U32 intra_l16_sobel_b0_qp2 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_b0_qp3 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_b_01_hevc;

    struct {
        RK_U32 intra_l16_sobel_b0_qp4 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_b0_qp5 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_b_02_hevc;

    struct {
        RK_U32 intra_l16_sobel_b0_qp6 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_b0_qp7 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_b_03_hevc;

    struct {
        RK_U32 intra_l16_sobel_b0_qp8 : 15;
        RK_U32 reserved0 : 17;
    } i16_sobel_b_04_hevc;

    struct {
        RK_U32 intra_l16_sobel_c0_qp0 : 6;
        RK_U32 intra_l16_sobel_c0_qp1 : 6;
        RK_U32 intra_l16_sobel_c0_qp2 : 6;
        RK_U32 intra_l16_sobel_c0_qp3 : 6;
        RK_U32 intra_l16_sobel_c0_qp4 : 6;
        RK_U32 reserved0 : 2;
    } i16_sobel_c_00_hevc;

    struct {
        RK_U32 intra_l16_sobel_c0_qp5 : 6;
        RK_U32 intra_l16_sobel_c0_qp6 : 6;
        RK_U32 intra_l16_sobel_c0_qp7 : 6;
        RK_U32 intra_l16_sobel_c0_qp8 : 6;
        RK_U32 reserved0 : 8;
    } i16_sobel_c_01_hevc;

    struct {
        RK_U32 intra_l16_sobel_d0_qp0 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_d0_qp1 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_d_00_hevc;

    struct {
        RK_U32 intra_l16_sobel_d0_qp2 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_d0_qp3 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_d_01_hevc;

    struct {
        RK_U32 intra_l16_sobel_d0_qp4 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_d0_qp5 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_d_02_hevc;

    struct {
        RK_U32 intra_l16_sobel_d0_qp6 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l16_sobel_d0_qp7 : 15;
        RK_U32 reserved1 : 1;
    } i16_sobel_d_03_hevc;

    struct {
        RK_U32 intra_l16_sobel_d0_qp8 : 15;
        RK_U32 reserved0 : 17;
    } i16_sobel_d_04_hevc;

    RK_U32 i16_sobel_e_00_17_hevc[18];// 0 2 4 ... low 32bit ; 1 3 5 ... high 2bit

    // 0x494
    struct {
        RK_U32 intra_l32_sobel_t2 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 intra_l32_sobel_t3 : 12;
        RK_U32 reserved1 : 4;
    } i32_sobel_t_00_hevc;

    struct {
        RK_U32 intra_l32_sobel_t4 : 6;
        RK_U32 reserved0 : 24;

    } i32_sobel_t_01_hevc;

    struct {
        RK_U32 intra_l32_sobel_t5 : 12;
        RK_U32 reserved0 : 4;
        RK_U32 intra_l32_sobel_t6 : 12;
        RK_U32 reserved1 : 4;
    } i32_sobel_t_02_hevc;

    struct {
        RK_U32 intra_l32_sobel_a1_qp0 : 6;
        RK_U32 intra_l32_sobel_a1_qp1 : 6;
        RK_U32 intra_l32_sobel_a1_qp2 : 6;
        RK_U32 intra_l32_sobel_a1_qp3 : 6;
        RK_U32 intra_l32_sobel_a1_qp4 : 6;
        RK_U32 reserved0 : 2;
    } i32_sobel_a_hevc;

    struct {
        RK_U32 intra_l32_sobel_b1_qp0 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l32_sobel_b1_qp1 : 15;
        RK_U32 reserved1 : 1;
    } i32_sobel_b_00_hevc;

    struct {
        RK_U32 intra_l32_sobel_b1_qp2 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l32_sobel_b1_qp3 : 15;
        RK_U32 reserved1 : 1;
    } i32_sobel_b_01_hevc;

    struct {
        RK_U32 intra_l32_sobel_b1_qp4 : 15;
        RK_U32 reserved0 : 17;
    } i32_sobel_b_02_hevc;

    struct {
        RK_U32 intra_l32_sobel_c1_qp0 : 6;
        RK_U32 intra_l32_sobel_c1_qp1 : 6;
        RK_U32 intra_l32_sobel_c1_qp2 : 6;
        RK_U32 intra_l32_sobel_c1_qp3 : 6;
        RK_U32 intra_l32_sobel_c1_qp4 : 6;
        RK_U32 reserved0 : 2;
    } i32_sobel_c_hevc;

    struct {
        RK_U32 intra_l32_sobel_d1_qp0 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l32_sobel_d1_qp1 : 15;
        RK_U32 reserved1 : 1;
    } i32_sobel_d_00_hevc;

    struct {
        RK_U32 intra_l32_sobel_d1_qp2 : 15;
        RK_U32 reserved0 : 1;
        RK_U32 intra_l32_sobel_d1_qp3 : 15;
        RK_U32 reserved1 : 1;
    } i32_sobel_d_01_hevc;

    struct {
        RK_U32 intra_l32_sobel_d1_qp4 : 15;
        RK_U32 reserved0 : 17;
    } i32_sobel_d_02_hevc;
    // 0x4c0~0x4e4
    RK_U32 i32_sobel_e_00_09_hevc[10];// 0 2 4 ... low 32bit ; 1 3 5 ... high 9bit
} H265eV54xL2RegSet;

#endif /* __HAL_H265E_VEPU54X_REG_L2_H__ */
