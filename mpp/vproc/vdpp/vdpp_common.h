/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __VDPP_COMMON_H__
#define __VDPP_COMMON_H__

#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_common.h"
#include "vdpp_api.h"

/* marco define */
#define VDPP_TILE_W_MAX     (120)
#define VDPP_TILE_H_MAX     (480)

#define SCALE_FACTOR_DN_FIXPOINT_SHIFT    (12)
#define SCALE_FACTOR_UP_FIXPOINT_SHIFT    (16)
#define GET_SCALE_FACTOR_DN(src,dst)      ((((src) - 1) << SCALE_FACTOR_DN_FIXPOINT_SHIFT)  / ((dst) - 1))
#define GET_SCALE_FACTOR_UP(src,dst)      ((((src) - 1) << SCALE_FACTOR_UP_FIXPOINT_SHIFT)  / ((dst) - 1))

enum ZME_FMT {
    FMT_YCbCr420_888    = 4,
    FMT_YCbCr444_888    = 6,
};

enum {
    SCL_NEI = 0,
    SCL_BIL = 1,
    SCL_BIC = 2,
    SCL_MPH = 3,
};

struct dmsr_params {
    bool   dmsr_enable;
    RK_U32 dmsr_str_pri_y;
    RK_U32 dmsr_str_sec_y;
    RK_U32 dmsr_dumping_y;
    RK_U32 dmsr_wgt_pri_gain_even_1;
    RK_U32 dmsr_wgt_pri_gain_even_2;
    RK_U32 dmsr_wgt_pri_gain_odd_1;
    RK_U32 dmsr_wgt_pri_gain_odd_2;
    RK_U32 dmsr_wgt_sec_gain;
    RK_U32 dmsr_blk_flat_th;
    RK_U32 dmsr_contrast_to_conf_map_x0;
    RK_U32 dmsr_contrast_to_conf_map_x1;
    RK_U32 dmsr_contrast_to_conf_map_y0;
    RK_U32 dmsr_contrast_to_conf_map_y1;
    RK_U32 dmsr_diff_core_th0;
    RK_U32 dmsr_diff_core_th1;
    RK_U32 dmsr_diff_core_wgt0;
    RK_U32 dmsr_diff_core_wgt1;
    RK_U32 dmsr_diff_core_wgt2;
    RK_U32 dmsr_edge_th_low_arr[7];
    RK_U32 dmsr_edge_th_high_arr[7];
};

struct dmsr_reg {

    struct {
        RK_U32 sw_dmsr_edge_low_thre_0      : 16;
        RK_U32 sw_dmsr_edge_high_thre_0     : 16;
    } reg0;         /* 0x0080 */

    struct {
        RK_U32 sw_dmsr_edge_low_thre_1      : 16;
        RK_U32 sw_dmsr_edge_high_thre_1     : 16;
    } reg1;         /* 0x0084 */

    struct {
        RK_U32 sw_dmsr_edge_low_thre_2      : 16;
        RK_U32 sw_dmsr_edge_high_thre_2     : 16;
    } reg2;         /* 0x0088 */

    struct {
        RK_U32 sw_dmsr_edge_low_thre_3      : 16;
        RK_U32 sw_dmsr_edge_high_thre_3     : 16;
    } reg3;         /* 0x008C */

    struct {
        RK_U32 sw_dmsr_edge_low_thre_4      : 16;
        RK_U32 sw_dmsr_edge_high_thre_4     : 16;
    } reg4;         /* 0x0090 */

    struct {
        RK_U32 sw_dmsr_edge_low_thre_5      : 16;
        RK_U32 sw_dmsr_edge_high_thre_5     : 16;
    } reg5;         /* 0x0094 */

    struct {
        RK_U32 sw_dmsr_edge_low_thre_6      : 16;
        RK_U32 sw_dmsr_edge_high_thre_6     : 16;
    } reg6;         /* 0x0098 */

    struct {
        RK_U32 sw_dmsr_edge_k_0     : 16;
        RK_U32 sw_dmsr_edge_k_1     : 16;
    } reg7;         /* 0x009C */

    struct {
        RK_U32 sw_dmsr_edge_k_2     : 16;
        RK_U32 sw_dmsr_edge_k_3     : 16;
    } reg8;         /* 0x00A0 */

    struct {
        RK_U32 sw_dmsr_edge_k_4     : 16;
        RK_U32 sw_dmsr_edge_k_5     : 16;
    } reg9;         /* 0x00A4 */

    struct {
        RK_U32 sw_dmsr_edge_k_6     : 16;
        RK_U32 sw_dmsr_dir_contrast_conf_f  : 16;
    } reg10;         /* 0x00A8 */

    struct {
        RK_U32 sw_dmsr_dir_contrast_conf_x0 : 16;
        RK_U32 sw_dmsr_dir_contrast_conf_x1  : 16;
    } reg11;         /* 0x00AC */

    struct {
        RK_U32 sw_dmsr_dir_contrast_conf_y0 : 16;
        RK_U32 sw_dmsr_dir_contrast_conf_y1 : 16;
    } reg12;         /* 0x00B0 */

    struct {
        RK_U32 sw_dmsr_var_th       : 16;
    } reg13;         /* 0x00B4 */

    struct {
        RK_U32 sw_dmsr_diff_coring_th0      : 8;
        RK_U32 sw_dmsr_diff_coring_th1      : 8;
    } reg14;         /* 0x00B8 */

    struct {
        RK_U32 sw_dmsr_diff_coring_wgt0     : 6;
        RK_U32 sw_reserved_1        : 2;
        RK_U32 sw_dmsr_diff_coring_wgt1     : 6;
        RK_U32 sw_reserved_2        : 2;
        RK_U32 sw_dmsr_diff_coring_wgt2     : 6;
    } reg15;         /* 0x00BC */

    struct {
        RK_U32 sw_dmsr_diff_coring_y0      : 14;
        RK_U32 sw_reserved_1        : 2;
        RK_U32 sw_dmsr_diff_coring_y1      : 14;
        RK_U32 sw_reserved_2        : 2;
    } reg16;         /* 0x00C0 */

