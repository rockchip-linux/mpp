/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_JPEGE_VEPU511_REG_H__
#define __HAL_JPEGE_VEPU511_REG_H__

#include "rk_type.h"
#include "vepu511_common.h"

typedef struct Vepu511JpegRoiRegion_t {
    struct {
        RK_U32 roi0_rdoq_start_x    : 11;
        RK_U32 roi0_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi0_rdoq_level      : 6;
        RK_U32 roi0_rdoq_en         : 1;
    } roi_cfg0;

    struct {
        RK_U32 roi0_rdoq_width_m1     : 11;
        RK_U32 roi0_rdoq_height_m1    : 11;
        /* the below 10 bits only for roi0 */
        RK_U32 reserved               : 3;
        RK_U32 frm_rdoq_level         : 6;
        RK_U32 frm_rdoq_en            : 1;
    } roi_cfg1;
} Vepu511JpegRoiRegion;

/* 0x00000400 reg256 - 0x0000050c reg323*/
typedef struct Vepu511JpegReg_t {
    /* 0x00000400 reg256 */
    RK_U32  adr_bsbt;

    /* 0x00000404 reg257 */
    RK_U32  adr_bsbb;

    /* 0x00000408 reg258 */
    RK_U32 adr_bsbs;

    /* 0x0000040c reg259 */
    RK_U32 adr_bsbr;

    /* 0x00000410 reg260 */
    RK_U32 adr_vsy_b;

    /* 0x00000414 reg261 */
    RK_U32 adr_vsc_b;

    /* 0x00000418 reg262 */
    RK_U32 adr_vsy_t;

    /* 0x0000041c reg263 */
    RK_U32 adr_vsc_t;

    /* 0x00000420 reg264 */
    RK_U32 adr_src0;

    /* 0x00000424 reg265 */
    RK_U32 adr_src1;

    /* 0x00000428 reg266 */
    RK_U32 adr_src2;

    /* 0x0000042c reg267 */
    RK_U32 bsp_size;

    /* 0x430 - 0x43c */
    RK_U32 reserved268_271[4];

    /* 0x00000440 reg272 */
    struct {
        RK_U32 pic_wd8_m1    : 11;
        RK_U32 reserved      : 1;
        RK_U32 pp0_vnum_m1   : 4;
        RK_U32 pic_hd8_m1    : 11;
        RK_U32 reserved1     : 1;
        RK_U32 pp0_jnum_m1   : 4;
    } enc_rsl;

    /* 0x00000444 reg273 */
    struct {
        RK_U32 pic_wfill    : 6;
        RK_U32 reserved     : 10;
        RK_U32 pic_hfill    : 6;
        RK_U32 reserved1    : 10;
    } src_fill;

    /* 0x00000448 reg274 */
    struct {
        RK_U32 alpha_swap            : 1;
        RK_U32 rbuv_swap             : 1;
        RK_U32 src_cfmt              : 4;
        RK_U32 reserved              : 2;
        RK_U32 src_range_trns_en     : 1;
        RK_U32 src_range_trns_sel    : 1;
        RK_U32 chroma_ds_mode        : 1;
        RK_U32 reserved1             : 21;
    } src_fmt;

    /* 0x0000044c reg275 */
    struct {
        RK_U32 csc_wgt_b2y    : 9;
        RK_U32 csc_wgt_g2y    : 9;
        RK_U32 csc_wgt_r2y    : 9;
        RK_U32 reserved       : 5;
    } src_udfy;

    /* 0x00000450 reg276 */
    struct {
        RK_U32 csc_wgt_b2u    : 9;
        RK_U32 csc_wgt_g2u    : 9;
        RK_U32 csc_wgt_r2u    : 9;
        RK_U32 reserved       : 5;
    } src_udfu;

    /* 0x00000454 reg277 */
    struct {
        RK_U32 csc_wgt_b2v    : 9;
        RK_U32 csc_wgt_g2v    : 9;
        RK_U32 csc_wgt_r2v    : 9;
        RK_U32 reserved       : 5;
    } src_udfv;

    /* 0x00000458 reg278 */
    struct {
        RK_U32 csc_ofst_v    : 8;
        RK_U32 csc_ofst_u    : 8;
        RK_U32 csc_ofst_y    : 5;
        RK_U32 reserved      : 11;
    } src_udfo;

    /* 0x0000045c reg279 */
    struct {
        RK_U32 cr_force_value     : 8;
        RK_U32 cb_force_value     : 8;
        RK_U32 chroma_force_en    : 1;
        RK_U32 reserved           : 9;
        RK_U32 src_mirr           : 1;
        RK_U32 src_rot            : 2;
        RK_U32 reserved1          : 1;
        RK_U32 rkfbcd_en          : 1;
        RK_U32 reserved2          : 1;
    } src_proc;

    /* 0x00000460 reg280 */
    struct {
        RK_U32 pic_ofst_x    : 14;
        RK_U32 reserved      : 2;
        RK_U32 pic_ofst_y    : 14;
        RK_U32 reserved1     : 2;
    } pic_ofst;

    /* 0x00000464 reg281 */
    struct {
        RK_U32 src_strd0    : 21;
        RK_U32 reserved     : 11;
    } src_strd0;

    /* 0x00000468 reg282 */
    struct {
        RK_U32 src_strd1    : 16;
        RK_U32 reserved     : 16;
    } src_strd1;

    /* 0x0000046c reg283 */
    struct {
        RK_U32 pp_corner_filter_strength      : 2;
        RK_U32 reserved                       : 2;
        RK_U32 pp_edge_filter_strength        : 2;
        RK_U32 reserved1                      : 2;
        RK_U32 pp_internal_filter_strength    : 2;
        RK_U32 reserved2                      : 22;
    } src_flt_cfg;

    /* 0x00000470 reg284 */
    struct {
        RK_U32 bias_y    : 15;
        RK_U32 reserved  : 17;
    } y_cfg;

    /* 0x00000474 reg285 */
    struct {
        RK_U32 bias_u    : 15;
        RK_U32 reserved  : 17;
    } u_cfg;

    /* 0x00000478 reg286 */
    struct {
        RK_U32 bias_v    : 15;
        RK_U32 reserved  : 17;
    } v_cfg;

    /* 0x0000047c reg287 */
    struct {
        RK_U32 ri              : 25;
        RK_U32 out_mode        : 1;
        RK_U32 start_rst_m     : 3;
        RK_U32 pic_last_ecs    : 1;
        RK_U32 reserved        : 1;
        RK_U32 stnd            : 1;
    } base_cfg;

    /* 0x00000480 reg288 */
    struct {
        RK_U32 uvc_partition0_len    : 12;
        RK_U32 uvc_partition_len     : 12;
        RK_U32 uvc_skip_len          : 6;
        RK_U32 reserved              : 2;
    } uvc_cfg;

    /* 0x00000484 reg289 */
    struct {
        RK_U32 reserved     : 4;
        RK_U32 eslf_badr    : 28;
    } adr_eslf;

    /* 0x00000488 reg290 */
    struct {
        RK_U32 eslf_rptr    : 10;
        RK_U32 eslf_wptr    : 10;
        RK_U32 eslf_blen    : 10;
        RK_U32 eslf_updt    : 2;
    } eslf_buf;

    /* 0x48c */
    RK_U32 reserved_291;

    /* 0x00000490 reg292 - 0x0000050c reg323*/
    Vepu511JpegRoiRegion roi_regions[16];
} Vepu511JpegReg;

