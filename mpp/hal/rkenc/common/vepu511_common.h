/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef VEPU511_COMMON_H
#define VEPU511_COMMON_H

#include "rk_venc_cmd.h"
#include "mpp_device.h"

#define VEPU511_CTL_OFFSET              (0 * sizeof(RK_U32))    /* 0x00000000 reg0000 - 0x00000060 reg0024 */
#define VEPU511_FRAME_OFFSET            (156 * sizeof(RK_U32))  /* 0x00000270 reg0156 - 0x000003fc reg0255 */
#define VEPU511_RC_ROI_OFFSET           (1024 * sizeof(RK_U32)) /* 0x00001000 reg1024 - 0x00001160 reg1112 */
#define VEPU511_PARAM_OFFSET            (1472 * sizeof(RK_U32)) /* 0x00001700 reg1472 - 0x000019cc reg1651 */
#define VEPU511_SQI_OFFSET              (2048 * sizeof(RK_U32)) /* 0x00002000 reg2048 - 0x0000216c reg2139 */
#define VEPU511_SCL_JPGTBL_OFFSET       (2176 * sizeof(RK_U32)) /* 0x00002200 reg2176 - 0x00002e1c reg2951 */
#define VEPU511_OSD_OFFSET              (3072 * sizeof(RK_U32)) /* 0x00003000 reg3072 - 0x00003264 reg3225 */
#define VEPU511_STATUS_OFFSET           (4096 * sizeof(RK_U32)) /* 0x00004000 reg4096 - 0x0000424c reg4243 */
#define VEPU511_DBG_OFFSET              (5120 * sizeof(RK_U32)) /* 0x00005000 reg5120 - 0x00005230 reg5260 */
#define VEPU511_REG_BASE_HW_STATUS      (0x2c)

#define VEPU511_MAX_ROI_NUM             8
#define VEPU511_SLICE_FIFO_LEN          8

#define REF_BODY_SIZE(w, h)             MPP_ALIGN((((w) * (h) * 3 / 2) + 48 * MPP_MAX(w, h)), SZ_4K)
#define REF_WRAP_BODY_EXT_SIZE(w, h)    MPP_ALIGN((240 * MPP_MAX(w, h)), SZ_4K)
#define REF_HEADER_SIZE(w, h)           MPP_ALIGN((((w) * (h) / 64) + MPP_MAX(w, h) / 2), SZ_4K)
#define REF_WRAP_HEADER_EXT_SIZE(w, h)  MPP_ALIGN((3 * MPP_MAX(w, h)), SZ_4K)

typedef enum ref_type_e {
    ST_REF_TO_ST,
    ST_REF_TO_LT,
    LT_REF_TO_ST,
    LT_REF_TO_LT,
} RefType;

typedef enum qbias_ofst_e {
    IFRAME_THD0 = 0,
    IFRAME_THD1,
    IFRAME_THD2,
    IFRAME_BIAS0,
    IFRAME_BIAS1,
    IFRAME_BIAS2,
    IFRAME_BIAS3,
    PFRAME_THD0,
    PFRAME_THD1,
    PFRAME_THD2,
    PFRAME_IBLK_BIAS0,
    PFRAME_IBLK_BIAS1,
    PFRAME_IBLK_BIAS2,
    PFRAME_IBLK_BIAS3,
    PFRAME_PBLK_BIAS0,
    PFRAME_PBLK_BIAS1,
    PFRAME_PBLK_BIAS2,
    PFRAME_PBLK_BIAS3
} QbiasOfst;

typedef enum aq_rnge_ofst_e {
    AQ16_RNGE = 0,
    AQ32_RNGE,
    AQ8_RNGE,
    AQ16_DIFF0,
    AQ16_DIFF1
} AqRngeOfst;

typedef struct wrap_info {
    RK_U32 bottom;
    RK_U32 top;
    RK_U32 cur_off;
    RK_U32 pre_off;
    RK_U32 size;
    RK_U32 total_size;
} WrapInfo;

typedef struct wrap_buf_info {
    WrapInfo hdr_lt;
    WrapInfo hdr;
    WrapInfo body_lt;
    WrapInfo body;
} WrapBufInfo;

typedef struct Vepu511Online_t {
    /* 0x00000270 reg156 */
    RK_U32 reg0156_adr_vsy_t;
    /* 0x00000274 reg157 */
    RK_U32 reg0157_adr_vsc_t;
    /* 0x00000278 reg158 */
    RK_U32 reg0158_adr_vsy_b;
    /* 0x0000027c reg159 */
    RK_U32 reg0159_adr_vsc_b;
} vepu511_online;

typedef struct Vepu511RoiRegion_t {
    struct {
        RK_U32 roi_lt_x    : 10;
        RK_U32 reserved    : 6;
        RK_U32 roi_lt_y    : 10;
        RK_U32 reserved1   : 6;
    } roi_pos_lt;

    struct {
        RK_U32 roi_rb_x    : 10;
        RK_U32 reserved    : 6;
        RK_U32 roi_rb_y    : 10;
        RK_U32 reserved1   : 6;
    } roi_pos_rb;

    struct {
        RK_U32 roi_qp_value       : 7;
        RK_U32 roi_qp_adj_mode    : 1;
        RK_U32 roi_pri            : 5;
        RK_U32 roi_en             : 1;
        RK_U32 reserved           : 18;
    } roi_base;

    union {
        struct {
            RK_U32 roi_mdc_intra16       : 4;
            RK_U32 roi_mdc_inter16       : 4;
            RK_U32 roi_mdc_split16       : 4;
            RK_U32 roi_mdc_res_intra16   : 4;
            RK_U32 roi_mdc_res_inter16   : 4;
            RK_U32 roi_mdc_res_zeromv16  : 4;
            RK_U32 roi_mdc_dpth_hevc     : 1;
            RK_U32 reserved              : 7;
        } h265;

        struct {
            RK_U32 roi_mdc_intra16       : 4;
            RK_U32 roi_mdc_inter16       : 4;
            RK_U32 roi_mdc_skip16        : 4;
            RK_U32 reserved              : 20;
        } h264;
    } roi_mdc0;

    /* h265 used */
    struct {
        RK_U32 roi_mdc_intra32         : 4;
        RK_U32 roi_mdc_inter32         : 4;
        RK_U32 roi_mdc_split32         : 4;
        RK_U32 roi_mdc_res_intra32     : 4;
        RK_U32 roi_mdc_res_inter32     : 4;
        RK_U32 roi_mdc_res_zeromv32    : 4;
        RK_U32 reserved                : 8;
    } roi_mdc1;
} Vepu511RoiRegion;

/*
 * Vepu511RoiCfg
 */
typedef struct Vepu511RoiCfg_t {
    /* 0x00001090 reg1060 - 0x0000112c reg1099 */
    Vepu511RoiRegion regions[8];
} Vepu511RoiCfg;

/* class: scaling list  */
/* 0x00002200 reg2176- 0x00002584 reg2401: scale_iq */
typedef struct Vepu511SclCfg_t {
    /* 0x2200 - 0x221F, valid for h.264/h.h265, jpeg no use */
    RK_U32 tu8_intra_y[16];
    RK_U32 tu8_intra_u[16]; /* tu8_inter_y[16] for h.264 */

    /* 0x2220 - 0x2584, valid for h.265 only */
    RK_U32 tu8_intra_v[16];
    RK_U32 tu8_inter_y[16];
    RK_U32 tu8_inter_u[16];
    RK_U32 tu8_inter_v[16];
    RK_U32 tu16_intra_y_ac[16];
    RK_U32 tu16_intra_u_ac[16];
    RK_U32 tu16_intra_v_ac[16];
    RK_U32 tu16_inter_y_ac[16];
    RK_U32 tu16_inter_u_ac[16];
    RK_U32 tu16_inter_v_ac[16];
    RK_U32 tu32_intra_y_ac[16];
    RK_U32 tu32_inter_y_ac[16];

    /* 0x2580 */
    struct {
        RK_U32 tu16_intra_y_dc  : 8;
        RK_U32 tu16_intra_u_dc  : 8;
        RK_U32 tu16_intra_v_dc  : 8;
        RK_U32 tu16_inter_y_dc  : 8;
    } tu_dc0;

    /* 0x2584 */
    struct {
        RK_U32 tu16_inter_u_dc  : 8;
        RK_U32 tu16_inter_v_dc  : 8;
        RK_U32 tu32_intra_y_dc  : 8;
        RK_U32 tu32_inter_y_dc  : 8;
    } tu_dc1;
} Vepu511SclCfg;

/* 0x00002590 - 0x00002c9c : scale_q */
typedef struct Vepu511SclCfgExt_t {
    /* 0x2590 - 0x268C, valid for h.264/h.h265, jpeg no use */
    RK_U32 tu8_intra_y[32];
    RK_U32 tu8_intra_u[32];

    /* 0x2690 - 0x2C8C, valid for h.265 only */
    RK_U32 tu8_intra_v[32];
    RK_U32 tu8_inter_y[32];
    RK_U32 tu8_inter_u[32];
    RK_U32 tu8_inter_v[32];
    RK_U32 tu16_intra_y_ac[32];
    RK_U32 tu16_intra_u_ac[32];
    RK_U32 tu16_intra_v_ac[32];
    RK_U32 tu16_inter_y_ac[32];
    RK_U32 tu16_inter_u_ac[32];
    RK_U32 tu16_inter_v_ac[32];
    RK_U32 tu32_intra_y_ac[32];
    RK_U32 tu32_inter_y_ac[32];

    /* 0x2C90 */
    struct {
        RK_U32 tu16_intra_y_dc  : 16;
        RK_U32 tu16_intra_u_dc  : 16;
    } tu_dc0;

    /* 0x2C94 */
    struct {
        RK_U32 tu16_intra_v_dc  : 16;
        RK_U32 tu16_inter_y_dc  : 16;
    } tu_dc1;

    /* 0x2C98 */
    struct {
        RK_U32 tu16_inter_u_dc  : 16;
        RK_U32 tu16_inter_v_dc  : 16;
    } tu_dc2;

    /* 0x2C9C */
    struct {
        RK_U32 tu32_intra_y_dc  : 16;
        RK_U32 tu32_inter_y_dc  : 16;
    } tu_dc3;
} Vepu511SclCfgExt;

