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
#include "mpp_env.h"
#include "mpp_list.h"
#include "mpp_common.h"

#include "mpp_frame_impl.h"
#include "mpp_buf_slot.h"


#define BUF_SLOT_DBG_FUNCTION           (0x00000001)
#define BUF_SLOT_DBG_SETUP              (0x00000002)
#define BUF_SLOT_DBG_OPS_RUNTIME        (0x00000010)
#define BUF_SLOT_DBG_BUFFER             (0x00000100)
#define BUF_SLOT_DBG_FRAME              (0x00000200)
#define BUF_SLOT_DBG_OPS_HISTORY        (0x10000000)

#define buf_slot_dbg(flag, fmt, ...)    _mpp_dbg(buf_slot_debug, flag, fmt, ## __VA_ARGS__)

static RK_U32 buf_slot_debug = 0;

#define slot_assert(impl, cond) do {                                    \
    if (!(cond)) {                                                      \
        dump_slots(impl);                                               \
        mpp_err("Assertion %s failed at %s:%d\n",                       \
               MPP_STRINGS(cond), __FILE__, __LINE__);                  \
        abort();                                                        \
    }                                                                   \
} while (0)

typedef struct MppBufSlotEntry_t MppBufSlotEntry;
typedef struct MppBufSlotsImpl_t MppBufSlotsImpl;

#define SLOT_OPS_MAX_COUNT              1024

typedef enum MppBufSlotOps_e {
    // status opertaion
    SLOT_INIT,
    SLOT_SET_ON_USE,
    SLOT_CLR_ON_USE,
    SLOT_SET_NOT_READY,
    SLOT_CLR_NOT_READY,
    SLOT_SET_CODEC_READY,
    SLOT_CLR_CODEC_READY,
    SLOT_SET_CODEC_USE,
    SLOT_CLR_CODEC_USE,
    SLOT_SET_HAL_INPUT,
    SLOT_CLR_HAL_INPUT,
    SLOT_SET_HAL_OUTPUT,
    SLOT_CLR_HAL_OUTPUT,
    SLOT_SET_QUEUE_USE,
    SLOT_CLR_QUEUE_USE,

    // queue operation
    SLOT_ENQUEUE,
    SLOT_DEQUEUE,

    // value operation
    SLOT_SET_EOS,
    SLOT_CLR_EOS,
    SLOT_SET_FRAME,
    SLOT_CLR_FRAME,
    SLOT_SET_BUFFER,
    SLOT_CLR_BUFFER,
} MppBufSlotOps;

static const char op_string[][16] = {
    "init           ",
    "set on use     ",
    "clr on use     ",
    "set not ready  ",
    "set ready      ",
    "set codec ready",
    "clr codec ready",
    "set codec use  ",
    "clr codec use  ",
    "set hal input  ",
    "clr hal input  ",
    "set hal output ",
    "clr hal output ",
    "set queue use  ",
    "clr queue use  ",

    "enqueue display",
    "dequeue display",

    "set eos        ",
    "clr eos        ",
    "set frame      ",
    "clr frame      ",
    "set buffer     ",
    "clr buffer     ",
};

static const MppBufSlotOps set_flag_op[SLOT_USAGE_BUTT] = {
    SLOT_SET_CODEC_READY,
    SLOT_SET_CODEC_USE,
    SLOT_SET_HAL_INPUT,
    SLOT_SET_HAL_OUTPUT,
    SLOT_SET_QUEUE_USE,
};

static const MppBufSlotOps clr_flag_op[SLOT_USAGE_BUTT] = {
    SLOT_CLR_CODEC_READY,
    SLOT_CLR_CODEC_USE,
    SLOT_CLR_HAL_INPUT,
    SLOT_CLR_HAL_OUTPUT,
    SLOT_CLR_QUEUE_USE,
};

static const MppBufSlotOps set_val_op[SLOT_PROP_BUTT] = {
    SLOT_SET_EOS,
    SLOT_SET_FRAME,
    SLOT_SET_BUFFER,
};

static const MppBufSlotOps clr_val_op[SLOT_PROP_BUTT] = {
    SLOT_CLR_EOS,
    SLOT_CLR_FRAME,
    SLOT_CLR_BUFFER,
};

typedef union SlotStatus_u {
    RK_U32 val;
    struct {
        // status flags
        RK_U32  on_used     : 1;
        RK_U32  not_ready   : 1;        // buffer slot is filled or not
        RK_U32  codec_use   : 1;        // buffer slot is used by codec ( dpb reference )
        RK_U32  hal_output  : 1;        // buffer slot is set to hw output will ready when hw done
        RK_U32  hal_use     : 8;        // buffer slot is used by hardware
        RK_U32  queue_use   : 5;        // buffer slot is used in different queue

        // value flags
        RK_U32  eos         : 1;        // buffer slot is last buffer slot from codec
        RK_U32  has_buffer  : 1;
        RK_U32  has_frame   : 1;
    };
} SlotStatus;

