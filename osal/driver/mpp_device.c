/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_device"

#include <string.h>

#include "mpp_env.h"
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
    case MPP_DEV_DELIMIT : {
        if (api->delimit)
            ret = api->delimit(impl_ctx);
    } break;
    case MPP_DEV_SET_CB_CTX: {
        if (api->set_cb_ctx)
            ret = api->set_cb_ctx(impl_ctx, param);
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
    case MPP_DEV_REG_OFFS : {
        if (api->reg_offs)
            ret = api->reg_offs(impl_ctx, param);
    } break;
    case MPP_DEV_RCB_INFO : {
        if (api->rcb_info)
            ret = api->rcb_info(impl_ctx, param);
    } break;
    case MPP_DEV_SET_INFO : {
        if (api->set_info)
            ret = api->set_info(impl_ctx, param);
    } break;
    case MPP_DEV_SET_ERR_REF_HACK : {
        if (api->set_err_ref_hack)
            ret = api->set_err_ref_hack(impl_ctx, param);
    } break;
    case MPP_DEV_LOCK_MAP : {
        if (api->lock_map)
            ret = api->lock_map(impl_ctx);
    } break;
    case MPP_DEV_UNLOCK_MAP : {
        if (api->unlock_map)
            ret = api->unlock_map(impl_ctx);
    } break;
    case MPP_DEV_ATTACH_FD : {
        if (api->attach_fd)
            ret = api->attach_fd(impl_ctx, param);
    } break;
    case MPP_DEV_DETACH_FD : {
        if (api->detach_fd)
            ret = api->detach_fd(impl_ctx, param);
    } break;
    case MPP_DEV_CMD_SEND : {
        if (api->cmd_send)
            ret = api->cmd_send(impl_ctx);
    } break;
    case MPP_DEV_CMD_POLL : {
        if (api->cmd_poll)
            ret = api->cmd_poll(impl_ctx, param);
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

    return mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
}

/* register offset multi config */
MPP_RET mpp_dev_multi_offset_init(MppDevRegOffCfgs **cfgs, RK_S32 size)
{
    RK_S32 cfg_size = sizeof(MppDevRegOffCfgs) + size * sizeof(MppDevRegOffsetCfg);
    MppDevRegOffCfgs *p = NULL;

    if (NULL == cfgs || size <= 0) {
        mpp_err_f("invalid pointer %p size %d\n", cfgs, size);
        return MPP_NOK;
    }

    p = mpp_calloc_size(MppDevRegOffCfgs, cfg_size);
    if (p)
        p->size = size;

    *cfgs = p;

    return (p) ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_dev_multi_offset_deinit(MppDevRegOffCfgs *cfgs)
{
    MPP_FREE(cfgs);

    return MPP_OK;
}

MPP_RET mpp_dev_multi_offset_reset(MppDevRegOffCfgs *cfgs)
{
    if (cfgs) {
        memset(cfgs->cfgs, 0, cfgs->count * sizeof(cfgs->cfgs[0]));
        cfgs->count = 0;
    }

    return MPP_OK;
}

MPP_RET mpp_dev_multi_offset_update(MppDevRegOffCfgs *cfgs, RK_S32 index, RK_U32 offset)
{
    MppDevRegOffsetCfg *cfg;
    RK_S32 count;
    RK_S32 i;

    if (NULL == cfgs)
        return MPP_NOK;

    if (cfgs->count >= cfgs->size) {
        mpp_err_f("invalid cfgs count %d : %d\n", cfgs->count, cfgs->size);
        return MPP_NOK;
    }

    count = cfgs->count;
    cfg = cfgs->cfgs;

    for (i = 0; i < count; i++, cfg++) {

        if (cfg->reg_idx == (RK_U32)index) {
            cfg->offset = offset;
            return MPP_OK;
        }
    }

    cfg->reg_idx = index;
    cfg->offset = offset;
    cfgs->count++;

    return MPP_OK;
}
