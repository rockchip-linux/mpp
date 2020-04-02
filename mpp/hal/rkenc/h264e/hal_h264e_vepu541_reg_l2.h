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

#ifndef __HAL_H264E_VEPU541_REG_L2_H__
#define __HAL_H264E_VEPU541_REG_L2_H__

#include "rk_type.h"

/*
 * L2CFG_ADDR
 * Address offset: 0x3F0 Access type: read and write
 * Level2 configuration address
 */
/*
 * L2CFG_WDATA
 * Address offset: 0x3F4 Access type: read and write
 * L2 configuration write data
 */
/*
 * L2 configuration write data.
 *
 * Single access:
 * write address to VEPU_L2CFG_ADDR then write data to VEPU_L2CFG_WDATA.
 *
 * Burst access:
 * write the start address to VEPU_L2CFG_ADDR then write datas
 * (to VEPU_L2CFG_WDATA) consecutively.
 * Address will be auto increased after write VEPU_L2CFG_WDATA,
 * no need to configure VEPU_L2CFG_ADDR.
 */

/*
 * L2CFG_RDATA
 * Address offset: 0x3F8 Access type: read and write
 * L2 configuration read data
 */
struct {
    /*
     * L2 configuration read data.
     *
     * Single access:
     * write address to VEPU_L2CFG_ADDR then read data from VEPU_L2CFG_RDATA.
     *
     * Burst access:
     * write the start address to VEPU_L2CFG_ADDR then read datas
     * (from VEPU_L2CFG_RDATA) consecutively.
     * Address will be auto increased after read VEPU_L2CFG_RDATA,
     * no need to configure VEPU_L2CFG_ADDR.
     */
    RK_U32  l2cfg_rdata;
} reg254;

/* reg gap 255 */
RK_U32 reg_255;


