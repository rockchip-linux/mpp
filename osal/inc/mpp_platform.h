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
    KERNEL_VERSION_BUTT,
} MppKernelVersion;

#ifdef __cplusplus
extern "C" {
#endif

MppIoctlVersion mpp_get_ioctl_version(void);
MppKernelVersion mpp_get_kernel_version(void);
RK_U32 mpp_get_2d_hw_flag(void);
RK_U32 mpp_get_client_hw_id(RK_S32 client_type);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PLATFORM__*/
