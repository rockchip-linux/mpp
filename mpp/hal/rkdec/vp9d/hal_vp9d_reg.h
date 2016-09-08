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

/*
 * @file       hal_vp9_reg.h
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#ifndef __HAL_VP9D_REG_H__
#define __HAL_VP9D_REG_H__
#include "rk_type.h"
#include "mpp_log.h"

extern RK_U32 vp9h_debug;
#define VP9H_DBG_FUNCTION          (0x00000001)
#define VP9H_DBG_PAR               (0x00000002)
#define VP9H_DBG_REG               (0x00000004)


#define vp9h_dbg(flag, fmt, ...) _mpp_dbg(vp9h_debug, flag, fmt, ## __VA_ARGS__)

typedef struct {
    struct {
        RK_U32   minor_ver   : 8;
        RK_U32    level       : 1;
        RK_U32    dec_support : 3;
        RK_U32    profile     : 1;
        RK_U32    reserve0    : 1;
        RK_U32    codec_flag  : 1;
        RK_U32    reserve1    : 1;
        RK_U32    prod_num    : 16;
    } swreg0_id;

    struct {
        RK_U32    sw_dec_e                        : 1;//0
        RK_U32    sw_dec_clkgate_e                : 1; // 1
        RK_U32    reserve0                        : 1;// 2
        RK_U32    sw_timeout_mode                 : 1; // 3
        RK_U32    sw_dec_irq_dis                  : 1;//4    // 4
        RK_U32    sw_dec_timeout_e                : 1; //5
        RK_U32    sw_buf_empty_en                 : 1; // 6
        RK_U32    sw_stmerror_waitdecfifo_empty   : 1; // 7
        RK_U32    sw_dec_irq                      : 1; // 8
        RK_U32    sw_dec_irq_raw                  : 1; // 9
        RK_U32    reserve2                        : 2;
        RK_U32    sw_dec_rdy_sta                  : 1; //12
        RK_U32    sw_dec_bus_sta                  : 1; //13
        RK_U32    sw_dec_error_sta                : 1; // 14
        RK_U32    sw_dec_timeout_sta              : 1; //15
        RK_U32    sw_dec_empty_sta                : 1; // 16
        RK_U32    sw_colmv_ref_error_sta          : 1; // 17
        RK_U32    sw_cabu_end_sta                 : 1; // 18
        RK_U32    sw_h264orvp9_error_mode         : 1; //19
        RK_U32    sw_softrst_en_p                 : 1; //20
        RK_U32    sw_force_softreset_valid        : 1; //21
        RK_U32    sw_softreset_rdy                : 1; // 22
    } swreg1_int;

    struct {
        RK_U32    sw_in_endian                    : 1;
        RK_U32    sw_in_swap32_e                  : 1;
        RK_U32    sw_in_swap64_e                  : 1;
        RK_U32    sw_str_endian                   : 1;
        RK_U32    sw_str_swap32_e                 : 1;
        RK_U32    sw_str_swap64_e                 : 1;
        RK_U32    sw_out_endian                   : 1;
        RK_U32    sw_out_swap32_e                 : 1;
        RK_U32    sw_out_cbcr_swap                : 1;
        RK_U32    reserve0                        : 1;
        RK_U32    sw_rlc_mode_direct_write        : 1;
        RK_U32    sw_rlc_mode                     : 1;
        RK_U32    sw_strm_start_bit               : 7;
        RK_U32    reserve1                        : 1;
        RK_U32    sw_dec_mode                     : 2;
        RK_U32    reserve2                        : 2;
        RK_U32    sw_h264_rps_mode                : 1;
        RK_U32    sw_h264_stream_mode             : 1;
        RK_U32    sw_h264_stream_lastpacket       : 1;
        RK_U32    sw_h264_firstslice_flag         : 1;
        RK_U32    sw_h264_frame_orslice           : 1;
        RK_U32    sw_buspr_slot_disable           : 1;
        RK_U32  reserve3                        : 2;
    } swreg2_sysctrl;

    struct {
        RK_U32    sw_y_hor_virstride              : 9;
        RK_U32    reserve                         : 2;
        RK_U32    sw_slice_num_highbit            : 1;
        RK_U32    sw_uv_hor_virstride             : 9;
        RK_U32    sw_slice_num_lowbits            : 11;
    } swreg3_picpar;

    RK_U32 swreg4_strm_rlc_base;
    RK_U32 swreg5_stream_len;
    RK_U32 swreg6_cabactbl_prob_base;
    RK_U32 swreg7_decout_base;

    struct {
        RK_U32    sw_y_virstride                  : 20;
        RK_U32    reverse0                        : 12;
    } swreg8_y_virstride;

    struct {
        RK_U32    sw_yuv_virstride                : 21;
        RK_U32    reverse                         : 11;
    } swreg9_yuv_virstride;


    //only for vp9
    struct {
        RK_U32    sw_vp9_cprheader_offset         : 16;
        RK_U32    reverse                         : 16;
    } swreg10_vp9_cprheader_offset;

    RK_U32 swreg11_vp9_referlast_base;
    RK_U32 swreg12_vp9_refergolden_base;
    RK_U32 swreg13_vp9_referalfter_base;
    RK_U32 swreg14_vp9_count_base;
    RK_U32 swreg15_vp9_segidlast_base;
    RK_U32 swreg16_vp9_segidcur_base;

    struct {
        RK_U32    sw_framewidth_last              : 16;
        RK_U32    sw_frameheight_last             : 16;
    } swreg17_vp9_frame_size_last;

    struct {
        RK_U32    sw_framewidth_golden            : 16;
        RK_U32    sw_frameheight_golden           : 16;
    } swreg18_vp9_frame_size_golden;


    struct {
        RK_U32    sw_framewidth_alfter            : 16;
        RK_U32    sw_frameheight_alfter           : 16;
    } swreg19_vp9_frame_size_altref;


    struct {
        RK_U32    sw_vp9segid_abs_delta                      : 1; //NOTE: only in reg#20, this bit is valid.
        RK_U32    sw_vp9segid_frame_qp_delta_en              : 1;
        RK_U32    sw_vp9segid_frame_qp_delta                 : 9;
        RK_U32    sw_vp9segid_frame_loopfitler_value_en      : 1;
        RK_U32    sw_vp9segid_frame_loopfilter_value         : 7;
        RK_U32    sw_vp9segid_referinfo_en                   : 1;
        RK_U32    sw_vp9segid_referinfo                      : 2;
        RK_U32    sw_vp9segid_frame_skip_en                  : 1;
        RK_U32    reverse                                    : 9;
    } swreg20_27_vp9_segid_grp[8];


    struct {
        RK_U32    sw_vp9_tx_mode                              : 3;
        RK_U32    sw_vp9_frame_reference_mode                 : 2;
        RK_U32    reserved                                    : 27;
    } swreg28_vp9_cprheader_config;


    struct {
        RK_U32    sw_vp9_lref_hor_scale                       : 16;
        RK_U32    sw_vp9_lref_ver_scale                       : 16;
    } swreg29_vp9_lref_scale;

    struct {
        RK_U32    sw_vp9_gref_hor_scale                       : 16;
        RK_U32    sw_vp9_gref_ver_scale                       : 16;
    } swreg30_vp9_gref_scale;

    struct {
        RK_U32    sw_vp9_aref_hor_scale                       : 16;
        RK_U32    sw_vp9_aref_ver_scale                       : 16;
    } swreg31_vp9_aref_scale;

    struct {
        RK_U32    sw_vp9_ref_deltas_lastframe                 : 28;
        RK_U32    reserve                                     : 4;
    } swreg32_vp9_ref_deltas_lastframe;

    struct {
        RK_U32    sw_vp9_mode_deltas_lastframe                : 14;
        RK_U32    reserve0                                    : 2;
        RK_U32    sw_segmentation_enable_lstframe             : 1;
        RK_U32    sw_vp9_last_show_frame                      : 1;
        RK_U32    sw_vp9_last_intra_only                      : 1;
        RK_U32    sw_vp9_last_widthheight_eqcur               : 1;
        RK_U32    sw_vp9_color_space_lastkeyframe             : 3;
        RK_U32    reserve1                                    : 9;
    } swreg33_vp9_info_lastframe;

    RK_U32 swreg34_vp9_intercmd_base;

    struct {
        RK_U32    sw_vp9_intercmd_num                         : 24;
        RK_U32    reserve                                     : 8;
    } swreg35_vp9_intercmd_num;

    struct {
        RK_U32    sw_vp9_lasttile_size                        : 24;
        RK_U32    reserve                                     : 8;
    } swreg36_vp9_lasttile_size;

    struct {
        RK_U32    sw_vp9_lastfy_hor_virstride                 : 9;
        RK_U32    reserve0                                    : 7;
        RK_U32    sw_vp9_lastfuv_hor_virstride                : 9;
        RK_U32    reserve1                                    : 7;
    } swreg37_vp9_lastf_hor_virstride;

    struct {
        RK_U32    sw_vp9_goldenfy_hor_virstride               : 9;
        RK_U32    reserve0                                    : 7;
        RK_U32    sw_vp9_goldenuv_hor_virstride               : 9;
        RK_U32    reserve1                                    : 7;
    } swreg38_vp9_goldenf_hor_virstride;

    struct {
        RK_U32    sw_vp9_altreffy_hor_virstride               : 9;
        RK_U32    reserve0                                    : 7;
        RK_U32    sw_vp9_altreffuv_hor_virstride              : 9;
        RK_U32    reserve1                                    : 7;
    } swreg39_vp9_altreff_hor_virstride;

    struct {
        RK_U32 sw_cur_poc                                     : 32;
    } swreg40_cur_poc;

    struct {
        RK_U32 reserve                                        : 3;
        RK_U32 sw_rlcwrite_base                               : 29;
    } swreg41_rlcwrite_base;

    struct {
        RK_U32 reserve                                        : 4;
        RK_U32 sw_pps_base                                    : 28;
    } swreg42_pps_base;

    struct {
        RK_U32 reserve                                        : 4;
        RK_U32 sw_rps_base                                    : 28;
    } swreg43_rps_base;

    struct {
        RK_U32 sw_strmd_error_e                               : 28;
        RK_U32 reserve                                        : 4;
    } swreg44_strmd_error_en;

    struct {
        RK_U32 vp9_error_info0                                : 32;
    } swreg45_vp9_error_info0;

    struct {
        RK_U32 sw_strmd_error_ctu_xoffset                     : 8;
        RK_U32 sw_strmd_error_ctu_yoffset                     : 8;
        RK_U32 sw_streamfifo_space2full                       : 7;
        RK_U32 reserve                                        : 1;
        RK_U32 sw_vp9_error_ctu0_en                           : 1;
    } swreg46_strmd_error_ctu;

    struct {
        RK_U32 sw_saowr_xoffet                                : 9;
        RK_U32 reserve                                        : 7;
        RK_U32 sw_saowr_yoffset                               : 10;
    } swreg47_sao_ctu_position;

    struct {
        RK_U32 sw_vp9_lastfy_virstride                        : 20;
        RK_U32 reserve                                        : 12;
    } swreg48_vp9_last_ystride;

    struct {
        RK_U32 sw_vp9_goldeny_virstride                       : 20;
        RK_U32 reserve                                        : 12;
    } swreg49_vp9_golden_ystride;

    struct {
        RK_U32 sw_vp9_altrefy_virstride                       : 20;
        RK_U32 reserve                                        : 12;
    } swreg50_vp9_altrefy_ystride;

    struct {
        RK_U32 sw_vp9_lastref_yuv_virstride                   : 21;
        RK_U32 reserve                                        : 11;
    } swreg51_vp9_lastref_yuvstride;


    RK_U32 swreg52_vp9_refcolmv_base;

    RK_U32 reg_not_use0[64 - 52 - 1];

    struct {
        RK_U32 sw_performance_cycle                           : 32;
    } swreg64_performance_cycle;

    struct {
        RK_U32 sw_axi_ddr_rdata                               : 32;
    } swreg65_axi_ddr_rdata;

    struct {
        RK_U32 sw_axi_ddr_wdata                               : 32;
    } swreg66_axi_ddr_wdata;

    struct {
        RK_U32 sw_busifd_resetn                               : 1;
        RK_U32 sw_cabac_resetn                                : 1;
        RK_U32 sw_dec_ctrl_resetn                             : 1;
        RK_U32 sw_transd_resetn                               : 1;
        RK_U32 sw_intra_resetn                                : 1;
        RK_U32 sw_inter_resetn                                : 1;
        RK_U32 sw_recon_resetn                                : 1;
        RK_U32 sw_filer_resetn                                : 1;
    } swreg67_fpgadebug_reset;

    struct {
        RK_U32 perf_cnt0_sel  : 6;
        RK_U32 reserve0       : 2;
        RK_U32 perf_cnt1_sel  : 6;
        RK_U32 reserve1       : 2;
        RK_U32 perf_cnt2_sel  : 6;
    } swreg68_performance_sel;

    struct {
        RK_U32 perf_cnt0 : 32;
    } swreg69_performance_cnt0;

    struct {
        RK_U32 perf_cnt1 : 32;
    } swreg70_performance_cnt1;

    struct {
        RK_U32 perf_cnt2 : 32;
    } swreg71_performance_cnt2;

    RK_U32 reg_not_use1[74 - 71 - 1];

    struct {
        RK_U32 sw_h264_cur_poc1 : 32;
    } swreg74_h264_cur_poc1;

    struct {
        RK_U32 vp9_error_info1 : 32;
    } swreg75_vp9_error_info1;

    struct {
        RK_U32 vp9_error_ctu1_x       : 6;
        RK_U32 reserve0               : 2;
        RK_U32 vp9_error_ctu1_y       : 6;
        RK_U32 reserve1               : 1;
        RK_U32 vp9_error_ctu1_en      : 1;
        RK_U32 reserve2               : 16;
    } swreg76_vp9_error_ctu1;
} VP9_REGS;
#endif