typedef struct Vepu541H264eRegL2Set_t {
    /*
     * IPRD_TTHDY4_0_H264 ~ IPRD_TTHDY4_1_H264
     * Address: 0x0004~0x0008 Access type: read and write
     * The texture thredsholds for H.264 LUMA 4x4 intra prediction
     */
    RK_U16  iprd_tthdy4[4];

    /*
     * IPRD_TTHDC8_0_H264 ~ IPRD_TTHDC8_1_H264
     * Address: 0x000C~0x0010 Access type: read and write
     * The texture threshold for H.264 CHROMA 8x8 intra prediction.
     */
    RK_U16  iprd_tthdc8[4];

    /*
     * IPRD_TTHDY8_0_H264 ~ IPRD_TTHDY8_1_H264
     * Address: 0x0014~0x0018 Access type: read and write
     * The texture thredsholds for H.264 LUMA 8x8 intra prediction
     */
    RK_U16  iprd_tthdy8[4];

    /*
     * IPRD_TTHD_UL_H264
     * Address: 0x001C Access type: read and write
     * Texture thredsholds of up and left MB for H.264 LUMA intra prediction.
     */
    RK_U32  iprd_tthd_ul;

    /*
     * IPRD_WGTY8_H264
     * Address: 0x0020 Access type: read and write
     * Weights of the cost for H.264 LUMA 8x8 intra prediction
     */
    RK_U8   iprd_wgty8[4];

    /*
     * IPRD_WGTY4_H264
     * Address: 0x0024 Access type: read and write
     * Weights of the cost for H.264 LUMA 4x4 intra prediction
     */
    RK_U8   iprd_wgty4[4];

    /*
     * IPRD_WGTY16_H264
     * Address: 0x0028 Access type: read and write
     * Weights of the cost for H.264 LUMA 16x16 intra prediction
     */
    RK_U8   iprd_wgty16[4];

    /*
     * IPRD_WGTC8_H264
     * Address: 0x002C Access type: read and write
     * Weights of the cost for H.264 CHROMA 8x8 intra prediction
     */
    RK_U8   iprd_wgtc8[4];

    /*
     * QNT_BIAS_COMB
     * Address: 0x0030 Access type: read and write
     * Quantization bias for H.264 and HEVC.
     */
    struct {
        /* Quantization bias for HEVC and H.264 I frame. */
        RK_U32  qnt_bias_i              : 10;
        /* Quantization bias for HEVC and H.264 P frame. */
        RK_U32  qnt_bias_p              : 10;
        RK_U32  reserved                : 12;
    } qnt_bias_comb;

    /*
     * ATR_THD0_H264
     * Address: 0x0034 Access type: read and write
     * H.264 anti ringing noise threshold configuration0.
     */
    struct {
        /* The 1st threshold for H.264 anti-ringing-noise. */
        RK_U32  atr_thd0                : 12;
        RK_U32  reserved0               : 4;
        /* The 2nd threshold for H.264 anti-ringing-noise. */
        RK_U32  atr_thd1                : 12;
        RK_U32  reserved1               : 4;
    } atr_thd0_h264;

    /*
     * ATR_THD1_H264
     * Address: 0x0038 Access type: read and write
     * H.264 anti ringing noise threshold configuration1.
     */
    struct {
        /* The 3rd threshold for H.264 anti-ringing-noise. */
        RK_U32  atr_thd2                : 12;
        RK_U32  reserved0               : 4;
        /* QP threshold of P frame for H.264 anti-ringing-nois. */
        RK_U32  atr_qp                  : 6;
        RK_U32  reserved1               : 10;
    } atr_thd1_h264;

    /*
     * ATR_WGT16_H264
     * Address: 0x003C Access type: read and write
     * Weights of 16x16 cost for H.264 anti ringing noise.
     */
    struct {
        /* The 1st weight for H.264 16x16 anti-ringing-noise. */
        RK_U32  atr_lv16_wgt0           : 8;
        /* The 2nd weight for H.264 16x16 anti-ringing-noise. */
        RK_U32  atr_lv16_wgt1           : 8;
        /* The 3rd weight for H.264 16x16 anti-ringing-noise. */
        RK_U32  atr_lv16_wgt2           : 8;
        RK_U32  reserved                : 8;
    } atr_wgt16_h264;

    /*
     * ATR_WGT8_H264
     * Address: 0x0040 Access type: read and write
     * Weights of 8x8 cost for H.264 anti ringing noise.
     */
    struct {
        /* The 1st weight for H.264 8x8 anti-ringing-noise. */
        RK_U32  atr_lv8_wgt0            : 8;
        /* The 2nd weight for H.264 8x8 anti-ringing-noise. */
        RK_U32  atr_lv8_wgt1            : 8;
        /* The 3rd weight for H.264 8x8 anti-ringing-noise. */
        RK_U32  atr_lv8_wgt2            : 8;
        RK_U32  reserved                : 8;
    } atr_wgt8_h264;

    /*
     * ATR_WGT4_H264
     * Address: 0x0044 Access type: read and write
     * Weights of 4x4 cost for H.264 anti ringing noise.
     */
    struct {
        /* The 1st weight for H.264 4x4 anti-ringing-noise. */
        RK_U32  atr_lv4_wgt0            : 8;
        /* The 2nd weight for H.264 4x4 anti-ringing-noise. */
        RK_U32  atr_lv4_wgt1            : 8;
        /* The 3rd weight for H.264 4x4 anti-ringing-noise. */
        RK_U32  atr_lv4_wgt2            : 8;
        RK_U32  reserved                : 8;
    } atr_wgt4_h264;

    /*
     * ATF_TTHD0_H264 ~ ATF_TTHD1_H264
     * Address: 0x0048~0x004C Access type: read and write
     * Texture threshold configuration for H.264 anti-flicker
     */
    RK_U16  atf_tthd[4];

    /*
     * ATF_STHD0_H264
     * Address: 0x0050 Access type: read and write
     * (CME) SAD threshold configuration1 for H.264 anti-flicker.
     */
    struct {
        /* (CME) SAD threshold0 of texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_sthd_10             : 14;
        RK_U32  reserved0               : 2;
        /* Max (CME) SAD threshold for H.264 anti-flicker. */
        RK_U32  atf_sthd_max            : 14;
        RK_U32  reserved1               : 2;
    } atf_sthd0_h264;

    /*
     * ATF_STHD1_H264
     * Address: 0x0054 Access type: read and write
     * (CME) SAD threshold configuration1 for H.264 anti-flicker.
     */
    struct {
        /* (CME) SAD threshold1 of texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_sthd_11             : 14;
        RK_U32  reserved0               : 2;
        /* (CME) SAD threshold0 of texture interval2 for H.264 anti-flicker. */
        RK_U32  atf_sthd_20             : 14;
        RK_U32  reserved1               : 2;
    } atf_sthd1_h264;

    /*
     * ATF_WGT0_H264
     * Address: 0x0058 Access type: read and write
     * Weight configuration0 for H.264 anti-flicker.
     */
    struct {
        /* The 1st weight in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_wgt10               : 9;
        RK_U32  reserved0               : 7;
        /* The 2nd weight in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_wgt11               : 9;
        RK_U32  reserved1               : 7;
    } atf_wgt0_h264;

    /*
     * ATF_WGT1_H264
     * Address: 0x005C Access type: read and write
     * Weight configuration1 for H.264 anti-flicker.
     */
    struct {
        /* The 3rd weight in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_wgt12               : 9;
        RK_U32  reserved0               : 7;
        /* The 1st weight in texture interval2 for H.264 anti-flicker. */
        RK_U32  atf_wgt20               : 9;
        RK_U32  reserved1               : 7;
    } atf_wgt1_h264;

    /*
     * ATF_WGT2_H264
     * Address: 0x0060 Access type: read and write
     * Weight configuration2 for H.264 anti-flicker.
     */
    struct {
        /* The 2nd weight in texture interval2 for H.264 anti-flicker. */
        RK_U32  atf_wgt21               : 9;
        RK_U32  reserved0               : 7;
        /* The weight in texture interval3 for H.264 anti-flicker. */
        RK_U32  atf_wgt30               : 9;
        RK_U32  reserved1               : 7;
    } atf_wgt2_h264;

    /*
     * ATF_OFST0_H264
     * Address: 0x0064 Access type: read and write
     * Offset configuration0 for H.264 anti-flicker.
     */
    struct {
        /* The 1st offset in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_ofst10              : 14;
        RK_U32  reserved0               : 2;
        /* The 2nd offset in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_ofst11              : 14;
        RK_U32  reserved1               : 2;
    } atf_ofst0_h264;

    /*
     * ATF_OFST1_H264
     * Address: 0x0068 Access type: read and write
     * Offset configuration1 for H.264 anti-flicker.
     */
    struct {
        /* The 3rd offset in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_ofst12              : 14;
        RK_U32  reserved0               : 2;
        /* The 1st offset in texture interval2 for H.264 anti-flicker. */
        RK_U32  atf_ofst20              : 14;
        RK_U32  reserved1               : 2;
    } atf_ofst1_h264;

    /*
     * ATF_OFST2_H264
     * Address: 0x006C Access type: read and write
     * Offset configuration2 for H.264 anti-flicker.
     */
    struct {
        /* The 2nd offset in texture interval1 for H.264 anti-flicker. */
        RK_U32  atf_ofst21              : 14;
        RK_U32  reserved0               : 2;
        /* The offset in texture interval3 for H.264 anti-flicker. */
        RK_U32  atf_ofst30              : 14;
        RK_U32  reserved1               : 2;
    } atf_ofst2_h264;

    /*
     * IPRD_WGT_QP0_HEVC ~ IPRD_WGT_QP51_HEVC
     * Address: 0x0070 ~ 0x013C Access type: read and write
     * Weight of SATD cost when QP is 0~51 for HEVC intra prediction.
     */
    RK_U32  iprd_wgt_qp[52];

    /*
     * RDO_WGTA_QP0_COMB ~ RDO_WGTA_QP51_COMB
     * Address: 0x0140 ~ 0x020C Access type: read and write
     * Weight of group A for HEVC and H.264 RDO mode decision when QP is 0~51.
     */
    RK_U32  wgt_qp_grpa[52];

    /*
     * RDO_WGTB_QP0_COMB ~ RDO_WGTB_QP51_COMB
     * Address: 0x0210 ~ 0x02DC Access type: read and write
     * Weight of group B for HEVC and H.264 RDO mode decision when QP is 0~51.
     */
    RK_U32  wgt_qp_grpb[52];

    /*
     * MADI_CFG
     * Address: 0x02E0 Access type: read and write
     * MADI configuration for CU32 and CU64.
     */
    /*
     * MADI generation mode for CU32 and CU64.
     * 1'h0: Follow 32x32 and 64x64 MADI functions.
     * 1'h1: Calculated by the mean of corresponding CU16 MADIs.
     */
    RK_U32  madi_mode;

    /*
     * AQ_TTHD0 ~ AQ_TTHD3
     * Address: 0x02E4 ~ 0x02F0 Access type: read and write
     * Texture threshold configuration for adaptive QP adjustment.
     */
    /* Texture threshold for adaptive QP adjustment. */
    RK_U8   aq_tthd[16];

    /*
     * AQ_STP0 ~ AQ_STP3
     * Address: 0x02F4 ~ 0x300 Access type: read and write
     * Adjustment step configuration0 for adaptive QP adjustment.
     */
    /*
     * MADI generation mode for CU32 and CU64.
     * 1'h0: Follow 32x32 and 64x64 MADI functions.
     * 1'h1: Calculated by the mean of corresponding CU16 MADIs.
     */
    /* QP adjust step when current texture strength is between n-1 and n step. */
    RK_S8   aq_step[16];

    /*
     * RME_MVD_PNSH_H264
     * Address: 0x0304 Access type: read and write
     * RME MVD(motion vector difference) cost penalty, H.264 only.
     */
    struct {
        /* MVD cost penalty enable. */
        RK_U32  mvd_pnlt_e              : 1;
        /* MVD cost penalty coefficienc. */
        RK_U32  mvd_pnlt_coef           : 5;
        /* MVD cost penalty constant. */
        RK_U32  mvd_pnlt_cnst           : 14;
        /* Low threshold of the MVs which should be punished. */
        RK_U32  mvd_pnlt_lthd           : 4;
        /* High threshold of the MVs which should be punished. */
        RK_U32  mvd_pnlt_hthd           : 4;
        RK_U32  reserved                : 4;
    } rme_mvd_penalty;

    /*
     * ATR1_THD0_H264
     * Address: 0x0308 Access type: read and write
     * H.264 anti ringing noise threshold configuration0 of group1.
     */
    struct {
        /* The 1st threshold for H.264 anti-ringing-noise of group1. */
        RK_U32  atr1_thd0               : 12;
        RK_U32  reserved0               : 4;
        /* The 2nd threshold for H.264 anti-ringing-noise of group1. */
        RK_U32  atr1_thd1               : 12;
        RK_U32  reserved1               : 4;
    } atr1_thd0_h264;

    /*
     * ATR1_THD0_H264
     * Address: 0x030C Access type: read and write
     * H.264 anti ringing noise threshold configuration1 of group1.
     */
    struct {
        /* The 3rd threshold for H.264 anti-ringing-noise of group1. */
        RK_U32  atr1_thd2               : 12;
        RK_U32  reserved0               : 20;
    } atr1_thd1_h264;
} Vepu541H264eRegL2Set;

#endif /* __HAL_H264E_VEPU541_REG_L2_H__ */
