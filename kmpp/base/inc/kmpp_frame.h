/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef KMPP_FRAME_H
#define KMPP_FRAME_H

#include "mpp_frame.h"

#define KMPP_FRAME_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, u32, rk_u32,              width,              FLAG_NONE,  width) \
    ENTRY(prefix, u32, rk_u32,              height,             FLAG_NONE,  height) \
    ENTRY(prefix, u32, rk_u32,              hor_stride,         FLAG_NONE,  hor_stride) \
    ENTRY(prefix, u32, rk_u32,              ver_stride,         FLAG_NONE,  ver_stride) \
    ENTRY(prefix, u32, rk_u32,              hor_stride_pixel,   FLAG_NONE,  hor_stride_pixel) \
    ENTRY(prefix, u32, rk_u32,              offset_x,           FLAG_NONE,  offset_x) \
    ENTRY(prefix, u32, rk_u32,              offset_y,           FLAG_NONE,  offset_y) \
    ENTRY(prefix, u32, rk_u32,              poc,                FLAG_NONE,  poc) \
    ENTRY(prefix, s64, rk_s64,              pts,                FLAG_NONE,  pts) \
    ENTRY(prefix, s64, rk_s64,              dts,                FLAG_NONE,  dts) \
    ENTRY(prefix, u32, rk_u32,              eos,                FLAG_NONE,  eos) \
    ENTRY(prefix, u32, rk_u32,              color_range,        FLAG_NONE,  color_range) \
    ENTRY(prefix, u32, rk_u32,              color_primaries,    FLAG_NONE,  color_primaries) \
    ENTRY(prefix, u32, rk_u32,              color_trc,          FLAG_NONE,  color_trc) \
    ENTRY(prefix, u32, rk_u32,              colorspace,         FLAG_NONE,  colorspace) \
    ENTRY(prefix, u32, rk_u32,              chroma_location,    FLAG_NONE,  chroma_location) \
    ENTRY(prefix, u32, rk_u32,              fmt,                FLAG_NONE,  fmt) \
    ENTRY(prefix, u32, rk_u32,              buf_size,           FLAG_NONE,  buf_size) \
    ENTRY(prefix, u32, rk_u32,              buf_fd,             FLAG_NONE,  buf_fd) \
    ENTRY(prefix, u32, rk_u32,              is_gray,            FLAG_NONE,  is_gray) \
    STRCT(prefix, shm, KmppShmPtr,          meta,               FLAG_NONE,  meta) \
    STRCT(prefix, st,  MppFrameRational,    sar,                FLAG_NONE,  sar) \
    STRCT(prefix, shm, KmppShmPtr,          buffer,             FLAG_NONE,  buffer)

#define KMPP_FRAME_INFO_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, s32, rk_s32,              chan_id,            FLAG_NONE,  chan_id) \
    ENTRY(prefix, u32, rk_u32,              line_nb,            FLAG_NONE,  line_nb) \
    ENTRY(prefix, u32, rk_u32,              eos,                FLAG_NONE,  eos) \
    ENTRY(prefix, u32, rk_u32,              info_change,        FLAG_NONE,  info_change)

#ifdef __cplusplus
extern "C" {
#endif

#define KMPP_OBJ_NAME           kmpp_frame
#define KMPP_OBJ_INTF_TYPE      KmppFrame
#define KMPP_OBJ_ENTRY_TABLE    KMPP_FRAME_ENTRY_TABLE
#include "kmpp_obj_func.h"

rk_s32 kmpp_frame_has_meta(KmppFrame frame);
rk_s32 kmpp_frame_get_meta_obj(KmppFrame frame, KmppMeta *meta);

/* Release borrowed FrameInfo wrappers with kmpp_obj_impl_put_f(). */
#define KMPP_OBJ_NAME           kmpp_frame_info
#define KMPP_OBJ_INTF_TYPE      KmppFrameInfo
#define KMPP_OBJ_ENTRY_TABLE    KMPP_FRAME_INFO_ENTRY_TABLE
#include "kmpp_obj_func.h"

#ifdef __cplusplus
}
#endif

#endif /* KMPP_FRAME_H */
