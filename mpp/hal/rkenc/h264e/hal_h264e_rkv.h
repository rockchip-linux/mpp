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

#ifndef __HAL_H264E_RK_H__
#define __HAL_H264E_RK_H__

#include "mpp_buffer.h"
#include "mpp_hal.h"
#include "hal_task.h"
/*
enc_mode
    0: N/A
    1: one frame encode by register configuration
    2: multi-frame encode start with link table
    3: multi-frame encode link table update
*/
#define RKV_H264E_SDK_TEST                  1


#define RKV_H264E_ENC_MODE                  1 //2/3
#define RKV_H264E_LINKTABLE_FRAME_NUM       1 //2

#if RKV_H264E_ENC_MODE == 2
#define RKV_H264E_LINKTABLE_EACH_NUM        RKV_H264E_LINKTABLE_FRAME_NUM
#else
#define RKV_H264E_LINKTABLE_EACH_NUM        1
#endif

#define RKV_H264E_NUM_REFS                  1
#define RKV_H264E_LONGTERM_REF_EN           0
//-------------------------------------------------------------------------------

#define RKV_H264E_LINKTABLE_MAX_SIZE        256
#define RKV_H264E_ADD_RESERVE_REGS          1
#define RKV_H264E_REMOVE_UNNECESSARY_REGS   1

#define RKV_H264E_CSP2_MASK           0x00ff  /* */
#define RKV_H264E_CSP2_NONE           0x0000  /* Invalid mode     */
#define RKV_H264E_CSP2_I420           0x0001  /* yuv 4:2:0 planar */
#define RKV_H264E_CSP2_YV12           0x0002  /* yvu 4:2:0 planar */
#define RKV_H264E_CSP2_NV12           0x0003  /* yuv 4:2:0, with one y plane and one packed u+v */
#define RKV_H264E_CSP2_I422           0x0004  /* yuv 4:2:2 planar */
#define RKV_H264E_CSP2_YV16           0x0005  /* yvu 4:2:2 planar */
#define RKV_H264E_CSP2_NV16           0x0006  /* yuv 4:2:2, with one y plane and one packed u+v */
#define RKV_H264E_CSP2_V210           0x0007  /* 10-bit yuv 4:2:2 packed in 32 */
#define RKV_H264E_CSP2_I444           0x0008  /* yuv 4:4:4 planar */
#define RKV_H264E_CSP2_YV24           0x0009  /* yvu 4:4:4 planar */
#define RKV_H264E_CSP2_BGR            0x000a  /* packed bgr 24bits   */
#define RKV_H264E_CSP2_BGRA           0x000b  /* packed bgr 32bits   */
#define RKV_H264E_CSP2_RGB            0x000c  /* packed rgb 24bits   */
#define RKV_H264E_CSP2_MAX            0x000d  /* end of list */
#define RKV_H264E_CSP2_VFLIP          0x1000  /* the csp is vertically flipped */
#define RKV_H264E_CSP2_HIGH_DEPTH     0x2000  /* the csp has a depth of 16 bits per pixel component */

#define RKV_H264E_B_PYRAMID_NONE         0
#define RKV_H264E_B_PYRAMID_STRICT       1
#define RKV_H264E_B_PYRAMID_NORMAL       2

#define RKV_H264E_CQM_FLAT               0
#define RKV_H264E_CQM_JVT                1
#define RKV_H264E_CQM_CUSTOM             2


#define RKV_H264E_INT_ONE_FRAME_FINISH    0x00000001
#define RKV_H264E_INT_LINKTABLE_FINISH    0x00000002
#define RKV_H264E_INT_SAFE_CLEAR_FINISH   0x00000004
#define RKV_H264E_INT_ONE_SLICE_FINISH    0x00000008
#define RKV_H264E_INT_BIT_STREAM_OVERFLOW 0x00000010
#define RKV_H264E_INT_BUS_WRITE_FULL      0x00000020
#define RKV_H264E_INT_BUS_WRITE_ERROR     0x00000040
#define RKV_H264E_INT_BUS_READ_ERROR      0x00000080
#define RKV_H264E_INT_TIMEOUT_ERROR       0x00000100

#if RKV_H264E_SDK_TEST
typedef struct h264e_hal_rkv_coveragetest_cfg_t {
    RK_U32 qp;
    RK_U32 preproc;
    RK_U32 osd;
    RK_U32 mbrc;
    RK_U32 roi;
} h264e_hal_rkv_coveragetest_cfg;
#endif


typedef enum h264e_rkv_csp_t {
    H264E_RKV_CSP_BGRA8888,     // 0
    H264E_RKV_CSP_BGR888,       // 1
    H264E_RKV_CSP_BGR565,       // 2
    H264E_RKV_CSP_NONE,         // 3
    H264E_RKV_CSP_YUV422SP,     // 4
    H264E_RKV_CSP_YUV422P,      // 5
    H264E_RKV_CSP_YUV420SP,     // 6
    H264E_RKV_CSP_YUV420P,      // 7
    H264E_RKV_CSP_YUYV422,      // 8
    H264E_RKV_CSP_UYVY422,      // 9
    H264E_RKV_CSP_BUTT,         // 10
} h264e_hal_rkv_csp;


typedef enum h264e_rkv_chroma_fmt_t {
    RKV_H264E_CHROMA_400 = 0,
    RKV_H264E_CHROMA_420 = 1,
    RKV_H264E_CHROMA_422 = 2,
    RKV_H264E_CHROMA_444 = 3,
} h264e_rkv_chroma_fmt;

typedef struct h264e_hal_rkv_csp_info_t {
    RK_U32 fmt;
    RK_U32 cswap; // U/V swap for YUV420SP/YUV422SP/YUYV422/UYUV422; R/B swap for BGRA88/RGB888/RGB565.
    RK_U32 aswap; // 0: BGRA, 1:ABGR.
} h264e_hal_rkv_csp_info;

