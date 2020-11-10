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

#ifndef __HAL_H264D_VDPU34X_REG_H__
#define __HAL_H264D_VDPU34X_REG_H__

#include "rk_type.h"

/* common regs */
typedef struct Vdpu34xRegCommon_t {
    struct SWREG8_IN_OUT {
        RK_U32      in_endian               : 1;
        RK_U32      in_swap32_e             : 1;
        RK_U32      in_swap64_e             : 1;
        RK_U32      str_endian              : 1;
        RK_U32      str_swap32_e            : 1;
        RK_U32      str_swap64_e            : 1;
        RK_U32      out_endian              : 1;
        RK_U32      out_swap32_e            : 1;
        RK_U32      out_cbcr_swap           : 1;
        RK_U32      reserve                 : 23;
    } dec_in_out;

    struct SWREG9_DEC_MODE {
        RK_U32      dec_mode                : 10;
        RK_U32      reserve                 : 22;
    } dec_mode;

    struct SWREG10_DEC_E {
        RK_U32      dec_e                   : 1;
        RK_U32      reserve                 : 31;
    } dec_en;

    struct SWREG11_IMPORTANT_EN {
        RK_U32      reserver                : 1;
        RK_U32      dec_clkgate_e           : 1;
        RK_U32      dec_e_strmd_clkgate_dis : 1;
        RK_U32      reserve0                : 1;

        RK_U32      dec_irq_dis             : 1;
        RK_U32      dec_timeout_e           : 1;
        RK_U32      buf_empty_en            : 1;
        RK_U32      reserve1                : 3;

        RK_U32      dec_e_rewrite_valid     : 1;
        RK_U32      reserve2                : 9;
        RK_U32      softrst_en_p            : 1;
        RK_U32      force_softreset_valid   : 1;
        RK_U32      reserve3                : 10;
    } dec_imp_en;

    struct SWREG12_SENCODARY_EN {
        RK_U32      wr_ddr_align_en         : 1;
        RK_U32      colmv_compress_en       : 1;
        RK_U32      fbc_e                   : 1;
        RK_U32      reserve0                : 1;

        RK_U32      buspr_slot_disable      : 1;
        RK_U32      error_info_en           : 1;
        RK_U32      info_collect_en         : 1;
        RK_U32      wait_reset_en           : 1;

        RK_U32      scanlist_addr_valid_en  : 1;
        RK_U32      reserve1                : 23;
    } dec_sec_en;

    struct SWREG13_EN_MODE_SET {
        RK_U32      timeout_mode                : 1;
        RK_U32      reserve0                    : 2;
        RK_U32      dec_commonirq_mode          : 1;
        RK_U32      reserve1                    : 2;
        RK_U32      stmerror_waitdecfifo_empty  : 1;
        RK_U32      reserve2                    : 2;
        RK_U32      h264orvp9_cabac_error_mode  : 1;
        RK_U32      reserve3                    : 2;
        RK_U32      allow_not_wr_unref_bframe   : 1;
        RK_U32      reserve4                    : 2;
        RK_U32      colmv_error_mode            : 1;

        RK_U32      reserve5            : 2;
        RK_U32      h26x_error_mode     : 1;
        RK_U32      reserve6            : 2;
        RK_U32      ycacherd_prior      : 1;
        RK_U32      reserve7            : 2;
        RK_U32      cur_pic_is_idr      : 1;
        RK_U32      reserve8            : 7;
    } dec_en_mode_set;

    struct SWREG14_FBC_PARAM_SET {
        RK_U32      fbc_force_uncompress    : 1;

        RK_U32      reserve0                : 2;
        RK_U32      allow_16x8_cp_flag      : 1;
        RK_U32      reserve1                : 2;

        RK_U32      fbc_h264_exten_4or8_flag: 1;
        RK_U32      reserve2                : 25;
    } dec_fbc_param_set;

    struct SWREG15_STREAM_PARAM_SET {
        RK_U32      rlc_mode_direct_write   : 1;
        RK_U32      rlc_mode                : 1;
        RK_U32      reserve0                : 3;

        RK_U32      strm_start_bit          : 7;
        RK_U32      reserve1                : 20;
    } dec_str_param_set;

    struct SWREG16_STREAM_LEN {
        RK_U32      stream_len          : 32;
    } dec_str_len;

    struct SWREG17_SLICE_NUMBER {
        RK_U32      slice_num           : 25;
        RK_U32      reserve             : 7;
    } dec_slice_num;

    struct SWREG18_Y_HOR_STRIDE {
        RK_U32      y_hor_virstride     : 16;
        RK_U32      reserve             : 16;
    } dec_y_hor_stride;

    struct SWREG19_UV_HOR_STRIDE {
        RK_U32      uv_hor_virstride    : 16;
        RK_U32      reserve             : 16;
    } dec_uv_hor_stride;

    union {
        struct SWREG20_Y_STRIDE {
            RK_U32      y_virstride         : 28;
            RK_U32      reserve             : 4;
        } dec_y_stride;

        struct SWREG20_FBC_PAYLOAD_OFFSET {
            RK_U32      reserve             : 4;
            RK_U32      payload_st_offset   : 28;
        } dec_fbc_payload_off;
    };


    struct SWREG21_ERROR_CTRL_SET {
        RK_U32      inter_error_prc_mode    : 1;
        RK_U32      error_intra_mode        : 1;
        RK_U32      error_deb_en            : 1;
        RK_U32      picidx_replace          : 5;

        RK_U32      reserve0                : 16;

        RK_U32      roi_error_ctu_cal_en    : 1;
        RK_U32      reserve1                : 7;
    } dec_err_ctl_set;

    struct SWREG22_ERR_ROI_CTU_OFFSET_START {
        RK_U32      roi_x_ctu_offset_st : 12;
        RK_U32      reserve0            : 4;
        RK_U32      roi_y_ctu_offset_st : 12;
        RK_U32      reserve1            : 4;
    } dec_err_roi_ctu_off_start;

    struct SWREG23_ERR_ROI_CTU_OFFSET_END {
        RK_U32      roi_x_ctu_offset_end    : 12;
        RK_U32      reserve0                : 4;
        RK_U32      roi_y_ctu_offset_end    : 12;
        RK_U32      reserve1                : 4;
    } dec_err_roi_ctu_off_end;

    struct SWREG24_CABAC_ERROR_EN_LOWBITS {
        RK_U32      cabac_err_en_lowbits    : 32;
    } dec_cabac_err_en_lowbits;

    struct SWREG25_CABAC_ERROR_EN_HIGHBITS {
        RK_U32      cabac_err_en_highbits   : 30;
        RK_U32      reserve                 : 2;
    } dec_cabac_err_en_highbits;

    struct SWREG26_BLOCK_GATING_EN {
        RK_U32      swreg_block_gating_e    : 16;
        RK_U32      block_gating_en_l2      : 4;
        RK_U32      reserve                 : 11;
        RK_U32      reg_cfg_gating_en       : 1;
    } dec_block_gating_en;
} Vdpu34xRegCommon;

