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

#define  MODULE_TAG "mpp_buf_slot"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_thread.h"

#include "mpp_buf_slot.h"

#define MPP_SLOT_UNUSED                 (0x00000000)
#define MPP_SLOT_USED                   (0x00000001)
#define MPP_SLOT_USED_AS_REF            (0x00000002)
#define MPP_SLOT_USED_AS_OUTPUT         (0x00000004)
#define MPP_SLOT_USED_AS_DISPLAY        (0x00000008)
#define MPP_SLOT_HW_READY               (0x00000010)

typedef struct {
    MppBuffer   buffer;
    RK_U32      status;
} MppBufSlotEntry;

typedef struct {
    Mutex           *lock;
    RK_U32          count;
    RK_U32          decode_count;
    RK_U32          display_count;
    MppBufSlotEntry *slots;
} MppBufSlotsImpl;

MPP_RET mpp_buf_slot_init(MppBufSlots *slots, RK_U32 count)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }
    MppBufSlotsImpl *impl = mpp_malloc_size(MppBufSlotsImpl,
                                  sizeof(MppBufSlotsImpl) + sizeof(MppBufSlotEntry) * count);
    if (NULL == impl) {
        *slots = NULL;
        return MPP_NOK;
    }

    impl->lock = new Mutex();
    impl->count = count;
    impl->decode_count = 0;
    impl->display_count = 0;

    impl->slots = (MppBufSlotEntry *)(impl + 1);
    memset(impl->slots, 0, sizeof(MppBufSlotEntry) * count);
    *slots = impl;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_deinit(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    delete impl->lock;
    mpp_free(slots);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_U32 *index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    RK_U32 i;
    MppBufSlotEntry *slot = impl->slots;
    Mutex::Autolock auto_lock(impl->lock);
    for (i = 0; i < impl->count; i++, slot++) {
        if (MPP_SLOT_UNUSED == slot->status) {
            *index = i;
            slot->status |= MPP_SLOT_USED;
            return MPP_OK;
        }
    }

    *index = -1;
    mpp_err("mpp_buf_slot_get_unused failed to get a unused slot\n");
    mpp_assert(0);
    return MPP_NOK;
}

MPP_RET mpp_buf_slot_set_ref(MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].status |= MPP_SLOT_USED_AS_REF;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_unref(MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].status &= ~MPP_SLOT_USED_AS_REF;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_output(MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].status |= MPP_SLOT_USED_AS_OUTPUT;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_display(MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].status |= MPP_SLOT_USED_AS_DISPLAY;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_hw_ready(MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].status |= MPP_SLOT_HW_READY;
    slot[index].status &= ~MPP_SLOT_USED_AS_OUTPUT;
    impl->decode_count++;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_unused(MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].status = MPP_SLOT_UNUSED;
    impl->display_count++;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_buffer(MppBufSlots slots, RK_U32 index, MppBuffer buffer)
{
    if (NULL == slots || NULL == buffer) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    mpp_assert(index < impl->count);
    Mutex::Autolock auto_lock(impl->lock);
    slot[index].buffer = buffer;
    return MPP_OK;
}

MppBuffer mpp_buf_slot_get_buffer(const MppBufSlots slots, RK_U32 index)
{
    if (NULL == slots) {
        mpp_err("%s found NULL input\n\n", __FUNCTION__);
        return NULL;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot = impl->slots;
    Mutex::Autolock auto_lock(impl->lock);
    mpp_assert(index < impl->count);
    return slot[index].buffer;
}

