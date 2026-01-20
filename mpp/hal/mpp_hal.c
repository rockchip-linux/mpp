/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_hal"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "osal_2str.h"

#include "mpp.h"
#include "mpp_soc.h"
#include "mpp_hal.h"
#include "mpp_frame_impl.h"

#include "avsd_syntax.h"

#define DEC_HAL_DBG_FLOW                (0x00000001)
#define DEC_HAL_DBG_API                 (0x00000002)

#define dec_hal_dbg(flag, fmt, ...)     mpp_dbg(mpp_hal_debug, flag, fmt, ## __VA_ARGS__)
#define dec_hal_dbg_f(flag, fmt, ...)   mpp_dbg_f(mpp_hal_debug, flag, fmt, ## __VA_ARGS__)

#define dec_hal_dbg_flow(fmt, ...)      dec_hal_dbg_f(DEC_HAL_DBG_FLOW, fmt, ## __VA_ARGS__)
#define dec_hal_dbg_api(fmt, ...)       dec_hal_dbg(DEC_HAL_DBG_API, fmt, ## __VA_ARGS__)

#define TRY_GET_DEC_HAL_API(cl_type) \
    do { \
        if (vcodec_type & (1 << cl_type)) { \
            api = mpp_dec_hal_api_get(coding, (cl_type)); \
            if (api) { \
                p->client_type = cl_type; \
                ret = hal_impl_init(p, api, cfg); \
                goto __DONE; \
            } \
        } \
    } while(0)

#define CHECK_HAL_API_FUNC(p, func) \
    if (NULL == p->api || NULL == p->api->func) \
        return MPP_OK;

typedef struct MppHalImpl_t {
    MppCodingType   coding;
    void            *ctx;
    const MppHalApi *api;
    MppHalCfg       cfg;
    RK_U32          client_type;
    MppBufferGroup  buf_group;
    MppDev          dev;

    HalTaskGroup    tasks;
} MppHalImpl;

/* max 32 coding type and max 2 apis on each soc for each coding type hardware decoder */
static const MppHalApi *vdec_apis[32][2] = { 0 };

static RK_U32 mpp_hal_debug = 0;
RK_U32 hal_h265d_debug = 0;
RK_U32 hal_h264d_debug = 0;
RK_U32 hal_vp8d_debug = 0;
RK_U32 hal_jpegd_debug = 0;
RK_U32 hal_avsd_debug = 0;
RK_U32 hal_vp9d_debug = 0;
RK_U32 hal_mpg4d_debug = 0;
RK_U32 hal_avs2d_debug = 0;
RK_U32 hal_m2vd_debug = 0;
RK_U32 hal_h263d_debug = 0;
RK_U32 hal_av1d_debug = 0;

MPP_RET mpp_hal_api_register(const MppHalApi *api)
{
    RockchipSocType soc_type = mpp_get_soc_type();
    const char *soc_name = mpp_get_soc_name();
    RK_S32 soc_match = 0;
    RK_S32 index;
    RK_S32 i;

    mpp_env_get_u32("mpp_hal_debug", &mpp_hal_debug, 0);

    dec_hal_dbg_api("api %s adding coding %d type %d client %d\n",
                    api->name, api->coding, api->type, api->client);

    for (i = 0; api->soc_type[i] != ROCKCHIP_SOC_BUTT; i++)
        if (api->soc_type[i] == soc_type) {
            dec_hal_dbg_api("api %s match soc %d %s success\n",
                            api->name, soc_type, soc_name);
            soc_match = 1;
            break;
        }

    if (!soc_match) {
        dec_hal_dbg_api("api %s match soc %d %s failed\n",
                        api->name, soc_type, soc_name);
        return MPP_NOK;
    }

    index = mpp_coding_to_index(api->coding);
    if (index < 0 || index >= MPP_ARRAY_ELEMS_S(vdec_apis))
        return MPP_NOK;

    if (NULL == vdec_apis[index][0])
        vdec_apis[index][0] = api;
    else if (NULL == vdec_apis[index][1])
        vdec_apis[index][1] = api;
    else
        return MPP_NOK;

    return MPP_OK;
}

