/*
*
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


#ifndef __HAL_H264D_GLOBAL_H__
#define __HAL_H264D_GLOBAL_H__

#include "mpp_hal.h"
#include "mpp_log.h"
#include "mpp_device.h"

#include "dxva_syntax.h"
#include "h264d_syntax.h"



#define H264D_DBG_ERROR             (0x00000001)
#define H264D_DBG_ASSERT            (0x00000002)
#define H264D_DBG_WARNNING          (0x00000004)
#define H264D_DBG_LOG               (0x00000008)

#define H264D_DBG_HARD_MODE         (0x00000010)

extern RK_U32 rkv_h264d_hal_debug;


#define H264D_DBG(level, fmt, ...)\
do {\
    if (level & rkv_h264d_hal_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


#define H264D_ERR(fmt, ...)\
do {\
    if (H264D_DBG_ERROR & rkv_h264d_hal_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define ASSERT(val)\
do {\
    if (H264D_DBG_ASSERT & rkv_h264d_hal_debug)\
        { mpp_assert(val); }\
} while (0)

#define H264D_WARNNING(fmt, ...)\
do {\
    if (H264D_DBG_WARNNING & rkv_h264d_hal_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define H264D_LOG(fmt, ...)\
do {\
    if (H264D_DBG_LOG & rkv_h264d_hal_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


//!< vaule check
#define VAL_CHECK(ret, val, ...)\
do{\
    if (!(val)){\
        ret = MPP_ERR_VALUE; \
        H264D_WARNNING("value error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)
//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        H264D_ERR("malloc buffer error(%d).\n", __LINE__); \
        ASSERT(0); goto __FAILED; \
}} while (0)


//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        H264D_WARNNING("input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)
//!< function return check
#define FUN_CHECK(val)\
do{\
    if ((val) < 0) {\
        H264D_WARNNING("Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)


typedef struct h264d_hal_ctx_t {
    MppHalApi                hal_api;

    DXVA_PicParams_H264_MVC  *pp;
    DXVA_Qmatrix_H264        *qm;
    RK_U32                   slice_num;
    DXVA_Slice_H264_Short    *slice_short;  //!<  MAX_SLICES
    DXVA_Slice_H264_Long     *slice_long;   //!<  MAX_SLICES
    RK_U8                    *bitstream;
    RK_U32                   strm_len;

    void                     *priv;       //!< resert data for extent
    //!< add
    HalDecTask               *in_task;
    MppBufSlots              frame_slots;
    MppBufSlots              packet_slots;
    MppBufferGroup           buf_group;

    IOInterruptCB            init_cb;
    MppDevCtx                dev_ctx;
    void                     *reg_ctx;
    RK_U32                   fast_mode;
} H264dHalCtx_t;



#endif /*__HAL_H264D_GLOBAL_H__*/
