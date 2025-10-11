/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_soc"

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "mpp_soc.h"
#include "mpp_platform.h"

#define MAX_SOC_NAME_LENGTH     128

#define get_srv_soc() \
    ({ \
        MppSocSrv *__tmp; \
        if (!srv_soc) { \
            mpp_soc_srv_init(); \
        } \
        if (srv_soc) { \
            __tmp = srv_soc; \
        } else { \
            mpp_err("mpp soc srv not init at %s\n", __FUNCTION__); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

#define CODING_TO_IDX(type)   \
    ((rk_u32)(type) >= (rk_u32)MPP_VIDEO_CodingKhronosExtensions) ? \
    ((rk_u32)(-1)) : \
    ((rk_u32)(type) >= (rk_u32)MPP_VIDEO_CodingVC1) ? \
    ((rk_u32)(type) - (rk_u32)MPP_VIDEO_CodingVC1 + 16) : \
    ((rk_u32)(type) - (rk_u32)MPP_VIDEO_CodingUnused)

#define HAVE_MPEG2  ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingMPEG2))))
#define HAVE_H263   ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingH263))))
#define HAVE_MPEG4  ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingMPEG4))))
#define HAVE_AVC    ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVC))))
#define HAVE_MJPEG  ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingMJPEG))))
#define HAVE_VP8    ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingVP8))))
#define HAVE_VP9    ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingVP9))))
#define HAVE_HEVC   ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingHEVC))))
#define HAVE_AVSP   ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVSPLUS))))
#define HAVE_AVS    ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVS))))
#define HAVE_AVS2   ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAVS2))))
#define HAVE_AV1    ((rk_u32)(1 << (CODING_TO_IDX(MPP_VIDEO_CodingAV1))))

#define CAP_CODING_VDPU         (HAVE_MPEG2|HAVE_H263|HAVE_MPEG4|HAVE_AVC|HAVE_MJPEG|HAVE_VP8|HAVE_AVS)
#define CAP_CODING_JPEGD_PP     (HAVE_MJPEG)
#define CAP_CODING_AVSD         (HAVE_AVS)
#define CAP_CODING_AVSPD        (HAVE_AVSP)
#define CAP_CODING_AV1D         (HAVE_AV1)
#define CAP_CODING_HEVC         (HAVE_HEVC)
#define CAP_CODING_VDPU341      (HAVE_AVC|HAVE_HEVC|HAVE_VP9)
#define CAP_CODING_VDPU341_LITE (HAVE_AVC|HAVE_HEVC)
#define CAP_CODING_VDPU381      (HAVE_AVC|HAVE_HEVC|HAVE_VP9|HAVE_AVS2)
#define CAP_CODING_VDPU382      (HAVE_AVC|HAVE_HEVC|HAVE_AVS2)
#define CAP_CODING_VDPU383      (HAVE_AVC|HAVE_HEVC|HAVE_VP9|HAVE_AVS2|HAVE_AV1)
#define CAP_CODING_VDPU384A     (HAVE_AVC|HAVE_HEVC)

#define CAP_CODING_VEPU1        (HAVE_AVC|HAVE_MJPEG|HAVE_VP8)
#define CAP_CODING_VEPU_LITE    (HAVE_AVC|HAVE_MJPEG)
#define CAP_CODING_VEPU22       (HAVE_HEVC)
#define CAP_CODING_VEPU54X      (HAVE_AVC|HAVE_HEVC)
#define CAP_CODING_VEPU540C     (HAVE_AVC|HAVE_HEVC|HAVE_MJPEG)
#define CAP_CODING_VEPU511      (HAVE_AVC|HAVE_HEVC|HAVE_MJPEG)

static const MppDecHwCap vdpu1 = {
    .cap_coding         = CAP_CODING_VDPU,
    .type               = VPU_CLIENT_VDPU1,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu1_2160p = {
    .cap_coding         = CAP_CODING_VDPU,
    .type               = VPU_CLIENT_VDPU1,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu1_jpeg_pp = {
    .cap_coding         = CAP_CODING_JPEGD_PP,
    .type               = VPU_CLIENT_VDPU1_PP,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 1,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2 = {
    .cap_coding         = CAP_CODING_VDPU,
    .type               = VPU_CLIENT_VDPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2_jpeg = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_VDPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2_jpeg_pp = {
    .cap_coding         = CAP_CODING_JPEGD_PP,
    .type               = VPU_CLIENT_VDPU2_PP,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 1,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2_jpeg_fix = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_VDPU2,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 1,
    .reserved           = 0,
};

static const MppDecHwCap vdpu2_jpeg_pp_fix  = {
    .cap_coding         = CAP_CODING_JPEGD_PP,
    .type               = VPU_CLIENT_VDPU2_PP,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 1,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 1,
    .reserved           = 0,
};

static const MppDecHwCap rk_hevc = {
    .cap_coding         = CAP_CODING_HEVC,
    .type               = VPU_CLIENT_HEVC_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap rk_hevc_1080p = {
    .cap_coding         = CAP_CODING_HEVC,
    .type               = VPU_CLIENT_HEVC_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341 = {
    .cap_coding         = CAP_CODING_VDPU341,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341_lite = {
    .cap_coding         = CAP_CODING_VDPU341_LITE,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341_lite_1080p = {
    .cap_coding         = CAP_CODING_VDPU341_LITE,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu341_h264 = {
    .cap_coding         = HAVE_AVC,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

/* vdpu34x support AFBC_V2 output */
static const MppDecHwCap vdpu34x = {
    .cap_coding         = CAP_CODING_VDPU341,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu38x = {
    .cap_coding         = CAP_CODING_VDPU381,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 1,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 2,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu382a = {
    .cap_coding         = CAP_CODING_VDPU381,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 1,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu382 = {
    .cap_coding         = CAP_CODING_VDPU382,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 1,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu382_lite = {
    .cap_coding         = CAP_CODING_VDPU341,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 1,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu383 = {
    .cap_coding         = CAP_CODING_VDPU383,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 1,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 1,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap vdpu384a = {
    .cap_coding         = CAP_CODING_VDPU384A,
    .type               = VPU_CLIENT_RKVDEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_colmv_compress = 1,
    .cap_hw_h265_rps    = 1,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 1,
    .cap_down_scale     = 1,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap avspd = {
    .cap_coding         = CAP_CODING_AVSPD,
    .type               = VPU_CLIENT_AVSPLUS_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
    .reserved           = 0,
};

static const MppDecHwCap rkjpegd = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_JPEG_DEC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 0,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 1,
    .reserved           = 0,
};

static const MppDecHwCap av1d = {
    .cap_coding         = CAP_CODING_AV1D,
    .type               = VPU_CLIENT_AV1DEC,
    .cap_fbc            = 1,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_colmv_compress = 0,
    .cap_hw_h265_rps    = 0,
    .cap_hw_vp9_prob    = 0,
    .cap_jpg_pp_out     = 0,
    .cap_10bit          = 0,
    .cap_down_scale     = 0,
    .cap_lmt_linebuf    = 1,
    .cap_core_num       = 1,
    .cap_hw_jpg_fix     = 0,
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

static const MppEncHwCap vepu2_no_vp8 = {
    .cap_coding         = HAVE_AVC | HAVE_MJPEG,
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

static const MppEncHwCap vepu2_jpeg_enhanced = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_VEPU2_JPEG,
    .cap_fbc            = 0,
    .cap_4k             = 1,
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

/* vepu58x */
static const MppEncHwCap vepu58x = {
    .cap_coding         = CAP_CODING_VEPU54X,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 0x1 | 0x2,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_hw_osd         = 1,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

static const MppEncHwCap vepu540c = {
    .cap_coding         = CAP_CODING_VEPU540C,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 0x1 | 0x2,
    .cap_4k             = 0,
    .cap_8k             = 0,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

static const MppEncHwCap vepu540c_no_hevc = {
    .cap_coding         = (HAVE_AVC | HAVE_MJPEG),
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

static const MppEncHwCap vepu510 = {
    .cap_coding         = CAP_CODING_VEPU54X,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

static const MppEncHwCap vepu511 = {
    .cap_coding         = CAP_CODING_VEPU511,
    .type               = VPU_CLIENT_RKVENC,
    .cap_fbc            = 2,
    .cap_4k             = 1,
    .cap_8k             = 0,
    .cap_hw_osd         = 1,
    .cap_hw_roi         = 1,
    .reserved           = 0,
};

static const MppEncHwCap rkjpege_vpu720 = {
    .cap_coding         = HAVE_MJPEG,
    .type               = VPU_CLIENT_JPEG_ENC,
    .cap_fbc            = 0,
    .cap_4k             = 1,
    .cap_8k             = 1,
    .cap_hw_osd         = 0,
    .cap_hw_roi         = 0,
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
        {   &rk_hevc_1080p, &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, NULL, },
        {   NULL, NULL, NULL, NULL, },
    },
    {   /* rk3066 has vpu1 only */
        "rk3066",
        ROCKCHIP_SOC_RK3066,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1,
        {   &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, NULL, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /* rk3188 has vpu1 only */
        "rk3188",
        ROCKCHIP_SOC_RK3188,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1,
        {   &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, NULL, NULL, },
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
        {   &rk_hevc, &vdpu1_2160p, &vdpu1_jpeg_pp, NULL, NULL, NULL, },
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
        {   &rk_hevc_1080p, &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, NULL, },
        {   &vepu1, NULL, NULL, NULL, },
    },
    {   /*
         * rk3128 has
         * 1 - vpu1
         * 2 - RK hevc 1080p decoder
         */
        "rk3128",
        ROCKCHIP_SOC_RK312X,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1 | HAVE_HEVC_DEC,
        {   &rk_hevc_1080p, &vdpu1, &vdpu1_jpeg_pp, NULL, NULL, NULL, },
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
        {   &vdpu341_lite_1080p, &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, NULL, },
        {   &vepu2_no_jpeg, NULL, NULL, NULL, },
    },
    {   /*
         * rk3368 has
         * 1 - vpu1
         * 2 - RK hevc 4K decoder
         */
        "rk3368",
        ROCKCHIP_SOC_RK3368,
        HAVE_VDPU1 | HAVE_VDPU1_PP | HAVE_VEPU1 | HAVE_HEVC_DEC,
        {   &rk_hevc, &vdpu1_2160p, &vdpu1_jpeg_pp, NULL, NULL, NULL, },
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
        {   &vdpu341, &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * rk3328 has codec:
         * 1 - vpu2
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 4 - H.265 encoder
         */
        "rk3328",
        ROCKCHIP_SOC_RK3328,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_VEPU22,
        {   &vdpu341, &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, NULL, },
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
        {   &vdpu341_lite, &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, NULL, },
        {   &vepu2_no_jpeg, NULL, NULL, NULL, },
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
        {   &vdpu341_lite, &vdpu2, &vdpu2_jpeg_pp, &avspd, NULL, NULL, },
        {   &vepu2_no_jpeg, &vepu22, NULL, NULL, },
    },
    {   /*
         * rk3229 has
         * 1 - vpu2
         * 2 - H.264/H.265/VP9 4K decoder
         */
        "rk3229",
        ROCKCHIP_SOC_RK3229,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC,
        {   &vdpu341, &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, NULL, },
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
        {   &vdpu2_jpeg, &vdpu341_h264, NULL, NULL, NULL, NULL, },
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
        {   &vdpu2_jpeg_fix, &vdpu341_lite, NULL, NULL, NULL, NULL, },
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
        {   &vdpu2_jpeg_fix, &vdpu341_lite, NULL, NULL, NULL, NULL, },
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
        {   &rk_hevc_1080p, &vdpu2, &vdpu2_jpeg_pp_fix, NULL, NULL, NULL, },
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
        {   &rk_hevc_1080p, &vdpu2, &vdpu2_jpeg_pp_fix, NULL, NULL, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * px30 has vpu2 only
         */
        "rk1808",
        ROCKCHIP_SOC_RK1808,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2,
        {   &vdpu2, &vdpu2_jpeg_pp, NULL, NULL, NULL, NULL, },
        {   &vepu2, NULL, NULL, NULL, },
    },
    {   /*
         * rk3566/rk3567/rk3568 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         * 3 - RK jpeg decoder
         */
        "rk3566",
        ROCKCHIP_SOC_RK3566,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu34x, &rkjpegd, &vdpu2, &vdpu2_jpeg_pp_fix, NULL, NULL, },
        {   &vepu540, &vepu2_no_vp8, NULL, NULL, },
    },
    {   /*
         * rk3566/rk3567/rk3568 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         * 3 - RK jpeg decoder
         */
        "rk3567",
        ROCKCHIP_SOC_RK3567,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu34x, &rkjpegd, &vdpu2, &vdpu2_jpeg_pp_fix, NULL, NULL, },
        {   &vepu540, &vepu2_no_vp8, NULL, NULL, },
    },
    {   /*
         * rk3566/rk3567/rk3568 has codec:
         * 1 - vpu2 for jpeg encoder and decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 4K encoder
         * 3 - RK jpeg decoder
         */
        "rk3568",
        ROCKCHIP_SOC_RK3568,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu34x, &rkjpegd, &vdpu2, &vdpu2_jpeg_pp_fix, NULL, NULL, },
        {   &vepu540, &vepu2_no_vp8, NULL, NULL, },
    },
    {   /*
         * rk3588 has codec:
         * 1 - vpu2 for jpeg/vp8 encoder and decoder
         * 2 - RK H.264/H.265/VP9 8K decoder
         * 3 - RK H.264/H.265 8K encoder
         * 4 - RK jpeg decoder
         */
        "rk3588",
        ROCKCHIP_SOC_RK3588,
        HAVE_VDPU2 | HAVE_VDPU2_PP | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC |
        HAVE_JPEG_DEC | HAVE_AV1DEC | HAVE_AVSDEC | HAVE_VEPU2_JPEG,
        {   &vdpu38x, &rkjpegd, &vdpu2, &vdpu2_jpeg_pp_fix, &av1d, &avspd},
        {   &vepu58x, &vepu2, &vepu2_jpeg_enhanced, NULL, },
    },
    {   /*
         * rk3528 has codec:
         * 1 - vpu2 for jpeg/vp8 decoder
         * 2 - RK H.264/H.265 4K decoder
         * 3 - RK H.264/H.265 1080P encoder
         * 4 - RK jpeg decoder
         */
        "rk3528",
        ROCKCHIP_SOC_RK3528,
        HAVE_RKVDEC | HAVE_RKVENC | HAVE_VDPU2 | HAVE_JPEG_DEC | HAVE_AVSDEC,
        {   &vdpu382, &rkjpegd, &vdpu2, &avspd, NULL, NULL, },
        {   &vepu540c, NULL, NULL, NULL, },
    },
    {   /*
        * rk3528a has codec:
         * 1 - vpu2 for jpeg/vp8 decoder
         * 2 - RK H.264/H.265/VP9 4K decoder
         * 3 - RK H.264/H.265 1080P encoder
         * 4 - RK jpeg decoder
         */
        "rk3528a",
        ROCKCHIP_SOC_RK3528,
        HAVE_RKVDEC | HAVE_RKVENC | HAVE_VDPU2 | HAVE_JPEG_DEC | HAVE_AVSDEC,
        {   &vdpu382a, &rkjpegd, &vdpu2, &avspd, NULL, NULL, },
        {   &vepu540c, NULL, NULL, NULL, },
    },
    {   /*
         * rk3562 has codec:
         * 1 - RK H.264/H.265/VP9 4K decoder
         * 2 - RK H.264 1080P encoder
         * 3 - RK jpeg decoder
         */
        "rk3562",
        ROCKCHIP_SOC_RK3562,
        HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu382_lite, &rkjpegd, NULL, NULL, NULL, NULL, },
        {   &vepu540c_no_hevc, NULL, NULL, NULL, },
    },
    {   /*
         * rk3576 has codec:
         * 1 - RK H.264/H.265/VP9/AVS2/AV1 8K decoder
         * 2 - RK H.264/H.265 8K encoder
         * 3 - RK jpeg decoder/encoder
         */
        "rk3576",
        ROCKCHIP_SOC_RK3576,
        HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC | HAVE_JPEG_ENC,
        {   &vdpu383, &rkjpegd, NULL, NULL, NULL, NULL},
        {   &vepu510, &rkjpege_vpu720, NULL, NULL},
    },
    {   /*
         * rv1126b has codec:
         * 1 - RK H.264/H.265 4K decoder
         * 2 - RK H.264/H.265/jpeg 4K encoder
         * 3 - RK jpeg decoder
         */
        "rv1126b",
        ROCKCHIP_SOC_RV1126B,
        HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC,
        {   &vdpu384a, &rkjpegd, NULL, NULL, NULL, NULL},
        {   &vepu511, NULL, NULL, NULL},
    },
};

static const MppSocInfo mpp_soc_default = {
    "unknown",
    ROCKCHIP_SOC_AUTO,
    HAVE_VDPU2 | HAVE_VEPU2 | HAVE_VDPU1 | HAVE_VEPU1,
    {   &vdpu2, &vdpu1, NULL, NULL, },
    {   &vepu2, &vepu1, NULL, NULL, },
};

static void read_soc_name(char *name, rk_s32 size)
{
    const char *path = "/proc/device-tree/compatible";
    rk_s32 fd = open(path, O_RDONLY);

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
    rk_s32 i;

    for (i = MPP_ARRAY_ELEMS(mpp_soc_infos) - 1; i >= 0; i--) {
        const char *compatible = mpp_soc_infos[i].compatible;

        if (strstr(soc_name, compatible)) {
            mpp_dbg_platform("match chip name: %s\n", compatible);
            return &mpp_soc_infos[i];
        }
    }

    return NULL;
}

typedef struct MppSocSrv_t {
    char                soc_name[MAX_SOC_NAME_LENGTH];
    const MppSocInfo    *soc_info;
    rk_u32              dec_coding_cap;
    rk_u32              enc_coding_cap;
} MppSocSrv;

static MppSocSrv *srv_soc = NULL;

static void mpp_soc_srv_init()
{
    MppSocSrv *srv = srv_soc;
    rk_u32 vcodec_type = 0;
    rk_u32 i;

    if (srv)
        return;

    srv = mpp_calloc(MppSocSrv, 1);
    if (!srv) {
        mpp_err_f("failed to allocate soc service\n");
        return;
    }

    srv_soc = srv;

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    read_soc_name(srv->soc_name, sizeof(srv->soc_name));
    srv->soc_info = check_soc_info(srv->soc_name);
    if (NULL == srv->soc_info) {
        mpp_dbg_platform("use default chip info\n");
        srv->soc_info = &mpp_soc_default;
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(srv->soc_info->dec_caps); i++) {
        const MppDecHwCap *cap = srv->soc_info->dec_caps[i];

        if (cap && cap->cap_coding) {
            srv->dec_coding_cap |= cap->cap_coding;
            vcodec_type |= (1 << cap->type);
        }
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(srv->soc_info->enc_caps); i++) {
        const MppEncHwCap *cap = srv->soc_info->enc_caps[i];

        if (cap && cap->cap_coding) {
            srv->enc_coding_cap |= cap->cap_coding;
            vcodec_type |= (1 << cap->type);
        }
    }

    mpp_dbg_platform("coding caps: dec %08x enc %08x\n",
                     srv->dec_coding_cap, srv->enc_coding_cap);
    mpp_dbg_platform("vcodec type from cap: %08x, from soc_info %08x\n",
                     vcodec_type, srv->soc_info->vcodec_type);
    mpp_assert(srv->soc_info->vcodec_type == vcodec_type);
}

static void mpp_soc_srv_deinit()
{
    MPP_FREE(srv_soc);
}

const char *mpp_get_soc_name(void)
{
    MppSocSrv *srv = get_srv_soc();
    const char *name = NULL;

    if (srv)
        name = srv->soc_name;

    return name;
}

const MppSocInfo *mpp_get_soc_info(void)
{
    MppSocSrv *srv = get_srv_soc();
    const MppSocInfo *info = NULL;

    if (srv)
        info = srv->soc_info;

    return info;
}

RockchipSocType mpp_get_soc_type(void)
{
    MppSocSrv *srv = get_srv_soc();
    RockchipSocType type = ROCKCHIP_SOC_AUTO;

    if (srv)
        type = srv->soc_info->soc_type;

    return type;
}

static rk_u32 is_valid_cap_coding(rk_u32 cap, MppCodingType coding)
{
    rk_s32 index = CODING_TO_IDX(coding);
    if (index > 0 && index < 32 && (cap & (rk_u32)(1 << index)))
        return 1;

    return 0;
}

rk_u32 mpp_check_soc_cap(MppCtxType type, MppCodingType coding)
{
    MppSocSrv *srv = get_srv_soc();
    rk_u32 cap = 0;

    if (srv) {
        if (type == MPP_CTX_DEC)
            cap = srv->dec_coding_cap;
        else if (type == MPP_CTX_ENC)
            cap = srv->enc_coding_cap;
        else
            return 0;
    }

    if (!cap)
        return 0;

    return is_valid_cap_coding(cap, coding);
}

const MppDecHwCap* mpp_get_dec_hw_info_by_client_type(MppClientType client_type)
{
    const MppDecHwCap* hw_info = NULL;
    const MppSocInfo *info = mpp_get_soc_info();
    rk_u32 i = 0;

    for (i = 0; i < MPP_ARRAY_ELEMS(info->dec_caps); i++) {
        if (info->dec_caps[i] && info->dec_caps[i]->type == client_type) {
            hw_info = info->dec_caps[i];
            break;
        }
    }

    return hw_info;
}

MPP_SINGLETON(MPP_SGLN_SOC, mpp_soc, mpp_soc_srv_init, mpp_soc_srv_deinit);
