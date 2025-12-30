/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPU38X_COM_H
#define VDPU38X_COM_H

#include <math.h>

#include "mpp_device.h"
#include "mpp_buf_slot.h"

#include "vdpu_com.h"
#include "hal_bufs.h"

// #define DUMP_VDPU38X_DATAS

#define OFFSET_CTRL_REGS                     (8 * sizeof(RK_U32))
#define OFFSET_INTERRUPT_REGS                (15 * sizeof(RK_U32))
#define OFFSET_CODEC_PARAS_REGS              (64 * sizeof(RK_U32))
#define OFFSET_COMMON_ADDR_REGS              (128 * sizeof(RK_U32))
#define OFFSET_CODEC_ADDR_REGS               (168 * sizeof(RK_U32))
#define OFFSET_COM_STATISTIC_REGS_VDPU383    (320 * sizeof(RK_U32))
#define OFFSET_COM_STATISTIC_REGS_VDPU384A   (320 * sizeof(RK_U32))
#define OFFSET_COM_STATISTIC_REGS_VDPU384B   (256 * sizeof(RK_U32))

typedef enum Vdpu38xFmt_e {
    MPP_HAL_FMT_YUV400 = 0,
    MPP_HAL_FMT_YUV420,
    MPP_HAL_FMT_YUV422,
    MPP_HAL_FMT_YUV444,
    MPP_HAL_FMT_BUTT,
} Vdpu38xFmt;

extern RK_U32 vdpu38x_rcb_type2loc_map[RCB_BUF_CNT];
extern RK_U32 vdpu38x_intra_uv_coef_map[MPP_HAL_FMT_BUTT];
extern RK_U32 vdpu38x_filter_row_uv_coef_map[MPP_HAL_FMT_BUTT];
extern RK_U32 vdpu38x_filter_col_uv_coef_map[MPP_HAL_FMT_BUTT];

typedef struct RcbTileInfo_t {
    RK_U32 idx;
    RK_U32 lt_x;
    RK_U32 lt_y;
    RK_U32 w;
    RK_U32 h;
} RcbTileInfo;

typedef MPP_RET (*Vdpu38xRcbCalc_f)(void *ctx, RK_U32 *total_sz);

typedef struct Vdpu38xRcbCtx_t {
    RK_U32              pic_w;
    RK_U32              pic_h;

    /* tile info */
    RcbTileInfo         *tile_infos;
    RK_U32              tile_num;
    RK_U32              tile_info_cap;
    RK_U32              tile_dir;   /* 0: left to right, 1: top to bottom */

    /* general */
    Vdpu38xFmt          fmt;
    RK_U32              bit_depth;
    RK_U32              buf_sz;
    /* h264 */
    RK_U32              mbaff_flag;
    /* avs2 */
    RK_U32              alf_en;
    /* av1 */
    RK_U32              lr_en;
    RK_U32              upsc_en;

    VdpuRcbInfo   buf_info[RCB_BUF_CNT];
    Vdpu38xRcbCalc_f    calc_func;
} Vdpu38xRcbCtx;

typedef struct Vdpu38xRegVersion_t {
    struct SWREG0_ID {
        RK_U32 minor_ver                      : 8;
        RK_U32 major_ver                      : 8;
        RK_U32 prod_num                       : 16;
    } reg0;
} Vdpu38xRegVersion;