/* 0x00002ca0 reg2856 - - 0x00002e1c reg2951 */
typedef struct JpegVepu511Tab_t {
    RK_U16 qua_tab0[64];
    RK_U16 qua_tab1[64];
    RK_U16 qua_tab2[64];
} JpegVepu511Tab;

typedef struct Vepu511JpegOsdCfg_t {
    /* 0x00003138 reg3150 */
    struct {
        RK_U32 osd_en                : 1;
        RK_U32 reserved              : 4;
        RK_U32 osd_qp_adj_en         : 1;
        RK_U32 osd_range_trns_en     : 1;
        RK_U32 osd_range_trns_sel    : 1;
        RK_U32 osd_fmt               : 4;
        RK_U32 osd_alpha_swap        : 1;
        RK_U32 osd_rbuv_swap         : 1;
        RK_U32 reserved1             : 8;
        RK_U32 osd_fg_alpha          : 8;
        RK_U32 osd_fg_alpha_sel      : 2;
    } osd_cfg0;

    /* 0x0000313c reg3151 */
    struct {
        RK_U32 osd_lt_xcrd    : 14;
        RK_U32 osd_lt_ycrd    : 14;
        RK_U32 osd_endn       : 4;
    } osd_cfg1;

    /* 0x00003140 reg3152 */
    struct {
        RK_U32 osd_rb_xcrd    : 14;
        RK_U32 osd_rb_ycrd    : 14;
        RK_U32 reserved       : 4;
    } osd_cfg2;

    /* 0x00003144 reg3153 */
    RK_U32 osd_st_addr;

    /* 0x3148 */
    RK_U32 reserved_3154;

    /* 0x0000314c reg3155 */
    struct {
        RK_U32 osd_stride        : 17;
        RK_U32 reserved          : 8;
        RK_U32 osd_ch_ds_mode    : 1;
        RK_U32 reserved1         : 6;
    } osd_cfg5;

    /* 0x00003150 reg3156 */
    struct {
        RK_U32 osd_v_b_lut0    : 8;
        RK_U32 osd_u_g_lut0    : 8;
        RK_U32 osd_y_r_lut0    : 8;
        RK_U32 osd_v_b_lut1    : 8;
    } osd_cfg6;

    /* 0x00003154 reg3157 */
    struct {
        RK_U32 osd0_u_g_lut1      : 8;
        RK_U32 osd0_y_r_lut1      : 8;
        RK_U32 osd0_alpha_lut0    : 8;
        RK_U32 osd0_alpha_lut1    : 8;
    } osd_cfg7;

    /* 0x3158 */
    RK_U32 reserved_3158;
} JpegVepu511Osd_cfg;

