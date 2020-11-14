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

#ifndef __VDPU34X_COM_H__
#define __VDPU34X_COM_H__

#include "rk_type.h"
#include "vdpu34x.h"

#define OFFSET_COMMON_REGS          (8 * sizeof(RK_U32))
#define OFFSET_CODEC_PARAMS_REGS    (64 * sizeof(RK_U32))
#define OFFSET_COMMON_ADDR_REGS     (128 * sizeof(RK_U32))
#define OFFSET_CODEC_ADDR_REGS      (160 * sizeof(RK_U32))
#define OFFSET_INTERRUPT_REGS       (224 * sizeof(RK_U32))

#define RCB_ALLINE_SIZE             (64)
#define RCB_INTRAR_COEF             (6)
#define RCB_TRANSDR_COEF            (1)
#define RCB_TRANSDC_COEF            (1)
#define RCB_STRMDR_COEF             (6)
#define RCB_INTERR_COEF             (6)
#define RCB_INTERC_COEF             (3)
#define RCB_DBLKR_COEF              (21)
#define RCB_SAOR_COEF               (5)
#define RCB_FBCR_COEF               (10)
#define RCB_FILTC_COEF              (67)

/* base: OFFSET_COMMON_REGS */
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

/* base: OFFSET_COMMON_ADDR_REGS */
typedef struct Vdpu34xRegCommonAddr_t {
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
    } rcb_filter_col_base;
} Vdpu34xRegCommonAddr;

/* base: OFFSET_COMMON_ADDR_REGS */
typedef struct Vdpu34xRegIrqStatus_t {
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
        RK_U32      all_frame_error_flag    : 1;
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
        RK_U32      cabac_error_ctu_offset  : 32;
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

    RK_U32  reserve_reg234_237[4];
} Vdpu34xRegIrqStatus;

#endif /* __VDPU34X_COM_H__ */
