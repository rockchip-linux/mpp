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

#ifndef __HAL_AVSD_VDPU1_REG_H__
#define __HAL_AVSD_VDPU1_REG_H__


typedef struct {
    RK_U32 sw00;
    struct {
        RK_U32 dec_e : 1;
        RK_U32 reserve0 : 3;
        RK_U32 dec_irq_dis : 1;
        RK_U32 dec_abort_e : 1;
        RK_U32 reserve1 : 2;
        RK_U32 dec_irq : 1;
        RK_U32 reserve2 : 2;
        RK_U32 dec_abort_int : 1;
        RK_U32 dec_rdy_int : 1;
        RK_U32 dec_bus_int : 1;
        RK_U32 dec_buffer_int : 1;
        RK_U32 reserve3 : 1;
        RK_U32 dec_error_int : 1;
        RK_U32 reserve4 : 1;
        RK_U32 dec_timeout : 1;
        RK_U32 reserve5 : 5;
        RK_U32 dec_pic_inf : 1;
        RK_U32 reserve6 : 7;
    } sw01;
    union {
        struct {
            RK_U32 dec_max_burst : 5;
            RK_U32 dec_scmd_dis : 1;
            RK_U32 dec_adv_pre_dis : 1;
            RK_U32 tiled_mode_lsb : 1;
            RK_U32 dec_out_endian : 1;
            RK_U32 dec_in_endian : 1;
            RK_U32 dec_clk_gate_e : 1;
            RK_U32 dec_latency : 6;
            RK_U32 dec_out_tiled_e : 1;
            RK_U32 dec_data_disc_e : 1;
            RK_U32 dec_outswap32_e : 1;
            RK_U32 dec_inswap32_e : 1;
            RK_U32 dec_strendian_e : 1;
            RK_U32 dec_strswap32_e : 1;
            RK_U32 dec_timeout_e : 1;
            RK_U32 dec_axi_rd_id : 8;
        };
        struct {
            RK_U32 reserve0 : 5;
            RK_U32 priority_mode : 3;
            RK_U32 reserve1 : 9;
            RK_U32 tiled_mode_msb : 1;
            RK_U32 dec_2chan_dis : 1;
            RK_U32 reserve2 : 13;
        };
    } sw02;
    struct {
        RK_U32 dec_axi_wr_id : 8;
        RK_U32 dec_ahb_hlock_e : 1;
        RK_U32 picord_count_e : 1;
        RK_U32 reserve0 : 1;
        RK_U32 reftopfirst_e : 1;
        RK_U32 write_mvs_e : 1;
        RK_U32 pic_fixed_quant : 1;
        RK_U32 filtering_dis : 1;
        RK_U32 dec_out_dis : 1;
        RK_U32 ref_topfield_e : 1;
        RK_U32 reserve1 : 1;
        RK_U32 fwd_interlace_e : 1;
        RK_U32 pic_topfiled_e : 1;
        RK_U32 pic_inter_e : 1;
        RK_U32 pic_b_e : 1;
        RK_U32 pic_fieldmode_e : 1;
        RK_U32 pic_interlace_e : 1;
        RK_U32 reserve2 : 2;
        RK_U32 skip_mode : 1;
        RK_U32 rlc_mode_e : 1;
        RK_U32 dec_mode : 4;
    } sw03;
    struct {
        RK_U32 pic_refer_flag : 1;
        RK_U32 reverse0 : 10;
        RK_U32 pic_mb_height_p : 8;
        RK_U32 mb_width_off : 4;
        RK_U32 pic_mb_width : 9;
    } sw04;
    union {
        struct {
            RK_U32 fieldpic_flag_e : 1;
            RK_S32 reserve0 : 31;
        };
        struct {
            RK_U32 beta_offset : 5;
            RK_U32 alpha_offset : 5;
            RK_U32 reserve1 : 16;
            RK_U32 strm_start_bit : 6;
        };
    } sw05;
    struct {
        RK_U32 stream_len : 24;
        RK_U32 stream_len_ext : 1;
        RK_U32 init_qp : 6;
        RK_U32 start_code_e : 1;
    } sw06;
    RK_U32 sw07_11[5];
    struct {
        RK_U32 rlc_vlc_base : 32;
    } sw12;
    struct {
        RK_U32 dec_out_base : 32;
    } sw13;
    union {
        RK_U32 refer0_base : 32;
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer0_topc_e : 1;
            RK_U32 refer0_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw14;
    union {
        struct {
            RK_U32 refer1_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer1_topc_e : 1;
            RK_U32 refer1_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw15;
    union {
        struct {
            RK_U32 refer2_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer2_topc_e : 1;
            RK_U32 refer2_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw16;
    union {
        struct {
            RK_U32 refer3_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer3_topc_e : 1;
            RK_U32 refer3_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw17;
    struct {
        RK_U32 prev_anc_type : 1;
        RK_U32 reverse0 : 31;
    } sw18;
    RK_U32 sw19_27[9];
    struct {
        RK_U32 ref_invd_cur_0 : 16;
        RK_U32 ref_invd_cur_1 : 16;
    } sw28;
    struct {
        RK_U32 ref_invd_cur_2 : 16;
        RK_U32 ref_invd_cur_3 : 16;
    } sw29;
    struct {
        RK_U32 ref_dist_cur_0 : 16;
        RK_U32 ref_dist_cur_1 : 16;
    } sw30;
    struct {
        RK_U32 ref_dist_cur_2 : 16;
        RK_U32 ref_dist_cur_3 : 16;
    } sw31;
    struct {
        RK_U32 ref_invd_col_0 : 16;
        RK_U32 ref_invd_col_1 : 16;
    } sw32;
    struct {
        RK_U32 ref_invd_col_2 : 16;
        RK_U32 ref_invd_col_3 : 16;
    } sw33;
    struct {
        RK_U32 reserve0 : 2;
        RK_U32 pred_bc_tap_1_1 : 10;
        RK_U32 pred_bc_tap_1_0 : 10;
        RK_U32 pred_bc_tap_0_3 : 10;
    } sw34;
    struct {
        RK_U32 reserve0 : 12;
        RK_U32 pred_bc_tap_1_3 : 10;
        RK_U32 pred_bc_tap_1_2 : 10;
    } sw35;
    RK_U32 sw36_40[5];
    struct {
        RK_U32 dir_mv_base : 32;
    } sw41;
    RK_U32 sw42_47[6];
    struct {
        RK_U32 reserve0 : 14;
        RK_U32 startmb_y : 9;
        RK_U32 startmb_x : 9;
    } sw48;
    struct {
        RK_U32 reserve0 : 2;
        RK_U32 pred_bc_tap_0_2 : 10;
        RK_U32 pred_bc_tap_0_1 : 10;
        RK_U32 pred_bc_tap_0_0 : 10;
    } sw49;
    RK_U32 sw50_54[5];
    struct {
        RK_U32 apf_threshold : 14;
        RK_U32 refbu2_picid : 5;
        RK_U32 refbu2_thr : 12;
        RK_U32 refbu2_buf_e : 1;
    } sw55;
    RK_U32 sw56;
    struct {
        RK_U32 stream_len_ext : 1;
        RK_U32 inter_dblspeed : 1;
        RK_U32 intra_dblspeed : 1;
        RK_U32 intra_dbl3t : 1;
        RK_U32 bus_pos_sel : 1;
        RK_U32 axi_sel : 1;
        RK_U32 pref_sigchan : 1;
        RK_U32 cache_en : 1;
        RK_U32 reserve : 24;
    } sw57;
    RK_U32 sw58_59[2];
    RK_U32 resever[40];
} AvsdVdpu1Regs_t;

#endif /*__HAL_AVSD_VDPU1_REG_H__*/