/* class: control/link */
/* 0x00000000 reg0 - 0x00000120 reg72 */
typedef struct Vepu511ControlCfg_t {
    /* 0x00000000 reg0 */
    struct {
        RK_U32 sub_ver      : 8;
        RK_U32 h264_cap     : 1;
        RK_U32 hevc_cap     : 1;
        RK_U32 jpeg_cap     : 1;
        RK_U32 reserved     : 1;
        RK_U32 res_cap      : 4;
        RK_U32 osd_cap      : 2;
        RK_U32 filtr_cap    : 2;
        RK_U32 bfrm_cap     : 1;
        RK_U32 fbc_cap      : 2;
        RK_U32 reserved1    : 1;
        RK_U32 ip_id        : 8;
    } version;

    /* 0x00000004 - 0x0000000c */
    RK_U32 reserved1_3[3];

    /* 0x00000010 reg4 */
    struct {
        RK_U32 lkt_num     : 8;
        RK_U32 vepu_cmd    : 3;
        RK_U32 reserved    : 21;
    } enc_strt;

    /* 0x00000014 reg5 */
    struct {
        RK_U32 safe_clr     : 1;
        RK_U32 force_clr    : 1;
        RK_U32 dvbm_clr     : 1;
        RK_U32 reserved     : 29;
    } enc_clr;

    /* 0x00000018 reg6 */
    struct {
        RK_U32 vswm_lcnt_soft    : 14;
        RK_U32 vswm_fcnt_soft    : 8;
        RK_U32 vswm_fsid_soft    : 1;
        RK_U32 reserved          : 1;
        RK_U32 dvbm_ack_soft     : 1;
        RK_U32 dvbm_ack_sel      : 1;
        RK_U32 dvbm_inf_sel      : 1;
        RK_U32 reserved1         : 5;
    } vs_ldly;

    /* 0x0000001c */
    RK_U32 reserved_7;

    /* 0x00000020 reg8 */
    struct {
        RK_U32 enc_done_en          : 1;
        RK_U32 lkt_node_done_en     : 1;
        RK_U32 sclr_done_en         : 1;
        RK_U32 vslc_done_en         : 1;
        RK_U32 vbsf_oflw_en         : 1;
        RK_U32 vbuf_lens_en         : 1;
        RK_U32 enc_err_en           : 1;
        RK_U32 vsrc_err_en          : 1;
        RK_U32 wdg_en               : 1;
        RK_U32 lkt_err_int_en       : 1;
        RK_U32 lkt_err_stop_en      : 1;
        RK_U32 lkt_force_stop_en    : 1;
        RK_U32 jslc_done_en         : 1;
        RK_U32 jbsf_oflw_en         : 1;
        RK_U32 jbuf_lens_en         : 1;
        RK_U32 dvbm_err_en          : 1;
        RK_U32 reserved             : 16;
    } int_en;

    /* 0x00000024 reg9 */
    struct {
        RK_U32 enc_done_msk          : 1;
        RK_U32 lkt_node_done_msk     : 1;
        RK_U32 sclr_done_msk         : 1;
        RK_U32 vslc_done_msk         : 1;
        RK_U32 vbsf_oflw_msk         : 1;
        RK_U32 vbuf_lens_msk         : 1;
        RK_U32 enc_err_msk           : 1;
        RK_U32 vsrc_err_msk          : 1;
        RK_U32 wdg_msk               : 1;
        RK_U32 lkt_err_int_msk       : 1;
        RK_U32 lkt_err_stop_msk      : 1;
        RK_U32 lkt_force_stop_msk    : 1;
        RK_U32 jslc_done_msk         : 1;
        RK_U32 jbsf_oflw_msk         : 1;
        RK_U32 jbuf_lens_msk         : 1;
        RK_U32 dvbm_err_msk          : 1;
        RK_U32 reserved              : 16;
    } int_msk;

    /* 0x00000028 reg10 */
    struct {
        RK_U32 enc_done_clr          : 1;
        RK_U32 lkt_node_done_clr     : 1;
        RK_U32 sclr_done_clr         : 1;
        RK_U32 vslc_done_clr         : 1;
        RK_U32 vbsf_oflw_clr         : 1;
        RK_U32 vbuf_lens_clr         : 1;
        RK_U32 enc_err_clr           : 1;
        RK_U32 vsrc_err_clr          : 1;
        RK_U32 wdg_clr               : 1;
        RK_U32 lkt_err_int_clr       : 1;
        RK_U32 lkt_err_stop_clr      : 1;
        RK_U32 lkt_force_stop_clr    : 1;
        RK_U32 jslc_done_clr         : 1;
        RK_U32 jbsf_oflw_clr         : 1;
        RK_U32 jbuf_lens_clr         : 1;
        RK_U32 dvbm_err_clr          : 1;
        RK_U32 reserved              : 16;
    } int_clr;

    /* 0x0000002c reg11 */
    struct {
        RK_U32 enc_done_sta          : 1;
        RK_U32 lkt_node_done_sta     : 1;
        RK_U32 sclr_done_sta         : 1;
        RK_U32 vslc_done_sta         : 1;
        RK_U32 vbsf_oflw_sta         : 1;
        RK_U32 vbuf_lens_sta         : 1;
        RK_U32 enc_err_sta           : 1;
        RK_U32 vsrc_err_sta          : 1;
        RK_U32 wdg_sta               : 1;
        RK_U32 lkt_err_int_sta       : 1;
        RK_U32 lkt_err_stop_sta      : 1;
        RK_U32 lkt_force_stop_sta    : 1;
        RK_U32 jslc_done_sta         : 1;
        RK_U32 jbsf_oflw_sta         : 1;
        RK_U32 jbuf_lens_sta         : 1;
        RK_U32 dvbm_err_sta          : 1;
        RK_U32 reserved              : 16;
    } int_sta;

    /* 0x00000030 reg12 */
    struct {
        RK_U32 jpeg_bus_edin        : 4;
        RK_U32 src_bus_edin         : 4;
        RK_U32 meiw_bus_edin        : 4;
        RK_U32 bsw_bus_edin         : 4;
        RK_U32 lktr_bus_edin        : 4;
        RK_U32 roir_bus_edin        : 4;
        RK_U32 lktw_bus_edin        : 4;
        RK_U32 rec_nfbc_bus_edin    : 4;
    } dtrns_map;

    /* 0x00000034 reg13 */
    struct {
        RK_U32 jsrc_bus_edin    : 4;
        RK_U32 reserved         : 12;
        RK_U32 axi_brsp_cke     : 13;
        RK_U32 reserved1        : 3;
    } dtrns_cfg;

    /* 0x00000038 reg14 */
    struct {
        RK_U32 vs_load_thd    : 24;
        RK_U32 reserved       : 8;
    } enc_wdg;

    /* 0x0000003c reg15 */
    struct {
        RK_U32 hurry_en      : 1;
        RK_U32 hurry_low     : 3;
        RK_U32 hurry_mid     : 3;
        RK_U32 hurry_high    : 3;
        RK_U32 qos_sub_e     : 1;
        RK_U32 reserved1     : 5;
        RK_U32 qos_period    : 16;
    } qos_cfg;

    /* 0x00000040 reg16 */
    struct {
        RK_U32 qos_ar_dprt    : 4;
        RK_U32 qos_ar_lprt    : 4;
        RK_U32 qos_ar_mprt    : 4;
        RK_U32 qos_ar_hprt    : 4;
        RK_U32 qos_aw_dprt    : 4;
        RK_U32 qos_aw_lprt    : 4;
        RK_U32 qos_aw_mprt    : 4;
        RK_U32 qos_aw_hprt    : 4;
    } qos_prty;

    /* 0x00000044 reg17 */
    RK_U32 hurry_thd_low;

    /* 0x00000048 reg18 */
    RK_U32 hurry_thd_mid;

    /* 0x0000004c reg19 */
    RK_U32 hurry_thd_high;

    /* 0x50 QOS_CHID */
    struct {
        RK_U32 qos_prt_mmu    : 4;
        RK_U32 qos_prt_rfpr   : 4;
        RK_U32 reserved1      : 24;
    } qos_chid;

    /* 0x00000054 reg21 */
    struct {
        RK_U32 cke              : 1;
        RK_U32 resetn_hw_en     : 1;
        RK_U32 rfpr_err_e       : 1;
        RK_U32 sram_ckg_en      : 1;
        RK_U32 link_err_stop    : 1;
        RK_U32 reserved         : 27;
    } opt_strg;

    /* 0x00000058 reg22 */
    union {
        struct {
            RK_U32 tq8_ckg           : 1;
            RK_U32 tq4_ckg           : 1;
            RK_U32 bits_ckg_8x8      : 1;
            RK_U32 bits_ckg_4x4_1    : 1;
            RK_U32 bits_ckg_4x4_0    : 1;
            RK_U32 inter_mode_ckg    : 1;
            RK_U32 inter_ctrl_ckg    : 1;
            RK_U32 inter_pred_ckg    : 1;
            RK_U32 intra8_ckg        : 1;
            RK_U32 intra4_ckg        : 1;
            RK_U32 reserved          : 22;
        } rdo_ckg_h264;

        struct {
            RK_U32 recon32_ckg       : 1;
            RK_U32 iqit32_ckg        : 1;
            RK_U32 q32_ckg           : 1;
            RK_U32 t32_ckg           : 1;
            RK_U32 cabac32_ckg       : 1;
            RK_U32 recon16_ckg       : 1;
            RK_U32 iqit16_ckg        : 1;
            RK_U32 q16_ckg           : 1;
            RK_U32 t16_ckg           : 1;
            RK_U32 cabac16_ckg       : 1;
            RK_U32 recon8_ckg        : 1;
            RK_U32 iqit8_ckg         : 1;
            RK_U32 q8_ckg            : 1;
            RK_U32 t8_ckg            : 1;
            RK_U32 cabac8_ckg        : 1;
            RK_U32 recon4_ckg        : 1;
            RK_U32 iqit4_ckg         : 1;
            RK_U32 q4_ckg            : 1;
            RK_U32 t4_ckg            : 1;
            RK_U32 cabac4_ckg        : 1;
            RK_U32 intra32_ckg       : 1;
            RK_U32 intra16_ckg       : 1;
            RK_U32 intra8_ckg        : 1;
            RK_U32 intra4_ckg        : 1;
            RK_U32 inter_pred_ckg    : 1;
            RK_U32 reserved          : 7;
        } rdo_ckg_hevc;
    };

    /* 0x0000005c reg23 */
    struct {
        RK_U32 core_id     : 2;
        RK_U32 reserved    : 30;
    } core_id;

    /* 0x00000060 reg24 */
    struct {
        RK_U32 dvbm_en           : 1;
        RK_U32 src_badr_sel      : 1;
        RK_U32 dvbm_vpu_fskp     : 1;
        RK_U32 src_oflw_drop     : 1;
        RK_U32 dvbm_isp_cnct     : 1;
        RK_U32 dvbm_vepu_cnct    : 1;
        RK_U32 vepu_expt_type    : 2;
        RK_U32 vinf_dly_cycle    : 8;
        RK_U32 ybuf_full_mgn     : 8;
        RK_U32 ybuf_oflw_mgn     : 8;
    } dvbm_cfg;
} Vepu511ControlCfg;

typedef struct Vepu511JpegRoi_t {
    struct {
        RK_U32 roi0_rdoq_start_x    : 11;
        RK_U32 roi0_rdoq_start_y    : 11;
        RK_U32 reserved             : 3;
        RK_U32 roi0_rdoq_level      : 6;
        RK_U32 roi0_rdoq_en         : 1;
    } roi_cfg0;

    struct {
        RK_U32 roi0_rdoq_width_m1     : 11;
        RK_U32 roi0_rdoq_height_m1    : 11;
        /* the below 10 bits only for roi0 */
        RK_U32 reserved               : 3;
        RK_U32 frm_rdoq_level         : 6;
        RK_U32 frm_rdoq_en            : 1;
    } roi_cfg1;
} Vepu511JpegRoi;

typedef struct Vepu511JpegTable_t {
    /* 0x2ca0 - 0x2e1c */
    RK_U16 qua_tab0[64];
    RK_U16 qua_tab1[64];
    RK_U16 qua_tab2[64];
} Vepu511JpegTable;

