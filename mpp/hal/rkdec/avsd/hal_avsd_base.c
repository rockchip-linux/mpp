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

#define MODULE_TAG "hal_avsd_base"
#include <assert.h>
#include "mpp_log.h"
#include "mpp_common.h"

#include "hal_avsd_base.h"


RK_U32 avsd_hal_debug = 0;

RK_U32 avsd_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

RK_U32 avsd_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

RK_U32 avsd_len_align(RK_U32 val)
{
    return (2 * MPP_ALIGN(val, 16));
}

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
    assert(mbuffer);
    ret_fd =  mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}

RK_S32 get_frame_fd(AvsdHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->frame_slots, idx, SLOT_BUFFER, &mbuffer);
    assert(mbuffer);
    ret_fd = mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}
