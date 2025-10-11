/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_buf_slot"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_frame_impl.h"
#include "mpp_buf_slot.h"
#include "mpp_compat_impl.h"
#include "rk_mpp_cfg.h"

#define BUF_SLOT_DBG_FUNCTION           (0x00000001)
#define BUF_SLOT_DBG_SETUP              (0x00000002)
#define BUF_SLOT_DBG_INFO_CHANGE        (0x00000004)
#define BUF_SLOT_DBG_OPS_RUNTIME        (0x00000010)
#define BUF_SLOT_DBG_BUFFER             (0x00000100)
#define BUF_SLOT_DBG_FRAME              (0x00000200)
#define BUF_SLOT_DBG_BUF_UESD           (0x00000400)
#define BUF_SLOT_DBG_INFO_SET           (0x00000800)
#define BUF_SLOT_DBG_OPS_HISTORY        (0x10000000)
#define BUF_SLOT_DBG_ALL                (0x10000011)

#define buf_slot_dbg(flag, fmt, ...)    _mpp_dbg(buf_slot_debug, flag, fmt, ## __VA_ARGS__)

static RK_U32 buf_slot_debug = 0;
static RK_U32 buf_slot_idx = 0;
static RK_U32 use_legacy_align = 0;

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

typedef union SlotStatus_u {
    RK_U32 val;
    struct {
        // status flags
        RK_U32  on_used     : 1;
        RK_U32  not_ready   : 1;        // buffer slot is filled or not
        RK_U32  codec_use   : 1;        // buffer slot is used by codec ( dpb reference )
        RK_U32  hal_output  : 2;        // buffer slot is set to hw output will ready when hw done
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

typedef struct MppBufSlotLogs_t {
    pthread_mutex_t     lock;
    RK_U16              max_count;
    RK_U16              log_count;
    RK_U16              log_write;
    RK_U16              log_read;
    MppBufSlotLog       *logs;
} MppBufSlotLogs;

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
    MppMutex            lock;
    RK_U32              slots_idx;

    // status tracing
    RK_U32              decode_count;
    RK_U32              display_count;

    MppCodingType       coding_type;

    // if slot changed, all will be hold until all slot is unused
    RK_U32              info_changed;
    RK_S32              info_change_slot_idx;
    RK_S32              new_count;

    // slot infomation for info change and eos
    RK_U32              eos;

    // buffer parameter, default alignement is 16
    MppSysCfg           sys_cfg;
    AlignFunc           hal_hor_align;          // default NULL
    AlignFunc           hal_ver_align;          // default NULL
    AlignFunc           hal_len_align;          // default NULL
    AlignFunc           hal_width_align;        // default NULL
    SlotHalFbcAdjCfg    hal_fbc_adj_cfg;        // hal fbc frame adjust config
    size_t              buf_size;
    RK_S32              buf_count;
    RK_S32              used_count;
    RK_U32              align_chk_log_env;
    RK_U32              align_chk_log_en;
    // buffer size equal to (h_stride * v_stride) * numerator / denominator
    // internal parameter
    RK_U32              numerator;
    RK_U32              denominator;

    // callback for free slot notify
    MppCbCtx            callback;

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
    MppBufSlotLogs      *logs;

    MppBufSlotEntry     *slots;
};

typedef struct MppBufSlotInfoSet_t {
    RK_U32 h_stride_by_pixel;
    RK_U32 h_stride_by_byte;
    RK_U32 v_stride;
    RK_U32 size_total;
} MppBufSlotInfoSet;

static RK_U32 default_align_16(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

/* Based on drm_gem_framebuffer_helper.c drm_gem_afbc_min_size() */
static RK_S32 get_afbc_min_size(RK_S32 width, RK_S32 height, RK_S32 bpp)
{
#define AFBC_HEADER_SIZE 16
#define AFBC_HDR_ALIGN 64
#define AFBC_SUPERBLOCK_PIXELS 256
#define AFBC_SUPERBLOCK_ALIGNMENT 128

    RK_S32 n_blocks, hdr_alignment, size;

    /* AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 and !AFBC_FORMAT_MOD_TILED */
    width = MPP_ALIGN(width, 16);
    height = MPP_ALIGN(height, 16);
    hdr_alignment = AFBC_HDR_ALIGN;

    n_blocks = (width * height) / AFBC_SUPERBLOCK_PIXELS;

    size = MPP_ALIGN(n_blocks * AFBC_HEADER_SIZE, hdr_alignment);
    size += n_blocks * MPP_ALIGN(bpp * AFBC_SUPERBLOCK_PIXELS / 8,
                                 AFBC_SUPERBLOCK_ALIGNMENT);
    return size;
}

static void prepare_info_set_legacy(MppBufSlotsImpl *impl, MppFrame frame,
                                    MppBufSlotInfoSet *info_set,
                                    RK_U32 force_def_align)
{
    const RK_U32 width  = mpp_frame_get_width(frame);
    const RK_U32 height = mpp_frame_get_height(frame);
    const MppFrameFormat fmt = mpp_frame_get_fmt(frame);
    RK_U32 depth = ((fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV420SP_10BIT ||
                    (fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV422SP_10BIT ||
                    (fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV444SP_10BIT) ? 10 : 8;
    RK_U32 codec_hor_stride = mpp_frame_get_hor_stride(frame);
    RK_U32 codec_ver_stride = mpp_frame_get_ver_stride(frame);
    RK_U32 coded_width = (impl->hal_width_align) ?
                         (impl->hal_width_align(width)) : width;

    RK_U32 hal_hor_stride = (codec_hor_stride) ?
                            (impl->hal_hor_align(codec_hor_stride)) :
                            (impl->hal_hor_align(coded_width * depth >> 3));
    RK_U32 hal_ver_stride = (codec_ver_stride) ?
                            (impl->hal_ver_align(codec_ver_stride)) :
                            (impl->hal_ver_align(height));
    RK_U32 hor_stride_pixel;
    RK_S32 size;

    hal_hor_stride = (force_def_align && codec_hor_stride) ? codec_hor_stride : hal_hor_stride;
    hal_ver_stride = (force_def_align && codec_ver_stride) ? codec_ver_stride : hal_ver_stride;

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        /*fbc stride default 64 align*/
        if (*compat_ext_fbc_hdr_256_odd)
            hal_hor_stride = (MPP_ALIGN(width, 256) | 256) * depth >> 3;
        else
            hal_hor_stride = MPP_ALIGN(width, 64) * depth >> 3;
    }

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP_10BIT:
    case MPP_FMT_YUV422SP_10BIT:
    case MPP_FMT_YUV444SP_10BIT: {
        hor_stride_pixel = hal_hor_stride * 8 / 10;
    } break;
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565: {
        hor_stride_pixel = hal_hor_stride / 2;
    } break;
    case MPP_FMT_RGB888:
    case MPP_FMT_BGR888: {
        hor_stride_pixel = hal_hor_stride / 3;
    } break;
    default : {
        hor_stride_pixel = hal_hor_stride;
    } break;
    }

    size = hal_hor_stride * hal_ver_stride;

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        hor_stride_pixel = MPP_ALIGN(hor_stride_pixel, 64);
        switch ((fmt & MPP_FRAME_FMT_MASK)) {
        case MPP_FMT_YUV420SP_10BIT : {
            size = get_afbc_min_size(hor_stride_pixel, hal_ver_stride, 15);
        } break;
        case MPP_FMT_YUV422SP_10BIT : {
            size = get_afbc_min_size(hor_stride_pixel, hal_ver_stride, 20);
        } break;
        case MPP_FMT_YUV420SP : {
            size = get_afbc_min_size(hor_stride_pixel, hal_ver_stride, 12);
        } break;
        case MPP_FMT_YUV422SP : {
            size = get_afbc_min_size(hor_stride_pixel, hal_ver_stride, 16);
        } break;
        case MPP_FMT_YUV444SP : {
            size = get_afbc_min_size(hor_stride_pixel, hal_ver_stride, 24);
        } break;
        case MPP_FMT_YUV444SP_10BIT : {
            size = get_afbc_min_size(hor_stride_pixel, hal_ver_stride, 30);
        } break;
        default : {
            size = hal_hor_stride * hal_ver_stride * 3 / 2;
            mpp_err("dec out fmt is no support");
        } break;
        }
        mpp_frame_set_fbc_size(frame, size);
    } else {
        size *= impl->numerator;
        size /= impl->denominator;
        size = impl->hal_len_align ? (RK_S32)impl->hal_len_align(hal_hor_stride * hal_ver_stride) : size;
    }

    info_set->h_stride_by_byte = hal_hor_stride;
    info_set->v_stride = hal_ver_stride;
    info_set->h_stride_by_pixel = hor_stride_pixel;
    info_set->size_total = size;
}

static void prepare_info_set_by_sys_cfg(MppBufSlotsImpl *impl, MppFrame frame,
                                        MppBufSlotInfoSet *info_set)
{
    const RK_U32 width  = mpp_frame_get_width(frame);
    const RK_U32 height = mpp_frame_get_height(frame);
    const RK_U32 codec_hor_stride = mpp_frame_get_hor_stride(frame);
    const RK_U32 codec_ver_stride = mpp_frame_get_ver_stride(frame);
    const MppFrameFormat fmt = mpp_frame_get_fmt(frame);

    /* set correct parameter */
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:enable", 1);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:type", impl->coding_type);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:fmt_codec", fmt & MPP_FRAME_FMT_MASK);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:fmt_fbc", fmt & MPP_FRAME_FBC_MASK);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:fmt_hdr", fmt & MPP_FRAME_HDR_MASK);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:width", width);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:height", height);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:h_stride_by_byte", codec_hor_stride);
    mpp_sys_cfg_set_u32(impl->sys_cfg, "dec_buf_chk:v_stride", codec_ver_stride);

    /* get result */
    mpp_sys_cfg_ioctl(impl->sys_cfg);

    mpp_sys_cfg_get_u32(impl->sys_cfg, "dec_buf_chk:h_stride_by_byte", &info_set->h_stride_by_byte);
    mpp_sys_cfg_get_u32(impl->sys_cfg, "dec_buf_chk:h_stride_by_pixel", &info_set->h_stride_by_pixel);
    mpp_sys_cfg_get_u32(impl->sys_cfg, "dec_buf_chk:v_stride", &info_set->v_stride);
    mpp_sys_cfg_get_u32(impl->sys_cfg, "dec_buf_chk:size_total", &info_set->size_total);

    return;
}