typedef struct rkv_h264d_params_regs_t {


    struct SWREG64_H26X_SET {
        RK_U32      h26x_frame_orslice      : 1;
        RK_U32      h26x_rps_mode           : 1;
        RK_U32      h26x_stream_mode        : 1;
        RK_U32      h26x_stream_lastpacket  : 1;
        RK_U32      h264_firstslice_flag    : 1;
        RK_U32      reserve                 : 27;
    } h26x_set;

    struct SWREG65_CUR_POC {
        RK_U32      cur_top_poc : 32;
    } cur_poc;

    struct SWREG66_H264_CUR_POC1 {
        RK_U32      cur_bot_poc : 32;
    } cur_poc1;

    struct SWREG67_98_H264_REF_POC {
        RK_U32      ref_poc : 32;
    } ref0_31_poc[32];

    struct SWREG99_H264_REG0_3_INFO {

        RK_U32      ref0_field              : 1;
        RK_U32      ref0_topfield_used      : 1;
        RK_U32      ref0_botfield_used      : 1;
        RK_U32      ref0_colmv_use_flag     : 1;
        RK_U32      ref0_reserve            : 4;

        RK_U32      ref1_field              : 1;
        RK_U32      ref1_topfield_used      : 1;
        RK_U32      ref1_botfield_used      : 1;
        RK_U32      ref1_colmv_use_flag     : 1;
        RK_U32      ref1_reserve            : 4;

        RK_U32      ref2_field              : 1;
        RK_U32      ref2_topfield_used      : 1;
        RK_U32      ref2_botfield_used      : 1;
        RK_U32      ref2_colmv_use_flag     : 1;
        RK_U32      ref2_reserve            : 4;

        RK_U32      ref3_field              : 1;
        RK_U32      ref3_topfield_used      : 1;
        RK_U32      ref3_botfield_used      : 1;
        RK_U32      ref3_colmv_use_flag     : 1;
        RK_U32      ref3_reserve            : 4;
    } ref0_3_info;

    struct SWREG100_H264_REG4_7_INFO {

        RK_U32      ref4_field              : 1;
        RK_U32      ref4_topfield_used      : 1;
        RK_U32      ref4_botfield_used      : 1;
        RK_U32      ref4_colmv_use_flag     : 1;
        RK_U32      ref4_reserve            : 4;

        RK_U32      ref5_field              : 1;
        RK_U32      ref5_topfield_used      : 1;
        RK_U32      ref5_botfield_used      : 1;
        RK_U32      ref5_colmv_use_flag     : 1;
        RK_U32      ref5_reserve            : 4;

        RK_U32      ref6_field              : 1;
        RK_U32      ref6_topfield_used      : 1;
        RK_U32      ref6_botfield_used      : 1;
        RK_U32      ref6_colmv_use_flag     : 1;
        RK_U32      ref6_reserve            : 4;

        RK_U32      ref7_field              : 1;
        RK_U32      ref7_topfield_used      : 1;
        RK_U32      ref7_botfield_used      : 1;
        RK_U32      ref7_colmv_use_flag     : 1;
        RK_U32      ref7_reserve            : 4;
    } ref4_7_info;

    struct SWREG101_H264_REG8_11_INFO {

        RK_U32      ref8_field              : 1;
        RK_U32      ref8_topfield_used      : 1;
        RK_U32      ref8_botfield_used      : 1;
        RK_U32      ref8_colmv_use_flag     : 1;
        RK_U32      ref8_reserve            : 4;

        RK_U32      ref9_field              : 1;
        RK_U32      ref9_topfield_used      : 1;
        RK_U32      ref9_botfield_used      : 1;
        RK_U32      ref9_colmv_use_flag     : 1;
        RK_U32      ref9_reserve            : 4;

        RK_U32      ref10_field             : 1;
        RK_U32      ref10_topfield_used     : 1;
        RK_U32      ref10_botfield_used     : 1;
        RK_U32      ref10_colmv_use_flag    : 1;
        RK_U32      ref10_reserve           : 4;

        RK_U32      ref11_field             : 1;
        RK_U32      ref11_topfield_used     : 1;
        RK_U32      ref11_botfield_used     : 1;
        RK_U32      ref11_colmv_use_flag    : 1;
        RK_U32      ref11_reserve           : 4;
    } ref8_11_info;

    struct SWREG102_H264_REG12_15_INFO {

        RK_U32      ref12_field             : 1;
        RK_U32      ref12_topfield_used     : 1;
        RK_U32      ref12_botfield_used     : 1;
        RK_U32      ref12_colmv_use_flag    : 1;
        RK_U32      ref12_reserve           : 4;

        RK_U32      ref13_field             : 1;
        RK_U32      ref13_topfield_used     : 1;
        RK_U32      ref13_botfield_used     : 1;
        RK_U32      ref13_colmv_use_flag    : 1;
        RK_U32      ref13_reserve           : 4;

        RK_U32      ref14_field             : 1;
        RK_U32      ref14_topfield_used     : 1;
        RK_U32      ref14_botfield_used     : 1;
        RK_U32      ref14_colmv_use_flag    : 1;
        RK_U32      ref14_reserve           : 4;

        RK_U32      ref15_field             : 1;
        RK_U32      ref15_topfield_used     : 1;
        RK_U32      ref15_botfield_used     : 1;
        RK_U32      ref15_colmv_use_flag    : 1;
        RK_U32      ref15_reserve           : 4;
    } ref12_15_info;

    struct SWREG103_111_NO_USE_REGS {
        RK_U32  reserve;
    } no_use_regs[9];

    struct SWREG112_ERROR_REF_INFO {
        RK_U32      avs2_ref_error_field        : 1;
        RK_U32      avs2_ref_error_topfield     : 1;
        RK_U32      ref_error_topfield_used     : 1;
        RK_U32      ref_error_botfield_used     : 1;
        RK_U32      reserve                     : 28;
    } err_ref_info;
} Vdpu34xRegH264dParam;