    struct {
        RK_U32 sw_dmsr_wgt_pri_gain_1_odd   : 6;
        RK_U32 sw_reserved_1        : 2;
        RK_U32 sw_dmsr_wgt_pri_gain_1_even  : 6;
        RK_U32 sw_reserved_2        : 2;
        RK_U32 sw_dmsr_wgt_pri_gain_2_odd   : 6;
        RK_U32 sw_reserved_3        : 2;
        RK_U32 sw_dmsr_wgt_pri_gain_2_even  : 6;
    } reg17;         /* 0x00C4 */

    struct {
        RK_U32 sw_dmsr_wgt_sec_gain_1       : 6;
        RK_U32 sw_reserved_1        : 2;
        RK_U32 sw_dmsr_wgt_sec_gain_2       : 6;
    } reg18;         /* 0x00C8 */

    struct {
        RK_U32 sw_dmsr_strength_pri : 5;
        RK_U32 sw_reserved_1        : 3;
        RK_U32 sw_dmsr_strength_sec : 5;
        RK_U32 sw_reserved_2        : 3;
        RK_U32 sw_dmsr_dump         : 4;
    } reg19;         /* 0x00CC */

    struct {
        RK_U32 sw_dmsr_obv_point_h  : 12;
        RK_U32 sw_dmsr_obv_point_v  : 12;
        RK_U32 sw_dmsr_obv_enable   : 1;
        RK_U32 sw_dmsr_obv_mode     : 1;
    } reg20;         /* 0x00D0 */

    RK_U32 reg_dmsr_21_23[3];
};               /* offset: 0x1080 */


typedef struct {
    RK_U16 act_width;
    RK_U16 dsp_width;

    RK_U16 act_height;
    RK_U16 dsp_height;

    RK_U8  dering_en;

    RK_U8  xsd_en;
    RK_U8  xsu_en;
    RK_U8  xsd_bypass;
    RK_U8  xsu_bypass;
    RK_U8  xscl_mode;
    RK_U16 xscl_factor;
    RK_U8  xscl_offset;

    RK_U8  ysd_en;
    RK_U8  ysu_en;
    RK_U8  ys_bypass;
    RK_U8  yscl_mode;
    RK_U16 yscl_factor;
    RK_U8  yscl_offset;

    RK_U8  xavg_en;
    RK_U8  xgt_en;
    RK_U8  xgt_mode;

    RK_U8  yavg_en;
    RK_U8  ygt_en;
    RK_U8  ygt_mode;

    RK_S16 (*xscl_zme_coe)[8];
    RK_S16 (*yscl_zme_coe)[8];
} scl_info;

typedef struct FdTransInfo_t {
    RK_U32        reg_idx;
    RK_U32        offset;
} RegOffsetInfo;

struct vdpp_addr {
    RK_U32 y;
    RK_U32 cbcr;
    RK_U32 cbcr_offset;
};

struct zme_params {
    RK_U32 zme_bypass_en;
    RK_U32 zme_dering_enable;
    RK_U32 zme_dering_sen_0;
    RK_U32 zme_dering_sen_1;
    RK_U32 zme_dering_blend_alpha;
    RK_U32 zme_dering_blend_beta;
    RK_S16 (*zme_tap8_coeff)[17][8];
    RK_S16 (*zme_tap6_coeff)[17][8];

    /* for scl_info */
    RK_U32 src_width;
    RK_U32 src_height;
    RK_U32 dst_width;
    RK_U32 dst_height;
    RK_U32 dst_fmt;
    /* 3576 feature */
    RK_U32 yuv_out_diff;
    RK_U32 dst_c_width;
    RK_U32 dst_c_height;
};


