/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_BUF_SLOT_H__
#define __MPP_BUF_SLOT_H__

#include "rk_type.h"
#include "mpp_buffer.h"

/*
 * mpp_dec will alloc 18 decoded picture buffer slot
 * buffer slot is for transferring information between parser / mpp/ hal
 * it represent the dpb routine in logical
 *
 * basic working flow:
 *
 *  buf_slot                      parser                         hal
 *
 *      +                            +                            +
 *      |                            |                            |
 *      |                   +--------+--------+                   |
 *      |                   |                 |                   |
 *      |                   | do parsing here |                   |
 *      |                   |                 |                   |
 *      |                   +--------+--------+                   |
 *      |                            |                            |
 *      |          get_slot          |                            |
 *      | <--------------------------+                            |
 *      |  get unused dpb slot for   |                            |
 *      |  current decoder output    |                            |
 *      |                            |                            |
 *      |  update dpb refer status   |                            |
 *      | <--------------------------+                            |
 *      |  parser will send marking  |                            |
 *      |  operation to dpb slot     |                            |
 *      |  including:                |                            |
 *      |  ref/unref/output/display  |                            |
 *      |                            |                            |
 *      |                            |                            |
 *      |                            |  set buffer status to hal  |
 *      +-------------------------------------------------------> |
 *      |                            |                            |
 *      |                            |                   +--------+--------+
 *      |                            |                   |                 |
 *      |                            |                   | reg generation  |
 *      |                            |                   |                 |
 *      |                            |                   +--------+--------+
 *      |                            |                            |
 *      |                            |  get buffer address info   |
 *      | <-------------------------------------------------------+
 *      |                            |  used buffer index to get  |
 *      |                            |  physical address or iommu |
 *      |                            |  address for hardware      |
 *      |                            |                            |
 *      |                            |                   +--------+--------+
 *      |                            |                   |                 |
 *      |                            |                   |   set/wait hw   |
 *      |                            |                   |                 |
 *      |                            |                   +--------+--------+
 *      |                            |                            |
 *      |                            |  update the output status  |
 *      | <-------------------------------------------------------+
 *      |                            |  mark picture is available |
 *      |                            |  for output and generate   |
 *      |                            |  output frame information  |
 *      +                            +                            +
 *
 * typical buffer status transfer
 *
 * ->   unused                  initial
 * ->   set_decoding            by parser
 * ->   set_buffer              by mpp - do alloc buffer here / info change here
 * ->   clr_decoding            by hal()
 *
 * next four step can be different order
 * ->   set_ref                 by parser
 * ->   set_display             by parser - slot ready to display, can be output
 * ->   clr_display             by mpp - output buffer struct
 * ->   clr_ref                 by parser
 *
 * ->   set_unused              automatic clear and dec buffer ref
 *
 */

typedef void* MppBufSlots;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * called by mpp context
 *
 * init / deinit - normal initialize and de-initialize function
 * setup         - called by parser when slot information changed
 * is_changed    - called by mpp to detect whether info change flow is needed
 * ready         - called by mpp when info changed is done
 *
 * typical info change flow:
 *
 * mpp_buf_slot_setup           called in parser with changed equal to 1
 * mpp_buf_slot_is_changed      called in mpp and found info change
 *
 * do info change outside
 *
 * mpp_buf_slot_ready           called in mpp when info change is done
 *
 */
MPP_RET mpp_buf_slot_init(MppBufSlots *slots);
MPP_RET mpp_buf_slot_deinit(MppBufSlots slots);
MPP_RET mpp_buf_slot_setup(MppBufSlots slots, RK_U32 count, RK_U32 size, RK_U32 changed);
RK_U32  mpp_buf_slot_is_changed(MppBufSlots slots);
MPP_RET mpp_buf_slot_ready(MppBufSlots slots);
RK_U32  mpp_buf_slot_get_size(MppBufSlots slots);

/*
 * called by parser
 *
 * mpp_buf_slot_get_unused
 *      - parser need a new slot ffor output, on field mode alloc one buffer for two field
 *
 * mpp_buf_slot_set_ref
 *      - mark a slot to be used as reference
 *
 * mpp_buf_slot_clr_ref
 *      - mark a slot to be unused as reference
 *
 * mpp_buf_slot_set_decoding
 *      - mark a slot to be output destination buffer
 *
 * mpp_buf_slot_set_display
 *      - mark a slot to be can be display
 *
 * called by mpp
 *
 * mpp_buf_slot_clr_display
 *      - mark a slot has been send out to display
 *
 * called by hal
 *
 * mpp_buf_slot_clr_decoding
 *      - mark a slot's buffer is already decoded by hardware
 *        NOTE: this call will clear used as output flag
 */
MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_U32 *index);
MPP_RET mpp_buf_slot_set_ref(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_clr_ref(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_set_decoding(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_clr_decoding(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_set_display(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_clr_display(MppBufSlots slots, RK_U32 index);

/*
 * mpp_buf_slot_set_buffer
 *      - called by dec thread when find a output index has not buffer
 *
 * mpp_buf_slot_get_buffer
 *      - called by hal module on register generation
 *
 * mpp_buf_slot_set_pts
 *      - called by parser when decoding a new frame
 *
 * mpp_buf_slot_get_pts
 *      - called by hal thread when output a frame
 */
MPP_RET     mpp_buf_slot_set_buffer(MppBufSlots slots, RK_U32 index, MppBuffer buffer);
MppBuffer   mpp_buf_slot_get_buffer(const MppBufSlots slots, RK_U32 index);
MPP_RET     mpp_buf_slot_set_pts(MppBufSlots slots, RK_U32 index, RK_S64 pts);
RK_S64      mpp_buf_slot_get_pts(const MppBufSlots slots, RK_U32 index);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_BUF_SLOT_H__*/
