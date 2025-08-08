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
#include "vepu5xx_common.h"

#define VEPU541_REG_BASE_HW_STATUS  0x0000001C
#define VEPU541_REG_BASE_STATISTICS 0x00000210
#define VEPU541_REG_BASE_OSD_PLT    0x00000400
#define VEPU541_REG_BASE_L2         0x00010004

#define VEPU541_MAX_ROI_NUM         8

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

typedef struct Vepu541B8NumQp_t {
    RK_U32  b8num_qp                : 18;
    RK_U32  reserved                : 14;
} Vepu541B8NumQp;

#ifdef __cplusplus
extern "C" {
#endif

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
MPP_RET vepu541_set_one_roi(void *buf, MppEncROIRegion *region, RK_S32 w, RK_S32 h);

MPP_RET vepu541_set_osd(Vepu5xxOsdCfg *cfg);
MPP_RET vepu540_set_osd(Vepu5xxOsdCfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __VEPU541_COMMON_H__ */
