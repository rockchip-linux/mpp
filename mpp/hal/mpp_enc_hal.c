/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_enc_hal"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_soc.h"
#include "mpp_platform.h"
#include "mpp_enc_hal.h"
#include "mpp_frame_impl.h"

#define ENC_HAL_DBG_FLOW                (0x00000001)
#define ENC_HAL_DBG_API                 (0x00000002)

#define enc_hal_dbg(flag, fmt, ...)     mpp_dbg(mpp_enc_hal_debug, flag, fmt, ## __VA_ARGS__)
#define enc_hal_dbg_f(flag, fmt, ...)   mpp_dbg_f(mpp_enc_hal_debug, flag, fmt, ## __VA_ARGS__)

#define enc_hal_dbg_flow(fmt, ...)      enc_hal_dbg_f(ENC_HAL_DBG_FLOW, fmt, ## __VA_ARGS__)
#define enc_hal_dbg_api(fmt, ...)       enc_hal_dbg(ENC_HAL_DBG_API, fmt, ## __VA_ARGS__)

#define TRY_GET_ENC_HAL_API(client_type) \
        if (vcodec_type & (1 << client_type)) { \
            api = mpp_enc_hal_api_get(coding, (client_type)); \
            if (api) { \
                ret = hal_impl_init(p, api, cfg); \
                break; \
            } \
        }

typedef struct MppEncHalImpl_t {
    MppCodingType       coding;

    void                *ctx;
    const MppEncHalApi  *api;

    HalTaskGroup        tasks;
} MppEncHalImpl;

/* max 32 coding type and max 2 apis on each soc for each coding type hardware encoder */
static const MppEncHalApi *venc_apis[32][2] = { 0 };

static RK_U32 mpp_enc_hal_debug = 0;
RK_U32 hal_h265e_debug = 0;
RK_U32 hal_h264e_debug = 0;
RK_U32 hal_jpege_debug = 0;
RK_U32 hal_vp8e_debug = 0;

MPP_RET mpp_enc_hal_api_register(const MppEncHalApi *api)
{
    RockchipSocType soc_type = mpp_get_soc_type();
    const char *soc_name = mpp_get_soc_name();
    RK_S32 soc_match = 0;
    RK_S32 index;
    RK_S32 i;

    mpp_env_get_u32("mpp_enc_hal_debug", &mpp_enc_hal_debug, 0);

    enc_hal_dbg_api("api %s adding coding %d client %d\n",
                    api->name, api->coding, api->client);

    for (i = 0; api->soc_type[i] != ROCKCHIP_SOC_BUTT; i++)
        if (api->soc_type[i] == soc_type) {
            enc_hal_dbg_api("api %s match soc %d %s success\n",
                            api->name, soc_type, soc_name);
            soc_match = 1;
            break;
        }

    if (!soc_match) {
        enc_hal_dbg_api("api %s match soc %d %s failed\n",
                        api->name, soc_type, soc_name);
        return MPP_NOK;
    }

    index = mpp_coding_to_index(api->coding);
    if (index < 0 || index >= MPP_ARRAY_ELEMS_S(venc_apis))
        return MPP_NOK;

    if (NULL == venc_apis[index][0])
        venc_apis[index][0] = api;
    else if (NULL == venc_apis[index][1])
        venc_apis[index][1] = api;
    else
        return MPP_NOK;

    return MPP_OK;
}

const MppEncHalApi *mpp_enc_hal_api_get(MppCodingType coding, MppClientType type)
{
    RK_S32 index = mpp_coding_to_index(coding);
    const MppEncHalApi *api;

    if (index < 0 || index >= MPP_ARRAY_ELEMS_S(venc_apis))
        return NULL;

    api = venc_apis[index][0];
    if (!api)
        return NULL;

    if (api->client == type) {
        enc_hal_dbg_api("found api %s for coding %d client %d\n",
                        api->name, coding, type);
        return api;
    }

    api = venc_apis[index][1];
    if (api && api->client == type) {
        enc_hal_dbg_api("found api %s for coding %d client %d\n",
                        api->name, coding, type);
        return api;
    }

    return NULL;
}

static MPP_RET hal_impl_init(MppEncHalImpl *p, const MppEncHalApi *api, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;

    p->coding = api->coding;
    p->api = api;
    p->ctx = mpp_calloc_size(void, p->api->ctx_size);

    ret = p->api->init(p->ctx, cfg);
    if (!ret) {
        ret = hal_task_group_init(&p->tasks, TASK_BUTT, cfg->task_cnt,
                                  sizeof(EncAsyncTaskInfo));
        if (ret) {
            mpp_err_f("hal_task_group_init failed ret %d\n", ret);
            MPP_FREE(p->ctx);
        }
        cfg->tasks = p->tasks;
    } else {
        mpp_err_f("hal %s init failed ret %d\n", api->name, ret);
    }

    return ret;
}

