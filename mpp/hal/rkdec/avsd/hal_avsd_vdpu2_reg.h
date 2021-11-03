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

#ifndef __HAL_AVSD_VDPU2_REG_H__
#define __HAL_AVSD_VDPU2_REG_H__


typedef struct {
    RK_U32 sw00_49[50];
    struct {
        RK_U32 dec_tiled_msb : 1;
        RK_U32 adtion_latency : 6;
        RK_U32 dec_fixed_quant : 1;
        RK_U32 filtering_dis : 1;
        RK_U32 skip_mode : 1;
        RK_U32 dec_ascmd0_dis : 1;
        RK_U32 adv_pref_dis : 1;
        RK_U32 dec_tiled_lsb : 1;
        RK_U32 refbuf_thrd : 12;
        RK_U32 refbuf_pid : 5;
        RK_U32 reverse0 : 2;
    } sw50;
    struct {
        RK_U32 stream_len : 24;
        RK_U32 stream_len_ext : 1;
        RK_U32 init_qp : 6;
        RK_U32 reverse0 : 1;
    } sw51;
    struct {
        RK_U32 startmb_y : 8;
        RK_U32 startmb_x : 9;
        RK_U32 adv_pref_thrd : 14;
        RK_U32 reverse0 : 1;
    } sw52;
    struct {
        RK_U32 dec_fmt_sel : 4;
        RK_U32 reverse0 : 28;
    } sw53;
    struct {
        RK_U32 dec_in_endian : 1;
        RK_U32 dec_out_endian : 1;
        RK_U32 dec_in_wordsp : 1;
        RK_U32 dec_out_wordsp : 1;
        RK_U32 dec_strm_wordsp : 1;
        RK_U32 dec_strendian_e : 1;
        RK_U32 reverse0 : 26;
    } sw54;
    struct {
        RK_U32 dec_irq : 1;
        RK_U32 dec_irq_dis : 1;
        RK_U32 reverse0 : 2;
        RK_U32 dec_rdy_sts : 1;
        RK_U32 pp_bus_sts : 1;
        RK_U32 buf_emt_sts : 1;
        RK_U32 reverse1 : 1;
        RK_U32 aso_det_sts : 1;
        RK_U32 slice_det_sts : 1;
        RK_U32 bslice_det_sts : 1;
        RK_U32 reverse2 : 1;
        RK_U32 error_det_sts : 1;
        RK_U32 timeout_det_sts : 1;
        RK_U32 reverse3 : 18;
    } sw55;
    struct {
        RK_U32 dec_axi_id_rd : 8;
        RK_U32 dec_axi_id_wr : 8;
        RK_U32 dec_max_burlen : 5;
        RK_U32 bus_pos_sel : 1;
        RK_U32 dec_data_discd_en : 1;
        RK_U32 axi_sel : 1;
        RK_U32 reverse0 : 8;
    } sw56;
    struct {
        RK_U32 dec_e : 1;
        RK_U32 refpic_buf2_en : 1;
        RK_U32 dec_out_dis : 1;
        RK_U32 reverse0 : 1;
        RK_U32 dec_clkgate_en : 1;
        RK_U32 timeout_sts_en : 1;
        RK_U32 rd_cnt_tab_en : 1;
        RK_U32 reverse1 : 1;
        RK_U32 first_reftop_en : 1;
        RK_U32 reftop_en : 1;
        RK_U32 dmmv_wr_en : 1;
        RK_U32 reverse2 : 1;
        RK_U32 fwd_interlace_e : 1;
        RK_U32 pic_topfield_e : 1;
        RK_U32 pic_inter_e : 1;
        RK_U32 pic_b_e : 1;
        RK_U32 pic_fieldmode_e : 1;
        RK_U32 pic_interlace_e : 1;
        RK_U32 reverse3 : 2;
        RK_U32 rlc_mode_en : 1;
        RK_U32 addit_ch_fmt_wen : 1;
        RK_U32 st_code_exit : 1;
        RK_U32 reverse4 : 2;
        RK_U32 inter_dblspeed : 1;
        RK_U32 intra_dblspeed : 1;
        RK_U32 intra_dbl3t : 1;
        RK_U32 pref_sigchan : 1;
        RK_U32 cache_en : 1;
        RK_U32 reverse5 : 1;
        RK_U32 dec_timeout_mode : 1;
    } sw57;
    RK_U32 sw58;
    struct {
        RK_U32 reserve : 2;
        RK_S32 pred_bc_tap_0_2 : 10;
        RK_S32 pred_bc_tap_0_1 : 10;
        RK_S32 pred_bc_tap_0_0 : 10;
    } sw59;
    RK_U32 sw60;
    RK_U32 sw61;
    struct {
        RK_U32 dmmv_st_adr : 32;
    } sw62;
    struct {
        RK_U32 dec_out_st_adr : 32;
    } sw63;
    struct {
        RK_U32 rlc_vlc_st_adr : 32;
    } sw64;
    RK_U32 sw65_119[55];
    struct {
        RK_U32 pic_refer_flag : 1;
        RK_U32 reserver0 : 6;
        RK_U32 mb_height_off : 4;
        RK_U32 pic_mb_height_p : 8;
        RK_U32 mb_width_off : 4;
        RK_U32 pic_mb_width : 9;
    } sw120;
    struct {
        RK_U32 reserve0 : 25;
        RK_U32 avs_h_ext : 1;
        RK_U32 reserve1 : 6;
    } sw121;
    struct {
        RK_U32 beta_offset : 5;
        RK_U32 alpha_offset : 5;
        RK_U32 reserver0 : 16;
        RK_U32 strm_start_bit : 6;
    } sw122;
    RK_U32 sw123_128[6];
    struct {
        RK_U32 ref_invd_col_0 : 16;
        RK_U32 ref_invd_col_1 : 16;
    } sw129;
    struct {
        RK_U32 ref_invd_col_2 : 16;
        RK_U32 ref_invd_col_3 : 16;
    } sw130;
    union {
        RK_U32 refer0_base : 32;
        struct { //!< left move 10bit
            RK_U32 refer0_topc_e  : 1;
            RK_U32 refer0_field_e : 1;
        };
    } sw131;
    struct {
        RK_U32 ref_dist_cur_0 : 16;
        RK_U32 ref_dist_cur_1 : 16;
    } sw132;
    struct {
        RK_U32 ref_dist_cur_2 : 16;
        RK_U32 ref_dist_cur_3 : 16;
    } sw133;
    union {
        RK_U32 refer2_base : 32;
        struct { //!< left move 10bit
            RK_U32 refer2_topc_e  : 1;
            RK_U32 refer2_field_e : 1;
        };
    } sw134;
    union {
        RK_U32 refer3_base : 32;
        struct { //!< left move 10bit
            RK_U32 refer3_topc_e  : 1;
            RK_U32 refer3_field_e : 1;
        };
    } sw135;
    struct {
        RK_U32 prev_anc_type : 1;
        RK_U32 reserver0 : 31;
    } sw136;
    RK_U32 sw137_145[9];
    struct {
        RK_U32 ref_invd_cur_0 : 16;
        RK_U32 ref_invd_cur_1 : 16;
    } sw146;
    struct {
        RK_U32 ref_invd_cur_2 : 16;
        RK_U32 ref_invd_cur_3 : 16;
    } sw147;
    union {
        RK_U32 refer1_base : 32;
        struct { //!< left move 10bit
            RK_U32 refer1_topc_e  : 1;
            RK_U32 refer1_field_e : 1;
        };
    } sw148;
    RK_U32 sw149_152[4];
    struct {
        RK_U32  reserve : 2;
        RK_U32  pred_bc_tap_1_1 : 10;
        RK_U32  pred_bc_tap_1_0 : 10;
        RK_U32  pred_bc_tap_0_3 : 10;
    } sw153;
    struct {
        RK_U32  reserve : 2;
        RK_U32  pred_bc_tap_2_0 : 10;
        RK_U32  pred_bc_tap_1_3 : 10;
        RK_U32  pred_bc_tap_1_2 : 10;
    } sw154;
    RK_U32 sw155_158[4];
} AvsdVdpu2Regs_t;

#endif /*__HAL_AVSD_VDPU2_REG_H__*/
