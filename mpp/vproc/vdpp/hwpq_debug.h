/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HWPQ_DEBUG_H_
#define __HWPQ_DEBUG_H_

#include <stdio.h>
#include "mpp_log.h"

#define HWPQ_VDPP_TRACE           (0x00000001)
#define HWPQ_VDPP_INFO            (0x00000002)
#define HWPQ_VDPP_DUMP_IN         (0x00000010)
#define HWPQ_VDPP_DUMP_OUT        (0x00000020)

#define HWPQ_VDPP_DBG(flag, fmt, ...)    _mpp_dbg(hwpq_vdpp_debug, flag, fmt, ## __VA_ARGS__)
#define HWPQ_VDPP_DBG_F(flag, fmt, ...)  _mpp_dbg_f(hwpq_vdpp_debug, flag, fmt, ## __VA_ARGS__)

#define hwpq_vdpp_dbg(type, fmt, ...) \
    do {\
        if (hwpq_vdpp_debug & type)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define hwpq_vdpp_enter() \
    do {\
        if (hwpq_vdpp_debug & HWPQ_VDPP_TRACE)\
            mpp_log("line(%d), func(%s), enter", __LINE__, __FUNCTION__);\
    } while (0)

#define hwpq_vdpp_leave() \
    do {\
        if (hwpq_vdpp_debug & HWPQ_VDPP_TRACE)\
            mpp_log("line(%d), func(%s), leave", __LINE__, __FUNCTION__);\
    } while (0)

#define hwpq_vdpp_info(fmt, ...) \
    do {\
        if (hwpq_vdpp_debug & HWPQ_VDPP_INFO)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

extern RK_U32 hwpq_vdpp_debug;

#endif // __HWPQ_DEBUG_H_