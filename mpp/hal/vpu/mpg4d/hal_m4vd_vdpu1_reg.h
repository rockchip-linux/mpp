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

#ifndef __HAL_M4V_VDPU1_REG_TBL_H__
#define __HAL_M4V_VDPU1_REG_TBL_H__

#include "rk_type.h"

/* Number registers for the decoder */
#define DEC_VDPU1_REGISTERS         (101)

typedef struct {
    RK_U32 SwReg00;

    struct {
        RK_U32 sw_dec_en         : 1;
        RK_U32 reserve0          : 3;
        RK_U32 sw_dec_irq_dis    : 1;
        RK_U32 reserve1          : 3;
        RK_U32 sw_dec_irq        : 1;
        RK_U32 reserve2          : 3;
        RK_U32 sw_dec_rdy_int    : 1;
        RK_U32 sw_dec_bus_int    : 1;
        RK_U32 sw_dec_buffer_int : 1;
        RK_U32 sw_dec_aso_int    : 1;
        RK_U32 sw_dec_error_int  : 1;
        RK_U32 sw_dec_slice_int  : 1;
        RK_U32 sw_dec_timeout    : 1;
        RK_U32 reserve3          : 5;
        RK_U32 sw_dec_pic_inf    : 1;
        RK_U32 reserve4          : 7;
    } SwReg01;

    struct {
        RK_U32 sw_dec_max_burst   : 5;
        RK_U32 sw_dec_scmd_dis    : 1;
        RK_U32 sw_dec_adv_pre_dis : 1;
        RK_U32 sw_tiled_mode_lsb  : 1;
        RK_U32 sw_dec_out_endian  : 1;
        RK_U32 sw_dec_in_endian   : 1;
        RK_U32 sw_dec_clk_gate_e  : 1;
        RK_U32 sw_dec_latency     : 6;
        RK_U32 sw_tiled_mode_msb  : 1;
        RK_U32 sw_dec_data_disc_e : 1;
        RK_U32 sw_dec_outswap32_e : 1;
        RK_U32 sw_dec_inswap32_e  : 1;
        RK_U32 sw_dec_strendian_e : 1;
        RK_U32 sw_dec_strswap32_e : 1;
        RK_U32 sw_dec_timeout_e   : 1;
        RK_U32 sw_dec_axi_rd_id   : 8;
    } SwReg02;

    struct {
        RK_U32 sw_dec_axi_wr_id   : 8;
        RK_U32 reserve0           : 1;
        RK_U32 sw_picord_count_e  : 1;
        RK_U32 sw_seq_mbaff_e     : 1;
        RK_U32 sw_reftopfirst_e   : 1;
        RK_U32 sw_write_mvs_e     : 1;
        RK_U32 sw_pic_fixed_quant : 1;
        RK_U32 sw_filtering_dis   : 1;
        RK_U32 sw_dec_out_dis     : 1;
        RK_U32 sw_ref_topfield_e  : 1;
        RK_U32 sw_sorenson_e      : 1;
        RK_U32 sw_fwd_interlace_e : 1;
        RK_U32 sw_pic_topfield_e  : 1;
        RK_U32 sw_pic_inter_e     : 1;
        RK_U32 sw_pic_b_e         : 1;
        RK_U32 sw_pic_fieldmode_e : 1;
        RK_U32 sw_pic_interlace_e : 1;
        RK_U32 sw_pjpeg_e         : 1;
        RK_U32 sw_divx3_e         : 1;
        RK_U32 sw_skip_mode       : 1;
        RK_U32 sw_rlc_mode_e      : 1;
        RK_U32 sw_dec_mode        : 4;
    } SwReg03;

    struct {
        RK_U32  sw_reserve0         : 5;
        RK_U32  sw_topfieldfirst_e  : 1;
        RK_U32  sw_alt_scan_e       : 1;
        RK_U32  sw_mb_height_off    : 4;
        RK_U32  sw_pic_mb_hight_p   : 8;
        RK_U32  sw_mb_width_off     : 4;
        RK_U32  sw_pic_mb_width     : 9;
    } SwReg04;

    struct {
        RK_U32 sw_vop_time_incr    : 16;
        RK_U32 sw_intradc_vlc_thr  : 3;
        RK_U32 sw_ch_qp_offset     : 5;
        RK_U32 sw_type1_quant_e    : 1;
        RK_U32 sw_sync_markers_e   : 1;
        RK_U32 sw_strm_start_bit   : 6;
    } SwReg05;

    struct {
        RK_U32 sw_stream_len      : 24;
        RK_U32 sw_ch_8pix_ileav_e : 1;
        RK_U32 sw_init_qp         : 6;
        RK_U32 sw_start_code_e    : 1;
    } SwReg06;

    struct {
        RK_U32 sw_framenum        : 16;
        RK_U32 sw_framenum_len    : 5;
        RK_U32 reserve0           : 5;
        RK_U32 sw_weight_bipr_idc : 2;
        RK_U32 sw_weight_pred_e   : 1;
        RK_U32 sw_dir_8x8_infer_e : 1;
        RK_U32 sw_blackwhite_e    : 1;
        RK_U32 sw_cabac_e         : 1;
    } SwReg07;

    struct {
        RK_U32 sw_idr_pic_id      : 16;
        RK_U32 sw_idr_pic_e       : 1;
        RK_U32 sw_refpic_mk_len   : 11;
        RK_U32 sw_8x8trans_flag_e : 1;
        RK_U32 sw_rdpic_cnt_pres  : 1;
        RK_U32 sw_filt_ctrl_pres  : 1;
        RK_U32 sw_const_intra_e   : 1;
    } SwReg08;

    struct {
        RK_U32 sw_poc_length      : 8;
        RK_U32 reserve0           : 6;
        RK_U32 sw_refidx0_active  : 5;
        RK_U32 sw_refidx1_active  : 5;
        RK_U32 sw_pps_id          : 8;
    } SwReg09;

    struct {
        RK_U32 sw_diff_mv_base    : 32;
    } SwReg10;

    RK_U32 SwReg11;

    struct {
        RK_U32 sw_rlc_vlc_base    : 32;
    } SwReg12;

    struct {
        RK_U32 dec_out_st_adr     : 32;
    } SwReg13;

    /* MPP passes fd of reference frame to kernel
    * with the whole register rather than higher 30-bit.
    * At the same time, the lower 2-bit will be assigned
    * by kernel.
    * */
    struct {
        //RK_U32 sw_refer0_topc_e    : 1;
        //RK_U32 sw_refer0_field_e   : 1;
        RK_U32 sw_refer0_base        : 32;
    } SwReg14;

    struct {
        //RK_U32 sw_refer1_topc_e    : 1;
        //RK_U32 sw_refer1_field_e   : 1;
        RK_U32 sw_refer1_base        : 32;
    } SwReg15;

    struct {
        //RK_U32 sw_refer2_topc_e    : 1;
        //RK_U32 sw_refer2_field_e   : 1;
        RK_U32 sw_refer2_base        : 32;
    } SwReg16;

    struct {
        //RK_U32 sw_refer3_topc_e    : 1;
        //RK_U32 sw_refer3_field_e   : 1;
        RK_U32 sw_refer3_base        : 32;
    } SwReg17;

    struct {
        RK_U32 sw_prev_anc_type      : 1;
        RK_U32 sw_mpeg4_vc1_rc       : 1;
        RK_U32 sw_mv_accuracy_fwd    : 1;
        RK_U32 sw_fcode_bwd_ver      : 4;
        RK_U32 sw_fcode_bwd_hor      : 4;
        RK_U32 sw_fcode_fwd_ver      : 4;
        RK_U32 sw_fcode_fwd_hor      : 4;
        RK_U32 sw_alt_scan_flag_e    : 1;
        RK_U32 reserve0              : 12;
    } SwReg18;

    struct {
        //RK_U32 sw_refer5_topc_e    : 1;
        //RK_U32 sw_refer5_field_e   : 1;
        RK_U32 sw_refer5_base        : 32;
    } SwReg19;

    struct {
        //RK_U32 sw_refer6_topc_e    : 1;
        //RK_U32 sw_refer6_field_e   : 1;
        RK_U32 sw_refer6_base        : 32;
    } SwReg20;

    struct {
        //RK_U32 sw_refer7_topc_e    : 1;
        //RK_U32 sw_refer7_field_e   : 1;
        RK_U32 sw_refer7_base        : 32;
    } SwReg21;


    RK_U32 SwReg22_33[12];

    struct {
        RK_U32  reserve           : 2;
        RK_U32 sw_pred_bc_tap_1_1 : 10;
        RK_U32 sw_pred_bc_tap_1_0 : 10;
        RK_U32 sw_pred_bc_tap_0_3 : 10;
    } SwReg34;

    RK_U32 SwReg35_39[5];

    struct {
        RK_U32 sw_qtable_base     : 32;
    } SwReg40;

    struct {
        RK_U32 sw_dir_mv_base     : 32;
    } SwReg41;

    RK_U32 SwReg42_47[6];

    struct {
        RK_U32 reserve0           : 15;
        RK_U32 sw_startmb_y       : 8;
        RK_U32 sw_startmb_x       : 9;
    } SwReg48;

    struct {
        RK_U32 reserve0           : 2;
        RK_U32 sw_pred_bc_tap_0_2 : 10;
        RK_U32 sw_pred_bc_tap_0_1 : 10;
        RK_U32 sw_pred_bc_tap_0_0 : 10;
    } SwReg49;

    RK_U32 SwReg50;

    struct {
        RK_U32 sw_refbu_y_offset  : 9;
        RK_U32 reserve0           : 3;
        RK_U32 sw_refbu_fparmod_e : 1;
        RK_U32 sw_refbu_eval_e    : 1;
        RK_U32 sw_refbu_picid     : 5;
        RK_U32 sw_refbu_thr       : 12;
        RK_U32 sw_refbu_e         : 1;
    } SwReg51;

    RK_U32 SwReg52_54[3];

    struct {
        RK_U32 sw_apf_threshold   : 14;
        RK_U32 reserve0           : 18;
    } SwReg55;

    RK_U32 SwReg56_100[46];
} M4vdVdpu1Regs_t;

#endif /*__HAL_M4V_VDPU1_REG_TBL_H__*/
