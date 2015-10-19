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

#define MODULE_TAG "mpp_frame"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_frame_impl.h"

static const char *module_name = MODULE_TAG;

static void setup_mpp_frame_name(MppFrameImpl *frame)
{
    frame->name = module_name;
}

static void check_mpp_frame_name(MppFrameImpl *frame)
{
    mpp_assert(frame->name == module_name);
    if (frame->name != module_name)
        abort();
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
    if (NULL == frame || NULL == *frame) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    check_mpp_frame_name((MppFrameImpl *)*frame);
    MppBuffer buffer = mpp_frame_get_buffer(*frame);
    if (buffer)
        mpp_buffer_put(buffer);

    mpp_free(*frame);
    *frame = NULL;
    return MPP_OK;
}

MppFrame mpp_frame_get_next(MppFrame frame)
{
    if (NULL == frame) {
        mpp_err_f("invalid NULL pointer input\n");
        return NULL;
    }

    MppFrameImpl *p = (MppFrameImpl *)frame;
    check_mpp_frame_name(p);
    return (MppFrame)p->next;
}

MPP_RET mpp_frame_set_next(MppFrame frame, MppFrame next)
{
    if (NULL == frame) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppFrameImpl *p = (MppFrameImpl *)frame;
    check_mpp_frame_name(p);
    p->next = (MppFrameImpl *)next;
    return MPP_OK;
}

MPP_RET mpp_frame_copy(MppFrame dst, MppFrame src)
{
    if (NULL == dst || NULL == src) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    check_mpp_frame_name((MppFrameImpl *)dst);
    memcpy(dst, src, sizeof(MppFrameImpl));
    return MPP_OK;
}

MPP_RET mpp_frame_info_cmp(MppFrame frame0, MppFrame frame1)
{
    if (NULL == frame0 || NULL == frame0) {
        mpp_err_f("invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppFrameImpl *f0 = (MppFrameImpl *)frame0;
    MppFrameImpl *f1 = (MppFrameImpl *)frame1;
    check_mpp_frame_name(f0);
    check_mpp_frame_name(f1);

    if ((f0->width              == f1->width)  &&
        (f0->height             == f1->height) &&
        (f0->hor_stride         == f1->hor_stride) &&
        (f0->ver_stride         == f1->ver_stride) &&
        (f0->buf_size           == f1->buf_size) &&
        (f0->color_range        == f1->color_range) &&
        (f0->color_primaries    == f1->color_primaries) &&
        (f0->color_trc          == f1->color_trc) &&
        (f0->colorspace         == f1->colorspace) &&
        (f0->chroma_location    == f1->chroma_location)) {
        return MPP_OK;
    }
    return MPP_NOK;
}

/*
 * object access function macro
 */
#define MPP_FRAME_ACCESSORS(type, field) \
    type mpp_frame_get_##field(const MppFrame s) \
    { \
        check_mpp_frame_name((MppFrameImpl*)s); \
        return ((MppFrameImpl*)s)->field; \
    } \
    void mpp_frame_set_##field(MppFrame s, type v) \
    { \
        check_mpp_frame_name((MppFrameImpl*)s); \
        ((MppFrameImpl*)s)->field = v; \
    }

MPP_FRAME_ACCESSORS(RK_U32, width)
MPP_FRAME_ACCESSORS(RK_U32, height)
MPP_FRAME_ACCESSORS(RK_U32, hor_stride)
MPP_FRAME_ACCESSORS(RK_U32, ver_stride)
MPP_FRAME_ACCESSORS(RK_U32, mode)
MPP_FRAME_ACCESSORS(RK_S64, pts)
MPP_FRAME_ACCESSORS(RK_S64, dts)
MPP_FRAME_ACCESSORS(RK_U32, eos)
MPP_FRAME_ACCESSORS(RK_U32, info_change)
MPP_FRAME_ACCESSORS(MppFrameColorRange, color_range)
MPP_FRAME_ACCESSORS(MppFrameColorPrimaries, color_primaries)
MPP_FRAME_ACCESSORS(MppFrameColorTransferCharacteristic, color_trc)
MPP_FRAME_ACCESSORS(MppFrameColorSpace, colorspace)
MPP_FRAME_ACCESSORS(MppFrameChromaLocation, chroma_location)
MPP_FRAME_ACCESSORS(MppBuffer, buffer)
MPP_FRAME_ACCESSORS(size_t, buf_size)

