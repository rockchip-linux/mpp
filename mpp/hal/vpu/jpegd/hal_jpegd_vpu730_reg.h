/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_JPEGD_VPU730_REG_H
#define HAL_JPEGD_VPU730_REG_H

#include "rk_type.h"

#define JPEGD_VPU730_REG_NUM    70

typedef struct {
    struct {
        rk_u32 minor_ver                        : 8;
        rk_u32 bit_depth                        : 1;
        rk_u32                                  : 7;
        rk_u32 prod_num                         : 16;
    } reg0_id;

    struct {
        rk_u32 ltb_cfg_num                      : 8;
        rk_u32 cmd                              : 6;
        rk_u32                                  : 18;
    } reg1_cmd;

    struct {
        rk_u32 in_endian                        : 1;
        rk_u32 in_swap32_e                      : 1;
        rk_u32 in_swap64_e                      : 1;
        rk_u32 str_endian                       : 1;
        rk_u32 str_swap32_e                     : 1;
        rk_u32 str_swap64_e                     : 1;
        rk_u32 out_endian                       : 1;
        rk_u32 out_swap32_e                     : 1;
        rk_u32 out_swap64_e                     : 1;
        rk_u32 out_cbcr_swap                    : 1;
        rk_u32 out_byte_swap                    : 1;
        rk_u32 care_strm_error_e                : 1;
        rk_u32 scaledown_mode                   : 2;
        rk_u32                                  : 4;
        rk_u32 wait_reset_e                     : 1;
        rk_u32 buf_empty_force_end_flag         : 1;
        rk_u32 fbc_e                            : 1;
        rk_u32 allow_16x8_cp_flag               : 1;
        rk_u32 fbc_force_uncompress             : 1;
        rk_u32 bgr_sequence                     : 1;
        rk_u32 fill_down_e                      : 1;
        rk_u32 fill_right_e                     : 1;
        rk_u32 dec_out_sequence                 : 1;
        rk_u32 yuv_out_format                   : 3;
        rk_u32 yuv2rgb_rec                      : 1;
        rk_u32 yuv2rgb_range                    : 1;
    } reg2_sys;

    struct {
        rk_u32 pic_width_m1                     : 16;
        rk_u32 pic_height_m1                    : 16;
    } reg3_pic_size;

    struct {
        rk_u32 jpeg_mode                        : 3;
        rk_u32                                  : 1;
        rk_u32 pixel_depth                      : 3;
        rk_u32                                  : 1;
        rk_u32 qtables_sel                      : 2;
        rk_u32                                  : 2;
        rk_u32 htables_sel                      : 2;
        rk_u32                                  : 1;
        rk_u32 dri_e                            : 1;
        rk_u32 dri_mcu_num_m1                   : 16;
    } reg4_pic_fmt;

    struct {
        rk_u32 y_hor_virstride                  : 16;
        rk_u32 uv_hor_virstride                 : 16;
    } reg5_hor_virstride;

    struct {
        rk_u32                                  : 4;
        rk_u32 y_virstride                      : 28;
    } reg6_y_virstride;

    struct {
        rk_u32 qtbl_len                         : 5;
        rk_u32                                  : 3;
        rk_u32 htbl_mincode_len                 : 5;
        rk_u32                                  : 3;
        rk_u32 htbl_value_len                   : 6;
        rk_u32                                  : 2;
        rk_u32 y_hor_virstride_h                : 1;
        rk_u32                                  : 7;
    } reg7_tbl_len;

    struct {
        rk_u32 strm_start_byte                  : 4;
        rk_u32 stream_len                       : 28;
    } reg8_strm_len;

    rk_u32 reg9_qtbl_base;
    rk_u32 reg10_htbl_mincode_base;
    rk_u32 reg11_htbl_value_base;
    rk_u32 reg12_strm_base;
    rk_u32 reg13_dec_out_base;

    struct {
        rk_u32 error_prc_mode                   : 1;
        rk_u32 strm_r0_err_mode                 : 2;
        rk_u32 strm_r1_err_mode                 : 2;
        rk_u32 strm_ffff_err_mode               : 2;
        rk_u32 strm_other_mark_mode             : 2;
        rk_u32 strm_dri_seq_err_mode            : 1;
        rk_u32                                  : 6;
        rk_u32 hfm_force_stop                   : 5;
        rk_u32                                  : 11;
    } reg14_strm_error;

    struct {
        rk_u32 strm_r0_marker                   : 8;
        rk_u32 strm_r0_mask                     : 8;
        rk_u32 strm_r1_marker                   : 8;
        rk_u32 strm_r1_mask                     : 8;
    } reg15_strm_mask;

    union {
        struct {
            rk_u32 dec_clk_gate_e               : 1;
            rk_u32 dec_strm_gate_e              : 1;
            rk_u32 dec_huffman_gate_e           : 1;
            rk_u32 dec_izq_gate_e               : 1;
            rk_u32 dec_idct_gate_e              : 1;
            rk_u32 busifd_gate_e                : 1;
            rk_u32 post_prs_gate_e              : 1;
            rk_u32 dec_sram_gate_e              : 1;
            rk_u32                              : 24;
        };
        rk_u32 val;
    } reg16_clk_gate;

    struct {
        rk_u32 perf_cnt0_sel                    : 4;
        rk_u32                                  : 4;
        rk_u32 perf_cnt1_sel                    : 4;
        rk_u32                                  : 4;
        rk_u32 perf_cnt2_sel                    : 4;
        rk_u32                                  : 11;
        rk_u32 perf_cnt_e                       : 1;
    } reg17_perf_cnt_sel;

    rk_u32 reg18_perf_cnt0;
    rk_u32 reg19_perf_cnt1;
    rk_u32 reg20_perf_cnt2;

    struct {
        rk_u32 low_delay_output                 : 17;
        rk_u32                                  : 15;
    } reg21_low_delay_output;

    struct {
        rk_u32 watchdog                         : 32;
    } reg22_watchdog;

    rk_u32 reg23_28[6];

    struct {
        rk_u32 st_low_delay_output              : 16;
        rk_u32                                  : 1;
        rk_u32 buwrmaster_frame_rdy             : 1;
        rk_u32                                  : 14;
    } reg29_st_low_delay_output;

    struct {
        rk_u32 axi_perf_work_e                  : 1;
        rk_u32 axi_perf_clr_e                   : 1;
        rk_u32 axi_frm_type                     : 1;
        rk_u32 axi_cnt_type                     : 1;
        rk_u32 rd_latency_id                    : 4;
        rk_u32 rd_latency_thr                   : 12;
        rk_u32                                  : 12;
    } reg30_perf_latency_ctrl0;

    struct {
        rk_u32 addr_align_type                  : 2;
        rk_u32 ar_cnt_id_type                   : 1;
        rk_u32 aw_cnt_id_type                   : 1;
        rk_u32 ar_count_id                      : 4;
        rk_u32 aw_count_id                      : 4;
        rk_u32 rd_band_width_mode               : 1;
        rk_u32                                  : 19;
    } reg30_perf_latency_ctrl1;

    struct {
        rk_u32 mcu_pos_x                        : 16;
        rk_u32 mcu_pos_y                        : 16;
    } reg32_dbg_mcu_pos;

    struct {
        rk_u32 stream_dri_seq_err_sta           : 1;
        rk_u32 stream_r0_err_sta                : 1;
        rk_u32 stream_r1_err_sta                : 1;
        rk_u32 stream_ffff_err_sta              : 1;
        rk_u32 stream_other_mark_err_sta        : 1;
        rk_u32                                  : 3;
        rk_u32 huffman_mcu_cnt_l                : 1;
        rk_u32 huffman_mcu_cnt_m                : 1;
        rk_u32 huffman_eoi_without_end          : 1;
        rk_u32 huffman_end_without_eoi          : 1;
        rk_u32 huffman_overflow                 : 1;
        rk_u32 huffman_buf_empty                : 1;
        rk_u32                                  : 2;
        rk_u32 first_error_idx                  : 4;
        rk_u32                                  : 12;
    } reg33_dbg_error_info;

    struct {
        RK_U32 rd_max_latency_num_ch0           : 16;
        RK_U32                                  : 16;
    } reg34_perf_rd_max_latency_num0;

    rk_u32 reg35_perf_rd_latency_samp_num;
    rk_u32 reg36_perf_rd_latency_acc_sum;
    rk_u32 reg37_perf_rd_axi_total_byte;
    rk_u32 reg38_perf_wr_axi_total_byte;
    rk_u32 reg39_perf_working_cnt;

    struct {
        rk_u32 bus_status_flag                  : 19;
        rk_u32                                  : 13;
    } reg40_dbg_bus_sta;

    struct {
        rk_u32 work_status_flag                 : 18;
        rk_u32                                  : 14;
    } reg41_dbg_work_sta;

    struct {
        rk_u32 dec_state                        : 8;
        rk_u32 ltb_mode                         : 14;
    } reg42_dbg_sta;

    struct {
        rk_u32 ltb_total_num                    : 8;
        rk_u32 ltb_decoded_num                  : 8;
        rk_u32 ltb_done_int_num                 : 8;
        rk_u32 ltb_loaded_num                   : 8;
    } reg43_st_link_num;

    struct {
        rk_u32 ltb_core_id                      : 4;
        rk_u32 ltb_stop_flag                    : 1;
        rk_u32 ltb_node_int                     : 1;
        rk_u32 ltb_task_id                      : 12;
    } reg44_st_link_info;

    struct {
        rk_u32 ltb_rd_addr_next                 : 32;
    } reg45_st_link_rd_addr_next;

    struct {
        rk_u32 ltb_wr_addr_next                 : 32;
    } reg46_st_link_wr_addr;

    rk_u32 reg47_63[17];

    struct {
        rk_u32 ltb_start_addr                   : 32;
    } reg64_ltb_start_addr;

    struct {
        rk_u32 core_id                          : 4;
        rk_u32 ltb_task_error_stop_int_en       : 1;
        rk_u32 ltb_task_node_stop_int_en        : 1;
    } reg65_link_mode;

    struct {
        rk_u32 dec_done_int_en                  : 1;
        rk_u32 safe_reset_int_en                : 1;
        rk_u32 bus_error_int_en                 : 1;
        rk_u32 dec_error_int_en                 : 1;
        rk_u32 timeout_int_en                   : 1;
        rk_u32 buffer_empty_int_en              : 1;
        rk_u32 low_delay_out_int_en             : 1;
        rk_u32 ltb_operation_error_int_en       : 1;
        rk_u32 ltb_node_done_int_en             : 1;
        rk_u32 ltb_force_stop_int_en            : 1;
        rk_u32 ltb_node_stop_int_en             : 1;
        rk_u32 ltb_error_stop_int_en            : 1;
        rk_u32 ltb_data_error_int_en            : 1;
        rk_u32                                  : 19;
    } reg66_int_en;

    struct {
        rk_u32 dec_done_int_mask                : 1;
        rk_u32 safe_reset_int_mask              : 1;
        rk_u32 bus_error_int_mask               : 1;
        rk_u32 dec_error_int_mask               : 1;
        rk_u32 timeout_int_mask                 : 1;
        rk_u32 buffer_empty_int_mask            : 1;
        rk_u32 low_delay_out_int_mask           : 1;
        rk_u32 ltb_operation_error_int_mask     : 1;
        rk_u32 ltb_node_done_int_mask           : 1;
        rk_u32 ltb_force_stop_int_mask          : 1;
        rk_u32 ltb_node_stop_int_mask           : 1;
        rk_u32 ltb_error_stop_int_mask          : 1;
        rk_u32 ltb_data_error_int_mask          : 1;
        rk_u32                                  : 19;
    } reg67_int_mask;

    struct {
        rk_u32 dec_done_int_clear               : 1;
        rk_u32 safe_reset_int_clear             : 1;
        rk_u32 bus_error_int_clear              : 1;
        rk_u32 dec_error_int_clear              : 1;
        rk_u32 timeout_int_clear                : 1;
        rk_u32 buffer_empty_int_clear           : 1;
        rk_u32 low_delay_out_int_clear          : 1;
        rk_u32 ltb_operation_error_int_clear    : 1;
        rk_u32 ltb_node_done_int_clear          : 1;
        rk_u32 ltb_force_stop_int_clear         : 1;
        rk_u32 ltb_node_stop_int_clear          : 1;
        rk_u32 ltb_error_stop_int_clear         : 1;
        rk_u32 ltb_data_error_int_clear         : 1;
        rk_u32                                  : 19;
    } reg68_int_clear;

    struct {
        rk_u32 dec_done_int_sta                 : 1;
        rk_u32 safe_reset_int_sta               : 1;
        rk_u32 bus_error_int_sta                : 1;
        rk_u32 dec_error_int_sta                : 1;
        rk_u32 timeout_int_sta                  : 1;
        rk_u32 buffer_empty_int_sta             : 1;
        rk_u32 low_delay_out_int_sta            : 1;
        rk_u32 ltb_operation_error_int_sta      : 1;
        rk_u32 ltb_node_done_int_sta            : 1;
        rk_u32 ltb_force_stop_int_sta           : 1;
        rk_u32 ltb_node_stop_int_sta            : 1;
        rk_u32 ltb_error_stop_int_sta           : 1;
        rk_u32 ltb_data_error_int_sta           : 1;
        rk_u32                                  : 19;
    } reg69_int_sta;
} JpegdVpu730Reg;

#endif /* HAL_JPEGD_VPU730_REG_H */