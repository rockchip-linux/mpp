/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_DEBUG_H
#define MPP_DEBUG_H

#include <stdlib.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"

#define MPP_DBG_TIMING                  (0x00000001)
#define MPP_DBG_PTS                     (0x00000002)
#define MPP_DBG_INFO                    (0x00000004)
#define MPP_DBG_PLATFORM                (0x00000010)

#define MPP_DBG_DUMP_LOG                (0x00000100)
#define MPP_DBG_DUMP_IN                 (0x00000200)
#define MPP_DBG_DUMP_OUT                (0x00000400)
#define MPP_DBG_DUMP_CFG                (0x00000800)

#define mpp_dbg(debug, flag, fmt, ...)     mpp_log_c((debug) & (flag), fmt, ## __VA_ARGS__)
#define mpp_dbg_f(debug, flag, fmt, ...)   mpp_log_cf((debug) & (flag), fmt, ## __VA_ARGS__)

#define sys_dbg(flag, fmt, ...)         mpp_dbg(mpp_debug, flag, fmt, ## __VA_ARGS__)
#define sys_dbg_f(flag, fmt, ...)       mpp_dbg_f(mpp_debug, flag, fmt, ## __VA_ARGS__)

#define sys_dbg_pts(fmt, ...)           sys_dbg(MPP_DBG_PTS, fmt, ## __VA_ARGS__)
#define sys_dbg_info(fmt, ...)          sys_dbg(MPP_DBG_INFO, fmt, ## __VA_ARGS__)
#define sys_dbg_platform(fmt, ...)      sys_dbg(MPP_DBG_PLATFORM, fmt, ## __VA_ARGS__)

#define MPP_ABORT                       (0x10000000)

/*
 * mpp_dbg usage:
 *
 * in h264d module define module debug flag variable like: h265d_debug
 * then define h265d_dbg macro as follow :
 *
 * extern RK_U32 h265d_debug;
 *
 * #define H265D_DBG_FUNCTION          (0x00000001)
 * #define H265D_DBG_VPS               (0x00000002)
 * #define H265D_DBG_SPS               (0x00000004)
 * #define H265D_DBG_PPS               (0x00000008)
 * #define H265D_DBG_SLICE_HDR         (0x00000010)
 *
 * #define h265d_dbg(flag, fmt, ...) mpp_dbg(h265d_debug, flag, fmt, ## __VA_ARGS__)
 *
 * finally use environment control the debug flag
 *
 * mpp_get_env_u32("h264d_debug", &h265d_debug, 0)
 *
 */
/*
 * sub-module debug flag usage example:
 * +------+-------------------+
 * | 8bit |      24bit        |
 * +------+-------------------+
 *  0~15 bit: software debug print
 * 16~23 bit: hardware debug print
 * 24~31 bit: information print format
 */

#define mpp_abort() do {                \
    if (mpp_debug & MPP_ABORT) {        \
        abort();                        \
    }                                   \
} while (0)

#define MPP_STRINGS(x)      MPP_TO_STRING(x)
#define MPP_TO_STRING(x)    #x

#define mpp_assert(cond) do {                                           \
    if (!(cond)) {                                                      \
        mpp_err("Assertion %s failed at %s:%d\n",                       \
               MPP_STRINGS(cond), __FUNCTION__, __LINE__);              \
        mpp_abort();                                                    \
    }                                                                   \
} while (0)

/* llog for long log */

#define mpp_llogf(fmt, ...)     mpp_llog(MPP_LOG_FATAL,   MODULE_TAG, fmt, NULL, ## __VA_ARGS__)
#define mpp_lloge(fmt, ...)     mpp_llog(MPP_LOG_ERROR,   MODULE_TAG, fmt, NULL, ## __VA_ARGS__)
#define mpp_llogw(fmt, ...)     mpp_llog(MPP_LOG_WARN,    MODULE_TAG, fmt, NULL, ## __VA_ARGS__)
#define mpp_llogi(fmt, ...)     mpp_llog(MPP_LOG_INFO,    MODULE_TAG, fmt, NULL, ## __VA_ARGS__)
#define mpp_llogd(fmt, ...)     mpp_llog(MPP_LOG_DEBUG,   MODULE_TAG, fmt, NULL, ## __VA_ARGS__)
#define mpp_llogv(fmt, ...)     mpp_llog(MPP_LOG_VERBOSE, MODULE_TAG, fmt, NULL, ## __VA_ARGS__)

#define mpp_llogf_f(fmt, ...)   mpp_llog(MPP_LOG_FATAL,   MODULE_TAG, fmt, __FUNCTION__, ## __VA_ARGS__)
#define mpp_lloge_f(fmt, ...)   mpp_llog(MPP_LOG_ERROR,   MODULE_TAG, fmt, __FUNCTION__, ## __VA_ARGS__)
#define mpp_llogw_f(fmt, ...)   mpp_llog(MPP_LOG_WARN,    MODULE_TAG, fmt, __FUNCTION__, ## __VA_ARGS__)
#define mpp_llogi_f(fmt, ...)   mpp_llog(MPP_LOG_INFO,    MODULE_TAG, fmt, __FUNCTION__, ## __VA_ARGS__)
#define mpp_llogd_f(fmt, ...)   mpp_llog(MPP_LOG_DEBUG,   MODULE_TAG, fmt, __FUNCTION__, ## __VA_ARGS__)
#define mpp_llogv_f(fmt, ...)   mpp_llog(MPP_LOG_VERBOSE, MODULE_TAG, fmt, __FUNCTION__, ## __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

extern RK_U32 mpp_debug;
void mpp_llog(int level, const char *tag, const char *fmt, const char *func, ...);

#ifdef __cplusplus
}
#endif

#endif /* MPP_DEBUG_H */