struct zme_reg {
    struct {
        struct {
            RK_U32 yrgb_hor_coe0_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe0_1  : 10;
        } reg0;         /* 0x0000 */
        struct {
            RK_U32 yrgb_hor_coe0_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe0_3  : 10;
        } reg1;         /* 0x0004 */
        struct {
            RK_U32 yrgb_hor_coe0_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe0_5  : 10;
        } reg2;         /* 0x0008 */
        struct {
            RK_U32 yrgb_hor_coe0_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe0_7  : 10;
        } reg3;         /* 0x000c */
        struct {
            RK_U32 yrgb_hor_coe1_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe1_1  : 10;
        } reg4;         /* 0x0010 */
        struct {
            RK_U32 yrgb_hor_coe1_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe1_3  : 10;
        } reg5;         /* 0x0014 */
        struct {
            RK_U32 yrgb_hor_coe1_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe1_5  : 10;
        } reg6;         /* 0x0018 */
        struct {
            RK_U32 yrgb_hor_coe1_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe1_7  : 10;
        } reg7;         /* 0x001c */
        struct {
            RK_U32 yrgb_hor_coe2_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe2_1  : 10;
        } reg8;         /* 0x0020 */
        struct {
            RK_U32 yrgb_hor_coe2_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe2_3  : 10;
        } reg9;         /* 0x0024 */
        struct {
            RK_U32 yrgb_hor_coe2_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe2_5  : 10;
        } reg10;         /* 0x0028 */
        struct {
            RK_U32 yrgb_hor_coe2_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe2_7  : 10;
        } reg11;         /* 0x002c */
        struct {
            RK_U32 yrgb_hor_coe3_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe3_1  : 10;
        } reg12;         /* 0x0030 */
        struct {
            RK_U32 yrgb_hor_coe3_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe3_3  : 10;
        } reg13;         /* 0x0034 */
        struct {
            RK_U32 yrgb_hor_coe3_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe3_5  : 10;
        } reg14;         /* 0x0038 */
        struct {
            RK_U32 yrgb_hor_coe3_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe3_7  : 10;
        } reg15;         /* 0x003c */
        struct {
            RK_U32 yrgb_hor_coe4_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe4_1  : 10;
        } reg16;         /* 0x0040 */
        struct {
            RK_U32 yrgb_hor_coe4_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe4_3  : 10;
        } reg17;         /* 0x0044 */
        struct {
            RK_U32 yrgb_hor_coe4_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe4_5  : 10;
        } reg18;         /* 0x0048 */
        struct {
            RK_U32 yrgb_hor_coe4_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe4_7  : 10;
        } reg19;         /* 0x004c */
        struct {
            RK_U32 yrgb_hor_coe5_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe5_1  : 10;
        } reg20;         /* 0x0050 */
        struct {
            RK_U32 yrgb_hor_coe5_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe5_3  : 10;
        } reg21;         /* 0x0054 */
        struct {
            RK_U32 yrgb_hor_coe5_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe5_5  : 10;
        } reg22;         /* 0x0058 */
        struct {
            RK_U32 yrgb_hor_coe5_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe5_7  : 10;
        } reg23;         /* 0x005c */
        struct {
            RK_U32 yrgb_hor_coe6_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe6_1  : 10;
        } reg24;         /* 0x0060 */
        struct {
            RK_U32 yrgb_hor_coe6_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe6_3  : 10;
        } reg25;         /* 0x0064 */
        struct {
            RK_U32 yrgb_hor_coe6_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe6_5  : 10;
        } reg26;         /* 0x0068 */
        struct {
            RK_U32 yrgb_hor_coe6_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe6_7  : 10;
        } reg27;         /* 0x006c */
        struct {
            RK_U32 yrgb_hor_coe7_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe7_1  : 10;
        } reg28;         /* 0x0070 */
        struct {
            RK_U32 yrgb_hor_coe7_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe7_3  : 10;
        } reg29;         /* 0x0074 */
        struct {
            RK_U32 yrgb_hor_coe7_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe7_5  : 10;
        } reg30;         /* 0x0078 */
        struct {
            RK_U32 yrgb_hor_coe7_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe7_7  : 10;
        } reg31;         /* 0x007c */
        struct {
            RK_U32 yrgb_hor_coe8_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe8_1  : 10;
        } reg32;         /* 0x0080 */
        struct {
            RK_U32 yrgb_hor_coe8_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe8_3  : 10;
        } reg33;         /* 0x0084 */
        struct {
            RK_U32 yrgb_hor_coe8_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe8_5  : 10;
        } reg34;         /* 0x0088 */
        struct {
            RK_U32 yrgb_hor_coe8_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe8_7  : 10;
        } reg35;         /* 0x008c */
        struct {
            RK_U32 yrgb_hor_coe9_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe9_1  : 10;
        } reg36;         /* 0x0090 */
        struct {
            RK_U32 yrgb_hor_coe9_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe9_3  : 10;
        } reg37;         /* 0x0094 */
        struct {
            RK_U32 yrgb_hor_coe9_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe9_5  : 10;
        } reg38;         /* 0x0098 */
        struct {
            RK_U32 yrgb_hor_coe9_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe9_7  : 10;
        } reg39;         /* 0x009c */
        struct {
            RK_U32 yrgb_hor_coe10_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe10_1  : 10;
        } reg40;         /* 0x00a0 */
        struct {
            RK_U32 yrgb_hor_coe10_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe10_3  : 10;
        } reg41;         /* 0x00a4 */
        struct {
            RK_U32 yrgb_hor_coe10_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe10_5  : 10;
        } reg42;         /* 0x00a8 */
        struct {
            RK_U32 yrgb_hor_coe10_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe10_7  : 10;
        } reg43;         /* 0x00ac */
        struct {
            RK_U32 yrgb_hor_coe11_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe11_1  : 10;
        } reg44;         /* 0x00b0 */
        struct {
            RK_U32 yrgb_hor_coe11_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe11_3  : 10;
        } reg45;         /* 0x00b4 */
        struct {
            RK_U32 yrgb_hor_coe11_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe11_5  : 10;
        } reg46;         /* 0x00b8 */
        struct {
            RK_U32 yrgb_hor_coe11_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe11_7  : 10;
        } reg47;         /* 0x00bc */
        struct {
            RK_U32 yrgb_hor_coe12_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe12_1  : 10;
        } reg48;         /* 0x00c0 */
        struct {
            RK_U32 yrgb_hor_coe12_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe12_3  : 10;
        } reg49;         /* 0x00c4 */
        struct {
            RK_U32 yrgb_hor_coe12_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe12_5  : 10;
        } reg50;         /* 0x00c8 */
        struct {
            RK_U32 yrgb_hor_coe12_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe12_7  : 10;
        } reg51;         /* 0x00cc */
        struct {
            RK_U32 yrgb_hor_coe13_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe13_1  : 10;
        } reg52;         /* 0x00d0 */
        struct {
            RK_U32 yrgb_hor_coe13_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe13_3  : 10;
        } reg53;         /* 0x00d4 */
        struct {
            RK_U32 yrgb_hor_coe13_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe13_5  : 10;
        } reg54;         /* 0x00d8 */
        struct {
            RK_U32 yrgb_hor_coe13_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe13_7  : 10;
        } reg55;         /* 0x00dc */
        struct {
            RK_U32 yrgb_hor_coe14_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe14_1  : 10;
        } reg56;         /* 0x00e0 */
        struct {
            RK_U32 yrgb_hor_coe14_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe14_3  : 10;
        } reg57;         /* 0x00e4 */
        struct {
            RK_U32 yrgb_hor_coe14_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe14_5  : 10;
        } reg58;         /* 0x00e8 */
        struct {
            RK_U32 yrgb_hor_coe14_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe14_7  : 10;
        } reg59;         /* 0x00ec */
        struct {
            RK_U32 yrgb_hor_coe15_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe15_1  : 10;
        } reg60;         /* 0x00f0 */
        struct {
            RK_U32 yrgb_hor_coe15_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe15_3  : 10;
        } reg61;         /* 0x00f4 */
        struct {
            RK_U32 yrgb_hor_coe15_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe15_5  : 10;
        } reg62;         /* 0x00f8 */
        struct {
            RK_U32 yrgb_hor_coe15_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe15_7  : 10;
        } reg63;         /* 0x00fc */
        struct {
            RK_U32 yrgb_hor_coe16_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe16_1  : 10;
        } reg64;         /* 0x0100 */
        struct {
            RK_U32 yrgb_hor_coe16_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe16_3  : 10;
        } reg65;         /* 0x0104 */
        struct {
            RK_U32 yrgb_hor_coe16_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe16_5  : 10;
        } reg66;         /* 0x0108 */
        struct {
            RK_U32 yrgb_hor_coe16_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_hor_coe16_7  : 10;
        } reg67;         /* 0x010c */

    } yrgb_hor_coe;

