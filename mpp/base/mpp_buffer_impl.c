/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_buffer"

#include <string.h>

#include "mpp_env.h"
#include "mpp_hash.h"
#include "mpp_lock.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_thread.h"
#include "mpp_mem_pool.h"
#include "mpp_singleton.h"

#include "mpp_buffer_impl.h"

#define MAX_GROUP_BIT                   8
#define MAX_MISC_GROUP_BIT              3
#define BUFFER_OPS_MAX_COUNT            1024
#define MPP_ALLOCATOR_WITH_FLAG_NUM     8

/* NOTE: user may call buffer / buf_grp deinit after buffer service deinited */
typedef enum MppBufSrvStatus_e {
    MPP_BUF_SRV_UNINITED    = -1,
    MPP_BUF_SRV_NORMAL      = 0,
    MPP_BUF_SRV_FINALIZED   = 1,
    MPP_BUF_SRV_BUTT,
} MppBufSrvStatus;

#define SEARCH_GROUP_BY_ID(srv, id)     (get_group_by_id(srv, id))

#define get_srv_buffer() \
    ({ \
        MppBufferService *__tmp; \
        if (srv_buffer) \
            __tmp = srv_buffer; \
        else { \
            switch (srv_status) { \
            case MPP_BUF_SRV_UNINITED : { \
                mpp_buffer_service_init(); \
                __tmp = srv_buffer; \
            } break; \
            case MPP_BUF_SRV_FINALIZED : { \
                /* if called after buf srv deinited return NULL without error log */ \
                __tmp = NULL; \
            } break; \
            default : { \
                mpp_err("mpp buffer srv not init status %d at %s\n", __FUNCTION__); \
                __tmp = NULL; \
            } break; \
            } \
        } \
        __tmp; \
    })

static void mpp_buffer_service_init();

typedef MPP_RET (*BufferOp)(MppAllocator allocator, MppBufferInfo *data);

typedef struct MppBufferService_t {
    rk_u32              group_id;
    rk_u32              group_count;
    rk_u32              finalizing;

    rk_u32              total_size;
    rk_u32              total_max;

    MppMutex            lock;
    // misc group for internal / externl buffer with different type
    rk_u32              misc[MPP_BUFFER_MODE_BUTT][MPP_BUFFER_TYPE_BUTT][MPP_ALLOCATOR_WITH_FLAG_NUM];
    rk_u32              misc_count;
    /* preset allocator apis */
    MppAllocator        allocator[MPP_BUFFER_TYPE_BUTT][MPP_ALLOCATOR_WITH_FLAG_NUM];
    MppAllocatorApi     *allocator_api[MPP_BUFFER_TYPE_BUTT];

    struct list_head    list_group;
    DECLARE_HASHTABLE(hash_group, MAX_GROUP_BIT);

    // list for used buffer which do not have group
    struct list_head    list_orphan;
} MppBufferService;

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

static MppMemPool pool_buf = NULL;
static MppMemPool pool_buf_grp = NULL;
static MppMemPool pool_buf_map_node = NULL;
static MppBufferService *srv_buffer = NULL;
static MppBufSrvStatus srv_status = MPP_BUF_SRV_UNINITED;
rk_u32 mpp_buffer_debug = 0;

static MppBufferGroupImpl *service_get_group(const char *tag, const char *caller,
                                             MppBufferMode mode, MppBufferType type,
                                             rk_u32 is_misc);

static void service_put_group(MppBufferService *srv, MppBufferGroupImpl *p, const char *caller);
static void service_dump(MppBufferService *srv, const char *info);

static MppBufferGroupImpl *get_group_by_id(MppBufferService *srv, rk_u32 id)
{
    MppBufferGroupImpl *impl = NULL;

    hash_for_each_possible(srv->hash_group, impl, hlist, id) {
        if (impl->group_id == id)
            break;
    }

    return impl;
}

