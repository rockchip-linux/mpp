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

#ifndef __MPP_DEC_H__
#define __MPP_DEC_H__

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
 */

typedef void* MppBufSlots;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * called by mpp context
 */
MPP_RET mpp_buf_slot_init(MppBufSlots *slots);
MPP_RET mpp_buf_slot_deinit(MppBufSlots slots);

/*
 * called by parser
 *
 * mpp_buf_slot_get_unused
 *      - parser need a new slot ffor output, on field mode alloc one buffer for two field
 *
 * mpp_buf_slot_set_ref
 *      - mark a slot to be used as reference
 *
 * mpp_buf_slot_set_unref
 *      - mark a slot to be unused as reference
 *
 * mpp_buf_slot_set_output
 *      - mark a slot to be output destination buffer
 *
 * mpp_buf_slot_set_display
 *      - mark a slot to be can be display
 *
 */
MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_U32 *index);
MPP_RET mpp_buf_slot_set_ref(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_set_unref(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_set_output(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_set_display(MppBufSlots slots, RK_U32 index);

/*
 * called by hal
 *
 * mpp_buf_slot_set_hw_ready
 *      - mark a slot's buffer is already decoded by hardware
 *        NOTE: this call will clear used as output flag
 *
 * called by mpp context display loop
 *
 * mpp_buf_slot_set_unused
 *      - mark a slot's buffer is unused when this buffer is outputed from mpp
 */
MPP_RET mpp_buf_slot_set_hw_ready(MppBufSlots slots, RK_U32 index);
MPP_RET mpp_buf_slot_set_unused(MppBufSlots slots, RK_U32 index);

MPP_RET mpp_buf_slot_set_buffer(MppBufSlots slots, RK_U32 index, MppBuffer buffer);
MppBuffer mpp_buf_slot_get_buffer(const MppBufSlots slots, RK_U32 index);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_DEC_H__*/
