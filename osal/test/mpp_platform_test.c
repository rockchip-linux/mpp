/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_plat_test"

#include "mpp_log.h"
#include "mpp_platform.h"
#include "vcodec_service.h"

int main()
{
    const char *dev = NULL;
    RK_U32 vcodec_type = mpp_get_vcodec_type();
    MppKernelVersion kernel_version = mpp_get_kernel_version();
    MppIoctlVersion ioctl_version = mpp_get_ioctl_version();

    mpp_log("kernel version: %s\n",
            kernel_version == KERNEL_UNKNOWN ? "unknown" :
            kernel_version == KERNEL_3_10    ? "3.10"    :
            kernel_version == KERNEL_4_4     ? "4.4"     :
            kernel_version == KERNEL_4_19    ? "4.19"    :
            kernel_version == KERNEL_5_10    ? "5.10"    :
            kernel_version == KERNEL_6_1     ? "6.1"     :
            NULL);
    mpp_log("ioctl  version: %s\n",
            ioctl_version == IOCTL_VCODEC_SERVICE ? "vcodec_service" :
            ioctl_version == IOCTL_MPP_SERVICE_V1 ? "mpp_service"    :
            "unknown");
    mpp_log("\n");

    mpp_log("chip name: %s\n", mpp_get_soc_name());
    mpp_log("\n");
    mpp_log("chip vcodec type %08x\n", vcodec_type);

    if (vcodec_type & (HAVE_VDPU1 | HAVE_VEPU1))
        mpp_log("found vpu1 codec\n");

    if (vcodec_type & (HAVE_VDPU2 | HAVE_VEPU2))
        mpp_log("found vpu2 codec\n");

    if (vcodec_type & HAVE_HEVC_DEC)
        mpp_log("found rk hevc decoder\n");

    if (vcodec_type & HAVE_RKVDEC)
        mpp_log("found rkvdec decoder\n");

    if (vcodec_type & HAVE_AVSDEC)
        mpp_log("found avs+ decoder\n");

    if (vcodec_type & HAVE_JPEG_DEC)
        mpp_log("found rk jpeg decoder\n");

    if (vcodec_type & HAVE_JPEG_ENC)
        mpp_log("found rk jpeg encoder\n");

    if (vcodec_type & HAVE_RKVENC)
        mpp_log("found rkvenc encoder\n");

    if (vcodec_type & HAVE_VEPU2)
        mpp_log("found vpu2 encoder\n");

    if (vcodec_type & HAVE_VEPU22)
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

    dev = mpp_get_vcodec_dev_name(MPP_CTX_DEC, MPP_VIDEO_CodingAVSPLUS);
    mpp_log("avs+  decoder: %s\n", dev);

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