typedef enum h264e_rkv_cqm4_t {
    RKV_H264E_CQM_4IY = 0,
    RKV_H264E_CQM_4PY = 1,
    RKV_H264E_CQM_4IC = 2,
    RKV_H264E_CQM_4PC = 3
} h264e_rkv_cqm4;

typedef enum h264e_rkv_cqm8_t {
    RKV_H264E_CQM_8IY = 0,
    RKV_H264E_CQM_8PY = 1,
    RKV_H264E_CQM_8IC = 2,
    RKV_H264E_CQM_8PC = 3,
} h264e_rkv_cqm8;

typedef enum h264e_hal_rkv_buf_grp_t {
    H264E_HAL_RKV_BUF_GRP_PP,
    H264E_HAL_RKV_BUF_GRP_DSP,
    H264E_HAL_RKV_BUF_GRP_MEI,
    H264E_HAL_RKV_BUF_GRP_CMV,
    H264E_HAL_RKV_BUF_GRP_ROI,
    H264E_HAL_RKV_BUF_GRP_REC,
    H264E_HAL_RKV_BUF_GRP_BUTT
} h264e_hal_rkv_buf_grp;

typedef struct h264e_hal_rkv_roi_cfg_t {
    RK_U8 qp_y          : 6;
    RK_U8 set_qp_y_en   : 1;
    RK_U8 forbit_inter  : 1;
} h264e_hal_rkv_roi_cfg;


typedef struct h264e_hal_rkv_buffers_t {
    MppBufferGroup hw_buf_grp[H264E_HAL_RKV_BUF_GRP_BUTT];

    //MppBuffer hw_input_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
    //MppBuffer hw_output_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBuffer hw_pp_buf[2];
    MppBuffer hw_dsp_buf[2]; //down scale picture
    MppBuffer hw_mei_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
    //MppBuffer hw_cmv_buf[2];
    MppBuffer hw_roi_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBuffer hw_rec_buf[RKV_H264E_NUM_REFS + 1]; //extra 1 frame for current recon
    MppBuffer hw_osd_buf[RKV_H264E_LINKTABLE_FRAME_NUM];
} h264e_hal_rkv_buffers;

typedef struct h264e_hal_rkv_nal_t {
    RK_S32 i_ref_idc;  /* nal_priority_e */
    RK_S32 i_type;     /* nal_unit_type_e */
    RK_S32 b_long_startcode;
    RK_S32 i_first_mb; /* If this NAL is a slice, the index of the first MB in the slice. */
    RK_S32 i_last_mb;  /* If this NAL is a slice, the index of the last MB in the slice. */

    /* Size of payload (including any padding) in bytes. */
    RK_S32     i_payload;
    /* If param->b_annexb is set, Annex-B bytestream with startcode.
    * Otherwise, startcode is replaced with a 4-byte size.
    * This size is the size used in mp4/similar muxing; it is equal to i_payload-4 */
    RK_U8 *p_payload;

    /* Size of padding in bytes. */
    RK_S32 i_padding;
    RK_S32 sh_head_len;
} h264e_hal_rkv_nal;

typedef struct h264e_hal_rkv_dump_files_t {
    FILE *fp_mpp_syntax_in;
    FILE *fp_mpp_reg_in;
    FILE *fp_mpp_reg_out;
    FILE *fp_mpp_strm_out;
    FILE *fp_mpp_feedback;
    FILE *fp_mpp_extra_ino_cfg;
} h264e_hal_rkv_dump_files;

typedef struct h264e_hal_rkv_stream_t {
    RK_U8 *p_start;
    RK_U8 *p;
    //RK_U8 *p_end;

    RK_U32 cur_bits;
    RK_S32 i_left;    /* i_count number of available bits */

    //add buf pointer
    RK_U8 *buf;
    RK_U32 count_bit;
} h264e_hal_rkv_stream;


typedef struct h264e_hal_rkv_extra_info_t {
    RK_S32               nal_num;
    h264e_hal_rkv_nal    nal[RKV_H264E_RKV_NAL_IDX_BUTT];
    RK_U8                *nal_buf;
    h264e_hal_rkv_stream stream;
    h264e_hal_sps        sps;
    h264e_hal_pps        pps;
} h264e_hal_rkv_extra_info;
#define RKV_H264E_REF_MAX 16
typedef RK_U8 pixel;
struct h264e_hal_rkv_weight_t;
typedef void (* weight_fn_t)( pixel *, intptr_t, pixel *, intptr_t, const struct h264e_hal_rkv_weight_t *, int );
typedef struct h264e_hal_rkv_weight_t {
    /* aligning the first member is a gcc hack to force the struct to be
     * 16 byte aligned, as well as force sizeof(struct) to be a multiple of 16 */
    RK_S16 cachea[8];
    RK_S16 cacheb[8];
    RK_S32 i_denom;
    RK_S32 i_scale;
    RK_S32 i_offset;
    weight_fn_t *weightfn;
} h264e_hal_rkv_weight;
typedef struct h264e_hal_rkv_hrd_t {
    double cpb_initial_arrival_time;
    double cpb_final_arrival_time;
    double cpb_removal_time;

    double dpb_output_time;
} h264e_hal_rkv_hrd;

struct h264e_hal_rkv_frame_t;
typedef struct h264e_hal_rkv_frame_t {
    MppBuffer   hw_buf;
    RK_S32      hw_buf_used;
    RK_S32      i_frame_cnt;     /* Presentation frame number */
    RK_S32      i_frame_num; /* 7.4.3 frame_num */
    RK_S32      long_term_flag;
    RK_S32      reorder_longterm_flag;
    RK_S32      i_poc;
    RK_S32      i_delta_poc[2];
    RK_S32      i_frame_type;
    RK_S64      i_pts;
    RK_S64      i_dts;
    RK_S32      b_kept_as_ref;
    RK_S32      b_keyframe;
    RK_S32      b_corrupt;
    RK_S32      i_reference_count;
} h264e_hal_rkv_frame;


