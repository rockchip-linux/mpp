/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#include <string.h>
#include "rk_hdr_meta_com.h"
#include "mpp_common.h"
#include "mpp_log.h"
#include "mpp_frame.h"

/* h(104) + d(100) + r(114) */
#define HDR_META_MAGIC 318

static RK_U32 hdr_get_offset_from_frame(MppFrame frame)
{
    return mpp_frame_get_buf_size(frame);
}

void fill_hdr_meta_to_frame(MppFrame frame, MppCodingType in_type)
{
    RK_U32 off = hdr_get_offset_from_frame(frame);
    MppBuffer buf = mpp_frame_get_buffer(frame);
    void *ptr = mpp_buffer_get_ptr(buf);
    MppFrameHdrDynamicMeta *dynamic_meta = mpp_frame_get_hdr_dynamic_meta(frame);
    MppFrameMasteringDisplayMetadata mastering_display = mpp_frame_get_mastering_display(frame);
    MppFrameContentLightMetadata content_light = mpp_frame_get_content_light(frame);
    RkMetaHdrHeader *hdr_static_meta_header;
    RkMetaHdrHeader *hdr_dynamic_meta_header;
    RK_U32 msg_idx = 0;
    RK_U16 hdr_format = HDR_NONE;
    MppMeta meta = NULL;
    RK_U32 max_size = mpp_buffer_get_size(buf);
    RK_U32 static_size, dynamic_size = 0, total_size = 0;
    HdrCodecType codec_type = HDR_CODEC_UNSPECIFIED;

    if (!ptr || !buf) {
        mpp_err_f("buf is null!\n");
        return;
    }

    if (mpp_frame_get_thumbnail_en(frame) == MPP_FRAME_THUMBNAIL_ONLY) {
        // only for 8K thumbnail downscale to 4K 8bit mode
        RK_U32 downscale_width = mpp_frame_get_width(frame) / 2;
        RK_U32 downscale_height = mpp_frame_get_height(frame) / 2;

        off = downscale_width * downscale_height * 3 / 2;
    }
    off = MPP_ALIGN(off, SZ_4K);

    static_size = sizeof(RkMetaHdrHeader) + sizeof(HdrStaticMeta);
    if (dynamic_meta && dynamic_meta->size)
        dynamic_size = sizeof(RkMetaHdrHeader) + dynamic_meta->size;

    total_size = static_size + dynamic_size;

    if ((off + total_size) > max_size) {
        mpp_err_f("fill hdr meta overflow off %d size %d max %d\n",
                  off, total_size, max_size);
        return;
    }
    meta = mpp_frame_get_meta(frame);
    mpp_meta_set_s32(meta, KEY_HDR_META_OFFSET, off);
    /* 1. fill hdr static meta date */
    hdr_static_meta_header = (RkMetaHdrHeader*)(ptr + off);
    /* For transmission */
    hdr_static_meta_header->magic = HDR_META_MAGIC;
    hdr_static_meta_header->size = static_size;
    hdr_static_meta_header->message_index = msg_idx++;

    switch (in_type) {
    case MPP_VIDEO_CodingAVS2 : {
        codec_type = HDR_AVS2;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        codec_type = HDR_HEVC;
    } break;
    case MPP_VIDEO_CodingAVC : {
        codec_type = HDR_H264;
    } break;
    case MPP_VIDEO_CodingAV1 : {
        codec_type = HDR_AV1;
    } break;
    default : break;
    }

    /* For payload identification */
    hdr_static_meta_header->hdr_payload_type = STATIC;
    hdr_static_meta_header->video_format = codec_type;
    {
        HdrStaticMeta *static_meta = (HdrStaticMeta*)hdr_static_meta_header->payload;

        static_meta->min_luminance = mastering_display.min_luminance;
        static_meta->max_luminance = mastering_display.max_luminance;
        static_meta->green_x = mastering_display.display_primaries[0][0];
        static_meta->green_y = mastering_display.display_primaries[0][1];
        static_meta->blue_x = mastering_display.display_primaries[1][0];
        static_meta->blue_y = mastering_display.display_primaries[1][1];
        static_meta->red_x = mastering_display.display_primaries[2][0];
        static_meta->red_y = mastering_display.display_primaries[2][1];
        static_meta->white_point_x = mastering_display.white_point[0];
        static_meta->white_point_y = mastering_display.white_point[1];
        static_meta->color_trc = mpp_frame_get_color_trc(frame);
        static_meta->color_space = mpp_frame_get_colorspace(frame);
        static_meta->color_primaries = mpp_frame_get_color_primaries(frame);
        static_meta->max_cll = content_light.MaxCLL;
        static_meta->max_fall = content_light.MaxFALL;
        /*
         * hlg:
         *  hevc trc = 18
         *  avs trc = 14
         * hdr10:
         *  hevc/h264 trc = 16
         *  avs trc = 12
         */
        switch (codec_type) {
        case HDR_AV1 :
        case HDR_HEVC :
        case HDR_H264 : {
            if (static_meta->color_trc == MPP_FRAME_TRC_ARIB_STD_B67)
                hdr_format = HLG;
            else if (static_meta->color_trc == MPP_FRAME_TRC_SMPTEST2084)
                hdr_format = HDR10;
        } break;
        case HDR_AVS2 : {
            if (static_meta->color_trc == MPP_FRAME_TRC_BT2020_10)
                hdr_format = HLG;
            else if (static_meta->color_trc == MPP_FRAME_TRC_BT1361_ECG)
                hdr_format = HDR10;
        } break;
        default : {
        } break;
        }
    }
    off += hdr_static_meta_header->size;

    /* 2. fill hdr dynamic meta date */
    if (dynamic_meta && dynamic_meta->size) {
        hdr_dynamic_meta_header = (RkMetaHdrHeader*)(ptr + off);

        /* For transmission */
        hdr_dynamic_meta_header->magic = HDR_META_MAGIC;
        hdr_dynamic_meta_header->size = dynamic_size;
        hdr_dynamic_meta_header->message_index = msg_idx++;

        /* For payload identification */
        hdr_dynamic_meta_header->hdr_payload_type = DYNAMIC;
        hdr_dynamic_meta_header->video_format = codec_type;
        hdr_format = dynamic_meta->hdr_fmt;

        memcpy(hdr_dynamic_meta_header->payload, dynamic_meta->data, dynamic_meta->size);
        hdr_dynamic_meta_header->message_total = msg_idx;
        hdr_dynamic_meta_header->hdr_format = hdr_format;
    }

    mpp_meta_set_s32(meta, KEY_HDR_META_SIZE, total_size);
    hdr_static_meta_header->message_total = msg_idx;
    hdr_static_meta_header->hdr_format = hdr_format;
}