typedef struct MppBufSlotLog_t {
    RK_U32              index;
    MppBufSlotOps       ops;
    SlotStatus          status_in;
    SlotStatus          status_out;
} MppBufSlotLog;

struct MppBufSlotEntry_t {
    MppBufSlotsImpl     *slots;
    struct list_head    list;
    SlotStatus          status;
    RK_U32              index;

    RK_U32              eos;
    MppFrame            frame;
    MppBuffer           buffer;
};

struct MppBufSlotsImpl_t {
    Mutex               *lock;
    RK_U32              count;
    RK_U32              size;

    // status tracing
    RK_U32              decode_count;
    RK_U32              display_count;

    // if slot changed, all will be hold until all slot is unused
    RK_U32              info_changed;
    RK_U32              new_count;
    RK_U32              new_size;

    // list for display
    struct list_head    queue[QUEUE_BUTT];

    // list for log
    mpp_list            *logs;

    MppBufSlotEntry     *slots;
};

static void add_slot_log(mpp_list *logs, RK_U32 index, MppBufSlotOps op, SlotStatus before, SlotStatus after)
{
    if (logs) {
        MppBufSlotLog log = {
            index,
            op,
            before,
            after,
        };
        if (logs->list_size() >= SLOT_OPS_MAX_COUNT)
            logs->del_at_head(NULL, sizeof(log));
        logs->add_at_tail(&log, sizeof(log));
    }
}

static void slot_ops_with_log(mpp_list *logs, MppBufSlotEntry *slot, MppBufSlotOps op)
{
    RK_U32 index  = slot->index;
    SlotStatus status = slot->status;
    SlotStatus before = status;
    switch (op) {
    case SLOT_INIT : {
        status.val = 0;
    } break;
    case SLOT_SET_ON_USE : {
        status.on_used = 1;
    } break;
    case SLOT_CLR_ON_USE : {
        status.on_used = 0;
    } break;
    case SLOT_SET_NOT_READY : {
        status.not_ready = 1;
    } break;
    case SLOT_CLR_NOT_READY : {
        status.not_ready = 0;
    } break;
    case SLOT_SET_CODEC_READY : {
        status.not_ready = 0;
    } break;
    case SLOT_CLR_CODEC_READY : {
        status.not_ready = 1;
    } break;
    case SLOT_SET_CODEC_USE : {
        status.codec_use = 1;
    } break;
    case SLOT_CLR_CODEC_USE : {
        status.codec_use = 0;
    } break;
    case SLOT_SET_HAL_INPUT : {
        status.hal_use++;
    } break;
    case SLOT_CLR_HAL_INPUT : {
        status.hal_use--;
    } break;
    case SLOT_SET_HAL_OUTPUT : {
        status.hal_output = 1;
        status.not_ready  = 1;
    } break;
    case SLOT_CLR_HAL_OUTPUT : {
        status.hal_output = 0;
        // NOTE: set output index ready here
        status.not_ready  = 0;
    } break;
    case SLOT_SET_QUEUE_USE :
    case SLOT_ENQUEUE : {
        status.queue_use++;
    } break;
    case SLOT_CLR_QUEUE_USE :
    case SLOT_DEQUEUE : {
        status.queue_use--;
    } break;
    case SLOT_SET_EOS : {
        status.eos = 1;
    } break;
    case SLOT_CLR_EOS : {
        status.eos = 0;
        slot->eos = 0;
    } break;
    case SLOT_SET_FRAME : {
        status.has_frame = 1;
    } break;
    case SLOT_CLR_FRAME : {
        status.has_frame = 0;
        slot->frame = NULL;
    } break;
    case SLOT_SET_BUFFER : {
        status.has_buffer = 1;
    } break;
    case SLOT_CLR_BUFFER : {
        status.has_buffer = 0;
        slot->buffer = NULL;
    } break;
    default : {
        mpp_err("found invalid operation code %d\n", op);
    } break;
    }
    slot->status = status;
    buf_slot_dbg(BUF_SLOT_DBG_OPS_RUNTIME, "index %2d op: %s status in %08x out %08x",
                 index, op_string[op], before.val, status.val);
    add_slot_log(logs, index, op, before, status);
}

