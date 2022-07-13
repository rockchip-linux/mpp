/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __HAL_AVS2D_GLOBAL_H__
#define __HAL_AVS2D_GLOBAL_H__

#include "mpp_device.h"
#include "hal_bufs.h"

#include "parser_api.h"
#include "hal_avs2d_api.h"
#include "avs2d_syntax.h"

#define AVS2D_HAL_DBG_ERROR             (0x00000001)
#define AVS2D_HAL_DBG_ASSERT            (0x00000002)
#define AVS2D_HAL_DBG_WARNNING          (0x00000004)
#define AVS2D_HAL_DBG_TRACE             (0x00000100)
#define AVS2D_HAL_DBG_REG               (0x00000200)
#define AVS2D_HAL_DBG_IN                (0x00000400)
#define AVS2D_HAL_DBG_OUT               (0x00000800)

#define AVS2D_HAL_DBG_OFFSET            (0x00010000)

extern RK_U32 avs2d_hal_debug;

#define AVS2D_HAL_DBG(level, fmt, ...)\
do {\
    if (level & avs2d_hal_debug)\
    { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define AVS2D_HAL_TRACE(fmt, ...)\
do {\
    if (AVS2D_HAL_DBG_TRACE & avs2d_hal_debug)\
    { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {    \
        ret = MPP_ERR_INIT; \
        AVS2D_HAL_DBG(AVS2D_HAL_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
    }\
} while (0)


#define FUN_CHECK(val)\
do{\
    if ((val) < 0) {\
        AVS2D_HAL_DBG(AVS2D_HAL_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
    }\
} while (0)


//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        mpp_err_f("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
    }\
} while (0)


#define FIELDPICTURE    0
#define FRAMEPICTURE    1

enum {
    IFRAME = 0,
    PFRAME = 1,
    BFRAME = 2
};

typedef struct avs2d_hal_ctx_t {
    MppHalApi               hal_api;
    MppBufSlots             frame_slots;
    MppBufSlots             packet_slots;
    MppBufferGroup          buf_group;
    HalBufs                 cmv_bufs;
    RK_U32                  mv_size;
    RK_U32                  mv_count;
    MppCbCtx                *dec_cb;
    MppDev                  dev;
    Avs2dSyntax_t           syntax;
    RK_U32                  fast_mode;

    void                    *reg_ctx;
    MppBuffer               shph_buf;
    MppBuffer               scalist_buf;

    RK_U32                   frame_no;
} Avs2dHalCtx_t;

#endif /*__HAL_AVS2D_GLOBAL_H__*/
