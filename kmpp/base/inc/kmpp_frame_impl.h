/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FRAME_IMPL_H__
#define __KMPP_FRAME_IMPL_H__

#include "kmpp_frame.h"

typedef struct KmppFrameImpl_t {
    const char *name;
    KmppObj obj;

    /*
     * dimension parameter for display
     */
    rk_u32 width;
    rk_u32 height;
    rk_u32 hor_stride;
    rk_u32 ver_stride;
    rk_u32 hor_stride_pixel;
    rk_u32 offset_x;
    rk_u32 offset_y;

    /*
     * poc - picture order count
     */
    rk_u32 poc;
    /*
     * pts - display time stamp
     * dts - decode time stamp
     */
    rk_s64 pts;
    rk_s64 dts;

    /*
        * eos - end of stream
        * info_change - set when buffer resized or frame infomation changed
        */
    rk_u32 eos;
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

    /*
     * buffer information
     * NOTE: buf_size only access internally
     */
    KmppShmPtr buffer;
    size_t buf_size;
    RK_U32 buf_fd;
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
    rk_u32 fbc_offset;
    rk_u32 is_gray;

    KmppShmPtr meta;
    KmppMeta self_meta;
} KmppFrameImpl;

#endif /* __KMPP_FRAME_IMPL_H__ */