/* class osd */
typedef struct Vepu511OsdRegion_t {
    struct {
        RK_U32 osd_en                : 1;
        RK_U32 reserved              : 4;
        RK_U32 osd_qp_adj_en         : 1;
        RK_U32 osd_range_trns_en     : 1;
        RK_U32 osd_range_trns_sel    : 1;
        RK_U32 osd_fmt               : 4;
        RK_U32 osd_alpha_swap        : 1;
        RK_U32 osd_rbuv_swap         : 1;
        RK_U32 reserved1             : 8;
        RK_U32 osd_fg_alpha          : 8;
        RK_U32 osd_fg_alpha_sel      : 2;
    } cfg0;

    struct {
        RK_U32 osd_lt_xcrd    : 14;
        RK_U32 osd_lt_ycrd    : 14;
        RK_U32 osd_endn       : 4;
    } cfg1;

    struct {
        RK_U32 osd_rb_xcrd    : 14;
        RK_U32 osd_rb_ycrd    : 14;
        RK_U32 reserved        : 4;
    } cfg2;

    RK_U32 osd_st_addr;

    RK_U32 reserved;

    struct {
        RK_U32 osd_stride        : 17;
        RK_U32 reserved          : 8;
        RK_U32 osd_ch_ds_mode    : 1;
        RK_U32 reserved1         : 6;
    } cfg5;

    RK_U8 lut[8];

    /* only for h.264/h.h265, jpeg no use */
    struct {
        RK_U32 osd_qp_adj_sel    : 1;
        RK_U32 osd_qp            : 7;
        RK_U32 osd_qp_max        : 6;
        RK_U32 osd_qp_min        : 6;
        RK_U32 osd_qp_prj        : 5;
        RK_U32 reserved          : 7;
    } cfg8;
} Vepu511OsdRegion;

/* class: osd */
/* 0x00003000 reg3072 - 0x00003134 reg3149 */
typedef struct Vepu511Osd_t {
    /* 0x00003000 reg3072 - 0x0000311c reg3143*/
    Vepu511OsdRegion osd_regions[8];

    /* 0x00003120 reg3144 */
    struct {
        RK_U32 osd_csc_yr    : 9;
        RK_U32 osd_csc_yg    : 9;
        RK_U32 osd_csc_yb    : 9;
        RK_U32 reserved      : 5;
    } osd_whi_cfg0;

    /* 0x00003124 reg3145 */
    struct {
        RK_U32 osd_csc_ur    : 9;
        RK_U32 osd_csc_ug    : 9;
        RK_U32 osd_csc_ub    : 9;
        RK_U32 reserved      : 5;
    } osd_whi_cfg1;

    /* 0x00003128 reg3146 */
    struct {
        RK_U32 osd_csc_vr    : 9;
        RK_U32 osd_csc_vg    : 9;
        RK_U32 osd_csc_vb    : 9;
        RK_U32 reserved      : 5;
    } osd_whi_cfg2;

    /* 0x0000312c reg3147 */
    struct {
        RK_U32 osd_csc_ofst_y    : 8;
        RK_U32 osd_csc_ofst_u    : 8;
        RK_U32 osd_csc_ofst_v    : 8;
        RK_U32 reserved          : 8;
    } osd_whi_cfg3;
} Vepu511Osd;

/* class: osd */
/*0x00003000 reg3072 - 0x00003264 reg3225 */
typedef struct Vepu511OsdComb_t {
    /*0x00003000 reg3072 - 0x0000312c reg3147 */
    Vepu511Osd osd;

    /* 0x00003130 reg3148 - 0x00003134 reg3149 */
    RK_U32 reserve[2];

    /*0x00003138 reg3150 - 0x00003264 reg3225 */
    Vepu511Osd jpeg_osd;
} Vepu511OsdComb;