/* 0x00003138 reg3150 - - 0x00003264 reg3225 */
typedef struct Vepu511JpegOsd_t {
    JpegVepu511Osd_cfg osd_cfg[8];
    /* 0x00003258 reg3222 */
    struct {
        RK_U32 osd_csc_yr    : 9;
        RK_U32 osd_csc_yg    : 9;
        RK_U32 osd_csc_yb    : 9;
        RK_U32 reserved      : 5;
    } osd_whi_cfg0;

    /* 0x0000325c reg3223 */
    struct {
        RK_U32 osd_csc_ur    : 9;
        RK_U32 osd_csc_ug    : 9;
        RK_U32 osd_csc_ub    : 9;
        RK_U32 reserved      : 5;
    } osd_whi_cfg1;

    /* 0x00003260 reg3224 */
    struct {
        RK_U32 osd_csc_vr    : 9;
        RK_U32 osd_csc_vg    : 9;
        RK_U32 osd_csc_vb    : 9;
        RK_U32 reserved      : 5;
    } osd_whi_cfg2;

    /* 0x00003264 reg3225 */
    struct {
        RK_U32 osd_csc_ofst_y    : 8;
        RK_U32 osd_csc_ofst_u    : 8;
        RK_U32 osd_csc_ofst_v    : 8;
        RK_U32 reserved          : 8;
    } osd_whi_cfg3;
} JpegVepu511Osd;

/* class: buffer/video syntax */
/* 0x00000270 reg156 - 0x0000050c reg323 */
typedef struct JpegVepu511Base_t {
    /* 0x00000270 reg156  - 0x0000039c reg231 */
    Vepu511FrmCommon    common;

    /* 0x000003a0 reg232 - 0x000003f4 reg253*/
    RK_U32 reserved232_253[22];

    /* 0x000003f8 reg254 */
    struct {
        RK_U32 slice_sta_x      : 9;
        RK_U32 reserved1        : 7;
        RK_U32 slice_sta_y      : 10;
        RK_U32 reserved2        : 5;
        RK_U32 slice_enc_ena    : 1;
    } slice_enc_cfg0;

    /* 0x000003fc reg255 */
    struct {
        RK_U32 slice_end_x    : 9;
        RK_U32 reserved       : 7;
        RK_U32 slice_end_y    : 10;
        RK_U32 reserved1      : 6;
    } slice_enc_cfg1;

    /* 0x00000400 reg256 - 0x0000050c reg323 */
    Vepu511JpegReg jpegReg;
} JpegVepu511Base;

typedef struct JpegV511RegSet_t {
    Vepu511ControlCfg   reg_ctl;
    JpegVepu511Base     reg_base;
    JpegVepu511Tab      jpeg_table;
    Vepu511OsdRegs      reg_osd;
    Vepu511Dbg          reg_dbg;
} JpegV511RegSet;

typedef struct JpegV511Status_t {
    RK_U32 hw_status;
    Vepu511Status st;
} JpegV511Status;

#endif
