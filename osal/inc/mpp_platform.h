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

#ifndef __MPP_PLATFORM__
#define __MPP_PLATFORM__

#include "rk_type.h"

/*
 * Platform flag detection is for rockchip hardware platform detection
 */
typedef enum MppIoctlVersion_e {
    IOCTL_VCODEC_SERVICE,
    IOCTL_MPP_SERVICE_V1,
    IOCTL_VERSION_BUTT,
} MppIoctlVersion;

/*
 * Platform video codec hardware feature
 */
typedef enum {
    VPU_CLIENT_VDPU1        = 0,    /* 0x00000001 */
    VPU_CLIENT_VDPU2        = 1,    /* 0x00000002 */
    VPU_CLIENT_VDPU1_PP     = 2,    /* 0x00000004 */
    VPU_CLIENT_VDPU2_PP     = 3,    /* 0x00000008 */

    VPU_CLIENT_HEVC_DEC     = 8,    /* 0x00000100 */
    VPU_CLIENT_RKVDEC       = 9,    /* 0x00000200 */
    VPU_CLIENT_AVSPLUS_DEC  = 12,   /* 0x00001000 */

    VPU_CLIENT_RKVENC       = 16,   /* 0x00010000 */
    VPU_CLIENT_VEPU1        = 17,   /* 0x00020000 */
    VPU_CLIENT_VEPU2        = 18,   /* 0x00040000 */
    VPU_CLIENT_VEPU2_LITE   = 19,   /* 0x00080000 */
    VPU_CLIENT_VEPU22       = 24,   /* 0x01000000 */
    VPU_CLIENT_BUTT,
} VPU_CLIENT2_TYPE;

/* RK combined codec */
#define HAVE_VDPU1          (1 << VPU_CLIENT_VDPU1)         /* 0x00000001 */
#define HAVE_VDPU2          (1 << VPU_CLIENT_VDPU2)         /* 0x00000002 */
#define HAVE_VDPU1_PP       (1 << VPU_CLIENT_VDPU1_PP)      /* 0x00000004 */
#define HAVE_VDPU2_PP       (1 << VPU_CLIENT_VDPU2_PP)      /* 0x00000008 */
/* RK standalone decoder */
#define HAVE_HEVC_DEC       (1 << VPU_CLIENT_HEVC_DEC)      /* 0x00000100 */
#define HAVE_RKVDEC         (1 << VPU_CLIENT_RKVDEC)        /* 0x00000200 */
#define HAVE_AVSDEC         (1 << VPU_CLIENT_AVSPLUS_DEC)   /* 0x00001000 */
/* RK standalone encoder */
#define HAVE_RKVENC         (1 << VPU_CLIENT_RKVENC)        /* 0x00010000 */
#define HAVE_VEPU1          (1 << VPU_CLIENT_VEPU1)         /* 0x00020000 */
#define HAVE_VEPU2          (1 << VPU_CLIENT_VEPU2)         /* 0x00040000 */
#define HAVE_VEPU2_LITE     (1 << VPU_CLIENT_VEPU2_LITE)    /* 0x00080000 */
/* External encoder */
#define HAVE_VEPU22         (1 << VPU_CLIENT_VEPU22)        /* 0x01000000 */

/* Platform image process hardware feature */
#define HAVE_IPP            (0x00000001)
#define HAVE_RGA            (0x00000002)
#define HAVE_RGA2           (0x00000004)
#define HAVE_IEP            (0x00000008)

/* Hal device id */
typedef enum MppDeviceId_e {
    DEV_VDPU,                   //!< vpu combined decoder
    DEV_VEPU,                   //!< vpu combined encoder
    DEV_RKVDEC,                 //!< rockchip h264 h265 vp9 combined decoder
    DEV_RKVENC,                 //!< rockchip h264 h265 combined encoder
    DEV_ID_BUTT,
} MppDeviceId;

#ifdef __cplusplus
extern "C" {
#endif

MppIoctlVersion mpp_get_ioctl_version(void);
const char *mpp_get_soc_name(void);
RK_U32 mpp_get_vcodec_type(void);
RK_U32 mpp_get_2d_hw_flag(void);
RK_U32 mpp_refresh_vcodec_type(RK_U32 vcodec_type);
const char *mpp_get_platform_dev_name(MppCtxType type, MppCodingType coding, RK_U32 platform);
const char *mpp_get_vcodec_dev_name(MppCtxType type, MppCodingType coding);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PLATFORM__*/

