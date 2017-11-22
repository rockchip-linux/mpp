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

#ifndef __HAL_H264E_RKV_H__
#define __HAL_H264E_RKV_H__

#include "mpp_buffer.h"
#include "mpp_hal.h"
#include "hal_task.h"
#include "hal_h264e_com.h"

#define RKV_H264E_ENC_MODE                  1 //2/3
#define RKV_H264E_LINKTABLE_FRAME_NUM       1 //2

#if RKV_H264E_ENC_MODE == 2
#define RKV_H264E_LINKTABLE_EACH_NUM        RKV_H264E_LINKTABLE_FRAME_NUM
#else
#define RKV_H264E_LINKTABLE_EACH_NUM        1
#endif

#define RKV_H264E_LINKTABLE_MAX_SIZE        256
#define RKV_H264E_ADD_RESERVE_REGS          1

#define RKV_H264E_INT_ONE_FRAME_FINISH    0x00000001
#define RKV_H264E_INT_LINKTABLE_FINISH    0x00000002
#define RKV_H264E_INT_SAFE_CLEAR_FINISH   0x00000004
#define RKV_H264E_INT_ONE_SLICE_FINISH    0x00000008
#define RKV_H264E_INT_BIT_STREAM_OVERFLOW 0x00000010
#define RKV_H264E_INT_BUS_WRITE_FULL      0x00000020
#define RKV_H264E_INT_BUS_WRITE_ERROR     0x00000040
#define RKV_H264E_INT_BUS_READ_ERROR      0x00000080
#define RKV_H264E_INT_TIMEOUT_ERROR       0x00000100

typedef enum H264eRkvFrameType_t {
    H264E_RKV_FRAME_P = 0,
    H264E_RKV_FRAME_B = 1,
    H264E_RKV_FRAME_I = 2
} H264eRkvFrameType;

typedef enum h264e_hal_rkv_buf_grp_t {
    H264E_HAL_RKV_BUF_GRP_PP,
    H264E_HAL_RKV_BUF_GRP_DSP,
    H264E_HAL_RKV_BUF_GRP_MEI,
    H264E_HAL_RKV_BUF_GRP_CMV,
    H264E_HAL_RKV_BUF_GRP_ROI,
    H264E_HAL_RKV_BUF_GRP_REC,
    H264E_HAL_RKV_BUF_GRP_BUTT
} h264e_hal_rkv_buf_grp;

typedef struct h264e_hal_rkv_buffers_t {
    MppBufferGroup hw_buf_grp[H264E_HAL_RKV_BUF_GRP_BUTT];

    MppBuffer hw_pp_buf[2];
    MppBuffer hw_dsp_buf[2]; //down scale picture
    MppBuffer hw_mei_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBuffer hw_roi_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBuffer hw_rec_buf[H264E_NUM_REFS + 1]; //extra 1 frame for current recon
} h264e_hal_rkv_buffers;

typedef struct  H264eOsdCfg_t {
    RK_U32    lt_pos_x : 8;
    RK_U32    lt_pos_y : 8;
    RK_U32    rd_pos_x : 8;
    RK_U32    rd_pos_y : 8;
} H264eOsdCfg; //OSD_POS0-7

