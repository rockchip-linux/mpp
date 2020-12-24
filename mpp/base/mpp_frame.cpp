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

#define MODULE_TAG "mpp_frame"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"
#include "mpp_meta_impl.h"

static const char *module_name = MODULE_TAG;

static void setup_mpp_frame_name(MppFrameImpl *frame)
{
    frame->name = module_name;
}

MPP_RET check_is_mpp_frame(void *frame)
{
    if (frame && ((MppFrameImpl*)frame)->name == module_name)
        return MPP_OK;

    mpp_err_f("pointer %p failed on check\n", frame);
    mpp_abort();
    return MPP_NOK;
}

MPP_RET mpp_frame_init(MppFrame *frame)
{
    if (NULL == frame) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppFrameImpl *p = mpp_calloc(MppFrameImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_NULL_PTR;
    }

    setup_mpp_frame_name(p);
    *frame = p;

    return MPP_OK;
}

MPP_RET mpp_frame_deinit(MppFrame *frame)
{
    if (NULL == frame || check_is_mpp_frame(*frame)) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppFrameImpl *p = (MppFrameImpl *)*frame;
    if (p->buffer)
        mpp_buffer_put(p->buffer);

    if (p->meta)
        mpp_meta_put(p->meta);

    if (p->stopwatch)
        mpp_stopwatch_put(p->stopwatch);

    mpp_free(*frame);
    *frame = NULL;
    return MPP_OK;
}

MppFrame mpp_frame_get_next(MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return NULL;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    return (MppFrame)p->next;
}

MPP_RET mpp_frame_set_next(MppFrame frame, MppFrame next)
{
    if (check_is_mpp_frame(frame))
        return MPP_ERR_UNKNOW;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    p->next = (MppFrameImpl *)next;
    return MPP_OK;
}

MppBuffer mpp_frame_get_buffer(MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return NULL;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    return (MppFrame)p->buffer;
}

void mpp_frame_set_buffer(MppFrame frame, MppBuffer buffer)
{
    if (check_is_mpp_frame(frame))
        return ;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    if (p->buffer != buffer) {
        if (buffer)
            mpp_buffer_inc_ref(buffer);

        if (p->buffer)
            mpp_buffer_put(p->buffer);

        p->buffer = buffer;
    }
}

RK_S32 mpp_frame_has_meta(const MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return 0;

    MppFrameImpl *p = (MppFrameImpl *)frame;

    return (NULL != p->meta);
}

MppMeta mpp_frame_get_meta(MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return NULL;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    if (NULL == p->meta)
        mpp_meta_get(&p->meta);

    return p->meta;
}

void mpp_frame_set_meta(MppFrame frame, MppMeta meta)
{
    if (check_is_mpp_frame(frame))
        return ;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    if (p->meta) {
        mpp_meta_put(p->meta);
        p->meta = NULL;
    }

    p->meta = meta;
}

void mpp_frame_set_stopwatch_enable(MppFrame frame, RK_S32 enable)
{
    if (check_is_mpp_frame(frame))
        return ;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    if (enable && NULL == p->stopwatch) {
        char name[32];

        snprintf(name, sizeof(name) - 1, "frm %8llx", p->pts);
        p->stopwatch = mpp_stopwatch_get(name);
        if (p->stopwatch)
            mpp_stopwatch_set_show_on_exit(p->stopwatch, 1);
    } else if (!enable && p->stopwatch) {
        mpp_stopwatch_put(p->stopwatch);
        p->stopwatch = NULL;
    }
}

MppStopwatch mpp_frame_get_stopwatch(const MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return NULL;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    return p->stopwatch;
}

MPP_RET mpp_frame_copy(MppFrame dst, MppFrame src)
{
    if (NULL == dst || check_is_mpp_frame(src)) {
        mpp_err_f("invalid input dst %p src %p\n", dst, src);
        return MPP_ERR_UNKNOW;
    }

    memcpy(dst, src, sizeof(MppFrameImpl));
    MppFrameImpl *p = (MppFrameImpl *)src;
    if (p->meta)
        mpp_meta_inc_ref(p->meta);

    return MPP_OK;
}

