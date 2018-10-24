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
#define BUF_SLOT_DBG_INFO_CHANGE        (0x00000004)
#define BUF_SLOT_DBG_OPS_RUNTIME        (0x00000010)
#define BUF_SLOT_DBG_BUFFER             (0x00000100)
#define BUF_SLOT_DBG_FRAME              (0x00000200)
#define BUF_SLOT_DBG_BUF_UESD           (0x00000400)
#define BUF_SLOT_DBG_OPS_HISTORY        (0x10000000)
#define BUF_SLOT_DBG_ALL                (0x10000011)

#define buf_slot_dbg(flag, fmt, ...)    _mpp_dbg(buf_slot_debug, flag, fmt, ## __VA_ARGS__)

static RK_U32 buf_slot_debug = 0;
static RK_U32 buf_slot_idx = 0;

#define slot_assert(impl, cond) do {                                    \
    if (!(cond)) {                                                      \
        dump_slots(impl);                                               \
        mpp_err("Assertion %s failed at %s:%d\n",                       \
               MPP_STRINGS(cond), __FUNCTION__, __LINE__);              \
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
    SLOT_ENQUEUE_OUTPUT = SLOT_ENQUEUE,
    SLOT_ENQUEUE_DISPLAY,
    SLOT_ENQUEUE_DEINTER,
    SLOT_ENQUEUE_CONVERT,
    SLOT_DEQUEUE,
    SLOT_DEQUEUE_OUTPUT = SLOT_DEQUEUE,
    SLOT_DEQUEUE_DISPLAY,
    SLOT_DEQUEUE_DEINTER,
    SLOT_DEQUEUE_CONVERT,

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

    "enqueue output ",
    "enqueue display",
    "enqueue deint  ",
    "enqueue convert",
    "dequeue output ",
    "dequeue display",
    "dequeue deint  ",
    "dequeue convert",

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
    RK_S32              index;
    MppBufSlotOps       ops;
    SlotStatus          status_in;
    SlotStatus          status_out;
} MppBufSlotLog;

struct MppBufSlotEntry_t {
    MppBufSlotsImpl     *slots;
    struct list_head    list;
    SlotStatus          status;
    RK_S32              index;

    RK_U32              eos;
    MppFrame            frame;
    MppBuffer           buffer;
};

struct MppBufSlotsImpl_t {
    Mutex               *lock;
    RK_U32              slots_idx;

    // status tracing
    RK_U32              decode_count;
    RK_U32              display_count;

    // if slot changed, all will be hold until all slot is unused
    RK_U32              info_changed;
    RK_S32              new_count;

    // slot infomation for info change and eos
    RK_U32              eos;

    // buffer parameter, default alignement is 16
    AlignFunc           hal_hor_align;          // default NULL
    AlignFunc           hal_ver_align;          // default NULL
    AlignFunc           hal_len_align;          // default NULL
    size_t              buf_size;
    RK_S32              buf_count;
    RK_S32              used_count;
    // buffer size equal to (h_stride * v_stride) * numerator / denominator
    // internal parameter
    RK_U32              numerator;
    RK_U32              denominator;

    // NOTE: use MppFrame to store the buffer/display infomation
    //       any buffer related infomation change comparing to previous frame will
    //       trigger a buffer info changed requirement
    //       any display related infomation change comparing to pevious frame will
    //       trigger a display info changed requirement
    MppFrame            info;
    MppFrame            info_set;

    // list for display
    struct list_head    queue[QUEUE_BUTT];

    // list for log
    mpp_list            *logs;

    MppBufSlotEntry     *slots;
};