/* class: st */
/* 0x00004000 reg4096 - 0x0000424c reg4243*/
typedef struct Vepu511Status_t {
    /* 0x00004000 reg4096 */
    RK_U32 bs_lgth_l32;

    /* 0x00004004 reg4097 */
    struct {
        RK_U32 bs_lgth_h8       : 8;
        RK_U32 st_rc_lst_dqp    : 6;
        RK_U32 reserved         : 2;
        RK_U32 sse_l16          : 16;
    } st_sse_bsl;

    /* 0x00004008 reg4098 */
    RK_U32 sse_h32;

    /* 0x0000400c reg4099 */
    RK_U32 qp_sum;

    /* 0x00004010 reg4100 */
    struct {
        RK_U32 sao_cnum    : 16;
        RK_U32 sao_ynum    : 16;
    } st_sao;

    /* 0x00004014 reg4101 */
    RK_U32 rdo_head_bits_l32;

    /* 0x00004018 reg4102 */
    struct {
        RK_U32 rdo_head_bits_h8    : 8;
        RK_U32 reserved            : 8;
        RK_U32 rdo_res_bits_l16    : 16;
    } st_head_res_bl;

    /* 0x0000401c reg4103 */
    RK_U32 rdo_res_bits_h24;

    /* 0x00004020 reg4104 */
    struct {
        RK_U32 st_enc             : 2;
        RK_U32 st_sclr            : 1;
        RK_U32 vepu_fbd_err       : 1;
        RK_U32 vepu_vsp_err       : 1;
        RK_U32 reserved           : 3;
        RK_U32 isp_src_oflw       : 1;
        RK_U32 vepu_src_oflw      : 1;
        RK_U32 vepu_sid_nmch      : 1;
        RK_U32 vepu_fcnt_nmch     : 1;
        RK_U32 reserved1          : 4;
        RK_U32 dvbm_finf_wful     : 1;
        RK_U32 dvbm_linf_wful     : 1;
        RK_U32 dvbm_fsid_nmch     : 1;
        RK_U32 dvbm_fcnt_early    : 1;
        RK_U32 dvbm_fcnt_late     : 1;
        RK_U32 dvbm_isp_oflw      : 1;
        RK_U32 dvbm_vepu_oflw     : 1;
        RK_U32 isp_time_out       : 1;
        RK_U32 anti_info_oflw     : 1;
        RK_U32 anti_vsrc_oflw     : 1;
        RK_U32 reserved2          : 1;
        RK_U32 dvbm_vsrc_sid      : 1;
        RK_U32 dvbm_vsrc_fcnt     : 4;
    } st_enc;

    /* 0x00004024 reg4105 */
    struct {
        RK_U32 fnum_cfg_done    : 8;
        RK_U32 fnum_cfg         : 8;
        RK_U32 fnum_int         : 8;
        RK_U32 fnum_enc_done    : 8;
    } st_lkt;

    /* 0x00004028 reg4106 */
    RK_U32 node_addr;

    /* 0x0000402c reg4107 */
    RK_U32 vbsbw_addr;

    /* 0x00004030 reg4108 */
    struct {
        RK_U32 axib_idl     : 8;
        RK_U32 axib_ovfl    : 8;
        RK_U32 axib_err     : 8;
        RK_U32 axir_err     : 8;
    } st_bus;

    /* 0x00004034 reg4109 */
    struct {
        RK_U32 sli_num_video     : 8;
        RK_U32 sli_num_jpeg      : 8;
        RK_U32 bpkt_num_video    : 7;
        RK_U32 bpkt_lst_video    : 1;
        RK_U32 bpkt_num_jpeg     : 7;
        RK_U32 bpkt_lst_jpeg     : 1;
    } st_snum;

    /* 0x00004038 reg4110 */
    struct {
        RK_U32 sli_len    : 30;
        RK_U32 sli_lst    : 1;
        RK_U32 sli_sid    : 1;
    } st_slen;

    /* 0x0000403c reg4111 */
    struct {
        RK_U32 task_id_proc    : 12;
        RK_U32 task_id_done    : 12;
        RK_U32 task_done       : 1;
        RK_U32 task_lkt_err    : 3;
        RK_U32 reserved        : 4;
    } st_link_task;

    /* 0x00004040 reg4112 */
    struct {
        RK_U32 eslf_nptr     : 10;
        RK_U32 eslf_empty    : 1;
        RK_U32 eslf_full     : 1;
        RK_U32 eslf_sid      : 1;
        RK_U32 reserved      : 19;
    } st_eslf_nptr;

    /* 0x00004044 reg4113 */
    struct {
        RK_U32 vsrd_posy         : 10;
        RK_U32 reserved          : 5;
        RK_U32 vsrd_fend         : 1;
        RK_U32 vsrd_posy_jpeg    : 10;
        RK_U32 reserved1         : 5;
        RK_U32 vsrd_fend_jpeg    : 1;
    } st_vlsd_rlvl;

    /* 0x00004048 reg4114 */
    struct {
        RK_U32 eslf_nptr_jpeg     : 10;
        RK_U32 eslf_empty_jpeg    : 1;
        RK_U32 eslf_full_jpeg     : 1;
        RK_U32 eslf_sid_jpeg      : 1;
        RK_U32 reserved           : 19;
    } st_eslf_nptr_jpeg;

    /* 0x404c - 0x405c */
    RK_U32 reserved4115_4119[5];

    /* 0x00004060 reg4120 */
    struct {
        RK_U32 sli_len_jpeg    : 30;
        RK_U32 sli_lst_jpeg    : 1;
        RK_U32 sli_sid_jpeg    : 1;
    } st_slen_jpeg;

    /* 0x00004064 reg4121 */
    RK_U32 jpeg_head_bits_l32;

    /* 0x00004068 reg4122 */
    struct {
        RK_U32 jpeg_head_bits_h8    : 1;
        RK_U32 reserved             : 31;
    } st_bsl_h8_jpeg;

    /* 0x0000406c reg4123 */
    RK_U32 jbsbw_addr;

    /* 0x00004070 reg4124 */
    RK_U32 luma_pix_sum_od;

    /* 0x4074 - 0x407c */
    RK_U32 reserved4125_4127[3];

    /* 0x00004080 reg4128 */
    struct {
        RK_U32 pnum_p64    : 17;
        RK_U32 reserved    : 15;
    } st_pnum_p64;

    /* 0x00004084 reg4129 */
    struct {
        RK_U32 pnum_p32    : 19;
        RK_U32 reserved    : 13;
    } st_pnum_p32;

    /* 0x00004088 reg4130 */
    struct {
        RK_U32 pnum_p16    : 21;
        RK_U32 reserved    : 11;
    } st_pnum_p16;

    /* 0x0000408c reg4131 */
    struct {
        RK_U32 pnum_p8     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_p8;

    /* 0x00004090 reg4132 */
    struct {
        RK_U32 pnum_i32    : 19;
        RK_U32 reserved    : 13;
    } st_pnum_i32;

    /* 0x00004094 reg4133 */
    struct {
        RK_U32 pnum_i16    : 21;
        RK_U32 reserved    : 11;
    } st_pnum_i16;

    /* 0x00004098 reg4134 */
    struct {
        RK_U32 pnum_i8     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_i8;

    /* 0x0000409c reg4135 */
    struct {
        RK_U32 pnum_i4     : 23;
        RK_U32 reserved    : 9;
    } st_pnum_i4;

    /* 0x000040a0 reg4136 */
    struct {
        RK_U32 num_b16     : 23;
        RK_U32 reserved    : 9;
    } st_bnum_b16;

    /* 0x40a4 */
    RK_U32 reserved_4137;

    /* 0x000040a8 reg4138 */
    RK_U32 madi16_sum;

    /* 0x000040ac reg4139 */
    RK_U32 madi32_sum;

    /* 0x000040b0 reg4140 */
    RK_U32 madp16_sum;

    /* 0x000040b4 reg4141 */
    struct {
        RK_U32 rdo_smear_cnt0    : 10;
        RK_U32 reserved          : 6;
        RK_U32 rdo_smear_cnt1    : 10;
        RK_U32 reserved1         : 6;
    } st_smear_cnt0;

    /* 0x000040b8 reg4142 */
    struct {
        RK_U32 rdo_smear_cnt2    : 10;
        RK_U32 reserved          : 6;
        RK_U32 rdo_smear_cnt3    : 10;
        RK_U32 reserved1         : 6;
    } st_smear_cnt1;

    /* 0x40bc */
    RK_U32 reserved_4143;

    /* 0x000040c0 reg4144 */
    struct {
        RK_U32 madi_th_lt_cnt0    : 16;
        RK_U32 madi_th_lt_cnt1    : 16;
    } st_madi_lt_num0;

    /* 0x000040c4 reg4145 */
    struct {
        RK_U32 madi_th_lt_cnt2    : 16;
        RK_U32 madi_th_lt_cnt3    : 16;
    } st_madi_lt_num1;

    /* 0x000040c8 reg4146 */
    struct {
        RK_U32 madi_th_rt_cnt0    : 16;
        RK_U32 madi_th_rt_cnt1    : 16;
    } st_madi_rt_num0;

    /* 0x000040cc reg4147 */
    struct {
        RK_U32 madi_th_rt_cnt2    : 16;
        RK_U32 madi_th_rt_cnt3    : 16;
    } st_madi_rt_num1;

    /* 0x000040d0 reg4148 */
    struct {
        RK_U32 madi_th_lb_cnt0    : 16;
        RK_U32 madi_th_lb_cnt1    : 16;
    } st_madi_lb_num0;

    /* 0x000040d4 reg4149 */
    struct {
        RK_U32 madi_th_lb_cnt2    : 16;
        RK_U32 madi_th_lb_cnt3    : 16;
    } st_madi_lb_num1;

    /* 0x000040d8 reg4150 */
    struct {
        RK_U32 madi_th_rb_cnt0    : 16;
        RK_U32 madi_th_rb_cnt1    : 16;
    } st_madi_rb_num0;

    /* 0x000040dc reg4151 */
    struct {
        RK_U32 madi_th_rb_cnt2    : 16;
        RK_U32 madi_th_rb_cnt3    : 16;
    } st_madi_rb_num1;

    /* 0x000040e0 reg4152 */
    struct {
        RK_U32 madp_th_lt_cnt0    : 16;
        RK_U32 madp_th_lt_cnt1    : 16;
    } st_madp_lt_num0;

    /* 0x000040e4 reg4153 */
    struct {
        RK_U32 madp_th_lt_cnt2    : 16;
        RK_U32 madp_th_lt_cnt3    : 16;
    } st_madp_lt_num1;

    /* 0x000040e8 reg4154 */
    struct {
        RK_U32 madp_th_rt_cnt0    : 16;
        RK_U32 madp_th_rt_cnt1    : 16;
    } st_madp_rt_num0;

    /* 0x000040ec reg4155 */
    struct {
        RK_U32 madp_th_rt_cnt2    : 16;
        RK_U32 madp_th_rt_cnt3    : 16;
    } st_madp_rt_num1;

    /* 0x000040f0 reg4156 */
    struct {
        RK_U32 madp_th_lb_cnt0    : 16;
        RK_U32 madp_th_lb_cnt1    : 16;
    } st_madp_lb_num0;

    /* 0x000040f4 reg4157 */
    struct {
        RK_U32 madp_th_lb_cnt2    : 16;
        RK_U32 madp_th_lb_cnt3    : 16;
    } st_madp_lb_num1;

    /* 0x000040f8 reg4158 */
    struct {
        RK_U32 madp_th_rb_cnt0    : 16;
        RK_U32 madp_th_rb_cnt1    : 16;
    } st_madp_rb_num0;

    /* 0x000040fc reg4159 */
    struct {
        RK_U32 madp_th_rb_cnt2    : 16;
        RK_U32 madp_th_rb_cnt3    : 16;
    } st_madp_rb_num1;

    /* 0x00004100 reg4160 */
    struct {
        RK_U32 cmv_th_lt_cnt0    : 16;
        RK_U32 cmv_th_lt_cnt1    : 16;
    } st_cmv_lt_num0;

    /* 0x00004104 reg4161 */
    struct {
        RK_U32 cmv_th_lt_cnt2    : 16;
        RK_U32 cmv_th_lt_cnt3    : 16;
    } st_cmv_lt_num1;

    /* 0x00004108 reg4162 */
    struct {
        RK_U32 cmv_th_rt_cnt0    : 16;
        RK_U32 cmv_th_rt_cnt1    : 16;
    } st_cmv_rt_num0;

    /* 0x0000410c reg4163 */
    struct {
        RK_U32 cmv_th_rt_cnt2    : 16;
        RK_U32 cmv_th_rt_cnt3    : 16;
    } st_cmv_rt_num1;

    /* 0x00004110 reg4164 */
    struct {
        RK_U32 cmv_th_lb_cnt0    : 16;
        RK_U32 cmv_th_lb_cnt1    : 16;
    } st_cmv_lb_num0;

    /* 0x00004114 reg4165 */
    struct {
        RK_U32 cmv_th_lb_cnt2    : 16;
        RK_U32 cmv_th_lb_cnt3    : 16;
    } st_cmv_lb_num1;

    /* 0x00004118 reg4166 */
    struct {
        RK_U32 cmv_th_rb_cnt0    : 16;
        RK_U32 cmv_th_rb_cnt1    : 16;
    } st_cmv_rb_num0;

    /* 0x0000411c reg4167 */
    struct {
        RK_U32 cmv_th_rb_cnt2    : 16;
        RK_U32 cmv_th_rb_cnt3    : 16;
    } st_cmv_rb_num1;

    /* 0x00004120 reg4168 */
    struct {
        RK_U32 org_y_r_max_value    : 8;
        RK_U32 org_y_r_min_value    : 8;
        RK_U32 org_u_g_max_value    : 8;
        RK_U32 org_u_g_min_value    : 8;
    } st_vsp_org_value0;

    /* 0x00004124 reg4169 */
    struct {
        RK_U32 org_v_b_max_value    : 8;
        RK_U32 org_v_b_min_value    : 8;
        RK_U32 reserved             : 16;
    } st_vsp_org_value1;

    /* 0x00004128 reg4170 */
    struct {
        RK_U32 jpeg_y_r_max_value    : 8;
        RK_U32 jpeg_y_r_min_value    : 8;
        RK_U32 jpeg_u_g_max_value    : 8;
        RK_U32 jpeg_u_g_min_value    : 8;
    } st_vsp_jpeg_value0;

    /* 0x0000412c reg4171 */
    struct {
        RK_U32 jpeg_v_b_max_value    : 8;
        RK_U32 jpeg_v_b_min_value    : 8;
        RK_U32 reserved              : 16;
    } st_vsp_jpeg_value1;

    /* 0x00004130 reg4172 */
    RK_U32 dsp_y_sum;

    /* 0x00004134 reg4173 */
    RK_U32 acc_zero_mv;

    /* 0x00004138 reg4174 */
    struct {
        RK_U32 ref1_inter8_num   : 23;
        RK_U32 reserved1         : 9;
    } st_ref1_inter8;

    /* 0x0000413c reg4175 */
    struct {
        RK_U32 acc_block_num    : 18;
        RK_U32 reserved         : 14;
    } st_blk_avb_sum;

    /* 0x00004140 reg4176 */
    struct {
        RK_U32 reserved1          : 15;
        RK_U32 acc_cmplx_num      : 17;
    } st_cmplx_sum;

    /* 0x00004144 reg4177 */
    struct {
        RK_U32 reserved1          : 15;
        RK_U32 acc_cover16_num    : 17;
    } st_cover_sum;

    /* 0x00004148 reg4178 */
    struct {
        RK_U32 reserved1          : 15;
        RK_U32 acc_bndry16_num    : 17;
    } st_bndry16_sum;

    /* 0x0000414c reg4179 */
    RK_U32 num0_grdnt_point_dep0;

    /* 0x00004150 reg4180 */
    RK_U32 num1_grdnt_point_dep0;

    /* 0x00004154 reg4181 */
    RK_U32 num2_grdnt_point_dep0;

    /* 0x00004158 reg4182 */
    struct {
        RK_U32 num                 : 19;
        RK_U32 reserved            : 13;
    } st_ref1_inter32;

    /* 0x0000415c reg4183 */
    struct {
        RK_U32 num                 : 21;
        RK_U32 reserved            : 11;
    } st_ref1_inter16;

    /* 0x00004160 reg4184 */
    struct {
        RK_U32 num0_point_skin   : 21;
        RK_U32 reserved1         : 11;
    } st_skin_sum0;

    /* 0x00004164 reg4185 */
    struct {
        RK_U32 num1_point_skin   : 21;
        RK_U32 reserved1         : 11;
    } st_skin_sum1;

    /* 0x00004168 reg4186 */
    struct {
        RK_U32 num2_point_skin   : 21;
        RK_U32 reserved1         : 11;
    } st_skin_sum2;

    /* 0x416c - 0x417c */
    RK_U32 reserved4185_4191[5];

    /* 0x00004180 reg4192 -0x0000424c reg4243 */
    RK_U32 st_b8_qp[52];
} Vepu511Status;

/* class: dbg/st/axipn */
/* 0x00005000 reg5120 - 0x00005230 reg5260*/
typedef struct Vepu511Dbg_t {
    /* 0x00005000 reg5120 */
    struct {
        RK_U32 vsp0_pos_x    : 16;
        RK_U32 vsp0_pos_y    : 16;
    } st_ppl_pos_vsp0;

    /* 0x00005004 reg5121 */
    struct {
        RK_U32 vsp1_pos_x    : 16;
        RK_U32 vsp1_pos_y    : 16;
    } st_ppl_pos_vsp1;

    /* 0x00005008 reg5122 */
    struct {
        RK_U32 vsp2_pos_x    : 16;
        RK_U32 vsp2_pos_y    : 16;
    } st_ppl_pos_vsp2;

    /* 0x0000500c reg5123 */
    struct {
        RK_U32 cme_pos_x    : 16;
        RK_U32 cme_pos_y    : 16;
    } st_ppl_pos_cme;

    /* 0x00005010 reg5124 */
    struct {
        RK_U32 swin_cmd_x    : 16;
        RK_U32 swin_cmd_y    : 16;
    } st_ppl_cmd_swin;

    /* 0x00005014 reg5125 */
    struct {
        RK_U32 swin_pos_x    : 16;
        RK_U32 swin_pos_y    : 16;
    } st_ppl_pos_swin;

    /* 0x00005018 reg5126 */
    struct {
        RK_U32 pren_pos_x    : 16;
        RK_U32 pren_pos_y    : 16;
    } st_ppl_pos_pren;

    /* 0x0000501c reg5127 */
    struct {
        RK_U32 rfme_pos_x    : 16;
        RK_U32 rfme_pos_y    : 16;
    } st_ppl_pos_rfme;

    /* 0x00005020 reg5128 */
    struct {
        RK_U32 rdo_pos_x    : 16;
        RK_U32 rdo_pos_y    : 16;
    } st_ppl_pos_rdo;

    /* 0x00005024 reg5129 */
    struct {
        RK_U32 lpf_pos_x    : 16;
        RK_U32 lpf_pos_y    : 16;
    } st_ppl_pos_lpf;

    /* 0x00005028 reg5130 */
    struct {
        RK_U32 etpy_pos_x    : 16;
        RK_U32 etpy_pos_y    : 16;
    } st_ppl_pos_etpy;

    /* 0x0000502c reg5131 */
    struct {
        RK_U32 jsp0_pos_x    : 16;
        RK_U32 jsp0_pos_y    : 16;
    } st_ppl_pos_jsp0;

    /* 0x00005030 reg5132 */
    struct {
        RK_U32 jsp1_pos_x    : 16;
        RK_U32 jsp1_pos_y    : 16;
    } st_ppl_pos_jsp1;

    /* 0x00005034 reg5133 */
    struct {
        RK_U32 jsp2_pos_x    : 16;
        RK_U32 jsp2_pos_y    : 16;
    } st_ppl_pos_jsp2;

    /* 0x00005038 reg5134 */
    struct {
        RK_U32 jpeg_pos_x   : 16;
        RK_U32 jpeg_pos_y   : 16;
    } st_ppl_pos_jpeg;

    /* 0x0000503c reg5135 */
    struct {
        RK_U32 vhdr_pos_y   : 16;
        RK_U32 jhdr_pos_y   : 16;
    } dbg_pos_pp_hdr;

    /* 0x00005040 reg5136 */
    struct {
        RK_U32 vhdr_src_err      : 1;
        RK_U32 vhdr_cmd_flst     : 1;
        RK_U32 reserved1         : 2;
        RK_U32 vsp0_org_err      : 1;
        RK_U32 vsp0_pp2_err      : 1;
        RK_U32 vsp0_paral_err    : 1;
        RK_U32 vsp0_cmd_flst     : 1;
        RK_U32 vhdr_vld          : 1;
        RK_U32 vhdr_rdy          : 1;
        RK_U32 vsp0_cmd_vld      : 1;
        RK_U32 vsp0_cmd_rdy      : 1;
        RK_U32 reserved2         : 12;
        RK_U32 vsp0_wrk_ena      : 1;
        RK_U32 reserved3         : 3;
        RK_U32 vsp0_wrk          : 1;
        RK_U32 vsp0_tout         : 1;
        RK_U32 reserved4         : 2;
    } dbg_ctrl_vsp0;

    /* 0x00005044 reg5137 */
    struct {
        RK_U32 vsp2_org_err     : 1;
        RK_U32 vsp2_meiw_err    : 1;
        RK_U32 vsp2_olm_err     : 1;
        RK_U32 reserved1        : 9;
        RK_U32 vsp2_madi_vld    : 1;
        RK_U32 vsp2_madi_rdy    : 1;
        RK_U32 vsp2_aq_vld      : 1;
        RK_U32 vsp2_aq_rdy      : 1;
        RK_U32 reserved2        : 8;
        RK_U32 vsp2_wrk_ena     : 1;
        RK_U32 reserved3        : 3;
        RK_U32 vsp2_wrk         : 1;
        RK_U32 vsp2_tout        : 1;
        RK_U32 reserved4        : 2;
    } dbg_ctrl_vsp2;

    /* 0x00005048 reg5138 */
    struct {
        RK_U32 cme_org_err     : 1;
        RK_U32 cme_roi_err     : 1;
        RK_U32 cme_win_err     : 1;
        RK_U32 cme_cmmv_err    : 1;
        RK_U32 cme_smvp_err    : 1;
        RK_U32 cme_meiw_err    : 1;
        RK_U32 cme_olm_err     : 1;
        RK_U32 cme_smr_err     : 1;
        RK_U32 cme_scqp_err    : 1;
        RK_U32 cme_dual_err    : 1;
        RK_U32 reserved        : 2;
        RK_U32 cme_qp_err      : 1;
        RK_U32 reserved1       : 11;
        RK_U32 cme_wrk_ena     : 1;
        RK_U32 reserved2       : 3;
        RK_U32 cme_wrk         : 1;
        RK_U32 cme_tout        : 1;
        RK_U32 reserved3       : 2;
    } dbg_ctrl_cme;

    /* 0x0000504c reg5139 */
    struct {
        RK_U32 cme_madp_vld       : 1;
        RK_U32 cme_madp_rdy0      : 1;
        RK_U32 cme_madp_rdy1      : 1;
        RK_U32 reserved           : 1;
        RK_U32 cme_mv16_vld       : 1;
        RK_U32 cme_mv16_rdy       : 1;
        RK_U32 cme_st_vld         : 1;
        RK_U32 cme_st_rdy         : 1;
        RK_U32 cme_stat_vld       : 1;
        RK_U32 cme_stat_rdy       : 1;
        RK_U32 cme_diff_vld       : 1;
        RK_U32 cme_diff_rdy       : 1;
        RK_U32 cme_cmmv_vld       : 1;
        RK_U32 cme_cmmv_rdy       : 1;
        RK_U32 cme_smvp_vld       : 1;
        RK_U32 cme_smvp_rdy       : 1;
        RK_U32 rdo_tmvp_rvld      : 1;
        RK_U32 rdo_tmvp_rrdy      : 1;
        RK_U32 rdo_tmvp_wvld      : 1;
        RK_U32 rdo_tmvp_wrdy      : 1;
        RK_U32 rdo_lbf_wvld       : 1;
        RK_U32 rdo_lbf_wrdy       : 1;
        RK_U32 rdo_lbf_rvld       : 1;
        RK_U32 rdo_lbf_rrdy       : 1;
        RK_U32 rdo_rfmv_rvld      : 1;
        RK_U32 rdo_rfmv_rrdy      : 1;
        RK_U32 rdo_hevc_qp_vld    : 1;
        RK_U32 rdo_hevc_qp_rdy    : 1;
        RK_U32 reserved1          : 4;
    } dbg_hs_ppl;

    /* 0x00005050 reg5140 */
    struct {
        RK_U32 swin_org_err      : 1;
        RK_U32 swin_ref_err      : 1;
        RK_U32 reserved          : 10;
        RK_U32 swin_cmd_vld      : 1;
        RK_U32 swin_cmd_rdy      : 1;
        RK_U32 reserved1         : 2;
        RK_U32 swin_buff_ptr     : 2;
        RK_U32 swin_buff_num0    : 2;
        RK_U32 swin_buff_num1    : 2;
        RK_U32 swin_buff_num2    : 2;
        RK_U32 reserved2         : 4;
        RK_U32 swin_wrk          : 1;
        RK_U32 swin_tout         : 1;
        RK_U32 reserved3         : 2;
    } dbg_ctrl_swin;

    /* 0x00005054 reg5141 */
    struct {
        RK_U32 pnra_org_err     : 1;
        RK_U32 pnra_qp_err      : 1;
        RK_U32 pnra_rfme_err    : 1;
        RK_U32 reserved         : 9;
        RK_U32 pnra_olm_vld     : 1;
        RK_U32 pnra_olm_rdy     : 1;
        RK_U32 pnra_subj_vld    : 1;
        RK_U32 pnra_subj_rdy    : 1;
        RK_U32 reserved1        : 8;
        RK_U32 pnra_wrk_ena     : 1;
        RK_U32 reserved2        : 3;
        RK_U32 pnra_wrk         : 1;
        RK_U32 pnra_tout        : 1;
        RK_U32 reserved3        : 2;
    } dbg_ctrl_pren;

    /* 0x00005058 reg5142 */
    struct {
        RK_U32 rfme_org_err     : 1;
        RK_U32 rfme_ref_err     : 1;
        RK_U32 rfme_cmmv_err    : 1;
        RK_U32 rfme_rfmv_err    : 1;
        RK_U32 rfme_tmvp_err    : 1;
        RK_U32 rfme_qp_err      : 1;
        RK_U32 rfme_pnra_err    : 1;
        RK_U32 reserved         : 5;
        RK_U32 rfme_tmvp_vld    : 1;
        RK_U32 rfme_tmvp_rdy    : 1;
        RK_U32 rfme_smvp_vld    : 1;
        RK_U32 rfme_smvp_rdy    : 1;
        RK_U32 rfme_cmmv_vld    : 1;
        RK_U32 rfme_cmmv_rdy    : 1;
        RK_U32 rfme_rfmv_vld    : 1;
        RK_U32 rfme_rfmv_rdy    : 1;
        RK_U32 reserved1        : 8;
        RK_U32 rfme_wrk         : 1;
        RK_U32 rfme_tout        : 1;
        RK_U32 reserved2        : 2;
    } dbg_ctrl_rfme;

    /* 0x0000505c reg5143 */
    struct {
        RK_U32 rdo_org_err         : 1;
        RK_U32 rdo_ref_err         : 1;
        RK_U32 rdo_inf_err         : 1;
        RK_U32 rdo_coef_err        : 1;
        RK_U32 rdo_lbfr_err        : 1;
        RK_U32 rdo_lbfw_err        : 1;
        RK_U32 rdo_madi_lbf_err    : 1;
        RK_U32 rdo_tmvp_wr_err     : 1;
        RK_U32 rdo_smear_err       : 1;
        RK_U32 rdo_mbqp_err        : 1;
        RK_U32 rdo_rc_err          : 1;
        RK_U32 rdo_ent_err         : 1;
        RK_U32 rdo_lpf_err         : 1;
        RK_U32 rdo_st_err          : 1;
        RK_U32 rdo_tmvp_rd_err     : 1;
        RK_U32 rdo_rfmv_err        : 1;
        RK_U32 reserved            : 12;
        RK_U32 rdo_wrk             : 1;
        RK_U32 rdo_tout            : 1;
        RK_U32 reserved1           : 2;
    } dbg_ctrl_rdo;

    /* 0x00005060 reg5144 */
    struct {
        RK_U32 lpf_org_err     : 1;
        RK_U32 lpf_rdo_err     : 1;
        RK_U32 reserved        : 10;
        RK_U32 lpf_rcol_vld    : 1;
        RK_U32 lpf_rcol_rdy    : 1;
        RK_U32 lpf_lbf_wvld    : 1;
        RK_U32 lpf_lbf_wrdy    : 1;
        RK_U32 lpf_lbf_rvld    : 1;
        RK_U32 lpf_lbf_rrdy    : 1;
        RK_U32 reserved1       : 6;
        RK_U32 lpf_wrk_ena     : 1;
        RK_U32 reserved2       : 3;
        RK_U32 lpf_wrk         : 1;
        RK_U32 lpf_tout        : 1;
        RK_U32 reserved3       : 2;
    } dbg_ctrl_lpf;

    /* 0x00005064 reg5145 */
    struct {
        RK_U32 reserved         : 12;
        RK_U32 etpy_slf_full    : 1;
        RK_U32 etpy_bsw_vld     : 1;
        RK_U32 etpy_bsw_rdy     : 1;
        RK_U32 reserved1        : 9;
        RK_U32 etpy_wrk_ena     : 1;
        RK_U32 reserved2        : 3;
        RK_U32 etpy_wrk         : 1;
        RK_U32 etpy_tout        : 1;
        RK_U32 reserved3        : 2;
    } dbg_ctrl_etpy;

    /* 0x00005068 reg5146 */
    struct {
        RK_U32 jhdr_src_err      : 1;
        RK_U32 jhdr_cmd_flst     : 1;
        RK_U32 reserved1         : 2;
        RK_U32 jsp0_org_err      : 1;
        RK_U32 jsp0_pp2_err      : 1;
        RK_U32 jsp0_paral_err    : 1;
        RK_U32 jsp0_cmd_flst     : 1;
        RK_U32 jhdr_vld          : 1;
        RK_U32 jhdr_rdy          : 1;
        RK_U32 jsp0_cmd_vld      : 1;
        RK_U32 jsp0_cmd_rdy      : 1;
        RK_U32 reserved2         : 12;
        RK_U32 jsp0_wrk_ena      : 1;
        RK_U32 reserved3         : 3;
        RK_U32 jsp0_wrk          : 1;
        RK_U32 jsp0_tout         : 1;
        RK_U32 reserved4         : 2;
    } dbg_ctrl_jsp0;

    /* 0x0000506c reg5147 */
    struct {
        RK_U32 jsp2_org_err     : 1;
        RK_U32 reserved1        : 11;
        RK_U32 jsp2_madi_vld    : 1;
        RK_U32 jsp2_madi_rdy    : 1;
        RK_U32 reserved2        : 10;
        RK_U32 jsp2_wrk_ena     : 1;
        RK_U32 reserved3        : 3;
        RK_U32 jsp2_wrk         : 1;
        RK_U32 jsp2_tout        : 1;
        RK_U32 reserved4        : 2;
    } dbg_ctrl_jsp2;

    /* 0x00005070 reg5148 */
    struct {
        RK_U32 jpeg_org_err    : 1;
        RK_U32 jpeg_st_err     : 1;
        RK_U32 reserved        : 10;
        RK_U32 jpg_slf_full    : 1;
        RK_U32 jpg_bsw_vld     : 1;
        RK_U32 jpg_bsw_rdy     : 1;
        RK_U32 reserved1       : 9;
        RK_U32 jpeg_wrk_ena    : 1;
        RK_U32 reserved2       : 3;
        RK_U32 jpeg_wrk        : 1;
        RK_U32 jpeg_tout       : 1;
        RK_U32 reserved3       : 2;
    } dbg_ctrl_jpeg;

    /* 0x00005074 reg5149 */
    struct {
        RK_U32 vsp1_org_err    : 1;
        RK_U32 reserved        : 7;
        RK_U32 vsp1_wrk_ena    : 1;
        RK_U32 reserved1       : 3;
        RK_U32 vsp1_wrk        : 1;
        RK_U32 vsp1_tout       : 1;
        RK_U32 reserved2       : 2;
        RK_U32 jsp1_org_err    : 1;
        RK_U32 reserved3       : 7;
        RK_U32 jsp1_wrk_ena    : 1;
        RK_U32 reserved4       : 3;
        RK_U32 jsp1_wrk        : 1;
        RK_U32 jsp1_tout       : 1;
        RK_U32 reserved5       : 2;
    } dbg_ctrl_pp1;

    /* 0x00005078 reg5150 */
    struct {
        RK_U32 pp2_frm_done     : 1;
        RK_U32 cime_frm_done    : 1;
        RK_U32 jpeg_frm_done    : 1;
        RK_U32 rdo_frm_done     : 1;
        RK_U32 lpf_frm_done     : 1;
        RK_U32 ent_frm_done     : 1;
        RK_U32 reserved         : 2;
        RK_U32 criw_frm_done    : 1;
        RK_U32 meiw_frm_done    : 1;
        RK_U32 smiw_frm_done    : 1;
        RK_U32 dma_frm_idle     : 1;
        RK_U32 reserved1        : 12;
        RK_U32 video_ena        : 1;
        RK_U32 jpeg_ena         : 1;
        RK_U32 vepu_pp_ena      : 1;
        RK_U32 reserved2        : 1;
        RK_U32 frm_wrk          : 1;
        RK_U32 frm_tout         : 1;
        RK_U32 reserved3        : 2;
    } dbg_tctrl;

    /* 0x0000507c reg5151 */
    struct {
        RK_U32 enc_frm_cur     : 3;
        RK_U32 lkt_src_paus    : 1;
        RK_U32 lkt_cfg_paus    : 1;
        RK_U32 reserved        : 27;
    } dbg_frm_task;

    /* 0x00005080 reg5152 */
    struct {
        RK_U32 sli_num     : 15;
        RK_U32 reserved    : 17;
    } st_sli_num;

    /* 0x5084 - 0x50fc */
    RK_U32 reserved5153_5183[31];

    /* 0x00005100 reg5184 */
    struct {
        RK_U32 empty_oafifo        : 1;
        RK_U32 full_cmd_oafifo     : 1;
        RK_U32 full_data_oafifo    : 1;
        RK_U32 empty_iafifo        : 1;

        RK_U32 full_cmd_iafifo     : 1;
        RK_U32 full_info_iafifo    : 1;
        RK_U32 fbd_brq_st          : 4;
        RK_U32 fbd_hdr_vld         : 1;
        RK_U32 fbd_bmng_end        : 1;

        RK_U32 nfbd_req_st         : 4;
        RK_U32 acc_axi_cmd         : 8;
        RK_U32 reserved            : 8;
    } dbg_pp_st;

    /* 0x00005104 reg5185 */
    struct {
        RK_U32 r_ena_lambd        : 1;
        RK_U32 r_fst_swinw_end    : 1;
        RK_U32 r_swinw_end        : 1;
        RK_U32 r_cnt_swinw        : 1;

        RK_U32 r_dspw_end         : 1;
        RK_U32 r_dspw_cnt         : 1;
        RK_U32 i_sjgen_work       : 1;
        RK_U32 r_end_rspgen       : 1;

        RK_U32 r_cost_gate        : 1;
        RK_U32 r_ds_gate          : 1;
        RK_U32 r_mvp_gate         : 1;
        RK_U32 i_smvp_arrdy       : 1;

        RK_U32 i_smvp_arvld       : 1;
        RK_U32 i_stptr_wrdy       : 1;
        RK_U32 i_stptr_wvld       : 1;
        RK_U32 i_rdy_atf          : 1;

        RK_U32 i_vld_atf          : 1;
        RK_U32 i_rdy_bmv16        : 1;
        RK_U32 i_vld_bmv16        : 1;
        RK_U32 i_wr_dsp           : 1;

        RK_U32 i_rdy_dsp          : 1;
        RK_U32 i_vld_dsp          : 1;
        RK_U32 r_rdy_org          : 1;
        RK_U32 i_vld_org          : 1;

        RK_U32 i_rdy_state        : 1;
        RK_U32 i_vld_state        : 1;
        RK_U32 i_rdy_madp         : 1;
        RK_U32 i_vld_madp         : 1;

        RK_U32 i_rdy_diff         : 1;
        RK_U32 i_vld_diff         : 1;
        RK_U32 reserved           : 2;
    } dbg_cime_st;

    /* 0x00005108 reg5186 */
    RK_U32 swin_dbg_inf;

    /* 0x0000510c reg5187 */
    struct {
        RK_U32 bbrq_cmps_left_len2    : 1;
        RK_U32 bbrq_cmps_left_len1    : 1;
        RK_U32 cmps_left_len0         : 1;
        RK_U32 bbrq_rdy2              : 1;
        RK_U32 dcps_vld2              : 1;
        RK_U32 bbrq_rdy1              : 1;
        RK_U32 dcps_vld1              : 1;
        RK_U32 bbrq_rdy0              : 1;
        RK_U32 dcps_vld0              : 1;
        RK_U32 hb_rdy2                : 1;
        RK_U32 bbrq_vld2              : 1;
        RK_U32 hb_rdy1                : 1;
        RK_U32 bbrq_vld1              : 1;
        RK_U32 hb_rdy0                : 1;
        RK_U32 bbrq_vld0              : 1;
        RK_U32 idle_msb2              : 1;
        RK_U32 idle_msb1              : 1;
        RK_U32 idle_msb0              : 1;
        RK_U32 cur_state_dcps         : 1;
        RK_U32 cur_state_bbrq         : 1;
        RK_U32 cur_state_hb           : 1;
        RK_U32 cke_bbrq_dcps          : 1;
        RK_U32 cke_dcps               : 1;
        RK_U32 cke_bbrq               : 1;
        RK_U32 rdy_lwcd_rsp           : 1;
        RK_U32 vld_lwcd_rsp           : 1;
        RK_U32 rdy_lwcd_req           : 1;
        RK_U32 vld_lwcd_req           : 1;
        RK_U32 rdy_lwrsp              : 1;
        RK_U32 vld_lwrsp              : 1;
        RK_U32 rdy_lwreq              : 1;
        RK_U32 vld_lwreq              : 1;
    } dbg_fbd_hhit0;

    /* 0x00005110 reg5188 */
    RK_U32 rfme_dbg_inf;

    /* 0x5114 */
    RK_U32 reserved_5189;

    /* 0x00005118 reg5190 */
    RK_U32 dbg0_fbd;

    /* 0x0000511c reg5191 */
    RK_U32 dbg1_fbd;

    /* 0x00005120 reg5192 */
    RK_U32 rdo_dbg0;

    /* 0x00005124 reg5193 */
    RK_U32 rdo_dbg1;

    /* 0x00005128 reg5194 */
    struct {
        RK_U32 h264_sh_st_cs    : 4;
        RK_U32 rsd_st_cs        : 4;
        RK_U32 h264_sd_st_cs    : 5;
        RK_U32 etpy_rdy         : 1;
        RK_U32 reserved         : 18;
    } dbg_etpy;

    /* 0x0000512c reg5195 */
    struct {
        RK_U32 chl_aw_vld        : 10;
        RK_U32 chl_aw_rdy        : 10;
        RK_U32 aw_vld_arb        : 1;
        RK_U32 aw_rdy_arb        : 1;
        RK_U32 aw_vld_crosclk    : 1;
        RK_U32 aw_rdy_crosclk    : 1;
        RK_U32 aw_rdy_mmu        : 1;
        RK_U32 aw_vld_mmu        : 1;
        RK_U32 aw_rdy_axi        : 1;
        RK_U32 aw_vld_axi        : 1;
        RK_U32 reserved          : 4;
    } dbg_dma_aw;

    /* 0x00005130 reg5196 */
    struct {
        RK_U32 chl_w_vld        : 10;
        RK_U32 chl_w_rdy        : 10;
        RK_U32 w_vld_arb        : 1;
        RK_U32 w_rdy_arb        : 1;
        RK_U32 w_vld_crosclk    : 1;
        RK_U32 w_rdy_crosclk    : 1;
        RK_U32 w_rdy_mmu        : 1;
        RK_U32 w_vld_mmu        : 1;
        RK_U32 w_rdy_axi        : 1;
        RK_U32 w_vld_axi        : 1;
        RK_U32 reserved         : 4;
    } dbg_dma_w;

    /* 0x00005134 reg5197 */
    struct {
        RK_U32 chl_ar_vld        : 9;
        RK_U32 chl_ar_rdy        : 9;
        RK_U32 reserved          : 2;
        RK_U32 ar_vld_arb        : 1;
        RK_U32 ar_rdy_arb        : 1;
        RK_U32 ar_vld_crosclk    : 1;
        RK_U32 ar_rdy_crosclk    : 1;
        RK_U32 ar_rdy_mmu        : 1;
        RK_U32 ar_vld_mmu        : 1;
        RK_U32 ar_rdy_axi        : 1;
        RK_U32 ar_vld_axi        : 1;
        RK_U32 reserved1         : 4;
    } dbg_dma_ar;

    /* 0x00005138 reg5198 */
    struct {
        RK_U32 chl_r_vld        : 9;
        RK_U32 chl_r_rdy        : 9;
        RK_U32 reserved         : 2;
        RK_U32 r_vld_arb        : 1;
        RK_U32 r_rdy_arb        : 1;
        RK_U32 r_vld_crosclk    : 1;
        RK_U32 r_rdy_crosclk    : 1;
        RK_U32 r_rdy_mmu        : 1;
        RK_U32 r_vld_mmu        : 1;
        RK_U32 r_rdy_axi        : 1;
        RK_U32 r_vld_axi        : 1;
        RK_U32 b_rdy_mmu        : 1;
        RK_U32 b_vld_mmu        : 1;
        RK_U32 b_rdy_axi        : 1;
        RK_U32 b_vld_axi        : 1;
    } dbg_dma_r;

    /* 0x0000513c reg5199 */
    struct {
        RK_U32 dbg_sclr    : 20;
        RK_U32 dbg_arb     : 12;
    } dbg_dma_dbg0;

    /* 0x00005140 reg5200 */
    struct {
        RK_U32 bsw_fsm_stus     : 4;
        RK_U32 bsw_aw_full      : 1;
        RK_U32 bsw_rdy_ent      : 1;
        RK_U32 bsw_vld_ent      : 1;
        RK_U32 jpg_bsw_stus     : 4;
        RK_U32 jpg_aw_full      : 1;
        RK_U32 jpg_bsw_rdy      : 1;
        RK_U32 jpg_bsw_vld      : 1;
        RK_U32 crpw_fsm_stus    : 3;
        RK_U32 hdwr_rdy         : 1;
        RK_U32 hdwr_vld         : 1;
        RK_U32 bdwr_rdy         : 1;
        RK_U32 bdwr_vld         : 1;
        RK_U32 nfbc_rdy         : 1;
        RK_U32 nfbc_vld         : 1;
        RK_U32 dsp_fsm_stus     : 2;
        RK_U32 dsp_wr_flg       : 1;
        RK_U32 dsp_rsy          : 1;
        RK_U32 dsp_vld          : 1;
        RK_U32 lpfw_fsm_stus    : 3;
        RK_U32 reserved         : 1;
    } dbg_dma_dbg1;

    /* 0x00005144 reg5201 */
    struct {
        RK_U32 awvld_mdo     : 1;
        RK_U32 awrdy_mdo     : 1;
        RK_U32 wvld_mdo      : 1;
        RK_U32 wrdy_mdo      : 1;
        RK_U32 awvld_odo     : 1;
        RK_U32 awrdy_odo     : 1;
        RK_U32 wvld_odo      : 1;
        RK_U32 wrdy_odo      : 1;
        RK_U32 awvld_rfmw    : 1;
        RK_U32 awrdy_rfmw    : 1;
        RK_U32 wvld_rfmw     : 1;
        RK_U32 wrdy_rfmw     : 1;
        RK_U32 arvld_rfmr    : 1;
        RK_U32 arrdy_rfmr    : 1;
        RK_U32 rvld_rfmr     : 1;
        RK_U32 rrdy_rfmr     : 1;
        RK_U32 reserved      : 16;
    } dbg_dma_vpp;

    /* 0x00005148 reg5202 */
    struct {
        RK_U32 rdo_st      : 20;
        RK_U32 reserved    : 12;
    } dbg_rdo_st;

    /* 0x0000514c reg5203 */
    struct {
        RK_U32 lpf_work               : 1;
        RK_U32 rdo_par_nrdy           : 1;
        RK_U32 rdo_rcn_nrdy           : 1;
        RK_U32 lpf_rcn_rdy            : 1;
        RK_U32 dblk_work              : 1;
        RK_U32 sao_work               : 1;
        RK_U32 reserved               : 18;
        RK_U32 tile_bdry_read         : 1;
        RK_U32 tile_bdry_write        : 1;
        RK_U32 tile_bdry_rrdy         : 1;
        RK_U32 rdo_read_tile_bdry     : 1;
        RK_U32 rdo_write_tile_bdry    : 1;
        RK_U32 reserved1              : 3;
    } dbg_lpf;

    /* 0x5150 */
    RK_U32 reserved_5204;

    /* 0x00005154 reg5205 */
    RK_U32 dbg0_cache;

    /* 0x00005158 reg5206 */
    RK_U32 dbg2_fbd;

    /* 0x515c */
    RK_U32 reserved_5207;

    /* 0x00005160 reg5208 */
    struct {
        RK_U32 ebuf_diff_cmd    : 8;
        RK_U32 lbuf_lpf_ncnt    : 7;
        RK_U32 lbuf_lpf_cien    : 1;
        RK_U32 lbuf_rdo_ncnt    : 7;
        RK_U32 lbuf_rdo_cien    : 1;
        RK_U32 lbuf_tctrl_ncnt  : 7;
        RK_U32 lbuf_tctrl_cien  : 1;
    } dbg_lbuf0;

    /* 0x00005164 reg5209 */
    struct {
        RK_U32 rvld_ebfr          : 1;
        RK_U32 rrdy_ebfr          : 1;
        RK_U32 arvld_ebfr         : 1;
        RK_U32 arrdy_ebfr         : 1;
        RK_U32 wvld_ebfw          : 1;
        RK_U32 wrdy_ebfw          : 1;
        RK_U32 awvld_ebfw         : 1;
        RK_U32 awrdy_ebfw         : 1;
        RK_U32 lpf_lbuf_rvld      : 1;
        RK_U32 lpf_lbuf_rrdy      : 1;
        RK_U32 lpf_lbuf_wvld      : 1;
        RK_U32 lpf_lbuf_wrdy      : 1;
        RK_U32 rdo_lbuf_rvld      : 1;
        RK_U32 rdo_lbuf_rrdy      : 1;
        RK_U32 rdo_lbuf_wvld      : 1;
        RK_U32 rdo_lbuf_wrdy      : 1;
        RK_U32 fme_lbuf_rvld      : 1;
        RK_U32 fme_lbuf_rrdy      : 1;
        RK_U32 cme_lbuf_rvld      : 1;
        RK_U32 cme_lbuf_rrdy      : 1;
        RK_U32 smear_lbuf_rvld    : 1;
        RK_U32 smear_lbuf_rrdy    : 1;
        RK_U32 smear_lbuf_wvld    : 1;
        RK_U32 smear_lbuf_wrdy    : 1;
        RK_U32 depth_lbuf_wvld    : 1;
        RK_U32 depth_lbuf_wrdy    : 1;
        RK_U32 rdo_lbufw_flag     : 1;
        RK_U32 rdo_lbufr_flag     : 1;
        RK_U32 cme_lbufr_flag     : 1;
        RK_U32 reserved           : 3;
    } dbg_lbuf1;

    /* 0x00005168 reg5210 */
    struct {
        RK_U32 dbg_isp_fcnt    : 8;
        RK_U32 dbg_isp_fcyc    : 24;
    } dbg_dvbm_isp0;

    /* 0x0000516c reg5211 */
    struct {
        RK_U32 dbg_isp_lcnt    : 14;
        RK_U32 dbg_isp_ltgl    : 1;
        RK_U32 dvbm_isp_fsid   : 1;
        RK_U32 dbg_isp_fcnt    : 8;
        RK_U32 dbg_isp_oflw    : 1;
        RK_U32 dbg_isp_ftgl    : 1;
        RK_U32 dbg_isp_full    : 1;
        RK_U32 dbg_isp_work    : 1;
        RK_U32 dbg_isp_lvld    : 1;
        RK_U32 dbg_isp_lrdy    : 1;
        RK_U32 dbg_isp_fvld    : 1;
        RK_U32 dbg_isp_frdy    : 1;
    } dbg_dvbm_isp1;

    /* 0x00005170 reg5212 */
    struct {
        RK_U32 dbg_bf0_isp_lcnt    : 14;
        RK_U32 dbg_bf0_isp_llst    : 1;
        RK_U32 dbg_bf0_isp_sofw    : 1;
        RK_U32 dbg_bf0_isp_fcnt    : 8;
        RK_U32 dbg_bf0_isp_fsid    : 1;
        RK_U32 reserved            : 5;
        RK_U32 dbg_bf0_isp_pnt     : 1;
        RK_U32 dbg_bf0_vpu_pnt     : 1;
    } dbg_dvbm_buf0_inf0;

    /* 0x00005174 reg5213 */
    struct {
        RK_U32 dbg_bf0_src_lcnt    : 14;
        RK_U32 dbg_bf0_src_llst    : 1;
        RK_U32 reserved            : 1;
        RK_U32 dbg_bf0_vpu_lcnt    : 14;
        RK_U32 dbg_bf0_vpu_llst    : 1;
        RK_U32 dbg_bf0_vpu_vofw    : 1;
    } dbg_dvbm_buf0_inf1;

    /* 0x00005178 reg5214 */
    struct {
        RK_U32 dbg_bf1_isp_lcnt    : 14;
        RK_U32 dbg_bf1_isp_llst    : 1;
        RK_U32 dbg_bf1_isp_sofw    : 1;
        RK_U32 dbg_bf1_isp_fcnt    : 8;
        RK_U32 dbg_bf1_isp_fsid    : 1;
        RK_U32 reserved            : 5;
        RK_U32 dbg_bf1_isp_pnt     : 1;
        RK_U32 dbg_bf1_vpu_pnt     : 1;
    } dbg_dvbm_buf1_inf0;

    /* 0x0000517c reg5215 */
    struct {
        RK_U32 dbg_bf1_src_lcnt    : 14;
        RK_U32 dbg_bf1_src_llst    : 1;
        RK_U32 reserved            : 1;
        RK_U32 dbg_bf1_vpu_lcnt    : 14;
        RK_U32 dbg_bf1_vpu_llst    : 1;
        RK_U32 dbg_bf1_vpu_vofw    : 1;
    } dbg_dvbm_buf1_inf1;

    /* 0x00005180 reg5216 */
    struct {
        RK_U32 dbg_bf2_isp_lcnt    : 14;
        RK_U32 dbg_bf2_isp_llst    : 1;
        RK_U32 dbg_bf2_isp_sofw    : 1;
        RK_U32 dbg_bf2_isp_fcnt    : 8;
        RK_U32 dbg_bf2_isp_fsid    : 1;
        RK_U32 reserved            : 5;
        RK_U32 dbg_bf2_isp_pnt     : 1;
        RK_U32 dbg_bf2_vpu_pnt     : 1;
    } dbg_dvbm_buf2_inf0;

    /* 0x00005184 reg5217 */
    struct {
        RK_U32 dbg_bf2_src_lcnt    : 14;
        RK_U32 dbg_bf2_src_llst    : 1;
        RK_U32 reserved            : 1;
        RK_U32 dbg_bf2_vpu_lcnt    : 14;
        RK_U32 dbg_bf2_vpu_llst    : 1;
        RK_U32 dbg_bf2_vpu_vofw    : 1;
    } dbg_dvbm_buf2_inf1;

    /* 0x00005188 reg5218 */
    struct {
        RK_U32 dbg_bf3_isp_lcnt    : 14;
        RK_U32 dbg_bf3_isp_llst    : 1;
        RK_U32 dbg_bf3_isp_sofw    : 1;
        RK_U32 dbg_bf3_isp_fcnt    : 1;
        RK_U32 dbg_bf3_isp_fsid    : 1;
        RK_U32 reserved            : 12;
        RK_U32 dbg_bf3_isp_pnt     : 1;
        RK_U32 dbg_bf3_vpu_pnt     : 1;
    } dbg_dvbm_buf3_inf0;

    /* 0x0000518c reg5219 */
    struct {
        RK_U32 dbg_bf3_src_lcnt    : 14;
        RK_U32 dbg_bf3_src_llst    : 1;
        RK_U32 reserved            : 1;
        RK_U32 dbg_bf3_vpu_lcnt    : 14;
        RK_U32 dbg_bf3_vpu_llst    : 1;
        RK_U32 dbg_bf3_vpu_vofw    : 1;
    } dbg_dvbm_buf3_inf1;

    /* 0x00005190 reg5220 */
    struct {
        RK_U32 dbg_isp_fptr     : 3;
        RK_U32 dbg_isp_full     : 1;
        RK_U32 dbg_src_fptr     : 3;
        RK_U32 dbg_dvbm_ctrl    : 1;
        RK_U32 dbg_vpu_fptr     : 3;
        RK_U32 dbg_vpu_empt     : 1;
        RK_U32 dbg_vpu_lvld     : 1;
        RK_U32 dbg_vpu_lrdy     : 1;
        RK_U32 dbg_vpu_fvld     : 1;
        RK_U32 dbg_vpu_frdy     : 1;
        RK_U32 dbg_fcnt_misp    : 4;
        RK_U32 dbg_fcnt_mvpu    : 4;
        RK_U32 dbg_fcnt_sofw    : 4;
        RK_U32 dbg_fcnt_vofw    : 4;
    } dbg_dvbm_ctrl;

    /* 0x5194 - 0x519c */
    RK_U32 reserved5221_5223[3];

    /* 0x000051a0 reg5224 */
    RK_U32 dbg_dvbm_buf0_yadr;

    /* 0x000051a4 reg5225 */
    RK_U32 dbg_dvbm_buf0_cadr;

    /* 0x000051a8 reg5226 */
    RK_U32 dbg_dvbm_buf1_yadr;

    /* 0x000051ac reg5227 */
    RK_U32 dbg_dvbm_buf1_cadr;

    /* 0x000051b0 reg5228 */
    RK_U32 dbg_dvbm_buf2_yadr;

    /* 0x000051b4 reg5229 */
    RK_U32 dbg_dvbm_buf2_cadr;

    /* 0x000051b8 reg5230 */
    RK_U32 dbg_dvbm_buf3_yadr;

    /* 0x000051bc reg5231 */
    RK_U32 dbg_dvbm_buf3_cadr;

    /* 0x000051c0 reg5232 */
    struct {
        RK_U32 dchs_rx_cnt    : 11;
        RK_U32 dchs_rx_id     : 2;
        RK_U32 dchs_rx_en     : 1;
        RK_U32 dchs_rx_ack    : 1;
        RK_U32 dchs_rx_req    : 1;
        RK_U32 dchs_tx_cnt    : 11;
        RK_U32 dchs_tx_id     : 2;
        RK_U32 dchs_tx_en     : 1;
        RK_U32 dchs_tx_ack    : 1;
        RK_U32 dchs_tx_req    : 1;
    } dbg_dchs_intfc;

    /* 0x000051c4 reg5233 */
    struct {
        RK_U32 lpfw_tx_cnt       : 11;
        RK_U32 lpfw_tx_en        : 1;
        RK_U32 crpw_tx_cnt       : 11;
        RK_U32 crpw_tx_en        : 1;
        RK_U32 dual_err_updt     : 1;
        RK_U32 dlyc_fifo_oflw    : 1;
        RK_U32 dlyc_tx_vld       : 1;
        RK_U32 dlyc_tx_rdy       : 1;
        RK_U32 dlyc_tx_empty     : 1;
        RK_U32 dchs_tx_idle      : 1;
        RK_U32 dchs_tx_asy       : 1;
        RK_U32 dchs_tx_syn       : 1;
    } dbg_dchs_tx_inf0;

    /* 0x000051c8 reg5234 */
    struct {
        RK_U32 criw_tx_cnt    : 11;
        RK_U32 criw_tx_en     : 1;
        RK_U32 smrw_tx_cnt    : 11;
        RK_U32 smrw_tx_en     : 1;
        RK_U32 reserved       : 8;
    } dbg_dchs_tx_inf1;

    /* 0x000051cc reg5235 */
    struct {
        RK_U32 dual_rx_cnt        : 11;
        RK_U32 dual_rx_id         : 2;
        RK_U32 dual_rx_en         : 1;
        RK_U32 dual_rx_syn        : 1;
        RK_U32 dual_rx_lock       : 1;
        RK_U32 dual_lpfr_dule     : 1;
        RK_U32 dual_cime_dule     : 1;
        RK_U32 dual_clomv_dule    : 1;
        RK_U32 dual_smear_dule    : 1;
        RK_U32 reserved           : 12;
    } dbg_dchs_rx_inf0;

    /* 0x51d0 - 0x51fc */
    RK_U32 reserved5236_5247[12];

    /* 0x00005200 reg5248 */
    RK_U32 frame_cyc;

    /* 0x00005204 reg5249 */
    RK_U32 vsp0_fcyc;

    /* 0x00005208 reg5250 */
    RK_U32 vsp1_fcyc;

    /* 0x0000520c reg5251 */
    RK_U32 vsp2_fcyc;

    /* 0x00005210 reg5252 */
    RK_U32 cme_fcyc;

    /* 0x00005214 reg5253 */
    RK_U32 ldr_fcyc;

    /* 0x00005218 reg5254 */
    RK_U32 rfme_fcyc;

    /* 0x0000521c reg5255 */
    RK_U32 fme_fcyc;

    /* 0x00005220 reg5256 */
    RK_U32 rdo_fcyc;

    /* 0x00005224 reg5257 */
    RK_U32 lpf_fcyc;

    /* 0x00005228 reg5258 */
    RK_U32 etpy_fcyc;

    /* 0x0000522c reg5259 */
    RK_U32 reserved_5259;

    /* 0x00005230 reg5260 */
    RK_U32 jsp0_fcyc;

    /* 0x00005234 reg5261 */
    RK_U32 jsp1_fcyc;

    /* 0x00005238 reg5262 */
    RK_U32 jsp2_fcyc;

    /* 0x0000523c reg5263 */
    RK_U32 jpeg_fcyc;
} Vepu511Dbg;

typedef struct Vepu511OsdCfg_t {
    void *reg_base;
    MppDev dev;
    MppEncOSDData3 *osd_data3;
} Vepu511OsdCfg;

/* ROI block configuration */
typedef struct Vepu511H264RoiBlkCfg {
    RK_U32 qp_adju        : 8;
    RK_U32 mdc_adju_inter : 4;
    RK_U32 mdc_adju_skip  : 4;
    RK_U32 mdc_adju_intra : 4;
    RK_U32 reserved       : 12;
} Vepu511H264RoiBlkCfg;

typedef struct Vepu511H265RoiBlkCfg {
    RK_U32 qp_adju            : 8;
    RK_U32 mdc_adju_intra     : 4;
    RK_U32 mdc_adju_inter     : 4;
    RK_U32 mdc_adju_split     : 4;
    RK_U32 mdc_adju_res_intra : 4;
    RK_U32 mdc_adju_res_inter : 4;
    RK_U32 mdc_adju_res_mv0   : 4;
} Vepu511H265RoiBlkCfg;

typedef struct Vepu511H265RoiDpt1BlkCfg {
    //W0
    RK_U32 cu32_qp_adju              : 8;
    RK_U32 cu32_mdc_adju_intra       : 4;
    RK_U32 cu32_mdc_adju_inter       : 4;
    RK_U32 cu32_mdc_adju_split       : 4;
    RK_U32 cu32_mdc_adju_res_intra   : 4;
    RK_U32 cu32_mdc_adju_res_inter   : 4;
    RK_U32 cu32_mdc_adju_res_mv0     : 4;
    //W1
    RK_U32 cu16_0_qp_adju            : 8;
    RK_U32 cu16_0_mdc_adju_intra     : 4;
    RK_U32 cu16_0_mdc_adju_inter     : 4;
    RK_U32 cu16_0_mdc_adju_split     : 4;
    RK_U32 cu16_0_mdc_adju_res_intra : 4;
    RK_U32 cu16_0_mdc_adju_res_inter : 4;
    RK_U32 cu16_0_mdc_adju_res_mv0   : 4;
    //W2
    RK_U32 cu16_1_qp_adju            : 8;
    RK_U32 cu16_1_mdc_adju_intra     : 4;
    RK_U32 cu16_1_mdc_adju_inter     : 4;
    RK_U32 cu16_1_mdc_adju_split     : 4;
    RK_U32 cu16_1_mdc_adju_res_intra : 4;
    RK_U32 cu16_1_mdc_adju_res_inter : 4;
    RK_U32 cu16_1_mdc_adju_res_mv0   : 4;
    //W3
    RK_U32 cu16_2_qp_adju            : 8;
    RK_U32 cu16_2_mdc_adju_intra     : 4;
    RK_U32 cu16_2_mdc_adju_inter     : 4;
    RK_U32 cu16_2_mdc_adju_split     : 4;
    RK_U32 cu16_2_mdc_adju_res_intra : 4;
    RK_U32 cu16_2_mdc_adju_res_inter : 4;
    RK_U32 cu16_2_mdc_adju_res_mv0   : 4;
    //W4
    RK_U32 cu16_3_qp_adju            : 8;
    RK_U32 cu16_3_mdc_adju_intra     : 4;
    RK_U32 cu16_3_mdc_adju_inter     : 4;
    RK_U32 cu16_3_mdc_adju_split     : 4;
    RK_U32 cu16_3_mdc_adju_res_intra : 4;
    RK_U32 cu16_3_mdc_adju_res_inter : 4;
    RK_U32 cu16_3_mdc_adju_res_mv0   : 4;
} Vepu511H265RoiDpt1BlkCfg;

typedef struct Vepu511JpegCfg_t {
    MppDev dev;
    void *jpeg_reg_base;
    void *reg_tab;
    void *enc_task;
    void *input_fmt;
    RK_U32 online;
    RK_U32 rst_marker;
} Vepu511JpegCfg;

typedef struct Vepu511SclJpgTbl_t {
    /* 0x2200 reg2176 - 0x2584 reg2401*/
    Vepu511SclCfg       scl;
    /* 0x2588 - 0x258f */
    RK_U32              reserve[2];
    /* 0x2590 - 0x2c9c*/
    Vepu511SclCfgExt    scl_ext;
    /* 0x2ca0 - 0x2e1c */
    Vepu511JpegTable    jpg_tbl;
} Vepu511SclJpgTbl;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vepu511_set_osd(Vepu511OsdCfg * cfg);
MPP_RET vepu511_set_roi(Vepu511RoiCfg *roi_reg_base, MppEncROICfg * roi, RK_S32 w, RK_S32 h);
MPP_RET vepu511_set_jpeg_reg(Vepu511JpegCfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* VEPU511_COMMON_H */
