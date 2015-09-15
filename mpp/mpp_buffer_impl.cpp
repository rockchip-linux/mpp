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

#define SEARCH_GROUP_NORMAL(id)         search_group_by_id_no_lock(&service.mListGroup,  id)
#define SEARCH_GROUP_ORPHAN(id)         search_group_by_id_no_lock(&service.mListOrphan, id)

class MppBufferService
{
public:
    MppBufferService();
    ~MppBufferService();

    Mutex               mLock;
    RK_U32              group_id;
    RK_U32              group_count;

    struct list_head    mListGroup;

    // list for used buffer which do not have group
    struct list_head    mListOrphan;
};

static MppBufferService service;

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
        if (buffer->internal)
            group->alloc_api->free(group->allocator, &buffer->info);

        group->usage -= buffer->info.size;
        group->count--;
    } else {
        group = SEARCH_GROUP_ORPHAN(buffer->group_id);
        mpp_assert(group);
        mpp_assert(buffer->mode == MPP_BUFFER_MODE_NORMAL);
        group->alloc_api->free(group->allocator, &buffer->info);
        group->usage -= buffer->info.size;
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

MPP_RET mpp_buffer_create(const char *tag, RK_U32 group_id, MppBufferInfo *info)
{
    MppBufferImpl *p = mpp_calloc(MppBufferImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_buffer_create failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    Mutex::Autolock auto_lock(&service.mLock);

    MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(group_id);
    if (group) {
        if (NULL == info->ptr && info->fd == -1) {
            MPP_RET ret = group->alloc_api->alloc(group->allocator, info);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_create failed to create buffer with size %d\n", info->size);
                mpp_free(p);
                return MPP_ERR_MALLOC;
            }
            p->internal = 1;
        }

        p->info = *info;
        p->mode = group->mode;

        if (NULL == tag)
            tag = group->tag;

        strncpy(p->tag, tag, sizeof(p->tag));
        p->group_id = group_id;
        INIT_LIST_HEAD(&p->list_status);
        list_add_tail(&p->list_status, &group->list_unused);
        group->usage += info->size;
        group->count++;
    } else {
        mpp_err("mpp_buffer_create can not create buffer without group\n");
        mpp_free(p);
        p = NULL;
    }

    return (p) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_buffer_destroy(MppBufferImpl *buffer)
{
    Mutex::Autolock auto_lock(&service.mLock);

    deinit_buffer_no_lock(buffer);

    return MPP_OK;
}

MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer)
{
    Mutex::Autolock auto_lock(&service.mLock);
    return inc_buffer_ref_no_lock(buffer);
}


MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer)
{
    if (buffer->ref_count <= 0) {
        mpp_err("mpp_buffer_ref_dec found non-positive ref_count %d\n", buffer->ref_count);
        return MPP_NOK;
    }

    Mutex::Autolock auto_lock(&service.mLock);

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

    return MPP_OK;
}

MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size)
{
    MppBufferImpl *buffer = NULL;

    Mutex::Autolock auto_lock(&service.mLock);

    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            if (pos->info.size == size) {
                buffer = pos;
                inc_buffer_ref_no_lock(buffer);
                break;
            }
        }
    }

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

    Mutex::Autolock auto_lock(&service.mLock);

    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);

    list_add_tail(&p->list_group, &service.mListGroup);

    snprintf(p->tag, sizeof(p->tag), "%s_%d", tag, service.group_id);
    p->mode     = mode;
    p->type     = type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->usage    = 0;
    p->count    = 0;
    p->group_id = service.group_id;
    p->limit_size   = 0;
    p->limit_count  = 0;

    // avoid group_id reuse
    do {
        service.group_id++;
        if (NULL == SEARCH_GROUP_NORMAL(service.group_id) &&
            NULL == SEARCH_GROUP_ORPHAN(service.group_id))
            break;
    } while (p->group_id != service.group_id);
    service.group_count++;

    mpp_alloctor_get(&p->allocator, &p->alloc_api, type);

    *group = p;

    return MPP_OK;
}

MPP_RET mpp_buffer_group_deinit(MppBufferGroupImpl *p)
{
    if (NULL == p) {
        mpp_err("mpp_buffer_group_deinit found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    Mutex::Autolock auto_lock(&service.mLock);

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
        list_add_tail(&p->list_group, &service.mListOrphan);
        mpp_err("mpp_group %p tag %s deinit with %d buffer not released\n", p, p->tag, p->usage);
        // if any buffer with mode MPP_BUFFER_MODE_COMMIT found it should be error
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            mpp_assert(pos->mode != MPP_BUFFER_MODE_LIMIT);
        }
    }

    return MPP_OK;
}

MppBufferService::MppBufferService()
    : group_id(0),
      group_count(0)
{
    INIT_LIST_HEAD(&mListGroup);
    INIT_LIST_HEAD(&mListOrphan);
}

MppBufferService::~MppBufferService()
{
    // remove all group first
    if (!list_empty(&mListGroup)) {
        MppBufferGroupImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &mListGroup, MppBufferGroupImpl, list_group) {
            mpp_buffer_group_deinit(pos);
        }
    }

    // remove all orphan buffer
    if (!list_empty(&mListOrphan)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &mListOrphan, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos);
        }
    }
}


