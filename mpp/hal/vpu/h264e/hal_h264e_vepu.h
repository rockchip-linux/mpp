/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H264E_VEPU_H__
#define __HAL_H264E_VEPU_H__

#include "hal_h264e_com.h"
#include "h264e_stream.h"

#define HAL_VPU_H264E_DBG_FUNCTION          (0x00000001)
#define HAL_VPU_H264E_DBG_QP                (0x00000010)

#define hal_vpu_h264e_dbg(flag, fmt, ...)   \
    _mpp_dbg(hal_vpu_h264e_debug, flag, fmt, ## __VA_ARGS__)
#define hal_vpu_h264e_dbg_f(flag, fmt, ...) \
    _mpp_dbg_f(hal_vpu_h264e_debug, flag, fmt, ## __VA_ARGS__)

#define hal_vpu_h264e_dbg_func(fmt, ...)    \
    hal_vpu_h264e_dbg_f(HAL_VPU_H264E_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define hal_vpu_h264e_dbg_qp(fmt, ...)      \
    hal_vpu_h264e_dbg(HAL_VPU_H264E_DBG_QP, fmt, ## __VA_ARGS__)

#define BIT(n)  (1<<(n))

#define H264E_CABAC_TABLE_BUF_SIZE          (52*2*464)

typedef enum H264eVpuFrameType_t {
    H264E_VPU_FRAME_P = 0,
    H264E_VPU_FRAME_I = 1
} H264eVpuFrameType;

typedef struct H264eVpuExtraInfo_t {
    H264eStream sps_stream;
    H264eStream pps_stream;
    H264eStream sei_stream;
    H264eSps sps;
    H264ePps pps;
    H264eSei sei;
    RK_U8 *sei_buf;
    RK_U32 sei_change_flg;
} H264eVpuExtraInfo;

typedef struct h264e_hal_vpu_buffers_t {
    RK_S32 cabac_init_idc;
    RK_S32 align_width;
    RK_S32 align_height;

    MppBufferGroup hw_buf_grp;
    MppBuffer hw_rec_buf[2];
    MppBuffer hw_cabac_table_buf;
    MppBuffer hw_nal_size_table_buf;
} h264e_hal_vpu_buffers;

#endif
