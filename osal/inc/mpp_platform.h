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

#include "rk_mpi.h"

/*
 * Platform flag detection is for rockchip hardware platform detection
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform video codec hardware feature
 */
/* RK combined codec */
#define HAVE_VPU1               (0x00000001)
#define HAVE_VPU2               (0x00000002)
/* RK standalone decoder */
#define HAVE_HEVC_DEC           (0x00000100)
#define HAVE_RKVDEC             (0x00000200)
#define HAVE_AVSDEC             (0x00001000)
/* RK standalone encoder */
#define HAVE_RKVENC             (0x00010000)
#define HAVE_VEPU               (0x00020000)
/* External encoder */
#define HAVE_H265ENC            (0x01000000)

/*
 * Platform image process hardware feature
 */
#define HAVE_IPP                (0x00000001)
#define HAVE_RGA                (0x00000002)
#define HAVE_RGA2               (0x00000004)
#define HAVE_IEP                (0x00000008)

const char *mpp_get_soc_name(void);
RK_U32 mpp_get_vcodec_type(void);
RK_U32 mpp_get_2d_hw_flag(void);
const char *mpp_get_platform_dev_name(MppCtxType type, MppCodingType coding, RK_U32 platform);
const char *mpp_get_vcodec_dev_name(MppCtxType type, MppCodingType coding);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PLATFORM__*/

