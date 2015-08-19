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

static MppBufferGroupImpl *search_buffer_group_by_id_no_lock(RK_U32 group_id)
{
    MppBufferGroupImpl *pos, *n;
    list_for_each_entry_safe(pos, n, &services.list_group, MppBufferGroupImpl, list_group) {
        if (pos->group_id == group_id) {
            return pos;
        }
    }
    return NULL;
}

MPP_RET deinit_buffer_no_lock(MppBufferImpl *buffer)
{
    mpp_assert(buffer->ref_count == 0);
    mpp_assert(buffer->used == 0);

    list_del_init(&buffer->list_status);
    MppBufferGroupImpl *group = search_buffer_group_by_id_no_lock(buffer->group_id);
    if (group)
        group->usage -= buffer->size;

    mpp_free(buffer);

    return MPP_OK;
}

static MPP_RET check_buffer_group_limit(MppBufferGroupImpl *group)
{
    if (group->usage > group->limit) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &group->list_unused, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos);
        }
    }

    return (group->usage > group->limit) ? (MPP_NOK) : (MPP_OK);
}

static MPP_RET inc_buffer_ref_no_lock(MppBufferImpl *buffer)
{
    MPP_RET ret = MPP_OK;
    if (!buffer->used) {
        MppBufferGroupImpl *group = search_buffer_group_by_id_no_lock(buffer->group_id);
        // NOTE: when increasing ref_count the unused buffer must be under certain group
        mpp_assert(group);
        buffer->used = 1;
        if (group) {
            list_del_init(&buffer->list_status);
            list_add_tail(&buffer->list_status, &group->list_used);
        } else {
            mpp_err("mpp_buffer_ref_inc unused buffer without group\n");
            ret = MPP_NOK;
        }
    }
    buffer->ref_count++;
    return ret;
}

MPP_RET mpp_buffer_init(MppBufferImpl **buffer, const char *tag, RK_U32 group_id, size_t size, MppBufferData *data)
{
    MppBufferImpl *p = mpp_malloc(MppBufferImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_buffer_init failed to allocate context\n");
        *buffer = NULL;
        return MPP_ERR_MALLOC;
    }

    MPP_BUFFER_SERVICE_LOCK();

    MppBufferGroupImpl *group = search_buffer_group_by_id_no_lock(group_id);
    if (group) {
        if (NULL == tag) {
            tag = group->tag;
        }
        strncpy(p->tag, tag, sizeof(p->tag));
        p->group_id = group_id;
        p->size = size;
        if (data) {
            p->data = *data;
        } else {
            // TODO: if data is NULL need to allocate from allocator
        }
        p->used = 0;
        p->ref_count = 0;
        INIT_LIST_HEAD(&p->list_status);
        list_add_tail(&p->list_status, &group->list_unused);
        group->usage += size;
    } else {
        mpp_free(p);
        p = NULL;
    }

    MPP_BUFFER_SERVICE_UNLOCK();

    *buffer = p;
    return (p) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_buffer_deinit(MppBufferImpl *buffer)
{
    MPP_BUFFER_SERVICE_LOCK();

    deinit_buffer_no_lock(buffer);

    MPP_BUFFER_SERVICE_UNLOCK();

    return MPP_OK;
}

MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer)
{
    MPP_RET ret;

    MPP_BUFFER_SERVICE_LOCK();

    ret = inc_buffer_ref_no_lock(buffer);

    MPP_BUFFER_SERVICE_UNLOCK();

    return ret;
}


MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer)
{
    if (buffer->ref_count <= 0) {
        mpp_err("mpp_buffer_ref_dec found non-positive ref_count %d\n", buffer->ref_count);
        return MPP_NOK;
    }

    MPP_BUFFER_SERVICE_LOCK();

    buffer->ref_count--;
    if (0 == buffer->ref_count) {
        buffer->used = 0;
        list_del_init(&buffer->list_status);
        MppBufferGroupImpl *group = search_buffer_group_by_id_no_lock(buffer->group_id);
        if (group) {
            list_add_tail(&buffer->list_status, &group->list_unused);
            check_buffer_group_limit(group);
        }
    }

    MPP_BUFFER_SERVICE_UNLOCK();

    return MPP_OK;
}

MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size)
{
    MppBufferImpl *buffer = NULL;

    MPP_BUFFER_SERVICE_LOCK();

    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            if (pos->size == size) {
                buffer = pos;
                inc_buffer_ref_no_lock(buffer);
                break;
            }
        }
    }

    MPP_BUFFER_SERVICE_UNLOCK();

    return buffer;
}

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, MppBufferType type)
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

    list_add_tail(&p->list_group, &services.list_group);

    snprintf(p->tag, sizeof(p->tag), "%s_%d", tag, services.group_id);
    p->type     = type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->usage    = 0;
    p->group_id = services.group_id;

    // avoid group_id reuse
    do {
        services.group_count++;
        if (NULL == search_buffer_group_by_id_no_lock(services.group_count))
            break;
    } while (p->group_id != services.group_count);

    MPP_BUFFER_SERVICE_UNLOCK();
    *group = p;

    return MPP_OK;
}

MPP_RET mpp_buffer_group_deinit(MppBufferGroupImpl *p)
{
    if (NULL == p) {
        mpp_err("mpp_buffer_group_deinit found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUFFER_SERVICE_LOCK();

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos);
        }
    }

    // move used buffer to orphan list
    if (!list_empty(&p->list_used)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            list_add_tail(&pos->list_status, &services.list_orphan);
        }
    }

    list_del_init(&p->list_group);
    services.group_count--;

    MPP_BUFFER_SERVICE_UNLOCK();

    return MPP_OK;
}

MPP_RET mpp_buffer_group_limit_reset(MppBufferGroupImpl *p, size_t limit)
{
    mpp_log("mpp_buffer_group %p limit reset from %u to %u\n", p->limit, limit);

    MPP_BUFFER_SERVICE_LOCK();
    p->limit = limit;
    MPP_RET ret = check_buffer_group_limit(p);
    MPP_BUFFER_SERVICE_UNLOCK();

    return ret;
}

