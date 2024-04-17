/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPU383_COM_H__
#define __VDPU383_COM_H__

#include "mpp_device.h"
#include "mpp_buf_slot.h"

#define OFFSET_CTRL_REGS            (8 * sizeof(RK_U32))
#define OFFSET_COMMON_ADDR_REGS     (128 * sizeof(RK_U32))
#define OFFSET_COM_NEW_REGS         (320 * sizeof(RK_U32))
#define OFFSET_CODEC_PARAS_REGS     (64 * sizeof(RK_U32))
#define OFFSET_CODEC_ADDR_REGS      (168 * sizeof(RK_U32))
#define OFFSET_INTERRUPT_REGS       (15 * sizeof(RK_U32))

#define RCB_ALLINE_SIZE             (64)

#define MPP_RCB_BYTES(bits)  MPP_ALIGN((bits + 7) / 8, RCB_ALLINE_SIZE)

// #define DUMP_VDPU383_DATAS

typedef enum Vdpu383RcbType_e {
    RCB_STRMD_ROW,
    RCB_STRMD_TILE_ROW,
    RCB_INTER_ROW,
    RCB_INTER_TILE_ROW,
    RCB_INTRA_ROW,
    RCB_INTRA_TILE_ROW,
    RCB_FILTERD_ROW,
    RCB_FILTERD_PROTECT_ROW,
    RCB_FILTERD_TILE_ROW,
    RCB_FILTERD_TILE_COL,
    RCB_FILTERD_AV1_UP_TILE_COL,

    RCB_BUF_COUNT,
} Vdpu383RcbType;

typedef struct Vdpu383RegVersion_t {
    struct SWREG0_ID {
        RK_U32 minor_ver                      : 8;
        RK_U32 major_ver                      : 8;
        RK_U32 prod_num                       : 16;
    } reg0;

} Vdpu383RegVersion;

typedef struct Vdpu383CtrlReg_t {
    /* SWREG8_DEC_MODE */
    RK_U32 reg8_dec_mode;

    struct SWREG9_IMPORTANT_EN {
        RK_U32 fbc_e                          : 1;
        RK_U32 tile_e                         : 1;
        RK_U32 reserve0                       : 2;
        RK_U32 buf_empty_en                   : 1;
        RK_U32 scale_down_en                  : 1;
        RK_U32 reserve1                       : 1;
        RK_U32 pix_range_det_e                : 1;
        RK_U32 av1_fgs_en                     : 1;
        RK_U32 reserve2                       : 7;
        RK_U32 line_irq_en                    : 1;
        RK_U32 out_cbcr_swap                  : 1;
        RK_U32 fbc_force_uncompress           : 1;
        RK_U32 fbc_sparse_mode                : 1;
        RK_U32 reserve3                       : 12;
    } reg9;

    struct SWREG10_BLOCK_GATING_EN {
        RK_U32 strmd_auto_gating_e            : 1;
        RK_U32 inter_auto_gating_e            : 1;
        RK_U32 intra_auto_gating_e            : 1;
        RK_U32 transd_auto_gating_e           : 1;
        RK_U32 recon_auto_gating_e            : 1;
        RK_U32 filterd_auto_gating_e          : 1;
        RK_U32 bus_auto_gating_e              : 1;
        RK_U32 ctrl_auto_gating_e             : 1;
        RK_U32 rcb_auto_gating_e              : 1;
        RK_U32 err_prc_auto_gating_e          : 1;
        RK_U32 reserve0                       : 22;
    } reg10;

    struct SWREG11_CFG_PARA {
        RK_U32 reserve0                       : 9;
        RK_U32 dec_timeout_dis                : 1;
        RK_U32 reserve1                       : 22;
    } reg11;

    struct SWREG12_CACHE_HASH_MASK {
        RK_U32 reserve0                       : 7;
        RK_U32 cache_hash_mask                : 25;
    } reg12;

    /* SWREG13_CORE_TIMEOUT_THRESHOLD */
    RK_U32 reg13_core_timeout_threshold;

    struct SWREG14_LINE_IRQ_CTRL {
        RK_U32 dec_line_irq_step              : 16;
        RK_U32 dec_line_offset_y_st           : 16;
    } reg14;

    /* copy from llp, media group add */
    struct SWREG15_IRQ_STA {
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
        RK_U32 reserve0                       : 7;
        RK_U32 error_spread_disable           : 1;
        RK_U32 reserve1                       : 15;
        RK_U32 roi_error_ctu_cal_en           : 1;
        RK_U32 reserve2                       : 7;
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

    RK_U32 reserve_reg24_26[3];

    struct SWREG27_ALIGN_EN {
        RK_U32 reserve0                       : 4;
        RK_U32 ctu_align_wr_en                : 1;
        RK_U32 reserve1                       : 27;
    } reg27;

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
        RK_U32 reserve3                       : 4;
    } reg30;
} Vdpu383CtrlReg;