typedef struct h264e_hal_rkv_dpb_ctx_t {
    //h264e_hal_rkv_frame *fenc;
    h264e_hal_rkv_frame *fdec;
    h264e_hal_rkv_frame *fref[2][RKV_H264E_REF_MAX + 1];
    h264e_hal_rkv_frame *fref_nearest[2]; //Used for RC
    struct {
        /* Unused frames: 0 = fenc, 1 = fdec */
        h264e_hal_rkv_frame **unused;

        /* frames used for reference + sentinels */
        h264e_hal_rkv_frame *reference[RKV_H264E_REF_MAX + 1]; //TODO: remove later


        RK_S32 i_last_keyframe;       /* Frame number of the last keyframe */
        RK_S32 i_last_idr;            /* Frame number of the last IDR (not RP)*/
        //RK_S64 i_largest_pts;
        //RK_S64 i_second_largest_pts;
    } frames;

    h264e_hal_rkv_frame    frame_buf[RKV_H264E_REF_MAX + 1];

    RK_S32      i_ref[2];
    RK_U32      i_nal_type;
    RK_U32      i_nal_ref_idc;

    RK_S32      i_frame_cnt;     /* Presentation frame number */
    RK_S32      i_frame_num; /* 7.4.3 frame_num */

    //move from slice header below
    RK_S32      i_slice_type;
    RK_S32      i_idr_pic_id;   /* -1 if nal_type != 5 */
    RK_S32      i_tmp_idr_pic_id;
    //RK_S32 i_poc;
    //RK_S32 i_delta_poc[2];
    //RK_S32 i_redundant_pic_cnt;
    RK_S32      i_max_ref0;
    RK_S32      i_max_ref1;
    RK_S32      b_ref_pic_list_reordering[2];
    struct {
        RK_S32 idc;
        RK_S32 arg;
    } ref_pic_list_order[2][RKV_H264E_REF_MAX];

    RK_S32 i_mmco_remove_from_end;
    RK_S32 i_mmco_command_count;
    struct { /* struct for future expansion */
        RK_S32 i_difference_of_pic_nums;
        RK_S32 i_poc;
        RK_S32 memory_management_control_operation;
    } mmco[RKV_H264E_REF_MAX];

    RK_S32 i_long_term_reference_flag;
} h264e_hal_rkv_dpb_ctx;

