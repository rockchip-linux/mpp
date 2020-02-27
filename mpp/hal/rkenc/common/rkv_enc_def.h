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
#ifndef __RKV_ENC_DEF_H__
#define __RKV_ENC_DEF_H__

#define RKV_ENC_INT_ONE_FRAME_FINISH    0x00000001
#define RKV_ENC_INT_LINKTABLE_FINISH    0x00000002
#define RKV_ENC_INT_SAFE_CLEAR_FINISH   0x00000004
#define RKV_ENC_INT_ONE_SLICE_FINISH    0x00000008
#define RKV_ENC_INT_BIT_STREAM_OVERFLOW 0x00000010
#define RKV_ENC_INT_BUS_WRITE_FULL      0x00000020
#define RKV_ENC_INT_BUS_WRITE_ERROR     0x00000040
#define RKV_ENC_INT_BUS_READ_ERROR      0x00000080
#define RKV_ENC_INT_TIMEOUT_ERROR       0x00000100

#define RKVENC_CODING_TYPE_AUTO          0x0000  /* Let enc choose the right type */
#define RKVENC_CODING_TYPE_IDR           0x0001
#define RKVENC_CODING_TYPE_I             0x0002
#define RKVENC_CODING_TYPE_P             0x0003
#define RKVENC_CODING_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define RKVENC_CODING_TYPE_B             0x0005
#define RKVENC_CODING_TYPE_KEYFRAME      0x0006  /* IDR or I depending on b_open_gop option */

#define RKVE_IOC_CUSTOM_BASE           0x1000
#define RKVE_IOC_SET_OSD_PLT           (RKVE_IOC_CUSTOM_BASE + 1)
#define RKVE_IOC_SET_L2_REG            (RKVE_IOC_CUSTOM_BASE + 2)


#define RKVE_CSP2_MASK             0x00ff  /* */
#define RKVE_CSP2_NONE             0x0000  /* Invalid mode     */
#define RKVE_CSP2_I420             0x0001  /* yuv 4:2:0 planar */
#define RKVE_CSP2_YV12             0x0002  /* yvu 4:2:0 planar */
#define RKVE_CSP2_NV12             0x0003  /* yuv 4:2:0, with one y plane and one packed u+v */
#define RKVE_CSP2_I422             0x0004  /* yuv 4:2:2 planar */
#define RKVE_CSP2_YV16             0x0005  /* yvu 4:2:2 planar */
#define RKVE_CSP2_NV16             0x0006  /* yuv 4:2:2, with one y plane and one packed u+v */
#define RKVE_CSP2_V210             0x0007  /* 10-bit yuv 4:2:2 packed in 32 */
#define RKVE_CSP2_I444             0x0008  /* yuv 4:4:4 planar */
#define RKVE_CSP2_YV24             0x0009  /* yvu 4:4:4 planar */
#define RKVE_CSP2_BGR              0x000a  /* packed bgr 24bits   */
#define RKVE_CSP2_BGRA             0x000b  /* packed bgr 32bits   */
#define RKVE_CSP2_RGB              0x000c  /* packed rgb 24bits   */
#define RKVE_CSP2_MAX              0x000d  /* end of list */
#define RKVE_CSP2_VFLIP            0x1000  /* the csp is vertically flipped */
#define RKVE_CSP2_HIGH_DEPTH       0x2000  /* the csp has a depth of 16 bits per pixel component */

#define RKVE_MB_RC_ONLY_QUALITY    0
#define RKVE_MB_RC_MORE_QUALITY    1
#define RKVE_MB_RC_BALANCE         2
#define RKVE_MB_RC_MORE_BITRATE    3
#define RKVE_MB_RC_ONLY_BITRATE    4
#define RKVE_MB_RC_WIDE_RANGE      5
#define RKVE_MB_RC_ONLY_AQ         6
#define RKVE_MB_RC_M_NUM           7

typedef enum RkveCsp_e {
    RKVE_CSP_BGRA8888,         // 0
    RKVE_CSP_BGR888,           // 1
    RKVE_CSP_BGR565,           // 2
    RKVE_CSP_NONE,             // 3
    RKVE_CSP_YUV422SP,         // 4
    RKVE_CSP_YUV422P,          // 5
    RKVE_CSP_YUV420SP,         // 6
    RKVE_CSP_YUV420P,          // 7
    RKVE_CSP_YUYV422,          // 8
    RKVE_CSP_UYVY422,          // 9
    RKVE_CSP_BUTT,             // 10
} RkveCsp;

typedef enum enc_hal_rkv_buf_grp_t {
    ENC_HAL_RKV_BUF_GRP_PP,
    ENC_HAL_RKV_BUF_GRP_DSP,
    ENC_HAL_RKV_BUF_GRP_MEI,
    ENC_HAL_RKV_BUF_GRP_CMV,
    ENC_HAL_RKV_BUF_GRP_ROI,
    ENC_HAL_RKV_BUF_GRP_REC,
    ENC_HAL_RKV_BUF_GRP_BUTT
} enc_hal_rkv_buf_grp;

typedef enum enc_rkv_lnktable_t {
    RKVENC_LINKTABLE_DISABLE = 1,
    RKVENC_LINKTABLE_START,
    RKVENC_LINKTABLE_UPDATE
} enc_rkv_lnk_table;

typedef struct {
    RK_U8 qp_y          : 6;
    RK_U8 set_qp_y_en   : 1;
    RK_U8 forbid_inter  : 1;
} RkvRoiCfg;

typedef struct {
    RK_U16 forbid_inter    : 1;      /* intra flag for cu 16x16 */
    RK_U16 reserved        : 3;      /* reserved */
    RK_U16 qp_area_idx     : 3;      /* roi range index */
    RK_U16 area_map        : 1;      /* roi en */
    RK_S16 qp_y            : 7;      /* qp_value,  absolute qp or relative qp*/
    RK_U16 set_qp_y        : 1;
} RkvRoiCfg_v2;

typedef enum ReOsdPltType_e {
    RKVE_OSD_PLT_TYPE_NONE     = -1,
    RKVE_OSD_PLT_TYPE_USERDEF  = 0,
    RKVE_OSD_PLT_TYPE_DEFAULT  = 1,
} RkveOsdPltType;


#define RKV_ENC_MODE                  RKVENC_LINKTABLE_DISABLE //2
#define RKVE_LINKTABLE_FRAME_NUM       1 //2

#if RKVE_ENC_MODE == RKVENC_LINKTABLE_START
#define RKVE_LINKTABLE_EACH_NUM        RKVE_LINKTABLE_FRAME_NUM
#else
#define RKVE_LINKTABLE_EACH_NUM        1
#endif
#define RKVE_LINKTABLE_MAX_SIZE        256

#define RKVE_RC_TEXTURE_THR_SIZE 16

#endif
