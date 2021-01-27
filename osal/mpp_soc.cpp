/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_soc"

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp_soc.h"
#include "mpp_platform.h"

#define MAX_SOC_NAME_LENGTH     128

#define CODING_TO_IDX(type)   \
    ((RK_U32)(type) >= (RK_U32)MPP_VIDEO_CodingKhronosExtensions) ? \
    ((RK_U32)(-1)) : \
    ((RK_U32)(type) >= (RK_U32)MPP_VIDEO_CodingVC1) ? \
    ((RK_U32)(type) - (RK_U32)MPP_VIDEO_CodingVC1 + 16) : \
    ((RK_U32)(type) - (RK_U32)MPP_VIDEO_CodingUnused)

#define HAVE_MPEG2  ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingMPEG2))))
#define HAVE_H263   ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingH263))))
#define HAVE_MPEG4  ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingMPEG4))))
#define HAVE_AVC    ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVC))))
#define HAVE_MJPEG  ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingMJPEG))))
#define HAVE_VP8    ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingVP8))))
#define HAVE_VP9    ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingVP9))))
#define HAVE_HEVC   ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingHEVC))))
#define HAVE_AVSP   ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVSPLUS))))
#define HAVE_AVS    ((RK_U32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVS))))

#define CAP_CODING_VDPU         (HAVE_MPEG2|HAVE_H263|HAVE_MPEG4|HAVE_AVC|HAVE_MJPEG|HAVE_VP8)
#define CAP_CODING_JPEGD_PP     (HAVE_MJPEG)
#define CAP_CODING_AVSD         (HAVE_AVS)
#define CAP_CODING_HEVC         (HAVE_HEVC)
#define CAP_CODING_VDPU341      (HAVE_AVC|HAVE_HEVC|HAVE_VP9)
#define CAP_CODING_VDPU341_LITE (HAVE_AVC|HAVE_HEVC)

#define CAP_CODING_VEPU1        (HAVE_AVC|HAVE_MJPEG|HAVE_VP8)
#define CAP_CODING_VEPU_LITE    (HAVE_AVC|HAVE_MJPEG)
#define CAP_CODING_VEPU22       (HAVE_HEVC)
#define CAP_CODING_VEPU54X      (HAVE_AVC|HAVE_HEVC)

