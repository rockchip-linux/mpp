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

#define SEARCH_GROUP_BY_ID(id)  ((MppBufferService::get_instance())->get_group_by_id(id))

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
    RK_S32              ref_count;
} MppBufLog;

// use this class only need it to init legacy group before main
class MppBufferService
{
private:

    // avoid any unwanted function
    MppBufferService();
    ~MppBufferService();
    MppBufferService(const MppBufferService &);
    MppBufferService &operator=(const MppBufferService &);

    // buffer group final release function
    void                release_group(MppBufferGroupImpl *group);

    RK_U32              get_group_id();
    RK_U32              group_id;
    RK_U32              group_count;
    RK_U32              buffer_id;
    RK_U32              buffer_count;

    MppBufferGroupImpl  *mLegacyGroup;

    struct list_head    mListGroup;

    // list for used buffer which do not have group
    struct list_head    mListOrphan;

    // buffer log function
    RK_U32              log_runtime_en;
    RK_U32              log_history_en;
    struct list_head    list_logs;

public:
    static MppBufferService *get_instance()
    {
        static MppBufferService instance;
        return &instance;
    }
    static Mutex *get_lock()
    {
        static Mutex lock;
        return &lock;
    }

    MppBufferGroupImpl  *get_group(const char *tag, const char *caller, MppBufferMode mode, MppBufferType type);
    MppBufferGroupImpl  *get_legacy_group();
    void                put_group(MppBufferGroupImpl *group);

    MppBufferGroupImpl  *get_group_by_id(RK_U32 id);
    void                add_log(MppBufferGroupImpl *group, MppBufferImpl *buffer, MppBufOps ops);

};

static const char *mode2str[MPP_BUFFER_MODE_BUTT] = {
    "internal",
    "external",
};

static const char *type2str[MPP_BUFFER_TYPE_BUTT] = {
    "normal",
    "ion",
    "v4l2",
	"drm",
};
static const char *ops2str[BUF_OPS_BUTT] = {
    "commit",
    "create",
    "ref inc",
    "ref dec",
    "destroy",
};

RK_U32 mpp_buffer_debug = 0;

MPP_RET deinit_buffer_no_lock(MppBufferImpl *buffer)
{
    mpp_assert(buffer->ref_count == 0);
    mpp_assert(buffer->used == 0);

    list_del_init(&buffer->list_status);
    MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);
    if (!group->is_orphan) {
        BufferOp func = (group->mode == MPP_BUFFER_INTERNAL) ?
                        (group->alloc_api->free) :
                        (group->alloc_api->release);
        func(group->allocator, &buffer->info);
        group->usage -= buffer->info.size;
        group->count--;
    } else {
        mpp_assert(buffer->mode == MPP_BUFFER_INTERNAL);
        group->alloc_api->free(group->allocator, &buffer->info);
        group->usage -= buffer->info.size;
        group->count--;
        if (0 == group->usage)
            MppBufferService::get_instance()->put_group(group);
    }

    mpp_free(buffer);

    return MPP_OK;
}

static MPP_RET inc_buffer_ref_no_lock(MppBufferImpl *buffer)
{
    MPP_RET ret = MPP_OK;
    if (!buffer->used) {
        MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);
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
    mpp_log("buffer %p fd %4d size %10d ref_count %3d discard %d caller %s\n",
            buffer, buffer->info.fd, buffer->info.size,
            buffer->ref_count, buffer->discard, buffer->caller);
}

MPP_RET mpp_buffer_create(const char *tag, const char *caller, RK_U32 group_id, MppBufferInfo *info)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = MPP_OK;
    BufferOp func = NULL;
    MppBufferImpl *p = NULL;
    MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(group_id);
    if (NULL == group) {
        mpp_err_f("can not create buffer without group\n");
        ret = MPP_NOK;
        goto RET;
    }

    if (group->limit_count && group->count >= group->limit_count) {
        mpp_err_f("reach group count limit %d\n", group->limit_count);
        ret = MPP_NOK;
        goto RET;
    }

    if (group->limit_size && info->size > group->limit_size) {
        mpp_err_f("required size %d reach group size limit %d\n", info->size, group->limit_size);
        ret = MPP_NOK;
        goto RET;
    }

    p = mpp_calloc(MppBufferImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to allocate context\n");
        ret = MPP_ERR_MALLOC;
        goto RET;
    }

    func = (group->mode == MPP_BUFFER_INTERNAL) ?
           (group->alloc_api->alloc) : (group->alloc_api->import);
    ret = func(group->allocator, info);
    if (MPP_OK != ret) {
        mpp_err_f("failed to create buffer with size %d\n", info->size);
        mpp_free(p);
        ret = MPP_ERR_MALLOC;
        goto RET;
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
RET:
    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MPP_RET mpp_buffer_destroy(MppBufferImpl *buffer)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = deinit_buffer_no_lock(buffer);

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = inc_buffer_ref_no_lock(buffer);

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}


MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = MPP_OK;

    if (buffer->ref_count <= 0) {
        mpp_err_f("found non-positive ref_count %d caller %s\n",
                  buffer->ref_count, buffer->caller);
        ret = MPP_NOK;
    } else {
        buffer->ref_count--;
        if (0 == buffer->ref_count) {
            buffer->used = 0;
            list_del_init(&buffer->list_status);
            MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);
            if (group == MppBufferService::get_instance()->get_legacy_group()) {
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

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MppBufferImpl *buffer = NULL;

    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            //mpp_log("[RKV_DEBUG]pos->info.size=%d, size=%d \n", pos->info.size, size);
            if (pos->info.size >= size) {
                buffer = pos;
                inc_buffer_ref_no_lock(buffer);
                break;
            } else {
                if (MPP_BUFFER_INTERNAL == p->mode) {
                    deinit_buffer_no_lock(pos);
                    p->count_unused--;
                }
            }
        }
    }

    MPP_BUF_FUNCTION_LEAVE();
    return buffer;
}

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, const char *caller,
                              MppBufferMode mode, MppBufferType type)
{
    AutoMutex auto_lock(MppBufferService::get_lock());

    mpp_assert(caller);
    MPP_BUF_FUNCTION_ENTER();

    *group = MppBufferService::get_instance()->get_group(tag, caller, mode, type);

    MPP_BUF_FUNCTION_LEAVE();
    return ((*group) ? (MPP_OK) : (MPP_NOK));
}

MPP_RET mpp_buffer_group_deinit(MppBufferGroupImpl *p)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    if (NULL == p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUF_FUNCTION_ENTER();

    MppBufferService::get_instance()->put_group(p);

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

MPP_RET mpp_buffer_group_reset(MppBufferGroupImpl *p)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    if (NULL == p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUF_FUNCTION_ENTER();

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
            p->count_unused--;
        }
    }

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

MPP_RET mpp_buffer_group_set_listener(MppBufferGroupImpl *p, void *listener)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    if (NULL == p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUF_FUNCTION_ENTER();

    p->listener = listener;

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

void mpp_buffer_group_dump(MppBufferGroupImpl *group)
{
    mpp_log("\ndumping buffer group %p id %d\n", group, group->group_id);
    mpp_log("mode %s\n", mode2str[group->mode]);
    mpp_log("type %s\n", type2str[group->type]);
    mpp_log("limit size %d count %d\n", group->limit_size, group->limit_count);

    mpp_log("used buffer count %d\n", group->count_used);

    MppBufferImpl *pos, *n;
    list_for_each_entry_safe(pos, n, &group->list_used, MppBufferImpl, list_status) {
        dump_buffer_info(pos);
    }

    mpp_log("unused buffer count %d\n", group->count_unused);
    list_for_each_entry_safe(pos, n, &group->list_unused, MppBufferImpl, list_status) {
        dump_buffer_info(pos);
    }
}

MppBufferGroupImpl *mpp_buffer_legacy_group()
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    return MppBufferService::get_instance()->get_legacy_group();
}

MppBufferService::MppBufferService()
    : group_id(0),
      group_count(0),
      buffer_id(0),
      buffer_count(0),
      mLegacyGroup(NULL),
      log_runtime_en(0),
      log_history_en(0)
{
    INIT_LIST_HEAD(&mListGroup);
    INIT_LIST_HEAD(&mListOrphan);
    INIT_LIST_HEAD(&list_logs);

    mpp_env_get_u32("mpp_buffer_debug", &mpp_buffer_debug, 0);
    log_runtime_en   = (mpp_buffer_debug & MPP_BUF_DBG_OPS_RUNTIME) ? (1) : (0);
    log_history_en   = (mpp_buffer_debug & MPP_BUF_DBG_OPS_HISTORY) ? (1) : (0);

    // NOTE: here can not call mpp_buffer_group_init for the service is not started
    mLegacyGroup = get_group("legacy", "MppBufferService", MPP_BUFFER_INTERNAL, MPP_BUFFER_TYPE_ION);
}