MPP_RET mpp_frame_info_cmp(MppFrame frame0, MppFrame frame1)
{
    if (check_is_mpp_frame(frame0) || check_is_mpp_frame(frame0)) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppFrameImpl *f0 = (MppFrameImpl *)frame0;
    MppFrameImpl *f1 = (MppFrameImpl *)frame1;

    if ((f0->width              == f1->width)  &&
        (f0->height             == f1->height) &&
        (f0->hor_stride         == f1->hor_stride) &&
        (f0->ver_stride         == f1->ver_stride) &&
        (f0->fmt                == f1->fmt) &&
        (f0->buf_size           == f1->buf_size)) {
        return MPP_OK;
    }
    return MPP_NOK;
}

RK_U32 mpp_frame_get_fbc_offset(MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return 0;

    MppFrameImpl *p = (MppFrameImpl *)frame;

    if (MPP_FRAME_FMT_IS_FBC(p->fmt)) {
        RK_U32 fbc_version = p->fmt & MPP_FRAME_FBC_MASK;
        RK_U32 fbc_offset = 0;

        if (fbc_version == MPP_FRAME_FBC_AFBC_V1) {
            fbc_offset = MPP_ALIGN(MPP_ALIGN(p->width, 16) *
                                   MPP_ALIGN(p->height, 16) / 16, SZ_4K);
        } else if (fbc_version == MPP_FRAME_FBC_AFBC_V2) {
            fbc_offset = 0;
        }
        p->fbc_offset = fbc_offset;
    }

    return p->fbc_offset;
}

RK_U32 mpp_frame_get_fbc_stride(MppFrame frame)
{
    if (check_is_mpp_frame(frame))
        return 0;

    MppFrameImpl *p = (MppFrameImpl *)frame;
    return MPP_ALIGN(p->width, 16);
}

/*
 * object access function macro
 */
#define MPP_FRAME_ACCESSORS(type, field) \
    type mpp_frame_get_##field(const MppFrame s) \
    { \
        check_is_mpp_frame((MppFrameImpl*)s); \
        return ((MppFrameImpl*)s)->field; \
    } \
    void mpp_frame_set_##field(MppFrame s, type v) \
    { \
        check_is_mpp_frame((MppFrameImpl*)s); \
        ((MppFrameImpl*)s)->field = v; \
    }

MPP_FRAME_ACCESSORS(RK_U32, width)
MPP_FRAME_ACCESSORS(RK_U32, height)
MPP_FRAME_ACCESSORS(RK_U32, hor_stride_pixel)
MPP_FRAME_ACCESSORS(RK_U32, hor_stride)
MPP_FRAME_ACCESSORS(RK_U32, ver_stride)
MPP_FRAME_ACCESSORS(RK_U32, offset_x)
MPP_FRAME_ACCESSORS(RK_U32, offset_y)
MPP_FRAME_ACCESSORS(RK_U32, mode)
MPP_FRAME_ACCESSORS(RK_U32, discard)
MPP_FRAME_ACCESSORS(RK_U32, viewid)
MPP_FRAME_ACCESSORS(RK_U32, poc)
MPP_FRAME_ACCESSORS(RK_S64, pts)
MPP_FRAME_ACCESSORS(RK_S64, dts)
MPP_FRAME_ACCESSORS(RK_U32, eos)
MPP_FRAME_ACCESSORS(RK_U32, info_change)
MPP_FRAME_ACCESSORS(MppFrameColorRange, color_range)
MPP_FRAME_ACCESSORS(MppFrameColorPrimaries, color_primaries)
MPP_FRAME_ACCESSORS(MppFrameColorTransferCharacteristic, color_trc)
MPP_FRAME_ACCESSORS(MppFrameColorSpace, colorspace)
MPP_FRAME_ACCESSORS(MppFrameChromaLocation, chroma_location)
MPP_FRAME_ACCESSORS(MppFrameFormat, fmt)
MPP_FRAME_ACCESSORS(MppFrameRational, sar)
MPP_FRAME_ACCESSORS(MppFrameMasteringDisplayMetadata, mastering_display)
MPP_FRAME_ACCESSORS(MppFrameContentLightMetadata, content_light)
MPP_FRAME_ACCESSORS(size_t, buf_size)
MPP_FRAME_ACCESSORS(RK_U32, errinfo)
