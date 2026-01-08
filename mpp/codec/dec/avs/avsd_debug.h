/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef AVSD_DEBUG_H
#define AVSD_DEBUG_H

#include "mpp_debug.h"

#define AVSD_DBG_ERROR             (0x00000001)
#define AVSD_DBG_ASSERT            (0x00000002)
#define AVSD_DBG_WARNNING          (0x00000004)
#define AVSD_DBG_LOG               (0x00000008)

#define AVSD_DBG_INPUT             (0x00000010)
#define AVSD_DBG_TIME              (0x00000020)
#define AVSD_DBG_SYNTAX            (0x00000040)
#define AVSD_DBG_REF               (0x00000080)

#define AVSD_DBG_CALLBACK          (0x00008000)

#define AVSD_PARSE_TRACE(fmt, ...)\
do {\
    if (AVSD_DBG_LOG & avsd_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)

#define AVSD_DBG(level, fmt, ...)\
do {\
    if (level & avsd_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        AVSD_DBG(AVSD_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)

//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if(!(val)) {\
        ret = MPP_ERR_MALLOC;\
        mpp_err_f("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

//!< function return check
#define FUN_CHECK(val)\
do{\
if ((val) < 0) {\
        AVSD_DBG(AVSD_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

extern RK_U32 avsd_debug;

#endif /* AVSD_DEBUG_H */
