/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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
#ifndef __HAL_JPEGD_RKV_REG_H__
#define __HAL_JPEGD_RKV_REG_H__

#define JPEGD_REG_NUM   (42)

#define RKV_JPEGD_LITTLE_ENDIAN (0)
#define RKV_JPEGD_BIG_ENDIAN    (1)

#define SCALEDOWN_DISABLE       (0)
#define SCALEDOWN_HALF          (1)
#define SCALEDOWN_QUARTER       (2)
#define SCALEDOWN_ONE_EIGHTS    (3)

#define OUTPUT_RASTER           (0)
#define OUTPUT_TILE             (1)

#define TIMEOUT_MODE_CYCLE_24   (0)
#define TIMEOUT_MODE_CYCLE_18   (1)

#define OUT_SEQUENCE_RASTER     (0)
#define OUT_SEQUENCE_TILE       (1)

#define YUV_TO_RGB_REC_BT601    (0)
#define YUV_TO_RGB_REC_BT709    (1)

#define YUV_TO_RGB_FULL_RANGE   (1)
#define YUV_TO_RGB_LIMIT_RANGE  (0)

#define YUV_OUT_FMT_NO_TRANS    (0)
#define YUV_OUT_FMT_2_RGB888    (1)
#define YUV_OUT_FMT_2_RGB565    (2)
// Not support YUV400 transmit to NV12
#define YUV_OUT_FMT_2_NV12      (3)
// Only support YUV422 or YUV444, YUV444 should scaledown uv
#define YUV_OUT_FMT_2_YUYV      (4)

#define YUV_MODE_400            (0)
#define YUV_MODE_411            (1)
#define YUV_MODE_420            (2)
#define YUV_MODE_422            (3)
#define YUV_MODE_440            (4)
#define YUV_MODE_444            (5)

#define BIT_DEPTH_8             (0)
#define BIT_DEPTH_12            (1)

// No quantization/huffman table or table is the same as previous
#define TBL_ENTRY_0             (0)
// Grayscale picture with only 1 quantization/huffman table
#define TBL_ENTRY_1             (1)
// Common case, one table for luma, one for chroma
#define TBL_ENTRY_2             (2)
// 3 table entries, one for luma, one for cb, one for cr
#define TBL_ENTRY_3             (3)

// Restart interval marker disable
#define RST_DISABLE             (0)
// Restart interval marker enable
#define RST_ENABLE              (1)