static void generate_info_set(MppBufSlotsImpl *impl, MppFrame frame, RK_U32 force_def_align)
{
    const RK_U32 width  = mpp_frame_get_width(frame);
    const RK_U32 height = mpp_frame_get_height(frame);
    const MppFrameFormat fmt = mpp_frame_get_fmt(frame);
    MppBufSlotInfoSet legacy_info_set;
    MppBufSlotInfoSet sys_cfg_info_set;
    MppBufSlotInfoSet *info_set_ptr = NULL;
    MppFrameImpl *info_set_impl = NULL;
    MppFrameImpl *frame_impl = NULL;

    prepare_info_set_legacy(impl, frame, &legacy_info_set, force_def_align);
    prepare_info_set_by_sys_cfg(impl, frame, &sys_cfg_info_set);

    mpp_frame_set_width(impl->info_set, width);
    mpp_frame_set_height(impl->info_set, height);
    mpp_frame_set_fmt(impl->info_set, fmt);
    info_set_ptr = use_legacy_align ? &legacy_info_set : &sys_cfg_info_set;
    mpp_frame_set_hor_stride(impl->info_set, info_set_ptr->h_stride_by_byte);
    mpp_frame_set_ver_stride(impl->info_set, info_set_ptr->v_stride);
    mpp_frame_set_hor_stride_pixel(impl->info_set, info_set_ptr->h_stride_by_pixel);
    mpp_frame_set_buf_size(impl->info_set, info_set_ptr->size_total);
    mpp_frame_set_buf_size(frame, info_set_ptr->size_total);
    mpp_frame_set_hor_stride(frame, info_set_ptr->h_stride_by_byte);
    mpp_frame_set_ver_stride(frame, info_set_ptr->v_stride);
    mpp_frame_set_hor_stride_pixel(frame, info_set_ptr->h_stride_by_pixel);
    impl->buf_size = info_set_ptr->size_total;

    if (mpp_frame_get_thumbnail_en(frame) == MPP_FRAME_THUMBNAIL_MIXED) {
        /*
         * The decode hw only support 1/2 scaling in width and height,
         * downscale output image only support raster mode with 8bit depth.
         */
        RK_U32 down_scale_ver = MPP_ALIGN(mpp_frame_get_height(frame) >> 1, 16);
        RK_U32 down_scale_hor = MPP_ALIGN(mpp_frame_get_width(frame) >> 1, 16);
        RK_U32 downscale_buf_size;
        RK_U32 down_scale_y_virstride = down_scale_ver * down_scale_hor;

        switch ((fmt & MPP_FRAME_FMT_MASK)) {
        case MPP_FMT_YUV400 : {
            downscale_buf_size = down_scale_y_virstride;
        } break;
        case MPP_FMT_YUV420SP_10BIT :
        case MPP_FMT_YUV420SP : {
            downscale_buf_size = down_scale_y_virstride * 3 / 2;
        } break;
        case MPP_FMT_YUV422SP_10BIT :
        case MPP_FMT_YUV422SP : {
            downscale_buf_size = down_scale_y_virstride * 2;
        } break;
        case MPP_FMT_YUV444SP_10BIT :
        case MPP_FMT_YUV444SP : {
            downscale_buf_size = down_scale_y_virstride * 3;
        } break;
        default : {
            downscale_buf_size = down_scale_y_virstride * 3 / 2;
        } break;
        }
        downscale_buf_size = MPP_ALIGN(downscale_buf_size, 16);
        impl->buf_size += downscale_buf_size;
        mpp_frame_set_buf_size(impl->info_set, impl->buf_size);
        mpp_frame_set_buf_size(frame, impl->buf_size);
    }
    info_set_impl = (MppFrameImpl *)impl->info_set;
    frame_impl    = (MppFrameImpl *)frame;
    info_set_impl->color_range      = frame_impl->color_range;
    info_set_impl->color_primaries  = frame_impl->color_primaries;
    info_set_impl->color_trc        = frame_impl->color_trc;
    info_set_impl->colorspace       = frame_impl->colorspace;
    info_set_impl->chroma_location  = frame_impl->chroma_location;

    if (impl->align_chk_log_en) {
        impl->align_chk_log_en = 0;
        if (legacy_info_set.h_stride_by_pixel != sys_cfg_info_set.h_stride_by_pixel)
            mpp_logi("mismatch h_stride_by_pixel %d - %d\n",
                     legacy_info_set.h_stride_by_pixel,
                     sys_cfg_info_set.h_stride_by_pixel);
        if (legacy_info_set.h_stride_by_byte != sys_cfg_info_set.h_stride_by_byte)
            mpp_logi("mismatch h_stride_by_byte %d - %d\n",
                     legacy_info_set.h_stride_by_byte,
                     sys_cfg_info_set.h_stride_by_byte);
        if (legacy_info_set.v_stride != sys_cfg_info_set.v_stride)
            mpp_logi("mismatch v_stride %d - %d\n",
                     legacy_info_set.v_stride,
                     sys_cfg_info_set.v_stride);
        if (legacy_info_set.size_total != sys_cfg_info_set.size_total)
            mpp_logi("mismatch size_total %d - %d\n",
                     legacy_info_set.size_total,
                     sys_cfg_info_set.size_total);
    }
}

