/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_JPEGE_VPU720_REG_H__
#define __HAL_JPEGE_VPU720_REG_H__

#include "rk_type.h"

typedef struct JpegeVpu720BaseReg_t {
    // 0x0000, IP version
    RK_U32 reg000_version;
    // 0x0004, Start Command
    struct {
        // Number of new nodes added to link table
        RK_U32 lkt_num                  : 8;
        /**
         * @brief VEPU command
         * 0 -- N/A
         * 1 -- One-frame encoding by register configuration
         * 2 -- Multi-frame encoding start with link table mode
         * 3 -- Multi-frame encoding update (with link table mode)
         * 4 -- link table encoding force pause
         * 5 -- continue link table encoder when link table stop
         * 6 -- safe_clr
         * 7 -- force_clr
         */
        RK_U32 vepu_cmd                 : 4;
        RK_U32                          : 20;
    } reg001_enc_strt;

    /*reserved 0x8 ~ 0xC*/
    RK_U32 reg002_003[2];

    // 0x0010, Interrupt Enable
    struct {
        // One frame encoding finish interrupt enable
        RK_U32 fenc_done_en             : 1;
        // Link table one node finish interrupt enable
        RK_U32 lkt_node_done_en         : 1;
        // Safe clear finish interrupt enable
        RK_U32 sclr_done_en             : 1;
        // One slice of video encoding finish interrupt enable
        RK_U32 vslc_done_en             : 1;
        // Video bit stream buffer overflow interrupt enable
        RK_U32 vbsb_oflw_en             : 1;
        // Video bit stream buffer write section interrupt enable
        RK_U32 vbsb_sct_en              : 1;
        // Frame encoding error interrupt enable
        RK_U32 fenc_err_en              : 1;
        // Watch dog (timeout) interrupt enable
        RK_U32 wdg_en                   : 1;
        // Link table operation error interrupt enable
        RK_U32 lkt_oerr_en              : 1;
        // Link table encoding error stop interrupt enable
        RK_U32 lkt_estp_en              : 1;
        // Link cmd force pause interrupt enable
        RK_U32 lkt_fstp_en              : 1;
        // Link table note stop interrupt enable
        RK_U32 lkt_note_stp_en          : 1;
        RK_U32 lkt_data_error_en        : 1;
        RK_U32                          : 19;
    } reg004_int_en;

    // 0x0014, Interrupt Mask
    struct {
        RK_U32 fenc_done_msk            : 1;
        RK_U32 lkt_node_done_msk        : 1;
        RK_U32 sclr_done_msk            : 1;
        RK_U32 vslc_done_msk            : 1;
        RK_U32 vbsb_oflw_msk            : 1;
        RK_U32 vbsb_sct_msk             : 1;
        RK_U32 fenc_err_msk             : 1;
        RK_U32 wdg_msk                  : 1;
        RK_U32 lkt_oerr_msk             : 1;
        RK_U32 lkt_estp_msk             : 1;
        RK_U32 lkt_fstp_msk             : 1;
        RK_U32 lkt_note_stp_msk         : 1;
        RK_U32 lkt_data_error_msk       : 1;
        RK_U32                          : 19;
    } reg005_int_msk;

    // 0x0018, Interrupt Clear
    struct {
        RK_U32 fenc_done_clr            : 1;
        RK_U32 lkt_node_done_clr        : 1;
        RK_U32 sclr_done_clr            : 1;
        RK_U32 vslc_done_clr            : 1;
        RK_U32 vbsb_oflw_clr            : 1;
        RK_U32 vbsb_sct_clr             : 1;
        RK_U32 fenc_err_clr             : 1;
        RK_U32 wdg_clr                  : 1;
        RK_U32 lkt_oerr_clr             : 1;
        RK_U32 lkt_estp_clr             : 1;
        RK_U32 lkt_fstp_clr             : 1;
        RK_U32 lkt_note_stp_clr         : 1;
        RK_U32 lkt_data_error_clr       : 1;
        RK_U32                          : 19;
    } reg006_int_clr;

    // 0x001c, Interrupt State;
    struct {
        RK_U32 fenc_done_state          : 1;
        RK_U32 lkt_node_done_state      : 1;
        RK_U32 sclr_done_state          : 1;
        RK_U32 vslc_done_state          : 1;
        RK_U32 vbsb_oflw_state          : 1;
        RK_U32 vbsb_sct_state           : 1;
        RK_U32 fenc_err_state           : 1;
        RK_U32 wdg_state                : 1;
        RK_U32 lkt_oerr_state           : 1;
        RK_U32 lkt_estp_state           : 1;
        RK_U32 lkt_fstp_state           : 1;
        RK_U32 lkt_note_stp_state       : 1;
        RK_U32 lkt_data_error_state     : 1;
        RK_U32                          : 19;
    } reg007_int_state;

    // 0x0020, Clock and RST CTRL
    struct {
        // Encoder auto reset core clock domain when frame finished
        RK_U32 resetn_hw_en             : 1;
        // Encoder SRAM auto clock gating enable
        RK_U32 sram_ckg_en              : 1;
        // Auto clock gating enable
        RK_U32 cke                      : 1;
        RK_U32                          : 29;
    } reg008_cru_ctrl;

    // 0x0024, Fast link table cfg buffer addr, 128 byte aligned
    RK_U32 reg009_lkt_base_addr;

    // 0x0028, Link table node operation configuration
    struct {
        // Only the ejpeg with the same core ID can use this node
        RK_U32 core_id                  : 4;
        // The enable of lkt error stop next frame
        RK_U32 lkt_err_stop_en          : 1;
        // The enable of lkt frame stop when the frame end
        RK_U32 lkt_node_stop_en         : 1;
        RK_U32                          : 10;
        /**
         * @brief  Data swap for link table read channel.
         * bit[3] -- swap 64 bits in 128 bits
         * bit[2] -- swap 32 bits in 64 bits;
         * bit[1] -- swap 16 bits in 32 bits;
         * bit[0] -- swap 8 bits in 16 bits;
         */
        RK_U32 lktr_bus_edin            : 4;
        /**
         * @brief
         * Data swap for link table write channel.
         * bit[3] -- swap 64 bits in 128 bits
         * bit[2] -- swap 32 bits in 64 bits;
         * bit[1] -- swap 16 bits in 32 bits;
         * bit[0] -- swap 8 bits in 16 bits;
         */
        RK_U32 lktw_bus_edin            : 4;
        RK_U32                          : 8;
    } reg010_node_ocfg;

    // 0x002c, watch dog configure register
    RK_U32 reg011_wdg_jpeg;

    // reserved, 0x0030
    RK_U32 reg012;

    // 0x0034, low delay esc operation configuration. Bit[0-30] : read ecs num.
    RK_U32 reg013_low_delay_ecs_ocfg;
    // 0x0038, low delay packet operation configuration. Bit[0-30] : read packet num.
    RK_U32 reg014_low_delay_packet_ocfg;

    // reserved, 0x003c
    RK_U32 reg015;

    // 0x0040, Address of JPEG Q table buffer, 128 byte aligned
    RK_U32 reg016_adr_qtbl;
    // 0x0044, Top address of JPEG Bit stream buffer, 16 byte aligned
    RK_U32 reg017_adr_bsbt;
    // 0x0048, Bottom address of JPEG bit stream buffer, 16 byte aligned
    RK_U32 reg018_adr_bsbb;
    // 0x004c, Read Address of JPEG bit stream buffer, 1 byte aligned
    RK_U32 reg019_adr_bsbr;
    // 0x0050, Start address of JPEG bit stream buffer, 1 byte aligned
    RK_U32 reg020_adr_bsbs;
    // 0x0054, Base address of ECS length buffer, 8 byte align
    RK_U32 reg021_adr_ecs_len;
    // 0x0058, Base address of the 1st storage area for video source buffer
    RK_U32 reg022_adr_src0;
    // 0x005c, Base address of the 2nd storage area for video source buffer
    RK_U32 reg023_adr_src1;
    // 0x0060, Base address of the 3rd storage area for video source buffer
    RK_U32 reg024_adr_src2;

    // reserved, 0x0064
    RK_U32 reg025;

    // 0x0068, rk jpeg encoder axi performance ctrl0 description
    struct {
        RK_U32 perf_work_e              : 1;
        RK_U32 perf_clr_e               : 1;
        RK_U32 perf_frm_type            : 1;
        RK_U32 cnt_type                 : 1;
        RK_U32 rd_latency_id            : 4;
        RK_U32 rd_latency_thr           : 12;
        RK_U32                          : 12;
    } reg026_axi_perf_ctrl0;

    // 0x006c, rk jpeg encoder axi performance ctrl1 description
    struct {
        RK_U32 addr_align_type          : 2;
        RK_U32 ar_cnt_id_type           : 1;
        RK_U32 aw_cnt_id_type           : 1;
        RK_U32 ar_count_id              : 4;
        RK_U32 aw_count_id              : 4;
        RK_U32 rd_total_bytes_mode      : 1;
        RK_U32                          : 19;
    } reg027_axi_perf_ctrl1;

    // 0x0070, reserved
    RK_U32 reg028;

    // 0x0074, picture size
    struct {
        // Ceil(encoding picture height / 8) -1
        RK_U32 pic_wd8_m1               : 13;
        RK_U32                          : 3;
        // Ceil(encoding picture height / 8) -1
        RK_U32 pic_hd8_m1               : 13;
        RK_U32                          : 3;
    } reg029_sw_enc_rsl;

    // 0x0078, JPEG source filling pixels for align
    struct {
        RK_U32 pic_wfill_jpeg           : 6;
        RK_U32                          : 10;
        RK_U32 pic_hfill_jpeg           : 6;
        RK_U32                          : 10;
    } reg030_sw_src_fill;

    /* reserved 0x7c */
    RK_U32 reg031;

    // 0x0080, JPEG source format
    struct {
        RK_U32                          : 1;
        RK_U32 rbuv_swap_jpeg           : 1;
        /**
         * @brief srouce color format
         * 4'h0: tile400
         * 4'h1: tile420
         * 4'h2: tile422
         * 4'h3: tile444
         * 4'h4: YUV422SP
         * 4'h5: YUV422P
         * 4'h6: YUV420SP
         * 4'h7: YUV420P
         * 4'h8: YUYV422
         * 4'h9: UYVY422
         * 4'ha: YUV400
         * 4'hc: YUV444SP
         * 4'hd: YUV444P
         * Others: Reserved
         */
        RK_U32 src_fmt                  : 4;
        /**
         * @brief color format of output from preprocess
         * 2'h0: YUV400;
         * 2'h1: YUV420;
         * 2'h2: YUV422;
         * 2'h3: YUV444;
         *
        */
        RK_U32 out_fmt                  : 2;
        RK_U32                          : 1;
        RK_U32 src_range_trns_en        : 1;
        RK_U32 src_range_trns_sel       : 1;
        /**
         * @brief Chroma downsample mode
         * 0 -- Average
         * 1 -- Drop
         */
        RK_U32 chroma_ds_mode           : 1;
        // Chroma value will be force to some value
        RK_U32 chroma_force_en          : 1;
        RK_U32                          : 2;
        // 1 00 src mirror image
        RK_U32 src_mirr_jpeg            : 1;
        RK_U32 u_force_value            : 8;
        RK_U32 v_force_value            : 8;
    } reg032_sw_src_fmt;

    // 0x0084, encoding picture offset
    struct {
        RK_U32 pic_ofst_x               : 16;
        RK_U32 pic_ofst_y               : 16;
    } reg033_sw_pic_ofst;

    // 0x0088, JPEG source stride0
    struct {
        RK_U32 src_strd_0               : 20;
        RK_U32                          : 12;
    } reg034_sw_src_strd_0;

    // 0x008c, JPEG source stride1
    struct {
        RK_U32 src_strd_1               : 19;
        RK_U32                          : 13;
    } reg035_sw_src_strd_1;

    // 0x0090, JPEG common config
    struct {
        /* the number of MCU in the restart interval */
        RK_U32 rst_intv                 : 16;
        RK_U32                          : 9;
        /**
         * @brief JPEG encoder output mode
         * 1'b0: frame by frame, without interrupt at any ECS;
         * 1'b1: low latency mode, with interrupt per ECS, flush all the
         *       bit stream after each ECS finished.
         */
        RK_U32 out_mode                 : 1;
        /* the number of the fisrt RSTm */
        RK_U32 rst_m                    : 3;
        /**
         * @brief Indicate if the current ECS is the last ECS of the whole picture.
         * If it is the last ecs, add EOI.
         */
        RK_U32 pic_last_ecs             : 1;
        /**
         * @brief reload Q table or not
         * 0 -- load Q table for current task
         * 1 -- no need to load Q table
         */
        RK_U32 jpeg_qtble_noload        : 1;
        RK_U32                          : 1;
    } reg036_sw_jpeg_enc_cfg;

    // 0x0094, Low dealy packet size config
    RK_U32 reg037_bsp_size_jpeg;

    // 0x0098, Bit stream output padding config
    struct {
        RK_U32 uvc_partition0_len       : 12;
        RK_U32 uvc_partition_len        : 12;
        RK_U32 uvc_skip_len             : 6;
        RK_U32                          : 2;
    } reg038_sw_uvc_cfg;

    // 0x009c, Y Quantify rounding
    struct {
        /* bias for Y at quantization */
        RK_U32 bias_y                   : 15;
        RK_U32                          : 17;
    } reg039_sw_jpeg_y_cfg;

    // 0x00a0, U Quantify rounding
    struct {

        /* bias for U at quantization */
        RK_U32 bias_u                   : 15;
        RK_U32                          : 17;
    } reg040_sw_jpeg_u_cfg;

    // 0x00a4, V Quantify rounding
    struct {
        /* bias for V at quantization */
        RK_U32 bias_v                   : 15;
        RK_U32                          : 17;
    } reg041_sw_jpeg_v_cfg;

    // 0x00a8, Data bus endian
    struct {
        /**
         * @brief Data swap for jpeg bit stream write channel
         * [3]: Swap 64 bits in 128 bits
         * [2]: Swap 32 bits in 64 bits
         * [1]: Swap 16 bits in 32 bits
         * [0]: Swap 8 bits in 16 bits
         */
        RK_U32 jbsw_bus_edin            : 4;
        // Data swap for video source loading channel.
        RK_U32 vsl_bus_edin             : 4;
        // Data swap for lkt state write channel
        RK_U32 ecs_len_edin             : 4;
        // Data swap for qtbl read channel
        RK_U32 sw_qtbl_edin             : 4;
    } reg042_dbus_endn;

} JpegeVpu720BaseReg;