MppBufferService::~MppBufferService()
{
    // first remove legacy group
    if (mLegacyGroup) {
        put_group(mLegacyGroup);
        mLegacyGroup = NULL;
    }

    // then remove the remaining group
    if (!list_empty(&mListGroup)) {
        MppBufferGroupImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &mListGroup, MppBufferGroupImpl, list_group) {
            put_group(pos);
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

RK_U32 MppBufferService::get_group_id()
{
    // avoid group_id reuse
    RK_U32 id = group_id++;
    while (get_group_by_id(group_id)){
        group_id++;
    }
    group_count++;
    return id;
}

MppBufferGroupImpl *MppBufferService::get_group(const char *tag, const char *caller,
                                                  MppBufferMode mode, MppBufferType type)
{
    MppBufferGroupImpl *p = mpp_calloc(MppBufferGroupImpl, 1);
    if (NULL == p) {
        mpp_err("MppBufferService failed to allocate group context\n");
        return NULL;
    }

    RK_U32 id = get_group_id();

    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);

    list_add_tail(&p->list_group, &mListGroup);

    if (tag) {
        snprintf(p->tag, sizeof(p->tag), "%s_%d", tag, id);
    } else {
        snprintf(p->tag, sizeof(p->tag), "unknown");
    }
    p->caller   = caller;
    p->mode     = mode;
    p->type     = type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->group_id = id;
    p->clear_on_exit = (mpp_buffer_debug & MPP_BUF_DBG_CLR_ON_EXIT) ? (1) : (0);

    mpp_allocator_get(&p->allocator, &p->alloc_api, type);

    return p;
}

MppBufferGroupImpl *MppBufferService::get_legacy_group()
{
    return mLegacyGroup;
}

void MppBufferService::put_group(MppBufferGroupImpl *p)
{
    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos);
            p->count_unused--;
        }
    }

    if (list_empty(&p->list_used)) {
        release_group(p);
    } else {
        mpp_err("mpp_group %p tag %s caller %s mode %s type %s deinit with %d bytes not released\n",
                p, p->tag, p->caller, mode2str[p->mode], type2str[p->type], p->usage);

        mpp_buffer_group_dump(p);

        /* if clear on exit we need to release remaining buffer */
        if (p->clear_on_exit) {
            MppBufferImpl *pos, *n;

            mpp_err("force release all remaining buffer\n");

            list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
                mpp_err("clearing buffer %p pos\n");
                pos->ref_count = 0;
                pos->used = 0;
                pos->discard = 0;
                deinit_buffer_no_lock(pos);
                p->count_used--;
            }

            release_group(p);
        } else {
            // otherwise move the group to list_orphan and wait for buffer release
            list_del_init(&p->list_group);
            list_add_tail(&p->list_group, &mListOrphan);
            p->is_orphan = 1;
        }
    }
}

void MppBufferService::release_group(MppBufferGroupImpl *group)
{
    mpp_assert(group->count_used == 0);
    mpp_assert(group->count_unused == 0);
    if (group->count_unused || group->count_used) {
        mpp_err("mpp_buffer_group_deinit mismatch counter used %4d unused %4d found\n",
            group->count_used, group->count_unused);
        group->count_unused = 0;
        group->count_used   = 0;
    }

    mpp_allocator_put(&group->allocator);
    list_del_init(&group->list_group);
    mpp_free(group);
    group_count--;

    if (group == mLegacyGroup) {
        mLegacyGroup = NULL;
    } else {
        /* if only legacy group left dump the legacy group */
        if (group_count == 1 && mLegacyGroup->count) {
            mpp_log("found legacy group has buffer remain, start dumping\n");
            mpp_buffer_group_dump(mLegacyGroup);
            abort();
        }
    }
}

MppBufferGroupImpl *MppBufferService::get_group_by_id(RK_U32 id)
{
    MppBufferGroupImpl *pos, *n;
    list_for_each_entry_safe(pos, n, &mListGroup, MppBufferGroupImpl, list_group) {
        if (pos->group_id == id) {
            return pos;
        }
    }

    list_for_each_entry_safe(pos, n, &mListOrphan, MppBufferGroupImpl, list_group) {
        if (pos->group_id == id) {
            return pos;
        }
    }

    return NULL;
}

void MppBufferService::add_log(MppBufferGroupImpl *group, MppBufferImpl *buffer, MppBufOps ops)
{
    if (log_runtime_en) {
        mpp_log("group %p buffer %p ops %s val ref_count %d\n", group, buffer, ops2str[ops], buffer->ref_count);
    }
    if (log_history_en) {
        MppBufLog *log = mpp_malloc(MppBufLog, 1);
        if (log) {
            INIT_LIST_HEAD(&log->list);
            log->buf = buffer;
            log->ops = ops;
            log->ref_count = buffer->ref_count;
        }
    }
}