typedef struct h264e_hal_rkv_dbg_info_t {
    struct {
        RK_U32    lkt_num : 8;
        RK_U32    rkvenc_cmd : 2; //MAY
        RK_U32    reserve : 6;
        RK_U32    enc_cke : 1;
        RK_U32    Reserve : 15;
    } swreg02; //ENC_STRT

    struct {
        RK_U32    lkt_addr : 32;
    } swreg04;  //LKT_ADDR
    struct {
        RK_U32    ofe_fnsh : 1;
        RK_U32    lkt_fnsh : 1;
        RK_U32    clr_fnsh : 1;
        RK_U32    ose_fnsh : 1;
        RK_U32    bs_ovflr : 1;
        RK_U32    brsp_ful : 1;
        RK_U32    brsp_err : 1;
        RK_U32    rrsp_err : 1;
        RK_U32    tmt_err :  1;
        RK_U32    reserve : 23;
    } swreg05;  //INT_EN

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

    struct {
        RK_U32    ppln_enc_lmt : 4;
        RK_U32    reserve : 4;
        RK_U32    rfp_load_thrd : 8;
        RK_U32    reserve1 : 16;
    } swreg11; //ENC_WDG

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

    struct {
        RK_U32    axi_brsp_cke : 7;
        RK_U32    cime_dspw_orsd : 1;
        RK_U32    reserve : 24;
    } swreg13; // DTRNS_CFG

    struct {
        RK_U32    src_aswap : 1;
        RK_U32    src_cswap : 1;
        RK_U32    src_cfmt : 4;
        RK_U32    src_clip_dis : 1;
        RK_U32    reserve : 25;
    } swreg14; //SRC_FMT

    struct {
        RK_U32    wght_b2y : 9;
        RK_U32    wght_g2y : 9;
        RK_U32    wght_r2y : 9;
        RK_U32    reserve : 5;
    } swreg15; //SRC_UDFY

    struct {
        RK_U32    wght_b2u : 9;
        RK_U32    wght_g2u : 9;
        RK_U32    wght_r2u : 9;
        RK_U32    reserve : 5;
    } swreg16; //SRC_UDFU

    struct {
        RK_U32    wght_b2v : 9;
        RK_U32    wght_g2v : 9;
        RK_U32    wght_r2v : 9;
        RK_U32    reserve : 5;
    } swreg17; //SRC_UDFV


    struct {
        RK_U32    ofst_rgb2v : 8;
        RK_U32    ofst_rgb2u : 8;
        RK_U32    ofst_rgb2y : 5;
        RK_U32    reserve : 11;
    } swreg18; //SRC_UDFO

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

    struct {
        RK_U32    tfltr_thld_y : 15;
        RK_U32    reserve : 1;
        RK_U32    tfltr_thld_c : 15;
        RK_U32    reserve1 : 1;
    } swreg20; // SRC_TTHLD

    RK_U32        swreg21_scr_stbl[5];    //4.22.   H3D_TBL0~39
    RK_U32        swreg22_h3d_tbl[40];    //4.22.   H3D_TBL0~39

    struct {
        RK_U32    src_ystrid : 14;
        RK_U32    reserve : 2;
        RK_U32    src_cstrid : 14;
        RK_U32    reserve1 : 2;
    } swreg23; //SRC_STRID

    struct {
        RK_U32          adr_srcy;           //swreg24
        RK_U32          adr_srcu;
        RK_U32          adr_srcv;
        RK_U32          fltw_addr;
        RK_U32          fltr_addr;
        RK_U32          ctuc_addr;
        RK_U32          rfpw_addr;
        RK_U32          rfpr_addr;
        RK_U32          cmvw_addr;
        RK_U32          cmvr_addr;
        RK_U32          dspw_addr;
        RK_U32          dspr_addr;
        RK_U32          meiw_addr;
        RK_U32          bsbt_addr;
        RK_U32          bsbb_addr;
        RK_U32          bsbr_addr;
        RK_U32          bsbw_addr;
    } addr_cfg;

    struct {
        RK_U32    sli_cut : 1;
        RK_U32    sli_cut_mode : 1;
        RK_U32    sli_cut_bmod : 1;
        RK_U32    sli_max_num : 10;
        RK_U32    sli_out_mode : 1;
        RK_U32    reserve : 2;
        RK_U32    sli_cut_cnum : 16;
    } swreg41; //    SLI_SPL

    struct {
        RK_U32    sli_cut_byte : 18;
        RK_U32    reserve : 14;
    } swreg42; // SLI_SPL_BYTE

    struct {
        RK_U32    cime_srch_h : 4;
        RK_U32    cime_srch_v : 4;
        RK_U32    rime_srch_h : 3;
        RK_U32    rime_srch_v : 3;
        RK_U32    reserved : 2;
        RK_U32    dlt_frm_num : 16;
    } swreg43; //ME_RNGE

    struct {
        RK_U32    pmv_mdst_h          : 8;
        RK_U32    pmv_mdst_v          : 8;
        RK_U32    mv_limit            : 2;
        RK_U32    mv_num              : 2;
        RK_U32    reserve             : 12;
    } swreg44; // ME_CNST

    struct {
        RK_U32    cime_rama_max : 11;
        RK_U32    cime_rama_h : 5;
        RK_U32    cach_l2_tag : 2;
        RK_U32    cach_l1_dtmr : 5;
        RK_U32    reserve : 9;
    } swreg45; //ME_RAM

    struct {
        RK_U32    rc_en : 1;
        RK_U32    rc_mode : 1;
        RK_U32    aqmode_en : 1;
        RK_U32    aq_strg : 10;
        RK_U32    Reserved : 3;
        RK_U32    rc_ctu_num : 16;
    } swreg46; //RC_CFG

    struct {
        RK_U32    bits_error0 : 16;
        RK_U32    bits_error1 : 16;
    } swreg47; //RC_ERP0 //TODO: merge with reg 48~51, in arrays bit_error[5]

    struct {
        RK_U32    bits_error2 : 16;
        RK_U32    bits_error3 : 16;
    } swreg48; //RC_ERP1

    struct {
        RK_U32    bits_error4 : 16;
        RK_U32    bits_error5 : 16;
    } swreg49; //RC_ERP2

    struct {
        RK_U32    bits_error6 : 16;
        RK_U32    bits_error7 : 16;
    } swreg50; //RC_ERP3

    struct {
        RK_U32    bits_error8 : 16;
        RK_U32    reserve : 16;
    } swreg51; //RC_ERP4

    struct {
        RK_U32    qp_adjuest0 : 5;
        RK_U32    qp_adjuest1 : 5;
        RK_U32    qp_adjuest2 : 5;
        RK_U32    qp_adjuest3 : 5;
        RK_U32    qp_adjuest4 : 5;
        RK_U32    qp_adjuest5 : 5;
        RK_U32    reserve : 2;
    } swreg52; //    RC_ADJ0

    struct {
        RK_U32    qp_adjuest6 : 5;
        RK_U32    qp_adjuest7 : 5;
        RK_U32    qp_adjuest8 : 5;
        RK_U32    reserve : 17;
    } swreg53; //    RC_ADJ1

    struct {
        RK_U32    rc_qp_mod : 2;
        RK_U32    rc_fact0 : 6;
        RK_U32    rc_fact1 : 6;
        RK_U32    Reserved : 2;
        RK_U32    rc_qp_range : 4;
        RK_U32    rc_max_qp : 6;
        RK_U32    rc_min_qp : 6;
    } swreg54; //RC_QP

    struct {
        RK_U32    ctu_ebits : 20;
        RK_U32    reserve : 12;
    } swreg55; //RC_TGT

    struct {
        RK_U32    rect_size : 1;
        RK_U32    inter_4x4 : 1;
        RK_U32    arb_sel : 1;
        RK_U32    vlc_lmt : 1;
        RK_U32    reserve : 1;
        RK_U32    rdo_mark : 8;
        RK_U32    reserve1            : 19;
    } swreg56; //RDO_CFG

    struct {
        RK_U32    nal_ref_idc : 2;
        RK_U32    nal_unit_type : 5;
        RK_U32    reserve : 25;
    } swreg57; //     SYNT_NAL

    struct {
        RK_U32    max_fnum : 4;
        RK_U32    drct_8x8 : 1;
        RK_U32    mpoc_lm4 : 4;
        RK_U32    reserve : 23;
    } swreg58; //     SYNT_SPS

    struct {
        RK_U32    etpy_mode : 1;
        RK_U32    trns_8x8 : 1;
        RK_U32    csip_flg : 1;
        RK_U32    num_ref0_idx : 2;
        RK_U32    num_ref1_idx : 2;
        RK_U32    pic_init_qp : 6;
        int             cb_ofst : 5;
        int             cr_ofst : 5;
        RK_U32    wght_pred : 1;
        RK_U32    dbf_cp_flg : 1;
        RK_U32    reserve : 7;
    } swreg59; //  SYNT_PPS

    struct {
        RK_U32    sli_type : 2;
        RK_U32    pps_id : 8;
        RK_U32    drct_smvp : 1;
        RK_U32    num_ref_ovrd : 1;
        RK_U32    cbc_init_idc : 2;
        RK_U32    reserve : 2;
        RK_U32    frm_num : 16;
    } swreg60; //     SYNT_SLI0

    struct {
        RK_U32    idr_pid : 16;
        RK_U32    poc_lsb : 16;
    } swreg61; //     SYNT_SLI1

    struct {
        RK_U32    rodr_pic_idx : 2;
        RK_U32    ref_list0_rodr : 1;
        RK_S32    sli_beta_ofst : 4;
        RK_S32    sli_alph_ofst : 4;
        RK_U32    dis_dblk_idc : 2;
        RK_U32    reserve : 3;
        RK_U32    rodr_pic_num : 16;
    } swreg62; // SYNT_SLI2_RODR

    struct {
        RK_U32    nopp_flg : 1;
        RK_U32    ltrf_flg : 1;
        RK_U32    arpm_flg : 1;
        RK_U32    mmco4_pre : 1;
        RK_U32    mmco_0 : 3;
        RK_U32    dopn_m1_0 : 16;
        RK_U32    reserve : 9;
    } swreg63; //  SYNT_REF_MARK0

    struct {
        RK_U32    mmco_1 : 3;
        RK_U32    dopn_m1_1 : 16;
        RK_U32    reserve : 13;
    } swreg64; // SYNT_REF_MARK1

    struct {
        RK_U32    osd_en : 8;
        RK_U32    osd_inv : 8;
        RK_U32    osd_clk_sel : 1;
        RK_U32    osd_plt_type : 1;
        RK_U32    reserve : 14;
    } swreg65; //OSD_CFG

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

    h264e_osd_pos       swreg67_osd_pos[8];

    RK_U32        swreg68_indx_addr_i[8];      //4.68.  OSD_ADDR0-7

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
    } swreg71; //  ST_SSE_QP

    struct {
        RK_U32    slice_scnum : 12;
        RK_U32    slice_slnum : 12;
        RK_U32    reserve : 8;
    } swreg72; //ST_SAO

    RK_U32    swreg73_osd_indx_tab_i[256];        //4.73.   OSD_PLT0~255

    struct {
        RK_U32    st_enc : 2;
        RK_U32    axiw_cln : 2;
        RK_U32    axir_cln : 2;
        RK_U32    reserve : 26;
    } swreg74; //     ST_ENC

    struct {
        RK_U32    fnum_enc : 8;
        RK_U32    fnum_cfg : 8;
        RK_U32    fnum_int : 8;
        RK_U32    reserve : 8;
    } swreg75; //     ST_LKT

    struct {
        RK_U32    node_addr : 32;
    } swreg76; //ST_NOD

    struct {
        RK_U32    Bsbw_ovfl : 1;
        RK_U32    reserve : 2;
        RK_U32    bsbw_addr : 29;
    } swreg77; //ST_BSB