const MppHalApi *mpp_dec_hal_api_get(MppCodingType coding, MppClientType type)
{
    RK_S32 index = mpp_coding_to_index(coding);
    const MppHalApi *api;

    if (index < 0 || index >= MPP_ARRAY_ELEMS_S(vdec_apis))
        return NULL;

    api = vdec_apis[index][0];
    if (!api)
        return NULL;

    if (api->client == type) {
        dec_hal_dbg_api("found api %s for coding %d client %d\n",
                        api->name, coding, type);
        return api;
    }

    api = vdec_apis[index][1];
    if (api && api->client == type) {
        dec_hal_dbg_api("found api %s for coding %d client %d\n",
                        api->name, coding, type);
        return api;
    }

    return NULL;
}

static MPP_RET hal_impl_init(MppHalImpl *p, const MppHalApi *api, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;

    p->coding = api->coding;
    p->api = api;
    p->ctx = mpp_calloc_size(void, p->api->ctx_size);
    if (NULL == p->ctx) {
        mpp_err_f("malloc failed for hal %s size %d\n",
                  api->name, p->api->ctx_size);

        return MPP_ERR_MALLOC;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_group, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err_f("mpp_buffer_group_get_internal failed. ret: %d\n", ret);
        goto __FAILED;
    }

    ret = mpp_dev_init(&p->dev, p->client_type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        mpp_buffer_group_put(p->buf_group);
        goto __FAILED;
    }

    p->cfg = *cfg;
    p->cfg.hw_info = mpp_get_dec_hw_info_by_client_type(p->client_type);
    p->cfg.buf_group = p->buf_group;
    p->cfg.dev = p->dev;

    ret = p->api->init(p->ctx, &p->cfg);
    if (ret)
        mpp_err_f("hal %s init failed ret %d\n", api->name, ret);

    *cfg = p->cfg;

    return ret;

__FAILED:
    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }
    if (p->buf_group) {
        mpp_buffer_group_put(p->buf_group);
        p->buf_group = NULL;
    }
    if (p->ctx) {
        mpp_free(p->ctx);
        p->ctx = NULL;
    }
    return ret;
}

MPP_RET mpp_hal_init(MppHal *ctx, MppHalCfg *cfg)
{
    MppHalImpl *p;
    const MppHalApi *api = NULL;
    MppCodingType coding;
    MPP_RET ret = MPP_NOK;
    RK_U32 vcodec_type;
    MppDecBaseCfg *base;

    if (NULL == ctx || NULL == cfg) {
        mpp_err_f("found NULL input ctx %p cfg %p\n", ctx, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    p = mpp_calloc(MppHalImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    base = &cfg->cfg->base;
    vcodec_type = mpp_get_vcodec_type();
    coding = cfg->coding;

    if (mpp_check_soc_cap(base->type, coding)) {
        RK_S32 hw_type = base->hw_type;

        if (hw_type > 0) {
            dec_hal_dbg_api("init with coding %d %s hw\n", coding, strof_client_type(hw_type));
            TRY_GET_DEC_HAL_API(hw_type);
            if (!api)
                mpp_err_f("coding %d invalid hw_type %d with vcodec_type %08x\n",
                          coding, hw_type, vcodec_type);
        }
    }

    switch (coding) {
    case MPP_VIDEO_CodingAVC: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_RKVDEC);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2_PP);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1_PP);
    } break;
    case MPP_VIDEO_CodingHEVC: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_RKVDEC);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_HEVC_DEC);
    } break;
    case MPP_VIDEO_CodingVP8: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2_PP);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1_PP);
    } break;
    case MPP_VIDEO_CodingVP9: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_RKVDEC);
    } break;
    case MPP_VIDEO_CodingMJPEG: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_JPEG_DEC);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2_PP);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1_PP);
    } break;
    case MPP_VIDEO_CodingAVSPLUS: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_AVSPLUS_DEC);
    } break;
    case MPP_VIDEO_CodingAVS:
    case MPP_VIDEO_CodingMPEG4:
    case MPP_VIDEO_CodingMPEG2:
    case MPP_VIDEO_CodingH263: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU2_PP);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_VDPU1_PP);
    } break;
    case MPP_VIDEO_CodingAV1: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_RKVDEC);
        TRY_GET_DEC_HAL_API(VPU_CLIENT_AV1DEC);
    } break;
    case MPP_VIDEO_CodingAVS2: {
        TRY_GET_DEC_HAL_API(VPU_CLIENT_RKVDEC);
    } break;
    default: {
        mpp_err_f("unsupported coding type %d soc %s\n", coding, mpp_get_soc_name());
        ret = MPP_NOK;
        goto __DONE;
    } break;
    }

