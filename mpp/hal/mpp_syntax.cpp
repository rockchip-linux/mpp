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

typedef struct {
    struct list_head    list;
    Mutex               *lock;
    MppSyntaxNode       *node;
} MppSyntaxGroupImpl;

MPP_RET mpp_syntax_group_init(MppSyntaxGroup *group, RK_U32 count)
{
    MppSyntaxGroupImpl *p = mpp_malloc_size(MppSyntaxGroupImpl,
                            sizeof(MppSyntaxGroupImpl) + count * sizeof(MppSyntaxNode));
    if (NULL == p) {
        *group = NULL;
        mpp_err_f("malloc group failed\n");
        return MPP_NOK;
    }
    memset(p, 0, sizeof(*p));
    INIT_LIST_HEAD(&p->list);
    p->lock = new Mutex();
    p->node = (MppSyntaxNode*)(p+1);
    Mutex::Autolock auto_lock(p->lock);
    RK_U32 i;
    for (i = 0; i < count; i++) {
        list_add_tail(&p->node[i].list, &p->list);
    }
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

MPP_RET mpp_syntax_get_node(MppSyntaxGroup group, MppSyntaxNode **node)
{
    MppSyntaxGroupImpl *p = (MppSyntaxGroupImpl *)group;
    Mutex::Autolock auto_lock(p->lock);
    if (list_empty(&p->list)) {
        *node = NULL;
        return MPP_NOK;
    }

    struct list_head *list = p->list.next;
    list_del_init(list);
    *node = list_entry(list, MppSyntaxNode, list);
    return MPP_OK;
}

MPP_RET mpp_syntax_put_node(MppSyntaxGroup group, MppSyntaxNode *node)
{
    MppSyntaxGroupImpl *p = (MppSyntaxGroupImpl *)group;
    Mutex::Autolock auto_lock(p->lock);
    list_add_tail(&node->list, &p->list);
    return MPP_OK;
}

