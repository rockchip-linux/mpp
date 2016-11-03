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

#include <unistd.h>

#include "mpp_platform.h"

RK_U32 mpp_get_vcodec_hw_flag(void)
{
    RK_U32 flag = 0;

#ifdef RKPLATFORM
    /* NOTE: current only support vpu2 */
    if (!access("/dev/vpu_service", F_OK))
        flag |= HAVE_VPU2;

    /* for rk3288 / rk3368 /rk312x hevc decoder */
    if (!access("/dev/hevc_service", F_OK))
        flag |= HAVE_HEVC_DEC;

    /* for rk3228 / rk3229 / rk3399 decoder */
    if (!access("/dev/rkvdec", F_OK))
        flag |= HAVE_RKVDEC;

    /* for rk1108 encoder */
    if (!access("/dev/rkvenc", F_OK))
        flag |= HAVE_RKVENC;
#endif

    return flag;
}

RK_U32 mpp_get_2d_hw_flag(void)
{
    RK_U32 flag = 0;

#ifdef RKPLATFORM

    if (!access("/dev/rga", F_OK))
        flag |= HAVE_RGA;

    if (!access("/dev/iep", F_OK))
        flag |= HAVE_IEP;
#endif

    return flag;
}


