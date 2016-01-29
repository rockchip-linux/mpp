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
#include "mpp_env.h"

#include "mpp_buffer_impl.h"

#define SEARCH_GROUP_NORMAL(id)         search_group_by_id_no_lock(&service.mListGroup,  id)
#define SEARCH_GROUP_ORPHAN(id)         search_group_by_id_no_lock(&service.mListOrphan, id)

typedef MPP_RET (*BufferOp)(MppAllocator allocator, MppBufferInfo *data);

typedef enum MppBufOps_e {
    BUF_COMMIT,
    BUF_CREATE,
    BUF_REF_INC,
    BUF_REF_DEC,
    BUF_DESTROY,
    BUF_OPS_BUTT,
} MppBufOps;

typedef struct MppBufLog_t {
    struct list_head    list;
    MppBuffer           buf;
    MppBufOps           ops;
    RK_U32              val_in;
    RK_U32              val_out;
} MppBufLog;


class MppBufferService
{
public:
    MppBufferService();
    ~MppBufferService();

    Mutex               mLock;
    RK_U32              group_id;
    RK_U32              group_count;

    MppBufferGroupImpl  *mLegacyGroup;

    struct list_head    mListGroup;

    // list for used buffer which do not have group
    struct list_head    mListOrphan;
};

static const char *mode2str[MPP_BUFFER_MODE_BUTT] = {
    "internal",
    "external",
};

static const char *type2str[MPP_BUFFER_TYPE_BUTT] = {
    "normal",
    "ion",
    "v4l2",
};
#if 0
static const char *ops2str[BUF_OPS_BUTT] = {
    "commit",
    "create",
    "ref inc",
    "ref dec",
    "destroy",
};
#endif
RK_U32 mpp_buffer_debug = 0;
static MppBufferService service;
#if 0
static void add_buf_log(MppBufferGroupImpl *group, MppBufferImpl *buffer, MppBufOps ops, RK_U32 val_in, RK_U32 val_out)
{
    if (group->log_runtime_en) {
        mpp_log("group %p buffer %p ops %s val in %d out %d\n", group, buffer, ops2str[ops], val_in, val_out);
    }
    if (group->log_history_en) {
        MppBufLog *log = mpp_malloc(MppBufLog, 1);
        if (log) {
            INIT_LIST_HEAD(&log->list);
            log->buf = buffer;
            log->ops = ops;
            log->val_in = val_in;
            log->val_out = val_out;
        }
    }
}
#endif
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
        BufferOp func = (group->mode == MPP_BUFFER_INTERNAL) ?
                        (group->alloc_api->free) :
                        (group->alloc_api->release);
        func(group->allocator, &buffer->info);
        group->usage -= buffer->info.size;
        group->count--;
        if (!buffer->discard)
            group->count_unused--;
    } else {
        group = SEARCH_GROUP_ORPHAN(buffer->group_id);
        mpp_assert(group);
        mpp_assert(buffer->mode == MPP_BUFFER_INTERNAL);
        group->alloc_api->free(group->allocator, &buffer->info);
        group->usage -= buffer->info.size;
        group->count--;
        if (0 == group->usage)
            deinit_group_no_lock(group);
    }

    mpp_free(buffer);

    return MPP_OK;
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
            group->count_used++;
            group->count_unused--;
        } else {
            mpp_err_f("unused buffer without group\n");
            ret = MPP_NOK;
        }
    }
    buffer->ref_count++;
    return ret;
}

static void dump_buffer_info(MppBufferImpl *buffer)
{
    mpp_log("buffer %p fd %4d size %10d ref_count %3d caller %s\n",
            buffer, buffer->info.fd, buffer->info.size, buffer->ref_count, buffer->caller);
}

MPP_RET mpp_buffer_create(const char *tag, const char *caller, RK_U32 group_id, MppBufferInfo *info)
{
    AutoMutex auto_lock(&service.mLock);

    MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(group_id);
    if (NULL == group) {
        mpp_err_f("can not create buffer without group\n");
        return MPP_NOK;
    }

    if (group->limit_count && group->count >= group->limit_count) {
        mpp_err_f("reach group count limit %d\n", group->limit_count);
        return MPP_NOK;
    }

    if (group->limit_size && info->size > group->limit_size) {
        mpp_err_f("required size %d reach group size limit %d\n", info->size, group->limit_size);
        return MPP_NOK;
    }

    MppBufferImpl *p = mpp_calloc(MppBufferImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    BufferOp func = (group->mode == MPP_BUFFER_INTERNAL) ?
                    (group->alloc_api->alloc) :
                    (group->alloc_api->import);
    MPP_RET ret = func(group->allocator, info);
    if (MPP_OK != ret) {
        mpp_err_f("failed to create buffer with size %d\n", info->size);
        mpp_free(p);
        return MPP_ERR_MALLOC;
    }

    p->info = *info;
    p->mode = group->mode;

    if (NULL == tag)
        tag = group->tag;

    strncpy(p->tag, tag, sizeof(p->tag));
    p->caller = caller;
    p->group_id = group_id;
    INIT_LIST_HEAD(&p->list_status);
    list_add_tail(&p->list_status, &group->list_unused);
    group->usage += info->size;
    group->count++;
    group->count_unused++;
    return MPP_OK;
}

MPP_RET mpp_buffer_destroy(MppBufferImpl *buffer)
{
    AutoMutex auto_lock(&service.mLock);

    deinit_buffer_no_lock(buffer);

    return MPP_OK;
}

MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer)
{
    AutoMutex auto_lock(&service.mLock);
    return inc_buffer_ref_no_lock(buffer);
}


MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer)
{
    AutoMutex auto_lock(&service.mLock);

    if (buffer->ref_count <= 0) {
        mpp_err_f("found non-positive ref_count %d\n", buffer->ref_count);
        return MPP_NOK;
    }

    buffer->ref_count--;
    if (0 == buffer->ref_count) {
        buffer->used = 0;
        list_del_init(&buffer->list_status);
        MppBufferGroupImpl *group = SEARCH_GROUP_NORMAL(buffer->group_id);
        if (group) {
            if (group == service.mLegacyGroup) {
                deinit_buffer_no_lock(buffer);
            } else {
                if (buffer->discard) {
                    deinit_buffer_no_lock(buffer);
                } else {
                    list_add_tail(&buffer->list_status, &group->list_unused);
                    group->count_unused++;
                }
            }
            group->count_used--;
            if (group->listener) {
                MppThread *thread = (MppThread *)group->listener;
                thread->signal();
            }
        }
    }

    return MPP_OK;
}

MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size)
{
    MppBufferImpl *buffer = NULL;

    AutoMutex auto_lock(&service.mLock);

    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            //mpp_log("[RKV_DEBUG]pos->info.size=%d, size=%d \n", pos->info.size, size);
            if (pos->info.size >= size) {
                buffer = pos;
                inc_buffer_ref_no_lock(buffer);
                break;
            } else {
                if (MPP_BUFFER_INTERNAL == p->mode)
                    deinit_buffer_no_lock(pos);
            }
        }
    }

    return buffer;
}

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, const char *caller,
                              MppBufferMode mode, MppBufferType type)
{
    MppBufferGroupImpl *p = mpp_calloc(MppBufferGroupImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to allocate context\n");
        *group = NULL;
        return MPP_ERR_MALLOC;
    }

    mpp_assert(caller);

    AutoMutex auto_lock(&service.mLock);

    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);
    INIT_LIST_HEAD(&p->list_logs);

    list_add_tail(&p->list_group, &service.mListGroup);

    if (tag) {
        snprintf(p->tag, sizeof(p->tag), "%s_%d", tag, service.group_id);
    } else {
        snprintf(p->tag, sizeof(p->tag), "unknown");
    }
    p->caller   = caller;
    p->mode     = mode;
    p->type     = type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->group_id = service.group_id;

    mpp_env_get_u32("mpp_buffer_debug", &mpp_buffer_debug, 0);
    p->log_runtime_en   = (mpp_buffer_debug | MPP_BUF_DBG_OPS_RUNTIME) ? (1) : (0);
    p->log_history_en   = (mpp_buffer_debug | MPP_BUF_DBG_OPS_HISTORY) ? (1) : (0);

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
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    AutoMutex auto_lock(&service.mLock);

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

        mpp_err("mpp_group %p tag %s caller %s mode %s type %s deinit with %d bytes not released\n",
                p, p->tag, p->caller, mode2str[p->mode], type2str[p->type], p->usage);

        mpp_buffer_group_dump(p);
    }

    return MPP_OK;
}

MPP_RET mpp_buffer_group_reset(MppBufferGroupImpl *p)
{
    if (NULL == p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    AutoMutex auto_lock(&service.mLock);

    if (!list_empty(&p->list_used)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            // mpp_buffer_ref_dec(pos);
            pos->discard = 1;
        }
    }

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos);
        }
    }

    return MPP_OK;
}

MPP_RET mpp_buffer_group_set_listener(MppBufferGroupImpl *p, void *listener)
{
    if (NULL == p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    AutoMutex auto_lock(&service.mLock);
    p->listener = listener;

    return MPP_OK;
}

void mpp_buffer_group_dump(MppBufferGroupImpl *group)
{
    mpp_log("\ndumping buffer group %p id %d\n", group, group->group_id);
    mpp_log("mode %s\n", mode2str[group->mode]);
    mpp_log("type %s\n", type2str[group->type]);
    mpp_log("limit size %d count %d\n", group->limit_size, group->limit_count);

    mpp_log("\nused buffer count %d\n", group->count_used);

    MppBufferImpl *pos, *n;
    list_for_each_entry_safe(pos, n, &group->list_used, MppBufferImpl, list_status) {
        dump_buffer_info(pos);
    }

    mpp_log("\nunused buffer count %d\n", group->count_unused);
    list_for_each_entry_safe(pos, n, &group->list_unused, MppBufferImpl, list_status) {
        dump_buffer_info(pos);
    }
}

MppBufferGroupImpl *mpp_buffer_legacy_group()
{
    return service.mLegacyGroup;
}

MppBufferService::MppBufferService()
    : group_id(0),
      group_count(0),
      mLegacyGroup(NULL)
{
    INIT_LIST_HEAD(&mListGroup);
    INIT_LIST_HEAD(&mListOrphan);

    // NOTE: here can not call mpp_buffer_group_init for the service is not started
    MppBufferGroupImpl *p = mpp_calloc(MppBufferGroupImpl, 1);
    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);
    INIT_LIST_HEAD(&p->list_logs);

    list_add_tail(&p->list_group, &mListGroup);

    snprintf(p->tag, sizeof(p->tag), "legacy");
    p->mode     = MPP_BUFFER_INTERNAL;
    p->type     = MPP_BUFFER_TYPE_ION;

    group_id++;
    group_count++;

    mpp_alloctor_get(&p->allocator, &p->alloc_api, MPP_BUFFER_TYPE_ION);

    mLegacyGroup = p;
}

MppBufferService::~MppBufferService()
{
    // first remove legacy group
    if (mLegacyGroup)
        mpp_buffer_group_deinit(mLegacyGroup);

    // then remove the reset group
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


