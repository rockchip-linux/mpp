/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPP_REG_H__
#define __VDPP_REG_H__

#include "rk_type.h"

#define VDPP_REG_OFF_DMSR               (0x80)
#define VDPP_REG_OFF_YRGB_HOR_COE       (0x2000)
#define VDPP_REG_OFF_YRGB_VER_COE       (0x2200)
#define VDPP_REG_OFF_CBCR_HOR_COE       (0x2400)
#define VDPP_REG_OFF_CBCR_VER_COE       (0x2600)
#define VDPP_REG_OFF_ZME_COMMON         (0x2800)

struct vdpp_reg {
    struct {

        struct {
            RK_U32 sw_vdpp_frm_en       : 1;
        } reg0;         // 0x0000

        struct {
            RK_U32 sw_vdpp_src_fmt      : 2;
            RK_U32 sw_reserved_1        : 2;
            RK_U32 sw_vdpp_src_yuv_swap : 2;
            RK_U32 sw_reserved_2        : 2;
            RK_U32 sw_vdpp_dst_fmt      : 2;
            RK_U32 sw_reserved_3        : 2;
            RK_U32 sw_vdpp_dst_yuv_swap : 2;
            RK_U32 sw_reserved_4        : 2;
            RK_U32 sw_vdpp_debug_data_en: 1;
            RK_U32 sw_reserved_5        : 3;
            RK_U32 sw_vdpp_rst_protect_dis      : 1;
            RK_U32 sys_vdpp_sreset_p    : 1;
            RK_U32 sw_vdpp_init_dis     : 1;
            RK_U32 sw_reserved_6        : 1;
            RK_U32 sw_vdpp_dbmsr_en     : 1;
        } reg1;         // 0x0004

        struct {
            RK_U32 sw_vdpp_working_mode : 2;
        } reg2;         // 0x0008

        RK_U32 reg3;    // 0x000C

        struct {
            RK_U32 sw_vdpp_clk_on       : 1;
            RK_U32 sw_md_clk_on         : 1;
            RK_U32 sw_dect_clk_on       : 1;
            RK_U32 sw_me_clk_on         : 1;
            RK_U32 sw_mc_clk_on         : 1;
            RK_U32 sw_eedi_clk_on       : 1;
            RK_U32 sw_ble_clk_on        : 1;
            RK_U32 sw_out_clk_on        : 1;
            RK_U32 sw_ctrl_clk_on       : 1;
            RK_U32 sw_ram_clk_on        : 1;
            RK_U32 sw_dma_clk_on        : 1;
            RK_U32 sw_reg_clk_on        : 1;
        } reg4;         // 0x0010

        struct {
            RK_U32 ro_arst_finish_done  : 1;
        } reg5;         // 0x0014

        RK_U32 reg6;    // 0x0018
        RK_U32 reg7;    // 0x001c

        struct {
            RK_U32 sw_vdpp_frm_done_en  : 1;
            RK_U32 sw_vdpp_osd_max_en   : 1;
            RK_U32 sw_reserved_1        : 2;
            RK_U32 sw_vdpp_bus_error_en : 1;
            RK_U32 sw_vdpp_timeout_int_en       : 1;
            RK_U32 sw_vdpp_config_error_en      : 1;
        } reg8;         // 0x0020

        struct {
            RK_U32 sw_vdpp_frm_done_clr : 1;
            RK_U32 sw_vdpp_osd_max_clr  : 1;
            RK_U32 sw_reserved_1        : 2;
            RK_U32 sw_vdpp_bus_error_clr: 1;
            RK_U32 sw_vdpp_timeout_int_clr      : 1;
            RK_U32 sw_vdpp_config_error_clr     : 1;
        } reg9;        // 0x0024

        struct {
            RK_U32 ro_frm_done_sts      : 1;
            RK_U32 ro_osd_max_sts       : 1;
            RK_U32 sw_reserved_1        : 2;
            RK_U32 ro_bus_error_sts     : 1;
            RK_U32 ro_timeout_sts       : 1;
            RK_U32 ro_config_error_sts  : 1;
        } reg10;        // 0x0028, read only

        struct {
            RK_U32 ro_frm_done_raw      : 1;
            RK_U32 ro_osd_max_raw       : 1;
            RK_U32 sw_reserved_1        : 2;
            RK_U32 ro_bus_error_raw     : 1;
            RK_U32 ro_timeout_raw       : 1;
            RK_U32 ro_config_error_raw  : 1;
        } reg11;        // 0x002C, read only

        struct {
            RK_U32 sw_vdpp_src_vir_y_stride     : 16;
        } reg12;         // 0x0030

        struct {
            RK_U32 sw_vdpp_dst_vir_y_stride     : 16;
        } reg13;         // 0x0034

        struct {
            RK_U32 sw_vdpp_src_pic_width        : 11;
            RK_U32 sw_reserved_1        : 1;
            RK_U32 sw_vdpp_src_right_redundant  : 4;
            RK_U32 sw_vdpp_src_pic_height       : 11;
            RK_U32 sw_reserved_2        : 1;
            RK_U32 sw_vdpp_src_down_redundant   : 3;
        } reg14;         // 0x0038

        struct {
            RK_U32 sw_vdpp_dst_pic_width        : 11;
            RK_U32 sw_reserved_1        : 1;
            RK_U32 sw_vdpp_dst_right_redundant  : 4;
            RK_U32 sw_vdpp_dst_pic_height       : 11;
        } reg15;         // 0x003C

        RK_U32 reg16;    // 0x0040
        RK_U32 reg17;    // 0x0044
        RK_U32 reg18;    // 0x0048
        RK_U32 reg19;    // 0x004C

        struct {
            RK_U32 sw_vdpp_timeout_cnt  : 31;
            RK_U32 sw_vdpp_timeout_en   : 1;
        } reg20;         // 0x0050

        struct {
            RK_U32 svnbuild     : 20;
            RK_U32 minor        : 8;
            RK_U32 major        : 4;
        } reg21;         // 0x0054

        struct {
            RK_U32 dbg_frm_cnt  : 16;
        } reg22;         // 0x0058

        RK_U32 reg23;    // 0x005C

        struct {
            RK_U32 sw_vdpp_src_addr_y   : 32;
        } reg24;         // 0x0060

        struct {
            RK_U32 sw_vdpp_src_addr_uv  : 32;
        } reg25;         // 0x0064

        struct {
            RK_U32 sw_vdpp_dst_addr_y   : 32;
        } reg26;         // 0x0068

        struct {
            RK_U32 sw_vdpp_dst_addr_uv  : 32;
        } reg27;         // 0x006C

    } common;            // offset: 0x1000
    RK_U32 reg_common_28_31[4];

};

#endif
