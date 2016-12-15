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


#ifndef __HAL_H264D_RKV_REG_H__
#define __HAL_H264D_RKV_REG_H__

#include "mpp_hal.h"

typedef struct h264d_rkv_regs_t {
    struct {
        RK_U32    minor_ver : 8;
        RK_U32    level : 1;
        RK_U32    dec_support : 3;
        RK_U32    profile : 1;
        RK_U32    reserve0 : 1;
        RK_U32    codec_flag : 1;
        RK_U32    reserve1 : 1;
        RK_U32    prod_num : 16;
    } sw00;
    struct {
        RK_U32    dec_e : 1;//0
        RK_U32    dec_clkgate_e : 1; // 1
        RK_U32    reserve0 : 1;// 2
        RK_U32    timeout_mode : 1; // 3
        RK_U32    dec_irq_dis : 1;//4    // 4
        RK_U32    dec_timeout_e : 1; //5
        RK_U32    buf_empty_en : 1; // 6
        RK_U32    stmerror_waitdecfifo_empty : 1; // 7
        RK_U32    dec_irq : 1; // 8
        RK_U32    dec_irq_raw : 1; // 9
        RK_U32    reserve2 : 2;
        RK_U32    dec_rdy_sta : 1; //12
        RK_U32    dec_bus_sta : 1; //13
        RK_U32    dec_error_sta : 1; // 14
        RK_U32    dec_timeout_sta : 1; //15
        RK_U32    dec_empty_sta : 1; // 16
        RK_U32    colmv_ref_error_sta : 1; // 17
        RK_U32    cabu_end_sta : 1; // 18
        RK_U32    h264orvp9_error_mode : 1; //19
        RK_U32    softrst_en_p : 1; //20
        RK_U32    force_softreset_valid : 1; //21
        RK_U32    softreset_rdy : 1; // 22
        RK_U32    reserve1 : 9;
    } sw01;
    struct {
        RK_U32    in_endian : 1;
        RK_U32    in_swap32_e : 1;
        RK_U32    in_swap64_e : 1;
        RK_U32    str_endian : 1;
        RK_U32    str_swap32_e : 1;
        RK_U32    str_swap64_e : 1;
        RK_U32    out_endian : 1;
        RK_U32    out_swap32_e : 1;
        RK_U32    out_cbcr_swap : 1;
        RK_U32    reserve0 : 1;
        RK_U32    rlc_mode_direct_write : 1;
        RK_U32    rlc_mode : 1;
        RK_U32    strm_start_bit : 7;
        RK_U32    reserve1 : 1;
        RK_U32    dec_mode : 2;
        RK_U32    reserve2 : 2;
        RK_U32    rps_mode : 1;
        RK_U32    stream_mode : 1;
        RK_U32    stream_lastpacket : 1;
        RK_U32    firstslice_flag : 1;
        RK_U32    frame_orslice : 1;
        RK_U32    buspr_slot_disable : 1;
        RK_U32    reverse3 : 2;
    } sw02;
    struct {
        RK_U32    y_hor_virstride : 9;
        RK_U32    reserve : 2;
        RK_U32    slice_num_highbit : 1;
        RK_U32    uv_hor_virstride : 9;
        RK_U32    slice_num_lowbits : 11;
    } sw03;
    struct {
        RK_U32    strm_rlc_base : 32;
    } sw04;
    struct {
        RK_U32    stream_len : 27;
        RK_U32    reverse0 : 5;
    } sw05;
    struct {
        RK_U32    cabactbl_base : 32;
    } sw06;
    struct {
        RK_U32    decout_base : 32;
    } sw07;
    struct {
        RK_U32    y_virstride : 20;
        RK_U32    reverse0 : 12;
    } sw08;
    struct {
        RK_U32    yuv_virstride : 21;
        RK_U32    reverse0 : 11;
    } sw09;
    struct {
        RK_U32    ref0_14_base : 10;
        RK_U32    ref0_14_field : 1;
        RK_U32    ref0_14_topfield_used : 1;
        RK_U32    ref0_14_botfield_used : 1;
        RK_U32    ref0_14_colmv_use_flag : 1;
    } sw10_24[15];
    struct {
        RK_U32    ref0_14_poc : 32;
    } sw25_39[15];
    struct {
        RK_U32    cur_poc : 32;
    } sw40;
    struct {
        RK_U32    rlcwrite_base;
    } sw41;
    struct {
        RK_U32    pps_base;
    } sw42;
    struct {
        RK_U32    rps_base;
    } sw43;
    struct {
        RK_U32    strmd_error_e : 28;
        RK_U32    reserve : 4;
    } sw44;
    struct {
        RK_U32    strmd_error_status : 28;
        RK_U32    colmv_error_ref_picidx : 4;
    } sw45;
    struct {
        RK_U32    strmd_error_ctu_xoffset : 8;
        RK_U32    strmd_error_ctu_yoffset : 8;
        RK_U32    streamfifo_space2full : 7;
        RK_U32    reserve0 : 1;
        RK_U32    vp9_error_ctu0_en : 1;
        RK_U32    reverse1 : 7;
    } sw46;
    struct {
        RK_U32    saowr_xoffet : 9;
        RK_U32    reserve0 : 7;
        RK_U32    saowr_yoffset : 10;
        RK_U32    reverse1 : 6;
    } sw47;
    struct {
        RK_U32    ref15_base : 10;
        RK_U32    ref15_field : 1;
        RK_U32    ref15_topfield_used : 1;
        RK_U32    ref15_botfield_used : 1;
        RK_U32    ref15_colmv_use_flag : 1;
        RK_U32    reverse0 : 18;
    } sw48;
    struct {
        RK_U32    ref15_29_poc : 32;
    } sw49_63[15];
    struct {
        RK_U32    performance_cycle : 32;
    } sw64;
    struct {
        RK_U32    axi_ddr_rdata : 32;
    } sw65;
    struct {
        RK_U32    axi_ddr_rdata : 32;
    } sw66;
    struct {
        RK_U32    busifd_resetn : 1;
        RK_U32    cabac_resetn : 1;
        RK_U32    dec_ctrl_resetn : 1;
        RK_U32    transd_resetn : 1;
        RK_U32    intra_resetn : 1;
        RK_U32    inter_resetn : 1;
        RK_U32    recon_resetn : 1;
        RK_U32    filer_resetn : 1;
        RK_U32    reverse0 : 24;
    } sw67;
    struct {
        RK_U32    perf_cnt0_sel : 6;
        RK_U32    reserve0 : 2;
        RK_U32    perf_cnt1_sel : 6;
        RK_U32    reserve1 : 2;
        RK_U32    perf_cnt2_sel : 6;
        RK_U32    reverse1 : 10;
    } sw68;
    struct {
        RK_U32    perf_cnt0 : 32;
    } sw69;
    struct {
        RK_U32    perf_cnt1 : 32;
    } sw70;
    struct {
        RK_U32    perf_cnt2 : 32;
    } sw71;
    struct {
        RK_U32    ref30_poc;
    } sw72;
    struct {
        RK_U32    ref31_poc;
    } sw73;
    struct {
        RK_U32    cur_poc1 : 32;
    } sw74;
    struct {
        RK_U32    errorinfo_base : 32;
    } sw75;
    struct {
        RK_U32    slicedec_num : 14;
        RK_U32    reserve0 : 1;
        RK_U32    strmd_detect_error_flag : 1;
        RK_U32    error_packet_num : 14;
        RK_U32    reverse1 : 2;
    } sw76;
    struct {
        RK_U32    error_en_highbits : 30;
        RK_U32    reserve : 2;
    } sw77;
    RK_U32        reverse[2];
} H264dRkvRegs_t;


#ifdef __cplusplus
extern "C" {
#endif

MPP_RET rkv_h264d_init    (void *hal, MppHalCfg *cfg);
MPP_RET rkv_h264d_deinit  (void *hal);
MPP_RET rkv_h264d_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET rkv_h264d_start   (void *hal, HalTaskInfo *task);
MPP_RET rkv_h264d_wait    (void *hal, HalTaskInfo *task);
MPP_RET rkv_h264d_reset   (void *hal);
MPP_RET rkv_h264d_flush   (void *hal);
MPP_RET rkv_h264d_control (void *hal, RK_S32 cmd_type, void *param);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_H264D_RKV_REG_H__ */
