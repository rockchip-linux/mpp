/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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
#ifndef __HAL_JPEGD_VDPU1_REG_TABLE_H__
#define __HAL_JPEGD_VDPU1_REG_TABLE_H__

#include "mpp_device_patch.h"

#define JPEGD_REG_NUM                       (101)

#define DEC_VDPU1_LITTLE_ENDIAN             (1)
#define DEC_VDPU1_BIG_ENDIAN                (0)

#define DEC_VDPU1_BUS_BURST_LENGTH_16       (16)

#define DEC_VDPU1_SCMD_DISABLE              (0)
#define DEC_VDPU1_LATENCY_COMPENSATION      (0)
#define DEC_VDPU1_DATA_DISCARD_ENABLE       (0)


typedef struct {
    struct {
        RK_U32  sw_pp_e             : 1;
        RK_U32  sw_pp_pipeline_e    : 1;
        RK_U32  reserved3           : 2;
        RK_U32  sw_pp_irq_dis       : 1;
        RK_U32  reserved2           : 3;
        RK_U32  sw_pp_irq           : 1;
        RK_U32  reserved1           : 3;
        RK_U32  sw_pp_rdy_int       : 1;
        RK_U32  sw_pp_bus_int       : 1;
        RK_U32  reserved0           : 18;
    } reg60_interrupt;

    struct {
        RK_U32  sw_pp_max_burst     : 5;
        RK_U32  sw_pp_out_swap32_e  : 1;
        RK_U32  sw_pp_out_endian    : 1;
        RK_U32  sw_pp_in_endian     : 1;
        RK_U32  sw_pp_clk_gate_e    : 1;
        RK_U32  sw_pp_data_disc_e   : 1;
        RK_U32  sw_pp_in_swap32_e   : 1;
        RK_U32  sw_pp_in_a1_endian  : 1;
        RK_U32  sw_pp_in_a1_swap32  : 1;
        RK_U32  sw_pp_in_a2_endsel  : 1;
        RK_U32  sw_pp_scmd_dis      : 1;
        RK_U32  sw_pp_ahb_hlock_e   : 1;
        RK_U32  sw_pp_axi_wr_id     : 8;
        RK_U32  sw_pp_axi_rd_id     : 8;
    } reg61_dev_conf;

    struct {
        RK_U32  sw_deint_edge_det   : 15;
        RK_U32  sw_deint_blend_e    : 1;
        RK_U32  sw_deint_threshold  : 14;
        RK_U32  reserved0           : 1;
        RK_U32  sw_deint_e          : 1;
    } reg62_deinterlace;

    RK_U32 reg63_pp_in_lu_base;
    RK_U32 reg64_pp_in_cb_base;
    RK_U32 reg65_pp_in_cr_base;
    RK_U32 reg66_pp_out_lu_base;
    RK_U32 reg67_pp_out_ch_base;

    struct {
        RK_U32  sw_contrast_off1    : 10;
        RK_U32  sw_contrast_off2    : 10;
        RK_U32  reserved0           : 4;
        RK_U32  sw_contrast_thr1    : 8;
    } reg68_contrast_adjust;

    struct {
        RK_U32  sw_contrast_thr2    : 8;
        RK_U32  sw_color_coeffa1    : 10;
        RK_U32  sw_color_coeffa2    : 10;
        RK_U32  sw_pp_out_cr_first  : 1;
        RK_U32  sw_pp_out_start_ch  : 1;
        RK_U32  sw_pp_in_cr_first   : 1;
        RK_U32  sw_pp_in_start_ch   : 1;
    } reg69;

    struct {
        RK_U32  sw_color_coeffb     : 10;
        RK_U32  sw_color_coeffc     : 10;
        RK_U32  sw_color_coeffd     : 10;
        RK_U32  reserved0           : 2;
    } reg70_color_coeff_0;

    struct {
        RK_U32  sw_color_coeffe     : 10;
        RK_U32  sw_color_coefff     : 8;
        RK_U32  sw_rotation_mode    : 3;
        RK_U32  sw_crop_startx      : 9;
        RK_U32  reserved0           : 2;
    } reg71_color_coeff_1;

    struct {
        RK_U32  sw_pp_in_width      : 9;
        RK_U32  sw_pp_in_height     : 8;
        RK_U32  reserved1           : 1;
        RK_U32  sw_rangemap_coef_y  : 5;
        RK_U32  reserved0           : 1;
        RK_U32  sw_crop_starty      : 8;
    } reg72_crop;

    RK_U32 reg73_pp_bot_yin_base;
    RK_U32 reg74_pp_bot_cin_base;

    RK_U32 reg75_reg78[4];

    struct {
        RK_U32   sw_scale_wratio    : 18;
        RK_U32   sw_rgb_g_padd      : 5;
        RK_U32   sw_rgb_r_padd      : 5;
        RK_U32   sw_rgb_pix_in32    : 1;
        RK_U32   sw_ycbcr_range     : 1;
        RK_U32   sw_rangemap_c_e    : 1;
        RK_U32   sw_rangemap_y_e    : 1;
    } reg79_scaling_0;

    struct {
        RK_U32   sw_scale_hratio    : 18;
        RK_U32   sw_rgb_b_padd      : 5;
        RK_U32   sw_ver_scale_mode  : 2;
        RK_U32   sw_hor_scale_mode  : 2;
        RK_U32   sw_pp_in_struct    : 3;
        RK_U32   sw_pp_fast_scale_e : 1;
        RK_U32   reserved0          : 1;
    } reg80_scaling_1;

    struct {
        RK_U32  sw_wscale_invra     : 16;
        RK_U32  sw_hscale_invra     : 16;
    } reg81_scaling_2;

    RK_U32 reg82_r_mask;
    RK_U32 reg83_g_mask;
    RK_U32 reg84_b_mask;

    struct {
        RK_U32  sw_pp_crop8_d_e     : 1;
        RK_U32  sw_pp_crop8_r_e     : 1;
        RK_U32  sw_pp_out_swap16_e  : 1;
        RK_U32  sw_pp_out_tiled_e   : 1;
        RK_U32  sw_pp_out_width     : 11;
        RK_U32  sw_pp_out_height    : 11;
        RK_U32  sw_pp_out_format    : 3;
        RK_U32  sw_pp_in_format     : 3;
    } reg85_ctrl;

    struct {
        RK_U32  sw_mask1_startx     : 11;
        RK_U32  sw_mask1_starty     : 11;
        RK_U32  sw_mask1_ablend_e   : 1;
        RK_U32  sw_rangemap_coef_c  : 5;
        RK_U32  reserved0           : 1;
        RK_U32  sw_pp_in_format_es  : 3;
    } reg86_mask_1;

    struct {
        RK_U32  sw_mask2_startx     : 11;
        RK_U32  sw_mask2_starty     : 11;
        RK_U32  sw_mask2_ablend_e   : 1;
        RK_U32  reserved            : 9;
    } reg87_mask_2;

    struct {
        RK_U32  sw_mask1_endx       : 11;
        RK_U32  sw_mask1_endy       : 11;
        RK_U32  sw_mask1_e          : 1;
        RK_U32  sw_ext_orig_width   : 9;
    } reg88_mask_1_size;

    struct {
        RK_U32  sw_mask2_endx       : 11;
        RK_U32  sw_mask2_endy       : 11;
        RK_U32  sw_mask2_e          : 1;
        RK_U32  reserved0           : 9;
    } reg89_mask_2_size;

    struct {
        RK_U32  sw_down_cross       : 11;
        RK_U32  reserved1           : 4;
        RK_U32  sw_up_cross         : 11;
        RK_U32  sw_down_cross_e     : 1;
        RK_U32  sw_up_cross_e       : 1;
        RK_U32  sw_left_cross_e     : 1;
        RK_U32  sw_right_cross_e    : 1;
        RK_U32  reserved0           : 2;
    } reg90_pip_1;

    struct {
        RK_U32  sw_left_cross       : 11;
        RK_U32  sw_right_cross      : 11;
        RK_U32  sw_pp_tiled_mode    : 2;
        RK_U32  sw_dither_select_b  : 2;
        RK_U32  sw_dither_select_g  : 2;
        RK_U32  sw_dither_select_r  : 2;
    } reg91_pip_2;

    struct {
        RK_U32  sw_display_width    : 12;
        RK_U32  reserved0           : 8;
        RK_U32  sw_crop_startx_ext  : 3;
        RK_U32  sw_crop_starty_ext  : 3;
        RK_U32  sw_pp_in_w_ext      : 3;
        RK_U32  sw_pp_in_h_ext      : 3;
    } reg92_display;

    RK_U32 reg93_ablend1_base;
    RK_U32 reg94_ablend2_base;
    RK_U32 reg95_ablend2_scanl;

    RK_U32 reg96_reg97[2];

    struct {
        RK_U32  sw_pp_out_w_ext     : 1;
        RK_U32  sw_pp_out_h_ext     : 1;
        RK_U32  reserved            : 30;
    } reg98_pp_out_ext;

    RK_U32 reg99_fuse;
    RK_U32 reg100_synthesis;
} post_processor_reg;

