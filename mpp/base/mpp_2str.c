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
        "h.263",
        "mpeg4",
        "wmv",
        "rv",
        "avc",
        "mjpeg",
        "vp8",
        "vp9",
    };
    static const char *coding_type_str1[] = {
        "vc1",
        "flv1",
        "divx3",
        "vp6",
        "hevc",
        "avs+",
        "avs",
    };

    if (coding >= MPP_VIDEO_CodingUnused && coding <= MPP_VIDEO_CodingVP9)
        return coding_type_str0[coding];
    else if (coding >= MPP_VIDEO_CodingVC1 && coding <= MPP_VIDEO_CodingAVS)
        return coding_type_str1[coding - MPP_VIDEO_CodingVC1];

    return NULL;
}
