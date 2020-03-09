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

#ifndef __MPP_FRAME_IMPL_H__
#define __MPP_FRAME_IMPL_H__

#include "mpp_frame.h"

typedef struct MppFrameImpl_t MppFrameImpl;

struct MppFrameImpl_t {
    const char  *name;

    /*
     * dimension parameter for display
     */
    RK_U32  width;
    RK_U32  height;
    RK_U32  hor_stride;
    RK_U32  ver_stride;

    /*
     * interlaced related mode status
     *
     * 0 - frame
     * 1 - top field
     * 2 - bottom field
     * 3 - paired top and bottom field
     * 4 - deinterlaced flag
     * 7 - deinterlaced paired field
     */
    RK_U32  mode;
    /*
     * current decoded frame whether to display
     *
     * 0 - reserve
     * 1 - discard
     */
    RK_U32  discard;
    /*
     * send decoded frame belong which view
     */
    RK_U32  viewid;
    /*
    * poc - picture order count
    */
    RK_U32  poc;
    /*
     * pts - display time stamp
     * dts - decode time stamp
     */
    RK_S64  pts;
    RK_S64  dts;

    /*
     * eos - end of stream
     * info_change - set when buffer resized or frame infomation changed
     */
    RK_U32  eos;
    RK_U32  info_change;
    RK_U32  errinfo;
    MppFrameColorRange color_range;
    MppFrameColorPrimaries color_primaries;
    MppFrameColorTransferCharacteristic color_trc;

    /**
     * YUV colorspace type.
     * It must be accessed using av_frame_get_colorspace() and
     * av_frame_set_colorspace().
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    MppFrameColorSpace colorspace;
    MppFrameChromaLocation chroma_location;

    MppFrameFormat fmt;

    MppFrameRational sar;
    MppFrameMasteringDisplayMetadata mastering_display;
    MppFrameContentLightMetadata content_light;

    /*
     * buffer information
     * NOTE: buf_size only access internally
     */
    MppBuffer       buffer;
    size_t          buf_size;

    /*
     * meta data information
     */
    MppMeta         meta;

    /*
     * frame buffer compression (FBC) information
     *
     * NOTE: some constraint on fbc data
     * 1. FBC config need two addresses but only one buffer.
     *    The second address should be represented by base + offset form.
     * 2. FBC has header address and payload address
     *    Both addresses should be 4K aligned.
     * 3. The header section size is default defined by:
     *    header size = aligned(aligned(width, 16) * aligned(height, 16) / 16, 4096)
     * 4. The stride in header section is defined by:
     *    stride = aligned(width, 16)
     */
    RK_U32          fbc_offset;

    /*
     * pointer for multiple frame output at one time
     */
    MppFrameImpl    *next;
};

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_frame_set_next(MppFrame frame, MppFrame next);
MPP_RET mpp_frame_copy(MppFrame frame, MppFrame next);
MPP_RET mpp_frame_info_cmp(MppFrame frame0, MppFrame frame1);
RK_U32  mpp_frame_get_fbc_offset(MppFrame frame);
RK_U32  mpp_frame_get_fbc_stride(MppFrame frame);

MPP_RET check_is_mpp_frame(void *pointer);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_FRAME_IMPL_H__*/