typedef struct  {
    struct {
        RK_U32  build_version   : 3;
        RK_U32  product_IDen    : 1;
        RK_U32  minor_version   : 8;
        RK_U32  major_version   : 4;
        RK_U32  product_numer   : 16;
    } reg0_id;

    struct {
        RK_U32  sw_dec_e            : 1;
        RK_U32  reserved4           : 3;
        RK_U32  sw_dec_irq_dis      : 1;
        RK_U32  reserved3           : 3;
        RK_U32  sw_dec_irq          : 1;
        RK_U32  reserved2           : 3;
        RK_U32  sw_dec_rdy_int      : 1;
        RK_U32  sw_dec_bus_int      : 1;
        RK_U32  sw_dec_buffer_int   : 1;
        RK_U32  sw_dec_aso_int      : 1;
        RK_U32  sw_dec_error_int    : 1;
        RK_U32  sw_dec_slice_int    : 1;
        RK_U32  sw_dec_timeout      : 1;
        RK_U32  reserved1           : 5;
        RK_U32  sw_dec_pic_inf      : 1;
        RK_U32  reserved0           : 7;
    } reg1_interrupt;

    struct {
        RK_U32  sw_dec_max_burst    : 5;
        RK_U32  sw_dec_scmd_dis     : 1;
        RK_U32  sw_dec_adv_pre_dis  : 1;
        RK_U32  sw_priority_mode    : 1; /* Not used */
        RK_U32  sw_dec_out_endian   : 1;
        RK_U32  sw_dec_in_endian    : 1;
        RK_U32  sw_dec_clk_gate_e   : 1;
        RK_U32  sw_dec_latency      : 6;
        RK_U32  sw_dec_out_tiled_e  : 1;
        RK_U32  sw_dec_data_disc_e  : 1;
        RK_U32  sw_dec_outswap32_e  : 1;
        RK_U32  sw_dec_inswap32_e   : 1;
        RK_U32  sw_dec_strendian_e  : 1;
        RK_U32  sw_dec_strswap32_e  : 1;
        RK_U32  sw_dec_timeout_e    : 1;
        RK_U32  sw_dec_axi_rn_id    : 8;
    } reg2_dec_ctrl;

    struct {
        RK_U32  sw_dec_axi_wr_id    : 8;
        RK_U32  sw_dec_ahb_hlock_e  : 1; /* Not used */
        RK_U32  sw_picord_count_e   : 1;
        RK_U32  sw_seq_mbaff_e      : 1;
        RK_U32  sw_reftopfirst_e    : 1;
        RK_U32  sw_write_mvs_e      : 1;
        RK_U32  sw_pic_fixed_quant  : 1;
        RK_U32  sw_filtering_dis    : 1;
        RK_U32  sw_dec_out_dis      : 1;
        RK_U32  sw_ref_topfield_e   : 1;
        RK_U32  sw_sorenson_e       : 1;
        RK_U32  sw_fwd_interlace_e  : 1;
        RK_U32  sw_pic_topfield_e   : 1;
        RK_U32  sw_pic_inter_e      : 1;
        RK_U32  sw_pic_b_e          : 1;
        RK_U32  sw_pic_fieldmode_e  : 1;
        RK_U32  sw_pic_interlace_e  : 1;
        RK_U32  sw_pjpeg_e          : 1;
        RK_U32  sw_divx3_e          : 1; /* Not used */
        RK_U32  sw_skip_mode        : 1;
        RK_U32  sw_rlc_mode_e       : 1;
        RK_U32  sw_dec_mode         : 4;
    } reg3;

    struct {
        RK_U32  sw_pic_mb_h_ext     : 3;
        RK_U32  sw_pic_mb_w_ext     : 3;
        RK_U32  sw_alt_scan_e       : 1;
        RK_U32  sw_mb_height_off    : 4;
        RK_U32  sw_pic_mb_height_p  : 8;
        RK_U32  sw_mb_width_off     : 4;
        RK_U32  sw_pic_mb_width     : 9;
    } reg4;

    /* stream decoding table selects */
    struct {
        RK_U32  sw_cb_dc_vlctable3 : 1;
        RK_U32  sw_cr_dc_vlctable3 : 1;
        RK_U32  sw_cb_dc_vlctable  : 1;
        RK_U32  sw_cr_dc_vlctable  : 1;
        RK_U32  sw_cb_ac_vlctable  : 1;
        RK_U32  sw_cr_ac_vlctable  : 1;
        RK_U32  sw_jpeg_stream_all : 1;
        RK_U32  sw_jpeg_filright_e : 1;
        RK_U32  sw_jpeg_mode       : 3;
        RK_U32  sw_jpeg_qtables    : 2;
        RK_U32  reserved0          : 12;
        RK_U32  sw_sync_marker_e   : 1;
        RK_U32  sw_strm0_start_bit : 6;
    } reg5;

    struct {
        RK_U32  sw_stream_len       : 24;
        RK_U32  sw_ch_8pix_ileav_e  : 1;
        RK_U32  sw_init_qp          : 6;
        RK_U32  sw_start_code_e     : 1;
    } reg6_stream_info;

    struct {
        RK_U32  sw_pjpeg_se         : 8;
        RK_U32  sw_pjpeg_ss         : 8;
        RK_U32  sw_pjpeg_al         : 4;
        RK_U32  sw_pjpeg_ah         : 4;
        RK_U32  sw_pjpeg_hdiv8      : 1;
        RK_U32  sw_pjpeg_wdiv8      : 1;
        RK_U32  sw_pjpeg_fildown_e  : 1;
        RK_U32  reserved0           : 5;
    } reg7;

    struct {
        RK_U32  sw_pjpeg_rest_freq  : 16;
        RK_U32  reserved0           : 16;
    } reg8;

    /* Not used for JPEG */
    struct {
        RK_U32  sw_stream1_len      : 24;
        RK_U32  sw_coeffs_part_am   : 4;
        RK_U32  reserved0           : 4;
    } reg9;

    /* Not used for JPEG */
    RK_U32 reg10_segment_map_base;

    /* Not used for JPEG */
    struct {
        RK_U32  sw_dct_start_bit_7   : 6;
        RK_U32  sw_dct_start_bit_6   : 6;
        RK_U32  sw_dct_start_bit_5   : 6;
        RK_U32  sw_dct_start_bit_4   : 6;
        RK_U32  sw_dct_start_bit_3   : 6;
        RK_U32  reserved0            : 2;
    } reg11;

    /* sw_rlc_vlc_base */
    RK_U32      reg12_input_stream_base;
    /* sw_dec_out_base */
    RK_U32      reg13_cur_pic_base; /* Decoder output base */
    RK_U32      reg14_sw_jpg_ch_out_base; /* sw_ch_out_base */

    struct {
        RK_U32  sw_jpeg_slice_h      : 8;
        RK_U32  sw_roi_en            : 1;
        RK_U32  sw_roi_decode        : 1;
        RK_U32  sw_roi_out_sel       : 2;
        RK_U32  sw_roi_distance      : 4;
        RK_U32  sw_roi_sample_size   : 2;
        RK_U32  sw_jpegroi_in_swap32 : 1;
        RK_U32  sw_jpegroi_in_endian : 1;
        RK_U32  sw_jpeg_height8_flag : 1;
        RK_U32  reserved0            : 11;
    } reg15;

    struct {
        RK_U32  sw_ac1_code1_cnt     : 2;
        RK_U32  reserved3            : 1;
        RK_U32  sw_ac1_code2_cnt     : 3;
        RK_U32  reserved2            : 1;
        RK_U32  sw_ac1_code3_cnt     : 4;
        RK_U32  sw_ac1_code4_cnt     : 5;
        RK_U32  sw_ac1_code5_cnt     : 6;
        RK_U32  reserved1            : 2;
        RK_U32  sw_ac1_code6_cnt     : 7;
        RK_U32  reserved0            : 1;
    } reg16;

    struct {
        RK_U32  sw_ac1_code7_cnt     : 8;
        RK_U32  sw_ac1_code8_cnt     : 8;
        RK_U32  sw_ac1_code9_cnt     : 8;
        RK_U32  sw_ac1_code10_cnt    : 8;
    } reg17;

    struct {
        RK_U32  sw_ac1_code11_cnt    : 8;
        RK_U32  sw_ac1_code12_cnt    : 8;
        RK_U32  sw_ac1_code13_cnt    : 8;
        RK_U32  sw_ac1_code14_cnt    : 8;
    } reg18;

    struct {
        RK_U32  sw_ac1_code15_cnt    : 8;
        RK_U32  sw_ac1_code16_cnt    : 8;
        RK_U32  sw_ac2_code1_cnt     : 2;
        RK_U32  reserved1            : 1;
        RK_U32  sw_ac2_code2_cnt     : 3;
        RK_U32  reserved0            : 1;
        RK_U32  sw_ac2_code3_cnt     : 4;
        RK_U32  sw_ac2_code4_cnt     : 5;
    } reg19;

    struct {
        RK_U32  sw_ac2_code5_cnt     : 6;
        RK_U32  reserved1            : 2;
        RK_U32  sw_ac2_code6_cnt     : 7;
        RK_U32  reserved0            : 1;
        RK_U32  sw_ac2_code7_cnt     : 8;
        RK_U32  sw_ac2_code8_cnt     : 8;
    } reg20;

    struct {
        RK_U32  sw_ac2_code9_cnt     : 8;
        RK_U32  sw_ac2_code10_cnt    : 8;
        RK_U32  sw_ac2_code11_cnt    : 8;
        RK_U32  sw_ac2_code12_cnt    : 8;
    } reg21;

    struct {
        RK_U32  sw_ac2_code13_cnt    : 8;
        RK_U32  sw_ac2_code14_cnt    : 8;
        RK_U32  sw_ac2_code15_cnt    : 8;
        RK_U32  sw_ac2_code16_cnt    : 8;
    } reg22;

    struct {
        RK_U32  sw_dc1_code1_cnt     : 2;
        RK_U32  reserved1            : 2;
        RK_U32  sw_dc1_code2_cnt     : 3;
        RK_U32  reserved0            : 1;
        RK_U32  sw_dc1_code3_cnt     : 4;
        RK_U32  sw_dc1_code4_cnt     : 4;
        RK_U32  sw_dc1_code5_cnt     : 4;
        RK_U32  sw_dc1_code6_cnt     : 4;
        RK_U32  sw_dc1_code7_cnt     : 4;
        RK_U32  sw_dc1_code8_cnt     : 4;
    } reg23;

    struct {
        RK_U32  sw_dc1_code9_cnt     : 4;
        RK_U32  sw_dc1_code10_cnt    : 4;
        RK_U32  sw_dc1_code11_cnt    : 4;
        RK_U32  sw_dc1_code12_cnt    : 4;
        RK_U32  sw_dc1_code13_cnt    : 4;
        RK_U32  sw_dc1_code14_cnt    : 4;
        RK_U32  sw_dc1_code15_cnt    : 4;
        RK_U32  sw_dc1_code16_cnt    : 4;
    } reg24;

    struct {
        RK_U32  sw_dc2_code1_cnt     : 2;
        RK_U32  reserved1            : 2;
        RK_U32  sw_dc2_code2_cnt     : 3;
        RK_U32  reserved0            : 1;
        RK_U32  sw_dc2_code3_cnt     : 4;
        RK_U32  sw_dc2_code4_cnt     : 4;
        RK_U32  sw_dc2_code5_cnt     : 4;
        RK_U32  sw_dc2_code6_cnt     : 4;
        RK_U32  sw_dc2_code7_cnt     : 4;
        RK_U32  sw_dc2_code8_cnt     : 4;
    } reg25;

    struct {
        RK_U32  sw_dc2_code9_cnt     : 4;
        RK_U32  sw_dc2_code10_cnt    : 4;
        RK_U32  sw_dc2_code11_cnt    : 4;
        RK_U32  sw_dc2_code12_cnt    : 4;
        RK_U32  sw_dc2_code13_cnt    : 4;
        RK_U32  sw_dc2_code14_cnt    : 4;
        RK_U32  sw_dc2_code15_cnt    : 4;
        RK_U32  sw_dc2_code16_cnt    : 4;
    } reg26;

    struct {
        RK_U32  sw_dc3_code1_cnt     : 2;
        RK_U32  reserved1            : 2;
        RK_U32  sw_dc3_code2_cnt     : 3;
        RK_U32  reserved0            : 1;
        RK_U32  sw_dc3_code3_cnt     : 4;
        RK_U32  sw_dc3_code4_cnt     : 4;
        RK_U32  sw_dc3_code5_cnt     : 4;
        RK_U32  sw_dc3_code6_cnt     : 4;
        RK_U32  sw_dc3_code7_cnt     : 4;
        RK_U32  sw_dc3_code8_cnt     : 4;
    } reg27;

    struct {
        RK_U32  sw_dc3_code9_cnt     : 4;
        RK_U32  sw_dc3_code10_cnt    : 4;
        RK_U32  sw_dc3_code11_cnt    : 4;
        RK_U32  sw_dc3_code12_cnt    : 4;
        RK_U32  sw_dc3_code13_cnt    : 4;
        RK_U32  sw_dc3_code14_cnt    : 4;
        RK_U32  sw_dc3_code15_cnt    : 4;
        RK_U32  sw_dc3_code16_cnt    : 4;
    } reg28;

    /* Not used for JPEG */
    RK_U32 reg29_reg39[11]; /* reg29 - reg39 */

    RK_U32      reg40_qtable_base;
    RK_U32      reg41_directmv_base;
    RK_U32      reg42_pjpeg_dccb_base;
    RK_U32      reg43_pjpeg_dccr_base;

    /* Not used for JPEG */
    RK_U32 reg44_reg50[7]; /* reg44 - reg50 */

    /* Not used for JPEG */
    struct {
        RK_U32  sw_refbu_y_offset   : 9;
        RK_U32  reserved0           : 3;
        RK_U32  sw_refbu_fparmod_e  : 1;
        RK_U32  sw_refbu_eval_e     : 1;
        RK_U32  sw_refbu_picid      : 5;
        RK_U32  sw_refbu_thr        : 12;
        RK_U32  sw_refbu_e          : 1;
    } reg51_refpicbuf_ctrl;

    /* Not used for JPEG */
    struct {
        RK_U32  sw_refbu_intra_sum  : 16;
        RK_U32  sw_refbu_hit_sum    : 16;
    } reg52_sum_inf;

    /* Not used for JPEG */
    struct {
        RK_U32  sw_refbu_mv_sum     : 22;
        RK_U32  reserved0           : 10;
    } reg53_sum_mv;

    /* Not used for JPEG */
    struct {
        RK_U32  reserved0           : 17;
        RK_U32  sw_dec_tiled_l      : 2; /* sw_priority_mode */
        RK_U32  sw_dec_vp8snap_e    : 1;
        RK_U32  sw_dec_mvc_prof     : 2;
        RK_U32  sw_dec_avs_prof     : 1;
        RK_U32  sw_dec_vp8_prof     : 1;
        RK_U32  sw_dec_vp7_prof     : 1;
        RK_U32  sw_dec_rtl_rom      : 1;
        RK_U32  sw_dec_rv_prof      : 2;
        RK_U32  sw_ref_buff2_exist  : 1;
        RK_U32  reserved            : 1;
        RK_U32  sw_dec_refbu_ilace  : 1;
        RK_U32  sw_dec_jpeg_exten   : 1;
    } reg54_synthesis_cfg;

    /* Not used for JPEG */
    struct {
        RK_U32  sw_apf_threshold    : 14;
        RK_U32  sw_refbu2_picid     : 5;
        RK_U32  sw_refbu2_thr       : 12;
        RK_U32  sw_refbu2_buf_e     : 1;
    } reg55;

    /* Not used for JPEG */
    struct {
        RK_U32  sw_refbu_bot_sum    : 16;
        RK_U32  sw_refbu_top_sum    : 16;
    } reg56_sum_of_partitions;

    RK_U32      reg57_decoder_fuse; /* Not used */
    RK_U32      reg58_debug;

    /* Not used for JPEG */
    RK_U32      reg59_addit_ch_st_base;
    post_processor_reg  post;
} JpegRegSet;

typedef struct JpegdIocRegInfo_t {
    JpegRegSet             regs;

    /* vepu_reg_num - vdpu_reg_num */
    RK_U32                 regs_diff[164 - JPEGD_REG_NUM];
    RegExtraInfo           extra_info;
} JpegdIocRegInfo;

#endif
