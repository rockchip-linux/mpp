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
    struct list_head    list;
    HalTaskGroupImpl    *group;
    RK_U32              used;
    HalTask             task;
};

struct HalTaskGroupImpl_t {
    struct list_head    list_unused;
    struct list_head    list_used;
    Mutex               *lock;
    MppCtxType          type;
    HalTaskImpl         *node;
};

static size_t get_task_size(HalTaskGroupImpl *group)
{
    return (group->type == MPP_CTX_DEC) ? (sizeof(HalDecTask)) : (sizeof(HalEncTask));
}

MPP_RET hal_task_group_init(HalTaskGroup *group, MppCtxType type, RK_U32 count)
{
    HalTaskGroupImpl *p = mpp_malloc_size(HalTaskGroupImpl,
                            sizeof(HalTaskGroupImpl) + count * sizeof(HalTaskImpl));
    if (NULL == p) {
        *group = NULL;
        mpp_err_f("malloc group failed\n");
        return MPP_NOK;
    }
    memset(p, 0, sizeof(*p));
    INIT_LIST_HEAD(&p->list_unused);
    INIT_LIST_HEAD(&p->list_used);
    p->lock = new Mutex();
    p->node = (HalTaskImpl*)(p+1);
    p->type = type;
    Mutex::Autolock auto_lock(p->lock);
    RK_U32 i;
    for (i = 0; i < count; i++) {
        p->node[i].group = p;
        list_add_tail(&p->node[i].list, &p->list_unused);
    }
    *group = p;
    return MPP_OK;
}

MPP_RET hal_task_group_deinit(HalTaskGroup group)
{
    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    if (p->lock) {
        delete p->lock;
        p->lock = NULL;
    }
    mpp_free(p);
    return MPP_OK;
}

MPP_RET hal_task_get_hnd(HalTaskGroup group, RK_U32 used, HalTaskHnd *hnd)
{
    HalTaskGroupImpl *p = (HalTaskGroupImpl *)group;
    Mutex::Autolock auto_lock(p->lock);
    struct list_head *head = (used) ? (&p->list_used) : (&p->list_unused);

    if (list_empty(head)) {
        *hnd = NULL;
        return MPP_NOK;
    }

    *hnd = list_entry(head->next, HalTaskImpl, list);
    return MPP_OK;
}

MPP_RET hal_task_set_used(HalTaskHnd hnd, RK_U32 used)
{
    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    HalTaskGroupImpl *group = impl->group;
    Mutex::Autolock auto_lock(group->lock);
    struct list_head *head = (used) ? (&group->list_used) : (&group->list_unused);
    list_del_init(&impl->list);
    list_add_tail(&impl->list, head);
    return MPP_OK;
}

MPP_RET hal_task_get_info(HalTaskHnd hnd, HalTask *task)
{
    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    memcpy(task, &impl->task, get_task_size(impl->group));
    return MPP_OK;
}

MPP_RET hal_task_set_info(HalTaskHnd hnd, HalTask *task)
{
    HalTaskImpl *impl = (HalTaskImpl *)hnd;
    memcpy(&impl->task, task, get_task_size(impl->group));
    return MPP_OK;
}

