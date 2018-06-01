/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H263D_BASE_H__
#define __HAL_H263D_BASE_H__

#include "mpp_device.h"

typedef struct h263d_reg_context {
    MppHalApi           hal_api;
    MppBufSlots         frm_slots;
    MppBufSlots         pkt_slots;
    IOInterruptCB       int_cb;
    MppDevCtx           dev_ctx;

    // save fd for curr/ref0/ref1 for reg_gen
    RK_S32              vpu_fd;
    RK_S32              fd_curr;
    RK_S32              fd_ref0;

    void*   regs;
} hal_h263_ctx;

void vpu_h263d_get_buffer_by_index(hal_h263_ctx *ctx, RK_S32 index, MppBuffer *buffer);

#endif
