/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_device"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_platform.h"
#include "mpp_device_debug.h"
#include "mpp_service_api.h"
#include "vcodec_service_api.h"

typedef struct MppDevImpl_t {
    MppClientType   type;

    void            *ctx;
    const MppDevApi *api;
} MppDevImpl;

RK_U32 mpp_device_debug = 0;

MPP_RET mpp_dev_init(MppDev *ctx, MppClientType type)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("mpp_device_debug", &mpp_device_debug, 0);

    *ctx = NULL;

    RK_U32 codec_type = mpp_get_vcodec_type();
    if (!(codec_type & (1 << type))) {
        mpp_err_f("found unsupported client type %d in platform %x\n",
                  type, codec_type);
        return MPP_ERR_VALUE;
    }

    MppIoctlVersion ioctl_version = mpp_get_ioctl_version();
    const MppDevApi *api = NULL;

    switch (ioctl_version) {
    case IOCTL_VCODEC_SERVICE : {
        api = &vcodec_service_api;
    } break;
    case IOCTL_MPP_SERVICE_V1 : {
        api = &mpp_service_api;
    } break;
    default : {
        mpp_err_f("invalid ioctl verstion %d\n", ioctl_version);
        return MPP_NOK;
    } break;
    }

    MppDevImpl *impl = mpp_calloc(MppDevImpl, 1);
    void *impl_ctx = mpp_calloc_size(void, api->ctx_size);
    if (NULL == impl || NULL == impl_ctx) {
        mpp_err_f("malloc failed impl %p impl_ctx %p\n", impl, impl_ctx);
        MPP_FREE(impl);
        MPP_FREE(impl_ctx);
        return MPP_ERR_MALLOC;
    }

    impl->ctx = impl_ctx;
    impl->api = api;
    impl->type = type;
    *ctx = impl;

    return api->init(impl_ctx, type);
}

MPP_RET mpp_dev_deinit(MppDev ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    MppDevImpl *p = (MppDevImpl *)ctx;
    MPP_RET ret = MPP_OK;

    if (p->api && p->api->deinit && p->ctx)
        ret = p->api->deinit(p->ctx);

    MPP_FREE(p->ctx);
    MPP_FREE(p);

    return ret;
}

MPP_RET mpp_dev_ioctl(MppDev ctx, RK_S32 cmd, void *param)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    MppDevImpl *p = (MppDevImpl *)ctx;
    const MppDevApi *api = p->api;
    void *impl_ctx = p->ctx;
    MPP_RET ret = MPP_OK;

    if (NULL == impl_ctx || NULL == api)
        return ret;

    switch (cmd) {
    case MPP_DEV_BATCH_ON : {
        if (api->attach)
            ret = api->attach(impl_ctx);
    } break;
    case MPP_DEV_BATCH_OFF : {
        if (api->detach)
            ret = api->detach(impl_ctx);
    } break;
    case MPP_DEV_REG_WR : {
        if (api->reg_wr)
            ret = api->reg_wr(impl_ctx, param);
    } break;
    case MPP_DEV_REG_RD : {
        if (api->reg_rd)
            ret = api->reg_rd(impl_ctx, param);
    } break;
    case MPP_DEV_REG_OFFSET : {
        if (api->reg_offset)
            ret = api->reg_offset(impl_ctx, param);
    } break;
    case MPP_DEV_RCB_INFO : {
        if (api->rcb_info)
            ret = api->rcb_info(impl_ctx, param);
    } break;
    case MPP_DEV_SET_INFO : {
        if (api->set_info)
            ret = api->set_info(impl_ctx, param);
    } break;
    case MPP_DEV_CMD_SEND : {
        if (api->cmd_send)
            ret = api->cmd_send(impl_ctx);
    } break;
    case MPP_DEV_CMD_POLL : {
        if (api->cmd_poll)
            ret = api->cmd_poll(impl_ctx);
    } break;
    default : {
        mpp_err_f("invalid cmd %d\n", cmd);
    } break;
    }

    return ret;
}

MPP_RET mpp_dev_set_reg_offset(MppDev dev, RK_S32 index, RK_U32 offset)
{
    MppDevRegOffsetCfg trans_cfg;

    trans_cfg.reg_idx = index;
    trans_cfg.offset = offset;

    mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);

    return MPP_OK;
}
