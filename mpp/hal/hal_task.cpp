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
#include "mpp_common.h"

#include "hal_task.h"

typedef struct HalTaskImpl_t        HalTaskImpl;
typedef struct HalTaskGroupImpl_t   HalTaskGroupImpl;

struct HalTaskImpl_t {
    struct list_head    list;
    HalTaskGroupImpl    *group;
    RK_S32              index;
    HalTaskStatus       status;
    void                *data;
};

struct HalTaskGroupImpl_t {
    MppCtxType          type;
    RK_S32              task_count;

    spinlock_t          lock;

    struct list_head    list[TASK_BUTT];
    RK_U32              count[TASK_BUTT];
    RK_S32              size;
    RK_S32              aligned_size;

    HalTaskImpl         tasks[];
};

MPP_RET hal_task_group_init(HalTaskGroup *group, RK_S32 count, RK_S32 size)
{
    if (NULL == group) {
        mpp_err_f("found invalid input group %p count %d\n", group, count);
        return MPP_ERR_UNKNOW;
    }

    HalTaskGroupImpl *p = NULL;
    RK_S32 aligned_size = MPP_ALIGN(size, sizeof(void *));
    RK_U8 *buf = NULL;

    do {
        p = mpp_calloc_size(HalTaskGroupImpl, sizeof(HalTaskGroupImpl) +
                            (sizeof(HalTaskImpl) + aligned_size) * count);
        if (NULL == p) {
            mpp_err_f("malloc group failed\n");
            break;
        }

        p->task_count = count;
        p->size = size;
        p->aligned_size = aligned_size;

        mpp_spinlock_init(&p->lock);

        for (RK_U32 i = 0; i < TASK_BUTT; i++)
            INIT_LIST_HEAD(&p->list[i]);

        buf = (RK_U8 *)(((HalTaskImpl *)(p + 1) + count));

        for (RK_S32 i = 0; i < count; i++) {
            HalTaskImpl *task = &p->tasks[i];

            INIT_LIST_HEAD(&task->list);
            task->index  = i;
            task->group  = p;
            task->status = TASK_IDLE;
            task->data   = buf + i * aligned_size;
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

MPP_RET hal_task_hnd_set_info(HalTaskHnd hnd, void *info)
{
    if (NULL == hnd || NULL == info) {
        mpp_err_f("found invaid input hnd %p info %p\n", hnd, info);
        return MPP_ERR_UNKNOW;
    }

    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;

    mpp_assert(impl->index < group->task_count);

    mpp_spinlock_lock(&group->lock);
    memcpy(impl->data, info, group->size);
    mpp_spinlock_unlock(&group->lock);

    return MPP_OK;
}

MPP_RET hal_task_hnd_get_info(HalTaskHnd hnd, void *info)
{
    if (NULL == hnd || NULL == info) {
        mpp_err_f("found invaid input hnd %p info %p\n", hnd, info);
        return MPP_ERR_UNKNOW;
    }

    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;

    mpp_assert(impl->index < group->task_count);

    mpp_spinlock_lock(&group->lock);
    memcpy(info, impl->data, group->size);
    mpp_spinlock_unlock(&group->lock);

    return MPP_OK;
}
