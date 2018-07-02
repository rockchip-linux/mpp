/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define BUFFER_OPS_MAX_COUNT            1024

#define SEARCH_GROUP_BY_ID(id)  ((MppBufferService::get_instance())->get_group_by_id(id))

typedef MPP_RET (*BufferOp)(MppAllocator allocator, MppBufferInfo *data);

typedef enum MppBufOps_e {
    GRP_CREATE,
    GRP_RELEASE,
    GRP_RESET,
    GRP_ORPHAN,
    GRP_DESTROY,

    GRP_OPS_BUTT    = GRP_DESTROY,
    BUF_COMMIT,
    BUF_CREATE,
    BUF_MMAP,
    BUF_REF_INC,
    BUF_REF_DEC,
    BUF_DISCARD,
    BUF_DESTROY,
    BUF_OPS_BUTT,
} MppBufOps;

typedef struct MppBufLog_t {
    struct list_head    list;
    RK_U32              group_id;
    RK_S32              buffer_id;
    MppBufOps           ops;
    RK_S32              ref_count;
    const char          *caller;
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
    void                destroy_group(MppBufferGroupImpl *group);

    RK_U32              get_group_id();
    RK_U32              group_id;
    RK_U32              group_count;
    RK_U32              finalizing;

    // misc group for internal / externl buffer with different type
    MppBufferGroupImpl  *misc[MPP_BUFFER_MODE_BUTT][MPP_BUFFER_TYPE_BUTT];
    RK_U32              misc_count;

    struct list_head    mListGroup;

    // list for used buffer which do not have group
    struct list_head    mListOrphan;

public:
    static MppBufferService *get_instance() {
        static MppBufferService instance;
        return &instance;
    }
    static Mutex *get_lock() {
        static Mutex lock;
        return &lock;
    }

    MppBufferGroupImpl  *get_group(const char *tag, const char *caller,
                                   MppBufferMode mode, MppBufferType type,
                                   RK_U32 is_misc);
    MppBufferGroupImpl  *get_misc(MppBufferMode mode, MppBufferType type);
    void                set_misc(MppBufferMode mode, MppBufferType type, MppBufferGroupImpl *val);
    void                put_group(MppBufferGroupImpl *group);
    MppBufferGroupImpl  *get_group_by_id(RK_U32 id);
    void                dump_misc_group();
    RK_U32              is_finalizing();
};

static const char *mode2str[MPP_BUFFER_MODE_BUTT] = {
    "internal",
    "external",
};

static const char *type2str[MPP_BUFFER_TYPE_BUTT] = {
    "normal",
    "ion",
    "dma-buf",
    "drm",
};
static const char *ops2str[BUF_OPS_BUTT] = {
    "grp create ",
    "grp release",
    "grp reset",
    "grp orphan",
    "grp destroy",

    "buf commit ",
    "buf create ",
    "buf mmap   ",
    "buf ref inc",
    "buf ref dec",
    "buf discard",
    "buf destroy",
};

RK_U32 mpp_buffer_debug = 0;

static void buffer_group_add_log(MppBufferGroupImpl *group,
                                 MppBufferImpl *buffer, MppBufOps ops, const char* caller)
{
    if (group->log_runtime_en) {
        if (buffer) {
            mpp_log("group %2d buffer %2d fd %2d ops %s ref_count %d caller %s\n",
                    group->group_id, buffer->buffer_id, buffer->info.fd,
                    ops2str[ops], buffer->ref_count, caller);
        } else {
            mpp_log("group %2d mode %d type %d ops %s\n", group->group_id,
                    group->mode, group->type, ops2str[ops]);
        }
    }
    if (group->log_history_en) {
        struct list_head *logs = &group->list_logs;
        MppBufLog *log = mpp_malloc(MppBufLog, 1);
        if (log) {
            INIT_LIST_HEAD(&log->list);
            log->group_id   = group->group_id;
            log->buffer_id  = (buffer) ? (buffer->buffer_id) : (-1);
            log->ops        = ops;
            log->ref_count  = (buffer) ? (buffer->ref_count) : (0);
            log->caller     = caller;

            if (group->log_count >= BUFFER_OPS_MAX_COUNT) {
                struct list_head *tmp = logs->next;
                list_del_init(tmp);
                mpp_free(list_entry(tmp, MppBufLog, list));
                group->log_count--;
            }
            list_add_tail(&log->list, logs);
            group->log_count++;
        }
    }
}

