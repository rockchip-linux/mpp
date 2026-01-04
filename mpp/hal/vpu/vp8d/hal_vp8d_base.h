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

#ifndef HAL_VP8D_BASE_H
#define HAL_VP8D_BASE_H

#include <stdio.h>

#include "mpp_hal.h"
#include "mpp_buf_slot.h"
#include "mpp_device.h"

#include "hal_dec_task.h"

#include "vp8d_syntax.h"

#define VP8H_DBG_FUNCTION          (0x00000001)
#define VP8H_DBG_REG               (0x00000002)
#define VP8H_DBG_DUMP_REG          (0x00000004)
#define VP8H_DBG_IRQ               (0x00000008)

extern RK_U32 hal_vp8d_debug;

#define vp8h_dbg(flag, fmt, ...) \
     mpp_dbg_f(hal_vp8d_debug, flag, fmt, ## __VA_ARGS__)

#define FUN_T(tag) \
    do {\
        if (VP8H_DBG_FUNCTION & hal_vp8d_debug)\
            { mpp_log("%s: line(%d), func(%s)", tag, __LINE__, __FUNCTION__); }\
    } while (0)

typedef struct VP8DHalContext {
    MppHalCfg       *cfg;
    void            *regs;
    RK_U8           reg_size;
    MppBuffer       probe_table;
    MppBuffer       seg_map;
    RK_U32          dec_frame_cnt;
    FILE            *fp_reg_in;
    FILE            *fp_reg_out;
} VP8DHalContext_t;

#endif
