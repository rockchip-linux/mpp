/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __VEPU580_COMMON_H__
#define __VEPU580_COMMON_H__

#include "vepu5xx_common.h"

#define VEPU580_SLICE_FIFO_LEN          32
#define VEPU580_OSD_ADDR_IDX_BASE       3092

typedef struct Vepu580OsdPltColor_t {
    /* V component */
    RK_U32  v                       : 8;
    /* U component */
    RK_U32  u                       : 8;
    /* Y component */
    RK_U32  y                       : 8;
    /* Alpha */
    RK_U32  alpha                   : 8;
} Vepu580OsdPltColor;

typedef struct Vepu580OsdPos_t {
    /* X coordinate/16 of OSD region's left-top point. */
    RK_U32  osd_lt_x                : 10;
    RK_U32  reserved0               : 6;
    /* Y coordinate/16 of OSD region's left-top point. */
    RK_U32  osd_lt_y                : 10;
    RK_U32  reserved1               : 6;
    /* X coordinate/16 of OSD region's right-bottom point. */
    RK_U32  osd_rb_x                : 10;
    RK_U32  reserved2               : 6;
    /* Y coordinate/16 of OSD region's right-bottom point. */
    RK_U32  osd_rb_y                : 10;
    RK_U32  reserved3               : 6;
} Vepu580OsdPos;

typedef struct Vepu580OsdReg_t {
    /*
     * OSD_INV_CFG
     * Address offset: 0x00003000 Access type: read and write
     * OSD color inverse  configuration
     */
    struct {
        /*
         * OSD color inverse enable of luma component,
         * each bit controls corresponding region.
         */
        RK_U32  osd_lu_inv_en           : 8;

        /* OSD color inverse enable of chroma component,
        * each bit controls corresponding region.
        */
        RK_U32  osd_ch_inv_en               : 8;
        /*
         * OSD color inverse expression switch for luma component
         * each bit controls corresponding region.
         * 1'h0: Expression need to determine the condition;
         * 1'h1: Expression don't need to determine the condition;
         */
        RK_U32  osd_lu_inv_msk          : 8;
        /*
         * OSD color inverse expression switch for chroma component
         * each bit controls corresponding region.
         * 1'h0: Expression need to determine the condition;
         * 1'h1: Expression don't need to determine the condition;
         */
        RK_U32  osd_ch_inv_msk          : 8;
    } reg3072;

    /*
     * OSD_INV
     * Address offset: 0x3004 Access type: read and write
     * OSD color inverse configuration
     */
    struct {
        /* Color inverse theshold for OSD region0. */
        RK_U32  osd_ithd_r0             : 4;
        /* Color inverse theshold for OSD region1. */
        RK_U32  osd_ithd_r1             : 4;
        /* Color inverse theshold for OSD region2. */
        RK_U32  osd_ithd_r2             : 4;
        /* Color inverse theshold for OSD region3. */
        RK_U32  osd_ithd_r3             : 4;
        /* Color inverse theshold for OSD region4. */
        RK_U32  osd_ithd_r4             : 4;
        /* Color inverse theshold for OSD region5. */
        RK_U32  osd_ithd_r5             : 4;
        /* Color inverse theshold for OSD region6. */
        RK_U32  osd_ithd_r6             : 4;
        /* Color inverse theshold for OSD region7. */
        RK_U32  osd_ithd_r7             : 4;
    } reg3073;

    /*
     * OSD_CFG
     * Address offset: 0x3008 Access type: read and write
     * OSD configuration
     */
    struct {
        /* OSD region enable, each bit controls corresponding OSD region. */
        RK_U32  osd_e                   : 8;
        /*
         * OSD color inverse expression type
         * each bit controls corresponding region.
         * 1'h0: AND;
         * 1'h1: OR
         */
        RK_U32  osd_itype           : 8;
        /*
         * OSD palette clock selection.
         * 1'h0: Configure bus clock domain.
         * 1'h1: Core clock domain.
         */
        RK_U32  osd_plt_cks             : 1;
        /*
         * OSD palette type.
         * 1'h1: Default type.
         * 1'h0: User defined type.
         */
        RK_U32  osd_plt_typ             : 1;
        RK_U32  reserved                : 14;
    } reg3074;

    RK_U32 reserved_3075;
    /*
     * OSD_POS reg3076_reg3091
     * Address offset: 0x3010~0x304c Access type: read and write
     * OSD region position
     */
    Vepu580OsdPos  osd_pos[8];

    /*
     * ADR_OSD reg3092_reg3099
     * Address offset: 0x00003050~reg306c Access type: read and write
     * Base address for OSD region, 16B aligned
     */
    RK_U32  osd_addr[8];

    RK_U32 reserved3100_3103[4];
    Vepu580OsdPltColor plt_data[256];
} Vepu580OsdReg;

MPP_RET vepu580_set_osd(Vepu5xxOsdCfg *cfg);

#endif /* __VEPU580_COMMON_H__ */