static void dump_slots(MppBufSlotsImpl *impl)
{
    RK_U32 i;
    MppBufSlotEntry *slot = impl->slots;

    mpp_log("\ndumping slots %p count %d size %d\n", impl, impl->count, impl->size);
    mpp_log("decode  count %d\n", impl->decode_count);
    mpp_log("display count %d\n", impl->display_count);

    for (i = 0; i < impl->count; i++, slot++) {
        SlotStatus status = slot->status;
        mpp_log("slot %2d used %d refer %d decoding %d display %d\n",
                i, status.on_used, status.codec_use, status.hal_use, status.queue_use);
    }

    mpp_log("\nslot operation history:\n\n");

    mpp_list *logs = impl->logs;
    if (logs) {
        while (logs->list_size()) {
            MppBufSlotLog log;
            logs->del_at_head(&log, sizeof(log));
            mpp_log("index %2d op: %s status in %08x out %08x",
                    log.index, op_string[log.ops], log.status_in.val, log.status_out.val);
        }
    }

}

static void init_slot_entry(MppBufSlotsImpl *impl, RK_S32 pos, RK_S32 count)
{
    mpp_list *logs = impl->logs;
    MppBufSlotEntry *slot = impl->slots;
    for (RK_S32 i = 0; i < count; i++, slot++) {
        slot->slots = impl;
        INIT_LIST_HEAD(&slot->list);
        slot->index = pos + i;
        slot->frame = NULL;
        slot_ops_with_log(logs, slot, SLOT_INIT);
    }
}

/*
 * only called on unref / displayed / decoded
 *
 * NOTE: MppFrame will be destroyed outside mpp
 *       but MppBuffer must dec_ref here
 */
static void check_entry_unused(MppBufSlotsImpl *impl, MppBufSlotEntry *entry)
{
    SlotStatus status = entry->status;

    if (status.on_used &&
        !status.not_ready &&
        !status.codec_use &&
        !status.hal_output &&
        !status.hal_use &&
        !status.queue_use) {
        if (entry->frame) {
            mpp_frame_deinit(&entry->frame);
            slot_ops_with_log(impl->logs, entry, SLOT_CLR_FRAME);
        } else
            mpp_buffer_put(entry->buffer);

        slot_ops_with_log(impl->logs, entry, SLOT_CLR_BUFFER);
        slot_ops_with_log(impl->logs, entry, SLOT_CLR_ON_USE);
    }
}

MPP_RET mpp_buf_slot_init(MppBufSlots *slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    MppBufSlotsImpl *impl = mpp_calloc(MppBufSlotsImpl, 1);
    if (NULL == impl) {
        *slots = NULL;
        return MPP_NOK;
    }

    mpp_env_get_u32("buf_slot_debug", &buf_slot_debug, BUF_SLOT_DBG_OPS_HISTORY);

    impl->lock = new Mutex();
    for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(impl->queue); i++) {
        INIT_LIST_HEAD(&impl->queue[i]);
    }

    if (buf_slot_debug & BUF_SLOT_DBG_OPS_HISTORY)
        impl->logs = new mpp_list(NULL);

    *slots = impl;
    return MPP_OK;
}

MPP_RET mpp_buf_slot_deinit(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(impl->queue); i++) {
        mpp_assert(list_empty(&impl->queue[i]));
    }
    MppBufSlotEntry *slot = (MppBufSlotEntry *)impl->slots;
    RK_U32 i;
    for (i = 0; i < impl->count; i++, slot++)
        mpp_assert(!slot->status.on_used);

    if (impl->logs)
        delete impl->logs;

    delete impl->lock;
    mpp_free(impl->slots);
    mpp_free(slots);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_setup(MppBufSlots slots, RK_U32 count, RK_U32 size, RK_U32 changed)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p setup: count %d size %d changed %d\n",
                 slots, count, size, changed);

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    if (NULL == impl->slots) {
        // first slot setup
        impl->count = count;
        impl->size  = size;
        impl->slots = mpp_calloc(MppBufSlotEntry, count);
        init_slot_entry(impl, 0, count);
    } else {
        // need to check info change or not
        if (!changed) {
            slot_assert(impl, size == impl->size);
            if (count > impl->count) {
                mpp_realloc(impl->slots, MppBufSlotEntry, count);
                init_slot_entry(impl, impl->count, (count - impl->count));
            }
        } else {
            // info changed, even size is the same we still need to wait for new configuration
            impl->new_count     = count;
            impl->new_size      = size;
            impl->info_changed  = 1;
        }
    }

    return MPP_OK;
}

RK_U32 mpp_buf_slot_is_changed(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    return impl->info_changed;
}

MPP_RET mpp_buf_slot_ready(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p is ready now\n", slots);

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    slot_assert(impl, impl->info_changed);
    slot_assert(impl, impl->slots);

    impl->info_changed  = 0;
    impl->size          = impl->new_size;
    if (impl->count != impl->new_count) {
        mpp_realloc(impl->slots, MppBufSlotEntry, impl->new_count);
        init_slot_entry(impl, 0, impl->new_count);
    }
    impl->count = impl->new_count;
    if (impl->logs) {
        mpp_list *logs = impl->logs;
        while (logs->list_size())
            logs->del_at_head(NULL, sizeof(MppBufSlotLog));
    }
    return MPP_OK;
}