#define dump_slots(...) _dump_slots(__FUNCTION__, ## __VA_ARGS__)

static void buf_slot_logs_reset(MppBufSlotLogs *logs)
{
    logs->log_count = 0;
    logs->log_write = 0;
    logs->log_read = 0;
}

static MppBufSlotLogs *buf_slot_logs_init(RK_U32 max_count)
{
    MppBufSlotLogs *logs = NULL;

    if (!max_count)
        return NULL;

    logs = mpp_malloc_size(MppBufSlotLogs, sizeof(MppBufSlotLogs) +
                           max_count * sizeof(MppBufSlotLog));
    if (!logs) {
        mpp_err_f("failed to create %d buf slot logs\n", max_count);
        return NULL;
    }

    logs->max_count = max_count;
    logs->logs = (MppBufSlotLog *)(logs + 1);
    buf_slot_logs_reset(logs);

    return logs;
}

static void buf_slot_logs_deinit(MppBufSlotLogs *logs)
{
    MPP_FREE(logs);
}

static void buf_slot_logs_write(MppBufSlotLogs *logs, RK_S32 index, MppBufSlotOps op,
                                SlotStatus before, SlotStatus after)
{
    MppBufSlotLog *log = NULL;

    log = &logs->logs[logs->log_write];
    log->index      = index;
    log->ops        = op;
    log->status_in  = before;
    log->status_out = after;

    logs->log_write++;
    if (logs->log_write >= logs->max_count)
        logs->log_write = 0;

    if (logs->log_count < logs->max_count)
        logs->log_count++;
    else {
        logs->log_read++;
        if (logs->log_read >= logs->max_count)
            logs->log_read = 0;
    }
}