static RK_U32 default_align_16(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static void generate_info_set(MppBufSlotsImpl *impl, MppFrame frame, RK_U32 force_default_align)
{
    RK_U32 width  = mpp_frame_get_width(frame);
    RK_U32 height = mpp_frame_get_height(frame);
    MppFrameFormat fmt = mpp_frame_get_fmt(frame);
    RK_U32 depth = (fmt == MPP_FMT_YUV420SP_10BIT
                    || fmt == MPP_FMT_YUV422SP_10BIT) ? 10 : 8;
    RK_U32 codec_hor_stride = mpp_frame_get_hor_stride(frame);
    RK_U32 codec_ver_stride = mpp_frame_get_ver_stride(frame);
    RK_U32 hal_hor_stride = (codec_hor_stride) ?
                            (impl->hal_hor_align(codec_hor_stride)) :
                            (impl->hal_hor_align(width * depth >> 3));
    RK_U32 hal_ver_stride = (codec_ver_stride) ?
                            (impl->hal_ver_align(codec_ver_stride)) :
                            (impl->hal_ver_align(height));

    hal_hor_stride = (force_default_align && codec_hor_stride) ? codec_hor_stride : hal_hor_stride;
    hal_ver_stride = (force_default_align && codec_ver_stride) ? codec_ver_stride : hal_ver_stride;

    RK_U32 size = hal_hor_stride * hal_ver_stride;
    size *= impl->numerator;
    size /= impl->denominator;
    size = impl->hal_len_align ? impl->hal_len_align(hal_hor_stride * hal_ver_stride) : size;

    mpp_frame_set_width(impl->info_set, width);
    mpp_frame_set_height(impl->info_set, height);
    mpp_frame_set_fmt(impl->info_set, fmt);
    mpp_frame_set_hor_stride(impl->info_set, hal_hor_stride);
    mpp_frame_set_ver_stride(impl->info_set, hal_ver_stride);
    mpp_frame_set_buf_size(impl->info_set, size);
    mpp_frame_set_buf_size(frame, size);
    impl->buf_size = size;

    MppFrameImpl *info_set_impl = (MppFrameImpl *)impl->info_set;
    MppFrameImpl *frame_impl    = (MppFrameImpl *)frame;
    info_set_impl->color_range      = frame_impl->color_range;
    info_set_impl->color_primaries  = frame_impl->color_primaries;
    info_set_impl->color_trc        = frame_impl->color_trc;
    info_set_impl->colorspace       = frame_impl->colorspace;
    info_set_impl->chroma_location  = frame_impl->chroma_location;
}

#define dump_slots(...) _dump_slots(__FUNCTION__, ## __VA_ARGS__)

static void _dump_slots(const char *caller, MppBufSlotsImpl *impl)
{
    RK_S32 i;
    MppBufSlotEntry *slot = impl->slots;

    mpp_log("\ncaller %s is dumping slots\n", caller, impl->slots_idx);
    mpp_log("slots %d %p buffer count %d buffer size %d\n", impl->slots_idx,
            impl, impl->buf_count, impl->buf_size);
    mpp_log("decode  count %d\n", impl->decode_count);
    mpp_log("display count %d\n", impl->display_count);

    for (i = 0; i < impl->buf_count; i++, slot++) {
        SlotStatus status = slot->status;
        mpp_log("slot %2d used %d refer %d decoding %d display %d status %08x\n",
                i, status.on_used, status.codec_use, status.hal_use, status.queue_use, status.val);
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

    mpp_assert(0);

    return;
}

static void add_slot_log(mpp_list *logs, RK_S32 index, MppBufSlotOps op, SlotStatus before, SlotStatus after)
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

static void slot_ops_with_log(MppBufSlotsImpl *impl, MppBufSlotEntry *slot, MppBufSlotOps op, void *arg)
{
    RK_U32 error = 0;
    RK_S32 index = slot->index;
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
        if (status.hal_use)
            status.hal_use--;
        else {
            mpp_err("can not clr hal_input on slot %d\n", slot->index);
            error = 1;
        }
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
    case SLOT_ENQUEUE_OUTPUT :
    case SLOT_ENQUEUE_DISPLAY :
    case SLOT_ENQUEUE_DEINTER :
    case SLOT_ENQUEUE_CONVERT : {
        status.queue_use++;
    } break;
    case SLOT_CLR_QUEUE_USE :
    case SLOT_DEQUEUE_OUTPUT :
    case SLOT_DEQUEUE_DISPLAY :
    case SLOT_DEQUEUE_DEINTER :
    case SLOT_DEQUEUE_CONVERT : {
        if (status.queue_use)
            status.queue_use--;
        else {
            mpp_err("can not clr queue_use on slot %d\n", slot->index);
            error = 1;
        }
    } break;
    case SLOT_SET_EOS : {
        status.eos = 1;
    } break;
    case SLOT_CLR_EOS : {
        status.eos = 0;
        slot->eos = 0;
    } break;
    case SLOT_SET_FRAME : {
        status.has_frame = (arg) ? (1) : (0);
    } break;
    case SLOT_CLR_FRAME : {
        status.has_frame = 0;
    } break;
    case SLOT_SET_BUFFER : {
        status.has_buffer = (arg) ? (1) : (0);
    } break;
    case SLOT_CLR_BUFFER : {
        status.has_buffer = 0;
    } break;
    default : {
        mpp_err("found invalid operation code %d\n", op);
        error = 1;
    } break;
    }
    slot->status = status;
    buf_slot_dbg(BUF_SLOT_DBG_OPS_RUNTIME, "slot %3d index %2d op: %s arg %010p status in %08x out %08x",
                 impl->slots_idx, index, op_string[op], arg, before.val, status.val);
    add_slot_log(impl->logs, index, op, before, status);
    if (error)
        dump_slots(impl);
}

static void init_slot_entry(MppBufSlotsImpl *impl, RK_S32 pos, RK_S32 count)
{
    MppBufSlotEntry *slot = impl->slots;
    for (RK_S32 i = 0; i < count; i++, slot++) {
        slot->slots = impl;
        INIT_LIST_HEAD(&slot->list);
        slot->index = pos + i;
        slot->frame = NULL;
        slot_ops_with_log(impl, slot, SLOT_INIT, NULL);
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
            slot_ops_with_log(impl, entry, SLOT_CLR_FRAME, entry->frame);
            mpp_frame_deinit(&entry->frame);
        }
        if (entry->buffer) {
            mpp_buffer_put(entry->buffer);
            slot_ops_with_log(impl, entry, SLOT_CLR_BUFFER, entry->buffer);
            entry->buffer = NULL;
        }

        slot_ops_with_log(impl, entry, SLOT_CLR_ON_USE, NULL);
        impl->used_count--;
    }
}