#if 0
    struct {
        RK_U32    axib_idl : 7;
        RK_U32    axib_ful : 7;
        RK_U32    axib_err : 7;
        RK_U32    axir_err : 6;
        RK_U32    reserve : 5;
    } swreg78; //ST_DTRNS

    struct {
        RK_U32    slice_num : 6;
        RK_U32    reserve : 26;
    } swreg79; //ST_SNUM

    struct {
        RK_U32    slice_len : 23;
        RK_U32    reserve : 9;
    } swreg80; //ST_SLEN

    struct {
        RK_U32    axip0_work : 1;
        RK_U32    axip0_clr : 1;
        RK_U32    axip0_frm : 1;
        RK_U32    reserve : 29;
    } swreg81; // AXIP0_CMD

    struct {
        RK_U32    axip0_ltcy_id : 4;
        RK_U32    axip0_ltcy_thr : 12;
        RK_U32    reserve : 16;
    } swreg82; // AXIP0_LTCY

    struct {
        RK_U32    axip0_cnt_type : 1;
        RK_U32    axip0_cnt_ddr : 2;
        RK_U32    axip0_cnt_rid : 5;
        RK_U32    axip0_cnt_wid : 5;
        RK_U32    reserve : 19;
    } swreg83; // AXIP0_CNT

    struct {
        RK_U32    axip1_work : 1;
        RK_U32    axip1_clr : 1;
        RK_U32    reserve : 30;
    } swreg84; // AXIP1_CMD

    struct {
        RK_U32    axip1_ltcy_id : 4;
        RK_U32    axip1_ltcy_thr : 12;
        RK_U32    reserve : 16;
    } swreg85; // AXIP1_LTCY

    struct {
        RK_U32    axip1_cnt_type : 1;
        RK_U32    axip1_cnt_ddr : 2;
        RK_U32    axip1_cnt_rid : 5;
        RK_U32    axip1_cnt_wid : 5;
        RK_U32    reserve : 19;
    } swreg86; // AXIP1_CNT

    struct {
        RK_U32    axip0_cnt_type : 16;
        RK_U32    reserve : 16;
    } swreg87; // ST_AXIP0_MAXL


    struct {
        RK_U32    axip0_num_ltcy : 32;
    } swreg88; // ST_AXIP0_NUML

    struct {
        RK_U32    axip0_sum_ltcy : 32;
    } swreg89; //     ST_AXIP0_SUML

    struct {
        RK_U32    axip0_byte_rd : 32;
    } swreg90; //     ST_AXIP0_RDB

    struct {
        RK_U32    axip0_byte_wr : 32;
    } swreg91; // ST_AXIP0_WRB

    struct {
        RK_U32    axip0_wrk_cyc : 32;
    } swreg92; //ST_AXIP0_CYCL

    struct {
        RK_U32    axip1_cnt_type : 16;
        RK_U32    reserve : 16;
    } swreg93; //     ST_AXIP1_MAXL

    struct {
        RK_U32    axip1_num_ltcy : 32;
    } swreg94; // ST_AXIP1_NUML

    struct {
        RK_U32    axip1_sum_ltcy : 32;
    } swreg95; //     ST_AXIP1_SUML

    struct {
        RK_U32    axip1_byte_rd : 32;
    } swreg96; //  ST_AXIP1_RDB

    struct {
        RK_U32    axip1_byte_wr : 32;
    } swreg97; // ST_AXIP1_WRB

    struct {
        RK_U32    axip1_wrk_cyc : 32;
    } swreg98; // ST_AXIP1_CYCL
