/*
*
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

#define MODULE_TAG "hal_h264d_vdpu_reg"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"

#include "h264d_log.h"
#include "hal_regdrv.h"
#include "hal_h264d_global.h"
#include "hal_h264d_api.h"
#include "hal_h264d_vdpu_pkt.h"
#include "hal_h264d_vdpu_reg.h"


HalRegDrv_t g_vdpu_drv[VDPU_MAX_SIZE + 1] = {
    { VDPU_DEC_PIC_INF       , 1 ,  1, 24, "sw01_dec_pic_inf       " },
    { VDPU_DEC_TIMEOUT       , 1 ,  1, 18, "sw01_dec_timeout       " },
    { VDPU_DEC_SLICE_INT     , 1 ,  1, 17, "sw01_dec_slice_int     " },
    { VDPU_DEC_ERROR_INT     , 1 ,  1, 16, "sw01_dec_error_int     " },
    { VDPU_DEC_ASO_INT       , 1 ,  1, 15, "sw01_dec_aso_int       " },
    { VDPU_DEC_BUFFER_INT    , 1 ,  1, 14, "sw01_dec_buffer_int    " },
    { VDPU_DEC_BUS_INT       , 1 ,  1, 13, "sw01_dec_bus_int       " },
    { VDPU_DEC_RDY_INT       , 1 ,  1, 12, "sw01_dec_rdy_int       " },
    { VDPU_DEC_IRQ           , 1 ,  1,  8, "sw01_dec_irq           " },
    { VDPU_DEC_IRQ_DIS       , 1 ,  1,  4, "sw01_dec_irq_dis       " },
    { VDPU_DEC_E             , 1 ,  1,  0, "sw01_dec_e             " },
    { VDPU_DEC_AXI_RD_ID     , 2 ,  8, 24, "sw02_dec_axi_rd_id     " },
    { VDPU_DEC_TIMEOUT_E     , 2 ,  1, 23, "sw02_dec_timeout_e     " },
    { VDPU_DEC_STRSWAP32_E   , 2 ,  1, 22, "sw02_dec_strswap32_e   " },
    { VDPU_DEC_STRENDIAN_E   , 2 ,  1, 21, "sw02_dec_strendian_e   " },
    { VDPU_DEC_INSWAP32_E    , 2 ,  1, 20, "sw02_dec_inswap32_e    " },
    { VDPU_DEC_OUTSWAP32_E   , 2 ,  1, 19, "sw02_dec_outswap32_e   " },
    { VDPU_DEC_DATA_DISC_E   , 2 ,  1, 18, "sw02_dec_data_disc_e   " },
    { VDPU_DEC_OUT_TILED_E   , 2 ,  1, 17, "sw02_dec_out_tiled_e   " },
    { VDPU_DEC_LATENCY       , 2 ,  6, 11, "sw02_dec_latency       " },
    { VDPU_DEC_CLK_GATE_E    , 2 ,  1, 10, "sw02_dec_clk_gate_e    " },
    { VDPU_DEC_IN_ENDIAN     , 2 ,  1,  9, "sw02_dec_in_endian     " },
    { VDPU_DEC_OUT_ENDIAN    , 2 ,  1,  8, "sw02_dec_out_endian    " },
    { VDPU_PRIORITY_MODE     , 2 ,  3,  5, "sw02_priority_mode     " },
    { VDPU_DEC_ADV_PRE_DIS   , 2 ,  1,  6, "sw02_dec_adv_pre_dis   " },
    { VDPU_DEC_SCMD_DIS      , 2 ,  1,  5, "sw02_dec_scmd_dis      " },
    { VDPU_DEC_MAX_BURST     , 2 ,  5,  0, "sw02_dec_max_burst     " },
    { VDPU_DEC_MODE          , 3 ,  4, 28, "sw03_dec_mode          " },
    { VDPU_RLC_MODE_E        , 3 ,  1, 27, "sw03_rlc_mode_e        " },
    { VDPU_SKIP_MODE         , 3 ,  1, 26, "sw03_skip_mode         " },
    { VDPU_DIVX3_E           , 3 ,  1, 25, "sw03_divx3_e           " },
    { VDPU_PJPEG_E           , 3 ,  1, 24, "sw03_pjpeg_e           " },
    { VDPU_PIC_INTERLACE_E   , 3 ,  1, 23, "sw03_pic_interlace_e   " },
    { VDPU_PIC_FIELDMODE_E   , 3 ,  1, 22, "sw03_pic_fieldmode_e   " },
    { VDPU_PIC_B_E           , 3 ,  1, 21, "sw03_pic_b_e           " },
    { VDPU_PIC_INTER_E       , 3 ,  1, 20, "sw03_pic_inter_e       " },
    { VDPU_PIC_TOPFIELD_E    , 3 ,  1, 19, "sw03_pic_topfield_e    " },
    { VDPU_FWD_INTERLACE_E   , 3 ,  1, 18, "sw03_fwd_interlace_e   " },
    { VDPU_SORENSON_E        , 3 ,  1, 17, "sw03_sorenson_e        " },
    { VDPU_REF_TOPFIELD_E    , 3 ,  1, 16, "sw03_ref_topfield_e    " },
    { VDPU_DEC_OUT_DIS       , 3 ,  1, 15, "sw03_dec_out_dis       " },
    { VDPU_FILTERING_DIS     , 3 ,  1, 14, "sw03_filtering_dis     " },
    { VDPU_MVC_E             , 3 ,  1, 13, "sw03_mvc_e             " },
    { VDPU_PIC_FIXED_QUANT   , 3 ,  1, 13, "sw03_pic_fixed_quant   " },
    { VDPU_WRITE_MVS_E       , 3 ,  1, 12, "sw03_write_mvs_e       " },
    { VDPU_REFTOPFIRST_E     , 3 ,  1, 11, "sw03_reftopfirst_e     " },
    { VDPU_SEQ_MBAFF_E       , 3 ,  1, 10, "sw03_seq_mbaff_e       " },
    { VDPU_PICORD_COUNT_E    , 3 ,  1,  9, "sw03_picord_count_e    " },
    { VDPU_DEC_AHB_HLOCK_E   , 3 ,  1,  8, "sw03_dec_ahb_hlock_e   " },
    { VDPU_DEC_AXI_WR_ID     , 3 ,  8,  0, "sw03_dec_axi_wr_id     " },
    { VDPU_PIC_MB_WIDTH      , 4 ,  9, 23, "sw04_pic_mb_width      " },
    { VDPU_MB_WIDTH_OFF      , 4 ,  4, 19, "sw04_mb_width_off      " },
    { VDPU_PIC_MB_HEIGHT_P   , 4 ,  8, 11, "sw04_pic_mb_height_p   " },
    { VDPU_MB_HEIGHT_OFF     , 4 ,  4,  7, "sw04_mb_height_off     " },
    { VDPU_ALT_SCAN_E        , 4 ,  1,  6, "sw04_alt_scan_e        " },
    { VDPU_TOPFIELDFIRST_E   , 4 ,  1,  5, "sw04_topfieldfirst_e   " },
    { VDPU_REF_FRAMES        , 4 ,  5,  0, "sw04_ref_frames        " },
    { VDPU_PIC_MB_W_EXT      , 4 ,  3,  3, "sw04_pic_mb_w_ext      " },
    { VDPU_PIC_MB_H_EXT      , 4 ,  3,  0, "sw04_pic_mb_h_ext      " },
    { VDPU_PIC_REFER_FLAG    , 4 ,  1,  0, "sw04_pic_refer_flag    " },
    { VDPU_STRM_START_BIT    , 5 ,  6, 26, "sw05_strm_start_bit    " },
    { VDPU_SYNC_MARKER_E     , 5 ,  1, 25, "sw05_sync_marker_e     " },
    { VDPU_TYPE1_QUANT_E     , 5 ,  1, 24, "sw05_type1_quant_e     " },
    { VDPU_CH_QP_OFFSET      , 5 ,  5, 19, "sw05_ch_qp_offset      " },
    { VDPU_CH_QP_OFFSET2     , 5 ,  5, 14, "sw05_ch_qp_offset2     " },
    { VDPU_FIELDPIC_FLAG_E   , 5 ,  1,  0, "sw05_fieldpic_flag_e   " },
    { VDPU_INTRADC_VLC_THR   , 5 ,  3, 16, "sw05_intradc_vlc_thr   " },
    { VDPU_VOP_TIME_INCR     , 5 , 16,  0, "sw05_vop_time_incr     " },
    { VDPU_DQ_PROFILE        , 5 ,  1, 24, "sw05_dq_profile        " },
    { VDPU_DQBI_LEVEL        , 5 ,  1, 23, "sw05_dqbi_level        " },
    { VDPU_RANGE_RED_FRM_E   , 5 ,  1, 22, "sw05_range_red_frm_e   " },
    { VDPU_FAST_UVMC_E       , 5 ,  1, 20, "sw05_fast_uvmc_e       " },
    { VDPU_TRANSDCTAB        , 5 ,  1, 17, "sw05_transdctab        " },
    { VDPU_TRANSACFRM        , 5 ,  2, 15, "sw05_transacfrm        " },
    { VDPU_TRANSACFRM2       , 5 ,  2, 13, "sw05_transacfrm2       " },
    { VDPU_MB_MODE_TAB       , 5 ,  3, 10, "sw05_mb_mode_tab       " },
    { VDPU_MVTAB             , 5 ,  3,  7, "sw05_mvtab             " },
    { VDPU_CBPTAB            , 5 ,  3,  4, "sw05_cbptab            " },
    { VDPU_2MV_BLK_PAT_TAB   , 5 ,  2,  2, "sw05_2mv_blk_pat_tab   " },
    { VDPU_4MV_BLK_PAT_TAB   , 5 ,  2,  0, "sw05_4mv_blk_pat_tab   " },
    { VDPU_QSCALE_TYPE       , 5 ,  1, 24, "sw05_qscale_type       " },
    { VDPU_CON_MV_E          , 5 ,  1,  4, "sw05_con_mv_e          " },
    { VDPU_INTRA_DC_PREC     , 5 ,  2,  2, "sw05_intra_dc_prec     " },
    { VDPU_INTRA_VLC_TAB     , 5 ,  1,  1, "sw05_intra_vlc_tab     " },
    { VDPU_FRAME_PRED_DCT    , 5 ,  1,  0, "sw05_frame_pred_dct    " },
    { VDPU_JPEG_QTABLES      , 5 ,  2, 11, "sw05_jpeg_qtables      " },
    { VDPU_JPEG_MODE         , 5 ,  3,  8, "sw05_jpeg_mode         " },
    { VDPU_JPEG_FILRIGHT_E   , 5 ,  1,  7, "sw05_jpeg_filright_e   " },
    { VDPU_JPEG_STREAM_ALL   , 5 ,  1,  6, "sw05_jpeg_stream_all   " },
    { VDPU_CR_AC_VLCTABLE    , 5 ,  1,  5, "sw05_cr_ac_vlctable    " },
    { VDPU_CB_AC_VLCTABLE    , 5 ,  1,  4, "sw05_cb_ac_vlctable    " },
    { VDPU_CR_DC_VLCTABLE    , 5 ,  1,  3, "sw05_cr_dc_vlctable    " },
    { VDPU_CB_DC_VLCTABLE    , 5 ,  1,  2, "sw05_cb_dc_vlctable    " },
    { VDPU_CR_DC_VLCTABLE3   , 5 ,  1,  1, "sw05_cr_dc_vlctable3   " },
    { VDPU_CB_DC_VLCTABLE3   , 5 ,  1,  0, "sw05_cb_dc_vlctable3   " },
    { VDPU_STRM1_START_BIT   , 5 ,  6, 18, "sw05_strm1_start_bit   " },
    { VDPU_HUFFMAN_E         , 5 ,  1, 17, "sw05_huffman_e         " },
    { VDPU_MULTISTREAM_E     , 5 ,  1, 16, "sw05_multistream_e     " },
    { VDPU_BOOLEAN_VALUE     , 5 ,  8,  8, "sw05_boolean_value     " },
    { VDPU_BOOLEAN_RANGE     , 5 ,  8,  0, "sw05_boolean_range     " },
    { VDPU_ALPHA_OFFSET      , 5 ,  5,  5, "sw05_alpha_offset      " },
    { VDPU_BETA_OFFSET       , 5 ,  5,  0, "sw05_beta_offset       " },
    { VDPU_START_CODE_E      , 6 ,  1, 31, "sw06_start_code_e      " },
    { VDPU_INIT_QP           , 6 ,  6, 25, "sw06_init_qp           " },
    { VDPU_CH_8PIX_ILEAV_E   , 6 ,  1, 24, "sw06_ch_8pix_ileav_e   " },
    { VDPU_STREAM_LEN        , 6 , 24,  0, "sw06_stream_len        " },
    { VDPU_CABAC_E           , 7 ,  1, 31, "sw07_cabac_e           " },
    { VDPU_BLACKWHITE_E      , 7 ,  1, 30, "sw07_blackwhite_e      " },
    { VDPU_DIR_8X8_INFER_E   , 7 ,  1, 29, "sw07_dir_8x8_infer_e   " },
    { VDPU_WEIGHT_PRED_E     , 7 ,  1, 28, "sw07_weight_pred_e     " },
    { VDPU_WEIGHT_BIPR_IDC   , 7 ,  2, 26, "sw07_weight_bipr_idc   " },
    { VDPU_FRAMENUM_LEN      , 7 ,  5, 16, "sw07_framenum_len      " },
    { VDPU_FRAMENUM          , 7 , 16,  0, "sw07_framenum          " },
    { VDPU_BITPLANE0_E       , 7 ,  1, 31, "sw07_bitplane0_e       " },
    { VDPU_BITPLANE1_E       , 7 ,  1, 30, "sw07_bitplane1_e       " },
    { VDPU_BITPLANE2_E       , 7 ,  1, 29, "sw07_bitplane2_e       " },
    { VDPU_ALT_PQUANT        , 7 ,  5, 24, "sw07_alt_pquant        " },
    { VDPU_DQ_EDGES          , 7 ,  4, 20, "sw07_dq_edges          " },
    { VDPU_TTMBF             , 7 ,  1, 19, "sw07_ttmbf             " },
    { VDPU_PQINDEX           , 7 ,  5, 14, "sw07_pqindex           " },
    { VDPU_BILIN_MC_E        , 7 ,  1, 12, "sw07_bilin_mc_e        " },
    { VDPU_UNIQP_E           , 7 ,  1, 11, "sw07_uniqp_e           " },
    { VDPU_HALFQP_E          , 7 ,  1, 10, "sw07_halfqp_e          " },
    { VDPU_TTFRM             , 7 ,  2,  8, "sw07_ttfrm             " },
    { VDPU_2ND_BYTE_EMUL_E   , 7 ,  1,  7, "sw07_2nd_byte_emul_e   " },
    { VDPU_DQUANT_E          , 7 ,  1,  6, "sw07_dquant_e          " },
    { VDPU_VC1_ADV_E         , 7 ,  1,  5, "sw07_vc1_adv_e         " },
    { VDPU_PJPEG_FILDOWN_E   , 7 ,  1, 26, "sw07_pjpeg_fildown_e   " },
    { VDPU_PJPEG_WDIV8       , 7 ,  1, 25, "sw07_pjpeg_wdiv8       " },
    { VDPU_PJPEG_HDIV8       , 7 ,  1, 24, "sw07_pjpeg_hdiv8       " },
    { VDPU_PJPEG_AH          , 7 ,  4, 20, "sw07_pjpeg_ah          " },
    { VDPU_PJPEG_AL          , 7 ,  4, 16, "sw07_pjpeg_al          " },
    { VDPU_PJPEG_SS          , 7 ,  8,  8, "sw07_pjpeg_ss          " },
    { VDPU_PJPEG_SE          , 7 ,  8,  0, "sw07_pjpeg_se          " },
    { VDPU_DCT1_START_BIT    , 7 ,  6, 26, "sw07_dct1_start_bit    " },
    { VDPU_DCT2_START_BIT    , 7 ,  6, 20, "sw07_dct2_start_bit    " },
    { VDPU_CH_MV_RES         , 7 ,  1, 13, "sw07_ch_mv_res         " },
    { VDPU_INIT_DC_MATCH0    , 7 ,  3,  9, "sw07_init_dc_match0    " },
    { VDPU_INIT_DC_MATCH1    , 7 ,  3,  6, "sw07_init_dc_match1    " },
    { VDPU_VP7_VERSION       , 7 ,  1,  5, "sw07_vp7_version       " },
    { VDPU_CONST_INTRA_E     , 8 ,  1, 31, "sw08_const_intra_e     " },
    { VDPU_FILT_CTRL_PRES    , 8 ,  1, 30, "sw08_filt_ctrl_pres    " },
    { VDPU_RDPIC_CNT_PRES    , 8 ,  1, 29, "sw08_rdpic_cnt_pres    " },
    { VDPU_8X8TRANS_FLAG_E   , 8 ,  1, 28, "sw08_8x8trans_flag_e   " },
    { VDPU_REFPIC_MK_LEN     , 8 , 11, 17, "sw08_refpic_mk_len     " },
    { VDPU_IDR_PIC_E         , 8 ,  1, 16, "sw08_idr_pic_e         " },
    { VDPU_IDR_PIC_ID        , 8 , 16,  0, "sw08_idr_pic_id        " },
    { VDPU_MV_SCALEFACTOR    , 8 ,  8, 24, "sw08_mv_scalefactor    " },
    { VDPU_REF_DIST_FWD      , 8 ,  5, 19, "sw08_ref_dist_fwd      " },
    { VDPU_REF_DIST_BWD      , 8 ,  5, 14, "sw08_ref_dist_bwd      " },
    { VDPU_LOOP_FILT_LIMIT   , 8 ,  4, 14, "sw08_loop_filt_limit   " },
    { VDPU_VARIANCE_TEST_E   , 8 ,  1, 13, "sw08_variance_test_e   " },
    { VDPU_MV_THRESHOLD      , 8 ,  3, 10, "sw08_mv_threshold      " },
    { VDPU_VAR_THRESHOLD     , 8 , 10,  0, "sw08_var_threshold     " },
    { VDPU_DIVX_IDCT_E       , 8 ,  1,  8, "sw08_divx_idct_e       " },
    { VDPU_DIVX3_SLICE_SIZE  , 8 ,  8,  0, "sw08_divx3_slice_size  " },
    { VDPU_PJPEG_REST_FREQ   , 8 , 16,  0, "sw08_pjpeg_rest_freq   " },
    { VDPU_RV_PROFILE        , 8 ,  2, 30, "sw08_rv_profile        " },
    { VDPU_RV_OSV_QUANT      , 8 ,  2, 28, "sw08_rv_osv_quant      " },
    { VDPU_RV_FWD_SCALE      , 8 , 14, 14, "sw08_rv_fwd_scale      " },
    { VDPU_RV_BWD_SCALE      , 8 , 14,  0, "sw08_rv_bwd_scale      " },
    { VDPU_INIT_DC_COMP0     , 8 , 16, 16, "sw08_init_dc_comp0     " },
    { VDPU_INIT_DC_COMP1     , 8 , 16,  0, "sw08_init_dc_comp1     " },
    { VDPU_PPS_ID            , 9 ,  8, 24, "sw09_pps_id            " },
    { VDPU_REFIDX1_ACTIVE    , 9 ,  5, 19, "sw09_refidx1_active    " },
    { VDPU_REFIDX0_ACTIVE    , 9 ,  5, 14, "sw09_refidx0_active    " },
    { VDPU_POC_LENGTH        , 9 ,  8,  0, "sw09_poc_length        " },
    { VDPU_ICOMP0_E          , 9 ,  1, 24, "sw09_icomp0_e          " },
    { VDPU_ISCALE0           , 9 ,  8, 16, "sw09_iscale0           " },
    { VDPU_ISHIFT0           , 9 , 16,  0, "sw09_ishift0           " },
    { VDPU_STREAM1_LEN       , 9 , 24,  0, "sw09_stream1_len       " },
    { VDPU_MB_CTRL_BASE      , 9 , 32,  0, "sw09_mb_ctrl_base      " },
    { VDPU_PIC_SLICE_AM      , 9 , 13,  0, "sw09_pic_slice_am      " },
    { VDPU_COEFFS_PART_AM    , 9 ,  4, 24, "sw09_coeffs_part_am    " },
    { VDPU_DIFF_MV_BASE      , 10, 32,  0, "sw10_diff_mv_base      " },
    { VDPU_PINIT_RLIST_F9    , 10,  5, 25, "sw10_pinit_rlist_f9    " },
    { VDPU_PINIT_RLIST_F8    , 10,  5, 20, "sw10_pinit_rlist_f8    " },
    { VDPU_PINIT_RLIST_F7    , 10,  5, 15, "sw10_pinit_rlist_f7    " },
    { VDPU_PINIT_RLIST_F6    , 10,  5, 10, "sw10_pinit_rlist_f6    " },
    { VDPU_PINIT_RLIST_F5    , 10,  5,  5, "sw10_pinit_rlist_f5    " },
    { VDPU_PINIT_RLIST_F4    , 10,  5,  0, "sw10_pinit_rlist_f4    " },
    { VDPU_ICOMP1_E          , 10,  1, 24, "sw10_icomp1_e          " },
    { VDPU_ISCALE1           , 10,  8, 16, "sw10_iscale1           " },
    { VDPU_ISHIFT1           , 10, 16,  0, "sw10_ishift1           " },
    { VDPU_SEGMENT_BASE      , 10, 32,  0, "sw10_segment_base      " },
    { VDPU_SEGMENT_UPD_E     , 10,  1,  1, "sw10_segment_upd_e     " },
    { VDPU_SEGMENT_E         , 10,  1,  0, "sw10_segment_e         " },
    { VDPU_I4X4_OR_DC_BASE   , 11, 32,  0, "sw11_i4x4_or_dc_base   " },
    { VDPU_PINIT_RLIST_F15   , 11,  5, 25, "sw11_pinit_rlist_f15   " },
    { VDPU_PINIT_RLIST_F14   , 11,  5, 20, "sw11_pinit_rlist_f14   " },
    { VDPU_PINIT_RLIST_F13   , 11,  5, 15, "sw11_pinit_rlist_f13   " },
    { VDPU_PINIT_RLIST_F12   , 11,  5, 10, "sw11_pinit_rlist_f12   " },
    { VDPU_PINIT_RLIST_F11   , 11,  5,  5, "sw11_pinit_rlist_f11   " },
    { VDPU_PINIT_RLIST_F10   , 11,  5,  0, "sw11_pinit_rlist_f10   " },
    { VDPU_ICOMP2_E          , 11,  1, 24, "sw11_icomp2_e          " },
    { VDPU_ISCALE2           , 11,  8, 16, "sw11_iscale2           " },
    { VDPU_ISHIFT2           , 11, 16,  0, "sw11_ishift2           " },
    { VDPU_DCT3_START_BIT    , 11,  6, 24, "sw11_dct3_start_bit    " },
    { VDPU_DCT4_START_BIT    , 11,  6, 18, "sw11_dct4_start_bit    " },
    { VDPU_DCT5_START_BIT    , 11,  6, 12, "sw11_dct5_start_bit    " },
    { VDPU_DCT6_START_BIT    , 11,  6,  6, "sw11_dct6_start_bit    " },
    { VDPU_DCT7_START_BIT    , 11,  6,  0, "sw11_dct7_start_bit    " },
    { VDPU_RLC_VLC_BASE      , 12, 32,  0, "sw12_rlc_vlc_base      " },
    { VDPU_DEC_OUT_BASE      , 13, 32,  0, "sw13_dec_out_base      " },
    { VDPU_REFER0_BASE       , 14, 32,  0, "sw14_refer0_base       " },
    { VDPU_REFER0_FIELD_E    , 14,  1,  1, "sw14_refer0_field_e    " },
    { VDPU_REFER0_TOPC_E     , 14,  1,  0, "sw14_refer0_topc_e     " },
    { VDPU_JPG_CH_OUT_BASE   , 14, 32,  0, "sw14_jpg_ch_out_base   " },
    { VDPU_REFER1_BASE       , 15, 32,  0, "sw15_refer1_base       " },
    { VDPU_REFER1_FIELD_E    , 15,  1,  1, "sw15_refer1_field_e    " },
    { VDPU_REFER1_TOPC_E     , 15,  1,  0, "sw15_refer1_topc_e     " },
    { VDPU_JPEG_SLICE_H      , 15,  8,  0, "sw15_jpeg_slice_h      " },
    { VDPU_REFER2_BASE       , 16, 32,  0, "sw16_refer2_base       " },
    { VDPU_REFER2_FIELD_E    , 16,  1,  1, "sw16_refer2_field_e    " },
    { VDPU_REFER2_TOPC_E     , 16,  1,  0, "sw16_refer2_topc_e     " },
    { VDPU_AC1_CODE6_CNT     , 16,  7, 24, "sw16_ac1_code6_cnt     " },
    { VDPU_AC1_CODE5_CNT     , 16,  6, 16, "sw16_ac1_code5_cnt     " },
    { VDPU_AC1_CODE4_CNT     , 16,  5, 11, "sw16_ac1_code4_cnt     " },
    { VDPU_AC1_CODE3_CNT     , 16,  4,  7, "sw16_ac1_code3_cnt     " },
    { VDPU_AC1_CODE2_CNT     , 16,  3,  3, "sw16_ac1_code2_cnt     " },
    { VDPU_AC1_CODE1_CNT     , 16,  2,  0, "sw16_ac1_code1_cnt     " },
    { VDPU_REFER3_BASE       , 17, 32,  0, "sw17_refer3_base       " },
    { VDPU_REFER3_FIELD_E    , 17,  1,  1, "sw17_refer3_field_e    " },
    { VDPU_REFER3_TOPC_E     , 17,  1,  0, "sw17_refer3_topc_e     " },
    { VDPU_AC1_CODE10_CNT    , 17,  8, 24, "sw17_ac1_code10_cnt    " },
    { VDPU_AC1_CODE9_CNT     , 17,  8, 16, "sw17_ac1_code9_cnt     " },
    { VDPU_AC1_CODE8_CNT     , 17,  8,  8, "sw17_ac1_code8_cnt     " },
    { VDPU_AC1_CODE7_CNT     , 17,  8,  0, "sw17_ac1_code7_cnt     " },
    { VDPU_REFER4_BASE       , 18, 32,  0, "sw18_refer4_base       " },
    { VDPU_REFER4_FIELD_E    , 18,  1,  1, "sw18_refer4_field_e    " },
    { VDPU_REFER4_TOPC_E     , 18,  1,  0, "sw18_refer4_topc_e     " },
    { VDPU_PIC_HEADER_LEN    , 18, 16, 16, "sw18_pic_header_len    " },
    { VDPU_PIC_4MV_E         , 18,  1, 13, "sw18_pic_4mv_e         " },
    { VDPU_RANGE_RED_REF_E   , 18,  1, 11, "sw18_range_red_ref_e   " },
    { VDPU_VC1_DIFMV_RANGE   , 18,  2,  9, "sw18_vc1_difmv_range   " },
    { VDPU_MV_RANGE          , 18,  2,  6, "sw18_mv_range          " },
    { VDPU_OVERLAP_E         , 18,  1,  5, "sw18_overlap_e         " },
    { VDPU_OVERLAP_METHOD    , 18,  2,  3, "sw18_overlap_method    " },
    { VDPU_ALT_SCAN_FLAG_E   , 18,  1, 19, "sw18_alt_scan_flag_e   " },
    { VDPU_FCODE_FWD_HOR     , 18,  4, 15, "sw18_fcode_fwd_hor     " },
    { VDPU_FCODE_FWD_VER     , 18,  4, 11, "sw18_fcode_fwd_ver     " },
    { VDPU_FCODE_BWD_HOR     , 18,  4,  7, "sw18_fcode_bwd_hor     " },
    { VDPU_FCODE_BWD_VER     , 18,  4,  3, "sw18_fcode_bwd_ver     " },
    { VDPU_MV_ACCURACY_FWD   , 18,  1,  2, "sw18_mv_accuracy_fwd   " },
    { VDPU_MV_ACCURACY_BWD   , 18,  1,  1, "sw18_mv_accuracy_bwd   " },
    { VDPU_MPEG4_VC1_RC      , 18,  1,  1, "sw18_mpeg4_vc1_rc      " },
    { VDPU_PREV_ANC_TYPE     , 18,  1,  0, "sw18_prev_anc_type     " },
    { VDPU_AC1_CODE14_CNT    , 18,  8, 24, "sw18_ac1_code14_cnt    " },
    { VDPU_AC1_CODE13_CNT    , 18,  8, 16, "sw18_ac1_code13_cnt    " },
    { VDPU_AC1_CODE12_CNT    , 18,  8,  8, "sw18_ac1_code12_cnt    " },
    { VDPU_AC1_CODE11_CNT    , 18,  8,  0, "sw18_ac1_code11_cnt    " },
    { VDPU_GREF_SIGN_BIAS    , 18,  1,  0, "sw18_gref_sign_bias    " },
    { VDPU_REFER5_BASE       , 19, 32,  0, "sw19_refer5_base       " },
    { VDPU_REFER5_FIELD_E    , 19,  1,  1, "sw19_refer5_field_e    " },
    { VDPU_REFER5_TOPC_E     , 19,  1,  0, "sw19_refer5_topc_e     " },
    { VDPU_TRB_PER_TRD_D0    , 19, 27,  0, "sw19_trb_per_trd_d0    " },
    { VDPU_ICOMP3_E          , 19,  1, 24, "sw19_icomp3_e          " },
    { VDPU_ISCALE3           , 19,  8, 16, "sw19_iscale3           " },
    { VDPU_ISHIFT3           , 19, 16,  0, "sw19_ishift3           " },
    { VDPU_AC2_CODE4_CNT     , 19,  5, 27, "sw19_ac2_code4_cnt     " },
    { VDPU_AC2_CODE3_CNT     , 19,  4, 23, "sw19_ac2_code3_cnt     " },
    { VDPU_AC2_CODE2_CNT     , 19,  3, 19, "sw19_ac2_code2_cnt     " },
    { VDPU_AC2_CODE1_CNT     , 19,  2, 16, "sw19_ac2_code1_cnt     " },
    { VDPU_AC1_CODE16_CNT    , 19,  8,  8, "sw19_ac1_code16_cnt    " },
    { VDPU_AC1_CODE15_CNT    , 19,  8,  0, "sw19_ac1_code15_cnt    " },
    { VDPU_SCAN_MAP_1        , 19,  6, 24, "sw19_scan_map_1        " },
    { VDPU_SCAN_MAP_2        , 19,  6, 18, "sw19_scan_map_2        " },
    { VDPU_SCAN_MAP_3        , 19,  6, 12, "sw19_scan_map_3        " },
    { VDPU_SCAN_MAP_4        , 19,  6,  6, "sw19_scan_map_4        " },
    { VDPU_SCAN_MAP_5        , 19,  6,  0, "sw19_scan_map_5        " },
    { VDPU_AREF_SIGN_BIAS    , 19,  1,  0, "sw19_aref_sign_bias    " },
    { VDPU_REFER6_BASE       , 20, 32,  0, "sw20_refer6_base       " },
    { VDPU_REFER6_FIELD_E    , 20,  1,  1, "sw20_refer6_field_e    " },
    { VDPU_REFER6_TOPC_E     , 20,  1,  0, "sw20_refer6_topc_e     " },
    { VDPU_TRB_PER_TRD_DM1   , 20, 27,  0, "sw20_trb_per_trd_dm1   " },
    { VDPU_ICOMP4_E          , 20,  1, 24, "sw20_icomp4_e          " },
    { VDPU_ISCALE4           , 20,  8, 16, "sw20_iscale4           " },
    { VDPU_ISHIFT4           , 20, 16,  0, "sw20_ishift4           " },
    { VDPU_AC2_CODE8_CNT     , 20,  8, 24, "sw20_ac2_code8_cnt     " },
    { VDPU_AC2_CODE7_CNT     , 20,  8, 16, "sw20_ac2_code7_cnt     " },
    { VDPU_AC2_CODE6_CNT     , 20,  7,  8, "sw20_ac2_code6_cnt     " },
    { VDPU_AC2_CODE5_CNT     , 20,  6,  0, "sw20_ac2_code5_cnt     " },
    { VDPU_SCAN_MAP_6        , 20,  6, 24, "sw20_scan_map_6        " },
    { VDPU_SCAN_MAP_7        , 20,  6, 18, "sw20_scan_map_7        " },
    { VDPU_SCAN_MAP_8        , 20,  6, 12, "sw20_scan_map_8        " },
    { VDPU_SCAN_MAP_9        , 20,  6,  6, "sw20_scan_map_9        " },
    { VDPU_SCAN_MAP_10       , 20,  6,  0, "sw20_scan_map_10       " },
    { VDPU_REFER7_BASE       , 21, 32,  0, "sw21_refer7_base       " },
    { VDPU_REFER7_FIELD_E    , 21,  1,  1, "sw21_refer7_field_e    " },
    { VDPU_REFER7_TOPC_E     , 21,  1,  0, "sw21_refer7_topc_e     " },
    { VDPU_TRB_PER_TRD_D1    , 21, 27,  0, "sw21_trb_per_trd_d1    " },
    { VDPU_AC2_CODE12_CNT    , 21,  8, 24, "sw21_ac2_code12_cnt    " },
    { VDPU_AC2_CODE11_CNT    , 21,  8, 16, "sw21_ac2_code11_cnt    " },
    { VDPU_AC2_CODE10_CNT    , 21,  8,  8, "sw21_ac2_code10_cnt    " },
    { VDPU_AC2_CODE9_CNT     , 21,  8,  0, "sw21_ac2_code9_cnt     " },
    { VDPU_SCAN_MAP_11       , 21,  6, 24, "sw21_scan_map_11       " },
    { VDPU_SCAN_MAP_12       , 21,  6, 18, "sw21_scan_map_12       " },
    { VDPU_SCAN_MAP_13       , 21,  6, 12, "sw21_scan_map_13       " },
    { VDPU_SCAN_MAP_14       , 21,  6,  6, "sw21_scan_map_14       " },
    { VDPU_SCAN_MAP_15       , 21,  6,  0, "sw21_scan_map_15       " },
    { VDPU_REFER8_BASE       , 22, 32,  0, "sw22_refer8_base       " },
    { VDPU_DCT_STRM1_BASE    , 22, 32,  0, "sw22_dct_strm1_base    " },
    { VDPU_REFER8_FIELD_E    , 22,  1,  1, "sw22_refer8_field_e    " },
    { VDPU_REFER8_TOPC_E     , 22,  1,  0, "sw22_refer8_topc_e     " },
    { VDPU_AC2_CODE16_CNT    , 22,  8, 24, "sw22_ac2_code16_cnt    " },
    { VDPU_AC2_CODE15_CNT    , 22,  8, 16, "sw22_ac2_code15_cnt    " },
    { VDPU_AC2_CODE14_CNT    , 22,  8,  8, "sw22_ac2_code14_cnt    " },
    { VDPU_AC2_CODE13_CNT    , 22,  8,  0, "sw22_ac2_code13_cnt    " },
    { VDPU_SCAN_MAP_16       , 22,  6, 24, "sw22_scan_map_16       " },
    { VDPU_SCAN_MAP_17       , 22,  6, 18, "sw22_scan_map_17       " },
    { VDPU_SCAN_MAP_18       , 22,  6, 12, "sw22_scan_map_18       " },
    { VDPU_SCAN_MAP_19       , 22,  6,  6, "sw22_scan_map_19       " },
    { VDPU_SCAN_MAP_20       , 22,  6,  0, "sw22_scan_map_20       " },
    { VDPU_REFER9_BASE       , 23, 32,  0, "sw23_refer9_base       " },
    { VDPU_DCT_STRM2_BASE    , 23, 32,  0, "sw23_dct_strm2_base    " },
    { VDPU_REFER9_FIELD_E    , 23,  1,  1, "sw23_refer9_field_e    " },
    { VDPU_REFER9_TOPC_E     , 23,  1,  0, "sw23_refer9_topc_e     " },
    { VDPU_DC1_CODE8_CNT     , 23,  4, 28, "sw23_dc1_code8_cnt     " },
    { VDPU_DC1_CODE7_CNT     , 23,  4, 24, "sw23_dc1_code7_cnt     " },
    { VDPU_DC1_CODE6_CNT     , 23,  4, 20, "sw23_dc1_code6_cnt     " },
    { VDPU_DC1_CODE5_CNT     , 23,  4, 16, "sw23_dc1_code5_cnt     " },
    { VDPU_DC1_CODE4_CNT     , 23,  4, 12, "sw23_dc1_code4_cnt     " },
    { VDPU_DC1_CODE3_CNT     , 23,  4,  8, "sw23_dc1_code3_cnt     " },
    { VDPU_DC1_CODE2_CNT     , 23,  3,  4, "sw23_dc1_code2_cnt     " },
    { VDPU_DC1_CODE1_CNT     , 23,  2,  0, "sw23_dc1_code1_cnt     " },
    { VDPU_SCAN_MAP_21       , 23,  6, 24, "sw23_scan_map_21       " },
    { VDPU_SCAN_MAP_22       , 23,  6, 18, "sw23_scan_map_22       " },
    { VDPU_SCAN_MAP_23       , 23,  6, 12, "sw23_scan_map_23       " },
    { VDPU_SCAN_MAP_24       , 23,  6,  6, "sw23_scan_map_24       " },
    { VDPU_SCAN_MAP_25       , 23,  6,  0, "sw23_scan_map_25       " },
    { VDPU_REFER10_BASE      , 24, 32,  0, "sw24_refer10_base      " },
    { VDPU_DCT_STRM3_BASE    , 24, 32,  0, "sw24_dct_strm3_base    " },
    { VDPU_REFER10_FIELD_E   , 24,  1,  1, "sw24_refer10_field_e   " },
    { VDPU_REFER10_TOPC_E    , 24,  1,  0, "sw24_refer10_topc_e    " },
    { VDPU_DC1_CODE16_CNT    , 24,  4, 28, "sw24_dc1_code16_cnt    " },
    { VDPU_DC1_CODE15_CNT    , 24,  4, 24, "sw24_dc1_code15_cnt    " },
    { VDPU_DC1_CODE14_CNT    , 24,  4, 20, "sw24_dc1_code14_cnt    " },
    { VDPU_DC1_CODE13_CNT    , 24,  4, 16, "sw24_dc1_code13_cnt    " },
    { VDPU_DC1_CODE12_CNT    , 24,  4, 12, "sw24_dc1_code12_cnt    " },
    { VDPU_DC1_CODE11_CNT    , 24,  4,  8, "sw24_dc1_code11_cnt    " },
    { VDPU_DC1_CODE10_CNT    , 24,  4,  4, "sw24_dc1_code10_cnt    " },
    { VDPU_DC1_CODE9_CNT     , 24,  4,  0, "sw24_dc1_code9_cnt     " },
    { VDPU_SCAN_MAP_26       , 24,  6, 24, "sw24_scan_map_26       " },
    { VDPU_SCAN_MAP_27       , 24,  6, 18, "sw24_scan_map_27       " },
    { VDPU_SCAN_MAP_28       , 24,  6, 12, "sw24_scan_map_28       " },
    { VDPU_SCAN_MAP_29       , 24,  6,  6, "sw24_scan_map_29       " },
    { VDPU_SCAN_MAP_30       , 24,  6,  0, "sw24_scan_map_30       " },
    { VDPU_REFER11_BASE      , 25, 32,  0, "sw25_refer11_base      " },
    { VDPU_DCT_STRM4_BASE    , 25, 32,  0, "sw25_dct_strm4_base    " },
    { VDPU_REFER11_FIELD_E   , 25,  1,  1, "sw25_refer11_field_e   " },
    { VDPU_REFER11_TOPC_E    , 25,  1,  0, "sw25_refer11_topc_e    " },
    { VDPU_DC2_CODE8_CNT     , 25,  4, 28, "sw25_dc2_code8_cnt     " },
    { VDPU_DC2_CODE7_CNT     , 25,  4, 24, "sw25_dc2_code7_cnt     " },
    { VDPU_DC2_CODE6_CNT     , 25,  4, 20, "sw25_dc2_code6_cnt     " },
    { VDPU_DC2_CODE5_CNT     , 25,  4, 16, "sw25_dc2_code5_cnt     " },
    { VDPU_DC2_CODE4_CNT     , 25,  4, 12, "sw25_dc2_code4_cnt     " },
    { VDPU_DC2_CODE3_CNT     , 25,  4,  8, "sw25_dc2_code3_cnt     " },
    { VDPU_DC2_CODE2_CNT     , 25,  3,  4, "sw25_dc2_code2_cnt     " },
    { VDPU_DC2_CODE1_CNT     , 25,  2,  0, "sw25_dc2_code1_cnt     " },
    { VDPU_SCAN_MAP_31       , 25,  6, 24, "sw25_scan_map_31       " },
    { VDPU_SCAN_MAP_32       , 25,  6, 18, "sw25_scan_map_32       " },
    { VDPU_SCAN_MAP_33       , 25,  6, 12, "sw25_scan_map_33       " },
    { VDPU_SCAN_MAP_34       , 25,  6,  6, "sw25_scan_map_34       " },
    { VDPU_SCAN_MAP_35       , 25,  6,  0, "sw25_scan_map_35       " },
    { VDPU_REFER12_BASE      , 26, 32,  0, "sw26_refer12_base      " },
    { VDPU_DCT_STRM5_BASE    , 26, 32,  0, "sw26_dct_strm5_base    " },
    { VDPU_REFER12_FIELD_E   , 26,  1,  1, "sw26_refer12_field_e   " },
    { VDPU_REFER12_TOPC_E    , 26,  1,  0, "sw26_refer12_topc_e    " },
    { VDPU_DC2_CODE16_CNT    , 26,  4, 28, "sw26_dc2_code16_cnt    " },
    { VDPU_DC2_CODE15_CNT    , 26,  4, 24, "sw26_dc2_code15_cnt    " },
    { VDPU_DC2_CODE14_CNT    , 26,  4, 20, "sw26_dc2_code14_cnt    " },
    { VDPU_DC2_CODE13_CNT    , 26,  4, 16, "sw26_dc2_code13_cnt    " },
    { VDPU_DC2_CODE12_CNT    , 26,  4, 12, "sw26_dc2_code12_cnt    " },
    { VDPU_DC2_CODE11_CNT    , 26,  4,  8, "sw26_dc2_code11_cnt    " },
    { VDPU_DC2_CODE10_CNT    , 26,  4,  4, "sw26_dc2_code10_cnt    " },
    { VDPU_DC2_CODE9_CNT     , 26,  4,  0, "sw26_dc2_code9_cnt     " },
    { VDPU_SCAN_MAP_36       , 26,  6, 24, "sw26_scan_map_36       " },
    { VDPU_SCAN_MAP_37       , 26,  6, 18, "sw26_scan_map_37       " },
    { VDPU_SCAN_MAP_38       , 26,  6, 12, "sw26_scan_map_38       " },
    { VDPU_SCAN_MAP_39       , 26,  6,  6, "sw26_scan_map_39       " },
    { VDPU_SCAN_MAP_40       , 26,  6,  0, "sw26_scan_map_40       " },
    { VDPU_REFER13_BASE      , 27, 32,  0, "sw27_refer13_base      " },
    { VDPU_REFER13_FIELD_E   , 27,  1,  1, "sw27_refer13_field_e   " },
    { VDPU_REFER13_TOPC_E    , 27,  1,  0, "sw27_refer13_topc_e    " },
    { VDPU_DC3_CODE8_CNT     , 27,  4, 28, "sw27_dc3_code8_cnt     " },
    { VDPU_DC3_CODE7_CNT     , 27,  4, 24, "sw27_dc3_code7_cnt     " },
    { VDPU_DC3_CODE6_CNT     , 27,  4, 20, "sw27_dc3_code6_cnt     " },
    { VDPU_DC3_CODE5_CNT     , 27,  4, 16, "sw27_dc3_code5_cnt     " },
    { VDPU_DC3_CODE4_CNT     , 27,  4, 12, "sw27_dc3_code4_cnt     " },
    { VDPU_DC3_CODE3_CNT     , 27,  4,  8, "sw27_dc3_code3_cnt     " },
    { VDPU_DC3_CODE2_CNT     , 27,  3,  4, "sw27_dc3_code2_cnt     " },
    { VDPU_DC3_CODE1_CNT     , 27,  2,  0, "sw27_dc3_code1_cnt     " },
    { VDPU_BITPL_CTRL_BASE   , 27, 32,  0, "sw27_bitpl_ctrl_base   " },
    { VDPU_REFER14_BASE      , 28, 32,  0, "sw28_refer14_base      " },
    { VDPU_DCT_STRM6_BASE    , 28, 32,  0, "sw28_dct_strm6_base    " },
    { VDPU_REFER14_FIELD_E   , 28,  1,  1, "sw28_refer14_field_e   " },
    { VDPU_REFER14_TOPC_E    , 28,  1,  0, "sw28_refer14_topc_e    " },
    { VDPU_REF_INVD_CUR_1    , 28, 16, 16, "sw28_ref_invd_cur_1    " },
    { VDPU_REF_INVD_CUR_0    , 28, 16,  0, "sw28_ref_invd_cur_0    " },
    { VDPU_DC3_CODE16_CNT    , 28,  4, 28, "sw28_dc3_code16_cnt    " },
    { VDPU_DC3_CODE15_CNT    , 28,  4, 24, "sw28_dc3_code15_cnt    " },
    { VDPU_DC3_CODE14_CNT    , 28,  4, 20, "sw28_dc3_code14_cnt    " },
    { VDPU_DC3_CODE13_CNT    , 28,  4, 16, "sw28_dc3_code13_cnt    " },
    { VDPU_DC3_CODE12_CNT    , 28,  4, 12, "sw28_dc3_code12_cnt    " },
    { VDPU_DC3_CODE11_CNT    , 28,  4,  8, "sw28_dc3_code11_cnt    " },
    { VDPU_DC3_CODE10_CNT    , 28,  4,  4, "sw28_dc3_code10_cnt    " },
    { VDPU_DC3_CODE9_CNT     , 28,  4,  0, "sw28_dc3_code9_cnt     " },
    { VDPU_SCAN_MAP_41       , 28,  6, 24, "sw28_scan_map_41       " },
    { VDPU_SCAN_MAP_42       , 28,  6, 18, "sw28_scan_map_42       " },
    { VDPU_SCAN_MAP_43       , 28,  6, 12, "sw28_scan_map_43       " },
    { VDPU_SCAN_MAP_44       , 28,  6,  6, "sw28_scan_map_44       " },
    { VDPU_SCAN_MAP_45       , 28,  6,  0, "sw28_scan_map_45       " },
    { VDPU_REFER15_BASE      , 29, 32,  0, "sw29_refer15_base      " },
    { VDPU_DCT_STRM7_BASE    , 29, 32,  0, "sw29_dct_strm7_base    " },
    { VDPU_REFER15_FIELD_E   , 29,  1,  1, "sw29_refer15_field_e   " },
    { VDPU_REFER15_TOPC_E    , 29,  1,  0, "sw29_refer15_topc_e    " },
    { VDPU_REF_INVD_CUR_3    , 29, 16, 16, "sw29_ref_invd_cur_3    " },
    { VDPU_REF_INVD_CUR_2    , 29, 16,  0, "sw29_ref_invd_cur_2    " },
    { VDPU_SCAN_MAP_46       , 29,  6, 24, "sw29_scan_map_46       " },
    { VDPU_SCAN_MAP_47       , 29,  6, 18, "sw29_scan_map_47       " },
    { VDPU_SCAN_MAP_48       , 29,  6, 12, "sw29_scan_map_48       " },
    { VDPU_SCAN_MAP_49       , 29,  6,  6, "sw29_scan_map_49       " },
    { VDPU_SCAN_MAP_50       , 29,  6,  0, "sw29_scan_map_50       " },
    { VDPU_REFER1_NBR        , 30, 16, 16, "sw30_refer1_nbr        " },
    { VDPU_REFER0_NBR        , 30, 16,  0, "sw30_refer0_nbr        " },
    { VDPU_REF_DIST_CUR_1    , 30, 16, 16, "sw30_ref_dist_cur_1    " },
    { VDPU_REF_DIST_CUR_0    , 30, 16,  0, "sw30_ref_dist_cur_0    " },
    { VDPU_FILT_TYPE         , 30,  1, 31, "sw30_filt_type         " },
    { VDPU_FILT_SHARPNESS    , 30,  3, 28, "sw30_filt_sharpness    " },
    { VDPU_FILT_MB_ADJ_0     , 30,  7, 21, "sw30_filt_mb_adj_0     " },
    { VDPU_FILT_MB_ADJ_1     , 30,  7, 14, "sw30_filt_mb_adj_1     " },
    { VDPU_FILT_MB_ADJ_2     , 30,  7,  7, "sw30_filt_mb_adj_2     " },
    { VDPU_FILT_MB_ADJ_3     , 30,  7,  0, "sw30_filt_mb_adj_3     " },
    { VDPU_REFER3_NBR        , 31, 16, 16, "sw31_refer3_nbr        " },
    { VDPU_REFER2_NBR        , 31, 16,  0, "sw31_refer2_nbr        " },
    { VDPU_SCAN_MAP_51       , 31,  6, 24, "sw31_scan_map_51       " },
    { VDPU_SCAN_MAP_52       , 31,  6, 18, "sw31_scan_map_52       " },
    { VDPU_SCAN_MAP_53       , 31,  6, 12, "sw31_scan_map_53       " },
    { VDPU_SCAN_MAP_54       , 31,  6,  6, "sw31_scan_map_54       " },
    { VDPU_SCAN_MAP_55       , 31,  6,  0, "sw31_scan_map_55       " },
    { VDPU_REF_DIST_CUR_3    , 31, 16, 16, "sw31_ref_dist_cur_3    " },
    { VDPU_REF_DIST_CUR_2    , 31, 16,  0, "sw31_ref_dist_cur_2    " },
    { VDPU_FILT_REF_ADJ_0    , 31,  7, 21, "sw31_filt_ref_adj_0    " },
    { VDPU_FILT_REF_ADJ_1    , 31,  7, 14, "sw31_filt_ref_adj_1    " },
    { VDPU_FILT_REF_ADJ_2    , 31,  7,  7, "sw31_filt_ref_adj_2    " },
    { VDPU_FILT_REF_ADJ_3    , 31,  7,  0, "sw31_filt_ref_adj_3    " },
    { VDPU_REFER5_NBR        , 32, 16, 16, "sw32_refer5_nbr        " },
    { VDPU_REFER4_NBR        , 32, 16,  0, "sw32_refer4_nbr        " },
    { VDPU_SCAN_MAP_56       , 32,  6, 24, "sw32_scan_map_56       " },
    { VDPU_SCAN_MAP_57       , 32,  6, 18, "sw32_scan_map_57       " },
    { VDPU_SCAN_MAP_58       , 32,  6, 12, "sw32_scan_map_58       " },
    { VDPU_SCAN_MAP_59       , 32,  6,  6, "sw32_scan_map_59       " },
    { VDPU_SCAN_MAP_60       , 32,  6,  0, "sw32_scan_map_60       " },
    { VDPU_REF_INVD_COL_1    , 32, 16, 16, "sw32_ref_invd_col_1    " },
    { VDPU_REF_INVD_COL_0    , 32, 16,  0, "sw32_ref_invd_col_0    " },
    { VDPU_FILT_LEVEL_0      , 32,  6, 18, "sw32_filt_level_0      " },
    { VDPU_FILT_LEVEL_1      , 32,  6, 12, "sw32_filt_level_1      " },
    { VDPU_FILT_LEVEL_2      , 32,  6,  6, "sw32_filt_level_2      " },
    { VDPU_FILT_LEVEL_3      , 32,  6,  0, "sw32_filt_level_3      " },
    { VDPU_REFER7_NBR        , 33, 16, 16, "sw33_refer7_nbr        " },
    { VDPU_REFER6_NBR        , 33, 16,  0, "sw33_refer6_nbr        " },
    { VDPU_SCAN_MAP_61       , 33,  6, 24, "sw33_scan_map_61       " },
    { VDPU_SCAN_MAP_62       , 33,  6, 18, "sw33_scan_map_62       " },
    { VDPU_SCAN_MAP_63       , 33,  6, 12, "sw33_scan_map_63       " },
    { VDPU_REF_INVD_COL_3    , 33, 16, 16, "sw33_ref_invd_col_3    " },
    { VDPU_REF_INVD_COL_2    , 33, 16,  0, "sw33_ref_invd_col_2    " },
    { VDPU_QUANT_DELTA_0     , 33,  5, 27, "sw33_quant_delta_0     " },
    { VDPU_QUANT_DELTA_1     , 33,  5, 22, "sw33_quant_delta_1     " },
    { VDPU_QUANT_0           , 33, 11, 11, "sw33_quant_0           " },
    { VDPU_QUANT_1           , 33, 11,  0, "sw33_quant_1           " },
    { VDPU_REFER9_NBR        , 34, 16, 16, "sw34_refer9_nbr        " },
    { VDPU_REFER8_NBR        , 34, 16,  0, "sw34_refer8_nbr        " },
    { VDPU_PRED_BC_TAP_0_3   , 34, 10, 22, "sw34_pred_bc_tap_0_3   " },
    { VDPU_PRED_BC_TAP_1_0   , 34, 10, 12, "sw34_pred_bc_tap_1_0   " },
    { VDPU_PRED_BC_TAP_1_1   , 34, 10,  2, "sw34_pred_bc_tap_1_1   " },
    { VDPU_REFER11_NBR       , 35, 16, 16, "sw35_refer11_nbr       " },
    { VDPU_REFER10_NBR       , 35, 16,  0, "sw35_refer10_nbr       " },
    { VDPU_PRED_BC_TAP_1_2   , 35, 10, 22, "sw35_pred_bc_tap_1_2   " },
    { VDPU_PRED_BC_TAP_1_3   , 35, 10, 12, "sw35_pred_bc_tap_1_3   " },
    { VDPU_PRED_BC_TAP_2_0   , 35, 10,  2, "sw35_pred_bc_tap_2_0   " },
    { VDPU_REFER13_NBR       , 36, 16, 16, "sw36_refer13_nbr       " },
    { VDPU_REFER12_NBR       , 36, 16,  0, "sw36_refer12_nbr       " },
    { VDPU_PRED_BC_TAP_2_1   , 36, 10, 22, "sw36_pred_bc_tap_2_1   " },
    { VDPU_PRED_BC_TAP_2_2   , 36, 10, 12, "sw36_pred_bc_tap_2_2   " },
    { VDPU_PRED_BC_TAP_2_3   , 36, 10,  2, "sw36_pred_bc_tap_2_3   " },
    { VDPU_REFER15_NBR       , 37, 16, 16, "sw37_refer15_nbr       " },
    { VDPU_REFER14_NBR       , 37, 16,  0, "sw37_refer14_nbr       " },
    { VDPU_PRED_BC_TAP_3_0   , 37, 10, 22, "sw37_pred_bc_tap_3_0   " },
    { VDPU_PRED_BC_TAP_3_1   , 37, 10, 12, "sw37_pred_bc_tap_3_1   " },
    { VDPU_PRED_BC_TAP_3_2   , 37, 10,  2, "sw37_pred_bc_tap_3_2   " },
    { VDPU_REFER_LTERM_E     , 38, 32,  0, "sw38_refer_lterm_e     " },
    { VDPU_PRED_BC_TAP_3_3   , 38, 10, 22, "sw38_pred_bc_tap_3_3   " },
    { VDPU_PRED_BC_TAP_4_0   , 38, 10, 12, "sw38_pred_bc_tap_4_0   " },
    { VDPU_PRED_BC_TAP_4_1   , 38, 10,  2, "sw38_pred_bc_tap_4_1   " },
    { VDPU_REFER_VALID_E     , 39, 32,  0, "sw39_refer_valid_e     " },
    { VDPU_PRED_BC_TAP_4_2   , 39, 10, 22, "sw39_pred_bc_tap_4_2   " },
    { VDPU_PRED_BC_TAP_4_3   , 39, 10, 12, "sw39_pred_bc_tap_4_3   " },
    { VDPU_PRED_BC_TAP_5_0   , 39, 10,  2, "sw39_pred_bc_tap_5_0   " },
    { VDPU_QTABLE_BASE       , 40, 32,  0, "sw40_qtable_base       " },
    { VDPU_DIR_MV_BASE       , 41, 32,  0, "sw41_dir_mv_base       " },
    { VDPU_BINIT_RLIST_B2    , 42,  5, 25, "sw42_binit_rlist_b2    " },
    { VDPU_BINIT_RLIST_F2    , 42,  5, 20, "sw42_binit_rlist_f2    " },
    { VDPU_BINIT_RLIST_B1    , 42,  5, 15, "sw42_binit_rlist_b1    " },
    { VDPU_BINIT_RLIST_F1    , 42,  5, 10, "sw42_binit_rlist_f1    " },
    { VDPU_BINIT_RLIST_B0    , 42,  5,  5, "sw42_binit_rlist_b0    " },
    { VDPU_BINIT_RLIST_F0    , 42,  5,  0, "sw42_binit_rlist_f0    " },
    { VDPU_PRED_BC_TAP_5_1   , 42, 10, 22, "sw42_pred_bc_tap_5_1   " },
    { VDPU_PRED_BC_TAP_5_2   , 42, 10, 12, "sw42_pred_bc_tap_5_2   " },
    { VDPU_PRED_BC_TAP_5_3   , 42, 10,  2, "sw42_pred_bc_tap_5_3   " },
    { VDPU_PJPEG_DCCB_BASE   , 42, 32,  0, "sw42_pjpeg_dccb_base   " },
    { VDPU_BINIT_RLIST_B5    , 43,  5, 25, "sw43_binit_rlist_b5    " },
    { VDPU_BINIT_RLIST_F5    , 43,  5, 20, "sw43_binit_rlist_f5    " },
    { VDPU_BINIT_RLIST_B4    , 43,  5, 15, "sw43_binit_rlist_b4    " },
    { VDPU_BINIT_RLIST_F4    , 43,  5, 10, "sw43_binit_rlist_f4    " },
    { VDPU_BINIT_RLIST_B3    , 43,  5,  5, "sw43_binit_rlist_b3    " },
    { VDPU_BINIT_RLIST_F3    , 43,  5,  0, "sw43_binit_rlist_f3    " },
    { VDPU_PRED_BC_TAP_6_0   , 43, 10, 22, "sw43_pred_bc_tap_6_0   " },
    { VDPU_PRED_BC_TAP_6_1   , 43, 10, 12, "sw43_pred_bc_tap_6_1   " },
    { VDPU_PRED_BC_TAP_6_2   , 43, 10,  2, "sw43_pred_bc_tap_6_2   " },
    { VDPU_PJPEG_DCCR_BASE   , 43, 32,  0, "sw43_pjpeg_dccr_base   " },
    { VDPU_BINIT_RLIST_B8    , 44,  5, 25, "sw44_binit_rlist_b8    " },
    { VDPU_BINIT_RLIST_F8    , 44,  5, 20, "sw44_binit_rlist_f8    " },
    { VDPU_BINIT_RLIST_B7    , 44,  5, 15, "sw44_binit_rlist_b7    " },
    { VDPU_BINIT_RLIST_F7    , 44,  5, 10, "sw44_binit_rlist_f7    " },
    { VDPU_BINIT_RLIST_B6    , 44,  5,  5, "sw44_binit_rlist_b6    " },
    { VDPU_BINIT_RLIST_F6    , 44,  5,  0, "sw44_binit_rlist_f6    " },
    { VDPU_PRED_BC_TAP_6_3   , 44, 10, 22, "sw44_pred_bc_tap_6_3   " },
    { VDPU_PRED_BC_TAP_7_0   , 44, 10, 12, "sw44_pred_bc_tap_7_0   " },
    { VDPU_PRED_BC_TAP_7_1   , 44, 10,  2, "sw44_pred_bc_tap_7_1   " },
    { VDPU_BINIT_RLIST_B11   , 45,  5, 25, "sw45_binit_rlist_b11   " },
    { VDPU_BINIT_RLIST_F11   , 45,  5, 20, "sw45_binit_rlist_f11   " },
    { VDPU_BINIT_RLIST_B10   , 45,  5, 15, "sw45_binit_rlist_b10   " },
    { VDPU_BINIT_RLIST_F10   , 45,  5, 10, "sw45_binit_rlist_f10   " },
    { VDPU_BINIT_RLIST_B9    , 45,  5,  5, "sw45_binit_rlist_b9    " },
    { VDPU_BINIT_RLIST_F9    , 45,  5,  0, "sw45_binit_rlist_f9    " },
    { VDPU_PRED_BC_TAP_7_2   , 45, 10, 22, "sw45_pred_bc_tap_7_2   " },
    { VDPU_PRED_BC_TAP_7_3   , 45, 10, 12, "sw45_pred_bc_tap_7_3   " },
    { VDPU_PRED_TAP_2_M1     , 45,  2, 10, "sw45_pred_tap_2_m1     " },
    { VDPU_PRED_TAP_2_4      , 45,  2,  8, "sw45_pred_tap_2_4      " },
    { VDPU_PRED_TAP_4_M1     , 45,  2,  6, "sw45_pred_tap_4_m1     " },
    { VDPU_PRED_TAP_4_4      , 45,  2,  4, "sw45_pred_tap_4_4      " },
    { VDPU_PRED_TAP_6_M1     , 45,  2,  2, "sw45_pred_tap_6_m1     " },
    { VDPU_PRED_TAP_6_4      , 45,  2,  0, "sw45_pred_tap_6_4      " },
    { VDPU_BINIT_RLIST_B14   , 46,  5, 25, "sw46_binit_rlist_b14   " },
    { VDPU_BINIT_RLIST_F14   , 46,  5, 20, "sw46_binit_rlist_f14   " },
    { VDPU_BINIT_RLIST_B13   , 46,  5, 15, "sw46_binit_rlist_b13   " },
    { VDPU_BINIT_RLIST_F13   , 46,  5, 10, "sw46_binit_rlist_f13   " },
    { VDPU_BINIT_RLIST_B12   , 46,  5,  5, "sw46_binit_rlist_b12   " },
    { VDPU_BINIT_RLIST_F12   , 46,  5,  0, "sw46_binit_rlist_f12   " },
    { VDPU_QUANT_DELTA_2     , 46,  5, 27, "sw46_quant_delta_2     " },
    { VDPU_QUANT_DELTA_3     , 46,  5, 22, "sw46_quant_delta_3     " },
    { VDPU_QUANT_2           , 46, 11, 11, "sw46_quant_2           " },
    { VDPU_QUANT_3           , 46, 11,  0, "sw46_quant_3           " },
    { VDPU_PINIT_RLIST_F3    , 47,  5, 25, "sw47_pinit_rlist_f3    " },
    { VDPU_PINIT_RLIST_F2    , 47,  5, 20, "sw47_pinit_rlist_f2    " },
    { VDPU_PINIT_RLIST_F1    , 47,  5, 15, "sw47_pinit_rlist_f1    " },
    { VDPU_PINIT_RLIST_F0    , 47,  5, 10, "sw47_pinit_rlist_f0    " },
    { VDPU_BINIT_RLIST_B15   , 47,  5,  5, "sw47_binit_rlist_b15   " },
    { VDPU_BINIT_RLIST_F15   , 47,  5,  0, "sw47_binit_rlist_f15   " },
    { VDPU_QUANT_DELTA_4     , 47,  5, 27, "sw47_quant_delta_4     " },
    { VDPU_QUANT_4           , 47, 11, 11, "sw47_quant_4           " },
    { VDPU_QUANT_5           , 47, 11,  0, "sw47_quant_5           " },
    { VDPU_STARTMB_X         , 48,  9, 23, "sw48_startmb_x         " },
    { VDPU_STARTMB_Y         , 48,  8, 15, "sw48_startmb_y         " },
    { VDPU_PRED_BC_TAP_0_0   , 49, 10, 22, "sw49_pred_bc_tap_0_0   " },
    { VDPU_PRED_BC_TAP_0_1   , 49, 10, 12, "sw49_pred_bc_tap_0_1   " },
    { VDPU_PRED_BC_TAP_0_2   , 49, 10,  2, "sw49_pred_bc_tap_0_2   " },
    { VDPU_REFBU_E           , 51,  1, 31, "sw51_refbu_e           " },
    { VDPU_REFBU_THR         , 51, 12, 19, "sw51_refbu_thr         " },
    { VDPU_REFBU_PICID       , 51,  5, 14, "sw51_refbu_picid       " },
    { VDPU_REFBU_EVAL_E      , 51,  1, 13, "sw51_refbu_eval_e      " },
    { VDPU_REFBU_FPARMOD_E   , 51,  1, 12, "sw51_refbu_fparmod_e   " },
    { VDPU_REFBU_Y_OFFSET    , 51,  9,  0, "sw51_refbu_y_offset    " },
    { VDPU_REFBU_HIT_SUM     , 52, 16, 16, "sw52_refbu_hit_sum     " },
    { VDPU_REFBU_INTRA_SUM   , 52, 16,  0, "sw52_refbu_intra_sum   " },
    { VDPU_REFBU_Y_MV_SUM    , 53, 22,  0, "sw53_refbu_y_mv_sum    " },
    { VDPU_REFBU2_BUF_E      , 55,  1, 31, "sw55_refbu2_buf_e      " },
    { VDPU_REFBU2_THR        , 55, 12, 19, "sw55_refbu2_thr        " },
    { VDPU_REFBU2_PICID      , 55,  5, 14, "sw55_refbu2_picid      " },
    { VDPU_APF_THRESHOLD     , 55, 14,  0, "sw55_apf_threshold     " },
    { VDPU_REFBU_TOP_SUM     , 56, 16, 16, "sw56_refbu_top_sum     " },
    { VDPU_REFBU_BOT_SUM     , 56, 16,  0, "sw56_refbu_bot_sum     " },
    { VDPU_DEC_CH8PIX_BASE   , 59, 32,  0, "sw59_dec_ch8pix_base   " },
    { VDPU_MAX_SIZE          , 60,  0,  0, "vdpu_max_size          " },
};


/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_init(void *hal, MppHalCfg *cfg)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = NULL;

    INP_CHECK(ret, NULL == hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    MEM_CHECK(ret, p_hal->regs = mpp_malloc_size(void, sizeof(HalRegDrvCtx_t)));
    p_drv = (HalRegDrvCtx_t *)p_hal->regs;

    p_drv->type     = cfg->type;
    p_drv->coding   = cfg->coding;
    p_drv->syn_size = VDPU_MAX_SIZE;
    p_drv->log      = (void *)p_hal->logctx.parr[LOG_WRITE_REG];

    MEM_CHECK(ret, p_drv->p_syn = mpp_calloc(HalRegDrv_t, p_drv->syn_size));
    for (i = 0; i < MPP_ARRAY_ELEMS(g_vdpu_drv); i++) {
        p_drv->p_syn[g_vdpu_drv[i].syn_id] = g_vdpu_drv[i];
        if (p_drv->syn_size == g_vdpu_drv[i].syn_id) {
            break;
        }
    }
    p_drv->reg_size = p_drv->p_syn[p_drv->syn_size].reg_id;
    MEM_CHECK(ret, p_drv->p_reg = mpp_calloc(RK_U32, p_drv->reg_size));


    //p_hal->pkts = mpp_calloc(H264dRkvPkt_t, 1);
    //MEM_CHECK(ret, p_hal->regs && p_hal->pkts);
    //FUN_CHECK(ret = alloc_fifo_packet(&p_hal->logctx, p_hal->pkts));

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)cfg;
__RETURN:
    return MPP_OK;
__FAILED:
    vdpu_h264d_deinit(hal);

    return ret;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_deinit(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    if (p_hal->regs) {
        MPP_FREE(((HalRegDrvCtx_t *)p_hal->regs)->p_syn);
        MPP_FREE(((HalRegDrvCtx_t *)p_hal->regs)->p_reg);
        MPP_FREE(p_hal->regs);
    }
    //free_fifo_packet(p_hal->pkts);

    //MPP_FREE(p_hal->pkts);
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    //H264dRkvPkt_t *pkts  = (H264dRkvPkt_t *)p_hal->pkts;
    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    //explain_input_buffer(hal, &task->dec);
    //prepare_spspps_packet(hal, &pkts->spspps);
    //prepare_framerps_packet(hal, &pkts->rps);
    //prepare_scanlist_packet(hal, &pkts->scanlist);
    //prepare_stream_packet(hal, &pkts->strm);
    //generate_regs(p_hal, &pkts->reg);

    mpp_log("++++++++++ hal_h264_decoder, g_framecnt=%d \n", p_hal->g_framecnt++);
    ((HalDecTask*)&task->dec)->valid = 0;
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);

__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)task;
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)task;
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    //memset(&p_hal->regs, 0, sizeof(H264_RkvRegs_t));
    //reset_fifo_packet(p_hal->pkts);

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)hal;
    (void)cmd_type;
    (void)param;
__RETURN:
    return ret = MPP_OK;
}