typedef struct Vdpu383RegCommonAddr_t {
    /* SWREG128_STRM_BASE */
    RK_U32 reg128_strm_base;

    /* SWREG129_RPS_BASE */
    RK_U32 reg129_rps_base;

    /* SWREG130_CABACTBL_BASE */
    RK_U32 reg130_cabactbl_base;

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

} Vdpu383RegCommonAddr;

typedef struct Vdpu383RegCommParas_t {
    /* SWREG64_H26X_PARA */
    RK_U32 reg64_unused_bits;

    /* SWREG65_STREAM_PARAM_SET */
    RK_U32 reg65_strm_start_bit;

    /* SWREG66_STREAM_LEN */
    RK_U32 reg66_stream_len;

    /* SWREG67_GLOBAL_LEN */
    RK_U32 reg67_global_len;

    /* SWREG68_HOR_STRIDE */
    RK_U32 reg68_hor_virstride;

    /* SWREG69_RASTER_UV_HOR_STRIDE */
    RK_U32 reg69_raster_uv_hor_virstride;

    /* SWREG70_Y_STRIDE */
    RK_U32 reg70_y_virstride;

    /* SWREG71_SCL_Y_HOR_VIRSTRIDE */
    RK_U32 reg71_scl_ref_hor_virstride;

    /* SWREG72_SCL_UV_HOR_VIRSTRIDE */
    RK_U32 reg72_scl_ref_raster_uv_hor_virstride;

    /* SWREG73_SCL_Y_VIRSTRIDE */
    RK_U32 reg73_scl_ref_virstride;

    /* SWREG74_FGS_Y_HOR_VIRSTRIDE */
    RK_U32 reg74_fgs_ref_hor_virstride;

    RK_U32 reserve_reg75_79[5];

    /* SWREG80_ERROR_REF_Y_HOR_VIRSTRIDE */
    RK_U32 reg80_error_ref_hor_virstride;

    /* SWREG81_ERROR_REF_UV_HOR_VIRSTRIDE */
    RK_U32 reg81_error_ref_raster_uv_hor_virstride;

    /* SWREG82_ERROR_REF_Y_VIRSTRIDE */
    RK_U32 reg82_error_ref_virstride;

    /* SWREG83_REF0_Y_HOR_VIRSTRIDE */
    RK_U32 reg83_ref0_hor_virstride;

    /* SWREG84_REF0_UV_HOR_VIRSTRIDE */
    RK_U32 reg84_ref0_raster_uv_hor_virstride;

    /* SWREG85_REF0_Y_VIRSTRIDE */
    RK_U32 reg85_ref0_virstride;

    /* SWREG86_REF1_Y_HOR_VIRSTRIDE */
    RK_U32 reg86_ref1_hor_virstride;

    /* SWREG87_REF1_UV_HOR_VIRSTRIDE */
    RK_U32 reg87_ref1_raster_uv_hor_virstride;

    /* SWREG88_REF1_Y_VIRSTRIDE */
    RK_U32 reg88_ref1_virstride;

    /* SWREG89_REF2_Y_HOR_VIRSTRIDE */
    RK_U32 reg89_ref2_hor_virstride;

    /* SWREG90_REF2_UV_HOR_VIRSTRIDE */
    RK_U32 reg90_ref2_raster_uv_hor_virstride;

    /* SWREG91_REF2_Y_VIRSTRIDE */
    RK_U32 reg91_ref2_virstride;

    /* SWREG92_REF3_Y_HOR_VIRSTRIDE */
    RK_U32 reg92_ref3_hor_virstride;

    /* SWREG93_REF3_UV_HOR_VIRSTRIDE */
    RK_U32 reg93_ref3_raster_uv_hor_virstride;

    /* SWREG94_REF3_Y_VIRSTRIDE */
    RK_U32 reg94_ref3_virstride;

    /* SWREG95_REF4_Y_HOR_VIRSTRIDE */
    RK_U32 reg95_ref4_hor_virstride;

    /* SWREG96_REF4_UV_HOR_VIRSTRIDE */
    RK_U32 reg96_ref4_raster_uv_hor_virstride;

    /* SWREG97_REF4_Y_VIRSTRIDE */
    RK_U32 reg97_ref4_virstride;

    /* SWREG98_REF5_Y_HOR_VIRSTRIDE */
    RK_U32 reg98_ref5_hor_virstride;

    /* SWREG99_REF5_UV_HOR_VIRSTRIDE */
    RK_U32 reg99_ref5_raster_uv_hor_virstride;

    /* SWREG100_REF5_Y_VIRSTRIDE */
    RK_U32 reg100_ref5_virstride;

    /* SWREG101_REF6_Y_HOR_VIRSTRIDE */
    RK_U32 reg101_ref6_hor_virstride;

    /* SWREG102_REF6_UV_HOR_VIRSTRIDE */
    RK_U32 reg102_ref6_raster_uv_hor_virstride;

    /* SWREG103_REF6_Y_VIRSTRIDE */
    RK_U32 reg103_ref6_virstride;

    /* SWREG104_REF7_Y_HOR_VIRSTRIDE */
    RK_U32 reg104_ref7_hor_virstride;

    /* SWREG105_REF7_UV_HOR_VIRSTRIDE */
    RK_U32 reg105_ref7_raster_uv_hor_virstride;

    /* SWREG106_REF7_Y_VIRSTRIDE */
    RK_U32 reg106_ref7_virstride;

} Vdpu383RegCommParas;

