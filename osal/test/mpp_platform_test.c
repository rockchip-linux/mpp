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

#define MODULE_TAG "mpp_plat_test"

#include "mpp_log.h"
#include "mpp_platform.h"

int main()
{
    const char *dev = NULL;
    RK_U32 vcodec_type = mpp_get_vcodec_type();

    mpp_log("chip name: %s\n", mpp_get_soc_name());
    mpp_log("\n");
    mpp_log("chip vcodec type %08x\n", vcodec_type);

    if (vcodec_type & HAVE_VPU1)
        mpp_log("found vpu1 codec\n");

    if (vcodec_type & HAVE_VPU2)
        mpp_log("found vpu2 codec\n");

    if (vcodec_type & HAVE_HEVC_DEC)
        mpp_log("found RK hevc decoder\n");

    if (vcodec_type & HAVE_RKVDEC)
        mpp_log("found rkvdec decoder\n");

    if (vcodec_type & HAVE_AVSDEC)
        mpp_log("found avs+ decoder\n");

    if (vcodec_type & HAVE_RKVENC)
        mpp_log("found rkvenc encoder\n");

    if (vcodec_type & HAVE_VEPU)
        mpp_log("found vpu2 encoder\n");

    if (vcodec_type & HAVE_H265ENC)
        mpp_log("found h265 stand-alone encoder\n");

    mpp_log("\n");
    mpp_log("start probing decoder device name:\n");

    dev = mpp_get_vcodec_dev_name(MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
    mpp_log("H.264 decoder: %s\n", dev);

    dev = mpp_get_vcodec_dev_name(MPP_CTX_DEC, MPP_VIDEO_CodingHEVC);
    mpp_log("H.265 decoder: %s\n", dev);

    dev = mpp_get_vcodec_dev_name(MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG);
    mpp_log("MJPEG decoder: %s\n", dev);

    dev = mpp_get_vcodec_dev_name(MPP_CTX_DEC, MPP_VIDEO_CodingVP9);
    mpp_log("VP9   decoder: %s\n", dev);

    dev = mpp_get_vcodec_dev_name(MPP_CTX_DEC, MPP_VIDEO_CodingAVS);
    mpp_log("avs   decoder: %s\n", dev);

    mpp_log("\n");
    mpp_log("start probing encoder device name:\n");

    dev = mpp_get_vcodec_dev_name(MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
    mpp_log("H.264 encoder: %s\n", dev);

    dev = mpp_get_vcodec_dev_name(MPP_CTX_ENC, MPP_VIDEO_CodingHEVC);
    mpp_log("H.265 encoder: %s\n", dev);

    dev = mpp_get_vcodec_dev_name(MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG);
    mpp_log("MJPEG encoder: %s\n", dev);

    mpp_log("mpp platform test done\n");

    return 0;
}