    struct {
        struct {
            RK_U32 yrgb_ver_coe0_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe0_1  : 10;
        } reg0;         /* 0x0200 */
        struct {
            RK_U32 yrgb_ver_coe0_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe0_3  : 10;
        } reg1;         /* 0x0204 */
        struct {
            RK_U32 yrgb_ver_coe0_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe0_5  : 10;
        } reg2;         /* 0x0208 */
        struct {
            RK_U32 yrgb_ver_coe0_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe0_7  : 10;
        } reg3;         /* 0x020c */
        struct {
            RK_U32 yrgb_ver_coe1_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe1_1  : 10;
        } reg4;         /* 0x0210 */
        struct {
            RK_U32 yrgb_ver_coe1_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe1_3  : 10;
        } reg5;         /* 0x0214 */
        struct {
            RK_U32 yrgb_ver_coe1_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe1_5  : 10;
        } reg6;         /* 0x0218 */
        struct {
            RK_U32 yrgb_ver_coe1_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe1_7  : 10;
        } reg7;         /* 0x021c */
        struct {
            RK_U32 yrgb_ver_coe2_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe2_1  : 10;
        } reg8;         /* 0x0220 */
        struct {
            RK_U32 yrgb_ver_coe2_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe2_3  : 10;
        } reg9;         /* 0x0224 */
        struct {
            RK_U32 yrgb_ver_coe2_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe2_5  : 10;
        } reg10;         /* 0x0228 */
        struct {
            RK_U32 yrgb_ver_coe2_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe2_7  : 10;
        } reg11;         /* 0x022c */
        struct {
            RK_U32 yrgb_ver_coe3_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe3_1  : 10;
        } reg12;         /* 0x0230 */
        struct {
            RK_U32 yrgb_ver_coe3_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe3_3  : 10;
        } reg13;         /* 0x0234 */
        struct {
            RK_U32 yrgb_ver_coe3_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe3_5  : 10;
        } reg14;         /* 0x0238 */
        struct {
            RK_U32 yrgb_ver_coe3_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe3_7  : 10;
        } reg15;         /* 0x023c */
        struct {
            RK_U32 yrgb_ver_coe4_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe4_1  : 10;
        } reg16;         /* 0x0240 */
        struct {
            RK_U32 yrgb_ver_coe4_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe4_3  : 10;
        } reg17;         /* 0x0244 */
        struct {
            RK_U32 yrgb_ver_coe4_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe4_5  : 10;
        } reg18;         /* 0x0248 */
        struct {
            RK_U32 yrgb_ver_coe4_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe4_7  : 10;
        } reg19;         /* 0x024c */
        struct {
            RK_U32 yrgb_ver_coe5_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe5_1  : 10;
        } reg20;         /* 0x0250 */
        struct {
            RK_U32 yrgb_ver_coe5_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe5_3  : 10;
        } reg21;         /* 0x0254 */
        struct {
            RK_U32 yrgb_ver_coe5_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe5_5  : 10;
        } reg22;         /* 0x0258 */
        struct {
            RK_U32 yrgb_ver_coe5_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe5_7  : 10;
        } reg23;         /* 0x025c */
        struct {
            RK_U32 yrgb_ver_coe6_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe6_1  : 10;
        } reg24;         /* 0x0260 */
        struct {
            RK_U32 yrgb_ver_coe6_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe6_3  : 10;
        } reg25;         /* 0x0264 */
        struct {
            RK_U32 yrgb_ver_coe6_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe6_5  : 10;
        } reg26;         /* 0x0268 */
        struct {
            RK_U32 yrgb_ver_coe6_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe6_7  : 10;
        } reg27;         /* 0x026c */
        struct {
            RK_U32 yrgb_ver_coe7_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe7_1  : 10;
        } reg28;         /* 0x0270 */
        struct {
            RK_U32 yrgb_ver_coe7_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe7_3  : 10;
        } reg29;         /* 0x0274 */
        struct {
            RK_U32 yrgb_ver_coe7_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe7_5  : 10;
        } reg30;         /* 0x0278 */
        struct {
            RK_U32 yrgb_ver_coe7_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe7_7  : 10;
        } reg31;         /* 0x027c */
        struct {
            RK_U32 yrgb_ver_coe8_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe8_1  : 10;
        } reg32;         /* 0x0280 */
        struct {
            RK_U32 yrgb_ver_coe8_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe8_3  : 10;
        } reg33;         /* 0x0284 */
        struct {
            RK_U32 yrgb_ver_coe8_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe8_5  : 10;
        } reg34;         /* 0x0288 */
        struct {
            RK_U32 yrgb_ver_coe8_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe8_7  : 10;
        } reg35;         /* 0x028c */
        struct {
            RK_U32 yrgb_ver_coe9_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe9_1  : 10;
        } reg36;         /* 0x0290 */
        struct {
            RK_U32 yrgb_ver_coe9_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe9_3  : 10;
        } reg37;         /* 0x0294 */
        struct {
            RK_U32 yrgb_ver_coe9_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe9_5  : 10;
        } reg38;         /* 0x0298 */
        struct {
            RK_U32 yrgb_ver_coe9_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe9_7  : 10;
        } reg39;         /* 0x029c */
        struct {
            RK_U32 yrgb_ver_coe10_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe10_1  : 10;
        } reg40;         /* 0x02a0 */
        struct {
            RK_U32 yrgb_ver_coe10_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe10_3  : 10;
        } reg41;         /* 0x02a4 */
        struct {
            RK_U32 yrgb_ver_coe10_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe10_5  : 10;
        } reg42;         /* 0x02a8 */
        struct {
            RK_U32 yrgb_ver_coe10_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe10_7  : 10;
        } reg43;         /* 0x02ac */
        struct {
            RK_U32 yrgb_ver_coe11_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe11_1  : 10;
        } reg44;         /* 0x02b0 */
        struct {
            RK_U32 yrgb_ver_coe11_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe11_3  : 10;
        } reg45;         /* 0x02b4 */
        struct {
            RK_U32 yrgb_ver_coe11_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe11_5  : 10;
        } reg46;         /* 0x02b8 */
        struct {
            RK_U32 yrgb_ver_coe11_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe11_7  : 10;
        } reg47;         /* 0x02bc */
        struct {
            RK_U32 yrgb_ver_coe12_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe12_1  : 10;
        } reg48;         /* 0x02c0 */
        struct {
            RK_U32 yrgb_ver_coe12_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe12_3  : 10;
        } reg49;         /* 0x02c4 */
        struct {
            RK_U32 yrgb_ver_coe12_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe12_5  : 10;
        } reg50;         /* 0x02c8 */
        struct {
            RK_U32 yrgb_ver_coe12_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe12_7  : 10;
        } reg51;         /* 0x02cc */
        struct {
            RK_U32 yrgb_ver_coe13_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe13_1  : 10;
        } reg52;         /* 0x02d0 */
        struct {
            RK_U32 yrgb_ver_coe13_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe13_3  : 10;
        } reg53;         /* 0x02d4 */
        struct {
            RK_U32 yrgb_ver_coe13_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe13_5  : 10;
        } reg54;         /* 0x02d8 */
        struct {
            RK_U32 yrgb_ver_coe13_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe13_7  : 10;
        } reg55;         /* 0x02dc */
        struct {
            RK_U32 yrgb_ver_coe14_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe14_1  : 10;
        } reg56;         /* 0x02e0 */
        struct {
            RK_U32 yrgb_ver_coe14_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe14_3  : 10;
        } reg57;         /* 0x02e4 */
        struct {
            RK_U32 yrgb_ver_coe14_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe14_5  : 10;
        } reg58;         /* 0x02e8 */
        struct {
            RK_U32 yrgb_ver_coe14_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe14_7  : 10;
        } reg59;         /* 0x02ec */
        struct {
            RK_U32 yrgb_ver_coe15_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe15_1  : 10;
        } reg60;         /* 0x02f0 */
        struct {
            RK_U32 yrgb_ver_coe15_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe15_3  : 10;
        } reg61;         /* 0x02f4 */
        struct {
            RK_U32 yrgb_ver_coe15_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe15_5  : 10;
        } reg62;         /* 0x02f8 */
        struct {
            RK_U32 yrgb_ver_coe15_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe15_7  : 10;
        } reg63;         /* 0x02fc */
        struct {
            RK_U32 yrgb_ver_coe16_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe16_1  : 10;
        } reg64;         /* 0x0300 */
        struct {
            RK_U32 yrgb_ver_coe16_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe16_3  : 10;
        } reg65;         /* 0x0304 */
        struct {
            RK_U32 yrgb_ver_coe16_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe16_5  : 10;
        } reg66;         /* 0x0308 */
        struct {
            RK_U32 yrgb_ver_coe16_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 yrgb_ver_coe16_7  : 10;
        } reg67;         /* 0x030c */

    } yrgb_ver_coe;

