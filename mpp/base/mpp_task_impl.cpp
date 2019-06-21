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

#define MODULE_TAG "mpp_task_impl"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_task_impl.h"
#include "mpp_meta_impl.h"

#define MAX_TASK_COUNT      8

#define MPP_TASK_DBG_FUNCTION       (0x00000001)

#define mpp_task_dbg(flag, fmt, ...)     _mpp_dbg(mpp_task_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_task_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_task_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_task_dbg_func(fmt, ...)      mpp_task_dbg_f(MPP_TASK_DBG_FUNCTION, fmt, ## __VA_ARGS__)

typedef struct MppTaskStatusInfo_t {
    struct list_head    list;
    RK_S32              count;
    MppTaskStatus       status;
    Condition           *cond;
} MppTaskStatusInfo;

typedef struct MppTaskQueueImpl_t {
    Mutex               *lock;
    RK_S32              task_count;
    RK_S32              ready;          // flag for deinit
    Condition           *finish_done;   // condition for deinit done

    // two ports inside of task queue
    MppPort             input;
    MppPort             output;

    MppTaskImpl         *tasks;

    MppTaskStatusInfo   info[MPP_TASK_STATUS_BUTT];
} MppTaskQueueImpl;

typedef struct MppPortImpl_t {
    MppPortType         type;
    MppTaskQueueImpl    *queue;

    MppTaskStatus       status_curr;
    MppTaskStatus       next_on_dequeue;
    MppTaskStatus       next_on_enqueue;
} MppPortImpl;

static const char *module_name = MODULE_TAG;

RK_U32 mpp_task_debug = 0;

static inline void setup_mpp_task_name(MppTaskImpl *task)
{
    task->name = module_name;
}

MPP_RET check_mpp_task_name(MppTask task)
{
    if (task && ((MppTaskImpl *)task)->name == module_name)
        return MPP_OK;

    mpp_err_f("pointer %p failed on check\n", task);
    mpp_abort();
    return MPP_NOK;
}

static MPP_RET mpp_port_init(MppTaskQueueImpl *queue, MppPortType type, MppPort *port)
{
    MppPortImpl *impl = mpp_malloc(MppPortImpl, 1);
    if (NULL == impl) {
        mpp_err_f("failed to malloc MppPort type %d\n", type);
        return MPP_ERR_MALLOC;
    }

    mpp_task_dbg_func("enter queue %p type %d\n", queue, type);

    impl->type  = type;
    impl->queue = queue;

    if (MPP_PORT_INPUT == type) {
        impl->status_curr     = MPP_INPUT_PORT;
        impl->next_on_dequeue = MPP_INPUT_HOLD;
        impl->next_on_enqueue = MPP_OUTPUT_PORT;
    } else {
        impl->status_curr     = MPP_OUTPUT_PORT;
        impl->next_on_dequeue = MPP_OUTPUT_HOLD;
        impl->next_on_enqueue = MPP_INPUT_PORT;
    }

    *port = (MppPort *)impl;

    mpp_task_dbg_func("leave queue %p port %p\n", queue, impl);

    return MPP_OK;
}