typedef struct rkv_common_addr_regs_t {

    /* offset 128 */
    struct SWREG128_STRM_RLC_BASE {
        RK_U32      strm_rlc_base   : 32;
    } str_rlc_base;

    struct SWREG129_RLCWRITE_BASE {
        RK_U32      rlcwrite_base   : 32;
    } rlcwrite_base;

    struct SWREG130_DECOUT_BASE {
        RK_U32      decout_base         : 10;
        RK_U32      field               : 1;
        RK_U32      topfield_used       : 1;
        RK_U32      botfield_used       : 1;
        RK_U32      field_paired_flag   : 1;
    } decout_base;

    struct SWREG131_COLMV_CUR_BASE {
        RK_U32      colmv_cur_base  : 32;
    } colmv_cur_base;


    struct SWREG132_ERROR_REF_BASE {
        RK_U32      error_ref_base  : 32;
    } error_ref_base;

    struct SWREG133_RCB_INTRAR_BASE {
        RK_U32      rcb_intra_base  : 32;
    } rcb_intra_base;

    struct SWREG134_RCB_TRANSDR_BASE {
        RK_U32      rcb_transd_row_base : 32;
    } rcb_transd_row_base;

    struct SWREG135_RCB_TRANSDC_BASE {
        RK_U32      rcb_transd_col_base : 32;
    } rcb_transd_col_base;

    struct SWREG136_RCB_STRMDR_BASE {
        RK_U32      rcb_streamd_row_base    : 32;
    } rcb_streamd_row_base;

    struct SWREG137_RCB_INTERR_BASE {
        RK_U32      rcb_inter_row_base  : 32;
    } rcb_inter_row_base;

    struct SWREG138_RCB_INTERC_BASE {
        RK_U32      rcb_inter_col_base  : 32;
    } rcb_inter_col_base;

    struct SWREG139_RCB_DBLKR_BASE {
        RK_U32      rcb_dblk_base   : 32;
    } rcb_dblk_base;

    struct SWREG140_RCB_SAOR_BASE {
        RK_U32      rcb_sao_base    : 32;
    } rcb_sao_base;

    struct SWREG141_RCB_FBCR_BASE {
        RK_U32      rcb_fbc_base    : 32;
    } rcb_fbc_base;

    struct SWREG142_RCB_FILTC_COL_BASE {
        RK_U32      rcb_filter_col_base    : 32;
    } rcb_filterd_col_base;
} Vdpu34xRegCommonAddr;