    struct {
        struct {
            RK_U32 cbcr_hor_coe0_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe0_1  : 10;
        } reg0;         /* 0x0400 */
        struct {
            RK_U32 cbcr_hor_coe0_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe0_3  : 10;
        } reg1;         /* 0x0404 */
        struct {
            RK_U32 cbcr_hor_coe0_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe0_5  : 10;
        } reg2;         /* 0x0408 */
        struct {
            RK_U32 cbcr_hor_coe0_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe0_7  : 10;
        } reg3;         /* 0x040c */
        struct {
            RK_U32 cbcr_hor_coe1_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe1_1  : 10;
        } reg4;         /* 0x0410 */
        struct {
            RK_U32 cbcr_hor_coe1_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe1_3  : 10;
        } reg5;         /* 0x0414 */
        struct {
            RK_U32 cbcr_hor_coe1_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe1_5  : 10;
        } reg6;         /* 0x0418 */
        struct {
            RK_U32 cbcr_hor_coe1_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe1_7  : 10;
        } reg7;         /* 0x041c */
        struct {
            RK_U32 cbcr_hor_coe2_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe2_1  : 10;
        } reg8;         /* 0x0420 */
        struct {
            RK_U32 cbcr_hor_coe2_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe2_3  : 10;
        } reg9;         /* 0x0424 */
        struct {
            RK_U32 cbcr_hor_coe2_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe2_5  : 10;
        } reg10;         /* 0x0428 */
        struct {
            RK_U32 cbcr_hor_coe2_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe2_7  : 10;
        } reg11;         /* 0x042c */
        struct {
            RK_U32 cbcr_hor_coe3_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe3_1  : 10;
        } reg12;         /* 0x0430 */
        struct {
            RK_U32 cbcr_hor_coe3_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe3_3  : 10;
        } reg13;         /* 0x0434 */
        struct {
            RK_U32 cbcr_hor_coe3_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe3_5  : 10;
        } reg14;         /* 0x0438 */
        struct {
            RK_U32 cbcr_hor_coe3_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe3_7  : 10;
        } reg15;         /* 0x043c */
        struct {
            RK_U32 cbcr_hor_coe4_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe4_1  : 10;
        } reg16;         /* 0x0440 */
        struct {
            RK_U32 cbcr_hor_coe4_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe4_3  : 10;
        } reg17;         /* 0x0444 */
        struct {
            RK_U32 cbcr_hor_coe4_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe4_5  : 10;
        } reg18;         /* 0x0448 */
        struct {
            RK_U32 cbcr_hor_coe4_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe4_7  : 10;
        } reg19;         /* 0x044c */
        struct {
            RK_U32 cbcr_hor_coe5_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe5_1  : 10;
        } reg20;         /* 0x0450 */
        struct {
            RK_U32 cbcr_hor_coe5_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe5_3  : 10;
        } reg21;         /* 0x0454 */
        struct {
            RK_U32 cbcr_hor_coe5_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe5_5  : 10;
        } reg22;         /* 0x0458 */
        struct {
            RK_U32 cbcr_hor_coe5_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe5_7  : 10;
        } reg23;         /* 0x045c */
        struct {
            RK_U32 cbcr_hor_coe6_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe6_1  : 10;
        } reg24;         /* 0x0460 */
        struct {
            RK_U32 cbcr_hor_coe6_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe6_3  : 10;
        } reg25;         /* 0x0464 */
        struct {
            RK_U32 cbcr_hor_coe6_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe6_5  : 10;
        } reg26;         /* 0x0468 */
        struct {
            RK_U32 cbcr_hor_coe6_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe6_7  : 10;
        } reg27;         /* 0x046c */
        struct {
            RK_U32 cbcr_hor_coe7_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe7_1  : 10;
        } reg28;         /* 0x0470 */
        struct {
            RK_U32 cbcr_hor_coe7_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe7_3  : 10;
        } reg29;         /* 0x0474 */
        struct {
            RK_U32 cbcr_hor_coe7_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe7_5  : 10;
        } reg30;         /* 0x0478 */
        struct {
            RK_U32 cbcr_hor_coe7_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe7_7  : 10;
        } reg31;         /* 0x047c */
        struct {
            RK_U32 cbcr_hor_coe8_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe8_1  : 10;
        } reg32;         /* 0x0480 */
        struct {
            RK_U32 cbcr_hor_coe8_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe8_3  : 10;
        } reg33;         /* 0x0484 */
        struct {
            RK_U32 cbcr_hor_coe8_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe8_5  : 10;
        } reg34;         /* 0x0488 */
        struct {
            RK_U32 cbcr_hor_coe8_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe8_7  : 10;
        } reg35;         /* 0x048c */
        struct {
            RK_U32 cbcr_hor_coe9_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe9_1  : 10;
        } reg36;         /* 0x0490 */
        struct {
            RK_U32 cbcr_hor_coe9_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe9_3  : 10;
        } reg37;         /* 0x0494 */
        struct {
            RK_U32 cbcr_hor_coe9_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe9_5  : 10;
        } reg38;         /* 0x0498 */
        struct {
            RK_U32 cbcr_hor_coe9_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe9_7  : 10;
        } reg39;         /* 0x049c */
        struct {
            RK_U32 cbcr_hor_coe10_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe10_1  : 10;
        } reg40;         /* 0x04a0 */
        struct {
            RK_U32 cbcr_hor_coe10_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe10_3  : 10;
        } reg41;         /* 0x04a4 */
        struct {
            RK_U32 cbcr_hor_coe10_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe10_5  : 10;
        } reg42;         /* 0x04a8 */
        struct {
            RK_U32 cbcr_hor_coe10_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe10_7  : 10;
        } reg43;         /* 0x04ac */
        struct {
            RK_U32 cbcr_hor_coe11_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe11_1  : 10;
        } reg44;         /* 0x04b0 */
        struct {
            RK_U32 cbcr_hor_coe11_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe11_3  : 10;
        } reg45;         /* 0x04b4 */
        struct {
            RK_U32 cbcr_hor_coe11_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe11_5  : 10;
        } reg46;         /* 0x04b8 */
        struct {
            RK_U32 cbcr_hor_coe11_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe11_7  : 10;
        } reg47;         /* 0x04bc */
        struct {
            RK_U32 cbcr_hor_coe12_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe12_1  : 10;
        } reg48;         /* 0x04c0 */
        struct {
            RK_U32 cbcr_hor_coe12_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe12_3  : 10;
        } reg49;         /* 0x04c4 */
        struct {
            RK_U32 cbcr_hor_coe12_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe12_5  : 10;
        } reg50;         /* 0x04c8 */
        struct {
            RK_U32 cbcr_hor_coe12_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe12_7  : 10;
        } reg51;         /* 0x04cc */
        struct {
            RK_U32 cbcr_hor_coe13_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe13_1  : 10;
        } reg52;         /* 0x04d0 */
        struct {
            RK_U32 cbcr_hor_coe13_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe13_3  : 10;
        } reg53;         /* 0x04d4 */
        struct {
            RK_U32 cbcr_hor_coe13_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe13_5  : 10;
        } reg54;         /* 0x04d8 */
        struct {
            RK_U32 cbcr_hor_coe13_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe13_7  : 10;
        } reg55;         /* 0x04dc */
        struct {
            RK_U32 cbcr_hor_coe14_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe14_1  : 10;
        } reg56;         /* 0x04e0 */
        struct {
            RK_U32 cbcr_hor_coe14_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe14_3  : 10;
        } reg57;         /* 0x04e4 */
        struct {
            RK_U32 cbcr_hor_coe14_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe14_5  : 10;
        } reg58;         /* 0x04e8 */
        struct {
            RK_U32 cbcr_hor_coe14_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe14_7  : 10;
        } reg59;         /* 0x04ec */
        struct {
            RK_U32 cbcr_hor_coe15_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe15_1  : 10;
        } reg60;         /* 0x04f0 */
        struct {
            RK_U32 cbcr_hor_coe15_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe15_3  : 10;
        } reg61;         /* 0x04f4 */
        struct {
            RK_U32 cbcr_hor_coe15_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe15_5  : 10;
        } reg62;         /* 0x04f8 */
        struct {
            RK_U32 cbcr_hor_coe15_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe15_7  : 10;
        } reg63;         /* 0x04fc */
        struct {
            RK_U32 cbcr_hor_coe16_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe16_1  : 10;
        } reg64;         /* 0x0500 */
        struct {
            RK_U32 cbcr_hor_coe16_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe16_3  : 10;
        } reg65;         /* 0x0504 */
        struct {
            RK_U32 cbcr_hor_coe16_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe16_5  : 10;
        } reg66;         /* 0x0508 */
        struct {
            RK_U32 cbcr_hor_coe16_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_hor_coe16_7  : 10;
        } reg67;         /* 0x050c */

    } cbcr_hor_coe;

