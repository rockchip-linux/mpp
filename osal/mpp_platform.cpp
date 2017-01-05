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

#ifdef RKPLATFORM
#include <fcntl.h>
#endif
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_platform.h"

typedef struct {
    const char *compatible;
    RK_U32 flag;
} MppVpuType;

static const MppVpuType mpp_vpu_version[] = {
    { "rk3066",  HAVE_VPU1,  },
    { "rk312x",  HAVE_VPU1,  },
    { "rk3288",  HAVE_VPU1,  },
    { "rk3368",  HAVE_VPU1,  },
    { "rk3399",  HAVE_VPU2,  },
    { "rk322x",  HAVE_VPU2,  },
    { "rk322xh", HAVE_VPU2,  },
    { "rk1108",  HAVE_VPU2,  },
    { "rv1108",  HAVE_VPU2,  },
};

RK_U32 mpp_get_vcodec_hw_flag(void)
{
    static RK_U32 flag = 0;

    if (flag)
        return flag;

#ifdef RKPLATFORM
    {
        /* judge vdpu support version */
        RK_S32 fd = -1;
        /* set vpu1 defalut for old chip without dts */
        flag = HAVE_VPU1;
        fd = open("/proc/device-tree/compatible", O_RDONLY);
        if (fd < 0) {
            mpp_err("open /proc/device-tree/compatible error.\n");
        } else {
            RK_U32 i = 0;
            char temp[256];
            if (read(fd, temp, sizeof(temp) - 1) > 0) {
                for (i = 0; i < MPP_ARRAY_ELEMS(mpp_vpu_version); i++) {
                    if (strstr(temp, mpp_vpu_version[i].compatible)) {
                        flag = mpp_vpu_version[i].flag;
                        break;
                    }
                }
            }
            close(fd);
        }
        /* for rk3288 / rk3368 /rk312x hevc decoder */
        if (!access("/dev/hevc_service", F_OK)) {
            flag |= HAVE_HEVC_DEC;
        }
        /* for rk3228 / rk3229 / rk3399 decoder */
        if (!access("/dev/rkvdec", F_OK)) {
            flag |= HAVE_RKVDEC;
        }
        /* for rk1108 encoder */
        if (!access("/dev/rkvenc", F_OK)) {
            flag |= HAVE_RKVENC;
        }
        /* for avs decoder */
        if (!access("/dev/avsd", F_OK)) {
            flag |= HAVE_AVSDEC;
        }
    }
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