static void buf_slot_logs_dump(MppBufSlotLogs *logs)
{
    while (logs->log_count) {
        MppBufSlotLog *log = &logs->logs[logs->log_read];

        mpp_log("index %2d op: %s status in %08x out %08x",
                log->index, op_string[log->ops], log->status_in.val, log->status_out.val);

        logs->log_read++;
        if (logs->log_read >= logs->max_count)
            logs->log_read = 0;
        logs->log_count--;
    }
    mpp_assert(logs->log_read == logs->log_write);
}

static void _dump_slots(const char *caller, MppBufSlotsImpl *impl)
{
    MppBufSlotEntry *slot = impl->slots;
    RK_S32 i;

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

    if (impl->logs)
        buf_slot_logs_dump(impl->logs);

    mpp_assert(0);

    return;
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
        status.hal_output++;
        status.not_ready  = 1;
    } break;
    case SLOT_CLR_HAL_OUTPUT : {
        if (status.hal_output)
            status.hal_output--;
        else
            mpp_err("can not clr hal_output on slot %d\n", slot->index);

        // NOTE: set output index ready here
        if (!status.hal_output)
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
    if (impl->logs)
        buf_slot_logs_write(impl->logs, index, op, before, status);
    if (error)
        dump_slots(impl);
}

static void init_slot_entry(MppBufSlotsImpl *impl, RK_S32 pos, RK_S32 count)
{
    MppBufSlotEntry *slot = impl->slots;
    RK_S32 i;

    for (i = 0; i < count; i++, slot++) {
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
static RK_S32 check_entry_unused(MppBufSlotsImpl *impl, MppBufSlotEntry *entry)
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
        return 1;
    }

    return 0;
}