static void clear_slots_impl(MppBufSlotsImpl *impl)
{
    MppBufSlotEntry *slot = (MppBufSlotEntry *)impl->slots;
    RK_S32 i;

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(impl->queue); i++) {
        if (!list_empty(&impl->queue[i]))
            dump_slots(impl);

        mpp_assert(list_empty(&impl->queue[i]));
    }

    for (i = 0; i < impl->buf_count; i++, slot++) {
        mpp_assert(!slot->status.on_used);
        if (slot->status.on_used) {
            dump_slots(impl);
            mpp_buf_slot_reset(impl, i);
        }
    }

    impl->used_count = 0;

    if (impl->info)
        mpp_frame_deinit(&impl->info);

    if (impl->info_set)
        mpp_frame_deinit(&impl->info_set);

    if (impl->logs)
        delete impl->logs;

    if (impl->lock)
        delete impl->lock;

    mpp_free(impl->slots);
    mpp_free(impl);
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

    do {
        impl->lock = new Mutex();
        if (NULL == impl->lock)
            break;

        for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(impl->queue); i++) {
            INIT_LIST_HEAD(&impl->queue[i]);
        }

        if (buf_slot_debug & BUF_SLOT_DBG_OPS_HISTORY) {
            impl->logs = new mpp_list(NULL);
            if (NULL == impl->logs)
                break;
        }

        if (mpp_frame_init(&impl->info))
            break;

        if (mpp_frame_init(&impl->info_set))
            break;

        // slots information default setup
        impl->hal_hor_align = default_align_16;
        impl->hal_ver_align = default_align_16;
        impl->hal_len_align = NULL;
        impl->numerator     = 9;
        impl->denominator   = 5;
        impl->slots_idx     = buf_slot_idx++;

        *slots = impl;
        return MPP_OK;
    } while (0);

    clear_slots_impl(impl);

    *slots = NULL;
    return MPP_NOK;
}

