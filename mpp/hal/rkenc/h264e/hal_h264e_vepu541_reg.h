/*
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

#ifndef __HAL_H264E_VEPU541_REG_H__
#define __HAL_H264E_VEPU541_REG_H__

#include "vepu541_common.h"

typedef struct Vepu541H264eRegSet_t {
    /*
     * VERSION
     * Address: 0x0000 Access type: read only
     * VEPU version. It contains IP function summary and sub-version informations.
     */
    struct {
        /* Sub-version(version 1.1) */
        RK_U32  sub_ver                 : 8;
        /* Support H.264 encoding */
        RK_U32  h264_enc                : 1;
        /* Support HEVC encoding */
        RK_U32  h265_enc                : 1;
        RK_U32  reserved0               : 2;
        /*
         * The maximum resolution supported
         * 4'd0: 4096x2304 pixels;
         * 4'd1: 1920x1088 pixels;
         * others: reserved
         */
        RK_U32  pic_size                : 4;
        /*
         * OSD capability.
         * 2'd0: 8-area OSD, with 256-color palette
         * 2'd3: no OSD
         * others: reserved
         */
        RK_U32  osd_cap                 : 2;
        /*
         * pre-process filter capability
         * 2'd0: basic pre-process filter
         * 2'd3: no pre-process filter
         * others: reserved
         */
        RK_U32  filtr_cap               : 2;
        /* B frame encoding capability */
        RK_U32  bfrm_cap                : 1;
        /* frame buffer compress capability */
        RK_U32  fbc_cap                 : 1;
        RK_U32  reserved1               : 2;
        /* IP indentifier for RKVENC default: 0x50 */
        RK_U32  rkvenc_ver              : 8;
    } reg000;

    /*
     * ENC_STRT
     * Address: 0x0004 Access type: read and write/write only
     * Start cmd register.(auto clock gating enable, auto reset enable and
     * tmvp adjust enable when frame done are also allocated here.)
     */
    struct {
        /*
         * Number of new nodes in link table.
         * It's valid only when rkvenc_cmd is 2 or 3.
         */
        RK_U32  lkt_num                 : 8;
        /*
         * Rockchip video encoder command:
         * 2'd0: N/A
         * 2'd1: one frame encode by register configuration
         * 2'd2: multi-frame encode start with link table
         * 2'd3: multi_frame_encode link table update
         */
        RK_U32  rkvenc_cmd              : 2;
        RK_U32  reserved0               : 6;
        /* RKVENC encoder clock gating enable */
        RK_U32  clk_gate_en             : 1;
        /* auto reset core clock domain when frame finished */
        RK_U32  resetn_hw_en            : 1;
        /* wait tmvp write done by dma */
        RK_U32  enc_done_tmvp_en        : 1;
        RK_U32  reserved1               : 13;
    } reg001;

    /*
     * ENC_CLR
     * Address offset: 0x0008 Access type: read and write
     * ENC_CLR.safe_clr only clears RKVENC DMA and confirms the integrity of
     * AXI transactions. To execute the global reset of RKVENC, user needs to
     * configure SOC CRU register which controls RKVENC's asynchronous reset
     */
    struct {
        /*
         * Safe clear. This filed only clears DMA module to confirm the
         * integrity of AXI transactions
         */
        RK_U32  safe_clr                : 1;
        /*
         * Force clear. Clear all the sub modules besides regfile and AHB data
         * path.
         */
        RK_U32  force_clr               : 1;
        RK_U32  reserved                : 30;
    } reg002;

    /*
     * LKT_ADDR
     * Address offset: 0x000c Access type: read and write
     * Link table
     */
    struct {
        /*
         * High 28 bits of the address for the first node in current link table
         * (16bytes aligned)
         */
        RK_U32  lkt_addr                : 32;
    } reg003;

    /*
     * INT_EN
     * Address offset: 0x0010 Access type: read and write
     * VEPU interrupt enable
     */
    struct {
        /* One frame encode finish interrupt enable */
        RK_U32  enc_done_en             : 1;
        /* Link table finish interrupt enable */
        RK_U32  lkt_done_en             : 1;
        /* Safe clear finish interrupt enable */
        RK_U32  sclr_done_en            : 1;
        /* Safe clear finish interrupt enable */
        RK_U32  enc_slice_done_en       : 1;
        /* Bit stream overflow interrupt enable */
        RK_U32  oflw_done_en            : 1;
        /* AXI write response fifo full interrupt enable */
        RK_U32  brsp_done_en            : 1;
        /* AXI write response channel error interrupt enable */
        RK_U32  berr_done_en            : 1;
        /* AXI read channel error interrupt enable */
        RK_U32  rerr_done_en            : 1;
        /* timeout error interrupt enable */
        RK_U32  wdg_done_en             : 1;
        RK_U32  reserved                : 23;
    } reg004;

    /*
     * INT_MSK
     * Address offset: 0x0014 Access type: read and write
     * VEPU interrupt mask
     */
    struct {
        /* One frame encode finish interrupt mask */
        RK_U32  enc_done_msk            : 1;
        /* Link table finish interrupt mask */
        RK_U32  lkt_done_msk            : 1;
        /* Safe clear finish interrupt mask */
        RK_U32  sclr_done_msk           : 1;
        /* Safe clear finish interrupt mask */
        RK_U32  enc_slice_done_msk      : 1;
        /* Bit stream overflow interrupt mask */
        RK_U32  oflw_done_msk           : 1;
        /* AXI write response fifo full interrupt mask */
        RK_U32  brsp_done_msk           : 1;
        /* AXI write response channel error interrupt mask */
        RK_U32  berr_done_msk           : 1;
        /* AXI read channel error interrupt mask */
        RK_U32  rerr_done_msk           : 1;
        /* timeout error interrupt mask */
        RK_U32  wdg_done_msk            : 1;
        RK_U32  reserved                : 23;
    } reg005;

    /*
     * INT_CLR
     * Address offset: 0x0018 Access type: read and write, write one to clear
     * VEPU interrupt clear
     */
    struct {
        /* One frame encode finish interrupt clear */
        RK_U32  enc_done_clr            : 1;
        /* Link table finish interrupt clear */
        RK_U32  lkt_done_clr            : 1;
        /* Safe clear finish interrupt clear */
        RK_U32  sclr_done_clr           : 1;
        /* One slice encode finish interrupt clear */
        RK_U32  enc_slice_done_clr      : 1;
        /* Bit stream overflow interrupt clear */
        RK_U32  oflw_done_clr           : 1;
        /* AXI write response fifo full interrupt clear */
        RK_U32  brsp_done_clr           : 1;
        /* AXI write response channel error interrupt clear */
        RK_U32  berr_done_clr           : 1;
        /* AXI read channel error interrupt clear */
        RK_U32  rerr_done_clr           : 1;
        /* timeout error interrupt clear */
        RK_U32  wdg_done_clr            : 1;
        RK_U32  reserved                : 23;
    } reg006;

    /*
     * INT_STA
     * Address offset: 0x001c Access type: read and write, write one to clear
     * VEPU interrupt status
     */
    struct {
        /* One frame encode finish interrupt status */
        RK_U32  enc_done_sta            : 1;
        /* Link table finish interrupt status */
        RK_U32  lkt_done_sta            : 1;
        /* Safe clear finish interrupt status */
        RK_U32  sclr_done_sta           : 1;
        /* One slice encode finish interrupt status */
        RK_U32  enc_slice_done_sta      : 1;
        /* Bit stream overflow interrupt status */
        RK_U32  oflw_done_sta           : 1;
        /* AXI write response fifo full interrupt status */
        RK_U32  brsp_done_sta           : 1;
        /* AXI write response channel error interrupt status */
        RK_U32  berr_done_sta           : 1;
        /* AXI read channel error interrupt status */
        RK_U32  rerr_done_sta           : 1;
        /* timeout error interrupt status */
        RK_U32  wdg_done_sta            : 1;
        RK_U32  reserved                : 23;
    } reg007;

    /* reg gap 008~011 */
    RK_U32 reg_008_011[4];

    /*
     * ENC_RSL
     * Address offset: 0x0030 Access type: read and write
     * Resolution
     */
    struct {
        /* ceil(picture width/8) - 1 */
        RK_U32  pic_wd8_m1              : 9;
        RK_U32  reserved0               : 1;
        /* filling pixels to maintain picture width 8 pixels aligned */
        RK_U32  pic_wfill               : 6;
        /* Ceil(picture_height/8)-1 */
        RK_U32  pic_hd8_m1              : 9;
        RK_U32  reserved1               : 1;
        /* Filling pixels to maintain picture height 8 pixels aligned */
        RK_U32  pic_hfill               : 6;
    } reg012;

    /*
     * ENC_PIC
     * Address offset: 0x0034 Access type: read and write
     * VEPU common configuration
     */
    struct {
        /* Video standard: 0->H.264; 1->HEVC */
        RK_U32  enc_stnd                : 1;
        /* ROI encode enable */
        RK_U32  roi_enc                 : 1;
        /* Current frame should be refered in future */
        RK_U32  cur_frm_ref             : 1;
        /* Output ME information */
        RK_U32  mei_stor                : 1;
        /* Output start code prefix */
        RK_U32  bs_scp                  : 1;
        /* 0: select table A, 1: select table B */
        RK_U32  lamb_mod_sel            : 1;
        RK_U32  reserved0               : 2;
        /* QP value for current frame encoding */
        RK_U32  pic_qp                  : 6;
        /* sum of reference pictures (indexed by difference POCs), HEVC only */
        RK_U32  tot_poc_num             : 5;
        /* bit width to express the maximum ctu number in current picure, HEVC only */
        RK_U32  log2_ctu_num            : 4;
        /* 1'h0: Select atr_thd group 1'h1: Select atr_thd group1 */
        RK_U32  atr_thd_sel             : 1;
        /* Dual-core handshake Rx ID. */
        RK_U32  dchs_rxid               : 2;
        /* Dual-core handshake tx ID. */
        RK_U32  dchs_txid               : 2;
        /* Dual-core handshake rx enable. */
        RK_U32  dchs_rxe                : 1;
        /* RDO intra-prediction satd path bypass enable. */
        RK_U32  satd_byps_en            : 1;
        /* Slice length fifo enable. */
        RK_U32  slen_fifo               : 1;
        /* Node interrupt enable (only for link table node configuration). */
        RK_U32  node_int                : 1;
    } reg013;

    /*
     * ENC_WDG
     * Address offset: 0x0038 Access type: read and write
     * VEPU watch dog configure register
     */
    struct {
        /*
         * Video source loading timeout threshold.
         * 24'h0: No time limit
         * 24'hx: x*256 core clock cycles
         */
        RK_U32  vs_load_thd             : 24;
        /*
         * Reference picture loading timeout threshold.
         * 8'h0: No time limit
         * 8'hx: x*256 core clock cycles
         */
        RK_U32  rfp_load_thrd           : 8;
    } reg014;

    /*
     * DTRNS_MAP
     * Address offset: 0x003c Access type: read and write
     * Data transaction mapping (endian and order)
     */
    struct {
        /* swap the position of 64bits in 128bits for lpf write data between tiles */
        RK_U32  lpfw_bus_ordr           : 1;
        /* Swap the position of 64 bits in 128 bits for co-located Mv(HEVC only). */
        RK_U32  cmvw_bus_ordr           : 1;
        /* Swap the position of 64 bits in 128 bits for down-sampled picture. */
        RK_U32  dspw_bus_ordr           : 1;
        /* Swap the position of 64 bits in 128 bits for reference picture. */
        RK_U32  rfpw_bus_ordr           : 1;
        /*
         * Data swap for video source loading channel.
         * [3]: Swap 64 bits in 128 bits
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32  src_bus_edin            : 4;
        /*
         * Data swap for ME information write channel.
         * [3]: Swap 64 bits in 128 bits
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32  meiw_bus_edin           : 4;
        /*
         * Data swap for bis stream write channel.
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32  bsw_bus_edin            : 3;
        /*
         * Data swap for link table read channel.
         * [3]: Swap 64 bits in 128 bits
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32  lktr_bus_edin           : 4;
        /*
         * Data swap for ROI configuration read channel.
         * [3]: Swap 64 bits in 128 bits
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32  roir_bus_edin           : 4;
        /*
         * Data swap for link table write channel.
         * [3]: Swap 64 bits in 128 bits
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32  lktw_bus_edin           : 4;
        /*
         * AFBC video source loading burst size.
         * 1'h0: 32 bytes
         * 1'h1: 64 bytes
         */
        RK_U32  afbc_bsize              : 1;
        RK_U32  reserved1               : 4;
    } reg015;

    /*
     * DTRNS_CFG
     * Address offset: 0x0040 Access type: read and write
     * (AXI bus) Data transaction configuration
     */
    union {
        struct vepu541_t {
            /*
             * AXI write response channel check enable.
             * [6]: Reconstructed picture write response check enable.
             * [5]: ME information write response check enable.
             * [4]: CTU information write response check enable.
             * [3]: Down-sampled picture write response check enable.
             * [2]: Bit stream write response check enable.
             * [1]: Link table mode write reponse check enable.
             * [0]: Reserved for video preprocess.
             */
            RK_U32  axi_brsp_cke            : 7;
            /*
             * Down sampled reference picture read outstanding enable.
             * 1'h0: No outstanding
             * 1'h1: Outstanding read, which improves data transaction efficiency,
             * but core clock frequency should not lower than bus clock frequency.
             */
            RK_U32  dspr_otsd               : 1;
            RK_U32  reserved                : 24;
        } vepu541;

        struct vepu540_t {
            RK_U32  reserved0               : 7;
            /*
             * Down sampled reference picture read outstanding enable.
             * 1'h0: No outstanding
             * 1'h1: Outstanding read, which improves data transaction efficiency,
             * but core clock frequency should not lower than bus clock frequency.
             */
            RK_U32  dspr_otsd               : 1;
            RK_U32  reserved1               : 8;
            /*
             * AXI write response channel check enable.
             * [7]: lpf write response check enable
             * [6]: Reconstructed picture write response check enable.
             * [5]: ME information write response check enable.
             * [4]: CTU information write response check enable.
             * [3]: Down-sampled picture write response check enable.
             * [2]: Bit stream write response check enable.
             * [1]: Link table mode write reponse check enable.
             * [0]: Reserved for video preprocess.
             */
            RK_U32  axi_brsp_cke            : 8;
            RK_U32  reserved2               : 8;
        } vepu540;
    } reg016;

    /*
     * SRC_FMT
     * Address offset: 0x0044 Access type: read and write
     * Video source format
     */
    struct {
        /*
         * Swap the position of alpha and RGB for ARBG8888.
         * 1'h0: BGRA8888 or RGBA8888.
         * 1'h1: ABGR8888 or ARGB8888.
         */
        RK_U32  alpha_swap              : 1;
        /*
         * Swap the position of R and B for BGRA8888, RGB888, RGB 656 format;
         * Swap the position of U and V for YUV422-SP, YUV420-SP, YUYV422 and UYUV422 format.
         * 1'h0: RGB or YUYV or UYVY.
         * 1'h1: BGR or YVYU or VYUY.
         */
        RK_U32  rbuv_swap               : 1;
        /*
         * Video source color format.
         * 4'h0: BGRA8888
         * 4'h1: RGB888
         * 4'h2: RGB565
         * 4'h4: YUV422 SP
         * 4'h5: YUV422 P
         * 4'h6: YUV420 SP
         * 4'h7: YUV420 P
         * 4'h8: YUYV422
         * 4'h9: UYVY422
         * Others: Reserved
         */
        RK_U32  src_cfmt                : 4;
        /*
         * Video source clip (low active).
         * 1'h0: [16:235] for luma and [16:240] for chroma.
         * 1'h1: [0:255] for both luma and chroma.
         */
        RK_U32  src_range               : 1;
        /*
         * Ourput reconstructed frame format
         * 1'h0: yuv420
         * 1'h1: yuv400
         */
        RK_U32  out_fmt_cfg             : 1;
        RK_U32  reserved                : 24;
    } reg017;

    /*
     * SRC_UDFY
     * Address offset: 0x0048 Access type: read and write
     * Weight of user defined formula for RBG to Y conversion
     */
    struct {
        /* Weight of BLUE  in RBG to Y conversion formula. */
        RK_U32  csc_wgt_b2y             : 9;
        /* Weight of GREEN in RBG to Y conversion formula. */
        RK_U32  csc_wgt_g2y             : 9;
        /* Weight of RED   in RBG to Y conversion formula. */
        RK_U32  csc_wgt_r2y             : 9;
        RK_U32  reserved                : 5;
    } reg018;

    /*
     * SRC_UDFU
     * Address offset: 0x004c Access type: read and write
     * Weight of user defined formula for RBG to U conversion
     */
    struct {
        /* Weight of BLUE  in RBG to U conversion formula. */
        RK_U32  csc_wgt_b2u             : 9;
        /* Weight of GREEN in RBG to U conversion formula. */
        RK_U32  csc_wgt_g2u             : 9;
        /* Weight of RED   in RBG to U conversion formula. */
        RK_U32  csc_wgt_r2u             : 9;
        RK_U32  reserved                : 5;
    } reg019;

    /*
     * SRC_UDFV
     * Address offset: 0x0050 Access type: read and write
     * Weight of user defined formula for RBG to V conversion
     */
    struct {
        /* Weight of BLUE  in RBG to V conversion formula. */
        RK_U32  csc_wgt_b2v             : 9;
        /* Weight of GREEN in RBG to V conversion formula. */
        RK_U32  csc_wgt_g2v             : 9;
        /* Weight of RED   in RBG to V conversion formula. */
        RK_U32  csc_wgt_r2v             : 9;
        RK_U32  reserved                : 5;
    } reg020;

    /*
     * SRC_UDFO
     * Address offset: 0x0054 Access type: read and write
     * Offset of user defined formula for RBG to YUV conversion
     */
    struct {
        /* Offset of RBG to V conversion formula. */
        RK_U32  csc_ofst_v              : 8;
        /* Offset of RBG to U conversion formula. */
        RK_U32  csc_ofst_u              : 8;
        /* Offset of RBG to Y conversion formula. */
        RK_U32  csc_ofst_y              : 5;
        RK_U32  reserved                : 11;
    } reg021;

    /*
     * SRC_PROC
     * Address offset: 0x0058 Access type: read and write
     * Video source process
     */
    struct {
        RK_U32  reserved0               : 26;
        /* Video source mirror mode enable. */
        RK_U32  src_mirr                : 1;
        /*
         * Video source rotation mode.
         * 2'h0: 0 degree
         * 2'h1: Clockwise 90 degree
         * 2'h2: Clockwise 180 degree
         * 2'h3: Clockwise 280 degree
         */
        RK_U32  src_rot                 : 2;
        /* Video source texture analysis enable. */
        RK_U32  txa_en                  : 1;
        /* AFBC decompress enable (for AFBC format video source). */
        RK_U32  afbcd_en                : 1;
        RK_U32  reserved1               : 1;
    } reg022;

    /*
     * SLI_CFG_H264
     * Address offset: 0x005C Access type: read and write
     * Slice cross lines configuration, h264 only.
     */
    struct {
        RK_U32  reserved0               : 31;
        /*
         * Slice cut cross lines enable,
         * using for breaking the resolution limit, h264 only.
         */
        RK_U32  sli_crs_en              : 1;
    } reg023;

    /* reg gap 024 */
    RK_U32 reg_024;

    /*
     * KLUT_OFST
     * Address offset: 0x0064 Access type: read and write
     * Offset of (RDO) chroma cost weight table
     */
    struct {
        /* Offset of (RDO) chroma cost weight table, values from 0 to 6. */
        RK_U32  chrm_klut_ofst          : 3;
        RK_U32  reserved                : 1;
    } reg025;

    /*
     * KLUT_WGT0
     * Address offset: 0x0068 Access type: read and write
     * (RDO) Chroma weight table configure register0
     */
    struct {
        /* Data0 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt0          : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data1 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt1_l9       : 9;
    } reg026;

    /*
     * KLUT_WGT1
     * Address offset: 0x006C Access type: read and write
     * (RDO) Chroma weight table configure register1
     */
    struct {
        /* High 9 bits of data1 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt1_h9       : 9;
        RK_U32  reserved                : 5;
        /* Data2 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt2          : 18;
    } reg027;

    /*
     * KLUT_WGT2
     * Address offset: 0x0070 Access type: read and write
     * (RDO) Chroma weight table configure register2
     */
    struct {
        /* Data3 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt3          : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data4 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt4_l9       : 9;
    } reg028;

    /*
     * KLUT_WGT3
     * Address offset: 0x0074 Access type: read and write
     * (RDO) Chroma weight table configure register3
     */
    struct {
        /* High 9 bits of data4 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt4_h9       : 9;
        RK_U32  reserved                : 5;
        /* Data5 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt5          : 18;
    } reg029;

    /*
     * KLUT_WGT4
     * Address offset: 0x0078 Access type: read and write
     * (RDO) Chroma weight table configure register4
     */
    struct {
        /* Data6 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt6          : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data7 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt7_l9       : 9;
    } reg030;

    /*
     * KLUT_WGT5
     * Address offset: 0x007C Access type: read and write
     * (RDO) Chroma weight table configure register5
     */
    struct {
        /* High 9 bits of data7 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt7_h9       : 9;
        RK_U32  reserved                : 5;
        /* Data8 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt8          : 18;
    } reg031;

    /*
     * KLUT_WGT6
     * Address offset: 0x0080 Access type: read and write
     * (RDO) Chroma weight table configure register6
     */
    struct {
        /* Data9 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt9          : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data10 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt10_l9      : 9;
    } reg032;

    /*
     * KLUT_WGT7
     * Address offset: 0x0084 Access type: read and write
     * (RDO) Chroma weight table configure register7
     */
    struct {
        /* High 9 bits of data10 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt10_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data11 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt11         : 18;
    } reg033;

    /*
     * KLUT_WGT8
     * Address offset: 0x0088 Access type: read and write
     * (RDO) Chroma weight table configure register8
     */
    struct {
        /* Data12 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt12         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data13 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt13_l9      : 9;
    } reg034;

    /*
     * KLUT_WGT9
     * Address offset: 0x008C Access type: read and write
     * (RDO) Chroma weight table configure register9
     */
    struct {
        /* High 9 bits of data13 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt13_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data14 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt14         : 18;
    } reg035;

    /*
     * KLUT_WGT10
     * Address offset: 0x0090 Access type: read and write
     * (RDO) Chroma weight table configure register10
     */
    struct {
        /* Data15 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt15         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data16 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt16_l9      : 9;
    } reg036;

    /*
     * KLUT_WGT11
     * Address offset: 0x0094 Access type: read and write
     * (RDO) Chroma weight table configure register11
     */
    struct {
        /* High 9 bits of data16 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt16_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data17 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt17         : 18;
    } reg037;

    /*
     * KLUT_WGT12
     * Address offset: 0x0098 Access type: read and write
     * (RDO) Chroma weight table configure register12
     */
    struct {
        /* Data18 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt18         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data19 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt19_l9      : 9;
    } reg038;

    /*
     * KLUT_WGT13
     * Address offset: 0x009C Access type: read and write
     * (RDO) Chroma weight table configure register13
     */
    struct {
        /* High 9 bits of data19 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt19_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data14 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt20         : 18;
    } reg039;

    /*
     * KLUT_WGT14
     * Address offset: 0x00A0 Access type: read and write
     * (RDO) Chroma weight table configure register14
     */
    struct {
        /* Data21 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt21         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data22 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt22_l9      : 9;
    } reg040;

    /*
     * KLUT_WGT15
     * Address offset: 0x00A4 Access type: read and write
     * (RDO) Chroma weight table configure register15
     */
    struct {
        /* High 9 bits of data22 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt22_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data23 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt23         : 18;
    } reg041;

    /*
     * KLUT_WGT16
     * Address offset: 0x00A8 Access type: read and write
     * (RDO) Chroma weight table configure register16
     */
    struct {
        /* Data24 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt24         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data25 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt25_l9      : 9;
    } reg042;

    /*
     * KLUT_WGT17
     * Address offset: 0x00AC Access type: read and write
     * (RDO) Chroma weight table configure register17
     */
    struct {
        /* High 9 bits of data25 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt25_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data26 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt26         : 18;
    } reg043;

    /*
     * KLUT_WGT18
     * Address offset: 0x00B0 Access type: read and write
     * (RDO) Chroma weight table configure register18
     */
    struct {
        /* Data27 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt27         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data28 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt28_l9      : 9;
    } reg044;

    /*
     * KLUT_WGT19
     * Address offset: 0x00B4 Access type: read and write
     * (RDO) Chroma weight table configure register19
     */
    struct {
        /* High 9 bits of data28 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt28_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data29 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt29         : 18;
    } reg045;

    /*
     * KLUT_WGT20
     * Address offset: 0x00B8 Access type: read and write
     * (RDO) Chroma weight table configure register20
     */
    struct {
        /* Data30 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt30         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data31 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt31_l9      : 9;
    } reg046;

    /*
     * KLUT_WGT21
     * Address offset: 0x00BC Access type: read and write
     * (RDO) Chroma weight table configure register21
     */
    struct {
        /* High 9 bits of data31 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt31_h9      : 9;
        RK_U32  reserved                : 5;
        /* Data32 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt32         : 18;
    } reg047;

    /*
     * KLUT_WGT22
     * Address offset: 0x00C0 Access type: read and write
     * (RDO) Chroma weight table configure register22
     */
    struct {
        /* Data33 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt33         : 18;
        RK_U32  reserved                : 5;
        /* Low 9 bits of data34 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt34_l9      : 9;
    } reg048;

    /*
     * KLUT_WGT23
     * Address offset: 0x00C4 Access type: read and write
     * (RDO) Chroma weight table configure register23
     */
    struct {
        /* High 9 bits of data34 in chroma cost weight table. */
        RK_U32  chrm_klut_wgt34_h9      : 9;
        RK_U32  reserved                : 23;
    } reg049;

    /*
     * RC_CFG
     * Address offset: 0x00C8 Access type: read and write
     * Rate control configuration
     */
    struct {
        /* Rate control enable. */
        RK_U32  rc_en                   : 1;
        /* Adaptive quantization enable. */
        RK_U32  aq_en                   : 1;
        /*
         * Mode of aq_delta calculation for CU32 and CU64.
         * 1'b0: aq_delta of CU32/CU64 is calculated by corresponding MADI32/64;
         * 1'b1: aq_delta of CU32/CU64 is calculated by corresponding 4/16 CU16 qp_deltas.
         */
        RK_U32  aq_mode                 : 1;
        RK_U32  reserved                : 13;
        /* RC adjustment intervals, base on CTU number. */
        RK_U32  rc_ctu_num              : 16;
    } reg050;

    /*
     * RC_QP
     * Address offset: 0x00CC Access type: read and write
     * QP configuration for rate control
     */
    struct {
        RK_U32  reserved                : 16;
        /*
         * QP adjust range(delta_qp) in rate control.
         * Delta_qp is constrained  between -rc_qp_range to rc_qp_range.
         */
        RK_S32  rc_qp_range             : 4;
        /* Max QP for rate control and AQ mode. */
        RK_U32  rc_max_qp               : 6;
        /* Min QP for rate control and AQ mode. */
        RK_U32  rc_min_qp               : 6;
    } reg051;

    /*
     * RC_TGT
     * Address offset: 0x00D0 Access type: read and write
     * The target bit rate for rate control
     */
    struct {
        /*
         * Target bit num for one 64x64 CTU(for HEVC)
         * or one 16x16 MB(for H.264), with 1/16 precision.
         */
        RK_U32  ctu_ebit                : 20;
        RK_U32  reserved                : 12;
    } reg052;

    /*
     * RC_ADJ0
     * Address offset: 0x00D4 Access type: read and write
     * QP adjust configuration for rate control
     */
    struct {
        /* QP adjust step0 for rate control. */
        RK_S32  qp_adj0                 : 5;
        /* QP adjust step1 for rate control. */
        RK_S32  qp_adj1                 : 5;
        /* QP adjust step2 for rate control. */
        RK_S32  qp_adj2                 : 5;
        /* QP adjust step3 for rate control. */
        RK_S32  qp_adj3                 : 5;
        /* QP adjust step4 for rate control. */
        RK_S32  qp_adj4                 : 5;
        RK_U32  reserved                : 7;
    } reg053;

    /*
     * RC_ADJ1
     * Address offset: 0x00D8 Access type: read and write
     * QP adjust configuration for rate control
     */
    struct {
        /* QP adjust step5 for rate control. */
        RK_S32  qp_adj5                 : 5;
        /* QP adjust step6 for rate control. */
        RK_S32  qp_adj6                 : 5;
        /* QP adjust step7 for rate control. */
        RK_S32  qp_adj7                 : 5;
        /* QP adjust step8 for rate control. */
        RK_S32  qp_adj8                 : 5;
        RK_U32  reserved                : 12;
    } reg054;

    /*
     * RC_DTHD0~8
     * Address offset: 0x00DC~0x00FC Access type: read and write
     * Bits rate deviation threshold0~8
     */
    struct {
        /* Bits rate deviation threshold0~8. */
        RK_U32  rc_dthd[9];
    } reg055_063;

    /*
     * ROI_QTHD0
     * Address offset: 0x0100 Access type: read and write
     * ROI QP threshold configuration0
     */
    struct {
        /* Min QP for 16x16 CU inside ROI area0. */
        RK_U32  qpmin_area0             : 6;
        /* Max QP for 16x16 CU inside ROI area0. */
        RK_U32  qpmax_area0             : 6;
        /* Min QP for 16x16 CU inside ROI area1. */
        RK_U32  qpmin_area1             : 6;
        /* Max QP for 16x16 CU inside ROI area1. */
        RK_U32  qpmax_area1             : 6;
        /* Min QP for 16x16 CU inside ROI area2. */
        RK_U32  qpmin_area2             : 6;
        RK_U32  reserved                : 2;
    } reg064;

    /*
     * ROI_QTHD1
     * Address offset: 0x0104 Access type: read and write
     * ROI QP threshold configuration1
     */
    struct {
        /* Max QP for 16x16 CU inside ROI area2. */
        RK_U32  qpmax_area2             : 6;
        /* Min QP for 16x16 CU inside ROI area3. */
        RK_U32  qpmin_area3             : 6;
        /* Max QP for 16x16 CU inside ROI area3. */
        RK_U32  qpmax_area3             : 6;
        /* Min QP for 16x16 CU inside ROI area4. */
        RK_U32  qpmin_area4             : 6;
        /* Min QP for 16x16 CU inside ROI area4. */
        RK_U32  qpmax_area4             : 6;
        RK_U32  reserved                : 2;
    } reg065;

    /*
     * ROI_QTHD2
     * Address offset: 0x0108 Access type: read and write
     * ROI QP threshold configuration2
     */
    struct {
        /* Min QP for 16x16 CU inside ROI area5. */
        RK_U32  qpmin_area5             : 6;
        /* Max QP for 16x16 CU inside ROI area5. */
        RK_U32  qpmax_area5             : 6;
        /* Min QP for 16x16 CU inside ROI area6. */
        RK_U32  qpmin_area6             : 6;
        /* Max QP for 16x16 CU inside ROI area6. */
        RK_U32  qpmax_area6             : 6;
        /* Min QP for 16x16 CU inside ROI area7. */
        RK_U32  qpmin_area7             : 6;
        RK_U32  reserved                : 2;
    } reg066;

    /*
     * ROI_QTHD3
     * Address offset: 0x010C Access type: read and write
     * ROI QP threshold configuration3
     */
    struct {
        /* Max QP for 16x16 CU inside ROI area7. */
        RK_U32  qpmax_area7             : 6;
        RK_U32  reserved                : 24;
        /*
         * QP theshold generation for the CUs whose size is bigger than 16x16.
         * 2'h0: Mean value of 16x16 CU QP thesholds
         * 2'h1: Max value of 16x16 CU QP thesholds
         * 2'h2: Min value of 16x16 CU QP thesholds
         * 2'h3: Reserved
         */
        RK_U32  qpmap_mode              : 2;
    } reg067;

    /*
     * PIC_OFST
     * Address offset: 0x0110 Access type: read and write
     * Encoding picture offset
     */
    struct {
        /* Vertical offset for encoding picture. */
        RK_U32  pic_ofst_y              : 13;
        RK_U32  reserved0               : 3;
        /* Horizontal offset for encoding picture. */
        RK_U32  pic_ofst_x              : 13;
        RK_U32  reserved1               : 3;
    } reg068;

    /*
     * SRC_STRID
     * Address offset: 0x0114 Access type: read and write
     * Video source stride
     */
    struct {
        /*
         * Video source stride0, based on pixel (byte).
         * Note that if the video format is YUV, src_strd is the LUMA component
         * stride while src_strid1 is the CHROMA component stride.
         */
        RK_U32  src_strd0               : 16;
        /*
         * CHROMA stride of video source, only for YUV format.
         * Note that U and V stride must be the same when color format is YUV
         * planar.
         */
        RK_U32  src_strd1               : 16;
    } reg069;

    /*
     * ADR_SRC0
     * Address offset: 0x0118 Access type: read and write
     * Base address of the 1st storage area for video source
     */
    struct {
        /*
         * Base address of the 1st storage area for video source.
         * ARGB8888, BGR888, RGB565, YUYV422 and UYUV422 have only one storage
         * area, while adr_src0 is configured as the base address of video
         * source frame buffer.
         * YUV422/420 semi-planar have 2 storage area, while adr_src0 is
         * configured as the base address of Y frame buffer.
         * YUV422/420 planar have 3 storage area, while adr_src0 is configured
         * as the base address of Y frame buffer.
         * Note that if the video source is compressed by AFBC, adr_src0 is
         * configured as the base address of compressed frame buffer.
         */
        RK_U32  adr_src0;
    } reg070;

    /*
     * ADR_SRC1
     * Address offset: 0x011C Access type: read and write
     * Base address of the 2nd storage area for video source
     */
    struct {
        /*
         * Base address of V frame buffer when video source is uncompress and
         * color format is YUV422/420 planar.
         */
        RK_U32  adr_src1;
    } reg071;

    /*
     * ADR_SRC2
     * Address offset: 0x0120 Access type: read and write
     * Base address of the 3rd storage area for video source
     */
    struct {
        /*
         * Base address of V frame buffer when video source is uncompress and
         * color format is YUV422/420 planar.
         */
        RK_U32  adr_src2;
    } reg072;

    /*
     * ADR_ROI
     * Address offset: 0x0124 Access type: read and write
     * Base address for ROI configuration, 16 bytes aligned
     */
    struct {
        /* High 28 bits of base address for ROI configuration. */
        RK_U32  roi_addr;
    } reg073;

    /*
     * ADR_RFPW_H
     * Address offset: 0x0128 Access type: read and write
     * Base address of header_block for compressed reference frame write,
     * 4K bytes aligned
     */
    struct {
        /*
         * High 20 bits of the header_block base address for compressed
         * reference frame write.
         */
        RK_U32  rfpw_h_addr;
    } reg074;

    /*
     * ADR_RFPW_B
     * Address offset: 0x012C Access type: read and write
     * Base address of body_block for compressed reference frame write,
     * 4K bytes aligned
     */
    struct {
        /*
         * High 20 bits of the body_block base address for compressed
         * reference frame write.
         */
        RK_U32  rfpw_b_addr;
    } reg075;

    /*
     * ADR_RFPR_H
     * Address offset: 0x0130 Access type: read and write
     * Base address of header_block for compressed reference frame read,
     * 4K bytes aligned
     */
    struct {
        /*
         * High 20 bits of the header_block base address for compressed
         * reference frame read.
         */
        RK_U32  rfpr_h_addr;
    } reg076;

    /*
     * ADR_RFPR_B
     * Address offset: 0x0134 Access type: read and write
     * Base address of body_block for compressed reference frame read,
     * 4K bytes aligned
     */
    struct {
        /*
         * High 20 bits of the body_block base address for compressed
         * reference frame read.
         */
        RK_U32  rfpr_b_addr;
    } reg077;

    /*
     * ADR_CMVW
     * Address offset: 0x0138 Access type: read and write
     * Base address for col-located Mv write, 1KB aligned, HEVC only
     */
    struct {
        /* High 22 bits of base address for col-located Mv write, HEVC only. */
        RK_U32  cmvw_addr;
    } reg078;

    /*
     * ADR_CMVR
     * Address offset: 0x013C Access type: read and write
     * Base address for col-located Mv read, 1KB aligned, HEVC only
     */
    struct {
        /* High 22 bits of base address for col-located Mv read, HEVC only. */
        RK_U32  cmvr_addr;
    } reg079;

    /*
     * ADR_DSPW
     * Address offset: 0x0140 Access type: read and write
     * Base address for down-sampled reference frame write, 1KB aligned
     */
    struct {
        /* High 22 bits of base address for down-sampled reference frame write. */
        RK_U32  dspw_addr;
    } reg080;

    /*
     * ADR_DSPR
     * Address offset: 0x0144 Access type: read and write
     * Base address for down-sampled reference frame read, 1KB aligned
     */
    struct {
        /* High 22 bits of base address for down-sampled reference frame read. */
        RK_U32  dspr_addr;
    } reg081;

    /*
     * ADR_MEIW
     * Address offset: 0x0148 Access type: read and write
     * Base address for ME information write, 1KB aligned
     */
    struct {
        /* High 22 bits of base address for ME information write. */
        RK_U32  meiw_addr;
    } reg082;

    /*
     * ADR_BSBT
     * Address offset: 0x014C Access type: read and write
     * Top address of bit stream buffer, 128B aligned
     */
    struct {
        /* High 25 bits of the top address of bit stream buffer. */
        RK_U32  bsbt_addr;
    } reg083;

    /*
     * ADR_BSBB
     * Address offset: 0x0150 Access type: read and write
     * Bottom address of bit stream buffer, 128B aligned
     */
    struct {
        /* High 25 bits of the bottom address of bit stream buffer. */
        RK_U32  bsbb_addr;
    } reg084;

    /*
     * ADR_BSBR
     * Address offset: 0x0154 Access type: read and write
     * Read address of bit stream buffer, 128B aligned
     */
    struct {
        /*
         * Read address of bit stream buffer, 128B aligned.
         * VEPU will pause when write address meets read address and then send
         * an interrupt. SW should move some data out from bit stream buffer
         * and change this register accordingly.
         * After that VEPU will continue processing automatically.
         */
        RK_U32  bsbr_addr;
    } reg085;

    /*
     * ADR_BSBS
     * Address offset: 0x0158 Access type: read and write
     * Start address of bit stream buffer
     */
    struct {
        /*
         * Start address of bit stream buffer.
         * VEPU begins to write bit stream from this address and increase
         * address automatically.
         * Note that the VEPU's real-time write address is marked in BSB_STUS.
         */
        RK_U32  adr_bsbs;
    } reg086;

    /*
     * SLI_SPLT
     * Address offset: 0x015C Access type: read and write
     * Slice split configuration
     */
    struct {
        /* Slice split enable. */
        RK_U32  sli_splt                : 1;
        /*
         * Slice split mode.
         * 1'h0: Slice splited by byte.
         * 1'h1: Slice splited by number of MB(H.264)/CTU(HEVC).
         */
        RK_U32  sli_splt_mode           : 1;
        /*
         * Slice split compensation when slice is splited by byte.
         * Byte distortion of current slice will be compensated in the next slice.
         */
        RK_U32  sli_splt_cpst           : 1;
        /* Max slice num in one frame. */
        RK_U32  sli_max_num_m1          : 10;
        /* Slice flush. Flush all the bit stream after each slice finished. */
        RK_U32  sli_flsh                : 1;
        RK_U32  reserved                : 2;
        /* Number of CTU/MB for slice split. Valid when slice is splited by CTU/MB. */
        RK_U32  sli_splt_cnum_m1        : 16;
    } reg087;

    /*
     * SLI_BYTE
     * Address offset: 0x0160 Access type: read and write
     * Number of bytes for slice split
     */
    struct {
        /* Byte number for each slice when slice is splited by byte. */
        RK_U32  sli_splt_byte           : 18;
        RK_U32  reserved                : 14;
    } reg088;

    /*
     * ME_RNGE
     * Address offset: 0x0164 Access type: read and write
     * Motion estimation range
     */
    struct {
        /* CME horizontal search range, base on 16 pixels. */
        RK_U32  cme_srch_h              : 4;
        /* CME vertical search range, base on 16 pixel. */
        RK_U32  cme_srch_v              : 4;
        /* RME horizontal search range, values from 3 to 7. */
        RK_U32  rme_srch_h              : 3;
        /* RME vertical search range, values from 4 to 5. */
        RK_U32  rme_srch_v              : 3;
        RK_U32  reserved                : 2;
        /* Frame number difference value between current and reference frame, HEVC only. */
        RK_U32  dlt_frm_num             : 16;
    } reg089;

    /*
     * ME_CNST
     * Address offset: 0x0168 Access type: read and write
     * Motion estimation configuration
     */
    struct {
        /* Min horizontal distance for PMV selection. */
        RK_U32  pmv_mdst_h              : 8;
        /* Min vertical distance for PMV selection. */
        RK_U32  pmv_mdst_v              : 8;
        /*
         * Motion vector limit ( by level), H.264 only.
         * 2'h0: Mvy is limited to [-64,63].
         * Others: Mvy is limited to [-128,127].
         */
        RK_U32  mv_limit                : 2;
        /* PMV number (should be constant2). */
        RK_U32  pmv_num                 : 2;
        /* Store col-Mv information to external memory, HEVC only. */
        RK_U32  colmv_stor              : 1;
        /* Load co-located Mvs as predicated Mv candidates, HEVC only. */
        RK_U32  colmv_load              : 1;
        /*
         * [4]: Disable 64x64 block RME.
         * [3]: Disable 32x32 block RME.
         * [2]: Disable 16x16 block RME.
         * [1]: Disable 8x8   block RME.
         * [0]: Disable 4x4   block RME.
         */
        RK_U32  rme_dis                 : 5;
        /*
         * [4]: Disable 64x64 block FME.
         * [3]: Disable 32x32 block FME.
         * [2]: Disable 16x16 block FME.
         * [1]: Disable 8x8   block FME.
         * [0]: Disable 4x4   block FME.
         */
        RK_U32  fme_dis                 : 5;
    } reg090;

    /*
     * ME_RAM
     * Address offset: 0x016C Access type: read and write
     * ME cache configuration
     */
    struct {
        /* CME's max RAM address. */
        RK_U32  cme_rama_max            : 11;
        /* Height of CME RAMA district, base on 4 pixels. */
        RK_U32  cme_rama_h              : 5;
        /*
         * L2 cach mapping, base on pixels.
         * 2'h0: 32x512
         * 2'h1: 16x1024
         * 2'h2: 8x2048
         * 2'h3: 4x4096
         */
        RK_U32  cach_l2_map             : 2;
        /* The width of CIME down-sample recon data linebuf, based on 64 pixel. */
        RK_U32  cme_linebuf_w           : 8;
        RK_U32  reserved                : 6;
    } reg091;

    /*
     * SYNT_LONG_REFM0
     * Address offset: 0x0170 Access type: read and write
     * Long term reference frame mark0 for HEVC
     */
    struct {
        /* Poc_lsb_lt[1] */
        RK_U32  poc_lsb_lt1             : 16;
        /* Poc_lsb_lt[2] */
        RK_U32  poc_lsb_lt2             : 16;
    } reg092;

    /*
     * SYNT_LONG_REFM1
     * Address offset: 0x0174 Access type: read and write
     * Long term reference frame mark1 for HEVC
     */
    struct {
        /* Delta_poc_msb_cycle_lt[1] */
        RK_U32  dlt_poc_msb_cycl1       : 16;
        /* Delta_poc_msb_cycle_lt[2] */
        RK_U32  dlt_poc_msb_cycl2       : 16;
    } reg093;

    /*
     * OSD_INV_CFG
     * Address offset: 0x0178 Access type: read and write
     * OSD color inverse  configuration
     *
     * Added in vepu540
     */
    struct {
        /*
         * OSD color inverse enable of chroma component,
         * each bit controls corresponding region.
         */
        RK_U32  osd_ch_inv_en           : 8;
        /*
         * OSD color inverse expression type
         * each bit controls corresponding region.
         * 1'h0: AND;
         * 1'h1: OR
         */
        RK_U32  osd_itype               : 8;
        /*
         * OSD color inverse expression switch for luma component
         * each bit controls corresponding region.
         * 1'h0: Expression need to determine the condition;
         * 1'h1: Expression don't need to determine the condition;
         */
        RK_U32  osd_lu_inv_msk          : 8;
        /*
         * OSD color inverse expression switch for chroma component
         * each bit controls corresponding region.
         * 1'h0: Expression need to determine the condition;
         * 1'h1: Expression don't need to determine the condition;
         */
        RK_U32  osd_ch_inv_msk          : 8;
    } reg094;

    /* reg gap 095~100 */
    RK_U32 reg_095_100[6];

    /*
     * IPRD_CSTS
     * Address offset: 0x0194 Access type: read and write
     * Cost function configuration for intra prediction
     */
    struct {
        /* LUMA variance threshold to select intra prediction cost function. */
        RK_U32  vthd_y                  : 12;
        RK_U32  reserved0               : 4;
        /* CHROMA variance threshold to select intra prediction cost function. */
        RK_U32  vthd_c                  : 12;
        RK_U32  reserved1               : 4;
    } reg101;

    /*
     * RDO_CFG_H264
     * Address offset: 0x0198 Access type: read and write
     * H.264 RDO configuration
     */
    struct {
        /* Limit sub_mb_rect_size for low level. */
        RK_U32  rect_size               : 1;
        /* 4x4 sub MB enable. */
        RK_U32  inter_4x4               : 1;
        /* Reserved */
        RK_U32  arb_sel                 : 1;
        /* CAVLC syntax limit. */
        RK_U32  vlc_lmt                 : 1;
        /* Chroma special candidates enable. */
        RK_U32  chrm_spcl               : 1;
        /*
         * [7]: Disable intra4x4.
         * [6]: Disable intra8x8.
         * [5]: Disable intra16x16.
         * [4]: Disable inter8x8 with T4.
         * [3]: Disable inter8x8 with T8.
         * [2]: Disable inter16x16 with T4.
         * [1]: Disable inter16x16 with T8.
         * [0]: Disable skip mode.
         */
        RK_U32  rdo_mask                : 8;
        /* Chroma cost weight adjustment(KLUT) enable. */
        RK_U32  ccwa_e                  : 1;
        /*
         * Scale list selection.
         * 1'h0: Flat scale list.
         * 1'h1: Default scale list.
         */
        RK_U32  scl_lst_sel             : 1;
        /* Anti-ring enable. */
        RK_U32  atr_e                   : 1;
        /* Edge of anti-flicker, base on MB. the MBs inside edge should not influenced. */
        RK_U32  atf_edg                 : 2;
        /* Block level anti-flicker enable. */
        RK_U32  atf_lvl_e               : 1;
        /* Intra mode anti-flicker enable. */
        RK_U32  atf_intra_e             : 1;
        /*
         * Scale list selection. (for vepu540)
         * 2'h0: Flat scale list.
         * 2'h1: Default scale list.
         * 2'h2: User defined.
         * 2'h3: Reserved.
         */
        RK_U32  scl_lst_sel_            : 2;
        RK_U32  reserved                : 9;
        /*
         * Rdo cost caculation expression for intra by using sad or satd.
         * 1'h0: SATD;
         * 1'h1: SAD;
         */
        RK_U32  satd_byps_flg           : 1;
    } reg102;

    /*
     * SYNT_NAL_H264
     * Address offset: 0x019C Access type: read and write
     * NAL configuration for H.264
     */
    struct {
        /* nal_ref_idc */
        RK_U32  nal_ref_idc             : 2;
        RK_U32  nal_unit_type           : 5;
        /* nal_unit_type */
        RK_U32  reserved                : 25;
    } reg103;

    /*
     * SYNT_SPS_H264
     * Address offset: 0x01A0 Access type: read and write
     * Sequence parameter set syntax configuration for H.264
     */
    struct {
        /* log2_max_frame_num_minus4 */
        RK_U32  max_fnum                : 4;
        /* direct_8x8_inference_flag */
        RK_U32  drct_8x8                : 1;
        /* log2_max_pic_order_cnt_lsb_minus4 */
        RK_U32  mpoc_lm4                : 4;
        RK_U32  reserved                : 23;
    } reg104;

    /*
     * SYNT_PPS_H264
     * Address offset: 0x01A4 Access type: read and write
     * Picture parameter set configuration for H.264
     */
    struct {
        /* entropy_coding_mode_flag */
        RK_U32  etpy_mode               : 1;
        /* transform_8x8_mode_flag */
        RK_U32  trns_8x8                : 1;
        /* constrained_intra_pred_flag */
        RK_U32  csip_flag               : 1;
        /* num_ref_idx_l0_active_minus1 */
        RK_U32  num_ref0_idx            : 2;
        /* num_ref_idx_l1_active_minus1 */
        RK_U32  num_ref1_idx            : 2;
        /* pic_init_qp_minus26 + 26 */
        RK_U32  pic_init_qp             : 6;
        /* chroma_qp_index_offset */
        RK_U32  cb_ofst                 : 5;
        /* second_chroma_qp_index_offset */
        RK_U32  cr_ofst                 : 5;
        /* weight_pred_flag */
        RK_U32  wght_pred               : 1;
        /* deblocking_filter_control_present_flag */
        RK_U32  dbf_cp_flg              : 1;
        RK_U32  reserved                : 7;
    } reg105;

    /*
     * SYNT_SLI0_H264
     * Address offset: 0x01A8 Access type: read and write
     * Slice header configuration0 for H.264
     */
    struct {
        /* slice_type: 0->P, 1->B, 2->I. */
        RK_U32  sli_type                : 2;
        /* pic_parameter_set_id */
        RK_U32  pps_id                  : 8;
        /* direct_spatial_mv_pred_flag */
        RK_U32  drct_smvp               : 1;
        /* num_ref_idx_active_override_flag */
        RK_U32  num_ref_ovrd            : 1;
        /* cabac_init_idc */
        RK_U32  cbc_init_idc            : 2;
        RK_U32  reserved                : 2;
        /* frame_num */
        RK_U32  frm_num                 : 16;
    } reg106;

    /*
     * SYNT_SLI1_H264
     * Address offset: 0x01AC Access type: read and write
     * Slice header configuration1 for H.264
     */
    struct {
        /* idr_pid */
        RK_U32  idr_pic_id              : 16;
        /* pic_order_cnt_lsb */
        RK_U32  poc_lsb                 : 16;
    } reg107;

    /*
     * SYNT_SLI2_H264
     * Address offset: 0x01B0 Access type: read and write
     * Slice header configuration2 for H.264
     */
    struct {
        /* reordering_of_pic_nums_idc */
        RK_U32  rodr_pic_idx            : 2;
        /* ref_pic_list_reordering_flag_l0 */
        RK_U32  ref_list0_rodr          : 1;
        /* slice_beta_offset_div2 */
        RK_U32  sli_beta_ofst           : 4;
        /* slice_alpha_c0_offset_div2 */
        RK_U32  sli_alph_ofst           : 4;
        /* disable_deblocking_filter_idc */
        RK_U32  dis_dblk_idc            : 2;
        RK_U32  reserved                : 3;
        /* abs_diff_pic_num_minus1/long_term_pic_num */
        RK_U32  rodr_pic_num            : 16;
    } reg108;

    /*
     * SYNT_REFM0_H264
     * Address offset: 0x01B4 Access type: read and write
     * Reference frame mark0 for H.264
     */
    struct {
        /* no_output_of_prior_pics_flag */
        RK_U32  nopp_flg                : 1;
        /* long_term_reference_flag */
        RK_U32  ltrf_flg                : 1;
        /* adaptive_ref_pic_marking_mode_flag */
        RK_U32  arpm_flg                : 1;
        /* A No.4 MMCO should be executed firstly if mmo4_pre is 1 */
        RK_U32  mmco4_pre               : 1;
        /* memory_management_control_operation */
        RK_U32  mmco_type0              : 3;
        /*
         * MMCO parameters which have different meanings according to different mmco_parm0 valus.
         * difference_of_pic_nums_minus1 for mmco_parm0 equals 0 or 3.
         * long_term_pic_num for mmco_parm0 equals 2.
         * long_term_frame_idx for mmco_parm0 equals 6.
         * max_long_term_frame_idx_plus1 for mmco_parm0 equals 4.
         */
        RK_U32  mmco_parm0              : 16;
        /* memory_management_control_operation[1] */
        RK_U32  mmco_type1              : 3;
        /* memory_management_control_operation[2] */
        RK_U32  mmco_type2              : 3;
        RK_U32  reserved                : 3;
    } reg109;

    /*
     * SYNT_REFM1_H264
     * Address offset: 0x01B8 Access type: read and write
     * Reference frame mark1 for H.264
     */
    struct {
        /*
         * MMCO parameters which have different meanings according to different mmco_parm1 valus.
         * difference_of_pic_nums_minus1 for mmco_parm1 equals 0 or 3.
         * long_term_pic_num for mmco_parm1 equals 2.
         * long_term_frame_idx for mmco_parm1 equals 6.
         * max_long_term_frame_idx_plus1 for mmco_parm1 equals 4.
         */
        RK_U32  mmco_parm1              : 16;
        /*
         * MMCO parameters which have different meanings according to different mmco_parm2 valus.
         * difference_of_pic_nums_minus1 for mmco_parm2 equals 0 or 3.
         * long_term_pic_num for mmco_parm2 equals 2.
         * long_term_frame_idx for mmco_parm2 equals 6.
         * max_long_term_frame_idx_plus1 for mmco_parm2 equals 4.
         */
        RK_U32  mmco_parm2              : 16;
    } reg110;

    /* reg gap 111 */
    RK_U32 reg_111;

    /*
     * OSD_CFG
     * Address offset: 0x01C0 Access type: read and write
     * OSD configuration
     */
    struct {
        /* OSD region enable, each bit controls corresponding OSD region. */
        RK_U32  osd_e                   : 8;
        /* OSD inverse color enable, each bit controls corresponding region. */
        RK_U32  osd_inv_e               : 8;
        /*
         * OSD palette clock selection.
         * 1'h0: Configure bus clock domain.
         * 1'h1: Core clock domain.
         */
        RK_U32  osd_plt_cks             : 1;
        /*
         * OSD palette type.
         * 1'h1: Default type.
         * 1'h0: User defined type.
         */
        RK_U32  osd_plt_typ             : 1;
        RK_U32  reserved                : 14;
    } reg112;

    /*
     * OSD_INV
     * Address offset: 0x01C4 Access type: read and write
     * OSD color inverse configuration
     */
    struct {
        /* Color inverse theshold for OSD region0. */
        RK_U32  osd_ithd_r0             : 4;
        /* Color inverse theshold for OSD region1. */
        RK_U32  osd_ithd_r1             : 4;
        /* Color inverse theshold for OSD region2. */
        RK_U32  osd_ithd_r2             : 4;
        /* Color inverse theshold for OSD region3. */
        RK_U32  osd_ithd_r3             : 4;
        /* Color inverse theshold for OSD region4. */
        RK_U32  osd_ithd_r4             : 4;
        /* Color inverse theshold for OSD region5. */
        RK_U32  osd_ithd_r5             : 4;
        /* Color inverse theshold for OSD region6. */
        RK_U32  osd_ithd_r6             : 4;
        /* Color inverse theshold for OSD region7. */
        RK_U32  osd_ithd_r7             : 4;
    } reg113;

    /*
     * SYNT_REFM2_H264
     * Address offset: 0x01C8 Access type: read and write
     * Reference frame mark2 for H.264
     */
    struct {
        /* long_term_frame_idx[0] (when mmco equal 3) */
        RK_U32  long_term_frame_idx0    : 4;
        /* long_term_frame_idx[1] (when mmco equal 3) */
        RK_U32  long_term_frame_idx1    : 4;
        /* long_term_frame_idx[2] (when mmco equal 3) */
        RK_U32  long_term_frame_idx2    : 4;
        RK_U32  reserved                : 20;
    } reg114;

    /*
     * SYNT_REFM3
     * Address offset: 0x01CC Access type: read and write
     * Reference frame mark3 for HEVC
     */
    RK_U32 reg115;

    /*
     * OSD_POS
     * Address offset: 0x01D0~0x01EC Access type: read and write
     * OSD region position
     */
    struct {
        Vepu541OsdPos  osd_pos[8];
    } reg116_123;

    /*
     * ADR_OSD
     * Address offset: 0x01F0~0x20C Access type: read and write
     * Base address for OSD region, 16B aligned
     */
    struct {
        RK_U32  osd_addr[8];
    } reg124_131;

    /*
     * ST_BSL
     * Address offset: 0x210 Access type: read only
     * Bit stream length for current frame
     */
    struct {
        /* Bit stream length for current frame. */
        RK_U32  bs_lgth                 : 27;
        RK_U32  reserved                : 5;
    } reg132;

    /*
     * ST_SSE_L32
     * Address offset: 0x214 Access type: read only
     * Low 32 bits of encoding distortion (SSE)
     */
    struct {
        RK_U32  sse_l32;
    } reg133;

    /*
     * ST_SSE_QP
     * Address offset: 0x218 Access type: read only
     * High 8 bits of encoding distortion (SSE) and sum of QP for the encoded frame
     */
    struct {
        /* Sum of QP for the encoded frame. */
        RK_U32  qp_sum                  : 22;
        RK_U32  reserved                : 2;
        /* High bits of encoding distortion(SSE). */
        RK_U32  sse_h8                  : 8;
    } reg134;

    /*
     * ST_SAO
     * Address offset: 0x21C Access type: read only
     * Number of CTUs which adjusted by SAO
     */
    struct {
        /* Number of CTUs whose CHROMA component are adjusted by SAO. */
        RK_U32  sao_cnum                : 12;
        /* Number of CTUs whose LUMA component are adjusted by SAO. */
        RK_U32  sao_ynum                : 12;
        RK_U32  reserved                : 8;
    } reg135;

    /* reg gap 136~137 */
    RK_U32 reg_136_137[2];

    /*
     * ST_ENC
     * Address offset: 0x228 Access type: read only
     * VEPU working status
     */
    struct {
        /*
         * VEPU working status.
         * 2'h0: Idle.
         * 2'h1: Working in register conifguration mode.
         * 2'h2: Working in link table configuration mode.
         */
        RK_U32  st_enc                  : 2;
        /*
         * Status of safe clear.
         * 1'h0: Safe clear is finished or not started.
         * 1'h1: VEPU is performing safe clear.
         */
        RK_U32  st_sclr                 : 1;
        RK_U32  reserved                : 29;
    } reg138;

    /*
     * ST_LKT
     * Address offset: 0x22C Access type: read only
     * Status of link table mode encoding
     */
    struct {
        /* Number of frames has been encoded since link table mode started. */
        RK_U32  fnum_enc                : 8;
        /* Number of frames has been configured since link table mode started. */
        RK_U32  fnum_cfg                : 8;
        /*
         * Number of frames has been encoded since link table mode started,
         * updated only when corresponding link table node send interrupt
         * (VEPU_ENC_PIC_node_int==1).
         */
        RK_U32  fnum_int                : 8;
        RK_U32  reserved                : 8;
    } reg139;

    /*
     * ST_NADR
     * Address offset: 0x230 Access type: read only
     * Address of the processing link table node
     */
    struct {
        /* High 28 bits of the address for the processing linke table node. */
        RK_U32  node_addr;
    } reg140;

    /*
     * ST_BSB
     * Address offset: 0x234 Access type: read only
     * Status of bit stream buffer
     */
    struct {
        /* High 28 bits of bit stream buffer write address. */
        RK_U32  bsbw_addr;
    } reg141;

    /*
     * ST_BUS
     * Address offset: 0x238 Access type: read only
     * VEPU bus status
     */
    struct {
        /*
         * AXI write response idle.
         * [6]: Reconstructed picture channel (AXI0_WID==5)
         * [5]: ME information channel (AXI0_WID==4)
         * [4]: Co-located Mv channel (AXI0_WID==3)
         * [3]: Down-sampled picture channel (AXI0_WID==2)
         * [2]: Bit stream channel (AXI0_WID==1)
         * [1]: Link table node channel (AXI0_WID==0)
         * [0]: Reserved
         */
        RK_U32  axib_idl                : 7;
        /*
         * AXI write response outstanding overflow.
         * [6]: Reconstructed picture channel (AXI0_WID==5)
         * [5]: ME information channel (AXI0_WID==4)
         * [4]: Co-located Mv channel (AXI0_WID==3)
         * [3]: Down-sampled picture channel (AXI0_WID==2)
         * [2]: Bit stream channel (AXI0_WID==1)
         * [1]: Link table node channel (AXI0_WID==0)
         * [0]: Reserved.
         */
        RK_U32  axib_ovfl               : 7;
        /*
         * AXI write response error.
         * [6]: Reconstructed picture channel (AXI0_WID==5)
         * [5]: ME information channel (AXI0_WID==4)
         * [4]: Co-located Mv channel (AXI0_WID==3)
         * [3]: Down-sampled picture channel (AXI0_WID==2)
         * [2]: Bit stream channel (AXI0_WID==1)
         * [1]: Link table node channel (AXI0_WID==0)
         * [0]: Reserved.
         */
        RK_U32  axib_err               : 7;
        /*
         * AXI read error.
         * [5]: ROI configuration (AXI0_ARID==7)
         * [4]: Down-sampled picture (AXI0_ARID==6)
         * [3]: Co-located Mv (AXI0_ARID==5)
         * [2]: Link table (AXI0_ARID==4)
         * [1]: Reference picture (AXI0_ARID==1,2,3,8)
         * [0]: Video source load (AXI1)
         */
        RK_U32  axir_err                : 6;
        RK_U32  reserved                : 5;
    } reg142;

    /*
     * ST_SNUM
     * Address offset: 0x23C Access type: read only
     * Slice number status
     */
    struct {
        /* Number for slices has been encoded and not read out (by reading ST_SLEN). */
        RK_U32  sli_num                 : 6;
        RK_U32  reserved                : 30;
    } reg143;

    /*
     * ST_SLEN
     * Address offset: 0x240 Access type: read only
     * Status of slice length
     */
    struct {
        /* Byte length for the earlist encoded slice which has not been read out( by reading VEPU_ST_SLEN). */
        RK_U32  sli_len                 : 25;
        RK_U32  reserved                : 7;
    } reg144;

    /*
     * ST_PNUM_P64
     * Address offset: 0x244 Access type: read only
     * Number of 64x64 inter predicted blocks
     */
    struct {
        /* Number of 64x64 inter predicted blocks. */
        RK_U32  pnum_p64                : 12;
        RK_U32  reserved                : 20;
    } reg145;

    /*
     * ST_PNUM_P32
     * Address offset: 0x248 Access type: read only
     * Number of 32x32 inter predicted blocks
     */
    struct {
        /* Number of 32x32 inter predicted blocks. */
        RK_U32  pnum_p32                : 14;
        RK_U32  reserved                : 18;
    } reg146;

    /*
     * ST_PNUM_P16
     * Address offset: 0x24C Access type: read only
     * Number of 16x16 inter predicted blocks
     */
    struct {
        /* Number of 16x16 inter predicted blocks. */
        RK_U32  pnum_p16                : 16;
        RK_U32  reserved                : 16;
    } reg147;

    /*
     * ST_PNUM_P8
     * Address offset: 0x250 Access type: read only
     * Number of 8x8 inter predicted blocks
     */
    struct {
        /* Number of 8x8 inter predicted blocks. */
        RK_U32  pnum_p8                 : 18;
        RK_U32  reserved                : 14;
    } reg148;

    /*
     * ST_PNUM_I32
     * Address offset: 0x254 Access type: read only
     * Number of 32x32 intra predicted blocks
     */
    struct {
        /* Number of 32x32 intra predicted blocks. */
        RK_U32  pnum_i32                : 14;
        RK_U32  reserved                : 18;
    } reg149;

    /*
     * ST_PNUM_I16
     * Address offset: 0x258 Access type: read only
     * Number of 16x16 intra predicted blocks
     */
    struct {
        /* Number of 16x16 intra predicted blocks. */
        RK_U32  pnum_i16                : 16;
        RK_U32  reserved                : 16;
    } reg150;

    /*
     * ST_PNUM_I8
     * Address offset: 0x25C Access type: read only
     * Number of 8x8 intra predicted blocks
     */
    struct {
        /* Number of 8x8 intra predicted blocks. */
        RK_U32  pnum_i8                 : 18;
        RK_U32  reserved                : 14;
    } reg151;

    /*
     * ST_PNUM_I4
     * Address offset: 0x260 Access type: read only
     * Number of 4x4 intra predicted blocks
     */
    struct {
        /* Number of 4x4 intra predicted blocks. */
        RK_U32  pnum_i4                 : 20;
        RK_U32  reserved                : 12;
    } reg152;

    /*
     * ST_B8_QP0~51
     * Address offset: 0x264~0x330 Access type: read only
     * Number of block8x8s with QP=0~51
     */
    struct {
        /*
         * Number of block8x8s with QP value.
         * HEVC CUs of which size are bigger that 8x8 are considered as
         * (CU_size/8)*(CU_size/8) clock8x8s;
         * while H.264 MB is considered as 4 block8x8s.
         */
        Vepu541B8NumQp num_qp[52];
    } reg153_204;

    /*
     * ST_CPLX_TMP
     * Address offset: 0x334 Access type: read only
     * Temporal complexity(MADP) for current encoding and reference frame
     */
    struct {
        /* Mean absolute differences between current encoding and reference frame. */
        RK_U32  madp;
    } reg205;

    /*
     * ST_BNUM_CME
     * Address offset: 0x338 Access type: read only
     * Number of CME blocks in frame.
     * H.264: number CME blocks (4 MBs) in 16x64 aligned extended frame,
     * except for the CME blocks configured as force intra.
     * HEVC : number CME blocks (CTU) in 64x64 aligned extended frame,
     * except for the CME blocks configured as force intra.
     */
    struct {
        /* Number of CTU (HEVC: 64x64; H.264: 64x16) for CME inter-frame prediction. */
        RK_U32  num_ctu                 : 16;
        RK_U32  reserved                : 16;
    } reg206;

    /*
     * ST_CPLX_SPT
     * Address offset: 0x33C Access type: read only
     * Spatial complexity(MADI) for current encoding frame
     */
    struct {
        /* Mean absolute differences for current encoding frame. */
        RK_U32  madi;
    } reg207;

    /*
     * ST_BNUM_B16
     * Address offset: 0x340 Access type: read only
     * Number of valid 16x16 blocks for one frame.
     */
    struct {
        /* Number of valid 16x16 blocks for one frame. */
        RK_U32  num_b16;
    } reg208;
} Vepu541H264eRegSet;

/* return register is a subset of the whole register. */
typedef struct Vepu541H264eRegRet_t {
    /*
     * INT_STA
     * Address offset: 0x001c Access type: read and write, write one to clear
     * VEPU interrupt status
     */
    struct {
        /* One frame encode finish interrupt status */
        RK_U32  enc_done_sta            : 1;
        /* Link table finish interrupt status */
        RK_U32  lkt_done_sta            : 1;
        /* Safe clear finish interrupt status */
        RK_U32  sclr_done_sta           : 1;
        /* One slice encode finish interrupt status */
        RK_U32  enc_slice_done_sta      : 1;
        /* Bit stream overflow interrupt status */
        RK_U32  oflw_done_sta           : 1;
        /* AXI write response fifo full interrupt status */
        RK_U32  brsp_done_sta           : 1;
        /* AXI write response channel error interrupt status */
        RK_U32  berr_done_sta           : 1;
        /* AXI read channel error interrupt status */
        RK_U32  rerr_done_sta           : 1;
        /* timeout error interrupt status */
        RK_U32  wdg_done_sta            : 1;
        RK_U32  reserved                : 23;
    } hw_status; /* reg007 */

    /*
     * ST_BSL
     * Address offset: 0x210 Access type: read only
     * Bit stream length for current frame
     */
    struct {
        /* Bit stream length for current frame. */
        RK_U32  bs_lgth                 : 27;
        RK_U32  reserved                : 5;
    } st_bsl; /* reg132 */

    /*
     * ST_SSE_L32
     * Address offset: 0x214 Access type: read only
     * Low 32 bits of encoding distortion (SSE)
     */
    struct {
        RK_U32  sse_l32;
    } st_sse_l32; /* reg133 */

    /*
     * ST_SSE_QP
     * Address offset: 0x218 Access type: read only
     * High 8 bits of encoding distortion (SSE) and sum of QP for the encoded frame
     */
    struct {
        /* Sum of QP for the encoded frame. */
        RK_U32  qp_sum                  : 22;
        RK_U32  reserved                : 2;
        /* High bits of encoding distortion(SSE). */
        RK_U32  sse_h8                  : 8;
    } st_sse_qp; /* reg134 */

    /*
     * ST_SAO
     * Address offset: 0x21C Access type: read only
     * Number of CTUs which adjusted by SAO
     */
    struct {
        /* Number of CTUs whose CHROMA component are adjusted by SAO. */
        RK_U32  sao_cnum                : 12;
        /* Number of CTUs whose LUMA component are adjusted by SAO. */
        RK_U32  sao_ynum                : 12;
        RK_U32  reserved                : 8;
    } st_sao; /* reg135 */

    /* reg gap 136~137 */
    RK_U32 reg_136_137[2];

    /*
     * ST_ENC
     * Address offset: 0x228 Access type: read only
     * VEPU working status
     */
    struct {
        /*
         * VEPU working status.
         * 2'h0: Idle.
         * 2'h1: Working in register conifguration mode.
         * 2'h2: Working in link table configuration mode.
         */
        RK_U32  st_enc                  : 2;
        /*
         * Status of safe clear.
         * 1'h0: Safe clear is finished or not started.
         * 1'h1: VEPU is performing safe clear.
         */
        RK_U32  st_sclr                 : 1;
        RK_U32  reserved                : 29;
    } st_enc; /* reg138 */

    /*
     * ST_LKT
     * Address offset: 0x22C Access type: read only
     * Status of link table mode encoding
     */
    struct {
        /* Number of frames has been encoded since link table mode started. */
        RK_U32  fnum_enc                : 8;
        /* Number of frames has been configured since link table mode started. */
        RK_U32  fnum_cfg                : 8;
        /*
         * Number of frames has been encoded since link table mode started,
         * updated only when corresponding link table node send interrupt
         * (VEPU_ENC_PIC_node_int==1).
         */
        RK_U32  fnum_int                : 8;
        RK_U32  reserved                : 8;
    } st_lkt; /* reg139 */

    /*
     * ST_NADR
     * Address offset: 0x230 Access type: read only
     * Address of the processing link table node
     */
    struct {
        /* High 28 bits of the address for the processing linke table node. */
        RK_U32  node_addr;
    } ST_NADR; /* reg140 */

    /*
     * ST_BSB
     * Address offset: 0x234 Access type: read only
     * Status of bit stream buffer
     */
    struct {
        /* High 28 bits of bit stream buffer write address. */
        RK_U32  bsbw_addr;
    } st_bsb; /* reg141 */

    /*
     * ST_BUS
     * Address offset: 0x238 Access type: read only
     * VEPU bus status
     */
    struct {
        /*
         * AXI write response idle.
         * [6]: Reconstructed picture channel (AXI0_WID==5)
         * [5]: ME information channel (AXI0_WID==4)
         * [4]: Co-located Mv channel (AXI0_WID==3)
         * [3]: Down-sampled picture channel (AXI0_WID==2)
         * [2]: Bit stream channel (AXI0_WID==1)
         * [1]: Link table node channel (AXI0_WID==0)
         * [0]: Reserved
         */
        RK_U32  axib_idl                : 7;
        /*
         * AXI write response outstanding overflow.
         * [6]: Reconstructed picture channel (AXI0_WID==5)
         * [5]: ME information channel (AXI0_WID==4)
         * [4]: Co-located Mv channel (AXI0_WID==3)
         * [3]: Down-sampled picture channel (AXI0_WID==2)
         * [2]: Bit stream channel (AXI0_WID==1)
         * [1]: Link table node channel (AXI0_WID==0)
         * [0]: Reserved.
         */
        RK_U32  axib_ovfl               : 7;
        /*
         * AXI write response error.
         * [6]: Reconstructed picture channel (AXI0_WID==5)
         * [5]: ME information channel (AXI0_WID==4)
         * [4]: Co-located Mv channel (AXI0_WID==3)
         * [3]: Down-sampled picture channel (AXI0_WID==2)
         * [2]: Bit stream channel (AXI0_WID==1)
         * [1]: Link table node channel (AXI0_WID==0)
         * [0]: Reserved.
         */
        RK_U32  axib_err               : 7;
        /*
         * AXI read error.
         * [5]: ROI configuration (AXI0_ARID==7)
         * [4]: Down-sampled picture (AXI0_ARID==6)
         * [3]: Co-located Mv (AXI0_ARID==5)
         * [2]: Link table (AXI0_ARID==4)
         * [1]: Reference picture (AXI0_ARID==1,2,3,8)
         * [0]: Video source load (AXI1)
         */
        RK_U32  axir_err                : 6;
        RK_U32  reserved                : 5;
    } st_bus; /* reg142 */

    /*
     * ST_SNUM
     * Address offset: 0x23C Access type: read only
     * Slice number status
     */
    RK_U32  st_slice_number;

    /*
     * ST_SLEN
     * Address offset: 0x240 Access type: read only
     * Status of slice length
     */
    RK_U32  st_slice_length;

    /*
     * ST_PNUM_P64
     * Address offset: 0x244 Access type: read only
     * Number of 64x64 inter predicted blocks
     */
    RK_U32  st_lvl64_inter_num;

    /*
     * ST_PNUM_P32
     * Address offset: 0x248 Access type: read only
     * Number of 32x32 inter predicted blocks
     */
    RK_U32  st_lvl32_inter_num;

    /*
     * ST_PNUM_P16
     * Address offset: 0x24C Access type: read only
     * Number of 16x16 inter predicted blocks
     */
    RK_U32  st_lvl16_inter_num;

    /*
     * ST_PNUM_P8
     * Address offset: 0x250 Access type: read only
     * Number of 8x8 inter predicted blocks
     */
    RK_U32  st_lvl8_inter_num;

    /*
     * ST_PNUM_I32
     * Address offset: 0x254 Access type: read only
     * Number of 32x32 intra predicted blocks
     */
    RK_U32  st_lvl32_intra_num;

    /*
     * ST_PNUM_I16
     * Address offset: 0x258 Access type: read only
     * Number of 16x16 intra predicted blocks
     */
    RK_U32  st_lvl16_intra_num;

    /*
     * ST_PNUM_I8
     * Address offset: 0x25C Access type: read only
     * Number of 8x8 intra predicted blocks
     */
    RK_U32  st_lvl8_intra_num;

    /*
     * ST_PNUM_I4
     * Address offset: 0x260 Access type: read only
     * Number of 4x4 intra predicted blocks
     */
    RK_U32  st_lvl4_intra_num;

    /*
     * ST_B8_QP0~51
     * Address offset: 0x264~0x330 Access type: read only
     * Number of block8x8s with QP=0~51
     */
    RK_U32  st_cu_num_qp[52];

    /*
     * ST_CPLX_TMP
     * Address offset: 0x334 Access type: read only
     * Temporal complexity(MADP) for current encoding and reference frame
     */
    RK_U32  st_madp;

    /*
     * ST_BNUM_CME
     * Address offset: 0x338 Access type: read only
     * Number of CME blocks in frame.
     * H.264: number CME blocks (4 MBs) in 16x64 aligned extended frame,
     * except for the CME blocks configured as force intra.
     * HEVC : number CME blocks (CTU) in 64x64 aligned extended frame,
     * except for the CME blocks configured as force intra.
     */
    RK_U32  st_ctu_num;

    /*
     * ST_CPLX_SPT
     * Address offset: 0x33C Access type: read only
     * Spatial complexity(MADI) for current encoding frame
     */
    RK_U32  st_madi;

    /*
     * ST_BNUM_B16
     * Address offset: 0x340 Access type: read only
     * Number of valid 16x16 blocks for one frame.
     */
    RK_U32  st_mb_num;
} Vepu541H264eRegRet;

#endif /* __HAL_H264E_VEPU541_REG_H__ */