typedef struct H264eRkvRegSet_t {

    /* reg[000] */
    struct {
        RK_U32    rkvenc_ver : 32;        //default : 0x0000_0001
    } swreg01;  //VERSION

    /* reg[001] */
    struct {
        RK_U32    lkt_num : 8;
        RK_U32    rkvenc_cmd : 2;
        RK_U32    reserve : 6;
        RK_U32    enc_cke : 1;
        RK_U32    Reserve : 15;
    } swreg02; //ENC_STRT

    /* reg[002] */
    struct {
        RK_U32    safe_clr : 1;
        RK_U32    reserve : 31;
    } swreg03; //ENC_CLR

    /* reg[003] */
    struct {
        RK_U32    lkt_addr : 32;
    } swreg04;  //LKT_ADDR

    /* reg[004] */
    struct {
        RK_U32    ofe_fnsh : 1;
        RK_U32    lkt_fnsh : 1;
        RK_U32    clr_fnsh : 1;
        RK_U32    ose_fnsh : 1;
        RK_U32    bs_ovflr : 1;
        RK_U32    brsp_ful : 1;
        RK_U32    brsp_err : 1;
        RK_U32    rrsp_err : 1;
        RK_U32    tmt_err : 1;
        RK_U32    reserve : 23;
    } swreg05;  //INT_EN

    /* reg[005] */
    struct {
        RK_U32    reserve : 32;
    } swreg06;  //4.5.  INT_MSK

    /* reg[006] */
    struct {
        RK_U32    clr_ofe_fnsh : 1;
        RK_U32    clr_lkt_fnsh : 1;
        RK_U32    clr_clr_fnsh : 1;
        RK_U32    clr_ose_fnsh : 1;
        RK_U32    clr_bs_ovflr : 1;
        RK_U32    clr_brsp_ful : 1;
        RK_U32    clr_brsp_err : 1;
        RK_U32    clr_rrsp_err : 1;
        RK_U32    clr_tmt_err : 1;
        RK_U32    reserve : 23;
    } swreg07;  //4.6.  INT_CLR

    /* reg[007] */
    struct {
        RK_U32    reserve : 32;
    } swreg08;  //4.7.  INT_STUS

    /* reg[008 - 011] */
#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_08_09[4];
#endif

    /* reg[012] */
    struct {
        RK_U32    pic_wd8_m1 : 9;
        RK_U32    reserve0 : 1;
        RK_U32    pic_wfill : 6;
        RK_U32    pic_hd8_m1 : 9;
        RK_U32    reserve1 : 1;
        RK_U32    pic_hfill : 6;
    } swreg09; // ENC_RSL

    /* reg[013] */
    struct {
        RK_U32    enc_stnd : 1;
        RK_U32    roi_enc : 1;
        RK_U32    cur_frm_ref : 1;
        RK_U32    mei_stor : 1;
        RK_U32    bs_scp : 1;
        RK_U32    reserve : 3;
        RK_U32    pic_qp : 6;
        RK_U32    reserve1 : 16;
        RK_U32    slice_int : 1;
        RK_U32    node_int            : 1;
    } swreg10; //ENC_PIC

    /* reg[014] */
    struct {
        RK_U32    ppln_enc_lmt : 4;
        RK_U32    reserve : 4;
        RK_U32    rfp_load_thrd : 8;
        RK_U32    reserve1 : 16;
    } swreg11; //ENC_WDG

    /* reg[015] */
    struct {
        RK_U32    src_bus_ordr : 1;
        RK_U32    cmvw_bus_ordr : 1;
        RK_U32    dspw_bus_ordr : 1;
        RK_U32    rfpw_bus_ordr : 1;
        RK_U32    src_bus_edin : 4;
        RK_U32    meiw_bus_edin : 4;
        RK_U32    bsw_bus_edin : 3;
        RK_U32    lktr_bus_edin : 4;
        RK_U32    ctur_bus_edin : 4;
        RK_U32    lktw_bus_edin : 4;
        RK_U32    rfp_map_dcol : 2;
        RK_U32    rfp_map_sctr : 1;
        RK_U32    reserve : 2;
    } swreg12; //DTRNS_MAP

    /* reg[016] */
    struct {
        RK_U32    axi_brsp_cke : 7;
        RK_U32    cime_dspw_orsd : 1;
        RK_U32    reserve : 24;
    } swreg13; // DTRNS_CFG

    /* reg[017] */
    struct {
        RK_U32    src_aswap : 1;
        RK_U32    src_cswap : 1;
        RK_U32    src_cfmt : 4;
        RK_U32    src_clip_dis : 1;
        RK_U32    reserve : 25;
    } swreg14; //SRC_FMT

    /* reg[018] */
    struct {
        RK_U32    wght_b2y : 9;
        RK_U32    wght_g2y : 9;
        RK_U32    wght_r2y : 9;
        RK_U32    reserve : 5;
    } swreg15; //SRC_UDFY

    /* reg[019] */
    struct {
        RK_U32    wght_b2u : 9;
        RK_U32    wght_g2u : 9;
        RK_U32    wght_r2u : 9;
        RK_U32    reserve : 5;
    } swreg16; //SRC_UDFU

    /* reg[020] */
    struct {
        RK_U32    wght_b2v : 9;
        RK_U32    wght_g2v : 9;
        RK_U32    wght_r2v : 9;
        RK_U32    reserve : 5;
    } swreg17; //SRC_UDFV

    /* reg[021] */
    struct {
        RK_U32    ofst_rgb2v : 8;
        RK_U32    ofst_rgb2u : 8;
        RK_U32    ofst_rgb2y : 5;
        RK_U32    reserve : 11;
    } swreg18; //SRC_UDFO

    /* reg[022] */
    struct {
        RK_U32    src_tfltr : 1;
        RK_U32    src_tfltr_we : 1;
        RK_U32    src_tfltr_bw : 1;
        RK_U32    src_sfltr : 1;
        RK_U32    src_mfltr_thrd : 5;
        RK_U32    src_mfltr_y : 1;
        RK_U32    src_mfltr_c : 1;
        RK_U32    src_bfltr_strg : 1;
        RK_U32    src_bfltr : 1;
        RK_U32    src_mbflt_odr : 1;
        RK_U32    src_matf_y : 1;
        RK_U32    src_matf_c : 1;
        RK_U32    src_shp_y : 1;
        RK_U32    src_shp_c : 1;
        RK_U32    src_shp_div : 3;
        RK_U32    src_shp_thld : 5;
        RK_U32    src_mirr : 1;
        RK_U32    src_rot : 2;
        RK_U32    src_matf_itsy : 1;
        RK_U32    reserve : 2;
    } swreg19; // SRC_PROC

    /* reg[023] */
    struct {
        RK_U32    tfltr_thld_y : 15;
        RK_U32    reserve : 1;
        RK_U32    tfltr_thld_c : 15;
        RK_U32    reserve1 : 1;
    } swreg20; // SRC_TTHLD

    /* reg[024 - 028] */
    RK_U32        swreg21_scr_stbl[5];    //4.22.   H3D_TBL0~39

    /* reg[029 - 068] */
    RK_U32        swreg22_h3d_tbl[40];    //4.22.   H3D_TBL0~39

    /* reg[069] */
    struct {
        RK_U32    src_ystrid : 14;
        RK_U32    reserve : 2;
        RK_U32    src_cstrid : 14;
        RK_U32    reserve1 : 2;
    } swreg23; //SRC_STRID

    /* reg[070] */
    RK_U32  swreg24_adr_srcy;

    /* reg[071] */
    RK_U32  swreg25_adr_srcu;

    /* reg[072] */
    RK_U32  swreg26_adr_srcv;

    /* reg[073] */
    RK_U32  swreg27_fltw_addr;

    /* reg[074] */
    RK_U32  swreg28_fltr_addr;

    /* reg[075] */
    RK_U32  swreg29_ctuc_addr;

    /* reg[076] */
    RK_U32  swreg30_rfpw_addr;

    /* reg[077] */
    RK_U32  swreg31_rfpr_addr;

    /* reg[078] */
    RK_U32  swreg32_cmvw_addr;

    /* reg[079] */
    RK_U32  swreg33_cmvr_addr;

    /* reg[080] */
    RK_U32  swreg34_dspw_addr;

    /* reg[081] */
    RK_U32  swreg35_dspr_addr;

    /* reg[082] */
    RK_U32  swreg36_meiw_addr;

    /* reg[083] */
    RK_U32  swreg37_bsbt_addr;

    /* reg[084] */
    RK_U32  swreg38_bsbb_addr;

    /* reg[085] */
    RK_U32  swreg39_bsbr_addr;

    /* reg[086] */
    RK_U32  swreg40_bsbw_addr;

    /* reg[087] */
    struct {
        RK_U32    sli_cut : 1;
        RK_U32    sli_cut_mode : 1;
        RK_U32    sli_cut_bmod : 1;
        RK_U32    sli_max_num : 10;
        RK_U32    sli_out_mode : 1;
        RK_U32    reserve : 2;
        RK_U32    sli_cut_cnum : 16;
    } swreg41; //     SLI_SPL

    /* reg[088] */
    struct {
        RK_U32    sli_cut_byte : 18;
        RK_U32    reserve : 14;
    } swreg42; // SLI_SPL_BYTE

    /* reg[089] */
    struct {
        RK_U32    cime_srch_h : 4;
        RK_U32    cime_srch_v : 4;
        RK_U32    rime_srch_h : 3;
        RK_U32    rime_srch_v : 3;
        RK_U32    reserved : 2;
        RK_U32    dlt_frm_num : 16;
    } swreg43; //ME_RNGE

    /* reg[090] */
    struct {
        RK_U32    pmv_mdst_h          : 8;
        RK_U32    pmv_mdst_v          : 8;
        RK_U32    mv_limit            : 2;
        RK_U32    mv_num              : 2;
        RK_U32    reserve             : 12;
    } swreg44; // ME_CNST

    /* reg[091] */
    struct {
        RK_U32    cime_rama_max : 11;
        RK_U32    cime_rama_h : 5;
        RK_U32    cach_l2_tag : 2;
        RK_U32    cach_l1_dtmr : 5;
        RK_U32    reserve : 9;
    } swreg45; //ME_RAM

    /* reg[092] */
    struct {
        RK_U32    rc_en : 1;
        RK_U32    rc_mode : 1;
        RK_U32    aqmode_en : 1;
        RK_U32    aq_strg : 10;
        RK_U32    Reserved : 3;
        RK_U32    rc_ctu_num : 16;
    } swreg46; //RC_CFG

    /* reg[093] */
    struct {
        RK_S32    bits_error0 : 16;
        RK_S32    bits_error1 : 16;
    } swreg47; //RC_ERP0

    /* reg[094] */
    struct {
        RK_S32    bits_error2 : 16;
        RK_S32    bits_error3 : 16;
    } swreg48; //RC_ERP1

    /* reg[095] */
    struct {
        RK_S32    bits_error4 : 16;
        RK_S32    bits_error5 : 16;
    } swreg49; //RC_ERP2

    /* reg[096] */
    struct {
        RK_S32    bits_error6 : 16;
        RK_S32    bits_error7 : 16;
    } swreg50; //RC_ERP3

    /* reg[097] */
    struct {
        RK_S32    bits_error8 : 16;
        RK_S32    reserve : 16;
    } swreg51; //RC_ERP4

    /* reg[098] */
    struct {
        RK_S32    qp_adjust0 : 5;
        RK_S32    qp_adjust1 : 5;
        RK_S32    qp_adjust2 : 5;
        RK_S32    qp_adjust3 : 5;
        RK_S32    qp_adjust4 : 5;
        RK_S32    qp_adjust5 : 5;
        RK_S32    reserve : 2;
    } swreg52; //     RC_ADJ0

    /* reg[099] */
    struct {
        RK_S32    qp_adjust6 : 5;
        RK_S32    qp_adjust7 : 5;
        RK_S32    qp_adjust8 : 5;
        RK_S32    reserve : 17;
    } swreg53; //     RC_ADJ1

    /* reg[100] */
    struct {
        RK_U32    rc_qp_mod : 2;
        RK_U32    rc_fact0 : 6;
        RK_U32    rc_fact1 : 6;
        RK_U32    Reserved : 2;
        RK_U32    rc_qp_range : 4;
        RK_U32    rc_max_qp : 6;
        RK_U32    rc_min_qp : 6;
    } swreg54; //RC_QP

    /* reg[101] */
    struct {
        RK_U32    ctu_ebits : 20;
        RK_U32    reserve : 12;
    } swreg55; //RC_TGT

    /* reg[102] */
    struct {
        RK_U32    rect_size : 1;
        RK_U32    inter_4x4 : 1;
        RK_U32    arb_sel : 1;
        RK_U32    vlc_lmt : 1;
        RK_U32    reserve : 1;
        RK_U32    rdo_mark : 8;
        RK_U32    reserve1            : 19;
    } swreg56; //RDO_CFG

    /* reg[103] */
    struct {
        RK_U32    nal_ref_idc : 2;
        RK_U32    nal_unit_type : 5;
        RK_U32    reserve : 25;
    } swreg57; //     SYNT_NAL

    /* reg[104] */
    struct {
        RK_U32    max_fnum : 4;
        RK_U32    drct_8x8 : 1;
        RK_U32    mpoc_lm4 : 4;
        RK_U32    reserve : 23;
    } swreg58; //     SYNT_SPS

    /* reg[105] */
    struct {
        RK_U32    etpy_mode : 1;
        RK_U32    trns_8x8 : 1;
        RK_U32    csip_flg : 1;
        RK_U32    num_ref0_idx : 2;
        RK_U32    num_ref1_idx : 2;
        RK_U32    pic_init_qp : 6;
        RK_S32    cb_ofst : 5;
        RK_S32    cr_ofst : 5;
        RK_U32    wght_pred : 1;
        RK_U32    dbf_cp_flg : 1;
        RK_U32    reserve : 7;
    } swreg59; //  SYNT_PPS

    /* reg[106] */
    struct {
        RK_U32    sli_type : 2;
        RK_U32    pps_id : 8;
        RK_U32    drct_smvp : 1;
        RK_U32    num_ref_ovrd : 1;
        RK_U32    cbc_init_idc : 2;
        RK_U32    reserve : 2;
        RK_U32    frm_num : 16;
    } swreg60; //     SYNT_SLI0

    /* reg[107] */
    struct {
        RK_U32    idr_pid : 16;
        RK_U32    poc_lsb : 16;
    } swreg61; //     SYNT_SLI1

    /* reg[108] */
    struct {
        RK_U32    rodr_pic_idx : 2;
        RK_U32    ref_list0_rodr : 1;
        RK_S32    sli_beta_ofst : 4;
        RK_S32    sli_alph_ofst : 4;
        RK_U32    dis_dblk_idc : 2;
        RK_U32    reserve : 3;
        RK_U32    rodr_pic_num : 16;
    } swreg62; // SYNT_SLI2_RODR

    /* reg[109] */
    struct {
        RK_U32    nopp_flg : 1;
        RK_U32    ltrf_flg : 1;
        RK_U32    arpm_flg : 1;
        RK_U32    mmco4_pre : 1;
        RK_U32    mmco_0 : 3;
        RK_U32    dopn_m1_0 : 16;
        RK_U32    reserve : 9;
    } swreg63; //  SYNT_REF_MARK0

    /* reg[110] */
    struct {
        RK_U32    mmco_1 : 3;
        RK_U32    dopn_m1_1 : 16;
        RK_U32    reserve : 13;
    } swreg64; // SYNT_REF_MARK1

    /* reg[111] */
#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_64_65[1];
#endif

    /* reg[112] */
    struct {
        RK_U32    osd_en : 8;
        RK_U32    osd_inv : 8;
        RK_U32    osd_clk_sel : 1;
        RK_U32    osd_plt_type : 1;
        RK_U32    reserve : 14;
    } swreg65; //OSD_CFG

    /* reg[113] */
    struct {
        RK_U32    osd_inv_r0 : 4;
        RK_U32    osd_inv_r1 : 4;
        RK_U32    osd_inv_r2 : 4;
        RK_U32    osd_inv_r3 : 4;
        RK_U32    osd_inv_r4 : 4;
        RK_U32    osd_inv_r5 : 4;
        RK_U32    osd_inv_r6 : 4;
        RK_U32    osd_inv_r7 : 4;
    } swreg66; //OSD_INV

    /* reg[114 - 115] */
#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_66_67[2];
#endif

    /* reg[116 - 123] */
    H264eOsdCfg swreg67_osd_pos[8];

    /* reg[124 - 131] */
    RK_U32    swreg68_indx_addr_i[8];      //4.68.  OSD_ADDR0-7

    /* reg[132] */
    struct {
        RK_U32    bs_lgth : 32;
    } swreg69; //ST_BSL

    /* reg[133] */
    struct {
        RK_U32    sse_l32 : 32;
    } swreg70; // ST_SSE_L32

    /* reg[134] */
    struct {
        RK_U32    qp_sum : 22;
        RK_U32    reserve : 2;
        RK_U32    sse_h8 : 8;
    } swreg71; //     ST_SSE_QP

    /* reg[135] */
    struct {
        RK_U32    slice_scnum : 12;
        RK_U32    slice_slnum : 12;
        RK_U32    reserve : 8;
    } swreg72; //ST_SAO

    /* reg[136] */
    struct {
        RK_U32    st_enc : 2;
        RK_U32    axiw_cln : 2;
        RK_U32    axir_cln : 2;
        RK_U32    reserve : 26;
    } swreg73; //     ST_ENC

    /* reg[137] */
    struct {
        RK_U32    fnum_enc : 8;
        RK_U32    fnum_cfg : 8;
        RK_U32    fnum_int : 8;
        RK_U32    reserve : 8;
    } swreg74; //     ST_LKT

    /* reg[138] */
    struct {
        RK_U32    node_addr : 32;
    } swreg75; //ST_NOD

    /* reg[139] */
    struct {
        RK_U32    Bsbw_ovfl : 1;
        RK_U32    reserve : 2;
        RK_U32    bsbw_addr : 29;
    } swreg76; //ST_BSB
} H264eRkvRegSet;