static MPP_RET mpp_port_deinit(MppPort port)
{
    mpp_task_dbg_func("enter port %p\n", port);
    mpp_free(port);
    mpp_task_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET _mpp_port_poll(const char *caller, MppPort port, MppPollType timeout)
{
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;

    AutoMutex auto_lock(queue->lock);
    MppTaskStatusInfo *curr = NULL;
    MPP_RET ret = MPP_NOK;

    mpp_task_dbg_func("caller %s enter port %p timeout %d\n", caller, port, timeout);
    if (!queue->ready) {
        mpp_err("try to query when %s queue is not ready\n",
                (port_impl->type == MPP_PORT_INPUT) ?
                ("input") : ("output"));
        goto RET;
    }

    curr = &queue->info[port_impl->status_curr];
    if (curr->count) {
        mpp_assert(!list_empty(&curr->list));
        ret = MPP_OK;
    } else {
        mpp_assert(list_empty(&curr->list));

        /* timeout
         * zero     - non-block
         * negtive  - block
         * positive - timeout value
         */
        if (timeout != MPP_POLL_NON_BLOCK) {
            mpp_assert(curr->cond);
            Condition *cond = curr->cond;
            RK_S32 wait_ret = 0;
            if (timeout == MPP_POLL_BLOCK) {
                mpp_task_dbg_func("port %p block wait start\n", port);
                wait_ret = cond->wait(queue->lock);
                mpp_task_dbg_func("port %p block wait done ret %d\n", port, wait_ret);
            } else {
                mpp_task_dbg_func("port %p timed wait start %d\n", port, timeout);
                wait_ret = cond->timedwait(queue->lock, timeout);
                mpp_task_dbg_func("port %p timed wait done ret %d\n", port, wait_ret);
            }

            if (curr->count) {
                mpp_assert(!list_empty(&curr->list));
                ret = MPP_OK;
            }
        }
    }
RET:
    mpp_task_dbg_func("caller %s leave port %p ret %d\n", caller, port, ret);
    return ret;
}

MPP_RET _mpp_port_dequeue(const char *caller, MppPort port, MppTask *task)
{
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;
    MppTaskStatusInfo *curr = NULL;
    MppTaskStatusInfo *next = NULL;
    MppTaskImpl *task_impl = NULL;
    MppTask p = NULL;

    AutoMutex auto_lock(queue->lock);
    MPP_RET ret = MPP_NOK;

    mpp_task_dbg_func("caller %s enter port %p\n", caller, port);

    if (!queue->ready) {
        mpp_err("try to dequeue when %s queue is not ready\n",
                (port_impl->type == MPP_PORT_INPUT) ?
                ("input") : ("output"));
        goto RET;
    }

    curr = &queue->info[port_impl->status_curr];
    next = &queue->info[port_impl->next_on_dequeue];

    mpp_task_dbg_func("move  port %p task %p %d -> %d\n", port, task,
                      port_impl->status_curr, port_impl->next_on_dequeue);

    *task = NULL;
    if (curr->count == 0) {
        mpp_assert(list_empty(&curr->list));
        goto RET;
    }

    mpp_assert(!list_empty(&curr->list));
    task_impl = list_entry(curr->list.next, MppTaskImpl, list);
    p = (MppTask)task_impl;
    check_mpp_task_name(p);
    list_del_init(&task_impl->list);
    curr->count--;
    mpp_assert(curr->count >= 0);

    list_add_tail(&task_impl->list, &next->list);
    next->count++;
    task_impl->status = next->status;

    *task = p;
    ret = MPP_OK;
RET:
    mpp_task_dbg_func("caller %s leave port %p task %p ret %d\n", caller, port, *task, ret);

    return ret;
}

MPP_RET _mpp_port_enqueue(const char *caller, MppPort port, MppTask task)
{
    MppTaskImpl *task_impl = (MppTaskImpl *)task;
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;
    MppTaskStatusInfo *curr = NULL;
    MppTaskStatusInfo *next = NULL;

    AutoMutex auto_lock(queue->lock);
    MPP_RET ret = MPP_NOK;

    mpp_task_dbg_func("caller %s enter port %p task %p\n", caller, port, task);

    if (!queue->ready) {
        mpp_err("try to enqueue when %s queue is not ready\n",
                (port_impl->type == MPP_PORT_INPUT) ?
                ("input") : ("output"));
        goto RET;
    }

    check_mpp_task_name(task);

    mpp_assert(task_impl->queue  == (MppTaskQueue *)queue);
    mpp_assert(task_impl->status == port_impl->next_on_dequeue);

    curr = &queue->info[task_impl->status];
    next = &queue->info[port_impl->next_on_enqueue];

    mpp_task_dbg_func("move  port %p task %p %d -> %d\n", port, task,
                      task_impl->status, port_impl->next_on_enqueue);

    list_del_init(&task_impl->list);
    curr->count--;
    list_add_tail(&task_impl->list, &next->list);
    next->count++;
    task_impl->status = next->status;

    next->cond->signal();
    mpp_task_dbg_func("signal port %p\n", next);
    ret = MPP_OK;
RET:
    mpp_task_dbg_func("caller %s leave port %p task %p ret %d\n", caller, port, task, ret);

    return ret;
}

MPP_RET _mpp_port_awake(const char *caller, MppPort port)
{
    if (port == NULL)
        return MPP_NOK;

    mpp_task_dbg_func("caller %s enter port %p\n", caller, port);
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;
    MppTaskStatusInfo *curr = NULL;
    if (queue) {
        AutoMutex auto_lock(queue->lock);
        curr = &queue->info[port_impl->status_curr];
        if (curr) {
            curr->cond->signal();
        }
    }

    mpp_task_dbg_func("caller %s leave port %p\n", caller, port);
    return MPP_OK;
}

MPP_RET mpp_task_queue_init(MppTaskQueue *queue)
{
    if (NULL == queue) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_NOK;
    MppTaskQueueImpl *p = NULL;
    Mutex *lock = NULL;
    Condition *cond[MPP_TASK_STATUS_BUTT] = { NULL };
    RK_S32 i;

    mpp_env_get_u32("mpp_task_debug", &mpp_task_debug, 0);
    mpp_task_dbg_func("enter\n");

    *queue = NULL;

    p = mpp_calloc(MppTaskQueueImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc queue failed\n");
        goto RET;
    }

    cond[MPP_INPUT_PORT] = new Condition();
    cond[MPP_INPUT_HOLD] = NULL;
    cond[MPP_OUTPUT_PORT] = new Condition();
    cond[MPP_OUTPUT_HOLD] = NULL;

    if (NULL == cond[MPP_INPUT_PORT] ||
        NULL == cond[MPP_OUTPUT_PORT]) {
        mpp_err_f("new condition failed\n");
        goto RET;
    }

    for (i = 0; i < MPP_TASK_STATUS_BUTT; i++) {
        INIT_LIST_HEAD(&p->info[i].list);
        p->info[i].count  = 0;
        p->info[i].status = (MppTaskStatus)i;
        p->info[i].cond = cond[i];
    }

    lock = new Mutex();
    if (NULL == lock) {
        mpp_err_f("new lock failed\n");
        goto RET;
    }

    p->lock = lock;

    if (mpp_port_init(p, MPP_PORT_INPUT, &p->input))
        goto RET;

    if (mpp_port_init(p, MPP_PORT_OUTPUT, &p->output)) {
        mpp_port_deinit(p->input);
        goto RET;
    }

    ret = MPP_OK;
RET:
    if (ret) {
        if (lock)
            delete lock;
        if (cond[MPP_INPUT_PORT])
            delete cond[MPP_INPUT_PORT];
        if (cond[MPP_OUTPUT_PORT])
            delete cond[MPP_OUTPUT_PORT];
        MPP_FREE(p);
    }

    *queue = p;

    mpp_task_dbg_func("leave ret %d queue %p\n", ret, p);
    return ret;
}

MPP_RET mpp_task_queue_setup(MppTaskQueue queue, RK_S32 task_count)
{
    MppTaskQueueImpl *impl = (MppTaskQueueImpl *)queue;
    AutoMutex auto_lock(impl->lock);

    // NOTE: queue can only be setup once
    mpp_assert(impl->tasks == NULL);
    mpp_assert(impl->task_count == 0);
    MppTaskImpl *tasks = mpp_calloc(MppTaskImpl, task_count);
    if (NULL == tasks) {
        mpp_err_f("malloc tasks list failed\n");
        return MPP_ERR_MALLOC;
    }

    impl->tasks = tasks;
    impl->task_count = task_count;

    MppTaskStatusInfo *info = &impl->info[MPP_INPUT_PORT];

    for (RK_S32 i = 0; i < task_count; i++) {
        setup_mpp_task_name(&tasks[i]);
        INIT_LIST_HEAD(&tasks[i].list);
        tasks[i].index  = i;
        tasks[i].queue  = (MppTaskQueue *)queue;
        tasks[i].status = MPP_INPUT_PORT;
        mpp_meta_get(&tasks[i].meta);

        list_add_tail(&tasks[i].list, &info->list);
        info->count++;
    }
    impl->ready = 1;
    return MPP_OK;
}

MPP_RET mpp_task_queue_deinit(MppTaskQueue queue)
{
    if (NULL == queue) {
        mpp_err_f("found NULL input queue\n");
        return MPP_ERR_NULL_PTR;
    }

    MppTaskQueueImpl *p = (MppTaskQueueImpl *)queue;
    p->lock->lock();

    p->ready = 0;
    p->info[MPP_INPUT_PORT].cond->signal();
    p->info[MPP_OUTPUT_PORT].cond->signal();
    if (p->tasks) {
        for (RK_S32 i = 0; i < p->task_count; i++) {
            MppTaskStatus status = p->tasks[i].status;

            /* we must ensure that all task return to init status */
            if (status == MPP_OUTPUT_PORT || status == MPP_OUTPUT_HOLD) {
                MppMeta meta = p->tasks[i].meta;

                mpp_err_f("idx %d task %p status %d meta size %d\n", i,
                          &p->tasks[i], p->tasks[i].status,
                          mpp_meta_size(meta));

                while (mpp_meta_size(meta)) {
                    MppMetaNode *node = mpp_meta_next_node(meta);

                    mpp_err_f("meta %p node %p id %d type %d\n",
                              meta, node, node->node_id, node->type_id);

                    MPP_FREE(node);
                }
            }

            mpp_assert(p->tasks[i].status == MPP_INPUT_PORT ||
                       p->tasks[i].status == MPP_INPUT_HOLD);
            mpp_meta_put(p->tasks[i].meta);
        }
        mpp_free(p->tasks);
    }

    if (p->input) {
        mpp_port_deinit(p->input);
        p->input = NULL;
    }
    if (p->output) {
        mpp_port_deinit(p->output);
        p->output = NULL;
    }
    p->lock->unlock();
    if (p->lock)
        delete p->lock;
    if (p->info[MPP_INPUT_PORT].cond) {
        delete p->info[MPP_INPUT_PORT].cond;
        p->info[MPP_INPUT_PORT].cond = NULL;
    }
    if (p->info[MPP_OUTPUT_PORT].cond) {
        delete p->info[MPP_OUTPUT_PORT].cond;
        p->info[MPP_OUTPUT_PORT].cond = NULL;
    }
    mpp_free(p);
    return MPP_OK;
}

MppPort mpp_task_queue_get_port(MppTaskQueue queue, MppPortType type)
{
    if (NULL == queue || type >= MPP_PORT_BUTT) {
        mpp_err_f("invalid input queue %p type %d\n", queue, type);
        return NULL;
    }

    MppTaskQueueImpl *impl = (MppTaskQueueImpl *)queue;
    return (type == MPP_PORT_INPUT) ? (impl->input) : (impl->output);
}

