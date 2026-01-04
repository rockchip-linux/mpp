/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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


#ifndef HAL_AV1D_GLOBAL_H
#define HAL_AV1D_GLOBAL_H

#include "mpp_hal.h"
#include "mpp_debug.h"
#include "mpp_device.h"
#include "hal_bufs.h"

#include "dxva_syntax.h"

#define AV1D_DBG_ERROR             (0x00000001)
#define AV1D_DBG_ASSERT            (0x00000002)
#define AV1D_DBG_WARNNING          (0x00000004)
#define AV1D_DBG_LOG               (0x00000008)

#define AV1D_DBG_HARD_MODE         (0x00000010)
#define AV1D_DBG_REG               (0x00000020)

extern RK_U32 hal_av1d_debug;

#define AV1D_DBG(level, fmt, ...)\
do {\
    if (level & hal_av1d_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


#define AV1D_ERR(fmt, ...)\
do {\
    if (AV1D_DBG_ERROR & hal_av1d_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define ASSERT(val)\
do {\
    if (AV1D_DBG_ASSERT & hal_av1d_debug)\
        { mpp_assert(val); }\
} while (0)

#define AV1D_WARNNING(fmt, ...)\
do {\
    if (AV1D_DBG_WARNNING & hal_av1d_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define AV1D_LOG(fmt, ...)\
do {\
    if (AV1D_DBG_LOG & hal_av1d_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


//!< vaule check
#define VAL_CHECK(ret, val, ...)\
do{\
    if (!(val)){\
        ret = MPP_ERR_VALUE; \
        AV1D_WARNNING("value error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)
//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        AV1D_ERR("malloc buffer error(%d).\n", __LINE__); \
        ASSERT(0); goto __FAILED; \
}} while (0)

#define BUF_CHECK(ret, val, ...)\
do{\
    if (val) {\
        ret = MPP_ERR_BUFFER_FULL; \
        AV1D_ERR("buffer get error(%d).\n", __LINE__); \
        ASSERT(0); goto __FAILED; \
}} while (0)

#define BUF_PUT(val, ...)\
do{\
    if (val) {\
        if (mpp_buffer_put(val)) {\
            AV1D_ERR("buffer put error(%d).\n", __LINE__); \
            ASSERT(0); \
        }\
}} while (0)

//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        AV1D_WARNNING("input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)
//!< function return check
#define FUN_CHECK(val)\
do{\
    if ((val) < 0) {\
        AV1D_WARNNING("Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

#define AV1DEC_MAX_PIC_BUFFERS 24

typedef struct av1d_hal_ctx_t {
    RK_U8                   *bitstream;
    RK_U32                  strm_len;

    HalDecTask              *in_task;
    MppHalCfg               *cfg;

    void                    *reg_ctx;
    RK_U32                  fast_mode;
} Av1dHalCtx;

#endif /* HAL_AV1D_GLOBAL_H */
