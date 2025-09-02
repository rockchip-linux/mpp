/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_DEV_DEFS_H__
#define __MPP_DEV_DEFS_H__

#include "rk_type.h"

/*
 * Platform video codec hardware feature
 */
typedef enum MppClientType_e {
    VPU_CLIENT_VDPU1        = 0,    /* 0x00000001 */
    VPU_CLIENT_VDPU2        = 1,    /* 0x00000002 */
    VPU_CLIENT_VDPU1_PP     = 2,    /* 0x00000004 */
    VPU_CLIENT_VDPU2_PP     = 3,    /* 0x00000008 */
    VPU_CLIENT_AV1DEC       = 4,    /* 0x00000010 */

    VPU_CLIENT_HEVC_DEC     = 8,    /* 0x00000100 */
    VPU_CLIENT_RKVDEC       = 9,    /* 0x00000200 */
    VPU_CLIENT_AVSPLUS_DEC  = 12,   /* 0x00001000 */
    VPU_CLIENT_JPEG_DEC     = 13,   /* 0x00002000 */

    VPU_CLIENT_RKVENC       = 16,   /* 0x00010000 */
    VPU_CLIENT_VEPU1        = 17,   /* 0x00020000 */
    VPU_CLIENT_VEPU2        = 18,   /* 0x00040000 */
    VPU_CLIENT_VEPU2_JPEG   = 19,   /* 0x00080000 */
    VPU_CLIENT_JPEG_ENC     = 20,   /* 0x00100000 */

    VPU_CLIENT_VEPU22       = 24,   /* 0x01000000 */

    IEP_CLIENT_TYPE         = 28,   /* 0x10000000 */
    VDPP_CLIENT_TYPE        = 29,   /* 0x20000000 */

    VPU_CLIENT_BUTT,
} MppClientType;

#define CLIENT_TYPE_MASK_DEC    0x0000ffff
#define CLIENT_TYPE_MASK_ENC    0x0fff0000
#define CLIENT_TYPE_MASK_VPROC  0xf0000000

/* RK combined codec */
#define HAVE_VDPU1          (1 << VPU_CLIENT_VDPU1)         /* 0x00000001 */
#define HAVE_VDPU2          (1 << VPU_CLIENT_VDPU2)         /* 0x00000002 */
#define HAVE_VDPU1_PP       (1 << VPU_CLIENT_VDPU1_PP)      /* 0x00000004 */
#define HAVE_VDPU2_PP       (1 << VPU_CLIENT_VDPU2_PP)      /* 0x00000008 */
#define HAVE_AV1DEC         (1 << VPU_CLIENT_AV1DEC)        /* 0x00000008 */
/* RK standalone decoder */
#define HAVE_HEVC_DEC       (1 << VPU_CLIENT_HEVC_DEC)      /* 0x00000100 */
#define HAVE_RKVDEC         (1 << VPU_CLIENT_RKVDEC)        /* 0x00000200 */
#define HAVE_AVSDEC         (1 << VPU_CLIENT_AVSPLUS_DEC)   /* 0x00001000 */
#define HAVE_JPEG_DEC       (1 << VPU_CLIENT_JPEG_DEC)      /* 0x00002000 */
/* RK standalone encoder */
#define HAVE_RKVENC         (1 << VPU_CLIENT_RKVENC)        /* 0x00010000 */
#define HAVE_VEPU1          (1 << VPU_CLIENT_VEPU1)         /* 0x00020000 */
#define HAVE_VEPU2          (1 << VPU_CLIENT_VEPU2)         /* 0x00040000 */
#define HAVE_VEPU2_JPEG     (1 << VPU_CLIENT_VEPU2_JPEG)    /* 0x00080000 */
#define HAVE_JPEG_ENC       (1 << VPU_CLIENT_JPEG_ENC)      /* 0x00100000 */
/* External encoder */
#define HAVE_VEPU22         (1 << VPU_CLIENT_VEPU22)        /* 0x01000000 */
/* RK Image Enhance Processor for deinterlacing */
#define HAVE_IEP            (1 << IEP_CLIENT_TYPE)          /* 0x10000000 */
/* RK Image Enhance Processor for dispaly */
#define HAVE_VDPP           (1 << VDPP_CLIENT_TYPE)         /* 0x20000000 */

/* Platform image process hardware feature */
#define HAVE_IPP            (0x00000001)
#define HAVE_RGA            (0x00000002)
#define HAVE_RGA2           (0x00000004)

#endif /*__MPP_DEV_DEFS_H__*/