#endif //#if 0
} h264e_hal_rkv_dbg_info;


/* cmodel version r2893 */

typedef struct  h264e_osd_cfg_t {
    RK_U32    lt_pos_x : 8;
    RK_U32    lt_pos_y : 8;
    RK_U32    rd_pos_x : 8;
    RK_U32    rd_pos_y : 8;
} h264e_osd_cfg; //OSD_POS0-7

typedef struct h264e_rkv_reg_set_t {

    struct {
        RK_U32    rkvenc_ver : 32;        //default : 0x0000_0001
    } swreg01;  //VERSION

    struct {
        RK_U32    lkt_num : 8;
        RK_U32    rkvenc_cmd : 2;
        RK_U32    reserve : 6;
        RK_U32    enc_cke : 1;
        RK_U32    Reserve : 15;
    } swreg02; //ENC_STRT

    struct {
        RK_U32    safe_clr : 1;
        RK_U32    reserve : 31;
    } swreg03; //ENC_CLR

    struct {
        RK_U32    lkt_addr : 32;
    } swreg04;  //LKT_ADDR

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

    struct {
        RK_U32    reserve : 32;
    } swreg06;  //4.5.  INT_MSK

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

    struct {
        RK_U32    reserve : 32;
    } swreg08;  //4.7.  INT_STUS

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_08_09[4];
#endif

    struct {
        RK_U32    pic_wd8_m1 : 9;
        RK_U32    reserve0 : 1;
        RK_U32    pic_wfill : 6;
        RK_U32    pic_hd8_m1 : 9;
        RK_U32    reserve1 : 1;
        RK_U32    pic_hfill : 6;
    } swreg09; // ENC_RSL


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

    struct {
        RK_U32    ppln_enc_lmt : 4;
        RK_U32    reserve : 4;
        RK_U32    rfp_load_thrd : 8;
        RK_U32    reserve1 : 16;
    } swreg11; //ENC_WDG

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

    struct {
        RK_U32    axi_brsp_cke : 7;
        RK_U32    cime_dspw_orsd : 1;
        RK_U32    reserve : 24;
    } swreg13; // DTRNS_CFG

    struct {
        RK_U32    src_aswap : 1;
        RK_U32    src_cswap : 1;
        RK_U32    src_cfmt : 4;
        RK_U32    src_clip_dis : 1;
        RK_U32    reserve : 25;
    } swreg14; //SRC_FMT

    struct {
        RK_U32    wght_b2y : 9;
        RK_U32    wght_g2y : 9;
        RK_U32    wght_r2y : 9;
        RK_U32    reserve : 5;
    } swreg15; //SRC_UDFY

    struct {
        RK_U32    wght_b2u : 9;
        RK_U32    wght_g2u : 9;
        RK_U32    wght_r2u : 9;
        RK_U32    reserve : 5;
    } swreg16; //SRC_UDFU

    struct {
        RK_U32    wght_b2v : 9;
        RK_U32    wght_g2v : 9;
        RK_U32    wght_r2v : 9;
        RK_U32    reserve : 5;
    } swreg17; //SRC_UDFV

    struct {
        RK_U32    ofst_rgb2v : 8;
        RK_U32    ofst_rgb2u : 8;
        RK_U32    ofst_rgb2y : 5;
        RK_U32    reserve : 11;
    } swreg18; //SRC_UDFO

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

    struct {
        RK_U32    tfltr_thld_y : 15;
        RK_U32    reserve : 1;
        RK_U32    tfltr_thld_c : 15;
        RK_U32    reserve1 : 1;
    } swreg20; // SRC_TTHLD

    RK_U32        swreg21_scr_stbl[5];    //4.22.   H3D_TBL0~39

    RK_U32        swreg22_h3d_tbl[40];    //4.22.   H3D_TBL0~39

    struct {
        RK_U32    src_ystrid : 14;
        RK_U32    reserve : 2;
        RK_U32    src_cstrid : 14;
        RK_U32    reserve1 : 2;
    } swreg23; //SRC_STRID

    RK_U32  swreg24_adr_srcy;           //swreg24
    RK_U32  swreg25_adr_srcu;           //swreg25
    RK_U32  swreg26_adr_srcv;           //swreg26
    RK_U32  swreg27_fltw_addr;          //swreg27
    RK_U32  swreg28_fltr_addr;          //swreg28
    RK_U32  swreg29_ctuc_addr;          //swreg29
    RK_U32  swreg30_rfpw_addr;          //swreg30
    RK_U32  swreg31_rfpr_addr;          //swreg31
    RK_U32  swreg32_cmvw_addr;          //swreg32
    RK_U32  swreg33_cmvr_addr;          //swreg33
    RK_U32  swreg34_dspw_addr;          //swreg34
    RK_U32  swreg35_dspr_addr;          //swreg35
    RK_U32  swreg36_meiw_addr;          //swreg36
    RK_U32  swreg37_bsbt_addr;          //swreg37
    RK_U32  swreg38_bsbb_addr;          //swreg38
    RK_U32  swreg39_bsbr_addr;          //swreg39
    RK_U32  swreg40_bsbw_addr;          //swreg40

    struct {
        RK_U32    sli_cut : 1;
        RK_U32    sli_cut_mode : 1;
        RK_U32    sli_cut_bmod : 1;
        RK_U32    sli_max_num : 10;
        RK_U32    sli_out_mode : 1;
        RK_U32    reserve : 2;
        RK_U32    sli_cut_cnum : 16;
    } swreg41; //     SLI_SPL

    struct {
        RK_U32    sli_cut_byte : 18;
        RK_U32    reserve : 14;
    } swreg42; // SLI_SPL_BYTE

    struct {
        RK_U32    cime_srch_h : 4;
        RK_U32    cime_srch_v : 4;
        RK_U32    rime_srch_h : 3;
        RK_U32    rime_srch_v : 3;
        RK_U32    reserved : 2;
        RK_U32    dlt_frm_num : 16;
    } swreg43; //ME_RNGE


    struct {
        RK_U32    pmv_mdst_h          : 8;
        RK_U32    pmv_mdst_v          : 8;
        RK_U32    mv_limit            : 2;
        RK_U32    mv_num              : 2;
        RK_U32    reserve             : 12;
    } swreg44; // ME_CNST

    struct {
        RK_U32    cime_rama_max : 11;
        RK_U32    cime_rama_h : 5;
        RK_U32    cach_l2_tag : 2;
        RK_U32    cach_l1_dtmr : 5;
        RK_U32    reserve : 9;
    } swreg45; //ME_RAM

    struct {
        RK_U32    rc_en : 1;
        RK_U32    rc_mode : 1;
        RK_U32    aqmode_en : 1;
        RK_U32    aq_strg : 10;
        RK_U32    Reserved : 3;
        RK_U32    rc_ctu_num : 16;
    } swreg46; //RC_CFG

    struct {
        RK_U32    bits_error0 : 16;
        RK_U32    bits_error1 : 16;
    } swreg47; //RC_ERP0

    struct {
        RK_U32    bits_error2 : 16;
        RK_U32    bits_error3 : 16;
    } swreg48; //RC_ERP1

    struct {
        RK_U32    bits_error4 : 16;
        RK_U32    bits_error5 : 16;
    } swreg49; //RC_ERP2

    struct {
        RK_U32    bits_error6 : 16;
        RK_U32    bits_error7 : 16;
    } swreg50; //RC_ERP3

    struct {
        RK_U32    bits_error8 : 16;
        RK_U32    reserve : 16;
    } swreg51; //RC_ERP4

    struct {
        RK_U32    qp_adjuest0 : 5;
        RK_U32    qp_adjuest1 : 5;
        RK_U32    qp_adjuest2 : 5;
        RK_U32    qp_adjuest3 : 5;
        RK_U32    qp_adjuest4 : 5;
        RK_U32    qp_adjuest5 : 5;
        RK_U32    reserve : 2;
    } swreg52; //     RC_ADJ0

    struct {
        RK_U32    qp_adjuest6 : 5;
        RK_U32    qp_adjuest7 : 5;
        RK_U32    qp_adjuest8 : 5;
        RK_U32    reserve : 17;
    } swreg53; //     RC_ADJ1

    struct {
        RK_U32    rc_qp_mod : 2;
        RK_U32    rc_fact0 : 6;
        RK_U32    rc_fact1 : 6;
        RK_U32    Reserved : 2;
        RK_U32    rc_qp_range : 4;
        RK_U32    rc_max_qp : 6;
        RK_U32    rc_min_qp : 6;
    } swreg54; //RC_QP

    struct {
        RK_U32    ctu_ebits : 20;
        RK_U32    reserve : 12;
    } swreg55; //RC_TGT


    struct {
        RK_U32    rect_size : 1;
        RK_U32    inter_4x4 : 1;
        RK_U32    arb_sel : 1;
        RK_U32    vlc_lmt : 1;
        RK_U32    reserve : 1;
        RK_U32    rdo_mark : 8;
        RK_U32    reserve1            : 19;
    } swreg56; //RDO_CFG

    struct {
        RK_U32    nal_ref_idc : 2;
        RK_U32    nal_unit_type : 5;
        RK_U32    reserve : 25;
    } swreg57; //     SYNT_NAL

    struct {
        RK_U32    max_fnum : 4;
        RK_U32    drct_8x8 : 1;
        RK_U32    mpoc_lm4 : 4;
        RK_U32    reserve : 23;
    } swreg58; //     SYNT_SPS

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

    struct {
        RK_U32    sli_type : 2;
        RK_U32    pps_id : 8;
        RK_U32    drct_smvp : 1;
        RK_U32    num_ref_ovrd : 1;
        RK_U32    cbc_init_idc : 2;
        RK_U32    reserve : 2;
        RK_U32    frm_num : 16;
    } swreg60; //     SYNT_SLI0

    struct {
        RK_U32    idr_pid : 16;
        RK_U32    poc_lsb : 16;
    } swreg61; //     SYNT_SLI1

    struct {
        RK_U32    rodr_pic_idx : 2;
        RK_U32    ref_list0_rodr : 1;
        RK_S32    sli_beta_ofst : 4;
        RK_S32    sli_alph_ofst : 4;
        RK_U32    dis_dblk_idc : 2;
        RK_U32    reserve : 3;
        RK_U32    rodr_pic_num : 16;
    } swreg62; // SYNT_SLI2_RODR

    struct {
        RK_U32    nopp_flg : 1;
        RK_U32    ltrf_flg : 1;
        RK_U32    arpm_flg : 1;
        RK_U32    mmco4_pre : 1;
        RK_U32    mmco_0 : 3;
        RK_U32    dopn_m1_0 : 16;
        RK_U32    reserve : 9;
    } swreg63; //  SYNT_REF_MARK0

    struct {
        RK_U32    mmco_1 : 3;
        RK_U32    dopn_m1_1 : 16;
        RK_U32    reserve : 13;
    } swreg64; // SYNT_REF_MARK1

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_64_65[1];
#endif
    struct {
        RK_U32    osd_en : 8;
        RK_U32    osd_inv : 8;
        RK_U32    osd_clk_sel : 1;
        RK_U32    osd_plt_type : 1;
        RK_U32    reserve : 14;
    } swreg65; //OSD_CFG

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

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_66_67[2];
#endif

    h264e_osd_cfg swreg67_osd_pos[8];

    RK_U32    swreg68_indx_addr_i[8];      //4.68.  OSD_ADDR0-7

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

#if !RKV_H264E_REMOVE_UNNECESSARY_REGS

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

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_79_80[113];
#endif

    RK_U32    swreg80_osd_indx_tab_i[256];        //4.73.   OSD_PLT0~255

    struct {
        RK_U32    axip0_work : 1;
        RK_U32    axip0_clr : 1;
        RK_U32    axip0_frm : 1;
        RK_U32    reserve : 29;
    } swreg81; // AXIP0_CMD

    struct {
        RK_U32    axip0_ltcy_id : 4;
        RK_U32    axip0_ltcy_thr : 12;
        RK_U32    reserve : 16;
    } swreg82; // AXIP0_LTCY

    struct {
        RK_U32    axip0_cnt_type : 1;
        RK_U32    axip0_cnt_ddr : 2;
        RK_U32    axip0_cnt_rid : 5;
        RK_U32    axip0_cnt_wid : 5;
        RK_U32    reserve : 19;
    } swreg83; // AXIP0_CNT

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_83_84[1];
#endif

    struct {
        RK_U32    axip1_work : 1;
        RK_U32    axip1_clr : 1;
        RK_U32    reserve : 30;
    } swreg84; // AXIP1_CMD

    struct {
        RK_U32    axip1_ltcy_id : 4;
        RK_U32    axip1_ltcy_thr : 12;
        RK_U32    reserve : 16;
    } swreg85; // AXIP1_LTCY

    struct {
        RK_U32    axip1_cnt_type : 1;
        RK_U32    axip1_cnt_ddr : 2;
        RK_U32    axip1_cnt_rid : 5;
        RK_U32    axip1_cnt_wid : 5;
        RK_U32    reserve : 19;
    } swreg86; // AXIP1_CNT

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_86_87[1];
#endif

    struct {
        RK_U32    axip0_cnt_type : 16;
        RK_U32    reserve : 16;
    } swreg87; // ST_AXIP0_MAXL


    struct {
        RK_U32    axip0_num_ltcy : 32;
    } swreg88; // ST_AXIP0_NUML

    struct {
        RK_U32    axip0_sum_ltcy : 32;
    } swreg89; //     ST_AXIP0_SUML

    struct {
        RK_U32    axip0_byte_rd : 32;
    } swreg90; //     ST_AXIP0_RDB

    struct {
        RK_U32    axip0_byte_wr : 32;
    } swreg91; // ST_AXIP0_WRB

    struct {
        RK_U32    axip0_wrk_cyc : 32;
    } swreg92; //ST_AXIP0_CYCL

#if RKV_H264E_ADD_RESERVE_REGS
    RK_U32        reserve_92_93[2];
#endif

    struct {
        RK_U32    axip1_cnt_type : 16;
        RK_U32    reserve : 16;
    } swreg93; //     ST_AXIP1_MAXL

    struct {
        RK_U32    axip1_num_ltcy : 32;
    } swreg94; // ST_AXIP1_NUML

    struct {
        RK_U32    axip1_sum_ltcy : 32;
    } swreg95; //     ST_AXIP1_SUML

    struct {
        RK_U32    axip1_byte_rd : 32;
    } swreg96; //  ST_AXIP1_RDB

    struct {
        RK_U32    axip1_byte_wr : 32;
    } swreg97; // ST_AXIP1_WRB

    struct {
        RK_U32    axip1_wrk_cyc : 32;
    } swreg98; // ST_AXIP1_CYCL

#endif //#if !RKV_H264E_REMOVE_UNNECESSARY_REGS
} h264e_rkv_reg_set;

