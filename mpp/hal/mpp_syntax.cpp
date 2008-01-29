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
#define MODULE_TAG "mpp_syntax"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_thread.h"

#include "mpp_syntax.h"

typedef struct MppSyntaxImpl_t      MppSyntaxImpl;
typedef struct MppSyntaxGroupImpl_t MppSyntaxGroupImpl;

struct MppSyntaxImpl_t {
    struct list_head    list;
    MppSyntaxGroupImpl  *group;
    RK_U32              used;
    MppSyntax           syntax;
};

struct MppSyntaxGroupImpl_t {
    struct list_head    list_unused;
    struct list_head    list_used;
    Mutex               *lock;
    MppSyntaxImpl       *node;
};

MPP_RET mpp_syntax_group_init(MppSyntaxGroup *group, RK_U32 count)
{
    MppSyntaxGroupImpl *p = mpp_malloc_size(MppSyntaxGroupImpl,
                            sizeof(MppSyntaxGroupImpl) + count * sizeof(MppSyntaxImpl));
    if (NULL == p) {
        *group = NULL;
        mpp_err_f("malloc group failed\n");
        return MPP_NOK;
    }
    memset(p, 0, sizeof(*p));
    INIT_LIST_HEAD(&p->list_unused);
    INIT_LIST_HEAD(&p->list_used);
    p->lock = new Mutex();
    p->node = (MppSyntaxImpl*)(p+1);
    Mutex::Autolock auto_lock(p->lock);
    RK_U32 i;
    for (i = 0; i < count; i++) {
        p->node[i].group = p;
        list_add_tail(&p->node[i].list, &p->list_unused);
    }
    *group = p;
    return MPP_OK;
}

MPP_RET mpp_syntax_group_deinit(MppSyntaxGroup group)
{
    MppSyntaxGroupImpl *p = (MppSyntaxGroupImpl *)group;
    if (p->lock) {
        delete p->lock;
        p->lock = NULL;
    }
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_syntax_get_hnd(MppSyntaxGroup group, RK_U32 used, MppSyntaxHnd *hnd)
{
    MppSyntaxGroupImpl *p = (MppSyntaxGroupImpl *)group;
    Mutex::Autolock auto_lock(p->lock);
    struct list_head *head = (used) ? (&p->list_used) : (&p->list_unused);

    if (list_empty(head)) {
        *hnd = NULL;
        return MPP_NOK;
    }

    *hnd = list_entry(head->next, MppSyntaxImpl, list);
    return MPP_OK;
}

MPP_RET mpp_syntax_set_used(MppSyntaxHnd hnd, RK_U32 used)
{
    MppSyntaxImpl *impl = (MppSyntaxImpl *)hnd;
    MppSyntaxGroupImpl *group = impl->group;
    Mutex::Autolock auto_lock(group->lock);
    struct list_head *head = (used) ? (&group->list_used) : (&group->list_unused);
    list_del_init(&impl->list);
    list_add_tail(&impl->list, head);
    return MPP_OK;
}

MPP_RET mpp_syntax_get_info(MppSyntaxHnd hnd, MppSyntax *syntax)
{
    MppSyntaxImpl *impl = (MppSyntaxImpl *)hnd;
    memcpy(syntax, &impl->syntax, sizeof(impl->syntax));
    return MPP_OK;
}

MPP_RET mpp_syntax_set_info(MppSyntaxHnd hnd, MppSyntax *syntax)
{
    MppSyntaxImpl *impl = (MppSyntaxImpl *)hnd;
    memcpy(&impl->syntax, syntax, sizeof(impl->syntax));
    return MPP_OK;
}

