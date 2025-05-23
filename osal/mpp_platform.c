/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_platform"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_platform.h"
#include "mpp_service.h"
#include "mpp_singleton.h"

#define get_srv_platform() \
    ({ \
        MppPlatformService *__tmp; \
        if (!srv_platform) { \
            mpp_plat_srv_init(); \
        } \
        if (srv_platform) { \
            __tmp = srv_platform; \
        } else { \
            mpp_err("mpp platform srv not init at %s\n", __FUNCTION__); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

typedef struct MppPlatformService_t {
    MppIoctlVersion     ioctl_version;
    MppKernelVersion    kernel_version;
    rk_u32              vcodec_type;
    rk_u32              hw_ids[32];
    MppServiceCmdCap    mpp_service_cmd_cap;
    const MppSocInfo    *soc_info;
    const char          *soc_name;
} MppPlatformService;

static MppPlatformService *srv_platform = NULL;

static MppKernelVersion check_kernel_version(void)
{
    static const char *kernel_version_path = "/proc/version";
    MppKernelVersion version = KERNEL_UNKNOWN;
    FILE *fp = NULL;
    char buf[32];

    if (access(kernel_version_path, F_OK | R_OK))
        return version;

    fp = fopen(kernel_version_path, "rb");
    if (fp) {
        size_t len = fread(buf, 1, sizeof(buf) - 1, fp);
        char *pos = NULL;

        buf[len] = '\0';
        pos = strstr(buf, "Linux version ");
        if (pos) {
            rk_s32 major = 0;
            rk_s32 minor = 0;
            rk_s32 last = 0;
            rk_s32 count = 0;

            pos += 14;
            count = sscanf(pos, "%d.%d.%d ", &major, &minor, &last);
            if (count >= 2 && major > 0 && minor > 0) {
                switch (major) {
                case 3: {
                    version = KERNEL_3_10;
                } break;
                case 4: {
                    version = KERNEL_4_4;
                    if (minor >= 19)
                        version = KERNEL_4_19;
                } break;
                case 5: {
                    version = KERNEL_5_10;
                } break;
                case 6: {
                    version = KERNEL_6_1;
                } break;
                default: break;
                }
            }
        }
        fclose(fp);
    }
    return version;
}

static void mpp_plat_srv_init()
{
    /* judge vdpu support version */
    MppPlatformService *srv = srv_platform;
    MppServiceCmdCap *cap;

    if (srv)
        return;

    srv = mpp_calloc(MppPlatformService, 1);
    if (!srv) {
        mpp_err_f("failed to allocate platform service\n");
        return;
    }

    srv_platform = srv;

    /* default value */
    cap = &srv->mpp_service_cmd_cap;
    cap->support_cmd = 0;
    cap->query_cmd = MPP_CMD_QUERY_BASE + 1;
    cap->init_cmd = MPP_CMD_INIT_BASE + 1;
    cap->send_cmd = MPP_CMD_SEND_BASE + 1;
    cap->poll_cmd = MPP_CMD_POLL_BASE + 1;
    cap->ctrl_cmd = MPP_CMD_CONTROL_BASE + 0;

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    /* read soc name */
    srv->soc_name = mpp_get_soc_name();
    srv->soc_info = mpp_get_soc_info();

    if (srv->soc_info->soc_type == ROCKCHIP_SOC_AUTO)
        mpp_log("can not found match soc name: %s\n", srv->soc_name);

    srv->ioctl_version = IOCTL_VCODEC_SERVICE;
    if (mpp_get_mpp_service_name()) {
        srv->ioctl_version = IOCTL_MPP_SERVICE_V1;
        check_mpp_service_cap(&srv->vcodec_type, srv->hw_ids, cap);
        mpp_dbg_platform("vcodec_type from kernel 0x%08x, vs from soc info 0x%08x\n",
                         srv->vcodec_type, srv->soc_info->vcodec_type);
    }
    srv->kernel_version = check_kernel_version();
    if (!srv->vcodec_type) {
        srv->vcodec_type = srv->soc_info->vcodec_type;
    } else {
        // Compare kernel result with soc infomation.
        rk_u32 diff_type = srv->vcodec_type ^ srv->soc_info->vcodec_type;
        rk_u32 i;

        for (i = 0; i <= VPU_CLIENT_VEPU22; i++) {
            rk_u32 mask = 1 << i;

            if (diff_type & mask) {
                MppClientType client_type = (MppClientType) i;

                mpp_dbg_platform("confliction found at client_type %d\n", client_type);

                if (srv->soc_info->vcodec_type & mask) {
                    mpp_err("client %d driver is not ready!\n", client_type);
                } else {
                    mpp_dbg_platform("client %d driver is ready but not declared!\n", client_type);
                    if (client_type == VPU_CLIENT_VDPU2_PP)
                        srv->vcodec_type &= ~mask;
                }
            }
        }

        mpp_dbg_platform("vcode_type 0x%08x\n", srv->vcodec_type);
    }

    return;
}

static void mpp_plat_srv_deinit()
{
    MPP_FREE(srv_platform);
}

MppIoctlVersion mpp_get_ioctl_version(void)
{
    MppPlatformService *srv = get_srv_platform();
    MppIoctlVersion ver = IOCTL_MPP_SERVICE_V1;

    if (srv)
        ver = srv->ioctl_version;

    return ver;
}

MppKernelVersion mpp_get_kernel_version(void)
{
    MppPlatformService *srv = get_srv_platform();
    MppKernelVersion ver = KERNEL_UNKNOWN;

    if (srv)
        ver = srv->kernel_version;

    return ver;
}

rk_u32 mpp_get_2d_hw_flag(void)
{
    rk_u32 flag = 0;

    if (!access("/dev/rga", F_OK))
        flag |= HAVE_RGA;

    if (!access("/dev/iep", F_OK))
        flag |= HAVE_IEP;

    return flag;
}

const MppServiceCmdCap *mpp_get_mpp_service_cmd_cap(void)
{
    MppPlatformService *srv = get_srv_platform();
    const MppServiceCmdCap *cap = NULL;

    if (srv)
        cap = &srv->mpp_service_cmd_cap;

    return cap;
}

rk_u32 mpp_get_client_hw_id(rk_s32 client_type)
{
    MppPlatformService *srv = get_srv_platform();
    rk_u32 hw_id = 0;

    if (srv && srv->vcodec_type & (1 << client_type))
        hw_id = srv->hw_ids[client_type];

    return hw_id;
}

rk_u32 mpp_get_vcodec_type(void)
{
    MppPlatformService *srv = get_srv_platform();
    static rk_u32 vcodec_type = 0;

    if (vcodec_type)
        return vcodec_type;

    if (srv)
        vcodec_type = srv->vcodec_type;

    return vcodec_type;
}

MPP_SINGLETON(MPP_SGLN_PLATFORM, mpp_platform, mpp_plat_srv_init, mpp_plat_srv_deinit);