static void clear_slots_impl(MppBufSlotsImpl *impl)
{
    MppBufSlotEntry *slot = (MppBufSlotEntry *)impl->slots;
    RK_S32 i;

    if (impl->sys_cfg)
        mpp_sys_cfg_put(impl->sys_cfg);

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

    if (impl->logs) {
        buf_slot_logs_deinit(impl->logs);
        impl->logs = NULL;
    }

    mpp_mutex_destroy(&impl->lock);

    mpp_free(impl->slots);
    mpp_free(impl);
}

MPP_RET mpp_buf_slot_init(MppBufSlots *slots)
{
    MppBufSlotsImpl *impl;

    if (!slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    impl = mpp_calloc(MppBufSlotsImpl, 1);
    if (!impl) {
        *slots = NULL;
        return MPP_NOK;
    }

    mpp_env_get_u32("buf_slot_debug", &buf_slot_debug,
                    BUF_SLOT_DBG_OPS_HISTORY | BUF_SLOT_DBG_INFO_SET);
    mpp_env_get_u32("use_legacy_align", &use_legacy_align, 0);

    do {
        if (mpp_sys_cfg_get(&impl->sys_cfg)) {
            mpp_err_f("mpp_sys_cfg_get failed\n");
            break;
        }

        mpp_mutex_init(&impl->lock);

        for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(impl->queue); i++) {
            INIT_LIST_HEAD(&impl->queue[i]);
        }

        if (buf_slot_debug & BUF_SLOT_DBG_OPS_HISTORY) {
            impl->logs = buf_slot_logs_init(SLOT_OPS_MAX_COUNT);
            if (!impl->logs)
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
        impl->info_change_slot_idx = -1;
        impl->align_chk_log_env = (buf_slot_debug & BUF_SLOT_DBG_INFO_SET) ? 1 : 0;
        impl->align_chk_log_en = impl->align_chk_log_env;

        *slots = impl;
        return MPP_OK;
    } while (0);

    clear_slots_impl(impl);

    *slots = NULL;
    return MPP_NOK;
}

MPP_RET mpp_buf_slot_deinit(MppBufSlots slots)
{
    if (!slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    clear_slots_impl((MppBufSlotsImpl *)slots);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_setup(MppBufSlots slots, RK_S32 count)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p setup: count %d\n", impl, count);

    mpp_mutex_lock(&impl->lock);

    if (!impl->slots) {
        // first slot setup
        impl->buf_count = impl->new_count = count;
        impl->slots = mpp_calloc(MppBufSlotEntry, count);
        init_slot_entry(impl, 0, count);
        impl->used_count = 0;
    } else {
        // record the slot count for info changed ready config
        if (count > impl->buf_count) {
            impl->slots = mpp_realloc(impl->slots, MppBufSlotEntry, count);
            mpp_assert(impl->slots);
            init_slot_entry(impl, impl->buf_count, (count - impl->buf_count));
        }
        impl->new_count = count;
    }

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

RK_U32 mpp_buf_slot_is_changed(MppBufSlots slots)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    RK_U32 info_changed = 0;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    mpp_mutex_lock(&impl->lock);
    info_changed = impl->info_changed;
    mpp_mutex_unlock(&impl->lock);

    return info_changed;
}

MPP_RET mpp_buf_slot_ready(MppBufSlots slots)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p is ready now\n", impl);

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, impl->slots);
    if (!impl->info_changed)
        mpp_log("found info change ready set without internal info change\n");

    // ready mean the info_set will be copy to info as the new configuration
    if (impl->buf_count != impl->new_count) {
        impl->slots = mpp_realloc(impl->slots, MppBufSlotEntry, impl->new_count);
        mpp_assert(impl->slots);
        init_slot_entry(impl, 0, impl->new_count);
    }
    impl->buf_count = impl->new_count;

    mpp_frame_copy(impl->info, impl->info_set);

    if (impl->logs)
        buf_slot_logs_reset(impl->logs);

    impl->info_changed = 0;
    impl->info_change_slot_idx = -1;

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

size_t mpp_buf_slot_get_size(MppBufSlots slots)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    size_t size = 0;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    mpp_mutex_lock(&impl->lock);
    size = impl->buf_size;
    mpp_mutex_unlock(&impl->lock);

    return size;
}

