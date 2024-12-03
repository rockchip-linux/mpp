/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H264E_VEPU511_REG_H__
#define __HAL_H264E_VEPU511_REG_H__

#include "rk_type.h"
#include "vepu511_common.h"

/* class: buffer/video syntax */
/* 0x00000270 reg156 - 0x00000538 reg334 */
typedef struct H264eVepu511Frame_t {
    /* 0x00000270 reg156  - 0x0000039c reg231 */
    Vepu511FrmCommon common;

    /* 0x000003a0 reg232 */
    struct {
        RK_U32 rect_size         : 1;
        RK_U32 reserved          : 2;
        RK_U32 vlc_lmt           : 1;
        RK_U32 reserved1         : 9;
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
        RK_U32 rdo_mark_mode         : 9;
        RK_U32 reserved              : 5;
        RK_U32 p16_interp_num        : 2;
        RK_U32 p16t8_rdo_num         : 2;
        RK_U32 p16t4_rmd_num         : 2;
        RK_U32 p8_interp_num         : 2;
        RK_U32 p8t8_rdo_num          : 2;
        RK_U32 p8t4_rmd_num          : 2;
        RK_U32 iframe_i16_rdo_num    : 2;
        RK_U32 i8_rdo_num            : 2;
        RK_U32 iframe_i4_rdo_num     : 2;
    } rdo_mark_mode;

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

    /* 0x000003d4 reg245 - 0x0x00000 reg251 */
    RK_U32 reserved245_251[7];

    /* 0x000003f0 reg252 */
    struct {
        RK_U32 mv_v_lmt_thd    : 14;
        RK_U32 reserved        : 1;
        RK_U32 mv_v_lmt_en     : 1;
        RK_U32 reserved1       : 16;
    } sli_cfg;

    /* 0x000003f4 reg253 */
    RK_U32 reserved253;

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

    /* 0x00000400 reg256 */
    struct {
        RK_U32 reserved          : 8;
        RK_U32 bsbt_addr_jpeg    : 24;
    } adr_bsbt_jpeg;

    /* 0x00000404 reg257 */
    struct {
        RK_U32 reserved          : 8;
        RK_U32 bsbb_addr_jpeg    : 24;
    } adr_bsbb_jpeg;

    /* 0x00000408 reg258 */
    RK_U32 adr_bsbs_jpeg;

    /* 0x0000040c reg259 */
    struct {
        RK_U32 bsadr_msk_jpeg    : 4;
        RK_U32 reserved          : 4;
        RK_U32 bsbr_addr_jpeg    : 24;
    } adr_bsbr_jpeg;

    /* 0x00000410 reg260 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsy_b_jpeg    : 28;
    } adr_vsy_b_jpeg;

    /* 0x00000414 reg261 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsc_b_jpeg    : 28;
    } adr_vsc_b_jpeg;

    /* 0x00000418 reg262 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsy_t_jpeg    : 28;
    } adr_vsy_t_jpeg;

    /* 0x0000041c reg263 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 adr_vsc_t_jpeg    : 28;
    } adr_vsc_t_jpeg;

    /* 0x00000420 reg264 */
    RK_U32 adr_src0_jpeg;

    /* 0x00000424 reg265 */
    RK_U32 adr_src1_jpeg;

    /* 0x00000428 reg266 */
    RK_U32 adr_src2_jpeg;

    /* 0x0000042c reg267 */
    RK_U32 bsp_size_jpeg;

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
    } enc_rsl_jpeg;

    /* 0x00000444 reg273 */
    struct {
        RK_U32 pic_wfill_jpeg    : 6;
        RK_U32 reserved          : 10;
        RK_U32 pic_hfill_jpeg    : 6;
        RK_U32 reserved1         : 10;
    } src_fill_jpeg;

    /* 0x00000448 reg274 */
    struct {
        RK_U32 alpha_swap_jpeg            : 1;
        RK_U32 rbuv_swap_jpeg             : 1;
        RK_U32 src_cfmt_jpeg              : 4;
        RK_U32 reserved                   : 2;
        RK_U32 src_range_trns_en_jpeg     : 1;
        RK_U32 src_range_trns_sel_jpeg    : 1;
        RK_U32 chroma_ds_mode_jpeg        : 1;
        RK_U32 reserved1                  : 21;
    } src_fmt_jpeg;

    /* 0x0000044c reg275 */
    struct {
        RK_U32 csc_wgt_b2y_jpeg    : 9;
        RK_U32 csc_wgt_g2y_jpeg    : 9;
        RK_U32 csc_wgt_r2y_jpeg    : 9;
        RK_U32 reserved            : 5;
    } src_udfy_jpeg;

    /* 0x00000450 reg276 */
    struct {
        RK_U32 csc_wgt_b2u_jpeg    : 9;
        RK_U32 csc_wgt_g2u_jpeg    : 9;
        RK_U32 csc_wgt_r2u_jpeg    : 9;
        RK_U32 reserved            : 5;
    } src_udfu_jpeg;

    /* 0x00000454 reg277 */
    struct {
        RK_U32 csc_wgt_b2v_jpeg    : 9;
        RK_U32 csc_wgt_g2v_jpeg    : 9;
        RK_U32 csc_wgt_r2v_jpeg    : 9;
        RK_U32 reserved            : 5;
    } src_udfv_jpeg;

    /* 0x00000458 reg278 */
    struct {
        RK_U32 csc_ofst_v_jpeg    : 8;
        RK_U32 csc_ofst_u_jpeg    : 8;
        RK_U32 csc_ofst_y_jpeg    : 5;
        RK_U32 reserved           : 11;
    } src_udfo_jpeg;

    /* 0x0000045c reg279 */
    struct {
        RK_U32 cr_force_value_jpeg     : 8;
        RK_U32 cb_force_value_jpeg     : 8;
        RK_U32 chroma_force_en_jpeg    : 1;
        RK_U32 reserved                : 9;
        RK_U32 src_mirr_jpeg           : 1;
        RK_U32 src_rot_jpeg            : 2;
        RK_U32 reserved1               : 1;
        RK_U32 rkfbcd_en_jpeg          : 1;
        RK_U32 reserved2               : 1;
    } src_proc_jpeg;

    /* 0x00000460 reg280 */
    struct {
        RK_U32 pic_ofst_x_jpeg    : 14;
        RK_U32 reserved           : 2;
        RK_U32 pic_ofst_y_jpeg    : 14;
        RK_U32 reserved1          : 2;
    } pic_ofst_jpeg;

    /* 0x00000464 reg281 */
    struct {
        RK_U32 src_strd0_jpeg    : 21;
        RK_U32 reserved          : 11;
    } src_strd0_jpeg;

    /* 0x00000468 reg282 */
    struct {
        RK_U32 src_strd1_jpeg    : 16;
        RK_U32 reserved          : 16;
    } src_strd1_jpeg;

    /* 0x0000046c reg283 */
    struct {
        RK_U32 pp_corner_filter_strength_jpeg      : 2;
        RK_U32 reserved                            : 2;
        RK_U32 pp_edge_filter_strength_jpeg        : 2;
        RK_U32 reserved1                           : 2;
        RK_U32 pp_internal_filter_strength_jpeg    : 2;
        RK_U32 reserved2                           : 22;
    } src_flt_cfg_jpeg;

    /* 0x00000470 reg284 */
    struct {
        RK_U32 jpeg_bias_y    : 15;
        RK_U32 reserved       : 17;
    } jpeg_y_cfg;

    /* 0x00000474 reg285 */
    struct {
        RK_U32 jpeg_bias_u    : 15;
        RK_U32 reserved       : 17;
    } jpeg_u_cfg;

    /* 0x00000478 reg286 */
    struct {
        RK_U32 jpeg_bias_v    : 15;
        RK_U32 reserved       : 17;
    } jpeg_v_cfg;

    /* 0x0000047c reg287 */
    struct {
        RK_U32 jpeg_ri              : 25;
        RK_U32 jpeg_out_mode        : 1;
        RK_U32 jpeg_start_rst_m     : 3;
        RK_U32 jpeg_pic_last_ecs    : 1;
        RK_U32 reserved             : 1;
        RK_U32 jpeg_stnd            : 1;
    } jpeg_base_cfg;

    /* 0x00000480 reg288 */
    struct {
        RK_U32 uvc_partition0_len_jpeg    : 12;
        RK_U32 uvc_partition_len_jpeg     : 12;
        RK_U32 uvc_skip_len_jpeg          : 6;
        RK_U32 reserved                   : 2;
    } uvc_cfg_jpeg;

    /* 0x00000484 reg289 */
    struct {
        RK_U32 reserved          : 4;
        RK_U32 eslf_badr_jpeg    : 28;
    } adr_eslf_jpeg;

    /* 0x00000488 reg290 */
    struct {
        RK_U32 eslf_rptr_jpeg    : 10;
        RK_U32 eslf_wptr_jpeg    : 10;
        RK_U32 eslf_blen_jpeg    : 10;
        RK_U32 eslf_updt_jpeg    : 2;
    } eslf_buf_jpeg;

    /* 0x48c */
    RK_U32 reserved_291;

    /* 0x00000490 reg292 */
    struct {
        RK_U32 roi0_rdoq_start_x    : 11;
        RK_U32 roi0_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi0_rdoq_level      : 6;
        RK_U32 roi0_rdoq_en         : 1;
    } jpeg_roi0_cfg0;

    /* 0x00000494 reg293 */
    struct {
        RK_U32 roi0_rdoq_width_m1     : 11;
        RK_U32 roi0_rdoq_height_m1    : 11;
        RK_U32 reserved               : 3;
        RK_U32 frm_rdoq_level         : 6;
        RK_U32 frm_rdoq_en            : 1;
    } jpeg_roi0_cfg1;

    /* 0x00000498 reg294 */
    struct {
        RK_U32 roi1_rdoq_start_x    : 11;
        RK_U32 roi1_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi1_rdoq_level      : 6;
        RK_U32 roi1_rdoq_en         : 1;
    } jpeg_roi1_cfg0;

    /* 0x0000049c reg295 */
    struct {
        RK_U32 roi1_rdoq_width_m1     : 11;
        RK_U32 roi1_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi1_cfg1;

    /* 0x000004a0 reg296 */
    struct {
        RK_U32 roi2_rdoq_start_x    : 11;
        RK_U32 roi2_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi2_rdoq_level      : 6;
        RK_U32 roi2_rdoq_en         : 1;
    } jpeg_roi2_cfg0;

    /* 0x000004a4 reg297 */
    struct {
        RK_U32 roi2_rdoq_width_m1     : 11;
        RK_U32 roi2_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi2_cfg1;

    /* 0x000004a8 reg298 */
    struct {
        RK_U32 roi3_rdoq_start_x    : 11;
        RK_U32 roi3_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi3_rdoq_level      : 6;
        RK_U32 roi3_rdoq_en         : 1;
    } jpeg_roi3_cfg0;

    /* 0x000004ac reg299 */
    struct {
        RK_U32 roi3_rdoq_width_m1     : 11;
        RK_U32 roi3_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi3_cfg1;

    /* 0x000004b0 reg300 */
    struct {
        RK_U32 roi4_rdoq_start_x    : 11;
        RK_U32 roi4_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi4_rdoq_level      : 6;
        RK_U32 roi4_rdoq_en         : 1;
    } jpeg_roi4_cfg0;

    /* 0x000004b4 reg301 */
    struct {
        RK_U32 roi4_rdoq_width_m1     : 11;
        RK_U32 roi4_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi4_cfg1;

    /* 0x000004b8 reg302 */
    struct {
        RK_U32 roi5_rdoq_start_x    : 11;
        RK_U32 roi5_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi5_rdoq_level      : 6;
        RK_U32 roi5_rdoq_en         : 1;
    } jpeg_roi5_cfg0;

    /* 0x000004bc reg303 */
    struct {
        RK_U32 roi5_rdoq_width_m1     : 11;
        RK_U32 roi5_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi5_cfg1;

    /* 0x000004c0 reg304 */
    struct {
        RK_U32 roi6_rdoq_start_x    : 11;
        RK_U32 roi6_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi6_rdoq_level      : 6;
        RK_U32 roi6_rdoq_en         : 1;
    } jpeg_roi6_cfg0;

    /* 0x000004c4 reg305 */
    struct {
        RK_U32 roi6_rdoq_width_m1     : 11;
        RK_U32 roi6_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi6_cfg1;

    /* 0x000004c8 reg306 */
    struct {
        RK_U32 roi7_rdoq_start_x    : 11;
        RK_U32 roi7_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi7_rdoq_level      : 6;
        RK_U32 roi7_rdoq_en         : 1;
    } jpeg_roi7_cfg0;

    /* 0x000004cc reg307 */
    struct {
        RK_U32 roi7_rdoq_width_m1     : 11;
        RK_U32 roi7_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi7_cfg1;

    /* 0x000004d0 reg308 */
    struct {
        RK_U32 roi8_rdoq_start_x    : 11;
        RK_U32 roi8_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi8_rdoq_level      : 6;
        RK_U32 roi8_rdoq_en         : 1;
    } jpeg_roi8_cfg0;

    /* 0x000004d4 reg309 */
    struct {
        RK_U32 roi8_rdoq_width_m1     : 11;
        RK_U32 roi8_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi8_cfg1;

    /* 0x000004d8 reg310 */
    struct {
        RK_U32 roi9_rdoq_start_x    : 11;
        RK_U32 roi9_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi9_rdoq_level      : 6;
        RK_U32 roi9_rdoq_en         : 1;
    } jpeg_roi9_cfg0;

    /* 0x000004dc reg311 */
    struct {
        RK_U32 roi9_rdoq_width_m1     : 11;
        RK_U32 roi9_rdoq_height_m1    : 11;
        RK_U32 reserved               : 10;
    } jpeg_roi9_cfg1;

    /* 0x000004e0 reg312 */
    struct {
        RK_U32 roi10_rdoq_start_x    : 11;
        RK_U32 roi10_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi10_rdoq_level      : 6;
        RK_U32 roi10_rdoq_en         : 1;
    } jpeg_roi10_cfg0;

    /* 0x000004e4 reg313 */
    struct {
        RK_U32 roi10_rdoq_width_m1     : 11;
        RK_U32 roi10_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi10_cfg1;

    /* 0x000004e8 reg314 */
    struct {
        RK_U32 roi11_rdoq_start_x    : 11;
        RK_U32 roi11_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi11_rdoq_level      : 6;
        RK_U32 roi11_rdoq_en         : 1;
    } jpeg_roi11_cfg0;

    /* 0x000004ec reg315 */
    struct {
        RK_U32 roi11_rdoq_width_m1     : 11;
        RK_U32 roi11_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi11_cfg1;

    /* 0x000004f0 reg316 */
    struct {
        RK_U32 roi12_rdoq_start_x    : 11;
        RK_U32 roi12_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi12_rdoq_level      : 6;
        RK_U32 roi12_rdoq_en         : 1;
    } jpeg_roi12_cfg0;

    /* 0x000004f4 reg317 */
    struct {
        RK_U32 roi12_rdoq_width_m1     : 11;
        RK_U32 roi12_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi12_cfg1;

    /* 0x000004f8 reg318 */
    struct {
        RK_U32 roi13_rdoq_start_x    : 11;
        RK_U32 roi13_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi13_rdoq_level      : 6;
        RK_U32 roi13_rdoq_en         : 1;
    } jpeg_roi13_cfg0;

    /* 0x000004fc reg319 */
    struct {
        RK_U32 roi13_rdoq_width_m1     : 11;
        RK_U32 roi13_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi13_cfg1;

    /* 0x00000500 reg320 */
    struct {
        RK_U32 roi14_rdoq_start_x    : 11;
        RK_U32 roi14_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi14_rdoq_level      : 6;
        RK_U32 roi14_rdoq_en         : 1;
    } jpeg_roi14_cfg0;

    /* 0x00000504 reg321 */
    struct {
        RK_U32 roi14_rdoq_width_m1     : 11;
        RK_U32 roi14_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi14_cfg1;

    /* 0x00000508 reg322 */
    struct {
        RK_U32 roi15_rdoq_start_x    : 11;
        RK_U32 roi15_rdoq_start_y    : 11;
        RK_U32 reserved              : 3;
        RK_U32 roi15_rdoq_level      : 6;
        RK_U32 roi15_rdoq_en         : 1;
    } jpeg_roi15_cfg0;

    /* 0x0000050c reg323 */
    struct {
        RK_U32 roi15_rdoq_width_m1     : 11;
        RK_U32 roi15_rdoq_height_m1    : 11;
        RK_U32 reserved                : 10;
    } jpeg_roi15_cfg1;

    /* 0x510 - 0x51c */
    RK_U32 reserved324_327[4];

    /* 0x00000520 reg328 */
    struct {
        RK_U32 reserved        : 4;
        RK_U32 base_addr_md    : 28;
    } adr_md_vpp;

    /* 0x00000524 reg329 */
    struct {
        RK_U32 reserved        : 4;
        RK_U32 base_addr_od    : 28;
    } adr_od_vpp;

    /* 0x00000528 reg330 */
    struct {
        RK_U32 reserved             : 4;
        RK_U32 base_addr_ref_mdw    : 28;
    } adr_ref_mdw;

    /* 0x0000052c reg331 */
    struct {
        RK_U32 reserved             : 4;
        RK_U32 base_addr_ref_mdr    : 28;
    } adr_ref_mdr;

    /* 0x00000530 reg332 */
    struct {
        RK_U32 sto_stride_md          : 8;
        RK_U32 sto_stride_od          : 8;
        RK_U32 cur_frm_en_md          : 1;
        RK_U32 ref_frm_en_md          : 1;
        RK_U32 switch_sad_md          : 2;
        RK_U32 night_mode_en_md       : 1;
        RK_U32 flycatkin_flt_en_md    : 1;
        RK_U32 en_od                  : 1;
        RK_U32 background_en_od       : 1;
        RK_U32 sad_comp_en_od         : 1;
        RK_U32 reserved               : 6;
        RK_U32 vepu_pp_en             : 1;
    } vpp_base_cfg;

    /* 0x00000534 reg333 */
    struct {
        RK_U32 thres_sad_md          : 12;
        RK_U32 thres_move_md         : 3;
        RK_U32 reserved              : 1;
        RK_U32 thres_dust_move_md    : 4;
        RK_U32 thres_dust_blk_md     : 3;
        RK_U32 reserved1             : 1;
        RK_U32 thres_dust_chng_md    : 8;
    } thd_md_vpp;

    /* 0x00000538 reg334 */
    struct {
        RK_U32 thres_complex_od        : 12;
        RK_U32 thres_complex_cnt_od    : 3;
        RK_U32 thres_sad_od            : 14;
        RK_U32 reserved                : 3;
    } thd_od_vpp;
} H264eVepu511Frame;

/* class: param */
/* 0x00001700 reg1472 - 0x000019cc reg1651 */
typedef struct H264eVepu511Param_t {
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
        RK_U32 bias_madi_th0    : 8;
        RK_U32 bias_madi_th1    : 8;
        RK_U32 bias_madi_th2    : 8;
        RK_U32 reserved         : 8;
    } bias_madi_thd_comb;

    /* 0x00001738 reg1486 */
    struct {
        RK_U32 bias_i_val3    : 10;
        RK_U32 reserved       : 22;
    } qnt1_i_bias_comb;

    /* 0x0000173c reg1487 */
    struct {
        RK_U32 bias_p_val0    : 10;
        RK_U32 bias_p_val1    : 10;
        RK_U32 bias_p_val2    : 10;
        RK_U32 reserved       : 2;
    } qnt0_p_bias_comb;

    /* 0x00001740 reg1488 */
    struct {
        RK_U32 bias_p_val3    : 10;
        RK_U32 reserved       : 22;
    } qnt1_p_bias_comb;

    /* 0x1744 - 0x175c */
    RK_U32 reserved1489_1495[7];

    /* 0x00001760 reg1496 */
    struct {
        RK_U32 cime_pmv_num      : 1;
        RK_U32 cime_fuse         : 1;
        RK_U32 reserved          : 2;
        RK_U32 move_lambda       : 4;
        RK_U32 rime_lvl_mrg      : 2;
        RK_U32 rime_prelvl_en    : 2;
        RK_U32 rime_prersu_en    : 3;
        RK_U32 fme_lvl_mrg       : 1;
        RK_U32 reserved1         : 16;
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
        RK_U32 cime_madp_th       : 12;
        RK_U32 ratio_consi_cfg    : 4;
        RK_U32 ratio_bmv_dist     : 4;
        RK_U32 reserved           : 12;
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

    /* 0x1780 - 0x18fc */
    RK_U32 reserved1504_1599[96];

    /* 0x00001900 reg1600 - 0x000019cc reg1651*/
    RK_U32 rdo_wgta_qp_grpa_0_51[52];
} H264eVepu511Param;

/* class: rdo/q_i */
/* 0x00002000 reg2048 - 0x000020b8 reg2094 */
typedef struct H264eVepu511SqiCfg_t {
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
    RK_U32 reserved_2070;

    /* 0x0000205c reg2071 */
    struct {
        RK_U32 lid_grdn_blk_cu16_th       : 8;
        RK_U32 lid_rmd_intra_jcoef_ang    : 5;
        RK_U32 lid_rdo_intra_rcoef_ang    : 5;
        RK_U32 lid_rmd_intra_jcoef_dp     : 6;
        RK_U32 lid_rdo_intra_rcoef_dp     : 6;
        RK_U32 lid_en                     : 1;
        RK_U32 reserved                   : 1;
    } line_intra_dir_cfg;

    RK_U32 reserved2072_2075[4];

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

    /* 0x000020bc reg2095 - 0x0000215c reg2135*/
    RK_U32 reserved_2095_2135[40];

    /* 0x00002160 reg2136 */
    struct {
        RK_U32 atr_thd0    : 8;
        RK_U32 atr_thd1    : 8;
        RK_U32 atr_thd2    : 8;
        RK_U32 atr_qp      : 6;
        RK_U32 reserved    : 2;
    } atr_thd;

    /* 0x00002164 reg2137 */
    struct {
        RK_U32 atr_lv16_wgt0    : 8;
        RK_U32 atr_lv16_wgt1    : 8;
        RK_U32 atr_lv16_wgt2    : 8;
        RK_U32 reserved         : 8;
    } atr_wgt16;

    /* 0x00002168 reg2138 */
    struct {
        RK_U32 atr_lv8_wgt0    : 8;
        RK_U32 atr_lv8_wgt1    : 8;
        RK_U32 atr_lv8_wgt2    : 8;
        RK_U32 reserved        : 8;
    } atr_wgt8;

    /* 0x0000216c reg2139 */
    struct {
        RK_U32 atr_lv4_wgt0    : 8;
        RK_U32 atr_lv4_wgt1    : 8;
        RK_U32 atr_lv4_wgt2    : 8;
        RK_U32 reserved        : 8;
    } atr_wgt4;
} H264eVepu511Sqi;

/* class: scaling list  */
/* 0x00002200 reg2176- 0x0000268c reg2467*/
typedef struct H264eVepu511SclCfg_t {
    /* 0x2200 - 0x227c, valid for h.264 iq_scal_t8_intra0~15 iq_scal_t8_inter0~15*/
    RK_U32 tu8_intra_y[16];
    RK_U32 tu8_intra_u[16];

    /* 0x2280 - 0x258c*/
    RK_U32 reserved_2208_2215[196];

    /* 0x2590 - 0x268c, valid for h.264 q_scal_t8_intra0~31 q_scal_t8_inter0~31*/
    RK_U32 q_t8_intra[32];
    RK_U32 q_t8_inter[32];
} H264eVepu511SclCfg;

typedef struct HalVepu511Reg_t {
    Vepu511ControlCfg       reg_ctl;
    H264eVepu511Frame       reg_frm;
    Vepu511RcRoi            reg_rc_roi;
    H264eVepu511Param       reg_param;
    H264eVepu511Sqi         reg_sqi;
    H264eVepu511SclCfg      reg_scl;
    Vepu511OsdRegs          reg_osd;
    Vepu511Status           reg_st;
    Vepu511Dbg              reg_dbg;
} HalVepu511RegSet;

#endif