typedef struct rkv_h264d_addr_regs_t {
    /* offset 160 */
    // H264 && HEVC
    // base: 0x300
    struct SWREG160_VP9_DELTA_PROB_BASE {
        RK_U32 vp9_delta_prob_base  : 32;
    } reg160_no_use;

    struct SWREG161_PPS_BASE {
        RK_U32      pps_base    : 32;
    } pps_base;

    RK_U32 reg_162_no_use;

    struct SWREG163_RPS_BASE {
        RK_U32      rps_base    : 32;
    } rps_base;

    struct SWREG164_179_H26x_REF_BASE {
        RK_U32      ref_base    : 32;
    } ref0_15_base[16];

    struct SWREG180_H26x_SCANLIST_BASE {
        RK_U32      scanlist_addr   : 32;
    } h26x_scanlist_base;

    struct SWREG181_196_H26x_COLMV_REF_BASE {
        RK_U32      colmv_base  : 32;
    } ref0_15_colmv_base[16];

    struct SWREG197_H26x_CABACTBL_BASE {
        RK_U32      cabactbl_base   : 32;
    } cabactbl_base;
} Vdpu34xRegH264dAddr;

typedef struct rkv_interrupt_regs_t {
    /* offset 224 */
    struct SWREG224_STA_INT {
        RK_U32      dec_irq         : 1;
        RK_U32      dec_irq_raw     : 1;

        RK_U32      dec_rdy_sta     : 1;
        RK_U32      dec_bus_sta     : 1;
        RK_U32      dec_error_sta   : 1;
        RK_U32      dec_timeout_sta : 1;
        RK_U32      buf_empty_sta   : 1;
        RK_U32      colmv_ref_error_sta : 1;
        RK_U32      cabu_end_sta    : 1;

        RK_U32      softreset_rdy   : 1;

        RK_U32      reserve         : 22;
    } sta_int;

    struct SWREG225_STA_ERR_INFO {
        RK_U32      dec_frame_error_flag    : 1;
        RK_U32      strmd_detect_error_flag : 1;
        RK_U32      reserve                 : 30;
    } err_info;

    struct SWREG226_STA_CABAC_ERROR_STATUS {
        RK_U32      strmd_error_status      : 28;
        RK_U32      reserve                 : 4;
    } strmd_error_status;

    struct SWREG227_STA_COLMV_ERROR_REF_PICIDX {
        RK_U32      colmv_error_ref_picidx  : 4;
        RK_U32      reserve                 : 28;
    } colmv_err_ref_picidx;

    struct SWREG228_STA_CABAC_ERROR_CTU_OFFSET {
        RK_U32      cabac_error_ctu_offset : 32;
    } strmd_err_ctu;

    struct SWREG229_STA_SAOWR_CTU_OFFSET {
        RK_U32      saowr_xoffset : 16;
        RK_U32      saowr_yoffset : 16;
    } saowr_ctu_pos_off;

    struct SWREG230_STA_SLICE_DEC_NUM {
        RK_U32      slicedec_num : 25;
        RK_U32      reserve      : 7;
    } h26x_slice_dec_num;

    struct SWREG231_STA_FRAME_ERROR_CTU_NUM {
        RK_U32      frame_ctu_err_num : 32;
    } frame_err_ctu_num;

    struct SWREG232_STA_ERROR_PACKET_NUM {
        RK_U32      packet_err_num  : 16;
        RK_U32      reserve         : 16;
    } err_packet_num;

    struct SWREG233_STA_ERR_CTU_NUM_IN_RO {
        RK_U32      error_ctu_num_in_roi : 24;
        RK_U32      reserve              : 8;
    } err_ctu_num_in_roi;

    struct SWREG234_STA_B_FRAME_FLAG {
        RK_U32      no_ref_bframe_dec   : 1;
        RK_U32      reserve             : 31;
    } b_frame_falg;

    struct SWREG235_STA_ERR_ROI_PIX_RANGE_Y {
        RK_U32      y_min_value : 10;
        RK_U32      reserve0    : 6;
        RK_U32      y_max_value : 10;
        RK_U32      reserve1    : 6;
    } err_roi_pix_range_y;

    struct SWREG236_STA_ERR_ROI_PIX_RANGE_U {
        RK_U32      u_min_value : 10;
        RK_U32      reserve0    : 6;
        RK_U32      u_max_value : 10;
        RK_U32      reserve1    : 6;
    } err_roi_pix_range_u;

    struct SWREG237_STA_ERR_ROI_PIX_RANGE_V {
        RK_U32      v_min_value     : 10;
        RK_U32      reserve0        : 6;
        RK_U32      v_max_value     : 10;
        RK_U32      reserve1        : 6;
    } err_roi_pix_range_v;
} Vdpu34xRegIrqStatus;

