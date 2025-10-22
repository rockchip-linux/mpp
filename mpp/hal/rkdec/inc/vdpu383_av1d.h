/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPU383_AV1D_H
#define VDPU383_AV1D_H

#include "rk_type.h"
#include "vdpu383_com.h"


typedef struct Vdpu383RegAv1dParas_t {
    /* SWREG64_AV1_PARA */
    RK_U32 reg64_unused_bits;

    /* SWREG65_STREAM_PARAM_SET */
    RK_U32 reg65_strm_start_bit;

    /* SWREG66_STREAM_LEN */
    RK_U32 reg66_stream_len;

    /* SWREG67_GLOBAL_LEN */
    RK_U32 reg67_global_len;

    /* SWREG68_HOR_STRIDE */
    RK_U32 reg68_hor_virstride;

    /* SWREG69_RASTER_UV_HOR_STRIDE */
    RK_U32 reg69_raster_uv_hor_virstride;

    /* SWREG70_Y_STRIDE */
    RK_U32 reg70_y_virstride;

    /* SWREG71_SCL_Y_HOR_VIRSTRIDE */
    RK_U32 reg71_scl_ref_hor_virstride;

    /* SWREG72_SCL_UV_HOR_VIRSTRIDE */
    RK_U32 reg72_scl_ref_raster_uv_hor_virstride;

    /* SWREG73_SCL_Y_VIRSTRIDE */
    RK_U32 reg73_scl_ref_virstride;

    /* SWREG74_FGS_Y_HOR_VIRSTRIDE */
    RK_U32 reg74_fgs_ref_hor_virstride;

    RK_U32 reserve_reg75_79[5];

    /* SWREG80_ERROR_REF_Y_HOR_VIRSTRIDE */
    RK_U32 reg80_error_ref_hor_virstride;

    /* SWREG81_ERROR_REF_UV_HOR_VIRSTRIDE */
    RK_U32 reg81_error_ref_raster_uv_hor_virstride;

    /* SWREG82_ERROR_REF_Y_VIRSTRIDE */
    RK_U32 reg82_error_ref_virstride;

    /* SWREG83_REF0_Y_HOR_VIRSTRIDE */
    RK_U32 reg83_ref0_hor_virstride;

    /* SWREG84_REF0_UV_HOR_VIRSTRIDE */
    RK_U32 reg84_ref0_raster_uv_hor_virstride;

    /* SWREG85_REF0_Y_VIRSTRIDE */
    RK_U32 reg85_ref0_virstride;

    /* SWREG86_REF1_Y_HOR_VIRSTRIDE */
    RK_U32 reg86_ref1_hor_virstride;

    /* SWREG87_REF1_UV_HOR_VIRSTRIDE */
    RK_U32 reg87_ref1_raster_uv_hor_virstride;

    /* SWREG88_REF1_Y_VIRSTRIDE */
    RK_U32 reg88_ref1_virstride;

    /* SWREG89_REF2_Y_HOR_VIRSTRIDE */
    RK_U32 reg89_ref2_hor_virstride;

    /* SWREG90_REF2_UV_HOR_VIRSTRIDE */
    RK_U32 reg90_ref2_raster_uv_hor_virstride;

    /* SWREG91_REF2_Y_VIRSTRIDE */
    RK_U32 reg91_ref2_virstride;

    /* SWREG92_REF3_Y_HOR_VIRSTRIDE */
    RK_U32 reg92_ref3_hor_virstride;

    /* SWREG93_REF3_UV_HOR_VIRSTRIDE */
    RK_U32 reg93_ref3_raster_uv_hor_virstride;

    /* SWREG94_REF3_Y_VIRSTRIDE */
    RK_U32 reg94_ref3_virstride;

    /* SWREG95_REF4_Y_HOR_VIRSTRIDE */
    RK_U32 reg95_ref4_hor_virstride;

    /* SWREG96_REF4_UV_HOR_VIRSTRIDE */
    RK_U32 reg96_ref4_raster_uv_hor_virstride;

    /* SWREG97_REF4_Y_VIRSTRIDE */
    RK_U32 reg97_ref4_virstride;

    /* SWREG98_REF5_Y_HOR_VIRSTRIDE */
    RK_U32 reg98_ref5_hor_virstride;

    /* SWREG99_REF5_UV_HOR_VIRSTRIDE */
    RK_U32 reg99_ref5_raster_uv_hor_virstride;

    /* SWREG100_REF5_Y_VIRSTRIDE */
    RK_U32 reg100_ref5_virstride;

    /* SWREG101_REF6_Y_HOR_VIRSTRIDE */
    RK_U32 reg101_ref6_hor_virstride;

    /* SWREG102_REF6_UV_HOR_VIRSTRIDE */
    RK_U32 reg102_ref6_raster_uv_hor_virstride;

    /* SWREG103_REF6_Y_VIRSTRIDE */
    RK_U32 reg103_ref6_virstride;

    /* SWREG104_REF7_Y_HOR_VIRSTRIDE */
    RK_U32 reg104_ref7_hor_virstride;

    /* SWREG105_REF7_UV_HOR_VIRSTRIDE */
    RK_U32 reg105_ref7_raster_uv_hor_virstride;

    /* SWREG106_REF7_Y_VIRSTRIDE */
    RK_U32 reg106_ref7_virstride;

} Vdpu383RegAv1dParas;