static MppBufLogs *buf_logs_init(rk_u32 max_count)
{
    MppBufLogs *logs = NULL;

    if (!max_count)
        return NULL;

    logs = mpp_malloc_size(MppBufLogs, sizeof(MppBufLogs) + max_count * sizeof(MppBufLog));
    if (!logs) {
        mpp_err_f("failed to create %d buf logs\n", max_count);
        return NULL;
    }

    {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&logs->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    logs->max_count = max_count;
    logs->log_count = 0;
    logs->log_write = 0;
    logs->log_read = 0;
    logs->logs = (MppBufLog *)(logs + 1);

    return logs;
}

static void buf_logs_deinit(MppBufLogs *logs)
{
    pthread_mutex_destroy(&logs->lock);
    MPP_FREE(logs);
}

static void buf_logs_write(MppBufLogs *logs, rk_u32 group_id, rk_s32 buffer_id,
                           MppBufOps ops, rk_s32 ref_count, const char *caller)
{
    MppBufLog *log = NULL;

    pthread_mutex_lock(&logs->lock);

    log = &logs->logs[logs->log_write];
    log->group_id   = group_id;
    log->buffer_id  = buffer_id;
    log->ops        = ops;
    log->ref_count  = ref_count;
    log->caller     = caller;

    logs->log_write++;
    if (logs->log_write >= logs->max_count)
        logs->log_write = 0;

    if (logs->log_count < logs->max_count)
        logs->log_count++;
    else {
        logs->log_read++;
        if (logs->log_read >= logs->max_count)
            logs->log_read = 0;
    }

    pthread_mutex_unlock(&logs->lock);
}

static void buf_logs_dump(MppBufLogs *logs)
{
    while (logs->log_count) {
        MppBufLog *log = &logs->logs[logs->log_read];

        if (log->buffer_id >= 0)
            mpp_log("group %3d buffer %4d ops %s ref_count %d caller %s\n",
                    log->group_id, log->buffer_id,
                    ops2str[log->ops], log->ref_count, log->caller);
        else
            mpp_log("group %3d ops %s\n", log->group_id, ops2str[log->ops]);

        logs->log_read++;
        if (logs->log_read >= logs->max_count)
            logs->log_read = 0;
        logs->log_count--;
    }
    mpp_assert(logs->log_read == logs->log_write);
}

static void buf_add_log(MppBufferImpl *buffer, MppBufOps ops, const char* caller)
{
    if (buffer->log_runtime_en) {
        mpp_log("group %3d buffer %4d fd %3d ops %s ref_count %d caller %s\n",
                buffer->group_id, buffer->buffer_id, buffer->info.fd,
                ops2str[ops], buffer->ref_count, caller);
    }
    if (buffer->logs)
        buf_logs_write(buffer->logs, buffer->group_id, buffer->buffer_id,
                       ops, buffer->ref_count, caller);
}

static void buf_grp_add_log(MppBufferGroupImpl *group, MppBufOps ops, const char* caller)
{
    if (group->log_runtime_en) {
        mpp_log("group %3d mode %d type %d ops %s\n", group->group_id,
                group->mode, group->type, ops2str[ops]);
    }
    if (group->logs)
        buf_logs_write(group->logs, group->group_id, -1, ops, 0, caller);
}

static void dump_buffer_info(MppBufferImpl *buffer)
{
    mpp_log("buffer %p fd %4d size %10d ref_count %3d discard %d caller %s\n",
            buffer, buffer->info.fd, buffer->info.size,
            buffer->ref_count, buffer->discard, buffer->caller);
}

void mpp_buffer_group_dump(MppBufferGroupImpl *group, const char *caller)
{
    MppBufferImpl *pos, *n;

    mpp_log("\ndumping buffer group %p id %d from %s\n", group,
            group->group_id, caller);
    mpp_log("mode %s\n", mode2str[group->mode]);
    mpp_log("type %s\n", type2str[group->type]);
    mpp_log("limit size %d count %d\n", group->limit_size, group->limit_count);

    mpp_log("used buffer count %d\n", group->count_used);

    list_for_each_entry_safe(pos, n, &group->list_used, MppBufferImpl, list_status) {
        dump_buffer_info(pos);
    }

    mpp_log("unused buffer count %d\n", group->count_unused);
    list_for_each_entry_safe(pos, n, &group->list_unused, MppBufferImpl, list_status) {
        dump_buffer_info(pos);
    }

    if (group->logs)
        buf_logs_dump(group->logs);
}

static void clear_buffer_info(MppBufferInfo *info)
{
    info->fd = -1;
    info->ptr = NULL;
    info->hnd = NULL;
    info->size = 0;
    info->index = -1;
    info->type = MPP_BUFFER_TYPE_BUTT;
}

static void service_put_buffer(MppBufferService *srv, MppBufferGroupImpl *group,
                               MppBufferImpl *buffer, rk_u32 reuse, const char *caller)
{
    struct list_head list_maps;
    MppDevBufMapNode *pos, *n;
    MppBufferInfo info;

    mpp_assert(group);

    pthread_mutex_lock(&buffer->lock);

    if (!srv && !srv->finalizing) {
        mpp_assert(buffer->ref_count == 0);
        if (buffer->ref_count > 0) {
            pthread_mutex_unlock(&buffer->lock);
            return;
        }
    }

    list_del_init(&buffer->list_status);

    if (reuse) {
        if (buffer->used && group) {
            group->count_used--;
            list_add_tail(&buffer->list_status, &group->list_unused);
            group->count_unused++;
        } else {
            mpp_err_f("can not reuse unused buffer %d at group %p:%d\n",
                      buffer->buffer_id, group, buffer->group_id);
        }
        buffer->used = 0;

        pthread_mutex_unlock(&buffer->lock);
        return;
    }

    /* remove all map from buffer */
    INIT_LIST_HEAD(&list_maps);
    list_for_each_entry_safe(pos, n, &buffer->list_maps, MppDevBufMapNode, list_buf) {
        list_move_tail(&pos->list_buf, &list_maps);
        pos->iova = (rk_u32)(-1);
    }
    mpp_assert(list_empty(&buffer->list_maps));
    info = buffer->info;
    if (group) {
        rk_u32 destroy = 0;
        rk_u32 size = buffer->info.size;

        if (buffer->used)
            group->count_used--;
        else
            group->count_unused--;

        group->usage -= size;
        group->buffer_count--;

        /* reduce total buffer size record */
        if (group->mode == MPP_BUFFER_INTERNAL && srv)
            MPP_FETCH_SUB(&srv->total_size, size);

        buf_add_log(buffer, BUF_DESTROY, caller);

        if (group->is_orphan && !group->usage && !group->is_finalizing)
            destroy = 1;

        if (destroy)
            service_put_group(srv, group, caller);
    } else {
        mpp_assert(srv_status);
    }
    clear_buffer_info(&buffer->info);

    pthread_mutex_unlock(&buffer->lock);

    list_for_each_entry_safe(pos, n, &list_maps, MppDevBufMapNode, list_buf) {
        MppDev dev = pos->dev;

        mpp_assert(dev);
        mpp_dev_ioctl(dev, MPP_DEV_LOCK_MAP, NULL);
        /* remove buffer from group */
        mpp_dev_ioctl(dev, MPP_DEV_DETACH_FD, pos);
        mpp_dev_ioctl(dev, MPP_DEV_UNLOCK_MAP, NULL);
        mpp_mem_pool_put(pool_buf_map_node, pos, caller);
    }

    /* release buffer here */
    {
        BufferOp func = (buffer->mode == MPP_BUFFER_INTERNAL) ?
                        (buffer->alloc_api->free) :
                        (buffer->alloc_api->release);

        func(buffer->allocator, &info);
    }

    mpp_mem_pool_put(pool_buf, buffer, caller);
}

static MPP_RET inc_buffer_ref(MppBufferImpl *buffer, const char *caller)
{
    MPP_RET ret = MPP_OK;

    pthread_mutex_lock(&buffer->lock);
    buffer->ref_count++;
    buf_add_log(buffer, BUF_REF_INC, caller);
    if (!buffer->used) {
        MppBufferGroupImpl *group = NULL;
        MppBufferService *srv = get_srv_buffer();

        if (srv) {
            mpp_mutex_lock(&srv->lock);
            group = SEARCH_GROUP_BY_ID(srv, buffer->group_id);
            mpp_mutex_unlock(&srv->lock);
        }
        // NOTE: when increasing ref_count the unused buffer must be under certain group
        mpp_assert(group);
        buffer->used = 1;
        if (group) {
            pthread_mutex_lock(&group->buf_lock);
            list_del_init(&buffer->list_status);
            list_add_tail(&buffer->list_status, &group->list_used);
            group->count_used++;
            group->count_unused--;
            pthread_mutex_unlock(&group->buf_lock);
        } else {
            mpp_err_f("unused buffer without group\n");
            ret = MPP_NOK;
        }
    }
    pthread_mutex_unlock(&buffer->lock);
    return ret;
}

MPP_RET mpp_buffer_create(const char *tag, const char *caller,
                          MppBufferGroupImpl *group, MppBufferInfo *info,
                          MppBufferImpl **buffer)
{
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = MPP_OK;
    BufferOp func = NULL;
    MppBufferImpl *p = NULL;

    if (!group) {
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

    p = (MppBufferImpl *)mpp_mem_pool_get(pool_buf, caller);
    if (!p) {
        mpp_err_f("failed to allocate context\n");
        ret = MPP_ERR_MALLOC;
        goto RET;
    }

    func = (group->mode == MPP_BUFFER_INTERNAL) ?
           (group->alloc_api->alloc) : (group->alloc_api->import);
    ret = func(group->allocator, info);
    if (ret) {
        mpp_err_f("failed to create buffer with size %d\n", info->size);
        mpp_mem_pool_put(pool_buf, p, caller);
        ret = MPP_ERR_MALLOC;
        goto RET;
    }

    if (!tag)
        tag = group->tag;

    snprintf(p->tag, sizeof(p->tag), "%s", tag);
    p->caller = caller;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&p->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    p->allocator = group->allocator;
    p->alloc_api = group->alloc_api;
    p->log_runtime_en = group->log_runtime_en;
    p->log_history_en = group->log_history_en;
    p->group_id = group->group_id;
    p->mode = group->mode;
    p->type = group->type;
    p->uncached = (group->flags & MPP_ALLOC_FLAG_CACHABLE) ? 0 : 1;
    p->logs = group->logs;
    p->info = *info;

    pthread_mutex_lock(&group->buf_lock);
    p->buffer_id = group->buffer_id++;
    INIT_LIST_HEAD(&p->list_status);
    INIT_LIST_HEAD(&p->list_maps);

    if (buffer) {
        p->ref_count++;
        p->used = 1;
        list_add_tail(&p->list_status, &group->list_used);
        group->count_used++;
        *buffer = p;
    } else {
        list_add_tail(&p->list_status, &group->list_unused);
        group->count_unused++;
    }

    group->usage += info->size;
    group->buffer_count++;
    pthread_mutex_unlock(&group->buf_lock);

    buf_add_log(p, (group->mode == MPP_BUFFER_INTERNAL) ? (BUF_CREATE) : (BUF_COMMIT), caller);

    if (group->mode == MPP_BUFFER_INTERNAL) {
        MppBufferService *srv = get_srv_buffer();

        if (srv) {
            rk_u32 total = MPP_ADD_FETCH(&srv->total_size, info->size);
            bool cas_ret;

            do {
                rk_u32 old_max = srv->total_max;
                rk_u32 new_max = MPP_MAX(total, old_max);

                cas_ret = MPP_BOOL_CAS(&srv->total_max, old_max, new_max);
            } while (!cas_ret);
        }
    }

    if (group->callback)
        group->callback(group->arg, group);
RET:
    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MPP_RET mpp_buffer_mmap(MppBufferImpl *buffer, const char* caller)
{
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = buffer->alloc_api->mmap(buffer->allocator, &buffer->info);
    if (ret)
        mpp_err_f("buffer %d group %d fd %d map failed caller %s\n",
                  buffer->buffer_id, buffer->group_id, buffer->info.fd, caller);

    buf_add_log(buffer, BUF_MMAP, caller);

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer, const char* caller)
{
    MPP_BUF_FUNCTION_ENTER();

    MPP_RET ret = inc_buffer_ref(buffer, caller);

    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}


MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer, const char* caller)
{
    MPP_RET ret = MPP_OK;
    rk_u32 release = 0;

    MPP_BUF_FUNCTION_ENTER();

    pthread_mutex_lock(&buffer->lock);

    if (buffer->ref_count <= 0) {
        buf_add_log(buffer, BUF_REF_DEC, caller);
        mpp_err_f("buffer from %s found non-positive ref_count %d caller %s\n",
                  buffer->caller, buffer->ref_count, caller);
        mpp_abort();
        ret = MPP_NOK;
        pthread_mutex_unlock(&buffer->lock);
        goto done;
    }

    buffer->ref_count--;
    if (buffer->ref_count == 0)
        release = 1;
    buf_add_log(buffer, BUF_REF_DEC, caller);

    pthread_mutex_unlock(&buffer->lock);

    if (release) {
        MppBufferGroupImpl *group = NULL;
        MppBufferService *srv = get_srv_buffer();

        if (srv) {
            mpp_mutex_lock(&srv->lock);
            group = SEARCH_GROUP_BY_ID(srv, buffer->group_id);
            mpp_mutex_unlock(&srv->lock);
        }

        mpp_assert(group);
        if (group) {
            rk_u32 reuse = 0;

            pthread_mutex_lock(&group->buf_lock);

            reuse = (!group->is_misc && !buffer->discard);
            service_put_buffer(srv, group, buffer, reuse, caller);

            if (group->callback)
                group->callback(group->arg, group);

            pthread_mutex_unlock(&group->buf_lock);
        }
    }

done:
    MPP_BUF_FUNCTION_LEAVE();
    return ret;
}

MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size, const char* caller)
{
    MppBufferImpl *buffer = NULL;

    MPP_BUF_FUNCTION_ENTER();

    pthread_mutex_lock(&p->buf_lock);

    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;
        rk_s32 found = 0;
        rk_s32 search_count = 0;

        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            mpp_buf_dbg(MPP_BUF_DBG_CHECK_SIZE, "request size %d on buf idx %d size %d\n",
                        size, pos->buffer_id, pos->info.size);
            if (pos->info.size >= size) {
                buffer = pos;
                pthread_mutex_lock(&buffer->lock);
                buffer->ref_count++;
                buffer->used = 1;
                buf_add_log(buffer, BUF_REF_INC, caller);
                list_del_init(&buffer->list_status);
                list_add_tail(&buffer->list_status, &p->list_used);
                p->count_used++;
                p->count_unused--;
                pthread_mutex_unlock(&buffer->lock);
                found = 1;
                break;
            } else {
                if (MPP_BUFFER_INTERNAL == p->mode) {
                    service_put_buffer(get_srv_buffer(), p, pos, 0, caller);
                } else
                    search_count++;
            }
        }

        if (!found && search_count) {
            mpp_err_f("can not found match buffer with size larger than %d\n", size);
            mpp_buffer_group_dump(p, caller);
        }
    }

    pthread_mutex_unlock(&p->buf_lock);

    MPP_BUF_FUNCTION_LEAVE();
    return buffer;
}

