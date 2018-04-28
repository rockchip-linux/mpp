/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_JPEGD_BASE_H__
#define __HAL_JPEGD_BASE_H__

#include <stdio.h>
#include "rk_type.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_device.h"

#define EXTRA_INFO_MAGIC                      (0x4C4A46)

typedef struct JpegdIocExtInfoSlot_t {
    RK_U32                 reg_idx;
    RK_U32                 offset;
} JpegdIocExtInfoSlot;

typedef struct JpegdIocExtInfo_t {
    RK_U32                 magic; /* tell kernel that it is extra info */
    RK_U32                 cnt;
    JpegdIocExtInfoSlot    slots[5];
} JpegdIocExtInfo;

typedef struct PPInfo_t {
    /* PP parameters */
    RK_U8                  pp_enable; /* 0 - disable; 1 - enable */
    RK_U8                  pp_in_fmt; /* PP input format */
    RK_U8                  pp_out_fmt;/* PP output format */
    RK_U8                  dither_enable; /* for PP output RGB565 */
    RK_U32                 crop_width;
    RK_U32                 crop_height;
    RK_U32                 crop_x;
    RK_U32                 crop_y;
} PPInfo;

typedef struct JpegdHalCtx {
    MppBufSlots            packet_slots;
    MppBufSlots            frame_slots;
    MppDevCtx              dev_ctx;
    void                   *regs;
    MppBufferGroup         group;
    MppBuffer              frame_buf;
    MppBuffer              pTableBase;
    MppHalApi              hal_api;

    MppFrameFormat         output_fmt;
    RK_U32                 set_output_fmt_flag;
    RK_U32                 hal_debug_enable;
    RK_U32                 frame_count;
    RK_U32                 output_yuv_count;

    RK_S32                 pkt_fd;    /* input stream's physical address(fd) */
    RK_S32                 frame_fd;  /* output picture's physical address(fd) */

    PPInfo                 pp_info;

    FILE                   *fp_reg_in;
    FILE                   *fp_reg_out;
} JpegdHalCtx;

#endif /* __HAL_JPEGD_COMMON_H__ */