typedef struct h264e_rkv_ioctl_extra_info_elem_t {
    RK_U32 reg_idx;
    RK_U32 offset;
} h264e_rkv_ioctl_extra_info_elem;

typedef struct h264e_rkv_ioctl_extra_info_t {
    RK_U32                          magic;
    RK_U32                          cnt;
    h264e_rkv_ioctl_extra_info_elem elem[20];
} h264e_rkv_ioctl_extra_info;

typedef struct h264e_rkv_ioctl_reg_info_t {
    RK_U32                      reg_num;
    h264e_rkv_reg_set           regs;
    h264e_rkv_ioctl_extra_info  extra_info;
} h264e_rkv_ioctl_reg_info;

/*
enc_mode
    0: N/A
    1: one frame encode by register configuration
    2: multi-frame encode start with link table
    3: multi_frame_encode link table update
*/
typedef struct h264e_rkv_ioctl_input_t {
    RK_U32                      enc_mode;
    RK_U32                      frame_num;

    h264e_rkv_ioctl_reg_info    reg_info[RKV_H264E_LINKTABLE_MAX_SIZE];
} h264e_rkv_ioctl_input;


typedef struct h264e_rkv_ioctl_output_elem_t {
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
} h264e_rkv_ioctl_output_elem;

typedef struct h264e_rkv_ioctl_output_t {
    RK_U32                          frame_num;
    h264e_rkv_ioctl_output_elem     elem[RKV_H264E_LINKTABLE_MAX_SIZE];
} h264e_rkv_ioctl_output;


#define RK_H264E_NUM_REGS     ((RK_S32)(sizeof(h264e_rkv_reg_set)/4))

MPP_RET hal_h264e_rkv_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_h264e_rkv_deinit  (void *hal);
MPP_RET hal_h264e_rkv_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_rkv_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_rkv_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_rkv_reset   (void *hal);
MPP_RET hal_h264e_rkv_flush   (void *hal);
MPP_RET hal_h264e_rkv_control (void *hal, RK_S32 cmd_type, void *param);

#endif
