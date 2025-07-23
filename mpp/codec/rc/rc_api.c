/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2016 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "rc_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_2str.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "rc_debug.h"
#include "rc_api.h"

#include "h264e_rc.h"
#include "h265e_rc.h"
#include "jpege_rc.h"
#include "vp8e_rc.h"
#include "rc_model_v2_smt.h"

#define get_srv_rc_api_srv(caller) \
    ({ \
        MppRcApiSrv *__tmp; \
        if (!rc_api_srv) { \
            rc_api_srv_init(); \
        } \
        if (rc_api_srv) { \
            __tmp = rc_api_srv; \
        } else { \
            mpp_err("mpp rc api srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

// use class to register RcImplApi
typedef struct RcImplApiNode_t {
    /* list to list in MppRcApiSrv */
    struct list_head    list;
    char                name[64];
    MppCodingType       type;
    RcApiBrief          brief;
    RcImplApi           api;
} RcImplApiNode;

typedef struct MppRcApiSrv_t {
    MppMutex            lock;
    RK_U32              api_cnt;
    /* list for list in RcImplApiNode */
    struct list_head    list;
} MppRcApiSrv;

static MppRcApiSrv *rc_api_srv = NULL;

static void rc_api_srv_init()
{
    MppRcApiSrv *srv = rc_api_srv;

    mpp_env_get_u32("rc_debug", &rc_debug, 0);

    if (srv)
        return;

    srv = mpp_malloc(MppRcApiSrv, 1);
    if (!srv) {
        mpp_err_f("failed to create rc api srv\n");
        return;
    }

    rc_api_srv = srv;

    mpp_mutex_init(&srv->lock);
    INIT_LIST_HEAD(&srv->list);
    srv->api_cnt = 0;

    /* add all default rc apis */
    rc_api_add(&default_h264e);
    rc_api_add(&default_h265e);
    rc_api_add(&default_jpege);
    rc_api_add(&default_vp8e);
    rc_api_add(&smt_h264e);
    rc_api_add(&smt_h265e);
}

static void rc_api_srv_deinit()
{
    MppRcApiSrv *srv = rc_api_srv;

    if (!srv)
        return;

    mpp_mutex_lock(&srv->lock);

    if (srv->api_cnt) {
        RcImplApiNode *pos, *n;

        list_for_each_entry_safe(pos, n, &srv->list, RcImplApiNode, list) {
            rc_dbg_impl("%-5s rc api %s is removed\n",
                        strof_coding_type(pos->type), pos->name);

            list_del_init(&pos->list);
            MPP_FREE(pos);
            srv->api_cnt--;
        }

        mpp_assert(srv->api_cnt == 0);
    }

    mpp_mutex_unlock(&srv->lock);
    mpp_mutex_destroy(&srv->lock);
    MPP_FREE(srv);
    rc_api_srv = NULL;
}

static RcImplApi *_rc_api_get(MppRcApiSrv *srv, MppCodingType type, const char *name)
{
    if (!srv->api_cnt)
        return NULL;

    if (name) {
        RcImplApiNode *pos, *n;

        list_for_each_entry_safe(pos, n, &srv->list, RcImplApiNode, list) {
            if (type == pos->type &&
                !strncmp(name, pos->name, sizeof(pos->name) - 1)) {
                rc_dbg_impl("%-5s rc api %s is selected\n",
                            strof_coding_type(type), pos->name);
                return &pos->api;
            }
        }
    }

    rc_dbg_impl("%-5s rc api %s can not be found\n", strof_coding_type(type), name);

    return NULL;
}

static void set_node_api(RcImplApiNode *node, const RcImplApi *api)
{
    node->api = *api;
    node->type = api->type;

    strncpy(node->name, api->name, sizeof(node->name) - 1);
    node->api.name = api->name;

    node->brief.type = api->type;
    node->brief.name = api->name;
}

MPP_RET rc_api_add(const RcImplApi *api)
{
    MppRcApiSrv *srv = get_srv_rc_api_srv(__FUNCTION__);
    RcImplApiNode *node = NULL;
    RcImplApi *node_api = NULL;

    if (!api) {
        mpp_err_f("unable to register NULL api\n");
        return MPP_NOK;
    }

    if (!srv)
        return MPP_NOK;

    mpp_mutex_lock(&srv->lock);

    /* search for same node for replacement */
    node_api = _rc_api_get(srv, api->type, api->name);

    if (!node_api) {
        node = mpp_malloc(RcImplApiNode, 1);
        if (!node) {
            mpp_err_f("failed to create api node\n");
            mpp_mutex_unlock(&srv->lock);
            return MPP_NOK;
        }

        INIT_LIST_HEAD(&node->list);
        list_add_tail(&node->list, &srv->list);

        srv->api_cnt++;
        rc_dbg_impl("%-5s rc api %s is added\n", strof_coding_type(api->type), api->name);
    } else {
        node = container_of(node_api, RcImplApiNode, api);
        rc_dbg_impl("%-5s rc api %s is updated\n", strof_coding_type(api->type), api->name);
    }

    set_node_api(node, api);
    mpp_mutex_unlock(&srv->lock);

    return MPP_OK;
}

RcImplApi *rc_api_get(MppCodingType type, const char *name)
{
    MppRcApiSrv *srv = get_srv_rc_api_srv(__FUNCTION__);

    if (!srv)
        return NULL;

    return _rc_api_get(srv, type, name);
}

MPP_RET rc_api_get_all(MppRcApiSrv *srv, RcApiBrief *brief, RK_S32 *count, RK_S32 max_count)
{
    RcImplApiNode *pos, *n;
    RK_S32 cnt = 0;

    mpp_mutex_lock(&srv->lock);

    list_for_each_entry_safe(pos, n, &srv->list, RcImplApiNode, list) {
        if (cnt >= max_count)
            break;

        brief[cnt++] = pos->brief;
    }

    *count = cnt;
    mpp_mutex_unlock(&srv->lock);

    return MPP_OK;
}

MPP_RET rc_api_get_by_type(MppRcApiSrv *srv, RcApiBrief *brief, RK_S32 *count,
                           RK_S32 max_count, MppCodingType type)
{
    RcImplApiNode *pos, *n;
    RK_S32 cnt = 0;

    mpp_mutex_lock(&srv->lock);

    list_for_each_entry_safe(pos, n, &srv->list, RcImplApiNode, list) {
        if (cnt >= max_count)
            break;

        if (pos->type != type)
            continue;

        brief[cnt++] = pos->brief;
    }

    *count = cnt;
    mpp_mutex_unlock(&srv->lock);

    return MPP_OK;
}

MPP_RET rc_brief_get_all(RcApiQueryAll *query)
{
    MppRcApiSrv *srv = rc_api_srv;
    RcApiBrief *brief;
    RK_S32 *count;
    RK_S32 max_count;

    if (!srv)
        return MPP_NOK;

    if (!query) {
        mpp_err_f("invalide NULL query input\n");
        return MPP_ERR_NULL_PTR;
    }

    brief = query->brief;
    count = &query->count;
    max_count = query->max_count;

    if (!brief || max_count <= 0) {
        mpp_err_f("invalide brief buffer %p max count %d\n", brief, max_count);
        return MPP_NOK;
    }

    return rc_api_get_all(srv, brief, count, max_count);
}

MPP_RET rc_brief_get_by_type(RcApiQueryType *query)
{
    MppRcApiSrv *srv = rc_api_srv;
    RcApiBrief *brief;
    RK_S32 *count;
    RK_S32 max_count;
    MppCodingType type;

    if (!srv)
        return MPP_NOK;

    if (!query) {
        mpp_err_f("invalide NULL query input\n");
        return MPP_ERR_NULL_PTR;
    }

    brief = query->brief;
    count = &query->count;
    max_count = query->max_count;
    type = query->type;

    if (!brief || max_count <= 0) {
        mpp_err_f("invalide brief buffer %p max count %d type %x\n",
                  brief, max_count, type);
        return MPP_NOK;
    }

    return rc_api_get_by_type(srv, brief, count, max_count, type);
}

MPP_SINGLETON(MPP_SGLN_ENC_RC_API, rc_api, rc_api_srv_init, rc_api_srv_deinit)
