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

#define MPP_BUFFER_SERVICE_LOCK()       pthread_mutex_lock(&service.lock)
#define MPP_BUFFER_SERVICE_UNLOCK()     pthread_mutex_unlock(&service.lock)

#define SEARCH_GROUP_NORMAL(id)         search_group_by_id_no_lock(&service.list_group,  id)
#define SEARCH_GROUP_ORPHAN(id)         search_group_by_id_no_lock(&service.list_orphan, id)

typedef struct {
    pthread_mutex_t     lock;
    RK_U32              group_id;
    RK_U32              group_count;

    struct list_head    list_group;

    // list for used buffer which do not have group
    struct list_head    list_orphan;
} MppBufferService;

static MppBufferService service = {
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
    0,
    0,
    LIST_HEAD_INIT(service.list_group),
    LIST_HEAD_INIT(service.list_orphan),
};

static MppBufferGroupImpl *search_group_by_id_no_lock(struct list_head *list, RK_U32 group_id)
{
    MppBufferGroupImpl *pos, *n;
    list_for_each_entry_safe(pos, n, list, MppBufferGroupImpl, list_group) {
        if (pos->group_id == group_id) {
            return pos;
        }
    }
    return NULL;
}

MPP_RET deinit_group_no_lock(MppBufferGroupImpl *group)
{
    mpp_alloctor_put(&group->allocator);
    list_del_init(&group->list_group);
    mpp_free(group);
    service.group_count--;
    return MPP_OK;
}

MPP_RET deinit_buffer_no_lock(MppBufferImpl *buffer)
{
    mpp_assert(buffer->ref_count == 0);
    mpp_assert(buffer->used == 0);

    list_del_init(&buffer->list_status);
    MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(buffer->group_id);
    if (group) {
        if (MPP_BUFFER_MODE_NORMAL == buffer->mode) {
            group->alloc_api->free(group->allocator, &buffer->data);
        }
        group->usage -= buffer->size;
    } else {
        group = SEARCH_GROUP_ORPHAN(buffer->group_id);
        mpp_assert(group);
        mpp_assert(buffer->mode == MPP_BUFFER_MODE_NORMAL);
        group->alloc_api->free(group->allocator, &buffer->data);
        group->usage -= buffer->size;
        group->count--;
        if (0 == group->usage)
            deinit_group_no_lock(group);
    }

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
        MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(buffer->group_id);
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

MPP_RET mpp_buffer_create(const char *tag, RK_U32 group_id, size_t size, MppBufferData *data)
{
    MppBufferImpl *p = mpp_malloc(MppBufferImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_buffer_create failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    MPP_BUFFER_SERVICE_LOCK();

    MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(group_id);
    if (group) {
        if (NULL == data) {
            MppBufferData tmp;
            MPP_RET ret = group->alloc_api->alloc(group->allocator, &tmp, size);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_create failed to create buffer with size %d\n", size);
                mpp_free(p);
                MPP_BUFFER_SERVICE_UNLOCK();
                return MPP_ERR_MALLOC;
            }

            p->data = tmp;
            p->mode = MPP_BUFFER_MODE_NORMAL;
        } else {
            p->data = *data;
            p->mode = MPP_BUFFER_MODE_LIMIT;
        }

        if (NULL == tag)
            tag = group->tag;

        strncpy(p->tag, tag, sizeof(p->tag));
        p->group_id = group_id;
        p->size = size;
        p->type = group->type;
        p->used = 0;
        p->ref_count = 0;
        INIT_LIST_HEAD(&p->list_status);
        list_add_tail(&p->list_status, &group->list_unused);
        group->usage += size;
        group->count++;
    } else {
        mpp_free(p);
        p = NULL;
    }

    MPP_BUFFER_SERVICE_UNLOCK();

    return (p) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_buffer_destroy(MppBufferImpl *buffer)
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
        MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(buffer->group_id);
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

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, MppBufferMode mode, MppBufferType type)
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

    list_add_tail(&p->list_group, &service.list_group);

    snprintf(p->tag, sizeof(p->tag), "%s_%d", tag, service.group_id);
    p->mode     = mode;
    p->type     = type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->usage    = 0;
    p->count    = 0;
    p->group_id = service.group_id;

    // avoid group_id reuse
    do {
        service.group_count++;
        if (NULL == SEARCH_GROUP_NORMAL(service.group_count) &&
            NULL == SEARCH_GROUP_ORPHAN(service.group_count))
            break;
    } while (p->group_id != service.group_count);

    mpp_alloctor_get(&p->allocator, &p->alloc_api);

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

    if (list_empty(&p->list_used)) {
        deinit_group_no_lock(p);
    } else {
        // otherwise move the group to list_orphan and wait for buffer release
        list_del_init(&p->list_group);
        list_add_tail(&p->list_group, &service.list_orphan);
        mpp_err("mpp_group %p deinit with %d buffer not released\n", p, p->usage);
        // if any buffer with mode MPP_BUFFER_MODE_COMMIT found it should be error
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            mpp_assert(pos->mode != MPP_BUFFER_MODE_LIMIT);
        }
    }

    MPP_BUFFER_SERVICE_UNLOCK();

    return MPP_OK;
}