__DONE:
    if (ret) {
        mpp_err_f("soc %s hal %s init failed ret %d\n",
                  mpp_get_soc_name(), p->api->name, ret);
        mpp_free(p->ctx);
        mpp_free(p);
        return ret;
    }

    *ctx = p;

    return MPP_OK;
}

MPP_RET mpp_hal_deinit(MppHal ctx)
{
    MppHalImpl *p;

    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (MppHalImpl*)ctx;

    p->api->deinit(p->ctx);

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }
    if (p->buf_group) {
        mpp_buffer_group_put(p->buf_group);
        p->buf_group = NULL;
    }

    mpp_free(p->ctx);
    mpp_free(p);

    return MPP_OK;
}

MPP_RET mpp_hal_reg_gen(MppHal ctx, HalTaskInfo *task)
{
    MppHalImpl *p = (MppHalImpl*)ctx;

    if (NULL == ctx || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", ctx, task);
        return MPP_ERR_NULL_PTR;
    }

#ifdef HAVE_AVSD
    if (p->coding == MPP_VIDEO_CodingAVS) {
        AvsdSyntax_t *syn = (AvsdSyntax_t *)task->dec.syntax.data;

        if (syn->pp.profileId == 0x48) {
            const MppHalApi *api;
            MppCodingType coding = MPP_VIDEO_CodingAVSPLUS;
            MPP_RET ret;

            ret = p->api->deinit(p->ctx);
            if (ret) {
                mpp_err_f("deinit decoder failed, ret %d\n", ret);
                return ret;
            }
            if (p->dev) {
                mpp_dev_deinit(p->dev);
                p->dev = NULL;
            }
            if (p->buf_group) {
                mpp_buffer_group_put(p->buf_group);
                p->buf_group = NULL;
            }
            MPP_FREE(p->ctx);

            api = mpp_dec_hal_api_get(coding, VPU_CLIENT_AVSPLUS_DEC);
            if (!api) {
                mpp_err_f("could not find avsplus hal api\n");
                return MPP_NOK;
            }
            ret = hal_impl_init(p, api, &p->cfg);
            if (ret) {
                mpp_err_f("hal_impl_init failed, ret %d\n", ret);
                return ret;
            }
        }
    }
#endif

    CHECK_HAL_API_FUNC(p, reg_gen);

    return p->api->reg_gen(p->ctx, task);
}

MPP_RET mpp_hal_hw_start(MppHal ctx, HalTaskInfo *task)
{
    if (NULL == ctx || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", ctx, task);
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    CHECK_HAL_API_FUNC(p, start);

    return p->api->start(p->ctx, task);
}

MPP_RET mpp_hal_hw_wait(MppHal ctx, HalTaskInfo *task)
{
    if (NULL == ctx || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", ctx, task);
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    CHECK_HAL_API_FUNC(p, wait);

    return p->api->wait(p->ctx, task);
}

MPP_RET mpp_hal_reset(MppHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    CHECK_HAL_API_FUNC(p, reset);

    return p->api->reset(p->ctx);
}

MPP_RET mpp_hal_flush(MppHal ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    CHECK_HAL_API_FUNC(p, flush);

    return p->api->flush(p->ctx);
}

MPP_RET mpp_hal_control(MppHal ctx, MpiCmd cmd, void *param)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppHalImpl *p = (MppHalImpl*)ctx;

    CHECK_HAL_API_FUNC(p, control);

    return p->api->control(p->ctx, cmd, param);
}