typedef struct Vdpu383RegAv1dAddr_t {
    /* SWREG168_DECOUT_BASE */
    RK_U32 reg168_decout_base;

    /* SWREG169_ERROR_REF_BASE */
    RK_U32 reg169_error_ref_base;

    /* SWREG170_REF0_BASE */
    // RK_U32 reg170_refer0_base;
    RK_U32 reg170_av1_last_base;

    /* SWREG171_REF1_BASE */
    // RK_U32 reg171_refer1_base;
    RK_U32 reg171_av1golden_base;

    /* SWREG172_REF2_BASE */
    // RK_U32 reg172_refer2_base;
    RK_U32 reg172_av1alfter_base;

    /* SWREG173_REF3_BASE */
    RK_U32 reg173_refer3_base;

    /* SWREG174_REF4_BASE */
    RK_U32 reg174_refer4_base;

    /* SWREG175_REF5_BASE */
    RK_U32 reg175_refer5_base;

    /* SWREG176_REF6_BASE */
    RK_U32 reg176_refer6_base;

    /* SWREG177_REF7_BASE */
    RK_U32 reg177_refer7_base;

    /* SWREG178_H26X_REF8_BASE */
    // RK_U32 reg178_refer8_base;
    RK_U32 reg178_av1_coef_rd_base;

    /* SWREG179_H26X_REF9_BASE */
    // RK_U32 reg179_refer9_base;
    RK_U32 reg179_av1_coef_wr_base;

    /* SWREG180_H26X_REF10_BASE */
    RK_U32 reg180_refer10_base;

    /* SWREG181_H26X_REF11_BASE */
    RK_U32 reg181_av1_rd_segid_base;

    /* SWREG182_H26X_REF12_BASE */
    RK_U32 reg182_av1_wr_segid_base;

    /* SWREG183_H26X_REF13_BASE */
    RK_U32 reg183_kf_prob_base;

    /* SWREG184_H26X_REF14_BASE */
    RK_U32 reg184_av1_noncoef_rd_base;

    /* SWREG185_H26X_REF15_BASE */
    RK_U32 reg185_av1_noncoef_wr_base;

    RK_U32 reserve_reg186_191[6];

    /* SWREG192_PAYLOAD_ST_CUR_BASE */
    RK_U32 reg192_payload_st_cur_base;

    /* SWREG193_FBC_PAYLOAD_OFFSET */
    RK_U32 reg193_fbc_payload_offset;

    /* SWREG194_PAYLOAD_ST_ERROR_REF_BASE */
    RK_U32 reg194_payload_st_error_ref_base;

    /* SWREG195_PAYLOAD_ST_REF0_BASE */
    RK_U32 reg195_payload_st_ref0_base;

    /* SWREG196_PAYLOAD_ST_REF1_BASE */
    RK_U32 reg196_payload_st_ref1_base;

    /* SWREG197_PAYLOAD_ST_REF2_BASE */
    RK_U32 reg197_payload_st_ref2_base;

    /* SWREG198_PAYLOAD_ST_REF3_BASE */
    RK_U32 reg198_payload_st_ref3_base;

    /* SWREG199_PAYLOAD_ST_REF4_BASE */
    RK_U32 reg199_payload_st_ref4_base;

    /* SWREG200_PAYLOAD_ST_REF5_BASE */
    RK_U32 reg200_payload_st_ref5_base;

    /* SWREG201_PAYLOAD_ST_REF6_BASE */
    RK_U32 reg201_payload_st_ref6_base;

    /* SWREG202_PAYLOAD_ST_REF7_BASE */
    RK_U32 reg202_payload_st_ref7_base;

    /* SWREG203_PAYLOAD_ST_REF8_BASE */
    RK_U32 reg203_payload_st_ref8_base;

    /* SWREG204_PAYLOAD_ST_REF9_BASE */
    RK_U32 reg204_payload_st_ref9_base;

    /* SWREG205_PAYLOAD_ST_REF10_BASE */
    RK_U32 reg205_payload_st_ref10_base;

    /* SWREG206_PAYLOAD_ST_REF11_BASE */
    RK_U32 reg206_payload_st_ref11_base;

    /* SWREG207_PAYLOAD_ST_REF12_BASE */
    RK_U32 reg207_payload_st_ref12_base;

    /* SWREG208_PAYLOAD_ST_REF13_BASE */
    RK_U32 reg208_payload_st_ref13_base;

    /* SWREG209_PAYLOAD_ST_REF14_BASE */
    RK_U32 reg209_payload_st_ref14_base;

    /* SWREG210_PAYLOAD_ST_REF15_BASE */
    RK_U32 reg210_payload_st_ref15_base;

    RK_U32 reserve_reg211_215[5];

    /* SWREG216_COLMV_CUR_BASE */
    RK_U32 reg216_colmv_cur_base;

    /* SWREG217_232_COLMV_REF0_BASE */
    RK_U32 reg217_232_colmv_ref_base[16];
} Vdpu383RegAv1dAddr;

typedef struct Vdpu383Av1dRegSet_t {
    Vdpu383RegVersion     reg_version;
    Vdpu383CtrlReg        ctrl_regs;         /* 8-30 */
    Vdpu383RegCommonAddr  common_addr;       /* 128-134, 140-161 */
    Vdpu383RegAv1dParas   av1d_paras;        /* 64-106 */
    Vdpu383RegAv1dAddr    av1d_addrs;        /* 168-185(ref) */
    /* 192-210(fbc) */
    /* 216-232(col mv) */
} Vdpu383Av1dRegSet;

#endif /* VDPU383_AV1D_H */
