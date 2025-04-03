/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FRAME_H__
#define __KMPP_FRAME_H__

#include "mpp_frame.h"

#define KMPP_FRAME_ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, u32, rk_u32, width) \
    ENTRY(prefix, u32, rk_u32, height) \
    ENTRY(prefix, u32, rk_u32, hor_stride) \
    ENTRY(prefix, u32, rk_u32, ver_stride) \
    ENTRY(prefix, u32, rk_u32, hor_stride_pixel) \
    ENTRY(prefix, u32, rk_u32, offset_x) \
    ENTRY(prefix, u32, rk_u32, offset_y) \
    ENTRY(prefix, u32, rk_u32, poc) \
    ENTRY(prefix, s64, rk_s64, pts) \
    ENTRY(prefix, s64, rk_s64, dts) \
    ENTRY(prefix, u32, rk_u32, eos) \
    ENTRY(prefix, u32, rk_u32, color_range) \
    ENTRY(prefix, u32, rk_u32, color_primaries) \
    ENTRY(prefix, u32, rk_u32, color_trc) \
    ENTRY(prefix, u32, rk_u32, colorspace) \
    ENTRY(prefix, u32, rk_u32, chroma_location) \
    ENTRY(prefix, u32, rk_u32, fmt) \
    ENTRY(prefix, u32, rk_u32, buf_size) \
    ENTRY(prefix, u32, rk_u32, is_gray)

#define KMPP_FRAME_STRUCT_TABLE(ENTRY, prefix) \
    ENTRY(prefix, shm, KmppShmPtr, meta) \
    ENTRY(prefix, shm, KmppShmPtr, buffer) \
    ENTRY(prefix, st,  MppFrameRational, sar)

#ifdef __cplusplus
extern "C" {
#endif

#define KMPP_OBJ_NAME           kmpp_frame
#define KMPP_OBJ_INTF_TYPE      KmppFrame
#define KMPP_OBJ_ENTRY_TABLE    KMPP_FRAME_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE   KMPP_FRAME_STRUCT_TABLE
#include "kmpp_obj_func.h"

#ifdef __cplusplus
}
#endif

#endif /*__KMPP_FRAME_H__*/
