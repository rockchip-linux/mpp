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

#ifndef __HAL_VP8E_VEPU1_REG_H__
#define __HAL_VP8E_VEPU1_REG_H__

#include "rk_type.h"

typedef struct {
    RK_U32 sw0;

    struct {
        RK_U32 val : 32;
    } sw1;

    struct {
        RK_U32 val : 32;
    } sw2;

    RK_U32 sw3_4[2];

    struct {
        RK_U32 base_stream : 32;
    } sw5;

    struct {
        RK_U32 base_control : 32;
    } sw6;

    struct {
        RK_U32 base_ref_lum : 32;
    } sw7;

    struct {
        RK_U32 base_ref_chr : 32;
    } sw8;

    struct {
        RK_U32 base_rec_lum : 32;
    } sw9;

    struct {
        RK_U32 base_rec_chr : 32;
    } sw10;

    struct {
        RK_U32 base_in_lum : 32;
    } sw11;

    struct {
        RK_U32 base_in_cb : 32;
    } sw12;

    struct {
        RK_U32 base_in_cr : 32;
    } sw13;

    struct {
        RK_U32 enable : 1;
        RK_U32 encoding_mode : 2;
        RK_U32 picture_type : 2;
        RK_U32 : 1;
        RK_U32 rec_write_disable : 1;
        RK_U32 : 3;
        RK_U32 height : 9;
        RK_U32 width : 9;
        RK_U32 int_slice_ready : 1;
        RK_U32 nal_size_write : 1;
        RK_U32 mv_write : 1;
        RK_U32 int_timeout : 1;
    } sw14;

    struct {
        RK_U32 input_rot : 2;
        RK_U32 input_format : 4;
        RK_U32 y_fill : 4;
        RK_U32 x_fill : 2;
        RK_U32 row_length : 14;
        RK_U32 lum_offset : 3;
        RK_U32 chr_offset : 3;
    } sw15;

    struct {
        RK_U32 base_ref_lum2 : 32;
    } sw16;

    struct {
        RK_U32 base_ref_chr2 : 32;
    } sw17;

    struct {
        RK_U32 ip_intra16_favor : 16;
        RK_U32 stream_mode : 1;
        RK_U32 inter_4_restrict : 1;
        RK_U32 cabac_enable : 1;
        RK_U32 cabac_initidc : 2;
        RK_U32 transform_8x8 : 1;
        RK_U32 disable_qp_mv : 1;
        RK_U32 slice_size : 7;
        RK_U32 deblocking : 2;
    } sw18;

    struct {
        RK_U32 dmv_penalty1p : 10;
        RK_U32 dmv_penalty4p : 10;
        RK_U32 dmv_penaltyqp : 10;
        RK_U32 split_mv : 1;
        RK_U32 : 1;
    } sw19;

    struct {
        RK_U32 split_penalty_8x4 : 10;
        RK_U32 split_penalty_8x8 : 10;
        RK_U32 split_penalty_16x8 : 10;
        RK_U32 : 2;
    } sw20;

    struct {
        RK_U32 inter_favor : 16;
        RK_U32 num_slices_ready : 8;
        RK_U32 skip_penalty : 8;
    } sw21;

    struct {
        RK_U32 strm_hdr_rem1 : 32;
    } sw22;

    struct {
        RK_U32 strm_hdr_rem2 : 32;
    } sw23;

    struct {
        RK_U32 strm_buf_limit : 32;
    } sw24;

    struct {
        RK_U32 qp_sum : 21;
        RK_U32 : 1;
        RK_U32 mad_threshold : 6;
        RK_U32 mad_qp_delta : 4;
    } sw25;

    struct {
        RK_U32 base_prob_count : 32;
    } sw26;

    struct {
        RK_U32 y1_quant_dc : 14;
        RK_U32 y1_zbin_dc : 9;
        RK_U32 y1_round_dc : 8;
        RK_U32 : 1;
    } sw27;

    struct {
        RK_U32 y1_quant_ac : 14;
        RK_U32 y1_zbin_ac : 9;
        RK_U32 y1_round_ac : 8;
        RK_U32 : 1;
    } sw28;

    struct {
        RK_U32 y2_quant_dc : 14;
        RK_U32 y2_zbin_dc : 9;
        RK_U32 y2_round_dc : 8;
        RK_U32 : 1;
    } sw29;

    struct {
        RK_U32 y2_quant_ac : 14;
        RK_U32 y2_zbin_ac : 9;
        RK_U32 y2_round_ac : 8;
        RK_U32 : 1;
    } sw30;

    struct {
        RK_U32 ch_quant_dc : 14;
        RK_U32 ch_zbin_dc : 9;
        RK_U32 ch_round_dc : 8;
        RK_U32 : 1;
    } sw31;

    struct {
        RK_U32 ch_quant_ac : 14;
        RK_U32 ch_zbin_ac : 9;
        RK_U32 ch_round_ac : 8;
        RK_U32 : 1;
    } sw32;

    struct {
        RK_U32 y1_dequant_dc : 8;
        RK_U32 y1_dequant_ac : 9;
        RK_U32 y2_dequant_dc : 9;
        RK_U32 mv_ref_idx : 2;
        RK_U32 : 4;
    } sw33;

    struct {
        RK_U32 y2_dequant_ac : 9;
        RK_U32 ch_dequant_dc : 8;
        RK_U32 ch_dequant_ac : 9;
        RK_U32 mv_ref_idx2 : 2;
        RK_U32 ref2_enable : 1;
        RK_U32 segment_enable : 1;
        RK_U32 segment_map_update : 1;
        RK_U32 : 1;
    } sw34;

    struct {
        RK_U32 bool_enc_value : 32;
    } sw35;

    struct {
        RK_U32 bool_enc_range : 8;
        RK_U32 bool_enc_value_bits : 5;
        RK_U32 dct_partition_count : 2;
        RK_U32 filter_level : 6;
        RK_U32 filter_sharpness : 3;
        RK_U32 golden_penalty : 8;
    } sw36;

    struct {
        RK_U32 rlc_sum : 23;
        RK_U32 start_offset : 6;
        RK_U32 : 3;
    } sw37;

    struct {
        RK_U32 mb_count : 16;
        RK_U32 mad_count : 16;
    } sw38;

    struct {
        RK_U32 base_next_lum : 32;
    } sw39;

    struct {
        RK_U32 stab_minimum : 24;
        RK_U32 : 6;
        RK_U32 stab_mode : 2;
    } sw40;
    RK_U32 sw41_50[10];

    struct {
        RK_U32 base_cabac_ctx : 32;
    } sw51;

    struct {
        RK_U32 base_mv_write : 32;
    } sw52;

    struct {
        RK_U32 rgb_coeff_a : 16;
        RK_U32 rgb_coeff_b : 16;
    } sw53;

    struct {
        RK_U32 rgb_coeff_c : 16;
        RK_U32 rgb_coeff_e : 16;
    } sw54;

    struct {
        RK_U32 rgb_coeff_f : 16;
        RK_U32 r_mask_msb : 5;
        RK_U32 g_mask_msb : 5;
        RK_U32 b_mask_msb : 5;
        RK_U32 : 1;
    } sw55;

    struct {
        RK_U32 intra_area_bottom : 8;
        RK_U32 intra_area_top : 8;
        RK_U32 intra_area_right : 8;
        RK_U32 intra_area_left : 8;
    } sw56;

    struct {
        RK_U32 cir_interval : 16;
        RK_U32 cir_start : 16;
    } sw57;

    struct {
        RK_U32 base_partition1 : 32;
    } sw58;

    struct {
        RK_U32 base_partition2 : 32;
    } sw59;

    struct {
        RK_U32 roi1_bottom : 8;
        RK_U32 roi1_top : 8;
        RK_U32 roi1_right : 8;
        RK_U32 roi1_left : 8;
    } sw60;

    struct {
        RK_U32 roi2_bottom : 8;
        RK_U32 roi2_top : 8;
        RK_U32 roi2_right : 8;
        RK_U32 roi2_left : 8;
    } sw61;

    struct {
        RK_U32 roi2_delta_qp : 4;
        RK_U32 roi1_delta_qp : 4;
        RK_U32 mvc_inter_view_flag : 1;
        RK_U32 mvc_anchor_pic_flag : 1;
        RK_U32 mvc_temporal_id : 3;
        RK_U32 mvc_view_id : 3;
        RK_U32 mvc_priority_id : 3;
        RK_U32 split_penalty4x4 : 9;
        RK_U32 zero_mv_favor : 4;
    } sw62;

    RK_U32 sw63;

    struct {
        RK_U32 mode0_penalty : 12;
        RK_U32 mode1_penalty : 12;
        RK_U32 : 8;
    } sw64;

    struct {
        RK_U32 mode2_penalty : 12;
        RK_U32 mode3_penalty : 12;
        RK_U32 : 8;
    } sw65;

    struct {
        RK_U32 b_mode_0_penalty : 12;
        RK_U32 b_mode_1_penalty : 12;
        RK_U32 : 8;
    } sw66_70[5];

    struct {
        RK_U32 base_segment_map : 32;
    } sw71;

    union {
        struct {
            RK_U32 y1_quant_dc : 14;
            RK_U32 y1_zbin_dc : 9;
            RK_U32 y1_round_dc : 8;
            RK_U32 : 1;
        } num_0;

        struct {
            RK_U32 y1_quant_ac : 14;
            RK_U32 y1_zbin_ac : 9;
            RK_U32 y1_round_ac : 8;
            RK_U32 : 1;
        } num_1;

        struct {
            RK_U32 y2_quant_dc : 14;
            RK_U32 y2_zbin_dc : 9;
            RK_U32 y2_round_dc : 8;
            RK_U32 : 1;
        } num_2;

        struct {
            RK_U32 y2_quant_ac : 14;
            RK_U32 y2_zbin_ac : 9;
            RK_U32 y2_round_ac : 8;
            RK_U32 : 1;
        } num_3;

        struct {
            RK_U32 ch_quant_dc : 14;
            RK_U32 ch_zbin_dc : 9;
            RK_U32 ch_round_dc : 8;
            RK_U32 : 1;
        } num_4;

        struct {
            RK_U32 ch_quant_ac : 14;
            RK_U32 ch_zbin_ac : 9;
            RK_U32 ch_round_ac : 8;
            RK_U32 : 1;
        } num_5;

        struct {
            RK_U32 y1_dequant_dc : 8;
            RK_U32 y1_dequant_ac : 9;
            RK_U32 y2_dequant_dc : 9;
            RK_U32 : 6;
        } num_6;

        struct {
            RK_U32 y2_dequant_ac : 9;
            RK_U32 ch_dequant_dc : 8;
            RK_U32 ch_dequant_ac : 9;
            RK_U32 filter_level : 6;
        } num_7;

    } sw72_95[24];

    struct {
        RK_U32 penalty_0 : 8;
        RK_U32 penalty_1 : 8;
        RK_U32 penalty_2 : 8;
        RK_U32 penalty_3 : 8;
    } sw96_127[32];

    struct {
        RK_U32 qpel_penalty_0 : 8;
        RK_U32 qpel_penalty_1 : 8;
        RK_U32 qpel_penalty_2 : 8;
        RK_U32 qpel_penalty_3 : 8;
    } sw128_159[32];


    struct {
        RK_U32 cost_inter : 12;
        RK_U32 dmv_cost_const : 12;
        RK_U32 : 8;
    } sw160;

    struct {
        RK_U32 cost_golden_ref : 12;
        RK_U32 : 20;
    } sw161;

    struct {
        RK_U32 lf_ref_delta0 : 7; //vp8_loopfilter_intra
        RK_U32 lf_ref_delta1 : 7; //vp8_loopfilter_lastref
        RK_U32 lf_ref_delta2 : 7; //vp8_loopfilter_glodenref
        RK_U32 lf_ref_delta3 : 7; //vp8_loopfilter_alterf
        RK_U32 : 4;
    } sw162;

    struct {
        RK_U32 lf_mode_delta0 : 7; //vp8_loopfilter_bpred
        RK_U32 lf_mode_delta1 : 7; //vp8_loopfilter_zeromv
        RK_U32 lf_mode_delta2 : 7; //vp8_loopfilter_newmv
        RK_U32 lf_mode_delta3 : 7; //vp8_loopfilter_splitmv
        RK_U32 : 4;
    } sw163;

} Vp8eVepu1Reg_t;

#endif /*__HAL_VP8E_VEPU1_REG_H__*/