typedef struct Vdpu38xCtrlReg_t {
    /* SWREG8_DEC_MODE */
    RK_U32 reg8_dec_mode;

    struct SWREG9_IMPORTANT_EN {
        RK_U32 secure_access                  : 1;
        RK_U32 reserve0                       : 3;
        RK_U32 low_latency_en                 : 1;
        RK_U32 scale_down_en                  : 1;
        RK_U32 collect_info_dis               : 1;
        RK_U32 pix_range_det_e                : 1;
        RK_U32 av1_fgs_en                     : 1;
        RK_U32 reserve2                       : 3;
        RK_U32 scale_down_ratio               : 1;
        RK_U32 scale_down_10bitto8bit_en      : 1;
        RK_U32 line_irq_en                    : 2;
        RK_U32 reserve3                       : 1;
        RK_U32 out_cbcr_swap                  : 1;
        RK_U32 fbc_force_uncompress           : 1;
        RK_U32 fbc_sparse_mode                : 1;
        RK_U32 reserve4                       : 3;
        /*
         * vdpu384b is unused
         * refer to collect_info_dis for its
         * original functionality.
         */
        RK_U32 inter_max_mv_detect_en         : 1;
        /*
         * 0:pp output raster.
         * 1:pp output tile4x4.
         * 2:pp output rkfbc64x4.
         * 3:pp output afbc32x8.
         */
        RK_U32 pp_output_mode                 : 2;
        RK_U32 reserve5                       : 6;
    } reg9;

    struct SWREG10_BLOCK_GATING_DIS {
        RK_U32 strmd_auto_gating_dis          : 1;
        RK_U32 inter_auto_gating_dis          : 1;
        RK_U32 intra_auto_gating_dis          : 1;
        RK_U32 transd_auto_gating_dis         : 1;
        RK_U32 recon_auto_gating_dis          : 1;
        RK_U32 filterd_auto_gating_dis        : 1;
        RK_U32 bus_auto_gating_dis            : 1;
        RK_U32 ctrl_auto_gating_dis           : 1;
        RK_U32 rcb_auto_gating_dis            : 1;
        RK_U32 err_prc_auto_gating_dis        : 1;
        RK_U32 cache_auto_gating_dis          : 1;
        RK_U32 reserve0                       : 21;
    } reg10;

    struct SWREG11_CFG_PARA {
        RK_U32 frame_irq_dis                  : 1;
        RK_U32 recon_ahead_release_dis        : 1;
        RK_U32 reserve0                       : 2;
        RK_U32 cache_perf_opt_disable         : 1;
        RK_U32 cache_dis                      : 1;
        RK_U32 cache_pu_parse_opt_dis         : 1;
        RK_U32 cache_multi_id_dis             : 1;
        RK_U32 cache_grp_dis                  : 1;
        RK_U32 dec_timeout_dis                : 1;
        RK_U32 reserve1                       : 3;
        RK_U32 coord_rpt_mode                 : 1;
        RK_U32 reserve2                       : 2;
        RK_U32 rd_outstanding                 : 8;
        RK_U32 wr_outstanding                 : 8;
    } reg11;

    RK_U32 reserve_reg12;

    /* SWREG13_CORE_TIMEOUT_THRESHOLD */
    RK_U32 reg13_core_timeout_threshold;

    struct SWREG14_LINE_IRQ_CTRL {
        RK_U32 dec_line_irq_step              : 16;
        RK_U32 dec_line_offset_y_st           : 16;
    } reg14;

    struct SWREG15_STA {
        RK_U32 rkvdec_frame_rdy_sta           : 1;
        RK_U32 rkvdec_strm_error_sta          : 1;
        RK_U32 rkvdec_core_timeout_sta        : 1;
        RK_U32 rkvdec_ip_timeout_sta          : 1;
        RK_U32 rkvdec_bus_error_sta           : 1;
        RK_U32 rkvdec_buffer_empty_sta        : 1;
        RK_U32 rkvdec_colmv_ref_error_sta     : 1;
        RK_U32 rkvdec_error_spread_sta        : 1;
        RK_U32 create_core_timeout_sta        : 1;
        RK_U32 wlast_miss_match_sta           : 1;
        RK_U32 rkvdec_core_rst_rdy_sta        : 1;
        RK_U32 rkvdec_ip_rst_rdy_sta          : 1;
        RK_U32 force_busidle_rdy_sta          : 1;
        RK_U32 ltb_pause_rdy_sta              : 1;
        RK_U32 ltb_end_flag                   : 1;
        RK_U32 unsupport_decmode_error_sta    : 1;
        RK_U32 wmask_bits                     : 15;
        RK_U32 reserve0                       : 1;
    } reg15;

    struct SWREG16_ERROR_CTRL_SET {
        RK_U32 error_proc_disable             : 1;
        RK_U32 error_proc_restart_mode        : 1;
        RK_U32 error_flush_dis                : 1;
        RK_U32 roi_error_ctu_cal_en           : 1;
        RK_U32 nxt_slice_retrieval_dis        : 1;
        RK_U32 filterd_err_wr_prot_dis        : 1;
        RK_U32 reserve0                       : 2;
        RK_U32 error_spread_disable           : 1;
        RK_U32 error_fill_mode                : 1;
        RK_U32 error_intra_creat_mode         : 1;
        RK_U32 reserve1                       : 13;
        RK_U32 inter_error_gbl_mv_x           : 4;
        RK_U32 inter_error_gbl_mv_y           : 4;
    } reg16;

    struct SWREG17_ERR_ROI_CTU_OFFSET_START {
        RK_U32 roi_x_ctu_offset_st            : 12;
        RK_U32 reserve0                       : 4;
        RK_U32 roi_y_ctu_offset_st            : 12;
        RK_U32 reserve1                       : 4;
    } reg17;

    struct SWREG18_ERR_ROI_CTU_OFFSET_END {
        RK_U32 roi_x_ctu_offset_end           : 12;
        RK_U32 reserve0                       : 4;
        RK_U32 roi_y_ctu_offset_end           : 12;
        RK_U32 reserve1                       : 4;
    } reg18;

    struct SWREG19_ERROR_REF_INFO {
        RK_U32 avs2_ref_error_field           : 1;
        RK_U32 avs2_ref_error_topfield        : 1;
        RK_U32 ref_error_topfield_used        : 1;
        RK_U32 ref_error_botfield_used        : 1;
        RK_U32 reserve0                       : 28;
    } reg19;

    /* SWREG20_CABAC_ERROR_EN_LOWBITS */
    RK_U32 reg20_cabac_error_en_lowbits;

    /* SWREG21_CABAC_ERROR_EN_HIGHBITS */
    RK_U32 reg21_cabac_error_en_highbits;

    RK_U32 reserve_reg22;

    struct SWREG23_INVALID_PIXEL_FILL {
        RK_U32 fill_y                         : 10;
        RK_U32 fill_u                         : 10;
        RK_U32 fill_v                         : 10;
        RK_U32 reserve0                       : 2;
    } reg23;

    RK_U32 reserve_reg24_27[4];

    struct SWREG28_DEBUG_PERF_LATENCY_CTRL0 {
        RK_U32 axi_perf_work_e                : 1;
        RK_U32 reserve0                       : 2;
        RK_U32 axi_cnt_type                   : 1;
        RK_U32 rd_latency_id                  : 8;
        RK_U32 reserve1                       : 4;
        RK_U32 rd_latency_thr                 : 12;
        RK_U32 reserve2                       : 4;
    } reg28;

    struct SWREG29_DEBUG_PERF_LATENCY_CTRL1 {
        RK_U32 addr_align_type                : 2;
        RK_U32 ar_cnt_id_type                 : 1;
        RK_U32 aw_cnt_id_type                 : 1;
        RK_U32 ar_count_id                    : 8;
        RK_U32 reserve0                       : 4;
        RK_U32 aw_count_id                    : 8;
        RK_U32 rd_band_width_mode             : 1;
        RK_U32 reserve1                       : 7;
    } reg29;

    struct SWREG30_QOS_CTRL {
        RK_U32 axi_wr_qos_level               : 4;
        RK_U32 reserve0                       : 4;
        RK_U32 axi_wr_qos                     : 4;
        RK_U32 reserve1                       : 4;
        RK_U32 axi_rd_qos_level               : 4;
        RK_U32 reserve2                       : 4;
        RK_U32 axi_rd_qos                     : 4;
        RK_U32 axi_mmu_rd_qos                 : 4;
    } reg30;

} Vdpu38xCtrlReg;