rk_u32 mpp_buffer_to_addr(MppBuffer buffer, size_t offset)
{
    MppBufferImpl *impl = (MppBufferImpl *)buffer;
    rk_u32 addr = 0;

    if (!impl) {
        mpp_err_f("NULL buffer convert to zero address\n");
        return 0;
    }

    if (impl->info.fd >= (1 << 10)) {
        mpp_err_f("buffer fd %d is too large\n");
        return 0;
    }

    if (impl->offset + offset >= SZ_4M) {
        mpp_err_f("offset %d + %d is larger than 4M use extra info to send offset\n");
        return 0;
    }

    addr = impl->info.fd + ((impl->offset + offset) << 10);

    return addr;
}

static MppDevBufMapNode *mpp_buffer_attach_dev_lock(const char *caller, MppBuffer buffer, MppDev dev)
{
    MppBufferImpl *impl = (MppBufferImpl *)buffer;
    MppDevBufMapNode *pos, *n;
    MppDevBufMapNode *node = NULL;
    MPP_RET ret = MPP_OK;

    mpp_dev_ioctl(dev, MPP_DEV_LOCK_MAP, NULL);

    pthread_mutex_lock(&impl->lock);

    list_for_each_entry_safe(pos, n, &impl->list_maps, MppDevBufMapNode, list_buf) {
        if (pos->dev == dev) {
            node = pos;
            goto DONE;
        }
    }

    node = (MppDevBufMapNode *)mpp_mem_pool_get(pool_buf_map_node, caller);
    if (!node) {
        mpp_err("mpp_buffer_attach_dev failed to allocate map node\n");
        ret = MPP_NOK;
        goto DONE;
    }

    INIT_LIST_HEAD(&node->list_buf);
    INIT_LIST_HEAD(&node->list_dev);
    node->lock_buf = &impl->lock;
    node->buffer = impl;
    node->dev = dev;
    node->pool = pool_buf_map_node;
    node->buf_fd = impl->info.fd;

    ret = mpp_dev_ioctl(dev, MPP_DEV_ATTACH_FD, node);
    if (ret) {
        mpp_mem_pool_put(pool_buf_map_node, node, caller);
        node = NULL;
        goto DONE;
    }
    list_add_tail(&node->list_buf, &impl->list_maps);

DONE:
    pthread_mutex_unlock(&impl->lock);
    mpp_dev_ioctl(dev, MPP_DEV_UNLOCK_MAP, NULL);

    return node;
}

