/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "rc_data_base"

#include "mpp_mem.h"

#include "rc_data_base.h"

MPP_RET node_group_init(NodeGroup **grp, RK_S32 node_size, RK_S32 node_count)
{
    NodeGroup *p = mpp_malloc_size(NodeGroup, sizeof(NodeGroup) +
                                   node_size * node_count);

    if (p) {
        p->node_size = node_size;
        p->node_count = node_count;
        p->first = (void *)(p + 1);
        p->last = (RK_U8 *)p->first + node_size * (node_count - 1);
    }

    *grp = p;
    return MPP_OK;
}

MPP_RET node_group_deinit(NodeGroup *p)
{
    MPP_FREE(p);
    return MPP_OK;
}

void *node_group_get(NodeGroup *p, RK_S32 idx)
{
    RK_U8 *node = (RK_U8 *)p->first;

    return (void *)(node + idx * p->node_size);
}

MPP_RET node_group_check(NodeGroup *p, void *node)
{
    void *first = p->first;
    void *last = p->last;

    return (node >= first && node <= last) ? (MPP_OK) : (MPP_NOK);
}