MPP_RET mpp_enc_hal_init(MppEncHal *ctx, MppEncHalCfg *cfg)
{
    MppEncHalImpl *p;
    const MppEncHalApi *api;
    MppCodingType coding;
    MPP_RET ret = MPP_NOK;
    RK_U32 vcodec_type;

    if (NULL == ctx || NULL == cfg) {
        mpp_err_f("found NULL input ctx %p cfg %p\n", ctx, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    p = mpp_calloc(MppEncHalImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    enc_hal_dbg_flow("enter\n");

    vcodec_type = mpp_get_vcodec_type();
    coding = cfg->coding;

    do {
        TRY_GET_ENC_HAL_API(VPU_CLIENT_RKVENC);
        if (coding == MPP_VIDEO_CodingMJPEG)
            TRY_GET_ENC_HAL_API(VPU_CLIENT_JPEG_ENC);
        TRY_GET_ENC_HAL_API(VPU_CLIENT_VEPU2);
        TRY_GET_ENC_HAL_API(VPU_CLIENT_VEPU1);
        TRY_GET_ENC_HAL_API(VPU_CLIENT_VEPU22);
    } while (0);

    if (ret) {
        mpp_err_f("could not found coding type %d\n", coding);
        MPP_FREE(p);
    }

    *ctx = p;

    enc_hal_dbg_flow("leave ret %d\n", ret);

    return ret;
}

MPP_RET mpp_enc_hal_deinit(MppEncHal ctx)
{
    MppEncHalImpl *p;

    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    enc_hal_dbg_flow("enter\n");

    p = (MppEncHalImpl*)ctx;
    p->api->deinit(p->ctx);
    mpp_free(p->ctx);
    if (p->tasks)
        hal_task_group_deinit(p->tasks);
    mpp_free(p);

    enc_hal_dbg_flow("leave\n");

    return MPP_OK;
}

MPP_RET mpp_enc_hal_prepare(void *hal)
{
    if (NULL == hal) {
        mpp_err_f("found NULL input ctx %p\n", hal);
        return MPP_ERR_NULL_PTR;
    }

    MppEncHalImpl *p = (MppEncHalImpl*)hal;
    if (!p->api || !p->api->prepare)
        return MPP_OK;

    return p->api->prepare(p->ctx);
}

MPP_RET mpp_enc_hal_check_part_mode(MppEncHal ctx)
{
    MppEncHalImpl *p = (MppEncHalImpl*)ctx;

    if (p && p->api && p->api->part_start && p->api->part_wait)
        return MPP_OK;

    return MPP_NOK;
}

MPP_RET mpp_enc_hal_start(void *hal, HalEncTask *task)
{
    if (NULL == hal || NULL == task) {
        mpp_err_f("found NULL input ctx %p task %p\n", hal, task);
        return MPP_ERR_NULL_PTR;
    }

    MppEncHalImpl *p = (MppEncHalImpl*)hal;
    if (!p->api || !p->api->start)
        return MPP_OK;

    /* Add buffer sync process */
    mpp_buffer_sync_partial_end(task->output, 0, task->length);

    return p->api->start(p->ctx, task);
}

#define MPP_ENC_HAL_TASK_FUNC(func) \
    MPP_RET mpp_enc_hal_##func(void *hal, HalEncTask *task)             \
    {                                                                   \
        if (NULL == hal || NULL == task) {                              \
            mpp_err_f("found NULL input ctx %p task %p\n", hal, task);  \
            return MPP_ERR_NULL_PTR;                                    \
        }                                                               \
                                                                        \
        MppEncHalImpl *p = (MppEncHalImpl*)hal;                         \
        if (!p->api || !p->api->func)                                   \
            return MPP_OK;                                              \
                                                                        \
        return p->api->func(p->ctx, task);                              \
    }

MPP_ENC_HAL_TASK_FUNC(get_task)
MPP_ENC_HAL_TASK_FUNC(gen_regs)
MPP_ENC_HAL_TASK_FUNC(wait)
MPP_ENC_HAL_TASK_FUNC(part_start)
MPP_ENC_HAL_TASK_FUNC(part_wait)
MPP_ENC_HAL_TASK_FUNC(ret_task)
