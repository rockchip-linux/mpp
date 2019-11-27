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

#include "rc_model_v2.h"

const RcImplApi *rc_apis[] = {
    &default_h264e,
    &default_h265e,
};

// use class to register RcImplApi
typedef struct RcImplApiNode_t {
    struct list_head    list;
    char                name[32];
    MppCodingType       type;
    RcImplApi           api;
} RcImplApiNode;

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

    RcImplApiNode *node = mpp_malloc(RcImplApiNode, 1);
    if (NULL == node) {
        mpp_err_f("failed to create api node\n");
        return MPP_NOK;
    }

    INIT_LIST_HEAD(&node->list);
    node->api = *api;
    node->type = api->type;
    strncpy(node->name, api->name, sizeof(node->name) - 1);
    node->api.name = api->name;

    rc_dbg_impl("rc impl %s type %x is added\n", api->name, api->type);

    list_add_tail(&node->list, &mApis);
    mApiCount++;

    return MPP_OK;
}

const RcImplApi *RcImplApiService::api_get(MppCodingType type, const char *name)
{
    AutoMutex auto_lock(get_lock());

    if (name) {
        RcImplApiNode *pos, *n;

        list_for_each_entry_safe(pos, n, &mApis, RcImplApiNode, list) {
            if (type == pos->type &&
                !strncmp(name, pos->name, sizeof(pos->name) - 1)) {
                rc_dbg_impl("rc impl %s is selected\n", pos->name);
                return (const RcImplApi *)&pos->api;
            }
        }
    }

    mpp_err_f("failed to find rc impl %s\n", name);

    return NULL;
}
