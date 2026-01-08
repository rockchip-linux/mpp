/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#ifndef AVS2D_DEBUG_H
#define AVS2D_DEBUG_H

#include "mpp_debug.h"

#define AVS2D_DBG_ERROR             (0x00000001)
#define AVS2D_DBG_ASSERT            (0x00000002)
#define AVS2D_DBG_WARNNING          (0x00000004)
#define AVS2D_DBG_LOG               (0x00000008)

#define AVS2D_DBG_INPUT             (0x00000010)   //!< input packet
#define AVS2D_DBG_TIME              (0x00000020)   //!< input packet
#define AVS2D_DBG_DPB               (0x00000040)

#define AVS2D_DBG_CALLBACK          (0x00008000)

extern RK_U32 avs2d_debug;

#define AVS2D_PARSE_TRACE(fmt, ...)\
do {\
    if (AVS2D_DBG_LOG & avs2d_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)

#define avs2d_dbg_dpb(fmt, ...)\
do {\
    if (AVS2D_DBG_DPB & avs2d_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define AVS2D_DBG(level, fmt, ...)\
do {\
    if (level & avs2d_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

//!< input check
#define INP_CHECK(ret, val, ...)\
do {\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        AVS2D_DBG(AVS2D_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)

//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do {\
    if(!(val)) {\
        ret = MPP_ERR_MALLOC;\
        mpp_err_f("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

//!< function return check
#define FUN_CHECK(val)\
do {\
    if ((val) < 0) {\
        AVS2D_DBG(AVS2D_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

#endif /* AVS2D_DEBUG_H */