typedef struct Vdpu383RegNew_t {
    struct SWREG320_IDLE_FLAG {
        RK_U32 reserve0                       : 24;
        RK_U32 rkvdec_bus_idle_flag           : 1;
        RK_U32 reserve1                       : 7;
    } reg320;

    RK_U32 reserve_reg321;

    /* SWREG322_PERF_MONITOR */
    RK_U32 reg322_perf_rd_max_latency_num;

    /* SWREG323_PERF_MONITOR */
    RK_U32 reg323_perf_rd_latency_samp_num;

    /* SWREG324_PERF_MONITOR */
    RK_U32 reg324_perf_rd_latency_acc_sum;

    /* SWREG325_PERF_MONITOR */
    RK_U32 reg325_perf_rd_axi_total_byte;

    /* SWREG326_PERF_MONITOR */
    RK_U32 reg326_perf_wr_axi_total_bytes;

    /* SWREG327_PERF_MONITOR */
    RK_U32 reg327_perf_working_cnt;

    RK_U32 reserve_reg328_336[9];

    /* SWREG337_REFLIST_IDX_USED */
    RK_U32 reg337_inter_sw_reflst_idx_use;

    RK_U32 reserve_reg338_348[11];

    /* SWREG349_PAYLOAD_CNT */
    RK_U32 reg349_filterd_payload_total_cnt;

    struct SWREG350_WR_OFFSET {
        RK_U32 filterd_report_offsety         : 16;
        RK_U32 filterd_report_offsetx         : 16;
    } reg350;

    struct SWREG351_MAX_PIX {
        RK_U32 filterd_max_y                  : 10;
        RK_U32 filterd_max_u                  : 10;
        RK_U32 filterd_max_v                  : 10;
        RK_U32 reserve0                       : 2;
    } reg351;

    struct SWREG352_MIN_PIX {
        RK_U32 filterd_min_y                  : 10;
        RK_U32 filterd_min_u                  : 10;
        RK_U32 filterd_min_v                  : 10;
        RK_U32 reserve0                       : 2;
    } reg352;

    /* SWREG353_WR_LINE_NUM */
    RK_U32 reg353_filterd_line_irq_offsety;

    RK_U32 reserve_reg354_355[2];

    struct SWREG356_RCB_RW_SUM {
        RK_U32 rcb_rd_sum_chk                 : 8;
        RK_U32 rcb_wr_sum_chk                 : 8;
        RK_U32 reserve0                       : 16;
    } reg356;

    RK_U32 reserve_reg357;

    struct SWREG358_ERR_CTU_NUM0 {
        RK_U32 error_ctu_num                  : 24;
        RK_U32 roi_error_ctu_num_lowbit       : 8;
    } reg358;

    /* SWREG359_ERR_CTU_NUM1 */
    RK_U32 reg359_roi_error_ctu_num_highbit;

} Vdpu383RegNew;