MPP_RET mpp_buf_slot_deinit(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    clear_slots_impl((MppBufSlotsImpl *)slots);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_setup(MppBufSlots slots, RK_S32 count)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p setup: count %d\n", slots, count);

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);

    if (NULL == impl->slots) {
        // first slot setup
        impl->buf_count = impl->new_count = count;
        impl->slots = mpp_calloc(MppBufSlotEntry, count);
        init_slot_entry(impl, 0, count);
        impl->used_count = 0;
    } else {
        // record the slot count for info changed ready config
        if (count > impl->buf_count) {
            mpp_realloc(impl->slots, MppBufSlotEntry, count);
            init_slot_entry(impl, impl->buf_count, (count - impl->buf_count));
        }
        impl->new_count = count;
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
    AutoMutex auto_lock(impl->lock);
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
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, impl->slots);
    if (!impl->info_changed)
        mpp_log("found info change ready set without internal info change\n");

    // ready mean the info_set will be copy to info as the new configuration
    if (impl->buf_count != impl->new_count) {
        mpp_realloc(impl->slots, MppBufSlotEntry, impl->new_count);
        init_slot_entry(impl, 0, impl->new_count);
    }
    impl->buf_count = impl->new_count;

    mpp_frame_copy(impl->info, impl->info_set);
    impl->buf_size = mpp_frame_get_buf_size(impl->info);

    if (impl->logs) {
        mpp_list *logs = impl->logs;
        while (logs->list_size())
            logs->del_at_head(NULL, sizeof(MppBufSlotLog));
    }
    impl->info_changed  = 0;
    return MPP_OK;
}

size_t mpp_buf_slot_get_size(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    return impl->buf_size;
}

MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_S32 *index)
{
    if (NULL == slots || NULL == index) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    RK_S32 i;
    MppBufSlotEntry *slot = impl->slots;
    for (i = 0; i < impl->buf_count; i++, slot++) {
        if (!slot->status.on_used) {
            *index = i;
            slot_ops_with_log(impl, slot, SLOT_SET_ON_USE, NULL);
            slot_ops_with_log(impl, slot, SLOT_SET_NOT_READY, NULL);
            impl->used_count++;
            return MPP_OK;
        }
    }

    *index = -1;
    mpp_err_f("failed to get a unused slot\n");
    dump_slots(impl);
    slot_assert(impl, 0);
    return MPP_NOK;
}

