/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#include "hal_task.h"

typedef struct HalTaskImpl_t        HalTaskImpl;
typedef struct HalTaskGroupImpl_t   HalTaskGroupImpl;

struct HalTaskImpl_t {
    RK_U32              index;
    HalTask             task;
};

struct HalTaskGroupImpl_t {
    RK_U32              count_put;
    RK_U32              count_get;
    MppCtxType          type;
    RK_S32              count;
    mpp_list            *tasks;
};

MPP_RET hal_task_group_init(HalTaskGroup *group, MppCtxType type, RK_U32 count)
{
    if (NULL == group || 0 == count) {
        mpp_err_f("found invalid input group %p count %d\n", group, count);
        return MPP_ERR_UNKNOW;
    }

    *group = NULL;
    HalTaskGroupImpl *p = mpp_malloc(HalTaskGroupImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc group failed\n");
        return MPP_NOK;
    }
    p->tasks = new mpp_list(NULL);
    if (NULL == p->tasks) {
        mpp_err_f("malloc task list failed\n");
        mpp_free(p);
        return MPP_NOK;
    }
    p->type  = type;
    p->count = count - 1;
    p->count_put = p->count_get = 0;
    *group = p;
    return MPP_OK;
}

MPP_RET hal_task_group_deinit(HalTaskGroup group)
{
    if (NULL == group) {
        mpp_err_f("found NULL input group\n");
        return MPP_ERR_NULL_PTR;
    }

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    if (p->tasks)
        delete p->tasks;
    mpp_free(p);
    return MPP_OK;
}

MPP_RET hal_task_can_put(HalTaskGroup group)
{
    if (NULL == group) {
        mpp_err_f("found NULL input group\n");
        return MPP_ERR_NULL_PTR;
    }

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    mpp_list *tasks = p->tasks;
    Mutex::Autolock auto_lock(tasks->mutex());
    return (tasks->list_size() < p->count) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET hal_task_can_get(HalTaskGroup group)
{
    if (NULL == group) {
        mpp_err_f("found NULL input group\n");
        return MPP_ERR_NULL_PTR;
    }

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    mpp_list *tasks = p->tasks;
    Mutex::Autolock auto_lock(tasks->mutex());
    return (tasks->list_size()) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET hal_task_put(HalTaskGroup group, HalTask *task)
{
    MPP_RET ret = hal_task_can_put(group);
    mpp_assert(ret == MPP_OK);

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    mpp_list *tasks = p->tasks;
    Mutex::Autolock auto_lock(tasks->mutex());
    tasks->add_at_tail(task, sizeof(*task));
    p->count_put++;
    return MPP_OK;
}

MPP_RET hal_task_get(HalTaskGroup group, HalTask *task)
{
    MPP_RET ret = hal_task_can_get(group);
    mpp_assert(ret == MPP_OK);

    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    mpp_list *tasks = p->tasks;
    Mutex::Autolock auto_lock(tasks->mutex());
    tasks->del_at_head(task, sizeof(*task));
    p->count_get++;
    return MPP_OK;
}