static void buffer_group_dump_log(MppBufferGroupImpl *group)
{
    if (group->log_history_en) {
        struct list_head *logs = &group->list_logs;

        while (!list_empty(logs)) {
            struct list_head *tmp = logs->next;
            MppBufLog *log = list_entry(tmp, MppBufLog, list);
            list_del_init(tmp);
            if (log->buffer_id >= 0) {
                mpp_log("group %2d buffer %2d ops %s ref_count %d caller %s\n",
                        group->group_id, log->buffer_id,
                        ops2str[log->ops], log->ref_count, log->caller);
            } else {
                mpp_log("group %3d ops %s\n", group->group_id,
                        ops2str[log->ops]);
            }
            mpp_free(log);
        }
    }
}

static MPP_RET deinit_buffer_no_lock(MppBufferImpl *buffer, const char *caller)
{
    if (!MppBufferService::get_instance()->is_finalizing()) {
        mpp_assert(buffer->ref_count == 0);
        mpp_assert(buffer->used == 0);
    }

    list_del_init(&buffer->list_status);
    MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);
    if (group) {
        BufferOp func = (group->mode == MPP_BUFFER_INTERNAL) ?
                        (group->alloc_api->free) :
                        (group->alloc_api->release);
        func(group->allocator, &buffer->info);
        group->usage -= buffer->info.size;
        group->buffer_count--;

        buffer_group_add_log(group, buffer, BUF_DESTROY, caller);

        if (group->is_orphan && !group->usage) {
            MppBufferService::get_instance()->put_group(group);
        }
    } else {
        mpp_assert(MppBufferService::get_instance()->is_finalizing());
    }

    mpp_free(buffer);

    return MPP_OK;
}

static MPP_RET inc_buffer_ref_no_lock(MppBufferImpl *buffer, const char *caller)
{
    MPP_RET ret = MPP_OK;
    MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);
    if (!buffer->used) {
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
    buffer_group_add_log(group, buffer, BUF_REF_INC, caller);
    buffer->ref_count++;
    return ret;
}

static void dump_buffer_info(MppBufferImpl *buffer)
{
    mpp_log("buffer %p fd %4d size %10d ref_count %3d discard %d caller %s\n",
            buffer, buffer->info.fd, buffer->info.size,
            buffer->ref_count, buffer->discard, buffer->caller);
}

MPP_RET mpp_buffer_create(const char *tag, const char *caller,
                          MppBufferGroupImpl *group, MppBufferInfo *info,
                          MppBufferImpl **buffer)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = MPP_OK;
    BufferOp func = NULL;
    MppBufferImpl *p = NULL;

    if (NULL == group) {
        mpp_err_f("can not create buffer without group\n");
        ret = MPP_NOK;
        goto RET;
    }

    if (group->limit_count && group->buffer_count >= group->limit_count) {
        if (group->log_runtime_en)
            mpp_log_f("group %d reach count limit %d\n", group->group_id, group->limit_count);
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
    p->group_id = group->group_id;
    p->buffer_id = group->buffer_id;
    INIT_LIST_HEAD(&p->list_status);
    list_add_tail(&p->list_status, &group->list_unused);

    group->buffer_id++;
    group->usage += info->size;
    group->buffer_count++;
    group->count_unused++;

    buffer_group_add_log(group, p,
                         (group->mode == MPP_BUFFER_INTERNAL) ? (BUF_CREATE) : (BUF_COMMIT),
                         caller);

    if (buffer) {
        inc_buffer_ref_no_lock(p, caller);
        *buffer = p;
    }

    if (group->callback)
        group->callback(group->arg, group);
RET:
    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MPP_RET mpp_buffer_mmap(MppBufferImpl *buffer, const char* caller)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = MPP_NOK;
    MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);

    if (group && group->alloc_api && group->alloc_api->mmap) {
        ret = group->alloc_api->mmap(group->allocator, &buffer->info);

        buffer_group_add_log(group, buffer, BUF_MMAP, caller);
    }

    if (ret)
        mpp_err_f("buffer %p group %p fd %d map failed caller %s\n",
                  buffer, group, buffer->info.fd, caller);

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer, const char* caller)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = inc_buffer_ref_no_lock(buffer, caller);

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}


MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer, const char* caller)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = MPP_OK;
    MppBufferGroupImpl *group = SEARCH_GROUP_BY_ID(buffer->group_id);
    if (group)
        buffer_group_add_log(group, buffer, BUF_REF_DEC, caller);

    if (buffer->ref_count <= 0) {
        mpp_err_f("found non-positive ref_count %d caller %s\n",
                  buffer->ref_count, buffer->caller);
        mpp_abort();
        ret = MPP_NOK;
    } else {
        buffer->ref_count--;
        if (0 == buffer->ref_count) {
            buffer->used = 0;
            list_del_init(&buffer->list_status);
            if (group == MppBufferService::get_instance()->get_misc(group->mode, group->type)) {
                deinit_buffer_no_lock(buffer, caller);
            } else {
                if (buffer->discard) {
                    deinit_buffer_no_lock(buffer, caller);
                } else {
                    list_add_tail(&buffer->list_status, &group->list_unused);
                    group->count_unused++;
                }
            }
            group->count_used--;
            if (group->callback)
                group->callback(group->arg, group);
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
        RK_S32 found = 0;
        RK_S32 search_count = 0;

        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            mpp_buf_dbg(MPP_BUF_DBG_CHECK_SIZE, "request size %d on buf idx %d size %d\n",
                        size, pos->buffer_id, pos->info.size);
            if (pos->info.size >= size) {
                buffer = pos;
                inc_buffer_ref_no_lock(buffer, __FUNCTION__);
                found = 1;
                break;
            } else {
                if (MPP_BUFFER_INTERNAL == p->mode) {
                    deinit_buffer_no_lock(pos, __FUNCTION__);
                    p->count_unused--;
                } else
                    search_count++;
            }
        }

        if (!found && search_count)
            mpp_err_f("can not found match buffer with size larger than %d\n", size);
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

    *group = MppBufferService::get_instance()->get_group(tag, caller, mode, type, 0);

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

    buffer_group_add_log(p, NULL, GRP_RESET, NULL);

    if (!list_empty(&p->list_used)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            buffer_group_add_log(p, pos, BUF_DISCARD, NULL);
            pos->discard = 1;
        }
    }

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos, __FUNCTION__);
            p->count_unused--;
        }
    }

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

MPP_RET mpp_buffer_group_set_callback(MppBufferGroupImpl *p,
                                      MppBufCallback callback, void *arg)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    if (NULL == p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUF_FUNCTION_ENTER();

    p->callback = callback;
    p->arg      = arg;

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

void mpp_buffer_group_dump(MppBufferGroupImpl *group, const char *caller)
{
    mpp_log("\ndumping buffer group %p id %d from %s\n", group,
            group->group_id, caller);
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

    buffer_group_dump_log(group);
}

void mpp_buffer_service_dump()
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MppBufferService::get_instance()->dump_misc_group();
}

MppBufferGroupImpl *mpp_buffer_get_misc_group(MppBufferMode mode, MppBufferType type)
{
    AutoMutex auto_lock(MppBufferService::get_lock());
    MppBufferGroupImpl *misc = MppBufferService::get_instance()->get_misc(mode, type);
    if (NULL == misc) {
        char tag[16];
        RK_S32 offset = 0;

        offset += snprintf(tag + offset, sizeof(tag) - offset, "misc");
        offset += snprintf(tag + offset, sizeof(tag) - offset, "_%s",
                           type == MPP_BUFFER_TYPE_ION ? "ion" :
                           type == MPP_BUFFER_TYPE_DRM ? "drm" : "na");
        offset += snprintf(tag + offset, sizeof(tag) - offset, "_%s",
                           mode == MPP_BUFFER_INTERNAL ? "int" : "ext");

        misc = MppBufferService::get_instance()->get_group(tag, __FUNCTION__, mode, type, 1);
    }
    return misc;
}