MPP_RET mpp_buf_slot_set_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot_ops_with_log(impl, &impl->slots[index], set_flag_op[type], NULL);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_clr_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];
    slot_ops_with_log(impl, slot, clr_flag_op[type], NULL);

    if (type == SLOT_HAL_OUTPUT)
        impl->decode_count++;

    check_entry_unused(impl, slot);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_enqueue(MppBufSlots slots, RK_S32 index, SlotQueueType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];
    slot_ops_with_log(impl, slot, (MppBufSlotOps)(SLOT_ENQUEUE + type), NULL);

    // add slot to display list
    list_del_init(&slot->list);
    list_add_tail(&slot->list, &impl->queue[type]);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_dequeue(MppBufSlots slots, RK_S32 *index, SlotQueueType type)
{
    if (NULL == slots || NULL == index) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    if (list_empty(&impl->queue[type]))
        return MPP_NOK;

    MppBufSlotEntry *slot = list_entry(impl->queue[type].next, MppBufSlotEntry, list);
    if (slot->status.not_ready)
        return MPP_NOK;

    // make sure that this slot is just the next display slot
    list_del_init(&slot->list);
    slot_assert(impl, slot->index < impl->buf_count);
    slot_ops_with_log(impl, slot, (MppBufSlotOps)(SLOT_DEQUEUE + type), NULL);
    impl->display_count++;
    *index = slot->index;

    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val)
{
    if (NULL == slots || NULL == val || type >= SLOT_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];
    slot_ops_with_log(impl, slot, set_val_op[type], val);

    switch (type) {
    case SLOT_EOS: {
        RK_U32 eos = *(RK_U32*)val;
        slot->eos = eos;
        if (slot->frame)
            mpp_frame_set_eos(slot->frame, eos);
    } break;
    case SLOT_FRAME: {
        MppFrame frame = val;

        slot_assert(impl, slot->status.not_ready);
        /*
         * we need to detect infomation change here
         * there are two types of info change:
         * 1. buffer size change
         *    this case need to reset buffer group and commit buffer with new size
         * 2. display info change
         *    if only width/height/fmt is change and buffer do not need to be reset
         *    only display info change is need
         */
        generate_info_set(impl, frame, 0);

        if (NULL == slot->frame)
            mpp_frame_init(&slot->frame);

        MppFrameImpl *src = (MppFrameImpl *)frame;
        MppFrameImpl *dst = (MppFrameImpl *)slot->frame;
        mpp_frame_copy(dst, src);
        // NOTE: stride from codec need to be change to hal stride
        //       hor_stride and ver_stride can not be zero
        //       they are the stride required by codec
        //       then hal will modify it according to hardware requirement
        mpp_assert(src->hor_stride);
        mpp_assert(src->ver_stride);
        dst->hor_stride = impl->hal_hor_align(src->hor_stride);
        dst->ver_stride = impl->hal_ver_align(src->ver_stride);
        dst->eos = slot->eos;

        if (mpp_frame_info_cmp(impl->info, impl->info_set)) {
            MppFrameImpl *old = (MppFrameImpl *)impl->info;

            impl->info_changed = 1;

            if (old->width || old->height) {
                mpp_dbg(MPP_DBG_INFO, "info change found\n");
                mpp_dbg(MPP_DBG_INFO,
                        "old width %4d height %4d stride hor %4d ver %4d fmt %4d\n",
                        old->width, old->height, old->hor_stride,
                        old->ver_stride, old->fmt);
            }
            mpp_dbg(MPP_DBG_INFO,
                    "new width %4d height %4d stride hor %4d ver %4d fmt %4d\n",
                    dst->width, dst->height, dst->hor_stride, dst->ver_stride,
                    dst->fmt);
            // info change found here
        }
    } break;
    case SLOT_BUFFER: {
        MppBuffer buffer = val;
        if (slot->buffer) {
            // NOTE: reset buffer only on stream buffer slot
            slot_assert(impl, NULL == slot->frame);
            mpp_buffer_put(slot->buffer);
        }
        mpp_buffer_inc_ref(buffer);
        slot->buffer = buffer;

        if (slot->frame)
            mpp_frame_set_buffer(slot->frame, buffer);
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

MPP_RET mpp_buf_slot_get_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val)
{
    if (NULL == slots || NULL == val || type >= SLOT_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];

    switch (type) {
    case SLOT_EOS: {
        *(RK_U32*)val = slot->eos;
    } break;
    case SLOT_FRAME: {
        MppFrame *frame = (MppFrame *)val;
        //*frame = (slot->status.has_frame) ? (slot->frame) : (NULL);

        mpp_assert(slot->status.has_frame);
        if (slot->status.has_frame) {
            if (NULL == *frame )
                mpp_frame_init(frame);
            if (*frame)
                mpp_frame_copy(*frame, slot->frame);
        } else
            *frame = NULL;
    } break;
    case SLOT_FRAME_PTR: {
        MppFrame *frame = (MppFrame *)val;
        mpp_assert(slot->status.has_frame);
        *frame = (slot->status.has_frame) ? (slot->frame) : (NULL);
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

MPP_RET mpp_buf_slot_reset(MppBufSlots slots, RK_S32 index)
{
    if (NULL == slots || index < 0) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p reset index %d\n", slots, index);

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];

    // make sure that this slot is just the next display slot
    list_del_init(&slot->list);
    slot_ops_with_log(impl, slot, SLOT_CLR_QUEUE_USE, NULL);
    slot_ops_with_log(impl, slot, SLOT_DEQUEUE, NULL);
    slot_ops_with_log(impl, slot, SLOT_CLR_ON_USE, NULL);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_default_info(MppBufSlots slots, RK_S32 index, void *val)
{
    if (NULL == slots || index < 0) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];

    slot_assert(impl, slot->status.not_ready);
    slot_assert(impl, NULL == slot->frame);
    slot_assert(impl, impl->info_set);

    if (NULL == slot->frame) {
        mpp_frame_init(&slot->frame);
        mpp_frame_copy(slot->frame, impl->info_set);
    }

    MppFrame *frame = (MppFrame *)val;
    *frame = slot->frame;

    slot_ops_with_log(impl, slot, SLOT_CLR_NOT_READY, NULL);
    slot_ops_with_log(impl, slot, SLOT_SET_FRAME, slot->frame);
    return MPP_OK;
}

RK_U32 mpp_slots_is_empty(MppBufSlots slots, SlotQueueType type)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    return list_empty(&impl->queue[type]) ? 1 : 0;
}

RK_S32 mpp_slots_get_used_count(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    return impl->used_count;
}

RK_S32 mpp_slots_get_unused_count(MppBufSlots slots)
{
    if (NULL == slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    slot_assert(impl, (impl->used_count >= 0) && (impl->used_count <= impl->buf_count));
    return impl->buf_count - impl->used_count;
}

MPP_RET mpp_slots_set_prop(MppBufSlots slots, SlotsPropType type, void *val)
{
    if (NULL == slots || NULL == val || type >= SLOTS_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    RK_U32 value = *((RK_U32*)val);
    switch (type) {
    case SLOTS_EOS: {
        impl->eos = value;
    } break;
    case SLOTS_NUMERATOR : {
        impl->numerator = value;
    } break;
    case SLOTS_DENOMINATOR : {
        impl->denominator = value;
    } break;
    case SLOTS_HOR_ALIGN: {
        impl->hal_hor_align = (AlignFunc)val;
    } break;
    case SLOTS_VER_ALIGN: {
        impl->hal_ver_align = (AlignFunc)val;
    } break;
    case SLOTS_LEN_ALIGN: {
        impl->hal_len_align = (AlignFunc)val;
    } break;
    case SLOTS_COUNT: {
        impl->buf_count = value;
    } break;
    case SLOTS_SIZE: {
        impl->buf_size = value;
    } break;
    case SLOTS_FRAME_INFO: {
        // do info change detection here
        generate_info_set(impl, (MppFrame)val, 1);
        mpp_frame_copy(impl->info, impl->info_set);
        {
            MppFrameImpl *p = (MppFrameImpl *)impl->info;
            mpp_log("set frame info: w %4d h %4d hor %4d ver %4d\n", p->width, p->height, p->hor_stride, p->ver_stride);
        }
        mpp_frame_copy((MppFrame)val, impl->info_set);
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

MPP_RET mpp_slots_get_prop(MppBufSlots slots, SlotsPropType type, void *val)
{
    if (NULL == slots || NULL == val || type >= SLOTS_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_NOK;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    AutoMutex auto_lock(impl->lock);
    MPP_RET ret = MPP_OK;

    switch (type) {
    case SLOTS_EOS: {
        *((RK_U32 *)val) = impl->eos;
    } break;
    case SLOTS_COUNT: {
        *((RK_U32 *)val) = impl->buf_count;
    } break;
    case SLOTS_SIZE: {
        *((RK_U32 *)val) = (RK_U32)impl->buf_size;
    } break;
    case SLOTS_FRAME_INFO: {
        MppFrame frame = (MppFrame)val;
        MppFrame info  = impl->info;
        mpp_frame_copy(frame, info);
    } break;
    default : {
        mpp_err("can not get slots prop type %d\n", type);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}
