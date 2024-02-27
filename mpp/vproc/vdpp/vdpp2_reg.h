/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPP2_REG_H__
#define __VDPP2_REG_H__

#include "rk_type.h"

#define VDPP_REG_OFF_DMSR               (0x80)
#define VDPP_REG_OFF_DCI                (0xE0)
#define VDPP_REG_OFF_ES                 (0x0100)
#define VDPP_REG_OFF_SHARP              (0x0200)
#define VDPP_REG_OFF_YRGB_HOR_COE       (0x2000)
#define VDPP_REG_OFF_YRGB_VER_COE       (0x2200)
#define VDPP_REG_OFF_CBCR_HOR_COE       (0x2400)
#define VDPP_REG_OFF_CBCR_VER_COE       (0x2600)
#define VDPP_REG_OFF_ZME_COMMON         (0x2800)

struct vdpp2_reg {
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
            RK_U32 sw_vdpp_yuvout_diff_en       : 1;
            RK_U32 sw_reserved_3        : 1;
            RK_U32 sw_vdpp_dst_yuv_swap : 2;
            RK_U32 sw_reserved_4        : 2;
            RK_U32 sw_vdpp_debug_data_en: 1;
            RK_U32 sw_reserved_5        : 3;
            RK_U32 sw_vdpp_rst_protect_dis      : 1;
            RK_U32 sys_vdpp_sreset_p    : 1;
            RK_U32 sw_vdpp_init_dis     : 1;
            RK_U32 sw_reserved_6        : 1;
            RK_U32 sw_vdpp_dbmsr_en     : 1;
            RK_U32 sw_dci_en            : 1;
        } reg1;         // 0x0004

        struct {
            RK_U32 sw_vdpp_working_mode : 2;
        } reg2;         // 0x0008

        struct {
            RK_U32 sw_vdpp_arqos_en     : 1;
            RK_U32 sw_vdpp_awqos_en     : 1;
            RK_U32 sw_reserved_1        : 2;
            RK_U32 sw_ar_mmu_qos3       : 4;
        } reg3;         // 0x000C

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

        struct {
            RK_U32 sw_ar_y_qos1         : 4;
            RK_U32 sw_ar_y_qos2         : 4;
            RK_U32 sw_ar_uv_qos1        : 4;
            RK_U32 sw_ar_uv_qos2        : 4;
        } reg6;         // 0x0018

        struct {
            RK_U32 sw_aw_id7_qos        : 4;
            RK_U32 sw_aw_id8_qos        : 4;
            RK_U32 sw_aw_id9_qos        : 4;
            RK_U32 sw_aw_id10_qos       : 4;
        } reg7;         // 0x001c

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
            RK_U32 sw_vdpp_dst_vir_c_stride     : 16;
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

        struct {
            RK_U32 sw_vdpp_dst_pic_width_c      : 11;
            RK_U32 reserved1                    : 1;
            RK_U32 sw_vdpp_dst_right_redundant_c: 4;
            RK_U32 sw_vdpp_dst_pic_height_c     : 11;
        } reg16;         // 0x0040

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

    struct {
        struct {
            RK_U32 sw_dci_yrgb_addr     : 32;
        } reg0;          // 0x00E0

        struct {
            RK_U32 sw_dci_yrgb_vir_stride       : 16;
            RK_U32 sw_dci_yrgb_gather_num       : 4;
            RK_U32 sw_dci_yrgb_gather_en: 1;
        } reg1;          // 0x00E4

        struct {
            RK_U32 sw_vdpp_src_pic_width: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_vdpp_src_pic_height       : 12;
        } reg2;          // 0x00E8

        struct {
            RK_U32 sw_dci_data_format   : 3;
            RK_U32 sw_dci_csc_range     : 1;
            RK_U32 sw_dci_vsd_mode      : 2;
            RK_U32 sw_dci_hsd_mode      : 1;
            RK_U32 sw_dci_alpha_swap    : 1;
            RK_U32 sw_dci_rb_swap       : 1;
            RK_U32 reserved1            : 7;
            RK_U32 sw_dci_blk_hsize     : 8;
            RK_U32 sw_dci_blk_vsize     : 8;
        } reg3;          // 0x00EC

        struct {
            RK_U32 sw_dci_hist_addr     : 32;
        } reg4;          // 0x00F0

    } dci;               // offset: 0x10E0

    RK_U32 reg_5_7[3];

    struct {
        struct {
            RK_U32 es_enable         : 1;
        } reg0;          // 0x0100

        struct {
            RK_U32 flat_th           : 8;
            RK_U32 dir_th            : 8;
        } reg1;          // 0x0104

        struct {
            RK_U32 tan_hi_th         : 9;
            RK_U32 reserved1         : 7;
            RK_U32 tan_lo_th         : 9;
        } reg2;          // 0x0108

        struct {
            RK_U32 ep_chk_en         : 1;
            RK_U32 reserved1         : 7;
            RK_U32 mem_gat_en        : 1;
        } reg3;          // 0x010C

        struct {
            RK_U32 diff_gain0        : 16;
            RK_U32 diff_limit        : 16;
        } reg4;          // 0x0110

        struct {
            RK_U32 lut_x0            : 16;
            RK_U32 diff_gain1        : 16;
        } reg5;          // 0x0114

        struct {
            RK_U32 lut_x2            : 16;
            RK_U32 lut_x1            : 16;
        } reg6;          // 0x0118

        struct {
            RK_U32 lut_x4            : 16;
            RK_U32 lut_x3            : 16;
        } reg7;          // 0x011C

        struct {
            RK_U32 lut_x6            : 16;
            RK_U32 lut_x5            : 16;
        } reg8;          // 0x0120

        struct {
            RK_U32 lut_x8            : 16;
            RK_U32 lut_x7            : 16;
        } reg9;          // 0x0124

        struct {
            RK_U32 lut_y0            : 8;
            RK_U32 lut_y1            : 8;
            RK_U32 lut_y2            : 8;
            RK_U32 lut_y3            : 8;
        } reg10;         // 0x0128

        struct {
            RK_U32 lut_y4            : 8;
            RK_U32 lut_y5            : 8;
            RK_U32 lut_y6            : 8;
            RK_U32 lut_y7            : 8;
        } reg11;         // 0x012C

        struct {
            RK_U32 lut_y8            : 8;
        } reg12;         // 0x0130

        struct {
            RK_U32 lut_k0            : 8;
            RK_U32 lut_k1            : 8;
            RK_U32 lut_k2            : 8;
            RK_U32 lut_k3            : 8;
        } reg13;         // 0x0134

        struct {
            RK_U32 lut_k4            : 8;
            RK_U32 lut_k5            : 8;
            RK_U32 lut_k6            : 8;
            RK_U32 lut_k7            : 8;
        } reg14;         // 0x0138

        struct {
            RK_U32 wgt_decay         : 8;
            RK_U32 wgt_gain          : 8;
        } reg15;         // 0x013C

        struct {
            RK_U32 conf_mean_th      : 8;
            RK_U32 conf_cnt_th       : 4;
            RK_U32 reserved1         : 4;
            RK_U32 low_conf_ratio    : 8;
            RK_U32 low_conf_th       : 8;
        } reg16;         // 0x0140

        struct {
            RK_U32 ink_en            : 1;
            RK_U32 reserved1         : 3;
            RK_U32 ink_mode          : 4;
        } reg17;         // 0x0144

        RK_U32 reg_18_27[10];

        struct {
            RK_U32 in_rdy            : 1;
            RK_U32 reserved1         : 3;
            RK_U32 mem_in_vsync      : 1;
            RK_U32 reserved2         : 3;
            RK_U32 in_hsync          : 1;
            RK_U32 reserved3         : 3;
            RK_U32 in_vld            : 1;
            RK_U32 reserved4         : 3;
            RK_U32 mem_in_line_cnt   : 11;
        } reg28;         // 0x0170

        struct {
            RK_U32 in_pix            : 16;
            RK_U32 in_dir            : 6;
            RK_U32 reserved1         : 2;
            RK_U32 in_flat           : 2;
        } reg29;         // 0x0174

        struct {
            RK_U32 out_rdy           : 1;
            RK_U32 reserved1         : 3;
            RK_U32 out_vsync         : 1;
            RK_U32 reserved2         : 3;
            RK_U32 out_hsync         : 1;
            RK_U32 reserved3         : 3;
            RK_U32 out_vld           : 1;
            RK_U32 reserved4         : 3;
            RK_U32 out_line_cnt      : 11;
        } reg30;         // 0x0178

        struct {
            RK_U32 out_pix           : 16;
        } reg31;         // 0x017C

    } es;                // offset: 0x1100

    RK_U32 reg_32_63[32];

    struct {
        struct {
            RK_U32 sw_sharp_enable      : 1;
            RK_U32 sw_lti_enable        : 1;
            RK_U32 sw_cti_enable        : 1;
            RK_U32 sw_peaking_enable    : 1;
            RK_U32 sw_peaking_ctrl_enable       : 1;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_proc_enable  : 1;
            RK_U32 sw_shoot_ctrl_enable : 1;
            RK_U32 sw_gain_ctrl_enable  : 1;
            RK_U32 sw_color_adj_enable  : 1;
            RK_U32 sw_texture_adj_enable: 1;
            RK_U32 sw_coloradj_bypass_en: 1;
            RK_U32 sw_ink_enable        : 1;
            RK_U32 sw_sharp_redundent_bypass    : 1;
        } reg0;          // 0x0200

        struct {
            RK_U32 sw_mem_gating_en     : 1;
            RK_U32 sw_lti_gating_en     : 1;
            RK_U32 sw_cti_gating_en     : 1;
            RK_U32 sw_peaking_gating_en : 1;
            RK_U32 sw_peaking_ctrl_gating_en    : 1;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_proc_gating_en       : 1;
            RK_U32 sw_shoot_ctrl_gating_en      : 1;
            RK_U32 sw_gain_ctrl_gating_en       : 1;
            RK_U32 sw_color_adj_gating_en       : 1;
            RK_U32 sw_texture_adj_gating_en     : 1;
        } reg1;          // 0x0204

        struct {
            RK_U32 sw_peaking_v00       : 4;
            RK_U32 sw_peaking_v01       : 4;
            RK_U32 sw_peaking_v02       : 4;
            RK_U32 sw_peaking_v10       : 4;
            RK_U32 sw_peaking_v11       : 4;
            RK_U32 sw_peaking_v12       : 4;
        } reg2;          // 0x0208

        struct {
            RK_U32 sw_peaking_v20       : 4;
            RK_U32 sw_peaking_v21       : 4;
            RK_U32 sw_peaking_v22       : 4;
            RK_U32 sw_peaking_usm0      : 4;
            RK_U32 sw_peaking_usm1      : 4;
            RK_U32 sw_peaking_usm2      : 4;
            RK_U32 sw_diag_coef         : 3;
        } reg3;          // 0x020C

        struct {
            RK_U32 sw_peaking_h00       : 6;
            RK_U32 reserved1            : 2;
            RK_U32 sw_peaking_h01       : 6;
            RK_U32 reserved2            : 2;
            RK_U32 sw_peaking_h02       : 6;
        } reg4;          // 0x0210

        RK_U32 reg5;

        struct {
            RK_U32 sw_peaking_h10       : 6;
            RK_U32 reserved1            : 2;
            RK_U32 sw_peaking_h11       : 6;
            RK_U32 reserved2            : 2;
            RK_U32 sw_peaking_h12       : 6;
        } reg6;          // 0x0218

        RK_U32 reg7;

        struct {
            RK_U32 sw_peaking_h20       : 6;
            RK_U32 reserved1            : 2;
            RK_U32 sw_peaking_h21       : 6;
            RK_U32 reserved2            : 2;
            RK_U32 sw_peaking_h22       : 6;
        } reg8;          // 0x0220

        RK_U32 reg_9_11[3];

        struct {
            RK_U32 sw_peaking0_idx_n0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_idx_n1   : 9;
        } reg12;         // 0x0230

        struct {
            RK_U32 sw_peaking0_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_idx_n3   : 9;
        } reg13;         // 0x0234

        struct {
            RK_U32 sw_peaking0_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_idx_p1   : 9;
        } reg14;         // 0x0238

        struct {
            RK_U32 sw_peaking0_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_idx_p3   : 9;
        } reg15;         // 0x023C

        struct {
            RK_U32 sw_peaking0_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_value_n2 : 9;
        } reg16;         // 0x0240

        struct {
            RK_U32 sw_peaking0_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_value_p1 : 9;
        } reg17;         // 0x0244

        struct {
            RK_U32 sw_peaking0_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking0_value_p3 : 9;
        } reg18;         // 0x0248

        struct {
            RK_U32 sw_peaking0_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking0_ratio_n12: 12;
        } reg19;         // 0x024C

        struct {
            RK_U32 sw_peaking0_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking0_ratio_p01: 12;
        } reg20;         // 0x0250

        struct {
            RK_U32 sw_peaking0_ratio_p12: 12;
            RK_U32 sw_peaking0_ratio_p23: 12;
        } reg21;         // 0x0254

        RK_U32 reg22;

        struct {
            RK_U32 sw_peaking1_idx_n0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_idx_n1   : 9;
        } reg23;         // 0x025C

        struct {
            RK_U32 sw_peaking1_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_idx_n3   : 9;
        } reg24;         // 0x0260

        struct {
            RK_U32 sw_peaking1_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_idx_p1   : 9;
        } reg25;         // 0x0264

        struct {
            RK_U32 sw_peaking1_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_idx_p3   : 9;
        } reg26;         // 0x0268

        struct {
            RK_U32 sw_peaking1_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_value_n2 : 9;
        } reg27;         // 0x026C

        struct {
            RK_U32 sw_peaking1_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_value_p1 : 9;
        } reg28;         // 0x0270

        struct {
            RK_U32 sw_peaking1_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking1_value_p3 : 9;
        } reg29;         // 0x0274

        struct {
            RK_U32 sw_peaking1_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking1_ratio_n12: 12;
        } reg30;         // 0x0278

        struct {
            RK_U32 sw_peaking1_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking1_ratio_p01: 12;
        } reg31;         // 0x027C

        struct {
            RK_U32 sw_peaking1_ratio_p12: 12;
            RK_U32 sw_peaking1_ratio_p23: 12;
        } reg32;         // 0x0280

        RK_U32 reg33;

        struct {
            RK_U32 sw_peaking2_idx_n0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_idx_n1   : 9;
        } reg34;         // 0x0288

        struct {
            RK_U32 sw_peaking2_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_idx_n3   : 9;
        } reg35;         // 0x028C

        struct {
            RK_U32 sw_peaking2_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_idx_p1   : 9;
        } reg36;         // 0x0290

        struct {
            RK_U32 sw_peaking2_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_idx_p3   : 9;
        } reg37;         // 0x0294

        struct {
            RK_U32 sw_peaking2_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_value_n2 : 9;
        } reg38;         // 0x0298

        struct {
            RK_U32 sw_peaking2_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_value_p1 : 9;
        } reg39;         // 0x029C

        struct {
            RK_U32 sw_peaking2_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking2_value_p3 : 9;
        } reg40;         // 0x02A0

        struct {
            RK_U32 sw_peaking2_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking2_ratio_n12: 12;
        } reg41;         // 0x02A4

        struct {
            RK_U32 sw_peaking2_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking2_ratio_p01: 12;
        } reg42;         // 0x02A8

        struct {
            RK_U32 sw_peaking2_ratio_p12: 12;
            RK_U32 sw_peaking2_ratio_p23: 12;
        } reg43;         // 0x02AC

        RK_U32 reg44;

        struct {
            RK_U32 sw_peaking3_idx_n0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_idx_n1   : 9;
        } reg45;         // 0x02B4

        struct {
            RK_U32 sw_peaking3_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_idx_n3   : 9;
        } reg46;         // 0x02B8

        struct {
            RK_U32 sw_peaking3_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_idx_p1   : 9;
        } reg47;         // 0x02BC

        struct {
            RK_U32 sw_peaking3_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_idx_p3   : 9;
        } reg48;         // 0x02C0

        struct {
            RK_U32 sw_peaking3_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_value_n2 : 9;
        } reg49;         // 0x02C4

        struct {
            RK_U32 sw_peaking3_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_value_p1 : 9;
        } reg50;         // 0x02C8

        struct {
            RK_U32 sw_peaking3_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking3_value_p3 : 9;
        } reg51;         // 0x02CC

        struct {
            RK_U32 sw_peaking3_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking3_ratio_n12: 12;
        } reg52;         // 0x02D0

        struct {
            RK_U32 sw_peaking3_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking3_ratio_p01: 12;
        } reg53;         // 0x02D4

        struct {
            RK_U32 sw_peaking3_ratio_p12: 12;
            RK_U32 sw_peaking3_ratio_p23: 12;
        } reg54;         // 0x02D8

        RK_U32 reg55;

        struct {
            RK_U32 sw_peaking4_idx_n0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_idx_n1   : 9;
        } reg56;         // 0x02E0

        struct {
            RK_U32 sw_peaking4_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_idx_n3   : 9;
        } reg57;         // 0x02E4

        struct {
            RK_U32 sw_peaking4_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_idx_p1   : 9;
        } reg58;         // 0x02E8

        struct {
            RK_U32 sw_peaking4_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_idx_p3   : 9;
        } reg59;         // 0x02EC

        struct {
            RK_U32 sw_peaking4_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_value_n2 : 9;
        } reg60;         // 0x02F0

        struct {
            RK_U32 sw_peaking4_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_value_p1 : 9;
        } reg61;         // 0x02F4

        struct {
            RK_U32 sw_peaking4_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking4_value_p3 : 9;
        } reg62;         // 0x02F8

        struct {
            RK_U32 sw_peaking4_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking4_ratio_n12: 12;
        } reg63;         // 0x02FC

        struct {
            RK_U32 sw_peaking4_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking4_ratio_p01: 12;
        } reg64;         // 0x0300

        struct {
            RK_U32 sw_peaking4_ratio_p12: 12;
            RK_U32 sw_peaking4_ratio_p23: 12;
        } reg65;         // 0x0304

        RK_U32 reg66;

        struct {
            RK_U32 sw_peaking5_idx_n0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_idx_n1   : 9;
        } reg67;         // 0x030C

        struct {
            RK_U32 sw_peaking5_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_idx_n3   : 9;
        } reg68;         // 0x0310

        struct {
            RK_U32 sw_peaking5_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_idx_p1   : 9;
        } reg69;         // 0x0314

        struct {
            RK_U32 sw_peaking5_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_idx_p3   : 9;
        } reg70;         // 0x0318

        struct {
            RK_U32 sw_peaking5_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_value_n2 : 9;
        } reg71;         // 0x031C

        struct {
            RK_U32 sw_peaking5_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_value_p1 : 9;
        } reg72;         // 0x0320

        struct {
            RK_U32 sw_peaking5_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking5_value_p3 : 9;
        } reg73;         // 0x0324

        struct {
            RK_U32 sw_peaking5_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking5_ratio_n12: 12;
        } reg74;         // 0x0328

        struct {
            RK_U32 sw_peaking5_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking5_ratio_p01: 12;
        } reg75;         // 0x032C

        struct {
            RK_U32 sw_peaking5_ratio_p12: 12;
            RK_U32 sw_peaking5_ratio_p23: 12;
        } reg76;         // 0x0330

        RK_U32 reg77;

        struct {
            RK_U32 sw_peaking6_idx_n0   : 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking6_idx_n1   : 12;
        } reg78;         // 0x0338

        struct {
            RK_U32 sw_peaking6_idx_n2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking6_idx_n3   : 9;
        } reg79;         // 0x033C

        struct {
            RK_U32 sw_peaking6_idx_p0   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking6_idx_p1   : 9;
        } reg80;         // 0x0340

        struct {
            RK_U32 sw_peaking6_idx_p2   : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking6_idx_p3   : 9;
        } reg81;         // 0x0344

        struct {
            RK_U32 sw_peaking6_value_n1 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking6_value_n2 : 9;
        } reg82;         // 0x0348

        struct {
            RK_U32 sw_peaking6_value_n3 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking6_value_p1 : 9;
        } reg83;         // 0x034C

        struct {
            RK_U32 sw_peaking6_value_p2 : 9;
            RK_U32 reserved1            : 7;
            RK_U32 sw_peaking6_value_p3 : 9;
        } reg84;         // 0x0350

        struct {
            RK_U32 sw_peaking6_ratio_n01: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking6_ratio_n12: 12;
        } reg85;         // 0x0354

        struct {
            RK_U32 sw_peaking6_ratio_n23: 12;
            RK_U32 reserved1            : 4;
            RK_U32 sw_peaking6_ratio_p01: 12;
        } reg86;         // 0x0358

        struct {
            RK_U32 sw_peaking6_ratio_p12: 12;
            RK_U32 sw_peaking6_ratio_p23: 12;
        } reg87;         // 0x035C

        RK_U32 reg_88_99[12];

        struct {
            RK_U32 sw_peaking_gain      : 10;
            RK_U32 reserved1            : 2;
            RK_U32 sw_nondir_thr        : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_dir_cmp_ratio     : 4;
            RK_U32 sw_nondir_wgt_ratio  : 5;
        } reg100;        // 0x0390

        struct {
            RK_U32 sw_nondir_wgt_offset : 8;
            RK_U32 sw_dir_cnt_thr       : 4;
            RK_U32 sw_dir_cnt_avg       : 3;
            RK_U32 reserved1            : 1;
            RK_U32 sw_dir_cnt_offset    : 4;
            RK_U32 sw_diag_dir_thr      : 7;
        } reg101;        // 0x0394

        struct {
            RK_U32 sw_diag_adjgain_tab0 : 4;
            RK_U32 sw_diag_adjgain_tab1 : 4;
            RK_U32 sw_diag_adjgain_tab2 : 4;
            RK_U32 sw_diag_adjgain_tab3 : 4;
            RK_U32 sw_diag_adjgain_tab4 : 4;
            RK_U32 sw_diag_adjgain_tab5 : 4;
            RK_U32 sw_diag_adjgain_tab6 : 4;
            RK_U32 sw_diag_adjgain_tab7 : 4;
        } reg102;        // 0x0398

        struct {
            RK_U32 sw_edge_alpha_over_non       : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_alpha_under_non      : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_edge_alpha_over_unlimit_non       : 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_edge_alpha_under_unlimit_non      : 7;
        } reg103;        // 0x039C

        struct {
            RK_U32 sw_edge_alpha_over_v : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_alpha_under_v: 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_edge_alpha_over_unlimit_v : 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_edge_alpha_under_unlimit_v: 7;
        } reg104;        // 0x03A0

        struct {
            RK_U32 sw_edge_alpha_over_h : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_alpha_under_h: 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_edge_alpha_over_unlimit_h : 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_edge_alpha_under_unlimit_h: 7;
        } reg105;        // 0x03A4

        struct {
            RK_U32 sw_edge_alpha_over_d0: 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_alpha_under_d0       : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_edge_alpha_over_unlimit_d0: 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_edge_alpha_under_unlimit_d0       : 7;
        } reg106;        // 0x03A8

        struct {
            RK_U32 sw_edge_alpha_over_d1: 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_edge_alpha_under_d1       : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_edge_alpha_over_unlimit_d1: 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_edge_alpha_under_unlimit_d1       : 7;
        } reg107;        // 0x03AC

        struct {
            RK_U32 sw_edge_delta_offset_non     : 8;
            RK_U32 sw_edge_delta_offset_v       : 8;
            RK_U32 sw_edge_delta_offset_h       : 8;
        } reg108;        // 0x03B0

        struct {
            RK_U32 sw_edge_delta_offset_d0      : 8;
            RK_U32 sw_edge_delta_offset_d1      : 8;
        } reg109;        // 0x03B4

        struct {
            RK_U32 sw_shoot_filt_radius : 1;
            RK_U32 reserved1            : 3;
            RK_U32 sw_shoot_delta_offset: 8;
            RK_U32 sw_shoot_alpha_over  : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_shoot_alpha_under : 7;
        } reg110;        // 0x03B8

        struct {
            RK_U32 sw_shoot_alpha_over_unlimit  : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_shoot_alpha_under_unlimit : 7;
        } reg111;        // 0x03BC

        struct {
            RK_U32 sw_adp_idx0          : 8;
            RK_U32 reserved1            : 2;
            RK_U32 sw_adp_idx1          : 8;
            RK_U32 reserved2            : 2;
            RK_U32 sw_adp_idx2          : 8;
        } reg112;        // 0x03C0

        struct {
            RK_U32 sw_adp_idx3          : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_adp_gain0         : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_adp_gain1         : 7;
        } reg113;        // 0x03C4

        struct {
            RK_U32 sw_adp_gain2         : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_adp_gain3         : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_adp_gain4         : 7;
        } reg114;        // 0x03C8

        struct {
            RK_U32 sw_adp_slp01         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_adp_slp12         : 11;
        } reg115;        // 0x03CC

        RK_U32 reg_116_127[12];

        struct {
            RK_U32 sw_adp_slp23         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_adp_slp34         : 11;
        } reg128;        // 0x0400

        struct {
            RK_U32 sw_adp_slp45         : 11;
            RK_U32 sw_var_idx0          : 8;
            RK_U32 reserved1            : 2;
            RK_U32 sw_var_idx1          : 8;
        } reg129;        // 0x0404

        struct {
            RK_U32 sw_var_idx2          : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_var_idx3          : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_var_gain0         : 7;
        } reg130;        // 0x0408

        struct {
            RK_U32 sw_var_gain1         : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_var_gain2         : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_var_gain3         : 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_var_gain4         : 7;
        } reg131;        // 0x040C

        struct {
            RK_U32 sw_var_slp01         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_var_slp12         : 11;
        } reg132;        // 0x0410

        struct {
            RK_U32 sw_var_slp23         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_var_slp34         : 11;
        } reg133;        // 0x0414

        struct {
            RK_U32 sw_var_slp45         : 11;
            RK_U32 reserved1            : 5;
            RK_U32 sw_lum_select        : 2;
            RK_U32 reserved2            : 2;
            RK_U32 sw_lum_idx0          : 8;
        } reg134;        // 0x0418

        struct {
            RK_U32 sw_lum_idx1          : 8;
            RK_U32 reserved1            : 2;
            RK_U32 sw_lum_idx2          : 8;
            RK_U32 reserved2            : 2;
            RK_U32 sw_lum_idx3          : 8;
        } reg135;        // 0x041C

        struct {
            RK_U32 sw_lum_gain0         : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_lum_gain1         : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_lum_gain2         : 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_lum_gain3         : 7;
        } reg136;        // 0x0420

        struct {
            RK_U32 sw_lum_gain4         : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_lum_slp01         : 11;
            RK_U32 reserved2            : 1;
            RK_U32 sw_lum_slp12         : 11;
        } reg137;        // 0x0424

        struct {
            RK_U32 sw_lum_slp23         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_lum_slp34         : 11;
        } reg138;        // 0x0428

        struct {
            RK_U32 sw_lum_slp45         : 11;
        } reg139;        // 0x042C

        struct {
            RK_U32 sw_adj_point_x0      : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_adj_point_y0      : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_adj_scaling_coef0 : 3;
        } reg140;        // 0x0430

        struct {
            RK_U32 sw_coloradj_tab0_0   : 5;
            RK_U32 sw_coloradj_tab0_1   : 5;
            RK_U32 sw_coloradj_tab0_2   : 5;
            RK_U32 sw_coloradj_tab0_3   : 5;
            RK_U32 sw_coloradj_tab0_4   : 5;
            RK_U32 sw_coloradj_tab0_5   : 5;
        } reg141;        // 0x0434

        struct {
            RK_U32 sw_coloradj_tab0_6   : 5;
            RK_U32 sw_coloradj_tab0_7   : 5;
            RK_U32 sw_coloradj_tab0_8   : 5;
            RK_U32 sw_coloradj_tab0_9   : 5;
            RK_U32 sw_coloradj_tab0_10  : 5;
            RK_U32 sw_coloradj_tab0_11  : 5;
        } reg142;        // 0x0438

        struct {
            RK_U32 sw_coloradj_tab0_12  : 5;
            RK_U32 sw_coloradj_tab0_13  : 5;
            RK_U32 sw_coloradj_tab0_14  : 5;
            RK_U32 sw_coloradj_tab0_15  : 5;
        } reg143;        // 0x043C

        struct {
            RK_U32 sw_adj_point_x1      : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_adj_point_y1      : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_adj_scaling_coef1 : 3;
        } reg144;        // 0x0440

        struct {
            RK_U32 sw_coloradj_tab1_0   : 5;
            RK_U32 sw_coloradj_tab1_1   : 5;
            RK_U32 sw_coloradj_tab1_2   : 5;
            RK_U32 sw_coloradj_tab1_3   : 5;
            RK_U32 sw_coloradj_tab1_4   : 5;
            RK_U32 sw_coloradj_tab1_5   : 5;
        } reg145;        // 0x0444

        struct {
            RK_U32 sw_coloradj_tab1_6   : 5;
            RK_U32 sw_coloradj_tab1_7   : 5;
            RK_U32 sw_coloradj_tab1_8   : 5;
            RK_U32 sw_coloradj_tab1_9   : 5;
            RK_U32 sw_coloradj_tab1_10  : 5;
            RK_U32 sw_coloradj_tab1_11  : 5;
        } reg146;        // 0x0448

        struct {
            RK_U32 sw_coloradj_tab1_12  : 5;
            RK_U32 sw_coloradj_tab1_13  : 5;
            RK_U32 sw_coloradj_tab1_14  : 5;
            RK_U32 sw_coloradj_tab1_15  : 5;
        } reg147;        // 0x044C

        struct {
            RK_U32 sw_adj_point_x2      : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_adj_point_y2      : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_adj_scaling_coef2 : 3;
        } reg148;        // 0x0450

        struct {
            RK_U32 sw_coloradj_tab2_0   : 5;
            RK_U32 sw_coloradj_tab2_1   : 5;
            RK_U32 sw_coloradj_tab2_2   : 5;
            RK_U32 sw_coloradj_tab2_3   : 5;
            RK_U32 sw_coloradj_tab2_4   : 5;
            RK_U32 sw_coloradj_tab2_5   : 5;
        } reg149;        // 0x0454

        struct {
            RK_U32 sw_coloradj_tab2_6   : 5;
            RK_U32 sw_coloradj_tab2_7   : 5;
            RK_U32 sw_coloradj_tab2_8   : 5;
            RK_U32 sw_coloradj_tab2_9   : 5;
            RK_U32 sw_coloradj_tab2_10  : 5;
            RK_U32 sw_coloradj_tab2_11  : 5;
        } reg150;        // 0x0458

        struct {
            RK_U32 sw_coloradj_tab2_12  : 5;
            RK_U32 sw_coloradj_tab2_13  : 5;
            RK_U32 sw_coloradj_tab2_14  : 5;
            RK_U32 sw_coloradj_tab2_15  : 5;
        } reg151;        // 0x045C

        struct {
            RK_U32 sw_adj_point_x3      : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_adj_point_y3      : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_adj_scaling_coef3 : 3;
        } reg152;        // 0x0460

        struct {
            RK_U32 sw_coloradj_tab3_0   : 5;
            RK_U32 sw_coloradj_tab3_1   : 5;
            RK_U32 sw_coloradj_tab3_2   : 5;
            RK_U32 sw_coloradj_tab3_3   : 5;
            RK_U32 sw_coloradj_tab3_4   : 5;
            RK_U32 sw_coloradj_tab3_5   : 5;
        } reg153;        // 0x0464

        struct {
            RK_U32 sw_coloradj_tab3_6   : 5;
            RK_U32 sw_coloradj_tab3_7   : 5;
            RK_U32 sw_coloradj_tab3_8   : 5;
            RK_U32 sw_coloradj_tab3_9   : 5;
            RK_U32 sw_coloradj_tab3_10  : 5;
            RK_U32 sw_coloradj_tab3_11  : 5;
        } reg154;        // 0x0468

        struct {
            RK_U32 sw_coloradj_tab3_12  : 5;
            RK_U32 sw_coloradj_tab3_13  : 5;
            RK_U32 sw_coloradj_tab3_14  : 5;
            RK_U32 sw_coloradj_tab3_15  : 5;
        } reg155;        // 0x046C

        struct {
            RK_U32 sw_idxmode_select    : 1;
            RK_U32 sw_ymode_select      : 2;
            RK_U32 reserved1            : 1;
            RK_U32 sw_tex_idx0          : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_tex_idx1          : 8;
        } reg156;        // 0x0470

        struct {
            RK_U32 sw_tex_idx2          : 8;
            RK_U32 reserved1            : 4;
            RK_U32 sw_tex_idx3          : 8;
            RK_U32 reserved2            : 4;
            RK_U32 sw_tex_gain0         : 7;
        } reg157;        // 0x0474

        struct {
            RK_U32 sw_tex_gain1         : 7;
            RK_U32 reserved1            : 1;
            RK_U32 sw_tex_gain2         : 7;
            RK_U32 reserved2            : 1;
            RK_U32 sw_tex_gain3         : 7;
            RK_U32 reserved3            : 1;
            RK_U32 sw_tex_gain4         : 7;
        } reg158;        // 0x0478

        struct {
            RK_U32 sw_tex_slp01         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_tex_slp12         : 11;
        } reg159;        // 0x047C

        struct {
            RK_U32 sw_tex_slp23         : 11;
            RK_U32 reserved1            : 1;
            RK_U32 sw_tex_slp34         : 11;
        } reg160;        // 0x0480

        struct {
            RK_U32 sw_tex_slp45         : 11;
        } reg161;        // 0x0484

        struct {
            RK_U32 sw_ltih_radius       : 1;
            RK_U32 reserved1            : 3;
            RK_U32 sw_ltih_slp1         : 9;
            RK_U32 reserved2            : 3;
            RK_U32 sw_ltih_thr1         : 9;
        } reg162;        // 0x0488

        struct {
            RK_U32 sw_ltih_noisethrneg  : 10;
            RK_U32 reserved1            : 2;
            RK_U32 sw_ltih_noisethrpos  : 10;
            RK_U32 reserved2            : 2;
            RK_U32 sw_ltih_tigain       : 5;
        } reg163;        // 0x048C

        struct {
            RK_U32 sw_ltiv_radius       : 1;
            RK_U32 reserved1            : 3;
            RK_U32 sw_ltiv_slp1         : 9;
            RK_U32 reserved2            : 3;
            RK_U32 sw_ltiv_thr1         : 9;
        } reg164;        // 0x0490

        struct {
            RK_U32 sw_ltiv_noisethrneg  : 10;
            RK_U32 reserved1            : 2;
            RK_U32 sw_ltiv_noisethrpos  : 10;
            RK_U32 reserved2            : 2;
            RK_U32 sw_ltiv_tigain       : 5;
        } reg165;        // 0x0494

        struct {
            RK_U32 sw_ctih_radius       : 1;
            RK_U32 reserved1            : 3;
            RK_U32 sw_ctih_slp1         : 9;
            RK_U32 reserved2            : 3;
            RK_U32 sw_ctih_thr1         : 9;
        } reg166;        // 0x0498

        struct {
            RK_U32 sw_ctih_noisethrneg  : 10;
            RK_U32 reserved1            : 2;
            RK_U32 sw_ctih_noisethrpos  : 10;
            RK_U32 reserved2            : 2;
            RK_U32 sw_ctih_tigain       : 5;
        } reg167;        // 0x049C

        RK_U32 reg_168_169[2];

        struct {
            RK_U32 sw_ink_mode          : 4;
        } reg170;        // 0x04A8

        RK_U32 reg171;

    } sharp;             // offset: 0x1200

};

#endif
