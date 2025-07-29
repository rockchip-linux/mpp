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

#include "mpp_time.h"
#include "mpp_frame.h"

typedef struct MppFrameImpl_t MppFrameImpl;

typedef union MppFrameStatus_u {
    /* total 64 bit frame status for internal flow */
    RK_U64              val;

    struct {
        /* bit 0 ~ 7 common frame status flag for both encoder and decoder */

        /*
         * data status flag
         * 0 - pixel data is invalid
         * 1 - pixel data is valid
         */
        RK_U32          valid           : 1;

        /* reference status flag */
        /*
         * 0 - inter frame
         * 1 - intra frame
         */
        RK_U32          is_intra        : 1;

        /*
         * Valid when is_intra is true
         * 0 - normal intra frame
         * 1 - IDR frame
         */
        RK_U32          is_idr          : 1;

        /*
         * 0 - mark as reference frame
         * 1 - mark as non-refernce frame
         */
        RK_U32          is_non_ref      : 1;

        /*
         * Valid when is_non_ref is false
         * 0 - mark as short-term reference frame
         * 1 - mark as long-term refernce frame
         */
        RK_U32          is_lt_ref       : 1;
        RK_U32          is_b_frame      : 1;

        /*
         * frame usage flag for decoder / encoder flow
         * 0 - mark as general frame
         * 1 - mark as used by decoder
         * 2 - mark as used by encoder
         * 4 - mark as used by vproc
         */
        RK_U32          usage           : 3;
    };
} MppFrameStatus;

struct MppFrameImpl_t {
    const char  *name;

    /*
     * dimension parameter for display
     */
    RK_U32  width;
    RK_U32  height;
    RK_U32  hor_stride;
    RK_U32  ver_stride;
    RK_U32  hor_stride_pixel;
    RK_U32  fbc_hdr_stride;
    RK_U32  offset_x;
    RK_U32  offset_y;

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
    MppFrameHdrDynamicMeta *hdr_dynamic_meta;

    /*
     * buffer information
     * NOTE: buf_size only access internally
     */
    MppBuffer       buffer;
    size_t          buf_size;

    /*
     * meta data information
     */
    MppTask         task;
    MppMeta         meta;
    MppStopwatch    stopwatch;

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
    size_t          fbc_size;

    /*
     * frame buffer contain downsacle pic
     *
     * downscale pic is no fbc fmt the downscale pic size
     * w/2 x h / 2
     */
    RK_U32          thumbnail_en;

    /*
     * frame status info for internal flow
     */
    MppFrameStatus  status;
};

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_frame_copy(MppFrame frame, MppFrame next);
MPP_RET mpp_frame_info_cmp(MppFrame frame0, MppFrame frame1);
RK_U32  mpp_frame_get_fbc_offset(MppFrame frame);
RK_U32  mpp_frame_get_fbc_stride(MppFrame frame);
size_t  mpp_frame_get_fbc_size(MppFrame frame);
void    mpp_frame_set_fbc_size(MppFrame frame, size_t size);

MppFrameStatus *mpp_frame_get_status(MppFrame frame);

/*
 * Debug for frame process timing
 */
void mpp_frame_set_stopwatch_enable(MppFrame frame, RK_S32 enable);
MppStopwatch mpp_frame_get_stopwatch(const MppFrame frame);

MPP_RET __check_is_mpp_frame(void *frame);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_FRAME_IMPL_H__*/
