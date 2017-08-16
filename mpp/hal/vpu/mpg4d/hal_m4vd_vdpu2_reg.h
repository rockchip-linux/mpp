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

#ifndef __HAL_M4V_VDPU2_REG_TBL_H__
#define __HAL_M4V_VDPU2_REG_TBL_H__

#include "rk_type.h"

typedef struct  {
    RK_U32      reg00_49[50];

    struct {
        RK_U32  sw_dec_tiled_msb    : 1;
        RK_U32  sw_dec_latency      : 6;
        RK_U32  sw_pic_fixed_quant  : 1;
        RK_U32  sw_dblk_flt_dis     : 1;
        RK_U32  sw_skip_sel         : 1;
        RK_U32  sw_dec_ascmd0_dis   : 1;
        RK_U32  sw_adv_pref_dis     : 1;
        RK_U32  sw_dec_tiled_lsb    : 1;
        RK_U32  sw_refbuf_thrd      : 12;
        RK_U32  sw_refbuf_pid       : 5;
        RK_U32  reserve1            : 2;
    } reg50_dec_ctrl;

    struct {
        RK_U32  sw_stream_len       : 24;
        RK_U32  reserve1            : 1;
        RK_U32  sw_qp_init_val      : 6;
        RK_U32  reserve2            : 1;
    } reg51_stream_info;

    struct {
        RK_U32  sw_ydim_mbst        : 8;
        RK_U32  sw_xdim_mbst        : 9;
        RK_U32  sw_adv_pref_thrd    : 14;
        RK_U32  sw_reserve          : 1;
    } reg52_error_concealment;

    RK_U32      reg53_dec_mode;

    struct {
        RK_U32  sw_dec_in_endian    : 1;
        RK_U32  sw_dec_out_endian   : 1;
        RK_U32  sw_dec_in_wordsp    : 1;
        RK_U32  sw_dec_out_wordsp   : 1;
        RK_U32  sw_dec_strswap32_e  : 1;
        RK_U32  sw_dec_strendian_e  : 1;
        RK_U32  reserve3            : 26;
    } reg54_endian;

    struct {
        RK_U32  sw_dec_irq          : 1;
        RK_U32  sw_dec_irq_dis      : 1;
        RK_U32  reserve0            : 2;
        RK_U32  sw_dec_rdy_int      : 1;
        RK_U32  sw_pp_bus_sts       : 1;
        RK_U32  sw_buf_emt_sts      : 1;
        RK_U32  reserve1            : 1;
        RK_U32  sw_aso_det_sts      : 1;
        RK_U32  sw_slice_det_sts    : 1;
        RK_U32  sw_bslice_det_sts   : 1;
        RK_U32  reserve2            : 1;
        RK_U32  sw_error_det_sts    : 1;
        RK_U32  sw_timeout_det_sts  : 1;
        RK_U32  reserve3            : 18;
    } reg55_Interrupt;

    struct {
        RK_U32  sw_dec_axi_id_rd    : 8;
        RK_U32  sw_dec_axi_id_wr    : 8;
        RK_U32  sw_dec_max_burlen   : 5;
        RK_U32  reserve0            : 1;
        RK_U32  sw_dec_data_discd_en: 1;
        RK_U32  reserve1            : 9;
    } reg56_axi_ctrl;

    struct {
        RK_U32  sw_dec_st_work         : 1;
        RK_U32  sw_refpic_buf2_en      : 1;
        RK_U32  sw_dec_wr_extmem_dis   : 1;
        RK_U32  reserve                : 1;
        RK_U32  sw_dec_clkgate_en      : 1;
        RK_U32  sw_timeout_sts_en      : 1;
        RK_U32  sw_rd_cnt_tab_en       : 1;
        RK_U32  sw_sequ_mbaff_en       : 1;
        RK_U32  sw_first_reftop_en     : 1;
        RK_U32  sw_reftop_en           : 1;
        RK_U32  sw_dmmv_wr_en          : 1;
        RK_U32  sw_sorspa_en           : 1;
        RK_U32  sw_fwd_refpic_mode_sel : 1;
        RK_U32  sw_pic_decfield_sel    : 1;
        RK_U32  sw_pic_type_sel0       : 1;
        RK_U32  sw_pic_type_sel1       : 1;
        RK_U32  sw_curpic_stru_sel     : 1;
        RK_U32  sw_curpic_code_sel     : 1;
        RK_U32  sw_prog_jpeg_en        : 1;
        RK_U32  sw_divx3_en            : 1;
        RK_U32  sw_rlc_mode_en         : 1;
        RK_U32  sw_addit_ch_fmt_wen    : 1;
        RK_U32  sw_st_code_exist       : 1;
        RK_U32  reserve1               : 8;
        RK_U32  sw_dec_timeout_mode    : 1;
    } reg57_enable_ctrl;

    struct {
        RK_U32  sw_soft_rst         : 1;
        RK_U32  reverse0            : 31;
    } reg58;

    struct {
        RK_U32  reserve             : 2;
        RK_U32  sw_pflt_set0_tap2   : 10;
        RK_U32  sw_pflt_set0_tap1   : 10;
        RK_U32  sw_pflt_set0_tap0   : 10;
    } reg59;

    RK_U32      reg60_addit_ch_st_base;
    RK_U32      reg61_qtable_base;
    RK_U32      reg62_directmv_base;
    RK_U32      reg63_cur_pic_base;
    RK_U32      reg64_input_stream_base;

    struct {
        RK_U32  sw_refbuf_y_ofset        : 9;
        RK_U32  sw_reserve               : 3;
        RK_U32  sw_refbuf_fildpar_mod_e  : 1;
        RK_U32  sw_refbuf_idcal_e        : 1;
        RK_U32  sw_refbuf_picid          : 5;
        RK_U32  sw_refbu_thr_level       : 12;
        RK_U32  sw_refbu_e               : 1;
    } reg65_refpicbuf_ctrl;

    struct {
        RK_U32  build_ver           : 3;
        RK_U32  ascii_id_en         : 1;
        RK_U32  minor_num           : 8;
        RK_U32  major_num           : 4;
        RK_U32  prod_id             : 16;
    } reg66_id;

    struct {
        RK_U32  sw_reserve          : 25;
        RK_U32  rom_imp_type        : 1;
        RK_U32  rv_allow_flag       : 2;
        RK_U32  refbuf2_allow_flag  : 1;
        RK_U32  divx_allow_flag     : 1;
        RK_U32  refbuf_allow_flag   : 1;
        RK_U32  jpeg_allow_flag     : 1;
    } reg67_synthesis_cfg;

    struct {
        RK_U32  sw_refbuf_sum_bot    : 16;
        RK_U32  sw_refbuf_sum_top    : 16;
    } reg68_sum_of_partitions;

    struct {
        RK_U32  sw_luma_sum_intra    : 16;
        RK_U32  sw_refbuf_sum_hit    : 16;
    } reg69_sum_inf;

    struct {
        RK_U32  sw_ycomp_mv_sum     : 22;
        RK_U32  sw_reserve          : 10;
    } reg70_sum_mv;

    RK_U32      reg71_119_reserve[49];

    struct {
        RK_U32  sw_reserve0         : 5;
        RK_U32  sw_topfieldfirst_e  : 1;
        RK_U32  sw_alt_scan_e       : 1;
        RK_U32  sw_mb_height_off    : 4;
        RK_U32  sw_pic_mb_hight_p   : 8;
        RK_U32  sw_mb_width_off     : 4;
        RK_U32  sw_pic_mb_width     : 9;
    } reg120;

    struct {
        RK_U32  sw_reserve          : 5;
        RK_U32  sw_vp7_version      : 1;
        RK_U32  sw_dc_match0        : 3;
        RK_U32  sw_dc_match1        : 3;
        RK_U32  sw_eable_bilinear   : 1;
        RK_U32  sw_remain_mv        : 1;
        RK_U32  sw_reserve1         : 6;
        RK_U32  sw_dct2_start_bit   : 6;
        RK_U32  sw_dct1_start_bit   : 6;
    } reg121;

    struct {
        RK_U32  sw_vop_time_incr    : 16;
        RK_U32  sw_intradc_vlc_thr  : 3;
        RK_U32  sw_ch_qp_offset     : 5;
        RK_U32  sw_quant_type_1_en  : 1;
        RK_U32  sw_sync_markers_en  : 1;
        RK_U32  sw_stream_start_word: 6;
    } reg122;

    struct {
        RK_U32  sw_dc_comp1         : 16;
        RK_U32  sw_dc_comp0         : 16;
    } reg123;

    struct {
        RK_U32  sw_stream1_len      : 24;
        RK_U32  sw_coeffs_part_am   : 4;
        RK_U32  sw_reserve          : 4;
    } reg124;

    struct {
        RK_U32  reserve             : 2;
        RK_U32  sw_pred_bc_tap_5_3  : 10;
        RK_U32  sw_pred_bc_tap_5_2  : 10;
        RK_U32  sw_pred_bc_tap_5_1  : 10;
    } reg125;

    struct {
        RK_U32  reserve             : 2;
        RK_U32  sw_pred_bc_tap_6_2  : 10;
        RK_U32  sw_pred_bc_tap_6_1  : 10;
        RK_U32  sw_pred_bc_tap_6_0  : 10;
    } reg126;

    struct {
        RK_U32  reserve             : 2;
        RK_U32  sw_pred_bc_tap_7_1  : 10;
        RK_U32  sw_pred_bc_tap_7_0  : 10;
        RK_U32  sw_pred_bc_tap_6_3  : 10;
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
        RK_U32  sw_filt_level_3     : 6;
        RK_U32  sw_filt_level_2     : 6;
        RK_U32  sw_filt_level_1     : 6;
        RK_U32  sw_filt_level_0     : 6;
        RK_U32  reserve             : 8;
    } reg129;

    struct {
        RK_U32  sw_quant_1          : 11;
        RK_U32  sw_quant_0          : 11;
        RK_U32  sw_quant_delta_1    : 5;
        RK_U32  sw_quant_delta_0    : 5;
    } reg130;


    RK_U32 reg131_ref0_base;

    struct {
        RK_U32  sw_filt_mb_adj_3    : 7;
        RK_U32  sw_filt_mb_adj_2    : 7;
        RK_U32  sw_filt_mb_adj_1    : 7;
        RK_U32  sw_filt_mb_adj_0    : 7;
        RK_U32  sw_filt_sharpness   : 3;
        RK_U32  sw_filt_type        : 1;
    } reg132;


    struct {
        RK_U32  sw_filt_ref_adj_3   : 7;
        RK_U32  sw_filt_ref_adj_2   : 7;
        RK_U32  sw_filt_ref_adj_1   : 7;
        RK_U32  sw_filt_ref_adj_0   : 7;
        RK_U32  sw_reserve          : 4;
    } reg133;

    RK_U32 reg134_ref2_base;
    RK_U32 reg135_ref3_base;

    struct {
        RK_U32  sw_prev_pic_type        : 1;
        RK_U32  sw_rounding             : 1;
        RK_U32  sw_fwd_mv_y_resolution  : 1;
        RK_U32  sw_vrz_bit_of_bwd_mv    : 4;
        RK_U32  sw_hrz_bit_of_bwd_mv    : 4;
        RK_U32  sw_vrz_bit_of_fwd_mv    : 4;
        RK_U32  sw_hrz_bit_of_fwd_mv    : 4;
        RK_U32  sw_alt_scan_flag_e      : 1;
        RK_U32  sw_reserve              : 12;
    } reg136;

    struct {
        RK_U32 sw_trb_per_trd_d0        : 27;
        RK_U32 sw_reserve               : 5;
    } reg137;

    struct {
        RK_U32 sw_trb_per_trd_dm1       : 27;
        RK_U32 sw_reserve               : 5;
    } reg138;

    struct {
        RK_U32 sw_trb_per_trd_d1        : 27;
        RK_U32 sw_reserve               : 5;
    } reg139;

    RK_U32 reg_dct_strm_base[5];
    RK_U32 reg145_bitpl_ctrl_base;
    RK_U32 reg_dct_strm1_base[2];

    RK_U32 reg148_ref1_base;

    RK_U32 reg149_segment_map_base;


    struct {
        RK_U32  sw_dct_start_bit_7   : 6;
        RK_U32  sw_dct_start_bit_6   : 6;
        RK_U32  sw_dct_start_bit_5   : 6;
        RK_U32  sw_dct_start_bit_4   : 6;
        RK_U32  sw_dct_start_bit_3   : 6;
        RK_U32  sw_reserve           : 2;
    } reg150;

    struct {
        RK_U32  sw_quant_3           : 11;
        RK_U32  sw_quant_2           : 11;
        RK_U32  sw_quant_delta_3     : 5;
        RK_U32  sw_quant_delta_2     : 5;
    } reg151;

    struct {
        RK_U32  sw_quant_5           : 11;
        RK_U32  sw_quant_4           : 11;
        RK_U32  sw_quant_delta_4     : 5;
        RK_U32  sw_reserve           : 5;
    } reg152;

    struct {
        RK_U32  reserve              : 2;
        RK_U32  sw_pred_bc_tap_1_1   : 10;
        RK_U32  sw_pred_bc_tap_1_0   : 10;
        RK_U32  sw_pred_bc_tap_0_3   : 10;
    } reg153;

    struct {
        RK_U32  reserve              : 2;
        RK_U32  sw_pred_bc_tap_2_0   : 10;
        RK_U32  sw_pred_bc_tap_1_3   : 10;
        RK_U32  sw_pred_bc_tap_1_2   : 10;
    } reg154;

    struct {
        RK_U32  reserve              : 2;
        RK_U32  sw_pred_bc_tap_2_3   : 10;
        RK_U32  sw_pred_bc_tap_2_2   : 10;
        RK_U32  sw_pred_bc_tap_2_1   : 10;
    } reg155;

    struct {
        RK_U32  reserve              : 2;
        RK_U32  sw_pred_bc_tap_3_2   : 10;
        RK_U32  sw_pred_bc_tap_3_1   : 10;
        RK_U32  sw_pred_bc_tap_3_0   : 10;
    } reg156;

    struct {
        RK_U32  reserve              : 2;
        RK_U32  sw_pred_bc_tap_4_1   : 10;
        RK_U32  sw_pred_bc_tap_4_0   : 10;
        RK_U32  sw_pred_bc_tap_3_3   : 10;
    } reg157;

    struct {
        RK_U32  reserve              : 2;
        RK_U32  sw_pred_bc_tap_5_0   : 10;
        RK_U32  sw_pred_bc_tap_4_3   : 10;
        RK_U32  sw_pred_bc_tap_4_2   : 10;
    } reg158;
} M4vdVdpu2Regs_t;

#endif /*__HAL_M4V_VDPU2_REG_TBL_H__*/
