/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPU384A_COM_H
#define VDPU384A_COM_H

#include "mpp_device.h"
#include "mpp_buf_slot.h"
#include "vdpu_com.h"

// #define DUMP_VDPU384A_DATAS

typedef struct Vdpu384aRegVersion_t {
    /* SWREG0_ID */
    struct {
        RK_U32 minor_ver                      : 8;
        RK_U32 major_ver                      : 8;
        RK_U32 prod_num                       : 16;
    } reg0;

} Vdpu384aRegVersion;

typedef struct Vdpu384aCtrlReg_t {
    /* SWREG8_DEC_MODE */
    RK_U32 reg8_dec_mode;

    /* SWREG9_IMPORTANT_EN */
    struct {
        RK_U32 dpb_output_dis                 : 1;
        /*
         * 0: dpb data use rkfbc64x4 channel
         * 1: dpb data user main pp channel
         * 2: dpb data use scl down channel
         */
        RK_U32 dpb_data_sel                   : 2;
        RK_U32 reserve0                       : 1;
        RK_U32 low_latency_en                 : 1;
        RK_U32 scale_down_en                  : 1;
        RK_U32 reserve1                       : 1;
        RK_U32 pix_range_det_e                : 1;
        RK_U32 av1_fgs_en                     : 1;
        RK_U32 reserve2                       : 3;
        RK_U32 scale_down_ratio               : 1;
        RK_U32 scale_down_10bitto8bit_en      : 1;
        RK_U32 line_irq_en                    : 3;
        RK_U32 out_cbcr_swap                  : 1;
        RK_U32 dpb_rkfbc_force_uncompress     : 1;
        RK_U32 dpb_rkfbc_sparse_mode          : 1;
        RK_U32 reserve3                       : 1;
        RK_U32 pp_m_fbc32x8_force_uncompress  : 1;
        RK_U32 pp_m_fbc32x8_sparse_mode       : 1;
        RK_U32 inter_max_mv_detect_en         : 1;
        /*
         * 0:disable pp main channel output
         * 1:pp main channel output raster picture to ddr.
         * 2:pp main channel output tile4x4 picture to ddr.
         * 3:pp main channel output afbc32x8 picture to ddr.
         */
        RK_U32 pp_m_output_mode               : 2;
        RK_U32 reserve4                       : 6;
    } reg9;

    /* SWREG10_BLOCK_GATING_EN */
    struct {
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

    /* SWREG11_CFG_PARA */
    struct {
        RK_U32 frame_irq_dis                  : 1;
        RK_U32 reserve0                       : 8;
        RK_U32 dec_timeout_dis                : 1;
        RK_U32 reserve1                       : 6;
        RK_U32 rd_outstanding                 : 8;
        RK_U32 wr_outstanding                 : 8;
    } reg11;

    /* SWREG12_CACHE_HASH_MASK */
    struct {
        RK_U32 reserve0                       : 7;
        RK_U32 cache_hash_mask                : 25;
    } reg12;

    /* SWREG13_CORE_TIMEOUT_THRESHOLD */
    RK_U32 reg13_core_timeout_threshold;

    /* SWREG14_LINE_IRQ_CTRL */
    struct {
        RK_U32 dec_line_irq_step              : 16;
        RK_U32 dec_line_offset_y_st           : 16;
    } reg14;

    /* copy from llp, media group add */
    /* SWREG15_IRQ_STA */
    struct {
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

    /* SWREG16_ERROR_CTRL_SET */
    struct {
        RK_U32 error_proc_disable             : 1;
        RK_U32 reserve0                       : 3;
        RK_U32 error_proc_mode                : 1;
        RK_U32 reserve1                       : 3;
        RK_U32 error_spread_disable           : 1;
        RK_U32 error_fill_mode                : 1;
        RK_U32 reserve2                       : 14;
        RK_U32 roi_error_ctu_cal_en           : 1;
        RK_U32 reserve3                       : 7;
    } reg16;

    /* SWREG17_ERR_ROI_CTU_OFFSET_START */
    struct {
        RK_U32 roi_x_ctu_offset_st            : 12;
        RK_U32 reserve0                       : 4;
        RK_U32 roi_y_ctu_offset_st            : 12;
        RK_U32 reserve1                       : 4;
    } reg17;

    /* SWREG18_ERR_ROI_CTU_OFFSET_END */
    struct {
        RK_U32 roi_x_ctu_offset_end           : 12;
        RK_U32 reserve0                       : 4;
        RK_U32 roi_y_ctu_offset_end           : 12;
        RK_U32 reserve1                       : 4;
    } reg18;

    /* SWREG19_ERROR_REF_INFO */
    struct {
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

    /* SWREG23_INVALID_PIXEL_FILL */
    struct {
        RK_U32 fill_y                         : 10;
        RK_U32 fill_u                         : 10;
        RK_U32 fill_v                         : 10;
        RK_U32 reserve0                       : 2;
    } reg23;

    RK_U32 reserve_reg24_27[4];

    /* SWREG28_DEBUG_PERF_LATENCY_CTRL0 */
    struct {
        RK_U32 axi_perf_work_e                : 1;
        RK_U32 reserve0                       : 2;
        RK_U32 axi_cnt_type                   : 1;
        RK_U32 rd_latency_id                  : 8;
        RK_U32 reserve1                       : 4;
        RK_U32 rd_latency_thr                 : 12;
        RK_U32 reserve2                       : 4;
    } reg28;

    /* SWREG29_DEBUG_PERF_LATENCY_CTRL1 */
    struct {
        RK_U32 addr_align_type                : 2;
        RK_U32 ar_cnt_id_type                 : 1;
        RK_U32 aw_cnt_id_type                 : 1;
        RK_U32 ar_count_id                    : 8;
        RK_U32 reserve0                       : 4;
        RK_U32 aw_count_id                    : 8;
        RK_U32 rd_band_width_mode             : 1;
        RK_U32 reserve1                       : 7;
    } reg29;

    /* SWREG30_QOS_CTRL */
    struct {
        RK_U32 axi_wr_qos_level               : 4;
        RK_U32 reserve0                       : 4;
        RK_U32 axi_wr_qos                     : 4;
        RK_U32 reserve1                       : 4;
        RK_U32 axi_rd_qos_level               : 4;
        RK_U32 reserve2                       : 4;
        RK_U32 axi_rd_qos                     : 4;
        RK_U32 reserve3                       : 4;
    } reg30;

} Vdpu384aCtrlReg;

typedef struct Vdpu384aRegCommParas_t {
    /* SWREG64_H26X_PARA */
    RK_U32 reg64_unused_bits;

    /* SWREG65_STREAM_PARAM_SET */
    RK_U32 reg65_strm_start_bit;

    /* SWREG66_STREAM_LEN */
    RK_U32 reg66_stream_len;

    /* SWREG67_GLOBAL_LEN */
    RK_U32 reg67_global_len;

    /* SWREG68_DPB_HOR_STRIDE */
    RK_U32 reg68_dpb_hor_virstride;

    RK_U32 reserve_reg69_70[2];

    /* SWREG71_SCL_Y_HOR_VIRSTRIDE */
    RK_U32 reg71_scl_ref_hor_virstride;

    /* SWREG72_SCL_UV_HOR_VIRSTRIDE */
    RK_U32 reg72_scl_ref_raster_uv_hor_virstride;

    /* SWREG73_SCL_Y_VIRSTRIDE */
    RK_U32 reg73_scl_ref_virstride;

    /* SWREG74_FGS_Y_HOR_VIRSTRIDE */
    RK_U32 reg74_fgs_ref_hor_virstride;

    RK_U32 reserve_reg75_76[2];

    /* SWREG77_HEAD_HOR_STRIDE */
    RK_U32 reg77_pp_m_hor_stride;

    /* SWREG78_PP_M_RASTER_UV_HOR_STRIDE */
    RK_U32 reg78_pp_m_uv_hor_stride;

    /* SWREG79_PP_M_Y_STRIDE */
    RK_U32 reg79_pp_m_y_virstride;

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

} Vdpu384aRegCommParas;

typedef struct Vdpu384aRegCommonAddr_t {
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

    /* SWREG135_PP_M_DECOUT_BASE */
    RK_U32 reg135_pp_m_decout_base;

    /* SWREG136_PP_M_FBC32x8_PAYLOAD_OFFSET */
    RK_U32 reg136_pp_m_fbc32x8_payload_offset;

    RK_U32 reserve_reg137_139[3];

    union {
        Vdpu38xRcbRegSet rcb_regs;
        struct {
            RK_U32 reg140_rcb_strmd_row_offset;
            RK_U32 reg141_rcb_strmd_row_len;
            RK_U32 reg142_rcb_strmd_tile_row_offset;
            RK_U32 reg143_rcb_strmd_tile_row_len;
            RK_U32 reg144_rcb_inter_row_offset;
            RK_U32 reg145_rcb_inter_row_len;
            RK_U32 reg146_rcb_inter_tile_row_offset;
            RK_U32 reg147_rcb_inter_tile_row_len;
            RK_U32 reg148_rcb_intra_row_offset;
            RK_U32 reg149_rcb_intra_row_len;
            RK_U32 reg150_rcb_intra_tile_row_offset;
            RK_U32 reg151_rcb_intra_tile_row_len;
            RK_U32 reg152_rcb_filterd_row_offset;
            RK_U32 reg153_rcb_filterd_row_len;
            RK_U32 reg154_rcb_filterd_protect_row_offset; // deleted in vdpu384a
            RK_U32 reg155_rcb_filterd_protect_row_len;    // deleted in vdpu384a
            RK_U32 reg156_rcb_filterd_tile_row_offset;
            RK_U32 reg157_rcb_filterd_tile_row_len;
            RK_U32 reg158_rcb_filterd_tile_col_offset;
            RK_U32 reg159_rcb_filterd_tile_col_len;
            RK_U32 reg160_rcb_filterd_av1_upscale_tile_col_offset;
            RK_U32 reg161_rcb_filterd_av1_upscale_tile_col_len;
        };
    };

    RK_U32 reserve_reg162_167[6];

    /* SWREG168_DECOUT_BASE */
    RK_U32 reg168_dpb_decout_base;

    /* SWREG169_ERROR_REF_BASE */
    RK_U32 reg169_error_ref_base;

    /* SWREG170_185_REF0_15_BASE */
    RK_U32 reg170_185_ref_base[16];

    RK_U32 reserve_reg186_191[6];

    /* SWREG192_PAYLOAD_ST_CUR_BASE */
    RK_U32 reg192_dpb_payload64x4_st_cur_base;

    /* SWREG193_FBC_PAYLOAD_OFFSET */
    RK_U32 reg193_dpb_fbc64x4_payload_offset;

    /* SWREG194_PAYLOAD_ST_ERROR_REF_BASE */
    RK_U32 reg194_payload_st_error_ref_base;

    /* SWREG195_210_PAYLOAD_ST_REF0_15_BASE */
    RK_U32 reg195_210_payload_st_ref_base[16];

    RK_U32 reserve_reg211_215[5];

    /* SWREG216_COLMV_CUR_BASE */
    RK_U32 reg216_colmv_cur_base;

    /* SWREG217_232_COLMV_REF0_15_BASE */
    RK_U32 reg217_232_colmv_ref_base[16];
} Vdpu384aRegCommonAddr;

typedef struct Vdpu384aRegStatistic_t {
    /* SWREG256_IDLE_FLAG */
    struct {
        RK_U32 reserve0                       : 24;
        RK_U32 rkvdec_bus_idle_flag           : 1;
        RK_U32 reserve1                       : 7;
    } reg256;

    RK_U32 reserve_reg257;

    /* SWREG258_PERF_MONITOR */
    RK_U32 reg258_perf_rd_max_latency_num;

    /* SWREG259_PERF_MONITOR */
    RK_U32 reg259_perf_rd_latency_samp_num;

    /* SWREG260_PERF_MONITOR */
    RK_U32 reg260_perf_rd_latency_acc_sum;

    /* SWREG261_PERF_MONITOR */
    RK_U32 reg261_perf_rd_axi_total_byte;

    /* SWREG262_PERF_MONITOR */
    RK_U32 reg262_perf_wr_axi_total_bytes;

    /* SWREG263_PERF_MONITOR */
    RK_U32 reg263_perf_working_cnt;

    RK_U32 reserve_reg264_272[9];

    /* SWREG273_REFLIST_IDX_USED */
    RK_U32 reg273_inter_sw_reflst_idx_use;

    RK_U32 reserve_reg274_284[11];

    /* SWREG285_PAYLOAD_CNT */
    RK_U32 reg285_filterd_payload_total_cnt;

    /* SWREG286_WR_OFFSET */
    struct {
        RK_U32 filterd_report_offsety         : 16;
        RK_U32 filterd_report_offsetx         : 16;
    } reg286;

    /* SWREG287_MAX_PIX */
    struct {
        RK_U32 filterd_max_y                  : 10;
        RK_U32 filterd_max_u                  : 10;
        RK_U32 filterd_max_v                  : 10;
        RK_U32 reserve0                       : 2;
    } reg287;

    /* SWREG288_MIN_PIX */
    struct {
        RK_U32 filterd_min_y                  : 10;
        RK_U32 filterd_min_u                  : 10;
        RK_U32 filterd_min_v                  : 10;
        RK_U32 reserve0                       : 2;
    } reg288;

    /* SWREG289_WR_LINE_NUM */
    RK_U32 reg289_filterd_line_irq_offsety;

    RK_U32 reserve_reg290_291[2];

    /* SWREG292_RCB_RW_SUM */
    struct {
        RK_U32 rcb_rd_sum_chk                 : 8;
        RK_U32 rcb_wr_sum_chk                 : 8;
        RK_U32 reserve0                       : 16;
    } reg292;

    RK_U32 reserve_reg293;

    /* SWREG294_ERR_CTU_NUM0 */
    struct {
        RK_U32 error_ctu_num                  : 24;
        RK_U32 roi_error_ctu_num_lowbit       : 8;
    } reg294;

    /* SWREG295_ERR_CTU_NUM1 */
    RK_U32 reg295_roi_error_ctu_num_highbit;

} Vdpu384aRegStatistic;

typedef struct Vdpu384aRegLlp_t {
    /* SWREG0_LINK_MODE */
    struct {
        RK_U32 llp_mmu_zap_cache_dis          : 1;
        RK_U32 reserve0                       : 15;
        RK_U32 core_work_mode                 : 1;
        RK_U32 ccu_core_work_mode             : 1;
        RK_U32 reserve1                       : 3;
        RK_U32 ltb_pause_flag                 : 1;
        RK_U32 reserve2                       : 10;
    } reg0;

    /* SWREG1_CFG_START_ADDR */
    struct {
        RK_U32 reserve0                       : 4;
        RK_U32 reg_cfg_addr                   : 28;
    } reg1;

    /* SWREG2_LINK_MODE */
    struct {
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

    /* SWREG18_IRQ */
    struct {
        RK_U32 rkvdec_irq                     : 1;
        RK_U32 rkvdec_line_irq                : 1;
        RK_U32 reserve0                       : 14;
        RK_U32 wmask                          : 2;
        RK_U32 reserve1                       : 14;
    } reg18;

    /* SWREG19_STA */
    struct {
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

    /* SWREG22_IP_EN */
    struct {
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
        RK_U32 reserve5                       : 5;
        RK_U32 mmu_sel                        : 1;
    } reg22;

    /* SWREG23_IN_OUT */
    struct {
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

} Vdpu384aRegLlp;

typedef struct Vdpu384aRegSet_t {
    Vdpu384aRegVersion     reg_version;       /* 0 */
    Vdpu384aCtrlReg        ctrl_regs;         /* 8-30 */
    Vdpu384aRegCommParas   comm_paras;        /* 64-74, 80-106 */
    Vdpu384aRegCommonAddr  comm_addrs;        /* 128-134, 140-161, 168-185, 192-210, 216-232 */
    Vdpu384aRegStatistic   statistic_regs;    /* 256-295 */
} Vdpu384aRegSet;

#ifdef  __cplusplus
extern "C" {
#endif

void vdpu384a_init_ctrl_regs(Vdpu384aRegSet *regs, MppCodingType codec_t);
void vdpu384a_setup_statistic(Vdpu384aCtrlReg *com);
void vdpu384a_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand);
void vdpu384a_setup_down_scale(MppFrame frame, MppDev dev, Vdpu384aCtrlReg *com, void* comParas);
void vdpu384a_update_thumbnail_frame_info(MppFrame frame);

#ifdef DUMP_VDPU384A_DATAS
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

#endif /* VDPU384A_COM_H */
