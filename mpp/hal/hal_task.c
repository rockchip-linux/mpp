/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_task"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "hal_task.h"

typedef struct HalTaskImpl_t        HalTaskImpl;
typedef struct HalTaskGroupImpl_t   HalTaskGroupImpl;

struct HalTaskImpl_t {
    struct list_head    list;
    HalTaskGroupImpl    *group;
    RK_S32              index;
    RK_S32              status;
    void                *data;
};

struct HalTaskGroupImpl_t {
    RK_S32              stage_count;
    RK_S32              task_count;

    spinlock_t          lock;

    RK_S32              size;
    RK_S32              aligned_size;

    struct list_head    *list;
    RK_U32              *count;
    HalTaskImpl         *tasks;
};

MPP_RET hal_task_group_init(HalTaskGroup *group, RK_S32 stage_cnt, RK_S32 task_cnt, RK_S32 task_size)
{
    if (NULL == group || stage_cnt < 0 || task_cnt < 0 || task_size < 0) {
        mpp_err_f("found invalid input group %p stage %d task %d size %d\n",
                  group, stage_cnt, task_cnt, task_size);
        return MPP_ERR_UNKNOW;
    }

    HalTaskGroupImpl *p = NULL;
    RK_S32 aligned_size = MPP_ALIGN(task_size, sizeof(void *));

    do {
        RK_U8 *buf = NULL;
        RK_S32 i;

        p = mpp_calloc_size(HalTaskGroupImpl, sizeof(HalTaskGroupImpl) +
                            (sizeof(HalTaskImpl) + aligned_size) * task_cnt +
                            (sizeof(struct list_head)) * stage_cnt +
                            (sizeof(RK_U32)) * stage_cnt);
        if (NULL == p) {
            mpp_err_f("malloc group failed\n");
            break;
        }

        p->stage_count = stage_cnt;
        p->task_count = task_cnt;
        p->size = task_size;
        p->aligned_size = aligned_size;
        p->list = (struct list_head *)((HalTaskImpl *)(p + 1));
        p->count = (RK_U32 *)(p->list + stage_cnt);
        p->tasks = (HalTaskImpl *)(p->count + stage_cnt);

        mpp_spinlock_init(&p->lock);

        for (i = 0; i < stage_cnt; i++)
            INIT_LIST_HEAD(&p->list[i]);

        buf = (RK_U8 *)(p->tasks + task_cnt);

        for (i = 0; i < task_cnt; i++) {
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

MPP_RET hal_task_get_hnd(HalTaskGroup group, RK_S32 status, HalTaskHnd *hnd)
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

MPP_RET hal_task_check_empty(HalTaskGroup group, RK_S32 status)
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

RK_S32 hal_task_get_count(HalTaskGroup group, RK_S32 status)
{
    if (NULL == group || status >= TASK_BUTT) {
        mpp_err_f("found invaid input group %p status %d\n", group, status);
        return -1;
    }

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    RK_S32 count;

    mpp_spinlock_lock(&p->lock);
    count = p->count[status];
    mpp_spinlock_unlock(&p->lock);

    return count;
}

MPP_RET hal_task_hnd_set_status(HalTaskHnd hnd, RK_S32 status)
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

void *hal_task_hnd_get_data(HalTaskHnd hnd)
{
    if (NULL == hnd) {
        mpp_err_f("found invaid input hnd %p\n", hnd);
        return NULL;
    }

    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;

    mpp_assert(impl->index < group->task_count);

    return impl->data;
}