typedef struct Vdpu38xRegCommParas_t {
    /* SWREG64_H26X_PARA */
    RK_U32 reg64_unused_bits;

    /* SWREG65_STREAM_PARAM_SET */
    RK_U32 reg65_strm_start_bit;

    /* SWREG66_STREAM_LEN */
    RK_U32 reg66_stream_len;

    /* SWREG67_GLOBAL_LEN */
    RK_U32 reg67_global_len;

    /* SWREG68_HEAD_HOR_STRIDE */
    RK_U32 reg68_pp_m_hor_stride;

    /* SWREG69_PP_M_UV_HOR_STRIDE */
    RK_U32 reg69_pp_m_uv_hor_stride;

    /* SWREG70_PP_M_Y_STRIDE */
    RK_U32 reg70_pp_m_y_virstride;

    /* SWREG71_SCL_Y_HOR_VIRSTRIDE */
    RK_U32 reg71_scl_ref_hor_virstride;

    /* SWREG72_SCL_UV_HOR_VIRSTRIDE */
    RK_U32 reg72_scl_ref_raster_uv_hor_virstride;

    /* SWREG73_SCL_Y_VIRSTRIDE */
    RK_U32 reg73_scl_ref_virstride;

    /* SWREG74_FGS_Y_HOR_VIRSTRIDE */
    RK_U32 reg74_fgs_ref_hor_virstride;

    /* SWREG75_FGS_UV_HOR_VIRSTRIDE */
    RK_U32 reg75_fgs_ref_uv_hor_virstride;

    /* SWREG76_FGS_Y_VIRSTRIDE */
    RK_U32 reg76_fgs_ref_virstride;

    RK_U32 reserve_reg77;

    struct SWREG78_ERROR_REF_PIC {
        RK_U32 error_ref_pic_width            : 16;
        RK_U32 error_ref_pic_height           : 16;
    } reg78;

    struct SWREG79_ERROR_REF_SCALE {
        RK_U32 error_ref_scale_step_x         : 16;
        RK_U32 error_ref_scale_step_y         : 16;
    } reg79;

    /* SWREG80_ERROR_REF_Y_HOR_VIRSTRIDE */
    RK_U32 reg80_error_ref_hor_virstride;

    /* SWREG81_ERROR_REF_UV_HOR_VIRSTRIDE */
    RK_U32 reg81_error_ref_raster_uv_hor_virstride;

    /* SWREG82_ERROR_REF_Y_VIRSTRIDE */
    RK_U32 reg82_error_ref_virstride;

    union {
        struct {
            RK_U32 hor_y_stride;
            RK_U32 hor_uv_stride;
            RK_U32 y_stride;
        } ref_stride[VDPU38X_REG_MAX_REF_CNT];
        struct {
            RK_U32 reg83_ref0_hor_virstride;
            RK_U32 reg84_ref0_raster_uv_hor_virstride;
            RK_U32 reg85_ref0_virstride;
            RK_U32 reg86_ref1_hor_virstride;
            RK_U32 reg87_ref1_raster_uv_hor_virstride;
            RK_U32 reg88_ref1_virstride;
            RK_U32 reg89_ref2_hor_virstride;
            RK_U32 reg90_ref2_raster_uv_hor_virstride;
            RK_U32 reg91_ref2_virstride;
            RK_U32 reg92_ref3_hor_virstride;
            RK_U32 reg93_ref3_raster_uv_hor_virstride;
            RK_U32 reg94_ref3_virstride;
            RK_U32 reg95_ref4_hor_virstride;
            RK_U32 reg96_ref4_raster_uv_hor_virstride;
            RK_U32 reg97_ref4_virstride;
            RK_U32 reg98_ref5_hor_virstride;
            RK_U32 reg99_ref5_raster_uv_hor_virstride;
            RK_U32 reg100_ref5_virstride;
            RK_U32 reg101_ref6_hor_virstride;
            RK_U32 reg102_ref6_raster_uv_hor_virstride;
            RK_U32 reg103_ref6_virstride;
            RK_U32 reg104_ref7_hor_virstride;
            RK_U32 reg105_ref7_raster_uv_hor_virstride;
            RK_U32 reg106_ref7_virstride;
        };
    };
} Vdpu38xRegCommParas;

