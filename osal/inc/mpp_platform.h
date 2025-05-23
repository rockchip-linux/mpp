/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_PLATFORM__
#define __MPP_PLATFORM__

#include "rk_type.h"
#include "mpp_soc.h"

/*
 * Platform flag detection is for rockchip hardware platform detection
 */
typedef enum MppIoctlVersion_e {
    IOCTL_VCODEC_SERVICE,
    IOCTL_MPP_SERVICE_V1,
    IOCTL_VERSION_BUTT,
} MppIoctlVersion;

typedef enum MppKernelVersion_e {
    KERNEL_UNKNOWN,
    KERNEL_3_10,
    KERNEL_4_4,
    KERNEL_4_19,
    KERNEL_5_10,
    KERNEL_6_1,
    KERNEL_VERSION_BUTT,
} MppKernelVersion;

#ifdef __cplusplus
extern "C" {
#endif

MppIoctlVersion mpp_get_ioctl_version(void);
MppKernelVersion mpp_get_kernel_version(void);
rk_u32 mpp_get_2d_hw_flag(void);
rk_u32 mpp_get_client_hw_id(RK_S32 client_type);
rk_u32 mpp_get_vcodec_type(void);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PLATFORM__*/
