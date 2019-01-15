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

#ifndef __HAL_JPEGD_VDPU2_REG_H__
#define __HAL_JPEGD_VDPU2_REG_H__

#include "mpp_device_patch.h"

#define JPEGD_REG_NUM                         (159)

#define DEC_MODE_JPEG                         (3)
#define DEC_LITTLE_ENDIAN                     (1)
#define DEC_BIG_ENDIAN                        (0)

#define DEC_BUS_BURST_LENGTH_16               (16)
#define DEC_SCMD_DISABLE                      (0)
#define DEC_LATENCY_COMPENSATION              (0)
#define DEC_DATA_DISCARD_ENABLE               (0)

typedef struct JpegRegSet {
    struct {
        RK_U32  sw_pp_max_burst     : 5;
        RK_U32  sw_pp_scmd_dis      : 1;
        RK_U32  sw_reserved_1       : 2;
        RK_U32  sw_pp_axi_rd_id     : 8;
        RK_U32  sw_pp_axi_wr_id     : 8;
    } reg0;

    struct {
        RK_U32  sw_color_coeffa1    : 10;
        RK_U32  sw_color_coeffa2    : 10;
        RK_U32  sw_color_coeffb     : 10;
    } reg1;

    struct {
        RK_U32  sw_color_coeffc     : 10;
        RK_U32  sw_color_coeffd     : 10;
        RK_U32  sw_color_coeffe     : 10;
    } reg2;

    struct {
        RK_U32  sw_pp_color_coefff  : 8;
    } reg3;

    struct {
        RK_U32  sw_scale_wratio     : 18;
        RK_U32  sw_hor_scale_mode   : 2;
        RK_U32  sw_ver_scale_mode   : 2;
    } reg4;

    struct {
        RK_U32  sw_scale_hratio    : 18;
    } reg5;

    struct {
        RK_U32  sw_wscale_invra    : 16;
        RK_U32  sw_hscale_invra    : 16;
    } reg6;

    RK_U32 reg7;
    RK_U32 reg8;
    RK_U32 reg9_r_mask;
    RK_U32 reg10_g_mask;
    RK_U32 reg11_b_mask;
    RK_U32 reg12_pp_bot_yin_base;
    RK_U32 reg13_pp_bot_cin_base;

    struct {
        RK_U32  sw_crop_startx      : 9;
        RK_U32  sw_crop_starty_ext  : 3;
        RK_U32  sw_reserved_1       : 4;
        RK_U32  sw_crop_starty      : 8;
        RK_U32  sw_crop_startx_ext  : 3;
        RK_U32  sw_reserved_2       : 1;
        RK_U32  sw_pp_crop8_d_e     : 1;
        RK_U32  sw_pp_crop8_r_e     : 1;
    } reg14;

    struct {
        RK_U32  sw_rangemap_coef_y      : 5;
        RK_U32  sw_ycbcr_range          : 1;
        RK_U32  sw_reserved_1           : 2;
        RK_U32  sw_rangemap_coef_c      : 5;
    } reg15;

    struct {
        RK_U32  sw_rgb_r_padd       : 5;
        RK_U32  sw_reserved_1       : 3;
        RK_U32  sw_rgb_g_padd       : 5;
        RK_U32  sw_reserved_2       : 3;
        RK_U32  sw_rgb_b_padd       : 5;
    } reg16;

    RK_U32 reg17;
    RK_U32 reg18_pp_in_lu_base;
    RK_U32 reg19;
    RK_U32 reg20;
    RK_U32 reg21_pp_out_lu_base;
    RK_U32 reg22_pp_out_ch_base;
    RK_U32 reg23;
    RK_U32 reg24;
    RK_U32 reg25;
    RK_U32 reg26;
    RK_U32 reg27;
    RK_U32 reg28;
    RK_U32 reg29;
    RK_U32 reg30;

    struct {
        RK_U32  sw_contrast_thr1      : 8;
        RK_U32  sw_contrast_thr2      : 8;
    } reg31;

    struct {
        RK_U32  sw_contrast_off1      : 10;
        RK_U32  sw_reserved_1         : 6;
        RK_U32  sw_contrast_off2      : 10;
    } reg32;

    RK_U32 reg33;

    struct {
        RK_U32  sw_pp_in_width      : 9;
        RK_U32  sw_pp_in_w_ext      : 3;
        RK_U32  sw_ext_orig_width   : 9;
        RK_U32  sw_pp_in_height     : 8;
        RK_U32  sw_pp_in_h_ext      : 3;
    } reg34;

    struct {
        RK_U32  sw_pp_out_width      : 11;
        RK_U32  sw_reserved_1        : 5;
        RK_U32  sw_pp_out_height     : 11;
    } reg35;

    struct {
        RK_U32  sw_dither_select_r      : 2;
        RK_U32  sw_dither_select_g      : 2;
        RK_U32  sw_dither_select_b      : 2;
    } reg36;

    struct {
        RK_U32  sw_pp_in_endian     : 1;
        RK_U32  sw_pp_in_a1_endian  : 1;
        RK_U32  sw_pp_in_a2_endsel  : 1;
        RK_U32  sw_pp_out_endian    : 1;
        RK_U32  sw_rgb_pix_in32     : 1;
        RK_U32  sw_reserved_1       : 3;
        RK_U32  sw_pp_in_swap32_e   : 1;
        RK_U32  sw_pp_in_a1_swap32  : 1;
        RK_U32  sw_pp_out_swap16_e  : 1;
        RK_U32  sw_pp_out_swap32_e  : 1;
        RK_U32  sw_reserved_2       : 4;
        RK_U32  sw_pp_in_start_ch   : 1;
        RK_U32  sw_pp_out_start_ch  : 1;
        RK_U32  sw_pp_in_cr_first   : 1;
        RK_U32  sw_pp_out_cr_first  : 1;
        RK_U32  sw_reserved_3       : 4;
        RK_U32  sw_pp_in_struct     : 3;
    } reg37;

    struct {
        RK_U32  sw_rotation_mode      : 3;
        RK_U32  sw_reserved_1         : 5;
        RK_U32  sw_pp_in_format       : 3;
        RK_U32  sw_pp_out_format      : 3;
        RK_U32  sw_reserved_2         : 2;
        RK_U32  sw_pp_in_format_es    : 3;
    } reg38;

    struct {
        RK_U32  sw_display_width      : 12;
    } reg39;

    RK_U32 reg40;

    struct {
        RK_U32  sw_pp_e             : 1;
        RK_U32  sw_deint_blend_e    : 1;
        RK_U32  sw_deint_e          : 1;
        RK_U32  sw_pp_clk_gate_e    : 1;
        RK_U32  sw_pp_pipeline_e    : 1;
        RK_U32  sw_reserved_1       : 3;
        RK_U32  sw_rangemap_y_e     : 1;
        RK_U32  sw_rangemap_c_e     : 1;
        RK_U32  sw_reserved_2       : 6;
        RK_U32  sw_pp_data_disc_e   : 1;
        RK_U32  sw_reserved_3       : 3;
        RK_U32  sw_mask1_e          : 1;
        RK_U32  sw_mask2_e          : 1;
        RK_U32  sw_mask1_ablend_e   : 1;
        RK_U32  sw_mask2_ablend_e   : 1;
        RK_U32  sw_up_cross_e       : 1;
        RK_U32  sw_down_cross_e     : 1;
        RK_U32  sw_left_cross_e     : 1;
        RK_U32  sw_right_cross_e    : 1;
        RK_U32  sw_pp_ahb_hlock_e   : 1;
    } reg41;

    RK_U32   ppReg2[8];
    struct {
        RK_U32  sw_dec_out_tiled_e  : 1;
        RK_U32  sw_dec_latency      : 6;
        RK_U32  sw_pic_fixed_quant  : 1;
        RK_U32  sw_filtering_dis    : 1;
        RK_U32  sw_skip_mode        : 1;
        RK_U32  sw_dec_scmd_dis     : 1;
        RK_U32  sw_dec_adv_pre_dis  : 1;
        RK_U32  sw_priority_mode    : 1;
        RK_U32  sw_refbu2_thr       : 12;
        RK_U32  sw_refbu2_picid     : 5;
        RK_U32  reserve1            : 2;
    } reg50_dec_ctrl;

    struct {
        RK_U32  sw_stream_len       : 24;
        RK_U32  reserve1            : 1;
        RK_U32  sw_init_qp          : 6;
        RK_U32  reserve2            : 1;
    } reg51_stream_info;

    struct {
        RK_U32  sw_startmb_y        : 8;
        RK_U32  sw_startmb_x        : 9;
        RK_U32  sw_apf_threshold    : 14;
        RK_U32  sw_reserve          : 1;
    } reg52_error_concealment;

    RK_U32       reg53_dec_mode;

    struct {
        RK_U32  sw_dec_in_endian    : 1;
        RK_U32  sw_dec_out_endian   : 1;
        RK_U32  sw_dec_inswap32_e   : 1;
        RK_U32  sw_dec_outswap32_e  : 1;
        RK_U32  sw_dec_strswap32_e  : 1;
        RK_U32  sw_dec_strendian_e  : 1;
        RK_U32  reserve3            : 26;
    } reg54_endian;

    struct {
        RK_U32  sw_dec_irq          : 1;
        RK_U32  sw_dec_irq_dis      : 1;
        RK_U32  reserve0            : 2;
        RK_U32  sw_dec_rdy_int      : 1;
        RK_U32  sw_dec_bus_int      : 1;
        RK_U32  sw_dec_buffer_int   : 1;
        RK_U32  reserve1            : 1;
        RK_U32  sw_dec_aso_int      : 1;
        RK_U32  sw_dec_slice_int    : 1;
        RK_U32  sw_dec_pic_inf      : 1;
        RK_U32  reserve2            : 1;
        RK_U32  sw_dec_error_int    : 1;
        RK_U32  sw_dec_timeout      : 1;
        RK_U32  reserve3            : 18;
    } reg55_Interrupt;

    struct {
        RK_U32  sw_dec_axi_rn_id    : 8;
        RK_U32  sw_dec_axi_wr_id    : 8;
        RK_U32  sw_dec_max_burst    : 5;
        RK_U32  resever             : 1;
        RK_U32  sw_dec_data_disc_e  : 1;
        RK_U32  resever1            : 9;
    } reg56_axi_ctrl;

    struct {
        RK_U32  sw_dec_e            : 1;
        RK_U32  sw_refbu2_buf_e     : 1;
        RK_U32  sw_dec_out_dis      : 1;
        RK_U32  resever             : 1;
        RK_U32  sw_dec_clk_gate_e   : 1;
        RK_U32  sw_dec_timeout_e    : 1;
        RK_U32  sw_picord_count_e   : 1;
        RK_U32  sw_seq_mbaff_e      : 1;
        RK_U32  sw_reftopfirst_e    : 1;
        RK_U32  sw_ref_topfield_e   : 1;
        RK_U32  sw_write_mvs_e      : 1;
        RK_U32  sw_sorenson_e       : 1;
        RK_U32  sw_fwd_interlace_e  : 1;
        RK_U32  sw_pic_topfield_e   : 1;
        RK_U32  sw_pic_inter_e      : 1;
        RK_U32  sw_pic_b_e          : 1;
        RK_U32  sw_pic_fieldmode_e  : 1;
        RK_U32  sw_pic_interlace_e  : 1;
        RK_U32  sw_pjpeg_e          : 1;
        RK_U32  sw_divx3_e          : 1;
        RK_U32  sw_rlc_mode_e       : 1;
        RK_U32  sw_ch_8pix_ileav_e  : 1;
        RK_U32  sw_start_code_e     : 1;
        RK_U32  resever1            : 8;
        RK_U32 sw_dec_ahb_hlock_e   : 1;
    } reg57_enable_ctrl;

    RK_U32          reg58_soft_rest;

    struct {
        RK_U32  resever             : 2;
        RK_U32  sw_pred_bc_tap_0_2  : 10;
        RK_U32  sw_pred_bc_tap_0_1  : 10;
        RK_U32  sw_pred_bc_tap_0_0  : 10;
    } reg59;

    RK_U32          reg60_addit_ch_st_base;
    RK_U32          reg61_qtable_base;
    RK_U32          reg62_directmv_base;
    RK_U32          reg63_dec_out_base;
    RK_U32          reg64_rlc_vlc_base;

    struct {
        RK_U32  sw_refbu_y_offset   : 9;
        RK_U32  sw_reserve          : 3;
        RK_U32  sw_refbu_fparmod_e  : 1;
        RK_U32  sw_refbu_eval_e     : 1;
        RK_U32  sw_refbu_picid      : 5;
        RK_U32  sw_refbu_thr        : 12;
        RK_U32  sw_refbu_e          : 1;
    } reg65_refpicbuf_ctrl;

    struct {
        RK_U32  build_version   : 3;
        RK_U32  product_IDen    : 1;
        RK_U32  minor_version   : 8;
        RK_U32  major_version   : 4;
        RK_U32  product_numer   : 16;
    } reg66_id;

    struct {
        RK_U32  sw_reserve          : 25;
        RK_U32  sw_dec_rtl_rom      : 1;
        RK_U32  sw_dec_rv_prof      : 2;
        RK_U32  sw_ref_buff2_exist  : 1;
        RK_U32  sw_dec_divx_prof    : 1;
        RK_U32  sw_dec_refbu_ilace  : 1;
        RK_U32  sw_dec_jpeg_exten   : 1;
    } reg67_synthesis_cfg;

    struct {
        RK_U32  sw_refbu_top_sum    : 16;
        RK_U32  sw_refbu_bot_sum    : 16;
    } reg68_sum_of_partitions;

    struct {
        RK_U32  sw_refbu_intra_sum  : 16;
        RK_U32  sw_refbu_hit_sum    : 16;
    } reg69_sum_inf;

    struct {
        RK_U32  sw_refbu_mv_sum : 22;
        RK_U32  sw_reserve      : 10;
    } reg70_sum_mv;

    RK_U32                        reg71_119_reserve[49];

    struct {
        RK_U32  sw_pic_mb_h_ext     : 3;
        RK_U32  sw_pic_mb_w_ext     : 3;
        RK_U32  sw_alt_scan_e       : 1;
        RK_U32  sw_mb_height_off    : 4;
        RK_U32  sw_pic_mb_hight_p   : 8;
        RK_U32  sw_mb_width_off     : 4;
        RK_U32  sw_pic_mb_width     : 9;
    } reg120;

    struct {
        RK_U32  sw_pjpeg_se         : 8;
        RK_U32  sw_pjpeg_ss         : 8;
        RK_U32  sw_pjpeg_al         : 4;
        RK_U32  sw_pjpeg_ah         : 4;
        RK_U32  sw_pjpeg_hdiv8      : 1;
        RK_U32  sw_pjpeg_wdiv8      : 1;
        RK_U32  sw_pjpeg_fildown_e  : 1;
    } reg121;

    struct {
        RK_U32  sw_cb_dc_vlctable3   : 1;
        RK_U32  sw_cr_dc_vlctable3   : 1;
        RK_U32  sw_cb_dc_vlctable    : 1;
        RK_U32  sw_cr_dc_vlctable    : 1;
        RK_U32  sw_cb_ac_vlctable    : 1;
        RK_U32  sw_cr_ac_vlctable    : 1;
        RK_U32  sw_jpeg_stream_all   : 1;
        RK_U32  sw_jpeg_filright_e   : 1;
        RK_U32  sw_jpeg_mode         : 3;
        RK_U32  sw_jpeg_qtables      : 2;
        RK_U32  sw_reserved_1        : 12;
        RK_U32  sw_sync_marker_e     : 1;
        RK_U32  sw_strm_start_bit    : 6;
    } reg122;

    struct {
        RK_U32  sw_pjpeg_rest_freq   : 16;
    } reg123;

    struct {
        RK_U32  sw_stream1_len      : 24;
        RK_U32  sw_coeffs_part_am   : 4;
        RK_U32  sw_reserved         : 4;
    } reg124;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_5_3   : 10;
        RK_U32  sw_pred_bc_tap_5_2   : 10;
        RK_U32  sw_pred_bc_tap_5_1   : 10;
    } reg125;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_6_2   : 10;
        RK_U32  sw_pred_bc_tap_6_1   : 10;
        RK_U32  sw_pred_bc_tap_6_0   : 10;
    } reg126;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_7_1   : 10;
        RK_U32  sw_pred_bc_tap_7_0   : 10;
        RK_U32  sw_pred_bc_tap_6_3   : 10;
    } reg127;

    struct {
        RK_U32  sw_pred_tap_6_4     : 2;
        RK_U32  sw_pred_tap_6_M1    : 2;
        RK_U32  sw_pred_tap_4_4     : 2;
        RK_U32  sw_pred_tap_4_M1    : 2;
        RK_U32  sw_pred_tap_2_4     : 2;
        RK_U32  sw_pred_tap_2_M1    : 2;
        RK_U32  sw_pred_bc_tap_7_3  : 10;
        RK_U32  sw_pred_bc_tap_7_2  : 10;
    } reg128;

    struct {
        RK_U32  sw_filt_level_3   : 6;
        RK_U32  sw_filt_level_2   : 6;
        RK_U32  sw_filt_level_1   : 6;
        RK_U32  sw_filt_level_0   : 6;
        RK_U32  resever           : 8;
    } reg129;

    struct {
        RK_U32  sw_quant_1        : 11;
        RK_U32  sw_quant_0        : 11;
        RK_U32  sw_quant_delta_1  : 5;
        RK_U32  sw_quant_delta_0  : 5;
    } reg130;


    RK_U32 reg131_jpg_ch_out_base;

    struct {
        RK_U32  sw_filt_mb_adj_3   : 7;
        RK_U32  sw_filt_mb_adj_2   : 7;
        RK_U32  sw_filt_mb_adj_1   : 7;
        RK_U32  sw_filt_mb_adj_0   : 7;
        RK_U32  sw_filt_sharpness  : 3;
        RK_U32  sw_filt_type       : 1;
    } reg132;


    struct {
        RK_U32  sw_filt_ref_adj_3  : 7;
        RK_U32  sw_filt_ref_adj_2  : 7;
        RK_U32  sw_filt_ref_adj_1  : 7;
        RK_U32  sw_filt_ref_adj_0  : 7;
        RK_U32  sw_reserved        : 4;
    } reg133;

    struct {
        RK_U32  sw_ac1_code1_cnt   : 2;
        RK_U32  sw_reserved_1      : 1;
        RK_U32  sw_ac1_code2_cnt   : 3;
        RK_U32  sw_reserved_2      : 1;
        RK_U32  sw_ac1_code3_cnt   : 4;
        RK_U32  sw_ac1_code4_cnt   : 5;
        RK_U32  sw_ac1_code5_cnt   : 6;
        RK_U32  sw_reserved_3      : 2;
        RK_U32  sw_ac1_code6_cnt   : 7;
    } reg134;

    struct {
        RK_U32  sw_ac1_code7_cnt   : 8;
        RK_U32  sw_ac1_code8_cnt   : 8;
        RK_U32  sw_ac1_code9_cnt   : 8;
        RK_U32  sw_ac1_code10_cnt  : 8;
    } reg135;

    struct {
        RK_U32  sw_ac1_code11_cnt   : 8;
        RK_U32  sw_ac1_code12_cnt   : 8;
        RK_U32  sw_ac1_code13_cnt   : 8;
        RK_U32  sw_ac1_code14_cnt   : 8;
    } reg136;

    struct {
        RK_U32  sw_ac1_code15_cnt   : 8;
        RK_U32  sw_ac1_code16_cnt   : 8;
        RK_U32  sw_ac2_code1_cnt    : 2;
        RK_U32  sw_reserved_1       : 1;
        RK_U32  sw_ac2_code2_cnt    : 3;
        RK_U32  sw_reserved_2       : 1;
        RK_U32  sw_ac2_code3_cnt    : 4;
        RK_U32  sw_ac2_code4_cnt    : 5;
    } reg137;

    struct {
        RK_U32  sw_ac2_code5_cnt    : 6;
        RK_U32  sw_reserved_1       : 2;
        RK_U32  sw_ac2_code6_cnt    : 7;
        RK_U32  sw_reserved_2       : 1;
        RK_U32  sw_ac2_code7_cnt    : 8;
        RK_U32  sw_ac2_code8_cnt    : 8;
    } reg138;

    struct {
        RK_U32  sw_ac2_code9_cnt    : 8;
        RK_U32  sw_ac2_code10_cnt   : 8;
        RK_U32  sw_ac2_code11_cnt   : 8;
        RK_U32  sw_ac2_code12_cnt   : 8;
    } reg139;

    struct {
        RK_U32  sw_ac2_code13_cnt   : 8;
        RK_U32  sw_ac2_code14_cnt   : 8;
        RK_U32  sw_ac2_code15_cnt   : 8;
        RK_U32  sw_ac2_code16_cnt   : 8;
    } reg140;

    struct {
        RK_U32  sw_dc1_code1_cnt    : 2;
        RK_U32  sw_reserved_1       : 2;
        RK_U32  sw_dc1_code2_cnt    : 3;
        RK_U32  sw_reserved_2       : 1;
        RK_U32  sw_dc1_code3_cnt    : 4;
        RK_U32  sw_dc1_code4_cnt    : 4;
        RK_U32  sw_dc1_code5_cnt    : 4;
        RK_U32  sw_dc1_code6_cnt    : 4;
        RK_U32  sw_dc1_code7_cnt    : 4;
        RK_U32  sw_dc1_code8_cnt    : 4;
    } reg141;

    struct {
        RK_U32  sw_dc1_code9_cnt     : 4;
        RK_U32  sw_dc1_code10_cnt    : 4;
        RK_U32  sw_dc1_code11_cnt    : 4;
        RK_U32  sw_dc1_code12_cnt    : 4;
        RK_U32  sw_dc1_code13_cnt    : 4;
        RK_U32  sw_dc1_code14_cnt    : 4;
        RK_U32  sw_dc1_code15_cnt    : 4;
        RK_U32  sw_dc1_code16_cnt    : 4;
    } reg142;

    struct {
        RK_U32  sw_dc2_code1_cnt    : 2;
        RK_U32  sw_reserved_1       : 2;
        RK_U32  sw_dc2_code2_cnt    : 3;
        RK_U32  sw_reserved_2       : 1;
        RK_U32  sw_dc2_code3_cnt    : 4;
        RK_U32  sw_dc2_code4_cnt    : 4;
        RK_U32  sw_dc2_code5_cnt    : 4;
        RK_U32  sw_dc2_code6_cnt    : 4;
        RK_U32  sw_dc2_code7_cnt    : 4;
        RK_U32  sw_dc2_code8_cnt    : 4;
    } reg143;

    struct {
        RK_U32  sw_dc2_code9_cnt     : 4;
        RK_U32  sw_dc2_code10_cnt    : 4;
        RK_U32  sw_dc2_code11_cnt    : 4;
        RK_U32  sw_dc2_code12_cnt    : 4;
        RK_U32  sw_dc2_code13_cnt    : 4;
        RK_U32  sw_dc2_code14_cnt    : 4;
        RK_U32  sw_dc2_code15_cnt    : 4;
        RK_U32  sw_dc2_code16_cnt    : 4;
    } reg144;

    RK_U32 reg145_bitpl_ctrl_base;
    RK_U32 reg_dct_strm1_base[2];

    struct {
        RK_U32  sw_slice_h           : 8;
        RK_U32  sw_reserved_1        : 12;
        RK_U32  sw_jpeg_height8_flag : 1;
        RK_U32  sw_syn_marker_e      : 1;
        RK_U32  sw_reserved_2        : 10;
    } reg148;

    RK_U32 reg149_segment_map_base;

    struct {
        RK_U32  sw_dct_start_bit_7   : 6;
        RK_U32  sw_dct_start_bit_6   : 6;
        RK_U32  sw_dct_start_bit_5   : 6;
        RK_U32  sw_dct_start_bit_4   : 6;
        RK_U32  sw_dct_start_bit_3   : 6;
        RK_U32  sw_reserved          : 2;
    } reg150;

    struct {
        RK_U32  sw_quant_3         : 11;
        RK_U32  sw_quant_2         : 11;
        RK_U32  sw_quant_delta_3   : 5;
        RK_U32  sw_quant_delta_2   : 5;
    } reg151;

    struct {
        RK_U32  sw_quant_5         : 11;
        RK_U32  sw_quant_4         : 11;
        RK_U32  sw_quant_delta_4   : 5;
        RK_U32  sw_reserved        : 5;
    } reg152;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_1_1   : 10;
        RK_U32  sw_pred_bc_tap_1_0   : 10;
        RK_U32  sw_pred_bc_tap_0_3   : 10;
    } reg153;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_2_0   : 10;
        RK_U32  sw_pred_bc_tap_1_3   : 10;
        RK_U32  sw_pred_bc_tap_1_2   : 10;
    } reg154;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_2_3   : 10;
        RK_U32  sw_pred_bc_tap_2_2   : 10;
        RK_U32  sw_pred_bc_tap_2_1   : 10;
    } reg155;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_3_2   : 10;
        RK_U32  sw_pred_bc_tap_3_1   : 10;
        RK_U32  sw_pred_bc_tap_3_0   : 10;
    } reg156;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_4_1   : 10;
        RK_U32  sw_pred_bc_tap_4_0   : 10;
        RK_U32  sw_pred_bc_tap_3_3   : 10;
    } reg157;

    struct {
        RK_U32  resever              : 2;
        RK_U32  sw_pred_bc_tap_5_0   : 10;
        RK_U32  sw_pred_bc_tap_4_3   : 10;
        RK_U32  sw_pred_bc_tap_4_2   : 10;
    } reg158;
} JpegRegSet;

typedef struct JpegdIocRegInfo_t {
    JpegRegSet             regs;

    /* vepu_reg_num - vdpu_reg_num */
    RK_U32                 regs_diff[184 - JPEGD_REG_NUM];
    RegExtraInfo           extra_info;
} JpegdIocRegInfo;

#endif /* __HAL_JPEGD_VDPU2_REG_H__ */
