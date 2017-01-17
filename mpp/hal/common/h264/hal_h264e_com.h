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

#ifndef __HAL_H264E_H__
#define __HAL_H264E_H__

#include "vpu.h"
#include "rk_mpi_cmd.h"
#include "mpp_packet.h"
#include "mpp_log.h"
#include "mpp_hal.h"
#include "mpp_rc.h"

#include "h264e_syntax.h"

extern RK_U32 h264e_hal_log_mode;

#define H264E_HAL_LOG_ERROR             0x00000001

#define H264E_HAL_LOG_SIMPLE            0x00000010

#define H264E_HAL_LOG_FLOW              0x00000100

#define H264E_HAL_LOG_DPB               0x00001000
#define H264E_HAL_LOG_HEADER            0x00002000
#define H264E_HAL_LOG_SEI               0x00004000
#define H264E_HAL_LOG_RC                0x00008000

#define H264E_HAL_LOG_DETAIL            0x00010000
#define H264E_HAL_LOG_DUMP_RC           0x00020000

#define H264E_HAL_LOG_FILE              0x00100000



#define H264E_HAL_MASK_2b               (RK_U32)0x00000003
#define H264E_HAL_MASK_3b               (RK_U32)0x00000007
#define H264E_HAL_MASK_4b               (RK_U32)0x0000000F
#define H264E_HAL_MASK_5b               (RK_U32)0x0000001F
#define H264E_HAL_MASK_6b               (RK_U32)0x0000003F
#define H264E_HAL_MASK_11b              (RK_U32)0x000007FF
#define H264E_HAL_MASK_14b              (RK_U32)0x00003FFF
#define H264E_HAL_MASK_16b              (RK_U32)0x0000FFFF


#define h264e_hal_debug_enter() \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_FLOW)\
            mpp_log("line(%d), func(%s), enter", __LINE__, __FUNCTION__);\
    } while (0)

#define h264e_hal_debug_leave() \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_FLOW)\
            mpp_log("line(%d), func(%s), leave", __LINE__, __FUNCTION__);\
    } while (0)