typedef struct JpegeVpu720StatusReg_t {
    // 0x00c0, Low 32 bits of JPEG header bits length.
    RK_U32 st_bsl_l32_jpeg_head_bits;
    // 0x00c4, High 32 bits of JPEG header bits length
    RK_U32 st_bsl_h32_jpeg_head_bits;

    // 0x00c8, Y and U source range
    struct {
        RK_U32 y_max_value              : 8;
        RK_U32 y_min_value              : 8;
        RK_U32 u_max_value              : 8;
        RK_U32 u_min_vlaue              : 8;
    } st_vsp_value0;

    // 0x00cc, V source range and total_ecs_num_minus
    struct {
        RK_U32 v_max_value              : 8;
        RK_U32 v_min_vlaue              : 8;
        RK_U32 total_ecs_num_minus1     : 8;
    } st_vsp_value1;

    // 0x00d0, bit[0-15]
    RK_U32 st_perf_rd_max_latency_num0;
    // 0x00d4
    RK_U32 st_perf_rd_latency_samp_num;
    // 0x00d8
    RK_U32 st_perf_rd_latency_acc_sum;
    // 0x00dc
    RK_U32 st_perf_rd_axi_total_byte;
    // 0x00e0
    RK_U32 st_perf_wr_axi_total_byte;
    // 0x00e4
    RK_U32 st_perf_working_cnt;

    RK_U32 sw_reserved_00e8_00ec[2];

    // 0x00f0
    struct {
        RK_U32 vsp_work_flag            : 1;
        RK_U32 jpeg_core_work_flag      : 1;
        RK_U32 dma_wr_work_flag         : 1;
        RK_U32 dma_work_flag            : 1;
    } st_wdg;

    // 0x00f4
    RK_U32 st_ppl_pos;
    // 0x00f8
    RK_U32 st_core_pos;

    // 0x00fc, Bus status
    struct {
        RK_U32 ejpeg_arready            : 1;
        RK_U32 ejpeg_cfg_arvalid        : 1;
        RK_U32 ejpeg_cfg_arvalid_type   : 1;
        RK_U32 ejpeg_cfg_arready        : 1;
        RK_U32 ejpeg_vsp_arvalid        : 1;
        RK_U32 ejpeg_vsp_arready        : 1;
        RK_U32 rkejpeg_arvalid          : 1;
        RK_U32 ejpeg_cfg_ar_cnt         : 2;
        RK_U32 rkejpeg_arready          : 1;
        RK_U32 rkejpeg_ravlid           : 1;
        RK_U32 rkejpeg_rid              : 4;
        RK_U32 rkejpeg_rresp            : 2;
        RK_U32 rkejpeg_rready           : 1;
        RK_U32 axi_wr_state_cs          : 1;
        RK_U32 ejpeg_strmd_awvalid      : 1;
        RK_U32 ejpeg_strmd_awtype       : 1;
        RK_U32 ejpeg_strmd_awready      : 1;
        RK_U32 ejpeg_strmd_wvalid       : 1;
        RK_U32 ejpeg_strmd_wready       : 1;
        RK_U32 ejpeg_cfg_awvalid        : 1;
        RK_U32 ejpeg_cfg_awready        : 1;
        RK_U32 rkejpeg_awvalid          : 1;
        RK_U32 rkejpeg_awid             : 1;
        RK_U32 rkejpeg_wvalid           : 1;
        RK_U32 rkejpeg_wready           : 1;
        RK_U32 ejpeg_freeze_flag        : 1;
        RK_U32                          : 1;
    } st_bus;

    // 0x0100, vsp_dbg_status
    RK_U32 dbg_ppl;
    // 0x0104, jpeg core dbg status
    RK_U32 dbg_jpeg_core;
    // reserved, 0x0108
    RK_U32 sw_reserved_0108;
    // 0x010c, The bit stream write address status
    RK_U32 st_adr_jbsbw;
    // 0x0110, ECS length buffer write address status
    RK_U32 st_adr_ecs_len;
    // 0x0114, the 1st storage aread for video source buffer read address status
    RK_U32 st_adr_src0_jpeg;
    // 0x0118, the 2nd storage aread for video source buffer read address status
    RK_U32 st_adr_src1_jpeg;
    // 0x011c, the 3rd storage aread for video source buffer read address status
    RK_U32 st_adr_src2_jpeg;

    // 0x0120, low delay packet num status
    struct {
        RK_U32 bs_packet_num            : 16;
        RK_U32 bs_packet_lst            : 1;
    } st_low_delay_packet_num;

    // 0x0124, low delay ecs_len num status
    struct {
        RK_U32 ecs_len_num              : 16;
        RK_U32 ecs_len_lst              : 1;
    } st_ecs_len_num;

    // 0x0128, JPEG common status
    struct {
        /**
         * @brief
         * 0: idle
         * 1: cru open
         * 2: lkt cfg load
         * 3: qtbl_cfg_load
         * 4:enc
         * 5:frame end
         * 6:cru_close
         * 7:lkt_error_stop
         * 8:lkt_force_stop
         * 9:lkt_node_stop
         */
        RK_U32 jpeg_enc_state           : 4;
        RK_U32 lkt_mode_en              : 1;
    } st_enc;

    // 0x012c, Link table num
    struct {
        RK_U32 lkt_cfg_num              : 8;
        RK_U32 lkt_done_num             : 8;
        RK_U32 lkt_int_num              : 8;
        RK_U32 lkt_cfg_load_num         : 8;
    } st_lkt_num;

    // 0x0130, link table cfg info
    struct {
        RK_U32 lkt_core_id              : 4;
        RK_U32                          : 4;
        RK_U32 lkt_stop_flag            : 1;
        RK_U32 lkt_node_int             : 1;
        RK_U32 lkt_task_id              : 12;
        RK_U32                          : 10;
    } st_lkt_info;

    //0x0134, next read addr for lkt_cfg
    RK_U32 st_lkt_cfg_next_addr;
    //0x0138, lkt state buffer write addr
    RK_U32 st_lkt_waddr;
} JpegeVpu720StatusReg;


#define JPEGE_VPU720_REG_BASE_INT_STATE (0x1c)
#define JPEGE_VPU720_REG_STATUS_OFFSET (0xc0)

typedef struct JpegeVpu720RegSet_t {
    JpegeVpu720BaseReg reg_base;
    JpegeVpu720StatusReg reg_st;
    RK_U32 int_state;
} JpegeVpu720Reg;

#endif /* __HAL_JPEGE_VPU720_REG_H__ */