/*
 *
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

#include "mpp_2str.h"
#include "h264_syntax.h"
#include "h265_syntax.h"

const char *strof_ctx_type(MppCtxType type)
{
    static const char *ctx_type_str[MPP_CTX_BUTT] = {
        "dec",
        "enc",
        "isp",
    };

    return ctx_type_str[type];
}

const char *strof_coding_type(MppCodingType coding)
{
    static const char *coding_type_str0[] = {
        "unused",
        "autodetect",
        "mpeg2",
        "h263",
        "mpeg4",
        "wmv",
        "rv",
        "h264",
        "mjpeg",
        "vp8",
        "vp9",
    };
    static const char *coding_type_str1[] = {
        "vc1",
        "flv1",
        "divx3",
        "vp6",
        "h265",
        "avs+",
        "avs",
    };

    if (coding >= MPP_VIDEO_CodingUnused && coding <= MPP_VIDEO_CodingVP9)
        return coding_type_str0[coding];
    else if (coding >= MPP_VIDEO_CodingVC1 && coding <= MPP_VIDEO_CodingAVS)
        return coding_type_str1[coding - MPP_VIDEO_CodingVC1];

    return NULL;
}

const char *strof_rc_mode(MppEncRcMode rc_mode)
{
    static const char *rc_mode_str[] = {
        "vbr",
        "cbr",
        "fixqp",
        "avbr",
    };

    if (rc_mode >= MPP_ENC_RC_MODE_VBR && rc_mode < MPP_ENC_RC_MODE_BUTT)
        return rc_mode_str[rc_mode];

    return NULL;
}
const char *strof_gop_mode(MppEncRcGopMode gop_mode)
{
    static const char *gop_mode_str[] = {
        "normalp",
        "smartp",
    };

    if (gop_mode >= MPP_ENC_RC_NORMAL_P && gop_mode < MPP_ENC_RC_GOP_MODE_BUTT)
        return gop_mode_str[gop_mode];

    return NULL;
}

const char *strof_profle(MppCodingType coding, RK_U32 profile)
{
    static const char *h264_profile_str[] = {
        "baseline",
        "main",
        "high",
        "high10",
    };
    static const char *h265_profile_str[] = {
        "main",
        "main10",
    };
    static const char *jpeg_profile_str[] = {
        "base",
    };
    static const char *vp8_profile_str[] = {
        "base",
    };
    static const char *unknown_str[] = {
        "unknown",
    };

    switch (coding) {
    case MPP_VIDEO_CodingAVC : {
        if (profile == H264_PROFILE_BASELINE)
            return h264_profile_str[0];
        else if (profile == H264_PROFILE_MAIN)
            return h264_profile_str[1];
        else if (profile == H264_PROFILE_HIGH)
            return h264_profile_str[2];
        else if (profile == H264_PROFILE_HIGH10)
            return h264_profile_str[3];
        else
            return unknown_str[0];
    } break;
    case MPP_VIDEO_CodingHEVC : {
        if (profile < 2)
            return h265_profile_str[0];
        else
            return unknown_str[0];
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        return jpeg_profile_str[0];
    } break;
    case MPP_VIDEO_CodingVP8 : {
        return vp8_profile_str[0];
    } break;
    default : {
    } break;
    }

    return NULL;
}
