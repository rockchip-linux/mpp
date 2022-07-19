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
#ifndef __VEPU541_COMMON_H__
#define __VEPU541_COMMON_H__

#include "rk_venc_cmd.h"
#include "mpp_device.h"

#define VEPU541_REG_BASE_HW_STATUS  0x0000001C
#define VEPU541_REG_BASE_STATISTICS 0x00000210
#define VEPU541_REG_BASE_OSD_PLT    0x00000400
#define VEPU541_REG_BASE_L2         0x00010004

#define VEPU541_MAX_ROI_NUM         8
#define VEPU580_SLICE_FIFO_LEN      32

typedef enum Vepu541Fmt_e {
    VEPU541_FMT_BGRA8888,   // 0
    VEPU541_FMT_BGR888,     // 1
    VEPU541_FMT_BGR565,     // 2
    VEPU541_FMT_NONE,       // 3
    VEPU541_FMT_YUV422SP,   // 4
    VEPU541_FMT_YUV422P,    // 5
    VEPU541_FMT_YUV420SP,   // 6
    VEPU541_FMT_YUV420P,    // 7
    VEPU541_FMT_YUYV422,    // 8
    VEPU541_FMT_UYVY422,    // 9
    VEPU541_FMT_BUTT,       // 10

    /* vepu540 add YUV400 support */
    VEPU540_FMT_YUV400      = VEPU541_FMT_BUTT,     // 10
    VEPU540_FMT_BUTT,       // 11

    /* vepu580 add YUV444 support */
    VEPU580_FMT_YUV444SP    = 12,
    VEPU580_FMT_YUV444P     = 13,
    VEPU580_FMT_BUTT,       // 14
} Vepu541Fmt;

typedef struct VepuFmtCfg_t {
    Vepu541Fmt      format;
    RK_U32          alpha_swap;
    RK_U32          rbuv_swap;
    RK_U32          src_range;
    RK_U32          src_endian;
    const RK_S32    *weight;
    const RK_S32    *offset;
} VepuFmtCfg;

/*
 * Vepu541RoiCfg
 *
 * Each Vepu541RoiCfg in roi buffer indicates a 16x16 cu encoding config at
 * corresponding position.
 *
 * NOTE: roi buffer should be aligned to 64x64 each is 4 16x16 in horizontal
 * and 4 16x16 in vertical. So the buffer need to be enlarged to avoid hardware
 * access error.
 */
typedef struct Vepu541RoiCfg_t {
    /*
     * Force_intra
     * 1 - The corresponding 16x16cu is forced to be intra
     * 0 - Not force to intra
     */
    RK_U16 force_intra  : 1;
    RK_U16 reserved     : 3;
    /*
     * Qp area index
     * The choosed qp area index.
     */
    RK_U16 qp_area_idx  : 3;
    /*
     * Area qp limit function enable flag
     * Force to be true in vepu541
     */
    RK_U16 qp_area_en   : 1;
    /*
     * Qp_adj
     * Qp_adj
     * in absolute qp mode qp_adj is the final qp used by encoder
     * in relative qp mode qp_adj is a adjustment to final qp
     */
    RK_S16 qp_adj       : 7;
    /*
     * Qp_adj_mode
     * Qp adjustment mode
     * 1 - absolute qp mode:
     *     the 16x16 MB qp is set to the qp_adj value
     * 0 - relative qp mode
     *     the 16x16 MB qp is adjusted by qp_adj value
     */
    RK_U16 qp_adj_mode  : 1;
} Vepu541RoiCfg;

typedef struct Vepu541OsdPos_t {
    /* X coordinate/16 of OSD region's left-top point. */
    RK_U32  osd_lt_x                : 8;
    /* Y coordinate/16 of OSD region's left-top point. */
    RK_U32  osd_lt_y                : 8;
    /* X coordinate/16 of OSD region's right-bottom point. */
    RK_U32  osd_rb_x                : 8;
    /* Y coordinate/16 of OSD region's right-bottom point. */
    RK_U32  osd_rb_y                : 8;
} Vepu541OsdPos;

typedef struct Vepu580OsdPos_t {
    /* X coordinate/16 of OSD region's left-top point. */
    RK_U32  osd_lt_x                : 10;
    RK_U32  reserved0               : 6;
    /* Y coordinate/16 of OSD region's left-top point. */
    RK_U32  osd_lt_y                : 10;
    RK_U32  reserved1               : 6;
    /* X coordinate/16 of OSD region's right-bottom point. */
    RK_U32  osd_rb_x                : 10;
    RK_U32  reserved2               : 6;
    /* Y coordinate/16 of OSD region's right-bottom point. */
    RK_U32  osd_rb_y                : 10;
    RK_U32  reserved3               : 6;
} Vepu580OsdPos;

typedef struct Vepu541B8NumQp_t {
    RK_U32  b8num_qp                : 18;
    RK_U32  reserved                : 14;
} Vepu541B8NumQp;

typedef struct Vepu541OsdPltColor_t {
    /* V component */
    RK_U32  v                       : 8;
    /* U component */
    RK_U32  u                       : 8;
    /* Y component */
    RK_U32  y                       : 8;
    /* Alpha */
    RK_U32  alpha                   : 8;
} Vepu541OsdPltColor;

typedef struct Vepu541OsdCfg_t {
    void                *reg_base;
    MppDev              dev;
    MppDevRegOffCfgs    *reg_cfg;
    MppEncOSDPltCfg     *plt_cfg;
    MppEncOSDData       *osd_data;
    MppEncOSDData2      *osd_data2;
} Vepu541OsdCfg;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vepu541_set_fmt(VepuFmtCfg *cfg, MppFrameFormat format);

/*
 * roi function
 *
 * vepu541_get_roi_buf_size
 * Calculate roi buffer size for image with size w * h
 *
 * vepu541_set_roi
 * Setup roi config buffeer for image with mb count mb_w * mb_h
 */
RK_S32  vepu541_get_roi_buf_size(RK_S32 w, RK_S32 h);
MPP_RET vepu541_set_roi(void *buf, MppEncROICfg *roi, RK_S32 w, RK_S32 h);

MPP_RET vepu541_set_osd(Vepu541OsdCfg *cfg);
MPP_RET vepu540_set_osd(Vepu541OsdCfg *cfg);
MPP_RET vepu580_set_osd(Vepu541OsdCfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __VEPU541_COMMON_H__ */