typedef struct Vdpu38xRegCommonAddr_t {
    /* SWREG128_STRM_BASE */
    RK_U32 reg128_strm_base;

    /* SWREG129_STREAM_BUF_ST_BASE */
    RK_U32 reg129_stream_buf_st_base;

    /* SWREG130_STREAM_BUF_END_BASE */
    RK_U32 reg130_stream_buf_end_base;

    /* SWREG131_GBL_BASE */
    RK_U32 reg131_gbl_base;

    /* SWREG132_SCANLIST_ADDR */
    RK_U32 reg132_scanlist_addr;

    /* SWREG133_SCL_BASE */
    RK_U32 reg133_scale_down_base;

    /* SWREG134_FGS_BASE */
    RK_U32 reg134_fgs_base;

    RK_U32 reserve_reg135_139[5];

    /* SWREG140_RCB_STRMD_ROW_OFFSET */
    RK_U32 reg140_rcb_strmd_row_offset;

    /* SWREG141_RCB_STRMD_ROW_LEN */
    RK_U32 reg141_rcb_strmd_row_len;

    /* SWREG142_RCB_STRMD_TILE_ROW_OFFSET */
    RK_U32 reg142_rcb_strmd_tile_row_offset;

    /* SWREG143_RCB_STRMD_TILE_ROW_LEN */
    RK_U32 reg143_rcb_strmd_tile_row_len;

    /* SWREG144_RCB_INTER_ROW_OFFSET */
    RK_U32 reg144_rcb_inter_row_offset;

    /* SWREG145_RCB_INTER_ROW_LEN */
    RK_U32 reg145_rcb_inter_row_len;

    /* SWREG146_RCB_INTER_TILE_ROW_OFFSET */
    RK_U32 reg146_rcb_inter_tile_row_offset;

    /* SWREG147_RCB_INTER_TILE_ROW_LEN */
    RK_U32 reg147_rcb_inter_tile_row_len;

    /* SWREG148_RCB_INTRA_ROW_OFFSET */
    RK_U32 reg148_rcb_intra_row_offset;

    /* SWREG149_RCB_INTRA_ROW_LEN */
    RK_U32 reg149_rcb_intra_row_len;

    /* SWREG150_RCB_INTRA_TILE_ROW_OFFSET */
    RK_U32 reg150_rcb_intra_tile_row_offset;

    /* SWREG151_RCB_INTRA_TILE_ROW_LEN */
    RK_U32 reg151_rcb_intra_tile_row_len;

    /* SWREG152_RCB_FILTERD_ROW_OFFSET */
    RK_U32 reg152_rcb_filterd_row_offset;

    /* SWREG153_RCB_FILTERD_ROW_LEN */
    RK_U32 reg153_rcb_filterd_row_len;

    /* SWREG154_RCB_FILTERD_PROTECT_ROW_OFFSET */
    RK_U32 reg154_rcb_filterd_protect_row_offset;

    /* SWREG155_RCB_FILTERD_PROTECT_ROW_LEN */
    RK_U32 reg155_rcb_filterd_protect_row_len;

    /* SWREG156_RCB_FILTERD_TILE_ROW_OFFSET */
    RK_U32 reg156_rcb_filterd_tile_row_offset;

    /* SWREG157_RCB_FILTERD_TILE_ROW_LEN */
    RK_U32 reg157_rcb_filterd_tile_row_len;

    /* SWREG158_RCB_FILTERD_TILE_COL_OFFSET */
    RK_U32 reg158_rcb_filterd_tile_col_offset;

    /* SWREG159_RCB_FILTERD_TILE_COL_LEN */
    RK_U32 reg159_rcb_filterd_tile_col_len;

    /* SWREG160_RCB_FILTERD_AV1_UPSCALE_TILE_COL_OFFSET */
    RK_U32 reg160_rcb_filterd_av1_upscale_tile_col_offset;

    /* SWREG161_RCB_FILTERD_AV1_UPSCALE_TILE_COL_LEN */
    RK_U32 reg161_rcb_filterd_av1_upscale_tile_col_len;

    RK_U32 reserve_reg162_167[6];

    /* SWREG168_DECOUT_BASE */
    RK_U32 reg168_decout_base;

    /* SWREG169_ERROR_REF_BASE */
    RK_U32 reg169_error_ref_base;

    union {
        /* h264 / h265 / avs2 */
        RK_U32 reg170_185_ref_base[16];
        /* vp9 */
        struct {
            RK_U32 reg170_180[11];
            RK_U32 reg181_segidlast_base;
            RK_U32 reg182_segidcur_base;
            RK_U32 reg183_reserve;
            RK_U32 reg184_lastprob_base;
            RK_U32 reg185_updateprob_base;
        };
        /* av1 */
        struct {
            RK_U32 reg170_av1_last_base;
            RK_U32 reg171_av1golden_base;
            RK_U32 reg172_av1alfter_base;
            RK_U32 reserve_reg173_177[5];
            RK_U32 reg178_av1_coef_rd_base;
            RK_U32 reg179_av1_coef_wr_base;
            RK_U32 reserve_reg180;
            RK_U32 reg181_av1_segid_last_base;
            RK_U32 reg182_av1_segid_cur_base;
            RK_U32 reserve_reg183;
            RK_U32 reg184_av1_noncoef_rd_base;
            RK_U32 reg185_av1_noncoef_wr_base;
        };
    };


    RK_U32 reserve_reg186_190[5];

    /* SWREG191_FGS_FBC_PAYLOAD_ST */
    RK_U32 reg191_fgs_fbc_payload_st_base;

    /* SWREG192_PAYLOAD_ST_CUR_BASE */
    RK_U32 reg192_payload_st_cur_base;

    /* SWREG193_FBC_PAYLOAD_OFFSET */
    RK_U32 reg193_fbc_payload_offset;

    /* SWREG194_PAYLOAD_ST_ERROR_REF_BASE */
    RK_U32 reg194_payload_st_error_ref_base;

    /* SWREG195_PAYLOAD_ST_REF0_BASE */
    RK_U32 reg195_210_payload_st_ref_base[16];

    RK_U32 reserve_reg211_215[5];

    /* SWREG216_COLMV_CUR_BASE */
    RK_U32 reg216_colmv_cur_base;

    /* SWREG217_232_COLMV_REF0_BASE */
    RK_U32 reg217_232_colmv_ref_base[16];

} Vdpu38xRegCommonAddr;