    struct {
        struct {
            RK_U32 cbcr_ver_coe0_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe0_1  : 10;
        } reg0;         /* 0x0600 */
        struct {
            RK_U32 cbcr_ver_coe0_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe0_3  : 10;
        } reg1;         /* 0x0604 */
        struct {
            RK_U32 cbcr_ver_coe0_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe0_5  : 10;
        } reg2;         /* 0x0608 */
        struct {
            RK_U32 cbcr_ver_coe0_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe0_7  : 10;
        } reg3;         /* 0x060c */
        struct {
            RK_U32 cbcr_ver_coe1_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe1_1  : 10;
        } reg4;         /* 0x0610 */
        struct {
            RK_U32 cbcr_ver_coe1_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe1_3  : 10;
        } reg5;         /* 0x0614 */
        struct {
            RK_U32 cbcr_ver_coe1_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe1_5  : 10;
        } reg6;         /* 0x0618 */
        struct {
            RK_U32 cbcr_ver_coe1_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe1_7  : 10;
        } reg7;         /* 0x061c */
        struct {
            RK_U32 cbcr_ver_coe2_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe2_1  : 10;
        } reg8;         /* 0x0620 */
        struct {
            RK_U32 cbcr_ver_coe2_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe2_3  : 10;
        } reg9;         /* 0x0624 */
        struct {
            RK_U32 cbcr_ver_coe2_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe2_5  : 10;
        } reg10;         /* 0x0628 */
        struct {
            RK_U32 cbcr_ver_coe2_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe2_7  : 10;
        } reg11;         /* 0x062c */
        struct {
            RK_U32 cbcr_ver_coe3_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe3_1  : 10;
        } reg12;         /* 0x0630 */
        struct {
            RK_U32 cbcr_ver_coe3_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe3_3  : 10;
        } reg13;         /* 0x0634 */
        struct {
            RK_U32 cbcr_ver_coe3_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe3_5  : 10;
        } reg14;         /* 0x0638 */
        struct {
            RK_U32 cbcr_ver_coe3_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe3_7  : 10;
        } reg15;         /* 0x063c */
        struct {
            RK_U32 cbcr_ver_coe4_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe4_1  : 10;
        } reg16;         /* 0x0640 */
        struct {
            RK_U32 cbcr_ver_coe4_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe4_3  : 10;
        } reg17;         /* 0x0644 */
        struct {
            RK_U32 cbcr_ver_coe4_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe4_5  : 10;
        } reg18;         /* 0x0648 */
        struct {
            RK_U32 cbcr_ver_coe4_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe4_7  : 10;
        } reg19;         /* 0x064c */
        struct {
            RK_U32 cbcr_ver_coe5_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe5_1  : 10;
        } reg20;         /* 0x0650 */
        struct {
            RK_U32 cbcr_ver_coe5_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe5_3  : 10;
        } reg21;         /* 0x0654 */
        struct {
            RK_U32 cbcr_ver_coe5_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe5_5  : 10;
        } reg22;         /* 0x0658 */
        struct {
            RK_U32 cbcr_ver_coe5_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe5_7  : 10;
        } reg23;         /* 0x065c */
        struct {
            RK_U32 cbcr_ver_coe6_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe6_1  : 10;
        } reg24;         /* 0x0660 */
        struct {
            RK_U32 cbcr_ver_coe6_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe6_3  : 10;
        } reg25;         /* 0x0664 */
        struct {
            RK_U32 cbcr_ver_coe6_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe6_5  : 10;
        } reg26;         /* 0x0668 */
        struct {
            RK_U32 cbcr_ver_coe6_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe6_7  : 10;
        } reg27;         /* 0x066c */
        struct {
            RK_U32 cbcr_ver_coe7_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe7_1  : 10;
        } reg28;         /* 0x0670 */
        struct {
            RK_U32 cbcr_ver_coe7_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe7_3  : 10;
        } reg29;         /* 0x0674 */
        struct {
            RK_U32 cbcr_ver_coe7_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe7_5  : 10;
        } reg30;         /* 0x0678 */
        struct {
            RK_U32 cbcr_ver_coe7_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe7_7  : 10;
        } reg31;         /* 0x067c */
        struct {
            RK_U32 cbcr_ver_coe8_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe8_1  : 10;
        } reg32;         /* 0x0680 */
        struct {
            RK_U32 cbcr_ver_coe8_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe8_3  : 10;
        } reg33;         /* 0x0684 */
        struct {
            RK_U32 cbcr_ver_coe8_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe8_5  : 10;
        } reg34;         /* 0x0688 */
        struct {
            RK_U32 cbcr_ver_coe8_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe8_7  : 10;
        } reg35;         /* 0x068c */
        struct {
            RK_U32 cbcr_ver_coe9_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe9_1  : 10;
        } reg36;         /* 0x0690 */
        struct {
            RK_U32 cbcr_ver_coe9_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe9_3  : 10;
        } reg37;         /* 0x0694 */
        struct {
            RK_U32 cbcr_ver_coe9_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe9_5  : 10;
        } reg38;         /* 0x0698 */
        struct {
            RK_U32 cbcr_ver_coe9_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe9_7  : 10;
        } reg39;         /* 0x069c */
        struct {
            RK_U32 cbcr_ver_coe10_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe10_1  : 10;
        } reg40;         /* 0x06a0 */
        struct {
            RK_U32 cbcr_ver_coe10_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe10_3  : 10;
        } reg41;         /* 0x06a4 */
        struct {
            RK_U32 cbcr_ver_coe10_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe10_5  : 10;
        } reg42;         /* 0x06a8 */
        struct {
            RK_U32 cbcr_ver_coe10_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe10_7  : 10;
        } reg43;         /* 0x06ac */
        struct {
            RK_U32 cbcr_ver_coe11_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe11_1  : 10;
        } reg44;         /* 0x06b0 */
        struct {
            RK_U32 cbcr_ver_coe11_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe11_3  : 10;
        } reg45;         /* 0x06b4 */
        struct {
            RK_U32 cbcr_ver_coe11_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe11_5  : 10;
        } reg46;         /* 0x06b8 */
        struct {
            RK_U32 cbcr_ver_coe11_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe11_7  : 10;
        } reg47;         /* 0x06bc */
        struct {
            RK_U32 cbcr_ver_coe12_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe12_1  : 10;
        } reg48;         /* 0x06c0 */
        struct {
            RK_U32 cbcr_ver_coe12_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe12_3  : 10;
        } reg49;         /* 0x06c4 */
        struct {
            RK_U32 cbcr_ver_coe12_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe12_5  : 10;
        } reg50;         /* 0x06c8 */
        struct {
            RK_U32 cbcr_ver_coe12_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe12_7  : 10;
        } reg51;         /* 0x06cc */
        struct {
            RK_U32 cbcr_ver_coe13_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe13_1  : 10;
        } reg52;         /* 0x06d0 */
        struct {
            RK_U32 cbcr_ver_coe13_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe13_3  : 10;
        } reg53;         /* 0x06d4 */
        struct {
            RK_U32 cbcr_ver_coe13_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe13_5  : 10;
        } reg54;         /* 0x06d8 */
        struct {
            RK_U32 cbcr_ver_coe13_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe13_7  : 10;
        } reg55;         /* 0x06dc */
        struct {
            RK_U32 cbcr_ver_coe14_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe14_1  : 10;
        } reg56;         /* 0x06e0 */
        struct {
            RK_U32 cbcr_ver_coe14_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe14_3  : 10;
        } reg57;         /* 0x06e4 */
        struct {
            RK_U32 cbcr_ver_coe14_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe14_5  : 10;
        } reg58;         /* 0x06e8 */
        struct {
            RK_U32 cbcr_ver_coe14_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe14_7  : 10;
        } reg59;         /* 0x06ec */
        struct {
            RK_U32 cbcr_ver_coe15_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe15_1  : 10;
        } reg60;         /* 0x06f0 */
        struct {
            RK_U32 cbcr_ver_coe15_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe15_3  : 10;
        } reg61;         /* 0x06f4 */
        struct {
            RK_U32 cbcr_ver_coe15_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe15_5  : 10;
        } reg62;         /* 0x06f8 */
        struct {
            RK_U32 cbcr_ver_coe15_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe15_7  : 10;
        } reg63;         /* 0x06fc */
        struct {
            RK_U32 cbcr_ver_coe16_0  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe16_1  : 10;
        } reg64;         /* 0x0700 */
        struct {
            RK_U32 cbcr_ver_coe16_2  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe16_3  : 10;
        } reg65;         /* 0x0704 */
        struct {
            RK_U32 cbcr_ver_coe16_4  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe16_5  : 10;
        } reg66;         /* 0x0708 */
        struct {
            RK_U32 cbcr_ver_coe16_6  : 10;
            RK_U32 sw_reserved_1    : 6;
            RK_U32 cbcr_ver_coe16_7  : 10;
        } reg67;         /* 0x070c */

    } cbcr_ver_coe;

