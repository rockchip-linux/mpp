/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "rc_impl"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "rc_debug.h"
#include "rc_impl.h"

#include "h264e_rc.h"
#include "h265e_rc.h"
#include "jpege_rc.h"
#include "vp8e_rc.h"
#include "rc_model_v2_smt.h"

const RcImplApi *rc_apis[] = {
    &default_h264e,
    &default_h265e,
    &default_jpege,
    &default_vp8e,
    &smt_h264e,
    &smt_h265e,
};

// use class to register RcImplApi
typedef struct RcImplApiNode_t {
    struct list_head    list;
    char                name[32];
    MppCodingType       type;
    RcApiBrief          brief;
    RcImplApi           api;
} RcImplApiNode;

static void set_node_api(RcImplApiNode *node, const RcImplApi *api)
{
    node->api = *api;
    node->type = api->type;

    strncpy(node->name, api->name, sizeof(node->name) - 1);
    node->api.name = api->name;

    node->brief.type = api->type;
    node->brief.name = api->name;
}

RcImplApiService::RcImplApiService()
{
    RK_U32 i;

    mpp_env_get_u32("rc_debug", &rc_debug, 0);

    INIT_LIST_HEAD(&mApis);
    mApiCount = 0;

    for (i = 0; i < MPP_ARRAY_ELEMS(rc_apis); i++)
        api_add(rc_apis[i]);
}

RcImplApiService::~RcImplApiService()
{
    AutoMutex auto_lock(get_lock());
    RcImplApiNode *pos, *n;

    list_for_each_entry_safe(pos, n, &mApis, RcImplApiNode, list) {
        MPP_FREE(pos);
        mApiCount--;
    }

    mpp_assert(mApiCount == 0);
}

MPP_RET RcImplApiService::api_add(const RcImplApi *api)
{
    AutoMutex auto_lock(get_lock());

    if (NULL == api) {
        mpp_err_f("unable to register NULL api\n");
        return MPP_NOK;
    }

    /* search for same node for replacement */
    RcImplApiNode *node = NULL;
    RcImplApi *node_api = api_get(api->type, api->name);

    if (NULL == node_api) {
        node = mpp_malloc(RcImplApiNode, 1);
        if (NULL == node) {
            mpp_err_f("failed to create api node\n");
            return MPP_NOK;
        }

        INIT_LIST_HEAD(&node->list);
        list_add_tail(&node->list, &mApis);

        mApiCount++;
        rc_dbg_impl("rc impl %s type %x is added\n", api->name, api->type);
    } else {
        node = container_of(node_api, RcImplApiNode, api);
        rc_dbg_impl("rc impl %s type %x is updated\n", api->name, api->type);
    }

    set_node_api(node, api);

    return MPP_OK;
}

RcImplApi *RcImplApiService::api_get(MppCodingType type, const char *name)
{
    AutoMutex auto_lock(get_lock());

    if (!mApiCount)
        return NULL;

    if (name) {
        RcImplApiNode *pos, *n;

        list_for_each_entry_safe(pos, n, &mApis, RcImplApiNode, list) {
            if (type == pos->type &&
                !strncmp(name, pos->name, sizeof(pos->name) - 1)) {
                rc_dbg_impl("rc impl %s is selected\n", pos->name);
                return &pos->api;
            }
        }
    }

    rc_dbg_impl("failed to find rc impl %s type %x\n", name, type);

    return NULL;
}

MPP_RET RcImplApiService::api_get_all(RcApiBrief *brief, RK_S32 *count, RK_S32 max_count)
{
    RK_S32 cnt = 0;
    RcImplApiNode *pos, *n;

    AutoMutex auto_lock(get_lock());

    list_for_each_entry_safe(pos, n, &mApis, RcImplApiNode, list) {
        if (cnt >= max_count)
            break;

        brief[cnt++] = pos->brief;
    }

    *count = cnt;

    return MPP_OK;
}

MPP_RET RcImplApiService::api_get_by_type(RcApiBrief *brief, RK_S32 *count,
                                          RK_S32 max_count, MppCodingType type)
{
    RK_S32 cnt = 0;
    RcImplApiNode *pos, *n;

    AutoMutex auto_lock(get_lock());

    list_for_each_entry_safe(pos, n, &mApis, RcImplApiNode, list) {
        if (cnt >= max_count)
            break;

        if (pos->type != type)
            continue;

        brief[cnt++] = pos->brief;
    }

    *count = cnt;

    return MPP_OK;
}

MPP_RET rc_api_add(const RcImplApi *api)
{
    return RcImplApiService::get_instance()->api_add(api);
}

MPP_RET rc_brief_get_all(RcApiQueryAll *query)
{
    if (NULL == query) {
        mpp_err_f("invalide NULL query input\n");
        return MPP_ERR_NULL_PTR;
    }

    RcApiBrief *brief = query->brief;
    RK_S32 *count = &query->count;
    RK_S32 max_count = query->max_count;

    if (NULL == brief || max_count <= 0) {
        mpp_err_f("invalide brief buffer %p max count %d\n", brief, max_count);
        return MPP_NOK;
    }

    return RcImplApiService::get_instance()->api_get_all(brief, count, max_count);
}

MPP_RET rc_brief_get_by_type(RcApiQueryType *query)
{
    if (NULL == query) {
        mpp_err_f("invalide NULL query input\n");
        return MPP_ERR_NULL_PTR;
    }

    RcApiBrief *brief = query->brief;
    RK_S32 *count = &query->count;
    RK_S32 max_count = query->max_count;
    MppCodingType type = query->type;

    if (NULL == brief || max_count <= 0) {
        mpp_err_f("invalide brief buffer %p max count %d type %x\n",
                  brief, max_count, type);
        return MPP_NOK;
    }

    return RcImplApiService::get_instance()->api_get_by_type(brief, count, max_count, type);
}
