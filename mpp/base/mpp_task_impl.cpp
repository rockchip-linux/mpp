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

#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_task_impl.h"

#define MAX_TASK_COUNT      8

typedef struct MppTaskStatusInfo_t {
    struct list_head    list;
    RK_S32              count;
    MppTaskStatus       status;
} MppTaskStatusInfo;

typedef struct MppTaskQueueImpl_t {
    Mutex               *lock;
    RK_S32              task_count;

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

void setup_mpp_task_name(MppTaskImpl *task)
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

    return MPP_OK;
}

static MPP_RET mpp_port_deinit(MppPort port)
{
    mpp_free(port);
    return MPP_OK;
}

MPP_RET mpp_port_can_dequeue(MppPort port)
{
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;

    AutoMutex auto_lock(queue->lock);
    MppTaskStatusInfo *curr = &queue->info[port_impl->status_curr];

    if (curr->count) {
        mpp_assert(!list_empty(&curr->list));
        return MPP_OK;
    }

    mpp_assert(list_empty(&curr->list));
    return MPP_NOK;
}

MPP_RET mpp_port_dequeue(MppPort port, MppTask *task)
{
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;

    AutoMutex auto_lock(queue->lock);
    MppTaskStatusInfo *curr = &queue->info[port_impl->status_curr];
    MppTaskStatusInfo *next = &queue->info[port_impl->next_on_dequeue];

    *task = NULL;
    if (curr->count == 0) {
        mpp_assert(list_empty(&curr->list));
        return MPP_OK;
    }

    MppTaskImpl *task_impl = list_entry(curr->list.next, MppTaskImpl, list);
    MppTask p = (MppTask)task_impl;
    check_mpp_task_name(p);
    list_del_init(&task_impl->list);
    curr->count--;
    mpp_assert(curr->count >= 0);

    list_add_tail(&task_impl->list, &next->list);
    next->count++;
    task_impl->status = next->status;

    *task = p;

    return MPP_OK;
}

MPP_RET mpp_port_enqueue(MppPort port, MppTask task)
{
    MppTaskImpl *task_impl = (MppTaskImpl *)task;
    MppPortImpl *port_impl = (MppPortImpl *)port;
    MppTaskQueueImpl *queue = port_impl->queue;
    check_mpp_task_name(task);

    mpp_assert(task_impl->queue  == (MppTaskQueue *)queue);
    mpp_assert(task_impl->status == port_impl->next_on_dequeue);

    AutoMutex auto_lock(queue->lock);
    MppTaskStatusInfo *curr = &queue->info[task_impl->status];
    MppTaskStatusInfo *next = &queue->info[port_impl->next_on_enqueue];

    list_del_init(&task_impl->list);
    curr->count--;
    list_add_tail(&task_impl->list, &next->list);
    next->count++;
    task_impl->status = next->status;

    return MPP_OK;
}

MPP_RET mpp_task_queue_init(MppTaskQueue *queue)
{
    if (NULL == queue) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppTaskQueueImpl *p = NULL;
    MppTaskImpl *tasks = NULL;
    Mutex *lock = NULL;

    do {
        RK_S32 i;

        p = mpp_calloc(MppTaskQueueImpl, 1);
        if (NULL == p) {
            mpp_err_f("malloc queue failed\n");
            break;
        }
        lock = new Mutex();
        if (NULL == lock) {
            mpp_err_f("new lock failed\n");
            break;;
        }

        for (i = 0; i < MPP_TASK_STATUS_BUTT; i++) {
            INIT_LIST_HEAD(&p->info[i].list);
            p->info[i].count  = 0;
            p->info[i].status = (MppTaskStatus)i;
        }

        p->lock         = lock;
        p->tasks        = tasks;

        if (mpp_port_init(p, MPP_PORT_INPUT, &p->input))
            break;

        if (mpp_port_init(p, MPP_PORT_OUTPUT, &p->output)) {
            mpp_port_deinit(p->input);
            break;
        }

        *queue = p;
        return MPP_OK;
    } while (0);

    if (p)
        mpp_free(p);
    if (lock)
        delete lock;
    if (tasks)
        mpp_free(tasks);

    *queue = NULL;
    return MPP_NOK;
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
    return MPP_OK;
}

MPP_RET mpp_task_queue_deinit(MppTaskQueue queue)
{
    if (NULL == queue) {
        mpp_err_f("found NULL input queue\n");
        return MPP_ERR_NULL_PTR;
    }

    MppTaskQueueImpl *p = (MppTaskQueueImpl *)queue;
    if (p->input) {
        mpp_port_deinit(p->input);
        p->input = NULL;
    }
    if (p->output) {
        mpp_port_deinit(p->output);
        p->output = NULL;
    }
    if (p->tasks) {
        for (RK_S32 i = 0; i < p->task_count; i++) {
            mpp_meta_put(p->tasks[i].meta);
        }
        mpp_free(p->tasks);
    }
    if (p->lock)
        delete p->lock;
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

