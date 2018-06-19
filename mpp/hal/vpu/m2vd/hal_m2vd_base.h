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

#ifndef __HAL_M2VD_BASE_H__
#define __HAL_M2VD_BASE_H__

#include <stdio.h>
#include "mpp_hal.h"
#include "mpp_buf_slot.h"
#include "mpp_device.h"
#include "mpp_mem.h"
#include "m2vd_syntax.h"

#define M2VD_BUF_SIZE_QPTAB            (256)
#define DEC_LITTLE_ENDIAN                     1
#define DEC_BIG_ENDIAN                        0
#define DEC_BUS_BURST_LENGTH_UNDEFINED        0
#define DEC_BUS_BURST_LENGTH_4                5
#define DEC_BUS_BURST_LENGTH_8                8
#define DEC_BUS_BURST_LENGTH_16               16

#define M2VH_DBG_FUNCTION          (0x00000001)
#define M2VH_DBG_REG               (0x00000002)
#define M2VH_DBG_DUMP_REG          (0x00000004)
#define M2VH_DBG_IRQ               (0x00000008)

extern RK_U32 m2vh_debug;

#define m2vh_dbg_func(tag) \
    do {\
        if (M2VH_DBG_FUNCTION & m2vh_debug)\
            { mpp_log("%s: line(%d), func(%s)", tag, __LINE__, __FUNCTION__); }\
    } while (0)

typedef enum M2VDPicCodingType_e {
    M2VD_CODING_TYPE_I = 1,
    M2VD_CODING_TYPE_P = 2,
    M2VD_CODING_TYPE_B = 3,
    M2VD_CODING_TYPE_D = 4
} M2VDPicCodingType;

typedef enum M2VDPicStruct_e {
    M2VD_PIC_STRUCT_TOP_FIELD    = 1,
    M2VD_PIC_STRUCT_BOTTOM_FIELD = 2,
    M2VD_PIC_STRUCT_FRAME        = 3
} M2VDPicStruct;

typedef struct M2vdHalCtx_t {
    MppHalApi        hal_api;
    MppBufSlots      packet_slots;
    MppBufSlots      frame_slots;
    void             *regs;
    MppBufferGroup   group;
    MppBuffer        qp_table;
    RK_U32           dec_frame_cnt;
    IOInterruptCB    int_cb;
    MppDevCtx        dev_ctx;
    FILE             *fp_reg_in;
    FILE             *fp_reg_out;
    RK_U32           reg_len;
} M2vdHalCtx;

#endif // __HAL_M2VD_BASE_H__
