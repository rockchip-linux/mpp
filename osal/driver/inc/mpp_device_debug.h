/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_DEVICE_DEBUG_H__
#define __MPP_DEVICE_DEBUG_H__

#include "mpp_debug.h"

#define MPP_DEVICE_DBG_FUNC                 (0x00000001)
#define MPP_DEVICE_DBG_PROBE                (0x00000002)
#define MPP_DEVICE_DBG_DETAIL               (0x00000004)
#define MPP_DEVICE_DBG_REG                  (0x00000010)
#define MPP_DEVICE_DBG_TIME                 (0x00000020)
#define MPP_DEVICE_DBG_MSG                  (0x00000040)
#define MPP_DEVICE_DBG_BUF                  (0x00000080)

#define mpp_dev_dbg(flag, fmt, ...)         _mpp_dbg(mpp_device_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_f(flag, fmt, ...)       _mpp_dbg_f(mpp_device_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_dev_dbg_func(fmt, ...)          mpp_dev_dbg_f(MPP_DEVICE_DBG_FUNC, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_probe(fmt, ...)         mpp_dev_dbg_f(MPP_DEVICE_DBG_PROBE, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_detail(fmt, ...)        mpp_dev_dbg(MPP_DEVICE_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_reg(fmt, ...)           mpp_dev_dbg(MPP_DEVICE_DBG_REG, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_time(fmt, ...)          mpp_dev_dbg(MPP_DEVICE_DBG_TIME, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_msg(fmt, ...)           mpp_dev_dbg(MPP_DEVICE_DBG_MSG, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_buf(fmt, ...)           mpp_dev_dbg(MPP_DEVICE_DBG_BUF, fmt, ## __VA_ARGS__)

extern RK_U32 mpp_device_debug;

#endif /* __MPP_DEVICE_DEBUG_H__ */