    struct {
        struct {
            RK_U32 bypass_en        : 1;
            RK_U32 align_en         : 1;
            RK_U32 reserved_1       : 2;
            RK_U32 format_in        : 4;
            RK_U32 format_out       : 4;
            RK_U32 reserved_2       : 19;
            RK_U32 auto_gating_en   : 1;
        } reg0;         /* 0x0800 */

        RK_U32 reg1;    /* 0x0804 */
        RK_U32 reg2;    /* 0x0808 */

        struct {
            RK_U32 vir_width        : 16;
            RK_U32 vir_height       : 16;
        } reg3;         /* 0x080C */

        struct {
            RK_U32 yrgb_xsd_en      : 1;
            RK_U32 yrgb_xsu_en      : 1;
            RK_U32 yrgb_scl_mode    : 2;
            RK_U32 yrgb_ysd_en      : 1;
            RK_U32 yrgb_ysu_en      : 1;
            RK_U32 yrgb_yscl_mode   : 2;
            RK_U32 yrgb_dering_en   : 1;
            RK_U32 yrgb_gt_en       : 1;
            RK_U32 yrgb_gt_mode     : 2;
            RK_U32 yrgb_xgt_en      : 1;
            RK_U32 reserved_1       : 1;
            RK_U32 yrgb_xgt_mode    : 2;
            RK_U32 yrgb_xsd_bypass  : 1;
            RK_U32 yrgb_ys_bypass   : 1;
            RK_U32 yrgb_xsu_bypass  : 1;
        } reg4;         /* 0x0810 */