RK_U32  mpp_buf_slot_get_size(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    return impl->size;
}

MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_U32 *index)
{
    if (NULL == slots || NULL == index) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    RK_U32 i;
    MppBufSlotEntry *slot = impl->slots;
    for (i = 0; i < impl->count; i++, slot++) {
        if (!slot->status.on_used) {
            *index = i;
            slot_ops_with_log(impl->logs, slot, SLOT_SET_ON_USE);
            slot_ops_with_log(impl->logs, slot, SLOT_SET_NOT_READY);
            return MPP_OK;
        }
    }

    *index = -1;
    mpp_err_f("failed to get a unused slot\n");
    dump_slots(impl);
    slot_assert(impl, 0);
    return MPP_NOK;
}

MPP_RET mpp_buf_slot_set_flag(MppBufSlots slots, RK_U32 index, SlotUsageType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    slot_assert(impl, index < impl->count);
    slot_ops_with_log(impl->logs, &impl->slots[index], set_flag_op[type]);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_clr_flag(MppBufSlots slots, RK_U32 index, SlotUsageType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    slot_assert(impl, index < impl->count);
    MppBufSlotEntry *slot = &impl->slots[index];
    slot_ops_with_log(impl->logs, slot, clr_flag_op[type]);

    if (type == SLOT_HAL_OUTPUT)
        impl->decode_count++;

    check_entry_unused(impl, slot);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_enqueue(MppBufSlots slots, RK_U32 index, SlotQueueType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    slot_assert(impl, index < impl->count);
    MppBufSlotEntry *slot = &impl->slots[index];
    slot_ops_with_log(impl->logs, slot, SLOT_ENQUEUE);

    // add slot to display list
    list_del_init(&slot->list);
    list_add_tail(&slot->list, &impl->queue[type]);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_dequeue(MppBufSlots slots, RK_U32 *index, SlotQueueType type)
{
    if (NULL == slots || NULL == index) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    if (list_empty(&impl->queue[type]))
        return MPP_NOK;

    MppBufSlotEntry *slot = list_entry(impl->queue[type].next, MppBufSlotEntry, list);
    if (slot->status.not_ready)
        return MPP_NOK;

    // make sure that this slot is just the next display slot
    list_del_init(&slot->list);
    slot_assert(impl, (RK_U32)slot->index < impl->count);
    slot_ops_with_log(impl->logs, slot, SLOT_DEQUEUE);
    impl->display_count++;
    *index = slot->index;

    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_prop(MppBufSlots slots, RK_U32 index, SlotPropType type, void *val)
{
    if (NULL == slots || NULL == val || type >= SLOT_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    slot_assert(impl, index < impl->count);
    MppBufSlotEntry *slot = &impl->slots[index];
    slot_ops_with_log(impl->logs, slot, set_val_op[type]);

    switch (type) {
    case SLOT_EOS: {
        RK_U32 eos = *(RK_U32*)val;
        slot->eos = eos;
    } break;
    case SLOT_FRAME: {
        MppFrame frame = val;
        slot_assert(impl, slot->status.not_ready);
        if (NULL == slot->frame)
            mpp_frame_init(&slot->frame);

        mpp_frame_copy(slot->frame, frame);
    } break;
    case SLOT_BUFFER: {
        MppBuffer buffer = val;
        if (slot->buffer) {
            // NOTE: reset buffer only on stream buffer slot
            slot_assert(impl, NULL == slot->frame);
            mpp_buffer_put(slot->buffer);
        }
        slot->buffer = buffer;
        if (slot->frame)
            mpp_frame_set_buffer(slot->frame, buffer);

        mpp_buffer_inc_ref(buffer);
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

MPP_RET mpp_buf_slot_get_prop(MppBufSlots slots, RK_U32 index, SlotPropType type, void *val)
{
    if (NULL == slots || NULL == val || type >= SLOT_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    Mutex::Autolock auto_lock(impl->lock);
    slot_assert(impl, index < impl->count);
    MppBufSlotEntry *slot = &impl->slots[index];

    switch (type) {
    case SLOT_EOS: {
        *(RK_U32*)val = slot->eos;
    } break;
    case SLOT_FRAME: {
        MppFrame *frame = (MppFrame *)val;
        mpp_assert(slot->status.has_frame);
        if (slot->status.has_frame) {
            mpp_frame_init(frame);
            if (*frame)
                mpp_frame_copy(*frame, slot->frame);
        } else
            *frame = NULL;
    } break;
    case SLOT_BUFFER: {
        MppBuffer *buffer = (MppBuffer *)val;
        *buffer = (slot->status.has_buffer) ? (slot->buffer) : (NULL);
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

