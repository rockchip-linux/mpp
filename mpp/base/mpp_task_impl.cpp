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

typedef struct MppTaskGroupImpl_t {
    MppPortType         type;
    RK_S32              task_count;
    Mutex               *lock;

    MppTaskImpl         *tasks;

    struct list_head    list[MPP_TASK_BUTT];
    RK_S32              count[MPP_TASK_BUTT];
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

MPP_RET mpp_port_init(MppPort *port, MppPortType type, RK_S32 task_count)
{
    if (NULL == port || type >= MPP_PORT_BUTT || task_count > MAX_TASK_COUNT) {
        mpp_err_f("invalid input port %p type %d count %d\n", port, type, task_count);
        return MPP_ERR_UNKNOW;
    }

    MppPortImpl *p = NULL;
    MppTaskImpl *tasks = NULL;
    Mutex *lock = NULL;
    MppTaskStatus status = (type == MPP_PORT_INPUT) ? (MPP_EXTERNAL_HOLD) : (MPP_INTERNAL_HOLD);

    do {
        p = mpp_calloc(MppPortImpl, 1);
        if (NULL == p) {
            mpp_err_f("malloc port failed\n");
            break;
        }
        lock = new Mutex();
        if (NULL == lock) {
            mpp_err_f("new lock failed\n");
            break;;
        }
        tasks = mpp_calloc(MppTaskImpl, task_count);
        if (NULL == tasks) {
            mpp_err_f("malloc tasks list failed\n");
            break;;
        }

        p->type         = type;
        p->task_count   = task_count;
        p->lock         = lock;
        p->tasks        = tasks;

        for (RK_U32 i = 0; i < MPP_TASK_BUTT; i++)
            INIT_LIST_HEAD(&p->list[i]);

        for (RK_S32 i = 0; i < task_count; i++) {
            setup_mpp_task_name(&tasks[i]);
            INIT_LIST_HEAD(&tasks[i].list);
            tasks[i].index  = i;
            tasks[i].port  = port;
            tasks[i].status = status;
            list_add_tail(&tasks[i].list, &p->list[status]);
            p->count[status]++;
        }
        *port = p;
        return MPP_OK;
    } while (0);

    if (p)
        mpp_free(p);
    if (lock)
        delete lock;
    if (tasks)
        mpp_free(tasks);

    *port = NULL;
    return MPP_NOK;
}

MPP_RET mpp_port_deinit(MppPort port)
{
    if (NULL == port) {
        mpp_err_f("found NULL input port\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPortImpl *p = (MppPortImpl *)port;
    if (p->tasks)
        mpp_free(p->tasks);
    if (p->lock)
        delete p->lock;
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_port_can_dequeue(MppPort port)
{
    if (NULL == port) {
        mpp_err_f("invalid input port %p\n", port);
        return MPP_ERR_NULL_PTR;
    }

    MppPortImpl *p = (MppPortImpl *)port;
    AutoMutex auto_lock(p->lock);
    MppTaskStatus status_curr;
    MppTaskStatus status_next;

    return MPP_OK;
}

MPP_RET mpp_port_can_enqueue(MppPort port)
{
    return MPP_OK;
}

static MppTaskImpl* mpp_task_get_by_status(MppPortImpl *p, MppTaskStatus status)
{
    struct list_head *list = &p->list[status];
    if (list_empty(list))
        return NULL;

    MppTaskImpl *task = list_entry(list->next, MppTaskImpl, list);
    mpp_assert(task->status == status);
    list_del_init(&task->list);
    return task;
}

static MPP_RET mpp_task_put_by_status(MppPortImpl *p, MppTaskStatus status, MppTaskImpl* task)
{
    task->status = status;
    list_add_tail(&task->list, &p->list[status]);
    return MPP_OK;
}

MPP_RET mpp_port_dequeue(MppPort port, MppTask *task)
{
    if (NULL == port || NULL == task) {
        mpp_err_f("invalid input port %p task %d\n", port, task);
        return MPP_ERR_UNKNOW;
    }

    MppPortImpl *p = (MppPortImpl *)port;
    AutoMutex auto_lock(p->lock);
    MppTaskStatus status_curr;
    MppTaskStatus status_next;

    switch (p->type) {
    case MPP_PORT_INPUT : {
        status_curr = MPP_EXTERNAL_QUEUE;
        status_next = MPP_EXTERNAL_HOLD;
    } break;
    case MPP_PORT_OUTPUT : {
        status_curr = MPP_INTERNAL_QUEUE;
        status_next = MPP_INTERNAL_HOLD;
    } break;
    default : {
        mpp_err_f("invalid queue type: %d\n", p->type);
        return MPP_NOK;
    } break;
    }

    MppTaskImpl *task_op = mpp_task_get_by_status(p, status_curr);
    if (NULL == task_op)
        return MPP_NOK;

    mpp_task_put_by_status(p, status_next, task_op);
    *task = (MppTask)task_op;
    return MPP_OK;
}

MPP_RET mpp_port_enqueue(MppPort port, MppTask task)
{
    if (NULL == port || NULL == task) {
        mpp_err_f("invalid input port %p task %d\n", port, task);
        return MPP_ERR_UNKNOW;
    }

    MppPortImpl *p = (MppPortImpl *)port;
    AutoMutex auto_lock(p->lock);
    MppTaskStatus status_curr;
    MppTaskStatus status_next;

    switch (p->type) {
    case MPP_PORT_INPUT : {
        status_curr = MPP_INTERNAL_HOLD;
        status_next = MPP_EXTERNAL_QUEUE;
    } break;
    case MPP_PORT_OUTPUT : {
        status_curr = MPP_EXTERNAL_HOLD;
        status_next = MPP_INTERNAL_QUEUE;
    } break;
    default : {
        mpp_err_f("invalid queue type: %d\n", p->type);
        return MPP_NOK;
    } break;
    }

    MppTaskImpl *task_op = (MppTaskImpl *)task;
    mpp_assert(task_op->status == status_curr);
    list_del_init(&task_op->list);

    mpp_task_put_by_status(p, status_next, task_op);
    return MPP_OK;
}

