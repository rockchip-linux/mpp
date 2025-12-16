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

#ifndef HAL_VP9D_CTX_H
#define HAL_VP9D_CTX_H

#include "mpp_device.h"
#include "mpp_hal.h"
#include "hal_bufs.h"
#include "vdpu_com.h"

#define VP9_CONTEXT 4

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
    RK_U8       color_space_last;
    RK_U8       last_frame_type;
} Vp9dLastInfo;

typedef struct Vp9dRegBuf_t {
    RK_S32      use_flag;
    MppBuffer   global_base;
    MppBuffer   probe_base;
    MppBuffer   count_base;
    MppBuffer   segid_cur_base;
    MppBuffer   segid_last_base;
    void        *hw_regs;
    MppBuffer   rcb_buf;
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

    const MppDecHwCap   *hw_info;
} HalVp9dCtx;

typedef struct Vdpu38xVp9dCtx_t {
    Vp9dRegBuf      g_buf[VDPU_FAST_REG_SET_CNT];
    MppBuffer       global_base;
    MppBuffer       probe_base;
    MppBuffer       count_base;
    MppBuffer       segid_cur_base;
    MppBuffer       segid_last_base;
    MppBuffer       prob_default_base;
    void*           hw_regs;
    RK_S32          mv_base_addr;
    RK_S32          pre_mv_base_addr;
    Vp9dLastInfo    ls_info;
    /*
     * swap between segid_cur_base & segid_last_base
     * 0  used segid_cur_base as last
     * 1  used segid_last_base as
     */
    RK_U32          last_segid_flag;
    RK_S32          width;
    RK_S32          height;
    /* rcb buffers info */
    void            *rcb_ctx;
    RK_U32          rcb_buf_size;
    VdpuRcbInfo     rcb_info[RCB_BUF_CNT];
    MppBuffer       rcb_buf;
    RK_U32          num_row_tiles;
    RK_U32          bit_depth;
    /* colmv buffers info */
    HalBufs         cmv_bufs;
    RK_S32          mv_size;
    RK_S32          mv_count;
    HalBufs         origin_bufs;
    RK_U32          prob_ctx_valid[VP9_CONTEXT];
    MppBuffer       prob_loop_base[VP9_CONTEXT];
    /* uncompress header data */
    RK_U8           header_data[0];
} Vdpu38xVp9dCtx;

#endif /* HAL_VP9D_CTX_H */