static const MppDecHwCap vdpu1 = {
    .cap_coding         = CAP_CODING_VDPU,
    .type               = VPU_CLIENT_VDPU1,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu1_2160p = {
    .cap_coding         = CAP_CODING_VDPU,
    .type               = VPU_CLIENT_VDPU1,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu1_jpeg_pp = {
    .cap_coding         = CAP_CODING_JPEGD_PP,
    .type               = VPU_CLIENT_VDPU1_PP,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 1,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2 = {
    .cap_coding         = CAP_CODING_VDPU,
    .type               = VPU_CLIENT_VDPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2_jpeg = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_VDPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2_jpeg_pp = {
    .cap_coding         = CAP_CODING_JPEGD_PP,
    .type               = VPU_CLIENT_VDPU2_PP,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 1,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap rk_hevc = {
    .cap_coding         = CAP_CODING_HEVC,
    .type               = VPU_CLIENT_HEVC_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .reserved           = 0,
};

static const MppDecHwCap rk_hevc_1080p = {
    .cap_coding         = CAP_CODING_HEVC,
    .type               = VPU_CLIENT_HEVC_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341 = {
    .cap_coding         = CAP_CODING_VDPU341,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341_lite = {
    .cap_coding         = CAP_CODING_VDPU341_LITE,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341_lite_1080p = {
    .cap_coding         = CAP_CODING_VDPU341_LITE,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341_h264 = {
    .cap_coding         = HAVE_AVC,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

/* vdpu34x support AFBC_V2 output */
static const MppDecHwCap vdpu34x = {
    .cap_coding         = CAP_CODING_VDPU341,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .reserved           = 0,
};

static const MppDecHwCap avsd = {
    .cap_coding         = CAP_CODING_AVSD,
    .type               = VPU_CLIENT_AVSPLUS_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppDecHwCap rkjpegd = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_JPEG_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_buf      = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .reserved           = 0,
};

static const MppEncHwCap vepu1 = {
    .cap_coding         = CAP_CODING_VEPU1,
    .type               = VPU_CLIENT_VEPU1,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 0,
    .reserved           = 0,
};

static const MppEncHwCap vepu2 = {
    .cap_coding         = CAP_CODING_VEPU1,
    .type               = VPU_CLIENT_VEPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 0,
    .reserved           = 0,
};

static const MppEncHwCap vepu2_no_jpeg = {
    .cap_coding         = HAVE_AVC | HAVE_VP8,
    .type               = VPU_CLIENT_VEPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 0,
    .reserved           = 0,
};

static const MppEncHwCap vepu2_jpeg = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_VEPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 0,
    .reserved           = 0,
};

static const MppEncHwCap vepu22 = {
    .cap_coding         = CAP_CODING_HEVC,
    .type               = VPU_CLIENT_VEPU22,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 0,
    .reserved           = 0,
};

static const MppEncHwCap vepu540p = {
    .cap_coding         = HAVE_AVC,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 1,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

static const MppEncHwCap vepu541 = {
    .cap_coding         = CAP_CODING_VEPU54X,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 1,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_hw_osd         = 1,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

/* vepu540 support both AFBC_V1 and AFBC_V2 input */
static const MppEncHwCap vepu540 = {
    .cap_coding         = CAP_CODING_VEPU54X,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 0x1 | 0x2,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 1,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

/*
 * NOTE:
 * vpu1 = vdpu1 + vepu1
 * vpu2 = vdpu2 + vepu2
 */
static const MppSocInfo mpp_soc_infos[] = {
    {   /*
         * rk3036 has
         * 1 - vdpu1
         * 2 - RK hevc decoder
         * rk3036 do NOT have encoder
         */
        "rk3036",
        ROCKCHIP_SOC_RK3036,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_HEVC_DEC,
        {   &rk_hevc_1080p, &vdpu1, &vdpu1_jpeg_pp, NULL, },
        {   NULL, NULL, NULL, NULL, },
    },
    {   /* rk3066 has vpu1 only */
        "rk3066",
        ROCKCHIP_SOC_RK3066,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1,
        {   &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /* rk3188 has vpu1 only */
        "rk3188",
        ROCKCHIP_SOC_RK3188,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1,
        {   &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /*
         * rk3288 has
         * 1 - vpu1 with 2160p AVC decoder
         * 2 - RK hevc 4K decoder
         */
        "rk3288",
        ROCKCHIP_SOC_RK3288,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1 | HAVE_HEVC_DEC,
        {   &rk_hevc, &vdpu1_2160p, &vdpu1_jpeg_pp, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /*
         * rk3126 has
         * 1 - vpu1
         * 2 - RK hevc 1080p decoder
         */
        "rk3126",
        ROCKCHIP_SOC_RK312X,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1 | HAVE_HEVC_DEC,
        {   &rk_hevc_1080p, &vdpu1, &vdpu1_jpeg_pp, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /*
         * rk3128h has
         * 1 - vpu2
         * 2 - RK H.264/H.265 1080p@60fps decoder
         * NOTE: rk3128H do NOT have jpeg encoder
         */
        "rk3128h",
        ROCKCHIP_SOC_RK3128H,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC,
        {   &vdpu341_lite_1080p, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2_no_jpeg, NULL, NULL, NULL, },
    },
    {   /*
         * rk3128 has
         * 1 - vpu1
         * 2 - RK hevc 1080p decoder
         */
        "rk3128",
        ROCKCHIP_SOC_RK312X,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1 | HAVE_HEVC_DEC,
        {   &rk_hevc_1080p, &vdpu1, &vdpu1_jpeg_pp, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /*
         * rk3368 has
         * 1 - vpu1
         * 2 - RK hevc 4K decoder
         */
        "rk3368",
        ROCKCHIP_SOC_RK3368,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1 | HAVE_HEVC_DEC,
        {   &rk_hevc, &vdpu1, &vdpu1_jpeg_pp, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /*
         * rk3399 has
         * 1 - vpu2
         * 2 - H.264/H.265/VP9 4K decoder
         */
        "rk3399",
        ROCKCHIP_SOC_RK3399,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC,
        {   &vdpu341, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * rk3228h has
         * 1 - vpu2
         * 2 - RK H.264/H.265 4K decoder
         * 3 - avs+ decoder
         * 4 - H.265 1080p encoder
         * rk3228h first for string matching
         */
        "rk3228h",
        ROCKCHIP_SOC_RK3228H,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_AVSDEC | HAVE_VEPU22,
        {   &vdpu341_lite, &vdpu2, &vdpu2_jpeg_pp, &avsd, },
        {   &vepu2_no_jpeg, &vepu22, NULL, NULL, },
    },
    {   /*
         * rk3228 has codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 4 - H.265 encoder
         */
        "rk3328",
        ROCKCHIP_SOC_RK3328,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_VEPU22,
        {   &vdpu341, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2, &vepu22, NULL, NULL, },
    },
    {   /*
         * rk3228 have codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265 4K decoder
         * NOTE: rk3228 do NOT have jpeg encoder
         */
        "rk3228",
        ROCKCHIP_SOC_RK3228,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC,
        {   &vdpu341_lite, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2_no_jpeg, NULL, NULL, NULL, },
    },
    {   /*
         * rk3229 has
         * 1 - vpu2
         * 2 - H.264/H.265/VP9 4K decoder
         */
        "rk3229",
        ROCKCHIP_SOC_RK3229,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC,
        {   &vdpu341, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * rv1108 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264 4K decoder
         * 3 - RK H.264 4K encoder
         */
        "rv1108",
        ROCKCHIP_SOC_RV1108,
        HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC,
        {   &vdpu2_jpeg, &vdpu341_h264, NULL, NULL, },
        {   &vepu2_jpeg, &vepu540p, NULL, NULL, },
    },
    {   /*
         * rv1109 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264/H.265 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         */
        "rv1109",
        ROCKCHIP_SOC_RV1109,
        HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC,
        {   &vdpu2_jpeg, &vdpu341_lite, NULL, NULL, },
        {   &vepu2_jpeg, &vepu541, NULL, NULL, },
    },
    {   /*
         * rv1126 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264/H.265 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         */
        "rv1126",
        ROCKCHIP_SOC_RV1126,
        HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC,
        {   &vdpu2_jpeg, &vdpu341_lite, NULL, NULL, },
        {   &vepu2_jpeg, &vepu541, NULL, NULL, },
    },
    {   /*
         * rk3326 has
         * 1 - vpu2
         * 2 - RK hevc 1080p decoder
         */
        "rk3326",
        ROCKCHIP_SOC_RK3326,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_HEVC_DEC,
        {   &rk_hevc_1080p, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * px30 has
         * 1 - vpu2
         * 2 - RK hevc 1080p decoder
         */
        "px30",
        ROCKCHIP_SOC_RK3326,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_HEVC_DEC,
        {   &rk_hevc_1080p, &vdpu2, &vdpu2_jpeg_pp, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * px30 has vpu2 only
         */
        "rk1808",
        ROCKCHIP_SOC_RK1808,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2,
        {   &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * rk3566/rk3568 has codec:
         * 1 - vpu2 for jpeg/vp8 encoder and decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         * 3 - RK jpeg decoder
         */
        "rk3566",
        ROCKCHIP_SOC_RK3566,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu34x, &rkjpegd, &vdpu2, &vdpu2_jpeg_pp, },
        {   &vepu540, &vepu2, NULL, NULL, },
    },
    {   /*
         * rk3566/rk3568 has codec:
         * 1 - vpu2 for jpeg/vp8 encoder and decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         * 3 - RK jpeg decoder
         */
        "rk3568",
        ROCKCHIP_SOC_RK3568,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu34x, &rkjpegd, &vdpu2, &vdpu2_jpeg_pp, },
        {   &vepu540, &vepu2, NULL, NULL, },
    },
};

static const MppSocInfo mpp_soc_default = {
    "unknown",
    ROCKCHIP_SOC_AUTO,
    HAVE_VDPU2 | HAVE_VEPU2 | HAVE_VDPU1 | HAVE_VEPU1,
    {   &vdpu2, &vdpu1, NULL, NULL, },
    {   &vepu2, &vepu1, NULL, NULL, },
};

static void read_soc_name(char *name, RK_S32 size)
{
    const char *path = "/proc/device-tree/compatible";
    RK_S32 fd = open(path, O_RDONLY);

    if (fd < 0) {
        mpp_err("open %s error\n", path);
    } else {
        ssize_t soc_name_len = 0;

        snprintf(name, size - 1, "unknown");
        soc_name_len = read(fd, name, size - 1);
        if (soc_name_len > 0) {
            name[soc_name_len] = '\0';
            /* replacing the termination character to space */
            for (char *ptr = name;; ptr = name) {
                ptr += strnlen(name, size);
                if (ptr >= name + soc_name_len - 1)
                    break;
                *ptr = ' ';
            }

            mpp_dbg_platform("chip name: %s\n", name);
        }

        close(fd);
    }
}


static const MppSocInfo *check_soc_info(const char *soc_name)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(mpp_soc_infos); i++) {
        const char *compatible = mpp_soc_infos[i].compatible;

        if (strstr(soc_name, compatible)) {
            mpp_dbg_platform("match chip name: %s\n", compatible);
            return &mpp_soc_infos[i];
        }
    }

    return NULL;
}

class MppSocService
{
private:
    // avoid any unwanted function
    MppSocService();
    ~MppSocService() {};
    MppSocService(const MppSocService &);
    MppSocService &operator=(const MppSocService &);

    char                soc_name[MAX_SOC_NAME_LENGTH];
    const MppSocInfo    *soc_info;
    RK_U32              dec_coding_cap;
    RK_U32              enc_coding_cap;

public:
    static MppSocService *get() {
        static MppSocService instance;
        return &instance;
    }

    const char          *get_soc_name() { return soc_name; };
    const MppSocInfo    *get_soc_info() { return soc_info; };
    RK_U32              get_dec_cap() { return dec_coding_cap; };
    RK_U32              get_enc_cap() { return enc_coding_cap; };
};

MppSocService::MppSocService()
    : soc_info(NULL),
      dec_coding_cap(0),
      enc_coding_cap(0)
{
    RK_U32 i;
    RK_U32 vcodec_type = 0;

    read_soc_name(soc_name, sizeof(soc_name));
    soc_info = check_soc_info(soc_name);
    if (NULL == soc_info) {
        mpp_dbg_platform("use default chip info\n");
        soc_info = &mpp_soc_default;
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(soc_info->dec_caps); i++) {
        const MppDecHwCap *cap = soc_info->dec_caps[i];

        if (cap && cap->cap_coding) {
            dec_coding_cap |= cap->cap_coding;
            vcodec_type |= (1 << cap->type);
        }
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(soc_info->enc_caps); i++) {
        const MppEncHwCap *cap = soc_info->enc_caps[i];

        if (cap && cap->cap_coding) {
            enc_coding_cap |= cap->cap_coding;
            vcodec_type |= (1 << cap->type);
        }
    }

    mpp_dbg_platform("coding caps: dec %08x enc %08x\n",
                     dec_coding_cap, enc_coding_cap);
    mpp_dbg_platform("vcodec type: %08x\n", soc_info->vcodec_type);
    mpp_assert(soc_info->vcodec_type == vcodec_type);
}

const char *mpp_get_soc_name(void)
{
    static const char *soc_name = NULL;

    if (soc_name)
        return soc_name;

    soc_name = MppSocService::get()->get_soc_name();
    return soc_name;
}

const MppSocInfo *mpp_get_soc_info(void)
{
    static const MppSocInfo *soc_info = NULL;

    if (soc_info)
        return soc_info;

    soc_info = MppSocService::get()->get_soc_info();
    return soc_info;
}

RockchipSocType mpp_get_soc_type(void)
{
    static RockchipSocType soc_type = ROCKCHIP_SOC_AUTO;

    if (soc_type)
        return soc_type;

    soc_type = MppSocService::get()->get_soc_info()->soc_type;
    return soc_type;
}

RK_U32 mpp_get_vcodec_type(void)
{
    static RK_U32 vcodec_type = 0;

    if (!vcodec_type)
        vcodec_type = MppSocService::get()->get_soc_info()->vcodec_type;

    return vcodec_type;
}

static RK_U32 is_valid_cap_coding(RK_U32 cap, MppCodingType coding)
{
    RK_S32 index = CODING_TO_IDX(coding);
    if (index > 0 && index < 32 && (cap & (RK_U32)(1 << index)))
        return true;

    return false;
}

RK_U32 mpp_check_soc_cap(MppCtxType type, MppCodingType coding)
{
    RK_U32 cap = 0;

    if (type == MPP_CTX_DEC)
        cap = MppSocService::get()->get_dec_cap();
    else if (type == MPP_CTX_ENC)
        cap = MppSocService::get()->get_enc_cap();
    else {
        mpp_err_f("invalid ctx type %d\n", type);
        return 0;
    }

    if (!cap)
        return 0;

    return is_valid_cap_coding(cap, coding);
}