RK_S32 mpp_buf_slot_get_count(MppBufSlots slots)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    size_t count = 0;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return -1;
    }

    mpp_mutex_lock(&impl->lock);
    count = impl->buf_count;
    mpp_mutex_unlock(&impl->lock);

    return count;
}

MPP_RET mpp_buf_slot_set_callback(MppBufSlots slots, MppCbCtx *cb_ctx)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_NOK;
    }

    mpp_mutex_lock(&impl->lock);
    impl->callback = *cb_ctx;
    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_S32 *index)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;
    RK_S32 i;

    if (!impl || !index) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    slot = impl->slots;

    mpp_mutex_lock(&impl->lock);

    for (i = 0; i < impl->buf_count; i++, slot++) {
        if (!slot->status.on_used) {
            *index = i;
            slot_ops_with_log(impl, slot, SLOT_SET_ON_USE, NULL);
            slot_ops_with_log(impl, slot, SLOT_SET_NOT_READY, NULL);
            impl->used_count++;
            mpp_mutex_unlock(&impl->lock);
            return MPP_OK;
        }
    }

    *index = -1;
    mpp_err_f("failed to get a unused slot\n");
    dump_slots(impl);
    slot_assert(impl, 0);

    mpp_mutex_unlock(&impl->lock);

    return MPP_NOK;
}

MPP_RET mpp_buf_slot_set_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot_ops_with_log(impl, &impl->slots[index], set_flag_op[type], NULL);

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_clr_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;
    RK_S32 unused = 0;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot = &impl->slots[index];
    slot_ops_with_log(impl, slot, clr_flag_op[type], NULL);

    if (type == SLOT_HAL_OUTPUT)
        impl->decode_count++;

    unused = check_entry_unused(impl, slot);

    mpp_mutex_unlock(&impl->lock);

    if (unused)
        mpp_callback(&impl->callback, impl);
    return MPP_OK;
}

