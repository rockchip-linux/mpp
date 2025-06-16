/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_mem_pool"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_singleton.h"

#include "mpp_mem_pool.h"

#define MEM_POOL_DBG_FLOW               (0x00000001)
#define MEM_POOL_DBG_EXIT               (0x00000002)

#define mem_pool_dbg(flag, fmt, ...)    _mpp_dbg(mpp_mem_pool_debug, flag, fmt, ## __VA_ARGS__)
#define mem_pool_dbg_f(flag, fmt, ...)  _mpp_dbg_f(mpp_mem_pool_debug, flag, fmt, ## __VA_ARGS__)

#define mem_pool_dbg_flow(fmt, ...)     mem_pool_dbg(MEM_POOL_DBG_FLOW, fmt, ## __VA_ARGS__)
#define mem_pool_dbg_exit(fmt, ...)     mem_pool_dbg(MEM_POOL_DBG_EXIT, fmt, ## __VA_ARGS__)

#define get_srv_mem_pool(caller) \
    ({ \
        MppMemPoolSrv *__tmp; \
        if (!srv_mem_pool) { \
            mem_pool_srv_init(); \
        } \
        if (srv_mem_pool) { \
            __tmp = srv_mem_pool; \
        } else { \
            mpp_err("mpp mem pool srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

static rk_u32 mpp_mem_pool_debug = 0;

typedef struct MppMemPoolNode_t {
    void                *check;
    struct list_head    list;
    void                *ptr;
    size_t              size;
} MppMemPoolNode;

typedef struct MppMemPoolImpl_t {
    void                *check;
    const char          *name;
    size_t              size;
    pthread_mutex_t     lock;
    struct list_head    service_link;

    struct list_head    used;
    struct list_head    unused;
    rk_s32              used_count;
    rk_s32              unused_count;

    /* extra flag for C++ static destruction order error */
    rk_s32              finalized;
} MppMemPoolImpl;

typedef struct  MppMemPoolService_t {
    struct list_head    list;
    pthread_mutex_t     lock;
} MppMemPoolSrv;

static MppMemPoolSrv *srv_mem_pool = NULL;

static void mem_pool_srv_init()
{
    MppMemPoolSrv *srv = srv_mem_pool;

    mpp_env_get_u32("mpp_mem_pool_debug", &mpp_mem_pool_debug, 0);

    if (srv)
        return;

    srv = mpp_malloc(MppMemPoolSrv, 1);
    if (!srv) {
        mpp_err_f("failed to allocate pool service\n");
        return;
    }

    srv_mem_pool = srv;

    {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&srv->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    INIT_LIST_HEAD(&srv->list);
}

static void put_pool(MppMemPoolSrv *srv, MppMemPoolImpl *impl)
{
    MppMemPoolNode *node, *m;

    if (impl != impl->check) {
        mpp_err_f("invalid mem impl %p check %p\n", impl, impl->check);
        return;
    }

    if (impl->finalized)
        return;

    pthread_mutex_lock(&impl->lock);

    if (!list_empty(&impl->unused)) {
        list_for_each_entry_safe(node, m, &impl->unused, MppMemPoolNode, list) {
            MPP_FREE(node);
            impl->unused_count--;
        }
    }

    if (!list_empty(&impl->used)) {
        mpp_err_f("pool %-16s found %d used buffer size %4d\n",
                  impl->name, impl->used_count, impl->size);

        list_for_each_entry_safe(node, m, &impl->used, MppMemPoolNode, list) {
            MPP_FREE(node);
            impl->used_count--;
        }
    }

    if (impl->used_count || impl->unused_count)
        mpp_err_f("pool %-16s size %4d found leaked buffer used:unused [%d:%d]\n",
                  impl->name, impl->size, impl->used_count, impl->unused_count);

    pthread_mutex_unlock(&impl->lock);

    if (srv) {
        pthread_mutex_lock(&srv->lock);
        list_del_init(&impl->service_link);
        pthread_mutex_unlock(&srv->lock);
    }

    impl->finalized = 1;
    mpp_free(impl);
}

static void mem_pool_srv_deinit()
{
    MppMemPoolSrv *srv = srv_mem_pool;

    if (!srv)
        return;

    if (!list_empty(&srv->list)) {
        MppMemPoolImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &srv->list, MppMemPoolImpl, service_link) {
            mem_pool_dbg_exit("pool %-16s size %4d leaked\n", pos->name, pos->size);
            put_pool(srv, pos);
        }
    }

    pthread_mutex_destroy(&srv->lock);

    mpp_free(srv);
    srv_mem_pool = NULL;
}

MppMemPool mpp_mem_pool_init(const char *name, size_t size, const char *caller)
{
    MppMemPoolSrv *srv = get_srv_mem_pool(caller);
    MppMemPoolImpl *pool;

    if (!srv)
        return NULL;

    pool = mpp_calloc(MppMemPoolImpl, 1);
    if (!pool)
        return NULL;

    {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&pool->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    pool->check = pool;
    pool->name = name;
    pool->size = size;
    pool->used_count = 0;
    pool->unused_count = 0;
    pool->finalized = 0;

    INIT_LIST_HEAD(&pool->used);
    INIT_LIST_HEAD(&pool->unused);
    INIT_LIST_HEAD(&pool->service_link);

    pthread_mutex_lock(&srv->lock);
    list_add_tail(&pool->service_link, &srv->list);
    pthread_mutex_unlock(&srv->lock);

    mem_pool_dbg_flow("pool %-16s size %4d init at %s\n", pool->name, size, caller);

    return pool;
}

void mpp_mem_pool_deinit(MppMemPool pool, const char *caller)
{
    MppMemPoolSrv *srv = get_srv_mem_pool(caller);
    MppMemPoolImpl *impl = (MppMemPoolImpl *)pool;

    mem_pool_dbg_flow("pool %-16s size %4d deinit at %s\n",
                      impl->name, impl->size, caller);

    put_pool(srv, impl);
}

void *mpp_mem_pool_get(MppMemPool pool, const char *caller)
{
    MppMemPoolImpl *impl = (MppMemPoolImpl *)pool;
    MppMemPoolNode *node = NULL;
    void* ptr = NULL;

    pthread_mutex_lock(&impl->lock);

    mem_pool_dbg_flow("pool %-16s size %4d get used:unused [%d:%d] at %s\n",
                      impl->name, impl->size, impl->used_count, impl->unused_count, caller);

    if (!list_empty(&impl->unused)) {
        node = list_first_entry(&impl->unused, MppMemPoolNode, list);
        if (node) {
            list_del_init(&node->list);
            list_add_tail(&node->list, &impl->used);
            impl->unused_count--;
            impl->used_count++;
            ptr = node->ptr;
            node->check = node;
            goto DONE;
        }
    }

    node = mpp_malloc_size(MppMemPoolNode, sizeof(MppMemPoolNode) + impl->size);
    if (!node) {
        mpp_err_f("failed to create node from size %4d pool\n", impl->size);
        goto DONE;
    }

    node->check = node;
    node->ptr = (void *)(node + 1);
    node->size = impl->size;
    INIT_LIST_HEAD(&node->list);
    list_add_tail(&node->list, &impl->used);
    impl->used_count++;
    ptr = node->ptr;

DONE:
    pthread_mutex_unlock(&impl->lock);
    if (node)
        memset(node->ptr, 0 , node->size);
    return ptr;
}

void mpp_mem_pool_put(MppMemPool pool, void *p, const char *caller)
{
    MppMemPoolImpl *impl = (MppMemPoolImpl *)pool;
    MppMemPoolNode *node = (MppMemPoolNode *)((rk_u8 *)p - sizeof(MppMemPoolNode));

    if (impl != impl->check) {
        mpp_err_f("invalid mem pool %p check %p\n", impl, impl->check);
        return ;
    }

    if (node != node->check) {
        mpp_err_f("invalid mem pool ptr %p node %p check %p\n",
                  p, node, node->check);
        return ;
    }

    pthread_mutex_lock(&impl->lock);

    mem_pool_dbg_flow("pool %-16s size %4d put used:unused [%d:%d] at %s\n",
                      impl->name, impl->size, impl->used_count, impl->unused_count, caller);

    list_del_init(&node->list);
    list_add(&node->list, &impl->unused);
    impl->used_count--;
    impl->unused_count++;
    node->check = NULL;

    pthread_mutex_unlock(&impl->lock);
}

MPP_SINGLETON(MPP_SGLN_MEM_POOL, mpp_mem_pool, mem_pool_srv_init, mem_pool_srv_deinit)