typedef struct Vdpu38xRegStatistic_t {
    struct SWREG256_CTRL_RESP {
        RK_U32 reserve0                       : 24;
        RK_U32 rkvdec_bus_idle_flag           : 1;
        RK_U32 reserve1                       : 7;
    } reg256;

    RK_U32 reserve_reg257;

    /* SWREG258_BUS_RESP */
    RK_U32 reg258_perf_rd_max_latency_num;

    /* SWREG259_BUS_RESP */
    RK_U32 reg259_perf_rd_latency_samp_num;

    /* SWREG260_BUS_RESP */
    RK_U32 reg260_perf_rd_latency_acc_sum;

    /* SWREG261_BUS_RESP */
    RK_U32 reg261_perf_rd_axi_total_byte;

    /* SWREG262_BUS_RESP */
    RK_U32 reg262_perf_wr_axi_total_bytes;

    /* SWREG263_BUS_RESP */
    RK_U32 reg263_perf_working_cnt;

    RK_U32 reserve_reg264_272[9];

    struct SWREG273_CABAC_RESP {
        RK_U32 qp_max                         : 9;
        RK_U32 reserve0                       : 3;
        RK_U32 qp_min                         : 9;
        RK_U32 reserve1                       : 3;
        RK_U32 qp_sum_low8bit                 : 8;
    } reg273;

    /* SWREG274_CABAC_RESP */
    RK_U32 reg274_qp_sum_high;

    /* SWREG275_CABAC_RESP */
    RK_U32 reg275_tu_need_iq_sum;

    /* SWREG276_INTER_RESP */
    RK_U32 reg276_inter_sw_reflst_idx_use;

    /* SWREG277_INTER_RESP */
    RK_U32 reg277_sw_mv_x_out_sum_31_0;

    struct SWREG278_INTER_RESP {
        RK_U32 inter_sw_mv_x_out_sum_43_32    : 12;
        RK_U32 inter_sw_mv_y_out_sum_19_0     : 20;
    } reg278;

    struct SWREG279_INTER_RESP {
        RK_U32 inter_sw_mv_y_out_sum_43_20    : 24;
        RK_U32 dbg_inter_min_mv_x             : 4;
        RK_U32 dbg_inter_min_mv_y             : 4;
    } reg279;

    struct SWREG280_INTER_RESP {
        RK_U32 inter_sw_mv_x_max_out          : 16;
        RK_U32 inter_sw_mv_y_max_out          : 16;
    } reg280;

    struct SWREG281_INTER_RESP {
        RK_U32 inter_sw_mv_x_min_out          : 16;
        RK_U32 inter_sw_mv_y_min_out          : 16;
    } reg281;

    struct SWREG282_INTER_RESP {
        RK_U32 inter_sw_skip_pu_cnt_sum       : 28;
        RK_U32 inter_sw_pu_cnt_sum_3_0        : 4;
    } reg282;

    struct SWREG283_INTER_RESP {
        RK_U32 inter_sw_pu_cnt_sum_27_4       : 24;
        RK_U32 inter_sw_min_pu_cnt_sum_7_0    : 8;
    } reg283;

    /* SWREG284_INTER_RESP */
    RK_U32 reg284_sw_min_pu_cnt_sum_27_8;

    RK_U32 reserve_reg285_286[2];

    /* SWREG287_INTER_RESP */
    RK_U32 reg287_inter_cache_info_reserve0;

    /* SWREG288_INTER_RESP */
    RK_U32 reg288_inter_cache_info_reserve1;

    /* SWREG289_INTER_RESP */
    RK_U32 reg289_inter_cache_info_reserve2;

    /* SWREG290_INTER_RESP */
    RK_U32 reg290_inter_dbg_info_reserve;

    RK_U32 reserve_reg291_298[8];

    /* SWREG299_FILTERD_RESP */
    RK_U32 reg299_filterd_payload_total_cnt;

    struct SWREG300_FILTERD_RESP {
        RK_U32 fdbg_filterd_report_offsety    : 16;
        RK_U32 fdbg_filterd_report_offsetx    : 16;
    } reg300;

    struct SWREG301_FILTERD_RESP {
        RK_U32 fdbg_filterd_max_y             : 10;
        RK_U32 fdbg_filterd_max_u             : 10;
        RK_U32 fdbg_filterd_max_v             : 10;
        RK_U32 reserve0                       : 2;
    } reg301;

    struct SWREG302_FILTERD_RESP {
        RK_U32 fdbg_filterd_min_y             : 10;
        RK_U32 fdbg_filterd_min_u             : 10;
        RK_U32 fdbg_filterd_min_v             : 10;
        RK_U32 reserve0                       : 2;
    } reg302;

    struct SWREG303_FILTERD_RESP {
        RK_U32 fdbg_filterd_ppm_line_irq_offsety : 16;
        RK_U32 fdbg_filterd_scl_line_irq_offsety : 16;
    } reg303;

    /* SWREG304_FILTERD_RESP */
    RK_U32 reg304_filterd_err_ctu_num;

    RK_U32 reserve_reg305_311[7];

    struct SWREG312_RCB_RESP {
        RK_U32 rcb_rd_sum_chk                 : 8;
        RK_U32 rcb_wr_sum_chk                 : 8;
        RK_U32 reserve0                       : 16;
    } reg312;

} Vdpu38xRegStatistic;