MppBufferService::MppBufferService()
    : group_id(0),
      group_count(0),
      finalizing(0),
      misc_count(0)
{
    RK_S32 i, j;

    INIT_LIST_HEAD(&mListGroup);
    INIT_LIST_HEAD(&mListOrphan);

    // NOTE: Do not create misc group at beginning. Only create on when needed.
    for (i = 0; i < MPP_BUFFER_MODE_BUTT; i++)
        for (j = 0; j < MPP_BUFFER_TYPE_BUTT; j++)
            misc[i][j] = NULL;
}

MppBufferService::~MppBufferService()
{
    RK_S32 i, j;

    finalizing = 1;

    // first remove legacy group which is the normal case
    if (misc_count) {
        mpp_log_f("cleaning misc group\n");
        for (i = 0; i < MPP_BUFFER_MODE_BUTT; i++)
            for (j = 0; j < MPP_BUFFER_TYPE_BUTT; j++) {
                MppBufferGroupImpl *pos = misc[i][j];
                if (pos) {
                    put_group(pos);
                    misc[i][j] = NULL;
                }
            }
    }

    // then remove the remaining group which is the leak one
    if (!list_empty(&mListGroup)) {
        MppBufferGroupImpl *pos, *n;

        mpp_log_f("cleaning leaked group\n");
        list_for_each_entry_safe(pos, n, &mListGroup, MppBufferGroupImpl, list_group) {
            put_group(pos);
        }
    }

    // remove all orphan buffer
    if (!list_empty(&mListOrphan)) {
        MppBufferImpl *pos, *n;

        mpp_log_f("cleaning leaked buffer\n");
        list_for_each_entry_safe(pos, n, &mListOrphan, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos, __FUNCTION__);
        }
    }
}

RK_U32 MppBufferService::get_group_id()
{
    // avoid group_id reuse
    RK_U32 id = group_id++;
    while (get_group_by_id(group_id)) {
        group_id++;
    }
    group_count++;
    return id;
}

MppBufferGroupImpl *MppBufferService::get_group(const char *tag, const char *caller,
                                                MppBufferMode mode, MppBufferType type,
                                                RK_U32 is_misc)
{
    MppBufferType buffer_type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    MppBufferGroupImpl *p = mpp_calloc(MppBufferGroupImpl, 1);
    if (NULL == p) {
        mpp_err("MppBufferService failed to allocate group context\n");
        return NULL;
    }

    RK_U32 id = get_group_id();

    INIT_LIST_HEAD(&p->list_logs);
    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);

    mpp_env_get_u32("mpp_buffer_debug", &mpp_buffer_debug, 0);
    p->log_runtime_en   = (mpp_buffer_debug & MPP_BUF_DBG_OPS_RUNTIME) ? (1) : (0);
    p->log_history_en   = (mpp_buffer_debug & MPP_BUF_DBG_OPS_HISTORY) ? (1) : (0);

    list_add_tail(&p->list_group, &mListGroup);

    if (tag) {
        snprintf(p->tag, sizeof(p->tag), "%s_%d", tag, id);
    } else {
        snprintf(p->tag, sizeof(p->tag), "unknown");
    }
    p->caller   = caller;
    p->mode     = mode;
    p->type     = buffer_type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->group_id = id;
    p->clear_on_exit = (mpp_buffer_debug & MPP_BUF_DBG_CLR_ON_EXIT) ? (1) : (0);

    mpp_allocator_get(&p->allocator, &p->alloc_api, type);

    buffer_group_add_log(p, NULL, GRP_CREATE, __FUNCTION__);

    mpp_assert(mode < MPP_BUFFER_MODE_BUTT);
    mpp_assert(buffer_type < MPP_BUFFER_TYPE_BUTT);

    if (is_misc) {
        misc[mode][buffer_type] = p;
        misc_count++;
    }

    return p;
}

