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

#ifndef __HAL_VP8E_VEPU2_REG_H__
#define __HAL_VP8E_VEPU2_REG_H__

#include "rk_type.h"

typedef struct {
    union {
        struct {
            RK_U32 y1_quant_dc : 14;
            RK_U32 : 2;
            RK_U32 y2_quant_dc : 14;
            RK_U32 : 2;
        } num_0;

        struct {
            RK_U32 ch_quant_dc : 14;
            RK_U32 : 2;
            RK_U32 y1_quant_ac : 14;
            RK_U32 : 2;
        } num_1;

        struct {
            RK_U32 y2_quant_ac : 14;
            RK_U32 : 2;
            RK_U32 ch_quant_ac : 14;
            RK_U32 : 2;
        } num_2;

        struct {
            RK_U32 y1_zbin_dc : 9;
            RK_U32 y2_zbin_dc : 9;
            RK_U32 ch_zbin_dc : 9;
            RK_U32 : 5;
        } num_3;

        struct {
            RK_U32 y1_zbin_ac : 9;
            RK_U32 y2_zbin_ac : 9;
            RK_U32 ch_zbin_ac : 9;
            RK_U32 : 5;
        } num_4;

        struct {
            RK_U32 y1_round_dc : 8;
            RK_U32 y2_round_dc : 8;
            RK_U32 ch_round_dc : 8;
            RK_U32 : 8;
        } num_5;

        struct {
            RK_U32 y1_round_ac : 8;
            RK_U32 y2_round_ac : 8;
            RK_U32 ch_round_ac : 8;
            RK_U32 : 8;
        } num_6;

        struct {
            RK_U32 y1_dequant_dc : 8;
            RK_U32 y2_dequant_dc : 9;
            RK_U32 ch_dequant_dc : 8;
            RK_U32 filter_level : 6;
            RK_U32 : 1;
        } num_7;

        struct {
            RK_U32 y1_dequant_ac : 9;
            RK_U32 y2_dequant_ac : 9;
            RK_U32 ch_dequant_ac : 9;
            RK_U32 : 5;
        } num_8;

    } sw0_26[27];

    struct {
        RK_U32 base_segment_map : 32;
    } sw27;

    struct {
        RK_U32 b_mode_0_penalty : 12;
        RK_U32 : 4;
        RK_U32 b_mode_1_penalty : 12;
        RK_U32 : 4;
    } sw28_32[5];

    struct {
        RK_U32 mode0_penalty : 12;
        RK_U32 : 4;
        RK_U32 mode1_penalty : 12;
        RK_U32 : 4;
    } sw33;

    struct {
        RK_U32 mode2_penalty : 12;
        RK_U32 : 4;
        RK_U32 mode3_penalty : 12;
        RK_U32 : 4;
    } sw34;

    RK_U32 sw35_39[5];

    struct {
        RK_U32 cost_inter : 12;
        RK_U32 : 4;
        RK_U32 lf_ref_delta0 : 7;
        RK_U32 : 1;
        RK_U32 lf_mode_delta0 : 7;
        RK_U32 : 1;
    } sw40;

    struct {
        RK_U32 cost_golden_ref : 12;
        RK_U32 : 4;
        RK_U32 dmv_cost_const : 12;
        RK_U32 : 4;
    } sw41;

    struct {
        RK_U32 lf_ref_delta2 : 7;
        RK_U32 : 1;
        RK_U32 lf_ref_delta1 : 7;
        RK_U32 : 1;
        RK_U32 lf_ref_delta3 : 7;
        RK_U32 : 9;
    } sw42;

    struct {
        RK_U32 lf_mode_delta2 : 7;
        RK_U32 : 1;
        RK_U32 lf_mode_delta1 : 7;
        RK_U32 : 1;
        RK_U32 lf_mode_delta3 : 7;
        RK_U32 : 9;
    } sw43;

    struct {
        RK_U32 base_partition1 : 32;
    } sw44;

    struct {
        RK_U32 base_partition2 : 32;
    } sw45;

    struct {
        RK_U32 intra_area_right : 8;
        RK_U32 intra_area_left : 8;
        RK_U32 intra_area_bottom : 8;
        RK_U32 intra_area_top : 8;
    } sw46;

    struct {
        RK_U32 cir_interval : 16;
        RK_U32 cir_start : 16;
    } sw47;

    struct {
        RK_U32 base_in_lum : 32;
    } sw48;

    struct {
        RK_U32 base_in_cb : 32;
    } sw49;

    struct {
        RK_U32 base_in_cr : 32;
    } sw50;

    struct {
        RK_U32 strm_hdr_rem1 : 32;
    } sw51;

    struct {
        RK_U32 strm_hdr_rem2 : 32;
    } sw52;

    struct {
        RK_U32 strm_buf_limit : 32;
    } sw53;

    struct {
        RK_U32 val : 32;
    } sw54;

    RK_U32 sw55;

    struct {
        RK_U32 base_ref_lum : 32;
    } sw56;

    struct {
        RK_U32 base_ref_chr : 32;
    } sw57;

    struct {
        RK_U32 : 11;
        RK_U32 qp_sum : 21;
    } sw58;

    struct {
        RK_U32 : 8;
        RK_U32 slice_size : 7;
        RK_U32 strea_mmode : 1;
        RK_U32 inter4_restrict : 1;
        RK_U32 transform_8x8 : 1;
        RK_U32 : 2;
        RK_U32 cabac_enable : 1;
        RK_U32 cabac_init_idc : 2;
        RK_U32 : 1;
        RK_U32 deblocking : 2;
        RK_U32 : 2;
        RK_U32 disable_qp_mv : 1;
        RK_U32 : 3;
    } sw59;

    struct {
        RK_U32 y_fill : 4;
        RK_U32 x_fill : 2;
        RK_U32 : 2;
        RK_U32 skip_penalty : 8;
        RK_U32 start_offset : 6;
        RK_U32 : 10;
    } sw60;

    struct {
        RK_U32 row_length : 14;
        RK_U32 : 2;
        RK_U32 lum_offset : 3;
        RK_U32 : 1;
        RK_U32 chr_offset : 3;
        RK_U32 : 9;
    } sw61;

    struct {
        RK_U32 rlc_sum : 23;
        RK_U32 : 9;
    } sw62;

    struct {
        RK_U32 base_rec_lum : 32;
    } sw63;

    struct {
        RK_U32 base_rec_chr : 32;
    } sw64;

    struct {
        RK_U32 y1_quant_ac : 14;
        RK_U32 y1_zbin_ac : 9;
        RK_U32 y1_round_ac : 8;
        RK_U32 : 1;
    } sw65;

    struct {
        RK_U32 y2_quant_dc : 14;
        RK_U32 y2_zbin_dc : 9;
        RK_U32 y2_round_dc : 8;
        RK_U32 : 1;
    } sw66;

    struct {
        RK_U32 y2_quant_ac : 14;
        RK_U32 y2_zbin_ac : 9;
        RK_U32 y2_round_ac : 8;
        RK_U32 : 1;
    } sw67;

    struct {
        RK_U32 ch_quant_dc : 14;
        RK_U32 ch_zbin_dc : 9;
        RK_U32 ch_round_dc : 8;
        RK_U32 : 1;
    } sw68;

    struct {
        RK_U32 ch_quant_ac : 14;
        RK_U32 ch_zbin_ac : 9;
        RK_U32 ch_round_ac : 8;
        RK_U32 : 1;
    } sw69;

    struct {
        RK_U32 y1_dequant_dc : 8;
        RK_U32 y1_dequant_ac : 9;
        RK_U32 y2_dequant_dc : 9;
        RK_U32 mv_ref_idx : 2 ;
        RK_U32 : 4;
    } sw70;

    struct {
        RK_U32 y2_dequant_ac : 9;
        RK_U32 ch_dequant_dc : 8;
        RK_U32 ch_dequant_ac : 9;
        RK_U32 mv_ref_idx2 : 2;
        RK_U32 ref2_enable : 1;
        RK_U32 segment_enable : 1;
        RK_U32 segment_map_update : 1;
        RK_U32 : 1;
    } sw71;

    struct {
        RK_U32 bool_enc_value : 32;
    } sw72;

    struct {
        RK_U32 bool_enc_range : 8;
        RK_U32 bool_enc_value_bits : 5;
        RK_U32 dct_partition_count : 2;
        RK_U32 filter_level : 6;
        RK_U32 filter_sharpness : 3;
        RK_U32 golden_penalty : 8;
    } sw73;

    struct {
        RK_U32 nal_size_write : 1;
        RK_U32 : 1;
        RK_U32 input_rot : 2;
        RK_U32 input_format : 4;
        RK_U32 : 8;
        RK_U32 num_slices_ready : 8;
        RK_U32 mad_threshold : 6;
        RK_U32 : 2;
    } sw74;

    struct {
        RK_U32 inter_favor : 16;
        RK_U32 ip_intra_16_favor : 16;
    } sw75;

    struct {
        RK_U32 base_ref_lum2 : 32;
    } sw76;

    struct {
        RK_U32 base_stream : 32;
    } sw77;

    struct {
        RK_U32 base_control : 32;
    } sw78;

    struct {
        RK_U32 base_next_lum : 32;
    } sw79;

    struct {
        RK_U32 base_mv_write : 32;
    } sw80;

    struct {
        RK_U32 base_cabac_ctx : 32;
    } sw81;

    struct {
        RK_U32 roi1_right : 8;
        RK_U32 roi1_left : 8;
        RK_U32 roi1_bottom : 8;
        RK_U32 roi1_top : 8;
    } sw82;

    struct {
        RK_U32 roi2_right : 8;
        RK_U32 roi2_left : 8;
        RK_U32 roi2_bottom : 8;
        RK_U32 roi2_top : 8;
    } sw83;

    RK_U32 sw84_93[10];

    struct {
        RK_U32 stab_gmvx : 6;
        RK_U32 stab_mode : 2;
        RK_U32 stab_minimum : 24;
    } sw94;

    struct {
        RK_U32 rgb_coeff_a : 16;
        RK_U32 rgb_coeff_b : 16;
    } sw95;

    struct {
        RK_U32 rgb_coeff_c : 16;
        RK_U32 rgb_coeff_e : 16;
    } sw96;

    struct {
        RK_U32 rgb_coeff_f : 16;
        RK_U32 : 16;
    } sw97;

    struct {
        RK_U32 r_mask_msb : 5;
        RK_U32 : 3;
        RK_U32 g_mask_msb : 5;
        RK_U32 : 3;
        RK_U32 b_mask_msb : 5;
        RK_U32 : 11;
    } sw98;

    struct {
        RK_U32 split_mv : 1;
        RK_U32 dmv_penalty_4p : 10;
        RK_U32 dmv_penalty_qp : 10;
        RK_U32 dmv_penalty_1p : 10;
        RK_U32 : 1;
    } sw99;

    struct {
        RK_U32 y1_quant_dc : 14;
        RK_U32 y1_zbin_dc : 9;
        RK_U32 y1_round_dc : 8;
        RK_U32 : 1;
    } sw100;

    RK_U32 sw101;

    struct {
        RK_U32 mvc_inter_view_flag : 1;
        RK_U32 mvc_temporal_id : 3;
        RK_U32 mvc_priority_id : 3;
        RK_U32 mvc_anchor_pic_flag : 1;
        RK_U32 mvc_view_id : 3;
        RK_U32 split_penalty_4x4 : 9;
        RK_U32 zero_mv_favor : 4;
        RK_U32 : 8;
    } sw102;

    struct {
        RK_U32 enable : 1;
        RK_U32 : 3;
        RK_U32 encoding_mode : 2;
        RK_U32 picture_type : 2;
        RK_U32 width : 9;
        RK_U32 : 3;
        RK_U32 height : 9;
        RK_U32 : 3;
    } sw103;

    struct {
        RK_U32 mad_count : 16;
        RK_U32 mb_count : 16;
    } sw104;

    struct {
        RK_U32 val : 32;
    } sw105;

    struct {
        RK_U32 base_ref_chr2 : 32;
    } sw106;

    struct {
        RK_U32 split_penalty_8x4 : 10;
        RK_U32 split_penalty_8x8 : 10;
        RK_U32 split_penalty_16x8 : 10;
        RK_U32 : 2;
    } sw107;

    struct {
        RK_U32 base_prob_count : 32;
    } sw108;

    struct {
        RK_U32 val : 16;
        RK_U32 int_slice_ready : 1;
        RK_U32 : 3;
        RK_U32 rec_write_disable : 1;
        RK_U32 : 3;
        RK_U32 mv_write : 1;
        RK_U32 : 7;
    } sw109;

    RK_U32 sw110_119[10];

    struct {
        RK_U32 penalty_0 : 8;
        RK_U32 penalty_1 : 8;
        RK_U32 penalty_2 : 8;
        RK_U32 penalty_3 : 8;
    } sw120_183[64];

} Vp8eVepu2Reg_t;

#endif /*__HAL_VP8E_VEPU2_REG_H__*/