typedef struct Vdpu38xRegLlp_t {
    struct SWREG0_LINK_MODE {
        RK_U32 llp_mmu_zap_cache_dis          : 1;
        RK_U32 reserve0                       : 15;
        RK_U32 core_work_mode                 : 1;
        RK_U32 ccu_core_work_mode             : 1;
        RK_U32 reserve1                       : 3;
        RK_U32 ltb_pause_flag                 : 1;
        RK_U32 reserve2                       : 10;
    } reg0;

    struct SWREG1_CFG_START_ADDR {
        RK_U32 reserve0                       : 4;
        RK_U32 reg_cfg_addr                   : 28;
    } reg1;

    struct SWREG2_LINK_MODE {
        RK_U32 pre_frame_num                  : 30;
        RK_U32 reserve0                       : 1;
        RK_U32 link_mode                      : 1;
    } reg2;

    /* SWREG3_CONFIG_DONE */
    RK_U32 reg3_done;

    /* SWREG4_DECODERED_NUM */
    RK_U32 reg4_num;

    /* SWREG5_DEC_TOTAL_NUM */
    RK_U32 reg5_total_num;

    /* SWREG6_LINK_MODE_EN */
    RK_U32 reg6_mode_en;

    /* SWREG7_SKIP_NUM */
    RK_U32 reg7_num;

    struct SWREG8_CUR_LTB_IDX {
        RK_U32 reserve0                       : 4;
        RK_U32 ltb_idx                        : 28;
    } reg8;

    RK_U32 reserve_reg9_15[7];

    /* SWREG16_DEC_E */
    RK_U32 reg16_dec_e;

    /* SWREG17_SOFT_RST */
    RK_U32 reg17_rkvdec_ip_rst_p;

    struct SWREG18_IRQ {
        RK_U32 rkvdec_irq                     : 1;
        RK_U32 rkvdec_line_irq                : 1;
        RK_U32 reserve0                       : 14;
        RK_U32 wmask                          : 2;
        RK_U32 reserve1                       : 14;
    } reg18;

    struct SWREG19_STA {
        RK_U32 rkvdec_frame_rdy_sta           : 1;
        RK_U32 rkvdec_strm_error_sta          : 1;
        RK_U32 rkvdec_core_timeout_sta        : 1;
        RK_U32 rkvdec_ip_timeout_sta          : 1;
        RK_U32 rkvdec_bus_error_sta           : 1;
        RK_U32 rkvdec_buffer_empty_sta        : 1;
        RK_U32 rkvdec_colmv_ref_error_sta     : 1;
        RK_U32 rkvdec_error_spread_sta        : 1;
        RK_U32 create_core_timeout_sta        : 1;
        RK_U32 wlast_miss_match_sta           : 1;
        RK_U32 rkvdec_core_rst_rdy_sta        : 1;
        RK_U32 rkvdec_ip_rst_rdy_sta          : 1;
        RK_U32 force_busidle_rdy_sta          : 1;
        RK_U32 ltb_pause_rdy_sta              : 1;
        RK_U32 ltb_end_flag                   : 1;
        RK_U32 unsupport_decmode_error_sta    : 1;
        RK_U32 wmask_bits                     : 15;
        RK_U32 reserve0                       : 1;
    } reg19;

    RK_U32 reserve_reg20;

    /* SWREG21_IP_TIMEOUT_THRESHOD */
    RK_U32 reg21_ip_timeout_threshold;

    struct SWREG22_IP_EN {
        RK_U32 ip_timeout_pause_flag          : 1;
        RK_U32 reserve0                       : 3;
        RK_U32 abnormal_auto_reset_dis        : 1;
        RK_U32 reserve1                       : 3;
        RK_U32 force_busidle_req_flag         : 1;
        RK_U32 reserve2                       : 3;
        RK_U32 bus_clkgate_dis                : 1;
        RK_U32 ctrl_clkgate_dis               : 1;
        RK_U32 reserve3                       : 1;
        RK_U32 irq_dis                        : 1;
        RK_U32 wid_reorder_dis                : 1;
        RK_U32 reserve4                       : 7;
        RK_U32 clk_cru_mode                   : 2;
        RK_U32 reserve5                       : 4;
        RK_U32 rkmmu_rd_mode                  : 1;
        RK_U32 mmu_sel                        : 1;
    } reg22;

    struct SWREG23_IN_OUT {
        RK_U32 endian                         : 1;
        RK_U32 swap32_e                       : 1;
        RK_U32 swap64_e                       : 1;
        RK_U32 str_endian                     : 1;
        RK_U32 str_swap32_e                   : 1;
        RK_U32 str_swap64_e                   : 1;
        RK_U32 reserve0                       : 26;
    } reg23;

    struct SWREG24_EXTRA_STRM_BASE {
        RK_U32 reserve0                       : 4;
        RK_U32 extra_stream_base              : 28;
    } reg24;

    /* SWREG25_EXTRA_STRM_LEN */
    RK_U32 reg25_extra_stream_len;

    /* SWREG26_EXTRA_STRM_PARA_SET */
    RK_U32 reg26_extra_strm_start_bit;

    struct SWREG27_BUF_EMPTY_RESTART {
        RK_U32 buf_emtpy_restart_p            : 1;
        RK_U32 extra_strm_last_packet_flag    : 1;
        RK_U32 reserve0                       : 30;
    } reg27;

    struct SWREG28_RCB_BASE {
        RK_U32 reserve0                       : 4;
        RK_U32 rcb_base                       : 28;
    } reg28;

    /* SWREG29_STRMD_LEN_USED */
    RK_U32 reg29_strm_len_used_bytes;

} Vdpu38xRegLlp;