typedef struct {
    struct {
        RK_U32 minor_ver                        : 8;
        RK_U32 bit_depth                        : 1;
        RK_U32                                  : 7;
        RK_U32 prod_num                         : 16;
    } reg0_id;

    struct {
        RK_U32 dec_e                            : 1;
        RK_U32 dec_irq_dis                      : 1;
        RK_U32 dec_timeout_e                    : 1;
        RK_U32 buf_empty_e                      : 1;
        RK_U32 buf_empty_reload_p               : 1;
        RK_U32 soft_rst_en_p                    : 1;
        RK_U32 dec_irq_raw                      : 1;
        RK_U32 wait_reset_e                     : 1;
        RK_U32 dec_irq                          : 1;
        RK_U32 dec_rdy_sta                      : 1;
        RK_U32 dec_bus_sta                      : 1;
        RK_U32 dec_error_sta                    : 1;
        RK_U32 dec_timeout_sta                  : 1;
        RK_U32 dec_buf_empty_sta                : 1;
        RK_U32 soft_rest_rdy                    : 1;
        RK_U32 buf_empty_force_end_flag         : 1;
        RK_U32 care_strm_error_e                : 1;
        RK_U32                                  : 15;
    } reg1_int;

    struct {
        RK_U32 in_endian                        : 1;
        RK_U32 in_swap32_e                      : 1;
        RK_U32 in_swap64_e                      : 1;
        RK_U32 str_endian                       : 1;
        RK_U32 str_swap32_e                     : 1;
        RK_U32 str_swap64_e                     : 1;
        RK_U32 out_endian                       : 1;
        RK_U32 out_swap32_e                     : 1;
        RK_U32 out_swap64_e                     : 1;
        RK_U32 out_cbcr_swap                    : 1;
        RK_U32                                  : 2;
        RK_U32 scaledown_mode                   : 2;
        RK_U32                                  : 2;
        RK_U32 time_out_mode                    : 1;
        RK_U32 force_softrest_valid             : 1;
        RK_U32                                  : 2;
        RK_U32 fbc_e                            : 1;
        RK_U32 allow_16x8_cp_flag               : 1;
        RK_U32 fbc_force_uncompress             : 1;
        RK_U32                                  : 1;
        RK_U32 fill_down_e                      : 1;
        RK_U32 fill_right_e                     : 1;
        RK_U32 dec_out_sequence                 : 1;
        RK_U32 yuv_out_format                   : 3;
        RK_U32 yuv2rgb_rec                      : 1;
        RK_U32 yuv2rgb_range                    : 1;

    } reg2_sys;

    struct {
        RK_U32 pic_width_m1                     : 16;
        RK_U32 pic_height_m1                    : 16;
    } reg3_pic_size;

    struct {
        RK_U32 jpeg_mode                        : 3;
        RK_U32                                  : 1;
        RK_U32 pixel_depth                      : 3;
        RK_U32                                  : 1;
        RK_U32 qtables_sel                      : 2;
        RK_U32                                  : 2;
        RK_U32 htables_sel                      : 2;
        RK_U32                                  : 1;
        RK_U32 dri_e                            : 1;
        RK_U32 dri_mcu_num_m1                   : 16;
    } reg4_pic_fmt;

    struct {
        RK_U32 y_hor_virstride                  : 16;
        RK_U32 uv_hor_virstride                 : 16;
    } reg5_hor_virstride;

    struct {
        RK_U32                                  : 4;
        RK_U32 y_virstride                      : 28;
    } reg6_y_virstride;

    struct {
        RK_U32 qtbl_len                         : 5;
        RK_U32                                  : 3;
        RK_U32 htbl_mincode_len                 : 5;
        RK_U32                                  : 3;
        RK_U32 htbl_value_len                   : 6;
        RK_U32                                  : 2;
        RK_U32 y_hor_virstride_h                : 1;
        RK_U32                                  : 7;
    } reg7_tbl_len;

    struct {
        RK_U32 strm_start_byte                  : 4;
        RK_U32 stream_len                       : 28;
    } reg8_strm_len;

    RK_U32 reg9_qtbl_base;   //64 bytes align

    RK_U32 reg10_htbl_mincode_base;  //64 bytes align

    RK_U32 reg11_htbl_value_base;    //64 bytes align

    RK_U32 reg12_strm_base;  //16 bytes align

    RK_U32 reg13_dec_out_base;   //64 bytes align

    struct {
        RK_U32 error_prc_mode                   : 1;
        RK_U32 strm_r0_err_mode                 : 2;
        RK_U32 strm_r1_err_mode                 : 2;
        RK_U32 strm_ffff_err_mode               : 2;//default skip 0xffff
        RK_U32 strm_other_mark_mode             : 2;
        RK_U32 strm_dri_seq_err_mode            : 1;
        RK_U32                                  : 6;
        RK_U32 hfm_force_stop                   : 5;
        RK_U32                                  : 11;
    } reg14_strm_error;

    struct {
        RK_U32 strm_r0_marker                   : 8;
        RK_U32 strm_r0_mask                     : 8;
        RK_U32 strm_r1_marker                   : 8;
        RK_U32 strm_r1_mask                     : 8;
    } reg15_strm_mask;

    union {
        struct {
            RK_U32  dec_clkgate_e                   : 1;
            RK_U32  dec_strm_gate_e                 : 1;
            RK_U32  dec_huffman_gate_e              : 1;
            RK_U32  dec_izq_gate_e                  : 1;
            RK_U32  dec_idct_gate_e                 : 1;
            RK_U32  busifd_gate_e                   : 1;
            RK_U32  post_prs_get_e                  : 1;
            RK_U32  dec_sram_gate_e                 : 1;
            RK_U32                                  : 24;
        };
        RK_U32 val;
    } reg16_clk_gate;

    RK_U32 reg17_29[13];

    struct {
        RK_U32 axi_per_work_e                   : 1;
        RK_U32 axi_per_clr_e                    : 1;
        RK_U32 axi_perf_frm_tyep                : 1;
        RK_U32 axi_cnt_type                     : 1;
        RK_U32 rd_latency_id                    : 4;
        RK_U32 rd_latency_thr                   : 12;
        RK_U32                                  : 12;
    } reg30_perf_latency_ctrl0;

    struct {
        RK_U32 addr_align_type                  : 2;
        RK_U32 ar_cnt_id_type                   : 1;
        RK_U32 aw_cnt_id_type                   : 1;
        RK_U32 ar_count_id                      : 4;
        RK_U32 aw_count_id                      : 4;
        RK_U32 rd_totoal_bytes_mode             : 1;
        RK_U32                                  : 19;
    } reg31_perf_latency_ctrl1;

    struct {
        RK_U32 mcu_pos_x                        : 16;
        RK_U32 mcu_pos_y                        : 16;
    } reg32_dbg_mcu_pos;

    struct {
        RK_U32 stream_dri_seq_err_sta           : 1;
        RK_U32 stream_r0_err_sta                : 1;
        RK_U32 stream_r1_err_sta                : 1;
        RK_U32 stream_ffff_err_sta              : 1;
        RK_U32 stream_other_mark_err_sta        : 1;
        RK_U32                                  : 3;
        RK_U32 huffman_mcu_cnt_l                : 1;
        RK_U32 huffman_mcu_cnt_m                : 1;
        RK_U32 huffman_eoi_without_end          : 1;
        RK_U32 huffman_end_without_eoi          : 1;
        RK_U32 huffman_overflow                 : 1;
        RK_U32 huffman_buf_empty                : 1;
        RK_U32                                  : 2;
        RK_U32 first_error_idx                  : 4;
        RK_U32                                  : 12;
    } regs33_dbg_error_info;

    struct {
        RK_U32 rd_max_latency_num_ch0           : 16;
        RK_U32                                  : 16;
    } reg34_perf_rd_max_latency_num0;

    RK_U32 reg35_perf_rd_latency_samp_num;

    RK_U32 reg36_perf_rd_latency_acc_sum;

    RK_U32 reg37_perf_rd_axi_total_byte;

    RK_U32 reg38_perf_wr_axi_total_byte;

    RK_U32 reg39_perf_working_cnt;

    struct {
        RK_U32 bus_status_flag                  : 19;
        RK_U32                                  : 13;
    } reg40_dbg_bus_sta;

    struct {
        RK_U32 work_status_flag                 : 18;
        RK_U32                                  : 12;
    } reg41_dbg_work_sta;

} JpegRegSet;

#endif /* __HAL_JPEGD_RKV_REG_H__ */