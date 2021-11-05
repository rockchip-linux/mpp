/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __MPP_CLUSTER_H__
#define __MPP_CLUSTER_H__

#include "mpp_err.h"
#include "mpp_list.h"
#include "mpp_thread.h"
#include "mpp_dev_defs.h"

#define MAX_PRIORITY            1

typedef void* MppNode;

typedef MPP_RET (*TaskProc)(void *param);

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_node_init(MppNode *node);
MPP_RET mpp_node_deinit(MppNode node);

MPP_RET mpp_node_set_func(MppNode node, TaskProc proc, void *param);

MPP_RET mpp_node_attach(MppNode node, MppClientType type);
MPP_RET mpp_node_detach(MppNode node);

#define mpp_node_trigger(node, trigger) mpp_node_trigger_f(__FUNCTION__, node, trigger)

MPP_RET mpp_node_trigger_f(const char *caller, MppNode node, RK_S32 trigger);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_CLUSTER_H__*/