MPP_RET mpp_buffer_attach_dev_f(const char *caller, MppBuffer buffer, MppDev dev)
{
    MppDevBufMapNode *node;

    node = mpp_buffer_attach_dev_lock(caller, buffer, dev);

    return node ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_buffer_detach_dev_f(const char *caller, MppBuffer buffer, MppDev dev)
{
    MppBufferImpl *impl = (MppBufferImpl *)buffer;
    MppDevBufMapNode *pos, *n;
    MPP_RET ret = MPP_OK;

    mpp_dev_ioctl(dev, MPP_DEV_LOCK_MAP, NULL);
    pthread_mutex_lock(&impl->lock);
    list_for_each_entry_safe(pos, n, &impl->list_maps, MppDevBufMapNode, list_buf) {
        if (pos->dev == dev) {
            list_del_init(&pos->list_buf);
            ret = mpp_dev_ioctl(dev, MPP_DEV_DETACH_FD, pos);
            mpp_mem_pool_put(pool_buf_map_node, pos, caller);
            break;
        }
    }
    pthread_mutex_unlock(&impl->lock);
    mpp_dev_ioctl(dev, MPP_DEV_UNLOCK_MAP, NULL);

    return ret;
}

rk_u32 mpp_buffer_get_iova_f(const char *caller, MppBuffer buffer, MppDev dev)
{
    MppDevBufMapNode *node;

    node = mpp_buffer_attach_dev_lock(caller, buffer, dev);

    return node ? node->iova : (rk_u32)(-1);
}

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, const char *caller,
                              MppBufferMode mode, MppBufferType type)
{
    MPP_BUF_FUNCTION_ENTER();
    mpp_assert(caller);

    *group = service_get_group(tag, caller, mode, type, 0);

    MPP_BUF_FUNCTION_LEAVE();
    return ((*group) ? (MPP_OK) : (MPP_NOK));
}

