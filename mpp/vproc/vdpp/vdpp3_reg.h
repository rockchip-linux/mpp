/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPP3_REG_H
#define VDPP3_REG_H

#include "vdpp2_reg.h"

#define VDPP3_REG_OFF_DMSR               (0x0080)
#define VDPP3_REG_OFF_DCI_HIST           (0x00E0)
#define VDPP3_REG_OFF_ES                 (0x0100)
#define VDPP3_REG_OFF_PYRAMID            (0x0190)
#define VDPP3_REG_OFF_BBD                (0x01B0)
#define VDPP3_REG_OFF_SHARP              (0x0200)
#define VDPP3_REG_OFF_ZME                (0x2800)


/* zme: 0x2800 - 0x2844 */
typedef struct vdpp3_zme_reg {
    struct {
        RK_U32 bypass_en        : 1;
        RK_U32 align_en         : 1;
        RK_U32 reserved_1       : 2;
        RK_U32 format_in        : 4;
        RK_U32 format_out       : 4;
        RK_U32 reserved_2       : 19;
        RK_U32 auto_gating_en   : 1;
    } reg0;         /* 0x0800 */

    /* reg1 0x0804 - reg2 0x080C */
    RK_U32 reg1_3[3];

    struct {
        RK_U32 yrgb_xsd_en       : 1;
        RK_U32 yrgb_xsu_en       : 1;
        RK_U32 yrgb_scl_mode     : 2;
        RK_U32 yrgb_ysd_en       : 1;
        RK_U32 yrgb_ysu_en       : 1;
        RK_U32 yrgb_yscl_mode    : 2;
        RK_U32 yrgb_dering_en    : 1;
        RK_U32 yrgb_gt_en        : 1;
        RK_U32 yrgb_gt_mode      : 2;
        RK_U32 reserved_1        : 4;
        RK_U32 yrgb_xsd_bypass   : 1;
        RK_U32 yrgb_ys_bypass    : 1;
        RK_U32 yrgb_xsu_bypass   : 1;
        RK_U32 reserved_2        : 1;
        RK_U32 yrgb_xscl_coe_sel : 4;
        RK_U32 yrgb_yscl_coe_sel : 4;
        RK_U32 reserved_3        : 4;
    } reg4;         /* 0x0810 */

    /* reg5 0x0814 - reg6 x0818 */
    RK_U32 reg5_6[2];

    struct {
        RK_U32 yrgb_dering_sen0 : 5;
        RK_U32 reserved_1       : 3;
        RK_U32 yrgb_dering_sen1 : 5;
        RK_U32 reserved_2       : 3;
        RK_U32 yrgb_dering_alpha: 5;
        RK_U32 reserved_3       : 3;
        RK_U32 yrgb_dering_delta: 5;
        RK_U32 reserved_4       : 3;
    } reg7;         /* 0x081C */

    struct {
        RK_U32 yrgb_xscl_factor : 16;
        RK_U32 yrgb_xscl_offset : 16;
    } reg8;         /* 0x0820 */

    struct {
        RK_U32 yrgb_yscl_factor : 16;
        RK_U32 yrgb_yscl_offset : 16;
    } reg9;         /* 0x0824 */

    /* reg10 0x0828 - reg11 0x082C */
    RK_U32 reg10_11[2];   /* 0x0828 */

    struct {
        RK_U32 cbcr_xsd_en      : 1;
        RK_U32 cbcr_xsu_en      : 1;
        RK_U32 cbcr_xscl_mode   : 2;
        RK_U32 cbcr_ysd_en      : 1;
        RK_U32 cbcr_ysu_en      : 1;
        RK_U32 cbcr_yscl_mode   : 2;
        RK_U32 cbcr_dering_en   : 1;
        RK_U32 cbcr_gt_en       : 1;
        RK_U32 cbcr_gt_mode     : 2;
        RK_U32 reserved_1       : 4;
        RK_U32 cbcr_xsd_bypass  : 1;
        RK_U32 cbcr_ys_bypass   : 1;
        RK_U32 cbcr_xsu_bypass  : 1;
        RK_U32 reserved_2       : 1;
        RK_U32 cbcr_xscl_coe_sel: 4;
        RK_U32 cbcr_yscl_coe_sel: 4;
        RK_U32 reserved_3       : 4;
    } reg12;         /* 0x0830 */

    /* reg13 0x0834 - reg14 0x0838 */
    RK_U32 reg13_14[2];

    struct {
        RK_U32 cbcr_dering_sen0 : 5;
        RK_U32 reserved_1       : 3;
        RK_U32 cbcr_dering_sen1 : 5;
        RK_U32 reserved_2       : 3;
        RK_U32 cbcr_dering_alpha: 5;
        RK_U32 reserved_3       : 3;
        RK_U32 cbcr_dering_delta: 5;
        RK_U32 reserved_4       : 3;
    } reg15;         /* 0x083C */

    struct {
        RK_U32 cbcr_xscl_factor : 16;
        RK_U32 cbcr_xscl_offset : 16;
    } reg16;         /* 0x0840 */

    struct {
        RK_U32 cbcr_yscl_factor : 16;
        RK_U32 cbcr_yscl_offset : 16;
    } reg17;         /* 0x0844 */
} RegZme3;

