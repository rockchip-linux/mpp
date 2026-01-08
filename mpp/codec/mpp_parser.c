/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_dec"

#include <string.h>

#include "mpp_2str.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_parser.h"

#define parser_DBG_FLOW                 (0x00000001)
#define parser_DBG_API                  (0x00000002)

#define parser_dbg(flag, fmt, ...)      mpp_dbg(mpp_parser_debug, flag, fmt, ## __VA_ARGS__)
#define parser_dbg_f(flag, fmt, ...)    mpp_dbg_f(mpp_parser_debug, flag, fmt, ## __VA_ARGS__)

#define parser_dbg_flow(fmt, ...)       parser_dbg_f(parser_DBG_FLOW, fmt, ## __VA_ARGS__)
#define parser_dbg_api(fmt, ...)        parser_dbg(parser_DBG_API, fmt, ## __VA_ARGS__)

typedef struct ParserImpl_t {
    const ParserApi     *api;
    void                *ctx;
} ParserImpl;

static const ParserApi *parser_apis[32] = { 0 };

static RK_U32 mpp_parser_debug = 0;
RK_U32 avsd_debug = 0;
RK_U32 avs2d_debug = 0;
RK_U32 h263d_debug = 0;
RK_U32 h264d_debug = 0;
RK_U32 h265d_debug = 0;
RK_U32 jpegd_debug = 0;
RK_U32 m2vd_debug = 0;
RK_U32 mpg4d_debug = 0;
RK_U32 vp8d_debug = 0;
RK_U32 vp9d_debug = 0;
RK_U32 av1d_debug = 0;

MPP_RET mpp_parser_api_register(const ParserApi *api)
{
    const char *str;
    RK_S32 index;

    if (NULL == api) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    str = strof_coding_type(api->coding);

    parser_dbg_api("parser %s adding api %s\n", str, api->name);

    index = mpp_coding_to_index(api->coding);
    if (index < 0 || index >= MPP_ARRAY_ELEMS(parser_apis))
        return MPP_NOK;

    if (NULL != parser_apis[index]) {
        mpp_logw("parser %s replaced %p -> %p\n",
                 str, parser_apis[index], api);
    }

    parser_apis[index] = api;
    return MPP_OK;
}

MPP_RET mpp_parser_init(Parser *prs, ParserCfg *cfg)
{
    ParserImpl *p;
    const ParserApi *api;
    MPP_RET ret;
    RK_S32 index;
    RK_S32 size;
    RK_U32 i;

    if (NULL == prs || NULL == cfg) {
        mpp_loge_f("found NULL input parser %p config %p\n", prs, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *prs = NULL;
    index = mpp_coding_to_index(cfg->coding);
    if (index < 0 || index >= MPP_ARRAY_ELEMS(parser_apis)) {
        mpp_loge_f("invalid coding type %d\n", cfg->coding);
        return MPP_NOK;
    }

    if (!parser_apis[index]) {
        mpp_loge_f("parser %s is not registered\n", strof_coding_type(cfg->coding));
        return MPP_NOK;
    }

    api = parser_apis[index];
    size = sizeof(ParserImpl) + api->ctx_size;
    p = mpp_calloc_size(ParserImpl, size);
    if (NULL == p) {
        mpp_loge_f("failed to alloc parser ctx size %d\n", api->ctx_size);
        return MPP_NOK;
    }

    p->api = api;
    p->ctx = (void *)(p + 1);
    ret = api->init(p->ctx, cfg);
    if (MPP_OK != ret) {
        mpp_loge_f("failed to init parser\n");
        mpp_free(p);
        return ret;
    }

    *prs = p;
    return MPP_OK;
}

MPP_RET mpp_parser_deinit(Parser prs)
{
    ParserImpl *p;

    if (NULL == prs) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (p->api && p->api->deinit)
        p->api->deinit(p->ctx);

    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_parser_prepare(Parser prs, MppPacket pkt, HalDecTask *task)
{
    ParserImpl *p;

    if (NULL == prs || NULL == pkt) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (!p->api || !p->api->prepare)
        return MPP_OK;

    return p->api->prepare(p->ctx, pkt, task);
}

MPP_RET mpp_parser_parse(Parser prs, HalDecTask *task)
{
    ParserImpl *p;

    if (NULL == prs || NULL == task) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (!p->api || !p->api->parse)
        return MPP_OK;

    return p->api->parse(p->ctx, task);
}

MPP_RET mpp_parser_callback(void *prs, void *err_info)
{
    ParserImpl *p;

    if (NULL == prs) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (!p->api || !p->api->callback)
        return MPP_OK;

    return p->api->callback(p->ctx, err_info);
}

MPP_RET mpp_parser_reset(Parser prs)
{
    ParserImpl *p;

    if (NULL == prs) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (!p->api || !p->api->reset)
        return MPP_OK;

    return p->api->reset(p->ctx);
}

MPP_RET mpp_parser_flush(Parser prs)
{
    ParserImpl *p;

    if (NULL == prs) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (!p->api || !p->api->flush)
        return MPP_OK;

    return p->api->flush(p->ctx);
}

MPP_RET mpp_parser_control(Parser prs, MpiCmd cmd, void *para)
{
    ParserImpl *p;

    if (NULL == prs) {
        mpp_loge_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (ParserImpl *)prs;
    if (!p->api || !p->api->control)
        return MPP_OK;

    return p->api->control(p->ctx, cmd, para);
}

