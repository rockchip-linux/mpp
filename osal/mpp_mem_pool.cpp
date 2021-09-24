/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_mem_pool"

#include <string.h>

#include "mpp_err.h"
#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_list.h"

#include "mpp_mem_pool.h"

#define MPP_MEM_POOL_DBG_FLOW           (0x00000001)

#define mem_pool_dbg(flag, fmt, ...)    _mpp_dbg(mpp_mem_pool_debug, flag, fmt, ## __VA_ARGS__)
#define mem_pool_dbg_f(flag, fmt, ...)  _mpp_dbg_f(mpp_mem_pool_debug, flag, fmt, ## __VA_ARGS__)

#define mem_pool_dbg_flow(fmt, ...)     mem_pool_dbg(MPP_MEM_POOL_DBG_FLOW, fmt, ## __VA_ARGS__)

RK_U32 mpp_mem_pool_debug = 0;

typedef struct MppMemPoolNode_t {
    void                *check;
    struct list_head    list;
    void                *ptr;
    size_t              size;
} MppMemPoolNode;

typedef struct MppMemPoolImpl_t {
    void                *check;
    size_t              size;
    pthread_mutex_t     lock;
    struct list_head    service_link;

    struct list_head    used;
    struct list_head    unused;
    RK_S32              used_count;
    RK_S32              unused_count;
} MppMemPoolImpl;

class MppMemPoolService
{
public:
    static MppMemPoolService* getInstance() {
        AutoMutex auto_lock(get_lock());
        static MppMemPoolService pool_service;
        return &pool_service;
    }
    static Mutex *get_lock() {
        static Mutex lock;
        return &lock;
    }

    MppMemPoolImpl *get_pool(size_t size);
    void put_pool(MppMemPoolImpl *impl);

private:
    MppMemPoolService();
    ~MppMemPoolService();
    struct list_head    mLink;
};

MppMemPoolService::MppMemPoolService()
{
    INIT_LIST_HEAD(&mLink);

    mpp_env_get_u32("mpp_mem_pool_debug", &mpp_mem_pool_debug, 0);
}

MppMemPoolService::~MppMemPoolService()
{
    if (!list_empty(&mLink)) {
        MppMemPoolImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &mLink, MppMemPoolImpl, service_link) {
            put_pool(pos);
        }
    }
}

MppMemPoolImpl *MppMemPoolService::get_pool(size_t size)
{
    MppMemPoolImpl *pool = mpp_malloc(MppMemPoolImpl, 1);
    if (NULL == pool)
        return NULL;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&pool->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    pool->check = pool;
    pool->size = size;
    pool->used_count = 0;
    pool->unused_count = 0;

    INIT_LIST_HEAD(&pool->used);
    INIT_LIST_HEAD(&pool->unused);
    INIT_LIST_HEAD(&pool->service_link);
    AutoMutex auto_lock(get_lock());
    list_add_tail(&pool->service_link, &mLink);

    return pool;
}

void MppMemPoolService::put_pool(MppMemPoolImpl *impl)
{
    MppMemPoolNode *node, *m;

    if (impl != impl->check) {
        mpp_err_f("invalid mem impl %p check %p\n", impl, impl->check);
        return ;
    }

    if (!list_empty(&impl->unused)) {
        list_for_each_entry_safe(node, m, &impl->unused, MppMemPoolNode, list) {
            MPP_FREE(node);
            impl->unused_count--;
        }
    }

    if (!list_empty(&impl->used)) {
        mpp_err_f("found %d used buffer size %d\n",
                  impl->used_count, impl->size);

        list_for_each_entry_safe(node, m, &impl->used, MppMemPoolNode, list) {
            MPP_FREE(node);
            impl->used_count--;
        }
    }

    mpp_assert(!impl->used_count);
    mpp_assert(!impl->unused_count);

    {
        AutoMutex auto_lock(get_lock());
        list_del_init(&impl->service_link);
    }

    mpp_free(impl);
}

MppMemPool mpp_mem_pool_init_f(const char *caller, size_t size)
{
    mem_pool_dbg_flow("pool %d init from %s", size, caller);

    return (MppMemPool)MppMemPoolService::getInstance()->get_pool(size);
}

void mpp_mem_pool_deinit_f(const char *caller, MppMemPool pool)
{
    MppMemPoolImpl *impl = (MppMemPoolImpl *)pool;

    mem_pool_dbg_flow("pool %d deinit from %s", impl->size, caller);

    MppMemPoolService::getInstance()->put_pool(impl);
}

void *mpp_mem_pool_get_f(const char *caller, MppMemPool pool)
{
    MppMemPoolImpl *impl = (MppMemPoolImpl *)pool;
    MppMemPoolNode *node = NULL;
    void* ptr = NULL;

    pthread_mutex_lock(&impl->lock);

    mem_pool_dbg_flow("pool %d get used:unused [%d:%d] from %s", impl->size,
                      impl->used_count, impl->unused_count, caller);

    if (!list_empty(&impl->unused)) {
        node = list_first_entry(&impl->unused, MppMemPoolNode, list);
        if (node) {
            list_del_init(&node->list);
            list_add_tail(&node->list, &impl->used);
            impl->unused_count--;
            impl->used_count++;
            ptr = node->ptr;
            goto DONE;
        }
    }

    node = mpp_malloc_size(MppMemPoolNode, sizeof(MppMemPoolNode) + impl->size);
    if (NULL == node) {
        mpp_err_f("failed to create node from size %d pool\n", impl->size);
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

void mpp_mem_pool_put_f(const char *caller, MppMemPool pool, void *p)
{
    MppMemPoolImpl *impl = (MppMemPoolImpl *)pool;
    MppMemPoolNode *node = (MppMemPoolNode *)((RK_U8 *)p - sizeof(MppMemPoolNode));

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

    mem_pool_dbg_flow("pool %d put used:unused [%d:%d] from %s", impl->size,
                      impl->used_count, impl->unused_count, caller);

    list_del_init(&node->list);
    list_add(&node->list, &impl->unused);
    impl->used_count--;
    impl->unused_count++;

    pthread_mutex_unlock(&impl->lock);
}