MppBufferGroupImpl *MppBufferService::get_misc(MppBufferMode mode, MppBufferType type)
{
    type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    if (type == MPP_BUFFER_TYPE_NORMAL)
        return NULL;

    mpp_assert(mode < MPP_BUFFER_MODE_BUTT);
    mpp_assert(type < MPP_BUFFER_TYPE_BUTT);

    return misc[mode][type];
}

void MppBufferService::set_misc(MppBufferMode mode, MppBufferType type, MppBufferGroupImpl *val)
{
    type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    if (type == MPP_BUFFER_TYPE_NORMAL)
        return ;

    mpp_assert(mode < MPP_BUFFER_MODE_BUTT);
    mpp_assert(type < MPP_BUFFER_TYPE_BUTT);

    misc[mode][type] = val;
}

void MppBufferService::put_group(MppBufferGroupImpl *p)
{
    buffer_group_add_log(p, NULL, GRP_RELEASE, __FUNCTION__);

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            deinit_buffer_no_lock(pos, __FUNCTION__);
            p->count_unused--;
        }
    }

    if (list_empty(&p->list_used)) {
        destroy_group(p);
    } else {
        if (!finalizing ||
            (finalizing && (mpp_buffer_debug & MPP_BUF_DBG_DUMP_ON_EXIT))) {
            mpp_err("mpp_group %p tag %s caller %s mode %s type %s deinit with %d bytes not released\n",
                    p, p->tag, p->caller, mode2str[p->mode], type2str[p->type], p->usage);

            mpp_buffer_group_dump(p, __FUNCTION__);
        }

        /* if clear on exit we need to release remaining buffer */
        if (p->clear_on_exit) {
            MppBufferImpl *pos, *n;

            mpp_err("force release all remaining buffer\n");

            list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
                mpp_err("clearing buffer %p\n", pos);
                pos->ref_count = 0;
                pos->used = 0;
                pos->discard = 0;
                deinit_buffer_no_lock(pos, __FUNCTION__);
                p->count_used--;
            }

            destroy_group(p);
        } else {
            // otherwise move the group to list_orphan and wait for buffer release
            buffer_group_add_log(p, NULL, GRP_ORPHAN, __FUNCTION__);
            list_del_init(&p->list_group);
            list_add_tail(&p->list_group, &mListOrphan);
            p->is_orphan = 1;
        }
    }
}

void MppBufferService::destroy_group(MppBufferGroupImpl *group)
{
    MppBufferMode mode = group->mode;
    MppBufferType type = group->type;

    mpp_assert(group->count_used == 0);
    mpp_assert(group->count_unused == 0);
    if (group->count_unused || group->count_used) {
        mpp_err("mpp_buffer_group_deinit mismatch counter used %4d unused %4d found\n",
                group->count_used, group->count_unused);
        group->count_unused = 0;
        group->count_used   = 0;
    }

    buffer_group_add_log(group, NULL, GRP_DESTROY, __FUNCTION__);

    if (group->log_history_en) {
        struct list_head *logs = &group->list_logs;
        while (!list_empty(logs)) {
            struct list_head *tmp = logs->next;
            list_del_init(tmp);
            mpp_free(list_entry(tmp, MppBufLog, list));
            group->log_count--;
        }
        mpp_assert(group->log_count == 0);
    }

    mpp_assert(group->allocator);
    mpp_allocator_put(&group->allocator);
    list_del_init(&group->list_group);
    mpp_free(group);
    group_count--;

    if (group == misc[mode][type]) {
        misc[mode][type] = NULL;
        misc_count--;
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

void MppBufferService::dump_misc_group()
{
    RK_S32 i, j;

    for (i = 0; i < MPP_BUFFER_MODE_BUTT; i++)
        for (j = 0; j < MPP_BUFFER_TYPE_BUTT; j++) {
            if (misc[i][j])
                mpp_buffer_group_dump(misc[i][j], __FUNCTION__);
        }
}

RK_U32 MppBufferService::is_finalizing()
{
    return finalizing;
}