        struct {
            RK_U32 yrgb_src_width   : 16;
            RK_U32 yrgb_src_height  : 16;
        } reg5;         /* 0x0814 */

        struct {
            RK_U32 yrgb_dst_width   : 16;
            RK_U32 yrgb_dst_height  : 16;
        } reg6;         /* 0x0818 */

        struct {
            RK_U32 yrgb_dering_sen0 : 5;
            RK_U32 reserved_1       : 3;
            RK_U32 yrgb_dering_sen1 : 5;
            RK_U32 reserved_2       : 3;
            RK_U32 yrgb_dering_alpha: 5;
            RK_U32 reserved_3       : 3;
            RK_U32 yrgb_dering_delta: 5;
        } reg7;         /* 0x081C */

        struct {
            RK_U32 yrgb_xscl_factor : 16;
            RK_U32 yrgb_xscl_offset : 16;
        } reg8;         /* 0x0820 */

        struct {
            RK_U32 yrgb_yscl_factor : 16;
            RK_U32 yrgb_yscl_offset : 16;
        } reg9;         /* 0x0824 */

        RK_U32 reg10;   /* 0x0828 */
        RK_U32 reg11;   /* 0x082C */

        struct {
            RK_U32 cbcr_xsd_en      : 1;
            RK_U32 cbcr_xsu_en      : 1;
            RK_U32 cbcr_scl_mode    : 2;
            RK_U32 cbcr_ysd_en      : 1;
            RK_U32 cbcr_ysu_en      : 1;
            RK_U32 cbcr_yscl_mode   : 2;
            RK_U32 cbcr_dering_en   : 1;
            RK_U32 cbcr_gt_en       : 1;
            RK_U32 cbcr_gt_mode     : 2;
            RK_U32 cbcr_xgt_en      : 1;
            RK_U32 reserved_1       : 1;
            RK_U32 cbcr_xgt_mode    : 2;
            RK_U32 cbcr_xsd_bypass  : 1;
            RK_U32 cbcr_ys_bypass   : 1;
            RK_U32 cbcr_xsu_bypass  : 1;
        } reg12;         /* 0x0830 */

        struct {
            RK_U32 cbcr_src_width   : 16;
            RK_U32 cbcr_src_height  : 16;
        } reg13;         /* 0x0834 */

        struct {
            RK_U32 cbcr_dst_width   : 16;
            RK_U32 cbcr_dst_height  : 16;
        } reg14;         /* 0x0838 */

        struct {
            RK_U32 cbcr_dering_sen0 : 5;
            RK_U32 reserved_1       : 3;
            RK_U32 cbcr_dering_sen1 : 5;
            RK_U32 reserved_2       : 3;
            RK_U32 cbcr_dering_alpha: 5;
            RK_U32 reserved_3       : 3;
            RK_U32 cbcr_dering_delta: 5;
        } reg15;         /* 0x083C */

        struct {
            RK_U32 cbcr_xscl_factor : 16;
            RK_U32 cbcr_xscl_offset : 16;
        } reg16;         /* 0x0840 */

        struct {
            RK_U32 cbcr_yscl_factor : 16;
            RK_U32 cbcr_yscl_offset : 16;
        } reg17;         /* 0x0844 */

    } common;

};               /* offset: 0x2000 */

#ifdef __cplusplus
extern "C" {
#endif

void set_dmsr_to_vdpp_reg(struct dmsr_params* p_dmsr_param, struct dmsr_reg* dmsr);

void vdpp_set_default_zme_param(struct zme_params* param);
void set_zme_to_vdpp_reg(struct zme_params *zme_params, struct zme_reg *zme);

#ifdef __cplusplus
}
#endif

#endif
