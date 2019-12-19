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

#ifndef __RC_DATA_BASE_H__
#define __RC_DATA_BASE_H__

#include "rk_type.h"
#include "mpp_err.h"

/*
 * cyclic fifo buffer management
 * Not use new [] for memory management with mpp_malloc/mpp_free
 */
typedef struct NodeGroup_t {
    /* size of the whole data node in byte */
    RK_S32              node_size;

    /* max number of node */
    RK_S32              node_count;

    void                *first;
    void                *last;
} NodeGroup;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET node_group_init(NodeGroup **grp, RK_S32 node_size, RK_S32 node_count);
MPP_RET node_group_deinit(NodeGroup *p);

void *node_group_get(NodeGroup *p, RK_S32 idx);
MPP_RET node_group_check(NodeGroup *p, void *node);

#ifdef __cplusplus
}
#endif

#endif /* __RC_DATA_BASE_H__ */