typedef struct Vdpu38xdRegSet_t {
    Vdpu38xRegVersion     reg_version;
    Vdpu38xCtrlReg        ctrl_regs;         /* 8-30 */
    Vdpu38xRegCommParas   comm_paras;        /* 64-106 */
    Vdpu38xRegCommonAddr  comm_addrs;        /* 128-134, 140-161, 168-185, 192-210, 216-232 */
    Vdpu38xRegStatistic   statistic_regs;    /* 256-312 */
} Vdpu38xRegSet;

typedef enum Vdpu38xTileLoc_e {
    VDPU38X_RCB_IN_TILE_ROW = 0,
    VDPU38X_RCB_IN_TILE_COL,
    VDPU38X_RCB_ON_TILE_ROW,
    VDPU38X_RCB_ON_TILE_COL,
} Vdpu38xTileLoc ;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET vdpu38x_rcb_calc_init(Vdpu38xRcbCtx **ctx);
MPP_RET vdpu38x_rcb_calc_deinit(Vdpu38xRcbCtx *ctx);
MPP_RET vdpu38x_rcb_reset(Vdpu38xRcbCtx *ctx);
MPP_RET vdpu38x_rcb_add_tile_info(Vdpu38xRcbCtx *ctx, RcbTileInfo *tile_info);
MPP_RET vdpu38x_rcb_dump_tile_info(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_pic_w(Vdpu38xRcbCtx *ctx, RK_U32 pic_w);
Vdpu38xFmt vdpu38x_fmt_mpp2hal(MppFrameFormat mpp_fmt);
RK_U32 vdpu38x_rcb_get_pic_w(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_pic_h(Vdpu38xRcbCtx *ctx, RK_U32 pic_h);
RK_U32 vdpu38x_rcb_get_pic_h(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_tile_dir(Vdpu38xRcbCtx *ctx, RK_U32 tile_dir);
RK_U32 vdpu38x_rcb_get_tile_dir(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_fmt(Vdpu38xRcbCtx *ctx, RK_U32 fmt);
Vdpu38xFmt vdpu38x_rcb_get_fmt(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_bit_depth(Vdpu38xRcbCtx *ctx, RK_U32 bit_depth);
RK_U32 vdpu38x_rcb_get_bit_depth(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_mbaff_flag(Vdpu38xRcbCtx *ctx, RK_U32 mbaff_flag);
RK_U32 vdpu38x_rcb_get_mbaff_flag(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_alf_en(Vdpu38xRcbCtx *ctx, RK_U32 alf_en);
RK_U32 vdpu38x_rcb_get_alf_en(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_lr_en(Vdpu38xRcbCtx *ctx, RK_U32 lr_en);
RK_U32 vdpu38x_rcb_get_lr_en(Vdpu38xRcbCtx *ctx);
void vdpu38x_rcb_set_upsc_en(Vdpu38xRcbCtx *ctx, RK_U32 upsc_en);
RK_U32 vdpu38x_rcb_get_upsc_en(Vdpu38xRcbCtx *ctx);
MPP_RET vdpu38x_rcb_get_len(Vdpu38xRcbCtx *ctx, Vdpu38xTileLoc loc, RK_U32 *len);
MPP_RET vdpu38x_rcb_get_extra_size(Vdpu38xRcbCtx *ctx, Vdpu38xTileLoc loc, RK_U32 *extra_sz);
RK_U32 vdpu38x_rcb_reg_info_update(Vdpu38xRcbCtx *ctx, Vdpu38xRcbType type, RK_U32 idx,
                                   RK_FLOAT sz);
RK_U32 vdpu38x_rcb_get_total_size(Vdpu38xRcbCtx *ctx);
MPP_RET vdpu38x_rcb_register_calc_handle(Vdpu38xRcbCtx *ctx, Vdpu38xRcbCalc_f func);
MPP_RET vdpu38x_rcb_calc_exec(Vdpu38xRcbCtx *ctx, RK_U32 *total_sz);
RK_S32 vdpu38x_set_rcbinfo(MppDev dev, VdpuRcbInfo *rcb_info);
MPP_RET vdpu38x_rcb_dump_rcb_result(Vdpu38xRcbCtx *ctx);
MPP_RET vdpu38x_get_fbc_off(MppFrame mframe, RK_U32 *head_stride, RK_U32 *pld_stride, RK_U32 *pld_offset);
MPP_RET vdpu38x_get_tile4x4_h_stride_coeff(MppFrameFormat fmt, RK_U32 *coeff);
void vdpu38x_setup_rcb(Vdpu38xRcbCtx *ctx, Vdpu38xRegCommonAddr *reg, MppDev dev, MppBuffer buf);
void vdpu384b_init_ctrl_regs(Vdpu38xRegSet *regs, MppCodingType codec_t);
void vdpu38x_setup_statistic(Vdpu38xCtrlReg *com);
void vdpu38x_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand);
void vdpu38x_setup_down_scale(MppFrame frame, MppDev dev, Vdpu38xCtrlReg *com, void* comParas);
void vdpu38x_update_thumbnail_frame_info(MppFrame frame);
MPP_RET vdpu38x_setup_scale_origin_bufs(MppFrame mframe, HalBufs *org_bufs, RK_S32 max_cnt);
RK_S32 hal_h265d_avs2d_calc_mv_size(RK_S32 pic_w, RK_S32 pic_h, RK_S32 ctu_w);

#ifdef DUMP_VDPU38X_DATAS
extern RK_U32 vdpu38x_dump_cur_frm;
extern char vdpu38x_dump_cur_dir[128];
extern char vdpu38x_dump_cur_fname_path[512];

MPP_RET vdpu38x_flip_string(char *str);
MPP_RET vdpu38x_dump_data_to_file(char *fname_path, void *data, RK_U32 data_bit_size,
                                  RK_U32 line_bits, RK_U32 big_end, RK_U32 append);
#endif

#ifdef  __cplusplus
}
#endif

#endif /* VDPU38X_COM_H */
