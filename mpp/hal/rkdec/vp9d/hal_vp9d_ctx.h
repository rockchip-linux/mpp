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

#ifndef __HAL_VP9D_CTX_H__
#define __HAL_VP9D_CTX_H__

#include "mpp_device.h"
#include "mpp_hal.h"
#include "hal_bufs.h"
#include "vdpu34x_com.h"

#define MAX_GEN_REG 3

typedef struct Vp9dLastInfo_t {
    RK_S32      abs_delta_last;
    RK_S8       last_ref_deltas[4];
    RK_S8       last_mode_deltas[2];
    RK_U8       segmentation_enable_flag_last;
    RK_U8       last_show_frame;
    RK_U8       last_intra_only;
    RK_U32      last_width;
    RK_U32      last_height;
    RK_S16      feature_data[8][4];
    RK_U8       feature_mask[8];
} Vp9dLastInfo;

typedef struct Vp9dRegBuf_t {
    RK_S32      use_flag;
    MppBuffer   probe_base;
    MppBuffer   count_base;
    MppBuffer   segid_cur_base;
    MppBuffer   segid_last_base;
    void        *hw_regs;
} Vp9dRegBuf;

typedef struct HalVp9dCtx_t {
    /* for hal api call back */
    const MppHalApi *api;

    /* for hardware info */
    MppClientType   client_type;
    RK_U32          hw_id;
    MppDev          dev;

    MppBufSlots     slots;
    MppBufSlots     packet_slots;
    MppBufferGroup  group;
    MppCbCtx        *dec_cb;
    RK_U32          fast_mode;
    void*           hw_ctx;
} HalVp9dCtx;

#endif /*__HAL_VP9D_CTX_H__*/
