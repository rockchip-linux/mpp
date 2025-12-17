/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_avsd_base"

#include "mpp_debug.h"
#include "mpp_common.h"

#include "hal_avsd_base.h"

RK_U32 avsd_hal_debug = 0;

RK_S32 get_queue_pic(AvsdHalCtx_t *p_hal)
{
    RK_U32 i = 0;
    RK_S32 ret_idx = -1;

    for (i = 0; i < MPP_ARRAY_ELEMS(p_hal->pic); i++) {
        if (!p_hal->pic[i].valid) {
            ret_idx = i;
            p_hal->pic[i].valid = 1;
            break;
        }
    }

    return ret_idx;
}

RK_S32 get_packet_fd(AvsdHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->packet_slots, idx, SLOT_BUFFER, &mbuffer);
    mpp_assert(mbuffer);
    ret_fd =  mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}

RK_S32 get_frame_fd(AvsdHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->frame_slots, idx, SLOT_BUFFER, &mbuffer);
    mpp_assert(mbuffer);
    ret_fd = mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}
