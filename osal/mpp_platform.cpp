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

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_platform.h"
#include "mpp_service.h"
#include "mpp_soc.h"

#define MAX_SOC_NAME_LENGTH     128

typedef struct {
    const char *compatible;
    RockchipSocType soc_type;
    RK_U32 vcodec_type;
} MppVpuType;

static const MppVpuType mpp_vpu_version[] = {
    { "rk3036",  ROCKCHIP_SOC_RK3036,   HAVE_VDPU1 | HAVE_VEPU1 | HAVE_HEVC_DEC, },
    { "rk3066",  ROCKCHIP_SOC_RK3066,   HAVE_VDPU1 | HAVE_VEPU1,                 },
    { "rk3188",  ROCKCHIP_SOC_RK3188,   HAVE_VDPU1 | HAVE_VEPU1,                 },
    { "rk3288",  ROCKCHIP_SOC_RK3288,   HAVE_VDPU1 | HAVE_VEPU1 | HAVE_HEVC_DEC, },
    { "rk3126",  ROCKCHIP_SOC_RK312X,   HAVE_VDPU1 | HAVE_VEPU1 | HAVE_HEVC_DEC, },
    { "rk3128h", ROCKCHIP_SOC_RK3128H,  HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC,   },
    { "rk3128",  ROCKCHIP_SOC_RK312X,   HAVE_VDPU1 | HAVE_VEPU1 | HAVE_HEVC_DEC, },
    { "rk3368",  ROCKCHIP_SOC_RK3368,   HAVE_VDPU1 | HAVE_VEPU1 | HAVE_HEVC_DEC, },
    { "rk3399",  ROCKCHIP_SOC_RK3399,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC,   },
    /* 3228h first for string matching */
    { "rk3228h", ROCKCHIP_SOC_RK3228H,  HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_AVSDEC | HAVE_VEPU22, },
    { "rk3328",  ROCKCHIP_SOC_RK3328,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_VEPU22, },
    { "rk3228",  ROCKCHIP_SOC_RK3228,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC,   },
    { "rk3229",  ROCKCHIP_SOC_RK3229,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC,   },
    { "rv1108",  ROCKCHIP_SOC_RV1108,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC, },
    { "rv1109",  ROCKCHIP_SOC_RV1109,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC, },
    { "rv1126",  ROCKCHIP_SOC_RV1126,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC, },
    { "rk3326",  ROCKCHIP_SOC_RK3326,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_HEVC_DEC, },
    { "px30",    ROCKCHIP_SOC_RK3326,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_HEVC_DEC, },
    { "rk1808",  ROCKCHIP_SOC_RK1808,   HAVE_VDPU2 | HAVE_VEPU2, },
    { "rk3566",  ROCKCHIP_SOC_RK3566,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC, },
    { "rk3568",  ROCKCHIP_SOC_RK3568,   HAVE_VDPU2 | HAVE_VEPU2 | HAVE_RKVDEC | HAVE_RKVENC | HAVE_JPEG_DEC, },
};

static void read_soc_name(char *name, RK_S32 size)
{
    static const char *mpp_soc_name_path = "/proc/device-tree/compatible";
    RK_S32 fd = open(mpp_soc_name_path, O_RDONLY);
    if (fd < 0) {
        mpp_err("open %s error\n", mpp_soc_name_path);
    } else {
        ssize_t soc_name_len = 0;

        snprintf(name, size, "unknown");
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

            mpp_dbg(MPP_DBG_PLATFORM, "chip name: %s\n", name);
        }

        close(fd);
    }
}

static const MppVpuType *check_vpu_type_by_soc_name(const char *soc_name)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(mpp_vpu_version); i++)
        if (strstr(soc_name, mpp_vpu_version[i].compatible))
            return &mpp_vpu_version[i];

    return NULL;
}

class MppPlatformService
{
private:
    // avoid any unwanted function
    MppPlatformService();
    ~MppPlatformService() {};
    MppPlatformService(const MppPlatformService &);
    MppPlatformService &operator=(const MppPlatformService &);