typedef struct H264eRkvIoctlExtraInfoElem_t {
    RK_U32 reg_idx;
    RK_U32 offset;
} H264eRkvIoctlExtraInfoElem;

typedef struct H264eRkvIoctlExtraInfo_t {
    RK_U32                          magic;
    RK_U32                          cnt;
    H264eRkvIoctlExtraInfoElem      elem[20];
} H264eRkvIoctlExtraInfo;

typedef struct H264eRkvIoctlRegInfo_t {
    RK_U32                  reg_num;
    H264eRkvRegSet          regs;
    H264eRkvIoctlExtraInfo  extra_info;
} H264eRkvIoctlRegInfo;

/*
enc_mode
    0: N/A
    1: one frame encode by register configuration
    2: multi-frame encode start with link table
    3: multi_frame_encode link table update
*/
typedef struct H264eRkvIoctlInput_t {
    RK_U32                  enc_mode;
    RK_U32                  frame_num;
    H264eRkvIoctlRegInfo    reg_info[RKV_H264E_LINKTABLE_MAX_SIZE];
} H264eRkvIoctlInput;

typedef struct H264eRkvIoctlOutputElem_t {
    RK_U32 hw_status;

    struct {
        RK_U32    bs_lgth : 32;
    } swreg69; //ST_BSL

    struct {
        RK_U32    sse_l32 : 32;
    } swreg70; // ST_SSE_L32

    struct {
        RK_U32    qp_sum : 22;
        RK_U32    reserve : 2;
        RK_U32    sse_h8 : 8;
    } swreg71; //     ST_SSE_QP

    struct {
        RK_U32    slice_scnum : 12;
        RK_U32    slice_slnum : 12;
        RK_U32    reserve : 8;
    } swreg72; //ST_SAO


    struct {
        RK_U32    st_enc : 2;
        RK_U32    axiw_cln : 2;
        RK_U32    axir_cln : 2;
        RK_U32    reserve : 26;
    } swreg73; //     ST_ENC

    struct {
        RK_U32    fnum_enc : 8;
        RK_U32    fnum_cfg : 8;
        RK_U32    fnum_int : 8;
        RK_U32    reserve : 8;
    } swreg74; //     ST_LKT

    struct {
        RK_U32    node_addr : 32;
    } swreg75; //ST_NOD

    struct {
        RK_U32    Bsbw_ovfl : 1;
        RK_U32    reserve : 2;
        RK_U32    bsbw_addr : 29;
    } swreg76; //ST_BSB

    struct {
        RK_U32    axib_idl : 7;
        RK_U32    axib_ful : 7;
        RK_U32    axib_err : 7;
        RK_U32    axir_err : 6;
        RK_U32    reserve : 5;
    } swreg77; //ST_DTRNS

    struct {
        RK_U32    slice_num : 6;
        RK_U32    reserve : 26;
    } swreg78; //ST_SNUM

    struct {
        RK_U32    slice_len : 23;
        RK_U32    reserve : 9;
    } swreg79; //ST_SLEN
} H264eRkvIoctlOutputElem;

