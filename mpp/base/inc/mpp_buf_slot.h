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

#ifndef __MPP_BUF_SLOT_H__
#define __MPP_BUF_SLOT_H__

#include "mpp_frame.h"

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
 * ->   set_hw_dst              by parser
 * ->   set_buffer              by mpp - do alloc buffer here / info change here
 * ->   clr_hw_dst              by hal()
 *
 * next four step can be different order
 * ->   set_dpb_ref             by parser
 * ->   set_display             by parser - slot ready to display, can be output
 * ->   clr_display             by mpp - output buffer struct
 * ->   clr_dpb_ref             by parser
 *
 * ->   set_unused              automatic clear and dec buffer ref
 *
 */

typedef void* MppBufSlots;

/*
 * buffer slot index range is 0~254, 0xff (255) will indicated the invalid index
 */
#define SLOT_IDX_BUTT                   (0xff)

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
MPP_RET mpp_buf_slot_setup(MppBufSlots slots, RK_S32 count);
RK_U32  mpp_buf_slot_is_changed(MppBufSlots slots);
MPP_RET mpp_buf_slot_ready(MppBufSlots slots);
size_t  mpp_buf_slot_get_size(MppBufSlots slots);
/*
 * called by parser
 *
 * mpp_buf_slot_get_unused
 *      - parser need a new slot for output, on field mode alloc one buffer for two field
 *
 * mpp_buf_slot_set_dpb_ref
 *      - mark a slot to be used as reference frame in dpb
 *
 * mpp_buf_slot_clr_dpb_ref
 *      - mark a slot to be unused as reference frame and remove from dpb
 *
 * mpp_buf_slot_set_hw_dst
 *      - mark a slot to be output destination buffer
 *      - NOTE: the frame information MUST be set here
 *
 * mpp_buf_slot_set_display
 *      - mark a slot to be can be display
 *      - NOTE: set display will generate a MppFrame for buffer slot internal usage
 *              for example store pts / buffer address, etc.
 *
 * mpp_buf_slot_inc_hw_ref
 *      - MUST be called once when one slot is used in hardware decoding as reference frame
 *
 * called by mpp
 *
 * mpp_buf_slot_get_hw_dst
 *      - mpp_dec need to get the output slot index to check buffer status
 *
 * mpp_buf_slot_clr_display
 *      - mark a slot has been send out to display
 *      - NOTE: will be called inside mpp_buf_slot_get_display
 *
 * called by hal
 *
 * mpp_buf_slot_clr_hw_dst
 *      - mark a slot's buffer is already decoded by hardware
 *      - NOTE: this call will clear used as output flag
 *
 * mpp_buf_slot_dec_hw_ref
 *      - when hal finished on hardware decoding it MUST be called once for each used slot
 */
MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_S32 *index);

/*
 * mpp_buf_slot_set_buffer
 *      - called by dec thread when find a output index has not buffer
 *
 * mpp_buf_slot_get_buffer
 *      - called by hal module on register generation
 *
 * mpp_buf_slot_get_display
 *      - called by hal thread to output a display slot's frame info
 *        NOTE: get display will generate a new MppFrame for external mpp_frame_deinit call
 *              So that external mpp_frame_deinit will not release the MppFrame used in buf_slot
 */

/*
 * NOTE:
 * buffer slot will be used both for frame and packet
 * when buffer slot is used for packet management only inc_hw_ref and dec_hw_ref is used
 */

typedef enum SlotUsageType_e {
    SLOT_CODEC_READY,       // bit flag             for buffer is prepared by codec
    SLOT_CODEC_USE,         // bit flag             for buffer is used as reference by codec
    SLOT_HAL_INPUT,         // counter              for buffer is used as hardware input
    SLOT_HAL_OUTPUT,        // counter + bit flag   for buffer is used as hardware output
    SLOT_QUEUE_USE,         // bit flag             for buffer is hold in different queues
    SLOT_USAGE_BUTT,
} SlotUsageType;

MPP_RET mpp_buf_slot_set_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type);
MPP_RET mpp_buf_slot_clr_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type);

// TODO: can be extended here
typedef enum SlotQueueType_e {
    QUEUE_OUTPUT,           // queue for mpp output to user
    QUEUE_DISPLAY,          // queue for decoder output display
    QUEUE_DEINTERLACE,      // queue for deinterlace process
    QUEUE_COLOR_CONVERT,    // queue for color convertion process
    QUEUE_BUTT,
} SlotQueueType;

MPP_RET mpp_buf_slot_enqueue(MppBufSlots slots, RK_S32  index, SlotQueueType type);
MPP_RET mpp_buf_slot_dequeue(MppBufSlots slots, RK_S32 *index, SlotQueueType type);

typedef enum SlotPropType_e {
    SLOT_EOS,
    SLOT_FRAME,
    SLOT_BUFFER,
    SLOT_FRAME_PTR,
    SLOT_PROP_BUTT,
} SlotPropType;

MPP_RET mpp_buf_slot_set_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val);
MPP_RET mpp_buf_slot_get_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val);

typedef enum SlotsPropType_e {
    SLOTS_EOS,
    SLOTS_NUMERATOR,            // numerator of buffer size scale ratio
    SLOTS_DENOMINATOR,          // denominator of buffer size scale ratio
    SLOTS_HOR_ALIGN,            // input must be buf_align function pointer
    SLOTS_VER_ALIGN,            // input must be buf_align function pointer
    SLOTS_LEN_ALIGN,
    SLOTS_COUNT,
    SLOTS_SIZE,
    SLOTS_FRAME_INFO,
    SLOTS_PROP_BUTT,
} SlotsPropType;

typedef RK_U32 (*AlignFunc)(RK_U32 val);

RK_U32 mpp_slots_is_empty(MppBufSlots slots, SlotQueueType type);
RK_S32  mpp_slots_get_used_count(MppBufSlots slots);
RK_S32  mpp_slots_get_unused_count(MppBufSlots slots);
MPP_RET mpp_slots_set_prop(MppBufSlots slots, SlotsPropType type, void *val);
MPP_RET mpp_slots_get_prop(MppBufSlots slots, SlotsPropType type, void *val);
MPP_RET mpp_buf_slot_reset(MppBufSlots slots, RK_S32 index); //rest slot status when info_change no ok

// special one for generate default frame to slot at index and return pointer
MPP_RET mpp_buf_slot_default_info(MppBufSlots slots, RK_S32 index, void *val);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_BUF_SLOT_H__*/