typedef struct RKVDE_PERFORMANCE_REGS_T {
    struct SWREG256_DEBUG_PERF_LATENCY_CTRL0 {
        RK_U32      axi_perf_work_e     : 1;
        RK_U32      axi_perf_clr_e      : 1;
        RK_U32      reserve0            : 1;
        RK_U32      axi_cnt_type        : 1;
        RK_U32      rd_latency_id       : 4;
        RK_U32      rd_latency_thr      : 12;
        RK_U32      reserve1            : 12;
    } perf_latency_ctl0;

    struct SWREG257_DEBUG_PERF_LATENCY_CTRL1 {
        RK_U32      addr_align_type     : 2;
        RK_U32      ar_cnt_id_type      : 1;
        RK_U32      aw_cnt_id_type      : 1;
        RK_U32      ar_count_id         : 4;
        RK_U32      aw_count_id         : 4;
        RK_U32      rd_band_width_mode  : 1;
        RK_U32      reserve             : 19;
    } perf_latency_ctl1;

    struct SWREG258_DEBUG_PERF_RD_MAX_LATENCY_NUM {
        RK_U32      rd_max_latency_num    : 16;
        RK_U32      reserve               : 16;
    } perf_rd_max_latency_num0;

    struct SWREG259_PERF_RD_LATENCY_SAMP_NUM {
        RK_U32      rd_latency_thr_num_ch0  : 32;
    } perf_rd_latency_samp_num;

    struct SWREG260_DEBUG_PERF_RD_LATENCY_ACC_SUM {
        RK_U32      rd_latency_acc_sum      : 32;
    } perf_rd_latency_acc_sum;

    struct SWREG261_DEBUG_PERF_RD_AXI_TOTAL_BYTE {
        RK_U32      perf_rd_axi_total_byte  : 32;
    } perf_rd_axi_total_byte;

    struct SWREG262_DEBUG_PERF_WR_AXI_TOTAL_BYTE {
        RK_U32      perf_wr_axi_total_byte  : 32;
    } perf_wr_axi_total_byte;

    struct SWREG263_DEBUG_PERF_WORKING_CNT {
        RK_U32      perf_working_cnt : 32;
    } perf_working_cnt;

    RK_U32  reserve_reg264;

    struct SWREG265_DEBUG_PERF_SEL {
        RK_U32      perf_cnt0_sel   : 6;
        RK_U32      reserve0        : 2;

        RK_U32      perf_cnt1_sel   : 6;
        RK_U32      reserve1        : 2;

        RK_U32      perf_cnt2_sel   : 6;
        RK_U32      reserve2        : 10;
    } perf_sel;

    struct SWREG266_DEBUG_PERF_CNT0 {
        RK_U32      perf_cnt0 : 32;
    } perf_cnt0;

    struct SWREG267_DEBUG_PERF_CNT1 {
        RK_U32      perf_cnt1 : 32;
    } perf_cnt1;

    struct SWREG268_DEBUG_PERF_CNT2 {
        RK_U32      perf_cnt2 : 32;
    } perf_cnt2;

    RK_U32 reserve_reg269;

    struct SWREG270_DEBUG_QOS_CTRL {
        RK_U32      bus2mc_buffer_qos_level : 8;
        RK_U32      reserve0                : 8;

        RK_U32      axi_rd_hurry_level      : 2;
        RK_U32      reserve1                : 2;
        RK_U32      axi_qr_qos              : 2;
        RK_U32      reserve2                : 2;

        RK_U32      axi_wr_hurry_level      : 2;
        RK_U32      reserve3                : 2;
        RK_U32      axi_rd_qos              : 2;
        RK_U32      reserve4                : 2;
    } qos_ctl;

    struct SWREG271_DEBUG_WAIT_CYCLE_QOS {
        RK_U32      wr_wait_cycle_qos       : 32;
    } wait_cycle_qos;

    struct SWREG272_DEBUG_INT {
        RK_U32      bu_rw_clean                 : 1;
        RK_U32      saowr_frame_rdy             : 1;
        RK_U32      saobu_frame_rdy_valid       : 1;
        RK_U32      colmvwr_frame_rdy_real      : 1;
        RK_U32      cabu_rlcend_valid_real      : 1;
        RK_U32      stream_rdburst_cnteq0_towr  : 1;
        RK_U32      wr_tansfer_cnt              : 6;
        RK_U32      reserve0                    : 4;
        RK_U32      streamfifo_space2full       : 7;
        RK_U32      reserve1                    : 9;
    } debug_int;

    struct SWREG273 {
        RK_U32      bus_status_flag         : 19;
        RK_U32      reserve0                : 12;
        RK_U32      pps_no_ref_bframe_dec_r : 1;
    } reg273;

    struct SWREG274 {
        RK_U32      y_min_value : 10;
        RK_U32      reserve0    : 6;
        RK_U32      y_max_value : 10;
        RK_U32      reserve1    : 6;
    } reg274;

    struct SWREG275 {
        RK_U32      u_min_value : 10;
        RK_U32      reserve0    : 6;
        RK_U32      u_max_value : 10;
        RK_U32      reserve1    : 6;
    } reg275;

    struct SWREG276 {
        RK_U32      v_min_value : 10;
        RK_U32      reserve0    : 6;
        RK_U32      v_max_value : 10;
        RK_U32      reserve1    : 6;
    } reg276;
} Vdpu34xRegStatistic;

typedef struct Vdpu34xH264dRegSet_t {
    Vdpu34xRegCommon        common;
    Vdpu34xRegH264dParam    h264d_param;
    Vdpu34xRegCommonAddr    common_addr;
    Vdpu34xRegH264dAddr     h264d_addr;
    Vdpu34xRegIrqStatus     irq_status;
    Vdpu34xRegStatistic     statistic;
} Vdpu34xH264dRegSet;

#endif /* __HAL_H264D_VDPU34X_REG_H__ */
