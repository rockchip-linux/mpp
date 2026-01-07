/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_DEBUG_H
#define H265D_DEBUG_H

#include "mpp_debug.h"

extern RK_U32 h265d_debug;

#define H265D_DBG_FUNCTION          (0x00000001)
#define H265D_DBG_SPLIT             (0x00000002)
#define H265D_DBG_PTS               (0x00000004)

#define H265D_DBG_VPS               (0x00000010)
#define H265D_DBG_SPS               (0x00000020)
#define H265D_DBG_PPS               (0x00000040)
#define H265D_DBG_SLICE             (0x00000080)
#define H265D_DBG_SEI               (0x00000100)
#define H265D_DBG_GLOBAL            (0x00001000)
#define H265D_DBG_REF               (0x00002000)
#define H265D_DBG_DETAIL            (0x00004000)

#define h265d_dbg(flag, fmt, ...)   mpp_dbg(h265d_debug, flag, fmt, ## __VA_ARGS__)

#define h265d_dbg_func(fmt, ...)    h265d_dbg(H265D_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define h265d_dbg_split(fmt, ...)   h265d_dbg(H265D_DBG_SPLIT, fmt, ## __VA_ARGS__)
#define h265d_dbg_pts(fmt, ...)     h265d_dbg(H265D_DBG_PTS, fmt, ## __VA_ARGS__)
#define h265d_dbg_vps(fmt, ...)     h265d_dbg(H265D_DBG_VPS, fmt, ## __VA_ARGS__)
#define h265d_dbg_sps(fmt, ...)     h265d_dbg(H265D_DBG_SPS, fmt, ## __VA_ARGS__)
#define h265d_dbg_pps(fmt, ...)     h265d_dbg(H265D_DBG_PPS, fmt, ## __VA_ARGS__)
#define h265d_dbg_slice(fmt, ...)   h265d_dbg(H265D_DBG_SLICE, fmt, ## __VA_ARGS__)
#define h265d_dbg_sei(fmt, ...)     h265d_dbg(H265D_DBG_SEI, fmt, ## __VA_ARGS__)
#define h265d_dbg_global(fmt, ...)  h265d_dbg(H265D_DBG_GLOBAL, fmt, ## __VA_ARGS__)
#define h265d_dbg_ref(fmt, ...)     h265d_dbg(H265D_DBG_REF, fmt, ## __VA_ARGS__)
#define h265d_dbg_detail(fmt, ...)  h265d_dbg(H265D_DBG_DETAIL, fmt, ## __VA_ARGS__)

#endif /* H265D_DEBUG_H */
