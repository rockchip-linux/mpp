/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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
#ifndef __HAL_VP8D_VDPU1_REG_H__
#define __HAL_VP8D_VDPU1_REG_H__

#include "rk_type.h"

#define VP8HWD_VP7             1
#define VP8HWD_VP8             2
#define VP8HWD_WEBP            3

#define DEC_MODE_VP7           9
#define DEC_MODE_VP8           10

#define VP8D_PROB_TABLE_SIZE  (1<<16)
#define VP8D_MAX_SEGMAP_SIZE  (2048 + 1024)

#define VP8D_REG_NUM    101

typedef struct  {
    struct {
        RK_U32  build_version       : 3;
        RK_U32  product_IDen        : 1;
        RK_U32  minor_version       : 8;
        RK_U32  major_version       : 4;
        RK_U32  product_numer       : 16;
    } reg0_id;

    struct {
        RK_U32  sw_dec_e            : 1;
        RK_U32  reserve4            : 3;
        RK_U32  sw_dec_irq_dis      : 1;
        RK_U32  reserve3            : 3;
        RK_U32  sw_dec_irq          : 1;
        RK_U32  reserve2            : 3;
        RK_U32  sw_dec_rdy_int      : 1;
        RK_U32  sw_dec_bus_int      : 1;
        RK_U32  sw_dec_buffer_int   : 1;
        RK_U32  sw_dec_aso_int      : 1;
        RK_U32  sw_dec_error_int    : 1;
        RK_U32  sw_dec_slice_int    : 1;
        RK_U32  sw_dec_timeout      : 1;
        RK_U32  reseved1            : 5;
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
        RK_U32  sw_dec_out_tiled_e  : 1; /* Not used */
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
        RK_U32  sw_pic_mb_hight_p   : 8;
        RK_U32  sw_mb_width_off     : 4;
        RK_U32  sw_pic_mb_width     : 9;
    } reg4;

    struct {
        RK_U32  sw_boolean_range    : 8;
        RK_U32  sw_boolean_value    : 8;
        RK_U32  reserved1           : 2;
        RK_U32  sw_strm1_start_bit  : 6;
        RK_U32  reserved0           : 2;
        RK_U32  sw_strm0_start_bit  : 6;
    } reg5;

    struct {
        RK_U32  sw_stream_len       : 24;
        RK_U32  sw_ch_8pix_ileav_e  : 1;
        RK_U32  sw_init_qp          : 6;
        RK_U32  sw_start_code_e     : 1;
    } reg6;

    struct {
        RK_U32  reserved1           : 5;
        RK_U32  sw_vp7_version      : 1;
        RK_U32  sw_dc_match1        : 3;
        RK_U32  sw_dc_match0        : 3;
        RK_U32  sw_bilin_mc_e       : 1;
        RK_U32  sw_ch_mv_res        : 1;
        RK_U32  reserved0           : 6;
        RK_U32  sw_dct2_start_bit   : 6;
        RK_U32  sw_dct1_start_bit   : 6;
    } reg7;

    struct {
        RK_U32  sw_dc_comp1         : 16;
        RK_U32  sw_dc_comp0         : 16;
    } reg8;

    struct {
        RK_U32  sw_stream1_len      : 24;
        RK_U32  sw_coeffs_part_am   : 4;
        RK_U32  reserved0           : 4;
    } reg9;

    RK_U32 reg10_segment_map_base;

    struct {
        RK_U32  sw_dct_start_bit_7   : 6;
        RK_U32  sw_dct_start_bit_6   : 6;
        RK_U32  sw_dct_start_bit_5   : 6;
        RK_U32  sw_dct_start_bit_4   : 6;
        RK_U32  sw_dct_start_bit_3   : 6;
        RK_U32  reserved0            : 2;
    } reg11;

    RK_U32      reg12_input_stream_base;
    RK_U32      reg13_cur_pic_base; /* Decoder output base */
    RK_U32      reg14_ref0_base; /* sw_ch_out_base */
    RK_U32      reg15_17[3]; /* Not used */

    RK_U32      reg18_golden_ref_base;

    union {
        RK_U32 alternate_ref_base; /* sw_refer5_base */
        struct {
            RK_U32  sw_scan_map_5   : 6;
            RK_U32  sw_scan_map_4   : 6;
            RK_U32  sw_scan_map_3   : 6;
            RK_U32  sw_scan_map_2   : 6;
            RK_U32  sw_scan_map_1   : 6;
            RK_U32  reserved0       : 2;
        };
    } reg19;

    struct {
        RK_U32  sw_scan_map_10      : 6;
        RK_U32  sw_scan_map_9       : 6;
        RK_U32  sw_scan_map_8       : 6;
        RK_U32  sw_scan_map_7       : 6;
        RK_U32  sw_scan_map_6       : 6;
        RK_U32  reserved0           : 2;
    } reg20;

    struct {
        RK_U32  sw_scan_map_15      : 6;
        RK_U32  sw_scan_map_14      : 6;
        RK_U32  sw_scan_map_13      : 6;
        RK_U32  sw_scan_map_12      : 6;
        RK_U32  sw_scan_map_11      : 6;
        RK_U32  reserved0           : 2;
    } reg21;

    RK_U32      reg_dct_strm0_base[5]; /* From reg22 to reg26 */
    RK_U32      reg27_bitpl_ctrl_base;
    RK_U32      reg_dct_strm1_base[2]; /* From reg28 to reg29 */

    struct {
        RK_U32  sw_filt_mb_adj_3    : 7;
        RK_U32  sw_filt_mb_adj_2    : 7;
        RK_U32  sw_filt_mb_adj_1    : 7;
        RK_U32  sw_filt_mb_adj_0    : 7;
        RK_U32  sw_filt_sharpness   : 3;
        RK_U32  sw_filt_type        : 1;
    } reg30;

    struct {
        RK_U32  sw_filt_ref_adj_3   : 7;
        RK_U32  sw_filt_ref_adj_2   : 7;
        RK_U32  sw_filt_ref_adj_1   : 7;
        RK_U32  sw_filt_ref_adj_0   : 7;
        RK_U32  reserved0           : 4;
    } reg31;

    struct {
        RK_U32  sw_filt_level_3     : 6;
        RK_U32  sw_filt_level_2     : 6;
        RK_U32  sw_filt_level_1     : 6;
        RK_U32  sw_filt_level_0     : 6;
        RK_U32  reserved0           : 8;
    } reg32;

    struct {
        RK_U32  sw_quant_1          : 11;
        RK_U32  sw_quant_0          : 11;
        RK_U32  sw_quant_delta_1    : 5;
        RK_U32  sw_quant_delta_0    : 5;
    } reg33;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_1_1  : 10;
        RK_U32  sw_pred_bc_tap_1_0  : 10;
        RK_U32  sw_pred_bc_tap_0_3  : 10;
    } reg34;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_2_0  : 10;
        RK_U32  sw_pred_bc_tap_1_3  : 10;
        RK_U32  sw_pred_bc_tap_1_2  : 10;
    } reg35;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_2_3  : 10;
        RK_U32  sw_pred_bc_tap_2_2  : 10;
        RK_U32  sw_pred_bc_tap_2_1  : 10;
    } reg36;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_3_2  : 10;
        RK_U32  sw_pred_bc_tap_3_1  : 10;
        RK_U32  sw_pred_bc_tap_3_0  : 10;
    } reg37;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_4_1  : 10;
        RK_U32  sw_pred_bc_tap_4_0  : 10;
        RK_U32  sw_pred_bc_tap_3_3  : 10;
    } reg38;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_5_0  : 10;
        RK_U32  sw_pred_bc_tap_4_3  : 10;
        RK_U32  sw_pred_bc_tap_4_2  : 10;
    } reg39;

    RK_U32      reg40_qtable_base;
    RK_U32      reg41_directmv_base;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_5_3  : 10;
        RK_U32  sw_pred_bc_tap_5_2  : 10;
        RK_U32  sw_pred_bc_tap_5_1  : 10;
    } reg42;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_6_2  : 10;
        RK_U32  sw_pred_bc_tap_6_1  : 10;
        RK_U32  sw_pred_bc_tap_6_0  : 10;
    } reg43;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_7_1  : 10;
        RK_U32  sw_pred_bc_tap_7_0  : 10;
        RK_U32  sw_pred_bc_tap_6_3  : 10;
    } reg44;

    struct {
        RK_U32  sw_pred_tap_6_4     : 2;
        RK_U32  sw_pred_tap_6_M1    : 2;
        RK_U32  sw_pred_tap_4_4     : 2;
        RK_U32  sw_pred_tap_4_M1    : 2;
        RK_U32  sw_pred_tap_2_4     : 2;
        RK_U32  sw_pred_tap_2_M1    : 2;
        RK_U32  sw_pred_bc_tap_7_3  : 10;
        RK_U32  sw_pred_bc_tap_7_2  : 10;
    } reg45;

    struct {
        RK_U32  sw_quant_3          : 11;
        RK_U32  sw_quant_2          : 11;
        RK_U32  sw_quant_delta_3    : 5;
        RK_U32  sw_quant_delta_2    : 5;
    } reg46;

    struct {
        RK_U32  sw_quant_5          : 11;
        RK_U32  sw_quant_4          : 11;
        RK_U32  reserved            : 5;
        RK_U32  sw_quant_delta_4    : 5;
    } reg47;

    struct {
        RK_U32  reserved0           : 15;
        RK_U32  sw_startmb_y        : 8;
        RK_U32  sw_startmb_x        : 9;
    } reg48;

    struct {
        RK_U32  reserved0           : 2;
        RK_U32  sw_pred_bc_tap_0_2  : 10;
        RK_U32  sw_pred_bc_tap_0_1  : 10;
        RK_U32  sw_pred_bc_tap_0_0  : 10;
    } reg49;

    RK_U32 reg50;

    struct {
        RK_U32  sw_refbu_y_offset   : 9;
        RK_U32  reserve0            : 3;
        RK_U32  sw_refbu_fparmod_e  : 1;
        RK_U32  sw_refbu_eval_e     : 1;
        RK_U32  sw_refbu_picid      : 5;
        RK_U32  sw_refbu_thr        : 12;
        RK_U32  sw_refbu_e          : 1;
    } reg51_refpicbuf_ctrl;

    struct {
        RK_U32  sw_refbu_intra_sum  : 16;
        RK_U32  sw_refbu_hit_sum    : 16;
    } reg52_sum_inf;

    struct {
        RK_U32  sw_refbu_mv_sum     : 22;
        RK_U32  reserve0            : 10;
    } reg53_sum_mv;

    struct {
        RK_U32  reserve0            : 17;
        /* sw_priority_mode */
        RK_U32  sw_dec_tiled_l      : 2;
        RK_U32  sw_dec_vp8snap_e    : 1;
        RK_U32  sw_dec_mvc_prof     : 2;
        RK_U32  sw_dec_avs_prof     : 1;
        RK_U32  sw_dec_vp8_prof     : 1;
        RK_U32  sw_dec_vp7_prof     : 1;
        RK_U32  sw_dec_rtl_rom      : 1;
        RK_U32  sw_dec_rv_prof      : 2;
        RK_U32  sw_ref_buff2_exist  : 1;
        RK_U32  reserve1            : 1;
        RK_U32  sw_dec_refbu_ilace  : 1;
        RK_U32  sw_dec_jpeg_exten   : 1;
    } reg54_synthesis_cfg;

    struct {
        RK_U32  sw_apf_threshold    : 14;
        RK_U32  sw_refbu2_picid     : 5;
        RK_U32  sw_refbu2_thr       : 12;
        RK_U32  sw_refbu2_buf_e     : 1;
    } reg55;

    struct {
        RK_U32  sw_refbu_bot_sum    : 16;
        RK_U32  sw_refbu_top_sum    : 16;
    } reg56_sum_of_partitions;

    /* Not used */
    RK_U32      reg57_decoder_fuse;
    RK_U32      reg58_debug;

    RK_U32      reg59_addit_ch_st_base;
    RK_U32      reg60_100_post_processor[41];
} VP8DRegSet_t;

#endif