    MppIoctlVersion     ioctl_version;
    RK_U32              vcodec_type;
    RK_U32              hw_ids[32];
    MppServiceCmdCap    mpp_service_cmd_cap;
    char                soc_name[MAX_SOC_NAME_LENGTH];
    RockchipSocType     soc_type;

public:
    static MppPlatformService *get_instance() {
        static MppPlatformService instance;
        return &instance;
    }

    MppIoctlVersion     get_ioctl_version(void) { return ioctl_version; };
    const char          *get_soc_name() { return soc_name; };
    RockchipSocType     get_soc_type() { return soc_type; };
    RK_U32              get_vcodec_type() { return vcodec_type; };
    MppServiceCmdCap    *get_mpp_service_cmd_cap() { return &mpp_service_cmd_cap; };
    RK_U32              get_hw_id(RK_S32 client_type);
};

MppPlatformService::MppPlatformService()
    : ioctl_version(IOCTL_VCODEC_SERVICE),
      vcodec_type(0)
{
    /* judge vdpu support version */
    MppServiceCmdCap *cap = &mpp_service_cmd_cap;

    /* default value */
    cap->support_cmd = 0;
    cap->query_cmd = MPP_CMD_QUERY_BASE + 1;
    cap->init_cmd = MPP_CMD_INIT_BASE + 1;
    cap->send_cmd = MPP_CMD_SEND_BASE + 1;
    cap->poll_cmd = MPP_CMD_POLL_BASE + 1;
    cap->ctrl_cmd = MPP_CMD_CONTROL_BASE + 0;

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    /* read soc name */
    read_soc_name(soc_name, sizeof(soc_name));

    /* set vpu1 defalut for old chip without dts */
    vcodec_type = HAVE_VDPU1 | HAVE_VEPU1;
    {
        const MppVpuType *hw_info = check_vpu_type_by_soc_name(soc_name);
        if (hw_info) {
            vcodec_type = hw_info->vcodec_type;
            soc_type = hw_info->soc_type;
            mpp_dbg(MPP_DBG_PLATFORM, "match soc name: %s\n", soc_name);
        } else
            mpp_log("can not found match soc name: %s\n", soc_name);
    }

    /* if /dev/mpp_service not double check */
    if (mpp_get_mpp_service_name()) {
        ioctl_version = IOCTL_MPP_SERVICE_V1;
        check_mpp_service_cap(&vcodec_type, hw_ids, cap);
    }

    mpp_dbg(MPP_DBG_PLATFORM, "vcodec type %08x\n", vcodec_type);
}

RK_U32 MppPlatformService::get_hw_id(RK_S32 client_type)
{
    RK_U32 hw_id = 0;

    if (vcodec_type & (1 << client_type))
        hw_id = hw_ids[client_type];

    return hw_id;
}

MppIoctlVersion mpp_get_ioctl_version(void)
{
    return MppPlatformService::get_instance()->get_ioctl_version();
}

const char *mpp_get_soc_name(void)
{
    static const char *soc_name = NULL;

    if (soc_name)
        return soc_name;

    soc_name = MppPlatformService::get_instance()->get_soc_name();
    return soc_name;
}

RockchipSocType mpp_get_soc_type(void)
{
    static RockchipSocType soc_type = ROCKCHIP_SOC_AUTO;

    if (soc_type)
        return soc_type;

    soc_type = MppPlatformService::get_instance()->get_soc_type();
    return soc_type;
}

RK_U32 mpp_get_vcodec_type(void)
{
    static RK_U32 vcodec_type = 0;

    if (!vcodec_type)
        vcodec_type = MppPlatformService::get_instance()->get_vcodec_type();

    return vcodec_type;
}

RK_U32 mpp_get_2d_hw_flag(void)
{
    RK_U32 flag = 0;

    if (!access("/dev/rga", F_OK))
        flag |= HAVE_RGA;

    if (!access("/dev/iep", F_OK))
        flag |= HAVE_IEP;

    return flag;
}

const MppServiceCmdCap *mpp_get_mpp_service_cmd_cap(void)
{
    return MppPlatformService::get_instance()->get_mpp_service_cmd_cap();
}

RK_U32 mpp_get_client_hw_id(RK_S32 client_type)
{
    return MppPlatformService::get_instance()->get_hw_id(client_type);
}