typedef struct H264eRkvIoctlOutput_t {
    RK_U32                      frame_num;
    H264eRkvIoctlOutputElem     elem[RKV_H264E_LINKTABLE_MAX_SIZE];
} H264eRkvIoctlOutput;

/* mode cfg */
typedef struct H264eRkvMbRcMcfg_t {
    RK_S32 aq_prop; //0~16, 0:only enable aq, 16:only enable checkpoint
    double aq_strength; //0~3
    RK_U32 mb_num;
    RK_U32 qp_range; //0~15
} H264eRkvMbRcMcfg;

/* quality cfg */
typedef struct H264eRkvMbRcQcfg_t {
    RK_U32 qp_min;
    RK_U32 qp_max;
} H264eRkvMbRcQcfg;

typedef struct H264eRkvMbRcQRcfg_t {
    RK_U32 qp_min_offset;
    RK_U32 qp_max_offset;
    RK_U32 qp_range;
} H264eRkvMbRcQRcfg;

MPP_RET hal_h264e_rkv_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_h264e_rkv_deinit  (void *hal);
MPP_RET hal_h264e_rkv_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_rkv_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_rkv_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_rkv_reset   (void *hal);
MPP_RET hal_h264e_rkv_flush   (void *hal);
MPP_RET hal_h264e_rkv_control (void *hal, RK_S32 cmd_type, void *param);

#endif