typedef struct Vdpu383RegLlp_t {
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

    /* SWREG8_CUR_LTB_IDX */
    RK_U32 reg8_ltb_idx;

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
        RK_U32 auto_reset_dis                 : 1;
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
        RK_U32 reserve5                       : 5;
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

    /* SWREG24_EXTRA_STRM_BASE */
    RK_U32 reg24_extra_stream_base;

    /* SWREG25_EXTRA_STRM_LEN */
    RK_U32 reg25_extra_stream_len;

    /* SWREG26_EXTRA_STRM_PARA_SET */
    RK_U32 reg26_extra_strm_start_bit;

    /* SWREG27_BUF_EMPTY_RESTART */
    RK_U32 reg27_buf_emtpy_restart_p;

    /* SWREG28_RCB_BASE */
    RK_U32 reg28_rcb_base;

} Vdpu383RegLlp;

typedef struct Vdpu383RcbInfo_t {
    RK_U32              reg_idx;
    RK_S32              size;
    RK_S32              offset;
} Vdpu383RcbInfo;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 vdpu383_get_rcb_buf_size(Vdpu383RcbInfo *info, RK_S32 width, RK_S32 height);
void vdpu383_setup_rcb(Vdpu383RegCommonAddr *reg, MppDev dev, MppBuffer buf, Vdpu383RcbInfo *info);
RK_S32 vdpu383_compare_rcb_size(const void *a, const void *b);
void vdpu383_setup_statistic(Vdpu383CtrlReg *com);
void vdpu383_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand);
RK_S32 vdpu383_set_rcbinfo(MppDev dev, Vdpu383RcbInfo *rcb_info);
void vdpu383_setup_down_scale(MppFrame frame, MppDev dev, Vdpu383CtrlReg *com, void* comParas);
void vdpu383_update_thumbnail_frame_info(MppFrame frame);

#ifdef DUMP_VDPU383_DATAS
extern RK_U32 dump_cur_frame;
extern char dump_cur_dir[128];
extern char dump_cur_fname_path[512];

MPP_RET flip_string(char *str);
MPP_RET dump_data_to_file(char *fname_path, void *data, RK_U32 data_bit_size,
                          RK_U32 line_bits, RK_U32 big_end);
#endif

#ifdef  __cplusplus
}
#endif

#endif /* __VDPU383_COM_H__ */
