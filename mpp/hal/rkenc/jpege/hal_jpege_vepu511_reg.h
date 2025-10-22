/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_JPEGE_VEPU511_REG_H
#define HAL_JPEGE_VEPU511_REG_H

#include "rk_type.h"
#include "vepu511_common.h"

#define VEPU511_JPEGE_FRAME_OFFSET      (0x300)
#define VEPU511_JPEGE_TABLE_OFFSET      (0x2ca0)
#define VEPU511_JPEGE_OSD_OFFSET        (0x3138)
#define VEPU511_JPEGE_STATUS_OFFSET     (0x4000)
#define VEPU511_JPEGE_INT_OFFSET        (0x2c)

typedef struct JpegVepu511Frame_t {
    /* 0x00000300 reg192 */
    struct {
        RK_U32 enc_stnd                : 2;
        RK_U32 cur_frm_ref             : 1;
        RK_U32 mei_stor                : 1;
        RK_U32 bs_scp                  : 1;
        RK_U32 meiw_mode               : 1;
        RK_U32 reserved0               : 2;
        RK_U32 pic_qp                  : 6;
        RK_U32 num_pic_tot_cur_hevc    : 5;
        RK_U32 log2_ctu_num_hevc       : 5;
        RK_U32 rfpr_compress_mode      : 1;
        RK_U32 reserved1               : 2;
        RK_U32 eslf_out_e_jpeg         : 1;
        RK_U32 jpeg_slen_fifo          : 1;
        RK_U32 eslf_out_e              : 1;
        RK_U32 slen_fifo               : 1;
        RK_U32 rec_fbc_dis             : 1;
    } video_enc_pic;

    RK_U32 reserved_reg304;

    /* 0x00000308 reg194 */
    struct {
        RK_U32 frame_id        : 8;
        RK_U32 frm_id_match    : 1;
        RK_U32 reserved        : 3;
        RK_U32 source_id       : 1;
        RK_U32 src_id_match    : 1;
        RK_U32 reserved1       : 2;
        RK_U32 ch_id           : 2;
        RK_U32 vrsp_rtn_en     : 1;
        RK_U32 vinf_req_en     : 1;
        RK_U32 reserved2       : 12;
    } enc_id;

    /* 0x0000030c reg195 - 0x000003fc reg255 */
    RK_U32 reserved195_255[61];

    /* 0x00000400 reg256 */
    RK_U32 adr_bsbt;

    /* 0x00000404 reg257 */
    RK_U32 adr_bsbb;

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
        RK_U32 jpeg_ri              : 25;
        RK_U32 jpeg_out_mode        : 1;
        RK_U32 jpeg_start_rst_m     : 3;
        RK_U32 jpeg_pic_last_ecs    : 1;
        RK_U32 reserved             : 1;
        RK_U32 jpeg_stnd            : 1;
    } base_cfg;

    /* 0x00000480 reg288 */
    struct {
        RK_U32 uvc_partition0_len    : 12;
        RK_U32 uvc_partition_len     : 12;
        RK_U32 uvc_skip_len          : 6;
        RK_U32 reserved              : 2;
    } uvc_cfg;

    /* 0x00000484 reg289 */
    RK_U32 adr_eslf;

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
    Vepu511JpegRoi roi_cfg[16];
} JpegVepu511Frame;

typedef struct JpegV511RegSet_t {
    Vepu511ControlCfg   reg_ctl;
    /* 0x300 - 0x50c */
    JpegVepu511Frame    reg_frm;
    /* 0x2ca0 - 0x2e1c */
    Vepu511JpegTable    reg_table;
    /* 0x00003138 reg3150 - 0x00003264 reg3225 */
    Vepu511Osd          reg_osd;
    /* 0x00005000 reg5120 - 0x00005230 reg5260*/
    Vepu511Dbg          reg_dbg;
} JpegV511RegSet;

typedef struct JpegV511Status_t {
    RK_U32 hw_status;
    Vepu511Status st;
} JpegV511Status;

#endif
