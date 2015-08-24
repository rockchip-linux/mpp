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

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_frame_impl.h"

MPP_FRAME_ACCESSORS(MppFrameImpl, RK_U32, width)
MPP_FRAME_ACCESSORS(MppFrameImpl, RK_U32, height)
MPP_FRAME_ACCESSORS(MppFrameImpl, RK_U32, hor_stride)
MPP_FRAME_ACCESSORS(MppFrameImpl, RK_U32, ver_stride)
MPP_FRAME_ACCESSORS(MppFrameImpl, RK_U32, mode)
MPP_FRAME_ACCESSORS(MppFrameImpl, RK_S64, pts)
MPP_FRAME_ACCESSORS(MppFrameImpl, RK_S64, dts)
MPP_FRAME_ACCESSORS(MppFrameImpl, MppFrameColorRange, color_range)
MPP_FRAME_ACCESSORS(MppFrameImpl, MppFrameColorPrimaries, color_primaries)
MPP_FRAME_ACCESSORS(MppFrameImpl, MppFrameColorTransferCharacteristic, color_trc)
MPP_FRAME_ACCESSORS(MppFrameImpl, MppFrameColorSpace, colorspace)
MPP_FRAME_ACCESSORS(MppFrameImpl, MppFrameChromaLocation, chroma_location)
MPP_FRAME_ACCESSORS(MppFrameImpl, MppBuffer, buffer)

MPP_RET mpp_frame_init(MppFrame *frame)
{
    if (NULL == frame) {
        mpp_err("mpp_frame_init invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppFrameImpl *p = mpp_calloc(MppFrameImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_frame_init malloc failed\n");
        return MPP_ERR_NULL_PTR;
    }

    *frame = p;

    return MPP_OK;
}

MPP_RET mpp_frame_deinit(MppFrame frame)
{
    if (NULL == frame) {
        mpp_err("mpp_frame_deinit invalid NULL pointer input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_free(frame);

    return MPP_OK;
}

