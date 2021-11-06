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

#define MODULE_TAG "hal_task"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_list.h"
#include "mpp_lock.h"

#include "hal_task.h"

typedef struct HalTaskImpl_t        HalTaskImpl;
typedef struct HalTaskGroupImpl_t   HalTaskGroupImpl;

struct HalTaskImpl_t {
    struct list_head    list;
    HalTaskGroupImpl    *group;
    RK_S32              index;
    HalTaskStatus       status;
    HalTaskInfo         task;
};

struct HalTaskGroupImpl_t {
    MppCtxType          type;
    RK_S32              task_count;

    spinlock_t          lock;

    struct list_head    list[TASK_BUTT];
    RK_U32              count[TASK_BUTT];

    HalTaskImpl         tasks[];
};

MPP_RET hal_task_group_init(HalTaskGroup *group, RK_S32 count)
{
    if (NULL == group) {
        mpp_err_f("found invalid input group %p count %d\n", group, count);
        return MPP_ERR_UNKNOW;
    }

    HalTaskGroupImpl *p = NULL;

    do {
        p = mpp_calloc_size(HalTaskGroupImpl, sizeof(HalTaskGroupImpl) +
                            sizeof(HalTaskImpl) * count);
        if (NULL == p) {
            mpp_err_f("malloc group failed\n");
            break;
        }

        p->task_count = count;
        mpp_spinlock_init(&p->lock);

        for (RK_U32 i = 0; i < TASK_BUTT; i++)
            INIT_LIST_HEAD(&p->list[i]);

        for (RK_S32 i = 0; i < count; i++) {
            HalTaskImpl *task = &p->tasks[i];

            INIT_LIST_HEAD(&task->list);
            task->index  = i;
            task->group  = p;
            task->status = TASK_IDLE;
            list_add_tail(&task->list, &p->list[TASK_IDLE]);
            p->count[TASK_IDLE]++;
        }
        *group = p;
        return MPP_OK;
    } while (0);

    MPP_FREE(p);

    *group = NULL;
    return MPP_NOK;
}

MPP_RET hal_task_group_deinit(HalTaskGroup group)
{
    MPP_FREE(group);
    return MPP_OK;
}

MPP_RET hal_task_get_hnd(HalTaskGroup group, HalTaskStatus status, HalTaskHnd *hnd)
{
    if (NULL == group || status >= TASK_BUTT || NULL == hnd) {
        mpp_err_f("found invaid input group %p status %d hnd %p\n", group, status, hnd);
        return MPP_ERR_UNKNOW;
    }

    *hnd = NULL;
    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    struct list_head *list = &p->list[status];

    mpp_spinlock_lock(&p->lock);
    if (list_empty(list)) {
        mpp_spinlock_unlock(&p->lock);
        return MPP_NOK;
    }

    HalTaskImpl *task = list_entry(list->next, HalTaskImpl, list);
    mpp_assert(task->status == status);
    *hnd = task;
    mpp_spinlock_unlock(&p->lock);
    return MPP_OK;
}

MPP_RET hal_task_check_empty(HalTaskGroup group, HalTaskStatus status)
{
    if (NULL == group || status >= TASK_BUTT) {
        mpp_err_f("found invaid input group %p status %d \n", group, status);
        return MPP_ERR_UNKNOW;
    }

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    struct list_head *list = &p->list[status];
    MPP_RET ret;

    mpp_spinlock_lock(&p->lock);
    ret = list_empty(list) ? MPP_OK : MPP_NOK;
    mpp_spinlock_unlock(&p->lock);

    return ret;
}
MPP_RET hal_task_get_count(HalTaskGroup group, HalTaskStatus status, RK_U32 *count)
{
    if (NULL == group || status >= TASK_BUTT || NULL == count) {
        mpp_err_f("found invaid input group %p status %d count %p\n", group, status, count);
        return MPP_ERR_UNKNOW;
    }

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;

    mpp_spinlock_lock(&p->lock);
    *count = p->count[status];
    mpp_spinlock_unlock(&p->lock);

    return MPP_OK;
}

MPP_RET hal_task_hnd_set_status(HalTaskHnd hnd, HalTaskStatus status)
{
    if (NULL == hnd || status >= TASK_BUTT) {
        mpp_err_f("found invaid input hnd %p status %d\n", hnd, status);
        return MPP_ERR_UNKNOW;
    }

    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;

    mpp_assert(group);
    mpp_assert(impl->index < group->task_count);

    mpp_spinlock_lock(&group->lock);
    list_del_init(&impl->list);
    list_add_tail(&impl->list, &group->list[status]);
    group->count[impl->status]--;
    group->count[status]++;
    impl->status = status;
    mpp_spinlock_unlock(&group->lock);

    return MPP_OK;
}

MPP_RET hal_task_hnd_set_info(HalTaskHnd hnd, HalTaskInfo *task)
{
    if (NULL == hnd || NULL == task) {
        mpp_err_f("found invaid input hnd %p task %p\n", hnd, task);
        return MPP_ERR_UNKNOW;
    }

    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;

    mpp_assert(impl->index < group->task_count);

    mpp_spinlock_lock(&group->lock);
    memcpy(&impl->task, task, sizeof(impl->task));
    mpp_spinlock_unlock(&group->lock);

    return MPP_OK;
}

MPP_RET hal_task_hnd_get_info(HalTaskHnd hnd, HalTaskInfo *task)
{
    if (NULL == hnd || NULL == task) {
        mpp_err_f("found invaid input hnd %p task %p\n", hnd, task);
        return MPP_ERR_UNKNOW;
    }

    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;

    mpp_assert(impl->index < group->task_count);

    mpp_spinlock_lock(&group->lock);
    memcpy(task, &impl->task, sizeof(impl->task));
    mpp_spinlock_unlock(&group->lock);

    return MPP_OK;
}

MPP_RET hal_task_info_init(HalTaskInfo *task, MppCtxType type)
{
    if (NULL == task || type >= MPP_CTX_BUTT) {
        mpp_err_f("found invalid input task %p type %d\n", task, type);
        return MPP_ERR_UNKNOW;
    }

    HalDecTask *p = &task->dec;

    p->valid  = 0;
    p->flags.val = 0;
    p->flags.eos = 0;
    p->input_packet = NULL;
    p->output = -1;
    p->input = -1;
    memset(&task->dec.syntax, 0, sizeof(task->dec.syntax));
    memset(task->dec.refer, -1, sizeof(task->dec.refer));

    return MPP_OK;
}