MPP_RET mpp_buffer_group_deinit(MppBufferGroupImpl *p)
{
    if (!p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUF_FUNCTION_ENTER();

    service_put_group(get_srv_buffer(), p, __FUNCTION__);

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

MPP_RET mpp_buffer_group_reset(MppBufferGroupImpl *p)
{
    if (!p) {
        mpp_err_f("found NULL pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_BUF_FUNCTION_ENTER();

    pthread_mutex_lock(&p->buf_lock);

    buf_grp_add_log(p, GRP_RESET, NULL);

    if (!list_empty(&p->list_used)) {
        MppBufferImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
            buf_add_log(pos, BUF_DISCARD, NULL);
            pos->discard = 1;
        }
    }

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferService *srv = get_srv_buffer();
        MppBufferImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            service_put_buffer(srv, p, pos, 0, __FUNCTION__);
        }
    }

    pthread_mutex_unlock(&p->buf_lock);

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

MPP_RET mpp_buffer_group_set_callback(MppBufferGroupImpl *p,
                                      MppBufCallback callback, void *arg)
{
    if (!p)
        return MPP_OK;

    MPP_BUF_FUNCTION_ENTER();

    p->arg      = arg;
    p->callback = callback;

    MPP_BUF_FUNCTION_LEAVE();
    return MPP_OK;
}

rk_u32 mpp_buffer_total_now()
{
    MppBufferService *srv = get_srv_buffer();
    rk_u32 size = 0;

    if (srv)
        size = srv->total_size;

    return size;
}

rk_u32 mpp_buffer_total_max()
{
    MppBufferService *srv = get_srv_buffer();
    rk_u32 size = 0;

    if (srv)
        size = srv->total_max;

    return size;
}

static rk_u32 type_to_flag(MppBufferType type)
{
    rk_u32 flag = MPP_ALLOC_FLAG_NONE;

    if (type & MPP_BUFFER_FLAGS_DMA32)
        flag += MPP_ALLOC_FLAG_DMA32;

    if (type & MPP_BUFFER_FLAGS_CACHABLE)
        flag += MPP_ALLOC_FLAG_CACHABLE;

    if (type & MPP_BUFFER_FLAGS_CONTIG)
        flag += MPP_ALLOC_FLAG_CMA;

    return flag;
}

static rk_u32 service_get_misc(MppBufferService *srv, MppBufferMode mode, MppBufferType type)
{
    rk_u32 flag = type_to_flag(type);

    type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    if (type == MPP_BUFFER_TYPE_NORMAL)
        return 0;

    mpp_assert(mode < MPP_BUFFER_MODE_BUTT);
    mpp_assert(type < MPP_BUFFER_TYPE_BUTT);
    mpp_assert(flag < MPP_ALLOC_FLAG_TYPE_NB);

    return srv->misc[mode][type][flag];
}

MppBufferGroupImpl *mpp_buffer_get_misc_group(MppBufferMode mode, MppBufferType type)
{
    MppBufferService *srv = get_srv_buffer();
    MppBufferGroupImpl *misc;
    MppBufferType buf_type;
    MppMutex *lock = &srv->lock;
    rk_u32 id;

    buf_type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    if (buf_type == MPP_BUFFER_TYPE_NORMAL)
        return NULL;

    mpp_assert(mode < MPP_BUFFER_MODE_BUTT);
    mpp_assert(buf_type < MPP_BUFFER_TYPE_BUTT);

    mpp_mutex_lock(lock);

    id = service_get_misc(srv, mode, type);
    if (!id) {
        char tag[32];
        rk_s32 offset = 0;

        offset += snprintf(tag + offset, sizeof(tag) - offset, "misc");
        offset += snprintf(tag + offset, sizeof(tag) - offset, "_%s",
                           buf_type == MPP_BUFFER_TYPE_ION ? "ion" :
                           buf_type == MPP_BUFFER_TYPE_DRM ? "drm" : "na");
        offset += snprintf(tag + offset, sizeof(tag) - offset, "_%s",
                           mode == MPP_BUFFER_INTERNAL ? "int" : "ext");

        misc = service_get_group(tag, __FUNCTION__, mode, type, 1);
    } else
        misc = get_group_by_id(srv, id);
    mpp_mutex_unlock(lock);

    return misc;
}

static void mpp_buffer_service_init()
{
    MppBufferService *srv = srv_buffer;
    rk_s32 i, j, k;

    if (srv)
        return;

    mpp_env_get_u32("mpp_buffer_debug", &mpp_buffer_debug, 0);

    srv = mpp_calloc(MppBufferService, 1);
    if (!srv) {
        mpp_err_f("alloc buffer service failed\n");
        return;
    }

    srv_buffer = srv;

    srv_status = MPP_BUF_SRV_NORMAL;
    pool_buf = mpp_mem_pool_init_f(MODULE_TAG, sizeof(MppBufferImpl));
    pool_buf_grp = mpp_mem_pool_init_f("mpp_buf_grp", sizeof(MppBufferGroupImpl));
    pool_buf_map_node = mpp_mem_pool_init_f("mpp_buf_map_node", sizeof(MppDevBufMapNode));

    srv->group_id = 1;

    INIT_LIST_HEAD(&srv->list_group);
    INIT_LIST_HEAD(&srv->list_orphan);

    // NOTE: Do not create misc group at beginning. Only create on when needed.
    for (i = 0; i < MPP_BUFFER_MODE_BUTT; i++)
        for (j = 0; j < MPP_BUFFER_TYPE_BUTT; j++)
            for (k = 0; k < MPP_ALLOCATOR_WITH_FLAG_NUM; k++)
                srv->misc[i][j][k] = 0;

    for (i = 0; i < (rk_s32)HASH_SIZE(srv->hash_group); i++)
        INIT_HLIST_HEAD(&srv->hash_group[i]);

    mpp_mutex_init(&srv->lock);
}

static void mpp_buffer_service_deinit()
{
    MppBufferService *srv = srv_buffer;
    rk_s32 i, j, k;

    if (!srv)
        return;

    srv->finalizing = 1;

    // first remove legacy group which is the normal case
    if (srv->misc_count) {
        mpp_log_f("cleaning misc group\n");
        for (i = 0; i < MPP_BUFFER_MODE_BUTT; i++)
            for (j = 0; j < MPP_BUFFER_TYPE_BUTT; j++)
                for (k = 0; k < MPP_ALLOCATOR_WITH_FLAG_NUM; k++) {
                    rk_u32 id = srv->misc[i][j][k];

                    if (id) {
                        service_put_group(srv, get_group_by_id(srv, id), __FUNCTION__);
                        srv->misc[i][j][k] = 0;
                    }
                }
    }

    // then remove the remaining group which is the leak one
    if (!list_empty(&srv->list_group)) {
        MppBufferGroupImpl *pos, *n;

        if (mpp_buffer_debug & MPP_BUF_DBG_DUMP_ON_EXIT)
            service_dump(srv, "leaked group found");

        mpp_log_f("cleaning leaked group\n");
        list_for_each_entry_safe(pos, n, &srv->list_group, MppBufferGroupImpl, list_group) {
            service_put_group(srv, pos, __FUNCTION__);
        }
    }

    // remove all orphan buffer group
    if (!list_empty(&srv->list_orphan)) {
        MppBufferGroupImpl *pos, *n;

        mpp_log_f("cleaning leaked buffer\n");

        list_for_each_entry_safe(pos, n, &srv->list_orphan, MppBufferGroupImpl, list_group) {
            pos->clear_on_exit = 1;
            pos->is_finalizing = 1;
            service_put_group(srv, pos, __FUNCTION__);
        }
    }

    for (i = 0; i < MPP_BUFFER_TYPE_BUTT; i++) {
        for (j = 0; j < MPP_ALLOCATOR_WITH_FLAG_NUM; j++) {
            if (srv->allocator[i][j])
                mpp_allocator_put(&(srv->allocator[i][j]));
        }
    }
    mpp_mutex_destroy(&srv->lock);

    MPP_FREE(srv_buffer);
    srv_status = MPP_BUF_SRV_FINALIZED;
}

rk_u32 service_get_group_id(MppBufferService *srv)
{
    static rk_u32 overflowed = 0;
    rk_u32 id = 0;

    if (!overflowed) {
        /* avoid 0 group id */
        if (srv->group_id)
            id = srv->group_id++;
        else {
            overflowed = 1;
            srv->group_id = 1;
        }
    }

    if (overflowed) {
        id = srv->group_id++;

        /* when it is overflow avoid the used id */
        while (get_group_by_id(srv, id))
            id = srv->group_id++;
    }

    srv->group_count++;

    return id;
}

static MppBufferGroupImpl *service_get_group(const char *tag, const char *caller,
                                             MppBufferMode mode, MppBufferType type,
                                             rk_u32 is_misc)
{
    MppBufferType buffer_type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    MppBufferGroupImpl *p = NULL;
    MppBufferService *srv = get_srv_buffer();
    MppMutex *lock;
    rk_u32 flag;
    rk_u32 id;

    /* env update */
    mpp_env_get_u32("mpp_buffer_debug", &mpp_buffer_debug, mpp_buffer_debug);

    if (mode >= MPP_BUFFER_MODE_BUTT || buffer_type >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err("MppBufferService get_group found invalid mode %d type %x\n", mode, type);
        return NULL;
    }

    if (!srv) {
        mpp_err("MppBufferService get_group failed to get service\n");
        return NULL;
    }

    lock = &srv->lock;
    p = (MppBufferGroupImpl *)mpp_mem_pool_get(pool_buf_grp, caller);
    if (!p) {
        mpp_err("MppBufferService failed to allocate group context\n");
        return NULL;
    }

    flag = type_to_flag(type);

    p->flags = (MppAllocFlagType)flag;

    {
        MppAllocator allocator = NULL;
        MppAllocatorApi *alloc_api = NULL;

        mpp_mutex_lock(lock);

        allocator = srv->allocator[buffer_type][flag];
        alloc_api = srv->allocator_api[buffer_type];

        // allocate general buffer first
        if (!allocator) {
            mpp_allocator_get(&allocator, &alloc_api, type, p->flags);
            srv->allocator[buffer_type][flag] = allocator;
            srv->allocator_api[buffer_type] = alloc_api;
        }

        p->allocator = allocator;
        p->alloc_api = alloc_api;
        p->flags = mpp_allocator_get_flags(allocator);

        mpp_mutex_unlock(lock);
    }

    if (!p->allocator || !p->alloc_api) {
        mpp_mem_pool_put(pool_buf_grp, p, caller);
        mpp_err("MppBufferService get_group failed to get allocater with mode %d type %x\n", mode, type);
        return NULL;
    }

    INIT_LIST_HEAD(&p->list_group);
    INIT_LIST_HEAD(&p->list_used);
    INIT_LIST_HEAD(&p->list_unused);
    INIT_HLIST_NODE(&p->hlist);

    p->log_runtime_en   = (mpp_buffer_debug & MPP_BUF_DBG_OPS_RUNTIME) ? (1) : (0);
    p->log_history_en   = (mpp_buffer_debug & MPP_BUF_DBG_OPS_HISTORY) ? (1) : (0);

    p->caller   = caller;
    p->mode     = mode;
    p->type     = buffer_type;
    p->limit    = BUFFER_GROUP_SIZE_DEFAULT;
    p->clear_on_exit = (mpp_buffer_debug & MPP_BUF_DBG_CLR_ON_EXIT) ? (1) : (0);
    p->dump_on_exit  = (mpp_buffer_debug & MPP_BUF_DBG_DUMP_ON_EXIT) ? (1) : (0);

    {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&p->buf_lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    if (p->log_history_en)
        p->logs = buf_logs_init(BUFFER_OPS_MAX_COUNT);

    mpp_mutex_lock(lock);

    id = service_get_group_id(srv);
    if (tag) {
        snprintf(p->tag, sizeof(p->tag) - 1, "%s_%d", tag, id);
    } else {
        snprintf(p->tag, sizeof(p->tag) - 1, "unknown");
    }
    p->group_id = id;

    list_add_tail(&p->list_group, &srv->list_group);
    hash_add(srv->hash_group, &p->hlist, id);

    buf_grp_add_log(p, GRP_CREATE, caller);

    if (is_misc) {
        srv->misc[mode][buffer_type][flag] = id;
        p->is_misc = 1;
        srv->misc_count++;
    }

    mpp_mutex_unlock(lock);

    return p;
}

static void destroy_group(MppBufferService *srv, MppBufferGroupImpl *group)
{
    mpp_assert(group->count_used == 0);
    mpp_assert(group->count_unused == 0);

    if (group->count_unused || group->count_used) {
        mpp_err("mpp_buffer_group %s deinit mismatch counter used %4d unused %4d found\n",
                group->caller, group->count_used, group->count_unused);
        group->count_unused = 0;
        group->count_used   = 0;
    }

    buf_grp_add_log(group, GRP_DESTROY, __FUNCTION__);

    list_del_init(&group->list_group);
    hash_del(&group->hlist);
    pthread_mutex_destroy(&group->buf_lock);

    if (group->logs) {
        buf_logs_deinit(group->logs);
        group->logs = NULL;
    }

    if (srv) {
        MppBufferMode mode = group->mode;
        MppBufferType type = group->type;
        rk_u32 flag = type_to_flag(type);
        rk_u32 id = group->group_id;

        srv->group_count--;

        if (id == srv->misc[mode][type][flag]) {
            srv->misc[mode][type][flag] = 0;
            srv->misc_count--;
        }
    }

    if (pool_buf_grp)
        mpp_mem_pool_put_f(pool_buf_grp, group);
}

static void service_put_group(MppBufferService *srv, MppBufferGroupImpl *p, const char *caller)
{
    MppMutex *lock;

    if (!srv)
        return;

    lock = &srv->lock;

    if (!srv->finalizing)
        mpp_mutex_lock(lock);

    buf_grp_add_log(p, GRP_RELEASE, caller);

    // remove unused list
    if (!list_empty(&p->list_unused)) {
        MppBufferImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &p->list_unused, MppBufferImpl, list_status) {
            service_put_buffer(srv, p, pos, 0, caller);
        }
    }

    if (list_empty(&p->list_used)) {
        destroy_group(srv, p);
    } else {
        if (!srv->finalizing || (srv->finalizing && p->dump_on_exit)) {
            mpp_err("mpp_group %p tag %s caller %s mode %s type %s deinit with %d bytes not released\n",
                    p, p->tag, p->caller, mode2str[p->mode], type2str[p->type], p->usage);

            mpp_buffer_group_dump(p, caller);
        }

        /* if clear on exit we need to release remaining buffer */
        if (p->clear_on_exit) {
            MppBufferImpl *pos, *n;

            if (p->dump_on_exit)
                mpp_err("force release all remaining buffer\n");

            list_for_each_entry_safe(pos, n, &p->list_used, MppBufferImpl, list_status) {
                if (p->dump_on_exit)
                    mpp_err("clearing buffer %p\n", pos);
                pos->ref_count = 0;
                pos->discard = 1;
                service_put_buffer(srv, p, pos, 0, caller);
            }

            destroy_group(srv, p);
        } else {
            // otherwise move the group to list_orphan and wait for buffer release
            buf_grp_add_log(p, GRP_ORPHAN, caller);
            list_del_init(&p->list_group);
            list_add_tail(&p->list_group, &srv->list_orphan);
            p->is_orphan = 1;
        }
    }

    if (!srv->finalizing)
        mpp_mutex_unlock(lock);
}


static void service_dump(MppBufferService *srv, const char *info)
{
    MppBufferGroupImpl *group;
    struct hlist_node *n;
    rk_u32 key;

    mpp_mutex_lock(&srv->lock);

    mpp_log("dumping all buffer groups for %s\n", info);

    if (hash_empty(srv->hash_group)) {
        mpp_log("no buffer group can be dumped\n");
    } else {
        hash_for_each_safe(srv->hash_group, key, n, group, hlist) {
            mpp_buffer_group_dump(group, __FUNCTION__);
        }
    }

    mpp_mutex_unlock(&srv->lock);
}

MPP_SINGLETON(MPP_SGLN_BUFFER, mpp_buffer, mpp_buffer_service_init, mpp_buffer_service_deinit)
