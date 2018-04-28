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

#ifndef __HAL_M4VD_COM_H__
#define __HAL_M4VD_COM_H__

#include "mpp_hal.h"
#include "mpp_log.h"
#include "mpp_device.h"

#include "mpg4d_syntax.h"

extern RK_U32 mpg4d_hal_debug;

#define MPEG4_MAX_MV_BUF_SIZE       ((1920/16)*(1088/16)*4*sizeof(RK_U32))

typedef struct mpeg4d_reg_context {
    MppBufSlots         frm_slots;
    MppBufSlots         pkt_slots;
    MppBufferGroup      group;
    IOInterruptCB       int_cb;
    MppDevCtx           dev_ctx;
    // save fd for curr/ref0/ref1 for reg_gen
    RK_S32              fd_curr;
    RK_S32              fd_ref0;
    RK_S32              fd_ref1;
    RK_U32              bitstrm_len;
    // mv info buffer
    // NOTE: mv buffer fix to 1080p size for convenience
    MppBuffer           mv_buf;
    MppBuffer           qp_table;

    void*               regs;
    MppHalApi           hal_api;
} hal_mpg4_ctx;

extern RK_U8 default_inter_matrix[64];
extern RK_U8 default_intra_matrix[64];

extern void vpu_mpg4d_get_buffer_by_index(hal_mpg4_ctx *ctx, RK_S32 index, MppBuffer *buffer);

#endif /*__HAL_M4VD_COM_H__*/