MPP_RET mpp_buf_slot_enqueue(MppBufSlots slots, RK_S32 index, SlotQueueType type)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot = &impl->slots[index];
    slot_ops_with_log(impl, slot, (MppBufSlotOps)(SLOT_ENQUEUE + type), NULL);

    // add slot to display list
    list_del_init(&slot->list);
    list_add_tail(&slot->list, &impl->queue[type]);

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_dequeue(MppBufSlots slots, RK_S32 *index, SlotQueueType type)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;

    if (!impl || !index) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_mutex_lock(&impl->lock);

    if (list_empty(&impl->queue[type])) {
        mpp_mutex_unlock(&impl->lock);
        return MPP_NOK;
    }

    slot = list_entry(impl->queue[type].next, MppBufSlotEntry, list);
    if (slot->status.not_ready) {
        mpp_mutex_unlock(&impl->lock);
        return MPP_NOK;
    }

    // make sure that this slot is just the next display slot
    list_del_init(&slot->list);
    slot_assert(impl, slot->index < impl->buf_count);
    slot_ops_with_log(impl, slot, (MppBufSlotOps)(SLOT_DEQUEUE + type), NULL);
    impl->display_count++;
    *index = slot->index;

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_set_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;

    if (!impl || !val || type >= SLOT_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot = &impl->slots[index];
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
        MppFrameImpl *src;
        MppFrameImpl *dst;

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

        if (!slot->frame)
            mpp_frame_init(&slot->frame);

        src = (MppFrameImpl *)frame;
        dst = (MppFrameImpl *)slot->frame;
        mpp_frame_copy(dst, src);
        // NOTE: stride from codec need to be change to hal stride
        //       hor_stride and ver_stride can not be zero
        //       they are the stride required by codec
        //       then hal will modify it according to hardware requirement
        mpp_assert(src->hor_stride);
        mpp_assert(src->ver_stride);
        dst->hor_stride = src->hor_stride;
        dst->ver_stride = src->ver_stride;
        dst->eos = slot->eos;

        if (mpp_frame_info_cmp(impl->info, impl->info_set)) {
            MppFrameImpl *old = (MppFrameImpl *)impl->info;

            impl->info_changed = 1;
            impl->info_change_slot_idx = index;

            if (impl->coding_type != MPP_VIDEO_CodingMJPEG)
                impl->align_chk_log_en = impl->align_chk_log_env;

            if (old->width || old->height) {
                mpp_dbg_info("info change found\n");
                mpp_dbg_info("old width %4d height %4d stride hor %4d ver %4d fmt %4d\n",
                             old->width, old->height, old->hor_stride,
                             old->ver_stride, old->fmt);
            }
            mpp_dbg_info("new width %4d height %4d stride hor %4d ver %4d fmt %4d\n",
                         dst->width, dst->height, dst->hor_stride, dst->ver_stride,
                         dst->fmt);
            // info change found here
        }
    } break;
    case SLOT_BUFFER: {
        MppBuffer buffer = val;

        if (slot->buffer) {
            // NOTE: reset buffer only on stream buffer slot
            slot_assert(impl, !slot->frame);
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

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_get_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;

    if (!impl || !val || type >= SLOT_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot = &impl->slots[index];

    switch (type) {
    case SLOT_EOS: {
        *(RK_U32*)val = slot->eos;
    } break;
    case SLOT_FRAME: {
        MppFrame *frame = (MppFrame *)val;
        //*frame = (slot->status.has_frame) ? (slot->frame) : (NULL);

        mpp_assert(slot->status.has_frame);
        if (slot->status.has_frame) {
            if (!*frame )
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

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_reset(MppBufSlots slots, RK_S32 index)
{
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    MppBufSlotEntry *slot;

    if (!impl || index < 0) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    buf_slot_dbg(BUF_SLOT_DBG_SETUP, "slot %p reset index %d\n", slots, index);

    mpp_mutex_lock(&impl->lock);

    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    slot = &impl->slots[index];

    // make sure that this slot is just the next display slot
    list_del_init(&slot->list);
    slot_ops_with_log(impl, slot, SLOT_CLR_QUEUE_USE, NULL);
    slot_ops_with_log(impl, slot, SLOT_DEQUEUE, NULL);
    slot_ops_with_log(impl, slot, SLOT_CLR_ON_USE, NULL);

    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_buf_slot_default_info(MppBufSlots slots, RK_S32 index, void *val)
{
    if (!slots || index < 0) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    mpp_mutex_lock(&impl->lock);
    slot_assert(impl, (index >= 0) && (index < impl->buf_count));
    MppBufSlotEntry *slot = &impl->slots[index];

    slot_assert(impl, slot->status.not_ready);
    slot_assert(impl, !slot->frame);
    slot_assert(impl, impl->info_set);

    if (!slot->frame) {
        mpp_frame_init(&slot->frame);
        mpp_frame_copy(slot->frame, impl->info_set);
    }

    MppFrame *frame = (MppFrame *)val;
    *frame = slot->frame;

    slot_ops_with_log(impl, slot, SLOT_CLR_NOT_READY, NULL);
    slot_ops_with_log(impl, slot, SLOT_SET_FRAME, slot->frame);
    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

RK_U32 mpp_slots_is_empty(MppBufSlots slots, SlotQueueType type)
{
    RK_U32 is_empty = 0;
    if (!slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    mpp_mutex_lock(&impl->lock);
    is_empty = list_empty(&impl->queue[type]) ? 1 : 0;
    mpp_mutex_unlock(&impl->lock);

    return is_empty;
}

RK_S32 mpp_slots_get_used_count(MppBufSlots slots)
{
    RK_S32 used_count = 0;
    if (!slots) {
        mpp_err_f("found NULL input\n");
        return 0;
    }
    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    mpp_mutex_lock(&impl->lock);
    used_count = impl->used_count;
    mpp_mutex_unlock(&impl->lock);

    return used_count;
}

RK_S32 mpp_slots_get_unused_count(MppBufSlots slots)
{
    RK_S32 unused_count = 0;
    if (!slots) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    mpp_mutex_lock(&impl->lock);
    slot_assert(impl, (impl->used_count >= 0) && (impl->used_count <= impl->buf_count));
    unused_count = impl->buf_count - impl->used_count;
    mpp_mutex_unlock(&impl->lock);

    return unused_count;
}

MPP_RET mpp_slots_set_prop(MppBufSlots slots, SlotsPropType type, void *val)
{
    if (!slots || !val || type >= SLOTS_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_ERR_UNKNOW;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    mpp_mutex_lock(&impl->lock);
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
        if (impl->info_change_slot_idx >= 0) {
            MppBufSlotEntry *slot = &impl->slots[impl->info_change_slot_idx];

            if (slot->frame) {
                MppFrameImpl *dst = (MppFrameImpl *)slot->frame;
                MppFrameImpl *src = (MppFrameImpl *)val;

                dst->fmt = src->fmt;
                dst->hor_stride = src->hor_stride;
                dst->hor_stride_pixel = src->hor_stride_pixel;
                dst->ver_stride = src->ver_stride;
                dst->buf_size = src->buf_size;

                if (MPP_FRAME_FMT_IS_FBC(dst->fmt) && impl->hal_fbc_adj_cfg.func)
                    impl->hal_fbc_adj_cfg.func(impl, dst, impl->hal_fbc_adj_cfg.expand);
            }

            impl->info_change_slot_idx = -1;
        }
    } break;
    case SLOTS_HAL_FBC_ADJ : {
        impl->hal_fbc_adj_cfg = *((SlotHalFbcAdjCfg *)val);
    } break;
    case SLOTS_CODING_TYPE : {
        impl->coding_type = *((MppCodingType *)val);
    } break;
    case SLOTS_WIDTH_ALIGN: {
        impl->hal_width_align = (AlignFunc)val;
    } break;
    default : {
    } break;
    }
    mpp_mutex_unlock(&impl->lock);

    return MPP_OK;
}

MPP_RET mpp_slots_get_prop(MppBufSlots slots, SlotsPropType type, void *val)
{
    if (!slots || !val || type >= SLOTS_PROP_BUTT) {
        mpp_err_f("found invalid input slots %p type %d val %p\n", slots, type, val);
        return MPP_NOK;
    }

    MppBufSlotsImpl *impl = (MppBufSlotsImpl *)slots;
    mpp_mutex_lock(&impl->lock);
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
    mpp_mutex_unlock(&impl->lock);

    return ret;
}
