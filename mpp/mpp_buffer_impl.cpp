/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define MODULE_TAG "mpp_buffer"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_buffer.h"
#include "mpp_buffer_impl.h"

#define MPP_BUFFER_SERVICE_LOCK()       pthread_mutex_lock(&services.lock)
#define MPP_BUFFER_SERVICE_UNLOCK()     pthread_mutex_unlock(&services.lock)

typedef struct {
    pthread_mutex_t     lock;
    RK_U32              group_id;
    RK_U32              group_count;

    struct list_head    list_group;

    // list for used buffer which do not have group
    struct list_head    list_orphan;
} MppBufferService;

static MppBufferService services =
{
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
    0,
    0,
    LIST_HEAD_INIT(services.list_group),
    LIST_HEAD_INIT(services.list_orphan),
};

static RK_U32 add_group_to_service(MppBufferGroupImpl *p)
{
    list_add_tail(&p->list_group, &services.list_group);
    services.group_count++;
    return services.group_id++;
}

static void del_group_from_service(MppBufferGroupImpl *p)
{
    list_del_init(&p->list_group);
    services.group_count--;
}

static void add_orphan_to_service(MppBufferImpl *p)
{
    list_add_tail(&p->list_status, &services.list_orphan);
}

static void del_orphan_from_service(MppBufferImpl *p)
{
    list_del_init(&p->list_status);
}

MPP_RET mpp_buffer_group_create(MppBufferGroupImpl **group, const char *tag, MppBufferType type)
{
    MppBufferGroupImpl *p = mpp_malloc(MppBufferGroupImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_buffer_group_get failed to allocate context\n");
        *group = NULL;
        return MPP_ERR_MALLOC;
    }

    MPP_BUFFER_SERVICE_LOCK();

    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);

    RK_U32 group_id = add_group_to_service(p);

    snprintf(p->tag, MPP_TAG_SIZE, "%s_%d", tag, group_id);
    p->type     = type;
    p->group_id = group_id;

    MPP_BUFFER_SERVICE_UNLOCK();
    *group = p;

    return MPP_OK;
}

MPP_RET mpp_buffer_group_destroy(MppBufferGroupImpl *p)
{
    if (NULL == p) {
        mpp_err("mpp_buffer_group_destroy found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUFFER_SERVICE_LOCK();

    del_group_from_service(p);

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            mpp_free(pos);
        }
    }

    if (!list_empty(&p->list_used)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            add_orphan_to_service(pos);
        }
    }

    MPP_BUFFER_SERVICE_UNLOCK();

    return MPP_OK;
}