/* pyramid: 0x1190 - 0x11A8 */
typedef struct vdpp3_pyr_reg {
    struct {
        RK_U32 sw_pyramid_en                      : 1;
        RK_U32 reserved                           : 31;
    } reg0;           // 0x0190

    struct {
        RK_U32 sw_pyramid_layer1_dst_addr         : 32;
    } reg1;           // 0x0194

    struct {
        RK_U32 sw_pyramid_layer1_dst_vir_wid      : 16;
        RK_U32 reserved                           : 16;
    } reg2;           // 0x0198

    struct {
        RK_U32 sw_pyramid_layer2_dst_addr         : 32;
    } reg3;           // 0x019C

    struct {
        RK_U32 sw_pyramid_layer2_dst_vir_wid      : 16;
        RK_U32 reserved                           : 16;
    } reg4;           // 0x01A0

    struct {
        RK_U32 sw_pyramid_layer3_dst_addr         : 32;
    } reg5;           // 0x01A4

    struct {
        RK_U32 sw_pyramid_layer3_dst_vir_wid      : 16;
        RK_U32 reserved                           : 16;
    } reg6;           // 0x01A8
} RegPyr;

/* bbd: 0x11B0 - 0x11B8 */
typedef struct vdpp3_bbd_reg {
    struct {
        RK_U32 sw_det_en                          : 1;
        RK_U32 reserved1                          : 3;
        RK_U32 sw_det_blcklmt                     : 8;
        RK_U32 reserved2                          : 20;
    } reg0;           // 0x01B0

    struct {
        RK_U32 blckbar_size_top                   : 12;
        RK_U32 reserved1                          : 4;
        RK_U32 blckbar_size_bottom                : 12;
        RK_U32 reserved2                          : 4;
    } reg1;           // 0x01B4

    struct {
        RK_U32 blckbar_size_left                  : 12;
        RK_U32 reserved1                          : 4;
        RK_U32 blckbar_size_right                 : 12;
        RK_U32 reserved2                          : 4;
    } reg2;           // 0x01B8
} RegBbd;

typedef struct vdpp3_reg {
    RegCom2 common; /* offset: 0x1000, end: 0x106C, size: 112 */
    RegDmsr dmsr;   /* offset: 0x1080, end: 0x10D0, size: 84 */
    RegHist hist;   /* offset: 0x10E0, end: 0x10F0, size: 20 */
    RegEs   es;     /* offset: 0x1100, end: 0x117C, size: 128 */
    RegShp  sharp;  /* offset: 0x1200, end: 0x14a8, size: 684 */
    RegPyr  pyr;    /* offset: 0x0190, end: 0x11A8, size: 28 */
    RegBbd  bbd;    /* offset: 0x01B0, end: 0x11B8, size: 12 */
    RegZme3 zme;    /* offset: 0x2000, end: 0x2844, size: 1160(valid)/2120(total) */
} Vdpp3Regs;

#endif /* VDPP3_REG_H */
