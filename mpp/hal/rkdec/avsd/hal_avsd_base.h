/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef HAL_AVSD_BASE_H
#define HAL_AVSD_BASE_H

#include "parser_api.h"
#include "avsd_syntax.h"
#include "mpp_hal.h"
#include "mpp_env.h"

#include "mpp_device.h"

#define AVSD_HAL_DBG_ERROR             (0x00000001)
#define AVSD_HAL_DBG_ASSERT            (0x00000002)
#define AVSD_HAL_DBG_WARNNING          (0x00000004)
#define AVSD_HAL_DBG_TRACE             (0x00000008)

#define AVSD_HAL_DBG_OFFSET            (0x00000010)
#define AVSD_HAL_DBG_WAIT              (0x00000020)

#define AVSD_DBG_HARD_MODE             (0x00010000)
#define AVSD_DBG_PROFILE_ID            (0x00020000)

extern RK_U32 hal_avsd_debug;

#define AVSD_HAL_DBG(level, fmt, ...)\
do {\
    if (level & hal_avsd_debug)\
    { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define AVSD_HAL_TRACE(fmt, ...)\
do {\
    if (AVSD_HAL_DBG_TRACE & hal_avsd_debug)\
    { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {    \
        ret = MPP_ERR_INIT; \
        AVSD_HAL_DBG(AVSD_HAL_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
    }\
} while (0)


#define FUN_CHECK(val)\
do{\
    if ((val) < 0) {\
        AVSD_HAL_DBG(AVSD_HAL_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
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

typedef struct avsd_hal_picture_t {
    RK_U32 valid;
    RK_U32 pic_type;
    RK_U32 pic_code_type;
    RK_U32 picture_distance;

    RK_S32 slot_idx;
} AvsdHalPic_t;


typedef struct avsd_hal_ctx_t {
    MppCodingType            coding;

    MppHalCfg                *cfg;

    AvsdSyntax_t             syn;
    RK_U32                   *p_regs;
    RK_U32                   regs_num;
    MppBuffer                mv_buf;

    AvsdHalPic_t             pic[3];
    //!< add for control
    RK_U32                   first_field;
    RK_U32                   prev_pic_structure;
    RK_U32                   prev_pic_code_type;
    RK_S32                   future2prev_past_dist;
    RK_S32                   work0;
    RK_S32                   work1;
    RK_S32                   work_out;
    RK_U32                   data_offset;

    RK_U32                   frame_no;
} AvsdHalCtx_t;


#ifdef __cplusplus
extern "C" {
#endif

RK_S32 get_queue_pic(AvsdHalCtx_t *p_hal);
RK_S32 get_packet_fd(AvsdHalCtx_t *p_hal, RK_S32 idx);
RK_S32 get_frame_fd(AvsdHalCtx_t *p_hal, RK_S32 idx);

#ifdef __cplusplus
}
#endif

#endif /* HAL_AVSD_COMMON_H */