#define h264e_hal_log_err(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_ERROR)\
            mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_detail(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_DETAIL)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_rc(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_RC)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_dpb(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_DPB)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_header(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_HEADER)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_sei(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_SEI)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_simple(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_SIMPLE)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h264e_hal_log_file(fmt, ...) \
    do {\
        if (h264e_hal_log_mode & H264E_HAL_LOG_FILE)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define H264E_HAL_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define H264E_HAL_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define H264E_HAL_MIN3(a,b,c) H264E_HAL_MIN((a),H264E_HAL_MIN((b),(c)))
#define H264E_HAL_MAX3(a,b,c) H264E_HAL_MAX((a),H264E_HAL_MAX((b),(c)))
#define H264E_HAL_MIN4(a,b,c,d) H264E_HAL_MIN((a),H264E_HAL_MIN3((b),(c),(d)))
#define H264E_HAL_MAX4(a,b,c,d) H264E_HAL_MAX((a),H264E_HAL_MAX3((b),(c),(d)))
#define H264E_HAL_CLIP3(v, min, max)  ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

#define H264E_HAL_FCLOSE(fp)    do{ if(fp) fclose(fp); fp = NULL; } while (0)

#define H264E_HAL_SET_REG(reg, addr, val)                                    \
    do {                                                                     \
        reg[(addr)>>2] = (RK_U32)(val);                                      \
        if (h264e_hal_log_mode & 0/*H264E_HAL_LOG_INFO*/)                              \
            mpp_log("line(%d) set reg[%03d/%03x]: %08x", __LINE__, (addr)>>2, addr, val); \
    } while (0)


#define H264E_HAL_VALIDATE_GT(val, name, limit)                    \
    do {                                                     \
        if ((val)<=(limit)) {                                 \
            mpp_err("%s(%d) should > %d", name, val, limit); \
            return MPP_NOK;                                  \
        }                                                    \
    } while (0)

#define H264E_HAL_VALIDATE_NEQ(val, name, limit)                    \
    do {                                                     \
        if((val)==(limit)) {                                 \
            mpp_err("%s(%d) should not = %d", name, val, limit); \
            return MPP_NOK;                                  \
        }                                                    \
    } while (0)

#define H264E_HAL_SPRINT(s, ...)  \
    do {                               \
        s += sprintf(s, ## __VA_ARGS__); \
    } while (0)

#define H264E_UUID_LENGTH                   16

#define H264E_REF_MAX                       16

#define H264E_SPSPPS_BUF_SIZE               512  //sps + pps
#define H264E_SEI_BUF_SIZE                  1024 //unit in byte, may not be large enough in the future
#define H264E_EXTRA_INFO_BUF_SIZE           (H264E_SPSPPS_BUF_SIZE + H264E_SEI_BUF_SIZE)

#define H264E_NUM_REFS               1
#define H264E_LONGTERM_REF_EN        0
#define H264E_CQM_FLAT               0
#define H264E_CQM_JVT                1
#define H264E_CQM_CUSTOM             2
#define H264E_B_PYRAMID_NONE         0
#define H264E_B_PYRAMID_STRICT       1
#define H264E_B_PYRAMID_NORMAL       2

#define H264E_CSP2_MASK           0x00ff  /* */
#define H264E_CSP2_NONE           0x0000  /* Invalid mode     */
#define H264E_CSP2_I420           0x0001  /* yuv 4:2:0 planar */
#define H264E_CSP2_YV12           0x0002  /* yvu 4:2:0 planar */
#define H264E_CSP2_NV12           0x0003  /* yuv 4:2:0, with one y plane and one packed u+v */
#define H264E_CSP2_I422           0x0004  /* yuv 4:2:2 planar */
#define H264E_CSP2_YV16           0x0005  /* yvu 4:2:2 planar */
#define H264E_CSP2_NV16           0x0006  /* yuv 4:2:2, with one y plane and one packed u+v */
#define H264E_CSP2_V210           0x0007  /* 10-bit yuv 4:2:2 packed in 32 */
#define H264E_CSP2_I444           0x0008  /* yuv 4:4:4 planar */
#define H264E_CSP2_YV24           0x0009  /* yvu 4:4:4 planar */
#define H264E_CSP2_BGR            0x000a  /* packed bgr 24bits   */
#define H264E_CSP2_BGRA           0x000b  /* packed bgr 32bits   */
#define H264E_CSP2_RGB            0x000c  /* packed rgb 24bits   */
#define H264E_CSP2_MAX            0x000d  /* end of list */
#define H264E_CSP2_VFLIP          0x1000  /* the csp is vertically flipped */
#define H264E_CSP2_HIGH_DEPTH     0x2000  /* the csp has a depth of 16 bits per pixel component */

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

typedef struct h264e_hal_rkv_csp_info_t {
    RK_U32 fmt;
    RK_U32 cswap; // U/V swap for YUV420SP/YUV422SP/YUYV422/UYUV422; R/B swap for BGRA88/RGB888/RGB565.
    RK_U32 aswap; // 0: BGRA, 1:ABGR.
} h264e_hal_rkv_csp_info;

/* transplant from vpu_api.h:EncInputPictureType */
typedef enum {
    H264E_VPU_CSP_YUV420P   = 0,    // YYYY... UUUU... VVVV
    H264E_VPU_CSP_YUV420SP  = 1,    // YYYY... UVUVUV...
    H264E_VPU_CSP_YUYV422   = 2,    // YUYVYUYV...
    H264E_VPU_CSP_UYVY422   = 3,    // UYVYUYVY...
    H264E_VPU_CSP_RGB565    = 4,    // 16-bit RGB
    H264E_VPU_CSP_BGR565    = 5,    // 16-bit RGB
    H264E_VPU_CSP_RGB555    = 6,    // 15-bit RGB
    H264E_VPU_CSP_BGR555    = 7,    // 15-bit RGB
    H264E_VPU_CSP_RGB444    = 8,    // 12-bit RGB
    H264E_VPU_CSP_BGR444    = 9,    // 12-bit RGB
    H264E_VPU_CSP_RGB888    = 10,   // 24-bit RGB
    H264E_VPU_CSP_BGR888    = 11,   // 24-bit RGB
    H264E_VPU_CSP_RGB101010 = 12,   // 30-bit RGB
    H264E_VPU_CSP_BGR101010 = 13,   // 30-bit RGB
    H264E_VPU_CSP_NONE,
    H264E_VPU_CSP_BUTT,
} h264e_hal_vpu_csp;

typedef struct h264e_hal_vpu_csp_info_t {
    RK_U32 fmt;
    RK_U32 r_mask_msb;
    RK_U32 g_mask_msb;
    RK_U32 b_mask_msb;
} h264e_hal_vpu_csp_info;

typedef enum h264e_chroma_fmt_t {
    H264E_CHROMA_400 = 0,
    H264E_CHROMA_420 = 1,
    H264E_CHROMA_422 = 2,
    H264E_CHROMA_444 = 3,
} h264e_chroma_fmt;

typedef enum h264e_cqm4_t {
    H264E_CQM_4IY = 0,
    H264E_CQM_4PY = 1,
    H264E_CQM_4IC = 2,
    H264E_CQM_4PC = 3
} h264e_cqm4;

typedef enum h264e_cqm8_t {
    H264E_CQM_8IY = 0,
    H264E_CQM_8PY = 1,
    H264E_CQM_8IC = 2,
    H264E_CQM_8PC = 3,
} h264e_cqm8;

typedef enum h264e_hal_slice_type_t {
    H264E_HAL_SLICE_TYPE_P  = 0,
    H264E_HAL_SLICE_TYPE_B  = 1,
    H264E_HAL_SLICE_TYPE_I  = 2,
} h264e_hal_slice_type;

typedef struct h264e_hal_sps_t {
    RK_S32 i_id;

    RK_S32 i_profile_idc;
    RK_S32 i_level_idc;

    RK_S32 b_constraint_set0;
    RK_S32 b_constraint_set1;
    RK_S32 b_constraint_set2;
    RK_S32 b_constraint_set3;

    RK_S32 i_log2_max_frame_num;

    RK_S32 i_poc_type;
    /* poc 0 */
    RK_S32 i_log2_max_poc_lsb;

    RK_S32 i_num_ref_frames;
    RK_S32 b_gaps_in_frame_num_value_allowed;
    RK_S32 i_mb_width;
    RK_S32 i_mb_height;
    RK_S32 b_frame_mbs_only;
    RK_S32 b_mb_adaptive_frame_field;
    RK_S32 b_direct8x8_inference;

    RK_S32 b_crop;
    struct {
        RK_S32 i_left;
        RK_S32 i_right;
        RK_S32 i_top;
        RK_S32 i_bottom;
    } crop;

    MppEncH264VuiCfg vui;

    RK_S32 b_qpprime_y_zero_transform_bypass;
    RK_S32 i_chroma_format_idc;

    /* only for backup, excluded in read SPS*/
    RK_S32 keyframe_max_interval;
} h264e_hal_sps;

typedef struct h264e_hal_pps_t {
    RK_S32 i_id;
    RK_S32 i_sps_id;

    RK_S32 b_cabac;

    RK_S32 b_pic_order;
    RK_S32 i_num_slice_groups;

    RK_S32 i_num_ref_idx_l0_default_active;
    RK_S32 i_num_ref_idx_l1_default_active;

    RK_S32 b_weighted_pred;
    RK_S32 i_weighted_bipred_idc;

    RK_S32 i_pic_init_qp;
    RK_S32 i_pic_init_qs;

    RK_S32 i_chroma_qp_index_offset;
    RK_S32 i_second_chroma_qp_index_offset;

    RK_S32 b_deblocking_filter_control;
    RK_S32 b_constrained_intra_pred;
    RK_S32 b_redundant_pic_cnt;

    RK_S32 b_transform_8x8_mode;

    RK_S32 b_cqm_preset; //pic_scaling_list_present_flag
    const RK_U8 *scaling_list[8]; /* could be 12, but we don't allow separate Cb/Cr lists */
} h264e_hal_pps;

typedef enum h264e_sei_payload_type_t {
    H264E_SEI_BUFFERING_PERIOD       = 0,
    H264E_SEI_PIC_TIMING             = 1,
    H264E_SEI_PAN_SCAN_RECT          = 2,
    H264E_SEI_FILLER                 = 3,
    H264E_SEI_USER_DATA_REGISTERED   = 4,
    H264E_SEI_USER_DATA_UNREGISTERED = 5,
    H264E_SEI_RECOVERY_POINT         = 6,
    H264E_SEI_DEC_REF_PIC_MARKING    = 7,
    H264E_SEI_FRAME_PACKING          = 45,
} h264e_sei_payload_type;

typedef enum h264e_rkv_nal_idx_t {
    RKV_H264E_RKV_NAL_IDX_SPS,
    RKV_H264E_RKV_NAL_IDX_PPS,
    RKV_H264E_RKV_NAL_IDX_SEI,
    RKV_H264E_RKV_NAL_IDX_BUTT,
} h264e_rkv_nal_idx;

typedef enum rkvenc_nal_unit_type_t {
    RKVENC_NAL_UNKNOWN     = 0,
    RKVENC_NAL_SLICE       = 1,
    RKVENC_NAL_SLICE_DPA   = 2,
    RKVENC_NAL_SLICE_DPB   = 3,
    RKVENC_NAL_SLICE_DPC   = 4,
    RKVENC_NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    RKVENC_NAL_SEI         = 6,    /* ref_idc == 0 */
    RKVENC_NAL_SPS         = 7,
    RKVENC_NAL_PPS         = 8,
    RKVENC_NAL_AUD         = 9,
    RKVENC_NAL_FILLER      = 12,
    /* ref_idc == 0 for 6,9,10,11,12 */
} rkvenc_nal_unit_type;

typedef enum rkvencnal_priority_t {
    RKVENC_NAL_PRIORITY_DISPOSABLE = 0,
    RKVENC_NAL_PRIORITY_LOW        = 1,
    RKVENC_NAL_PRIORITY_HIGH       = 2,
    RKVENC_NAL_PRIORITY_HIGHEST    = 3,
} rkvenc_nal_priority;

typedef struct h264e_hal_vui_param_t {
    /* they will be reduced to be 0 < x <= 65535 and prime */
    RK_S32         i_sar_height;
    RK_S32         i_sar_width;

    RK_S32         i_overscan;    /* 0=undef, 1=no overscan, 2=overscan */

    /* see h264 annex E for the values of the following */
    RK_S32         i_vidformat;
    RK_S32         b_fullrange;
    RK_S32         i_colorprim;
    RK_S32         i_transfer;
    RK_S32         i_colmatrix;
    RK_S32         i_chroma_loc;    /* both top & bottom */
} h264e_hal_vui_param;

typedef struct h264e_hal_sei_t {
    RK_U32                       frame_cnt;
    h264e_control_extra_info_cfg extra_info_cfg;
    MppEncRcCfg                  rc_cfg;
} h264e_hal_sei;

typedef struct h264e_hal_ref_param_t {
    RK_S32         i_frame_reference;  /* Maximum number of reference frames */
    RK_S32         i_ref_pos;
    RK_S32         i_long_term_en;
    RK_S32         i_long_term_internal;
    RK_S32         hw_longterm_mode;
    RK_S32         i_dpb_size;         /* Force a DPB size larger than that implied by B-frames and reference frames.
                                        * Useful in combination with interactive error resilience. */
    RK_S32         i_frame_packing;
} h264e_hal_ref_param;


typedef struct h264e_hal_param_t {
    RK_S32 hw_type; // 0:rkv, 1:vpu
    RK_S32 constrained_intra;

    h264e_hal_vui_param vui;
    h264e_hal_ref_param ref;
} h264e_hal_param;

typedef struct h264e_hal_context_t {
    MppHalApi                       api;
    RK_S32                          vpu_fd;
    IOInterruptCB                   int_cb;
    h264e_feedback                  feedback;
    void                            *regs;
    void                            *ioctl_input;
    void                            *ioctl_output;
    void                            *buffers;
    void                            *extra_info;
    void                            *dpb_ctx;
    RK_U32                          frame_cnt_gen_ready;
    RK_U32                          frame_cnt_send_ready;
    RK_U32                          num_frames_to_send;
    RK_U32                          frame_cnt;
    h264e_hal_param                 param;
    RK_U32                          enc_mode;
    void                            *dump_files;

    void                            *param_buf;
    MppPacket                       packeted_param;
    void                            *test_cfg;

    RK_U32                          osd_plt_type; //0:user define, 1:default
    MppEncOSDData                   osd_data;
    MppEncSeiMode                   sei_mode;

    MppEncCfgSet                    *cfg;
    MppEncCfgSet                    *set;
    RK_U32                          idr_pic_id;

    RK_U32                          buffer_ready;
    H264eHwCfg                      hw_cfg;
    MppLinReg                       *inter_qs;
    MppLinReg                       *intra_qs;
    MppData                         *qp_p;
} h264e_hal_context;

MPP_RET hal_h264e_set_sps(h264e_hal_context *ctx, h264e_hal_sps *sps);
MPP_RET hal_h264e_set_pps(h264e_hal_context *ctx, h264e_hal_pps *pps, h264e_hal_sps *sps);
void hal_h264e_set_param(h264e_hal_param *p, RK_S32 hw_type);

const RK_U8 * const h264e_cqm_jvt[8];
const RK_U8 h264e_zigzag_scan4[2][16];
const RK_U8 h264e_zigzag_scan8[2][64];
void hal_h264e_rkv_set_format(H264eHwCfg *hw_cfg, MppEncPrepCfg *prep_cfg);
void hal_h264e_vpu_set_format(H264eHwCfg *hw_cfg, MppEncPrepCfg *prep_cfg);

#endif
