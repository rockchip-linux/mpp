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

#ifndef __MPP_TASK_IMPL_H__
#define __MPP_TASK_IMPL_H__

#include "mpp_list.h"
#include "mpp_meta.h"

/*
 * mpp task status transaction
 *
 * mpp task is generated from mpp port. When create a mpp port the corresponding task
 * will be created, too. Then external user will dequeue task from port and enqueue to
 * mpp port for process.
 *
 * input port task work flow:
 *
 * description                      |   call            | status transaction
 * 1. input port init               | enqueue(external) |                   -> external_queue
 * 2. input port user dequeue       | dequeue(external) | external_queue    -> external_hold
 * 3. user setup task for processing|                   |
 * 4. input port user enqueue       | enqueue(external) | external_hold     -> internal_queue
 * 5. input port mpp start process  | dequeue(internal) | internal_queue    -> internal_hold
 * 6. mpp process task              |                   |
 * 7. input port mpp task finish    | enqueue(internal) | internal_hold     -> external_queue
 * loop to 2
 *
 * output port task work flow:
 * description                      |   call            | status transaction
 * 1. output port init              | enqueue(internal) |                   -> internal_queue
 * 2. output port mpp task dequeue  | dequeue(internal) | internal_queue    -> internal_hold
 * 3. mpp setup task by processed frame/packet          |
 * 4. output port mpp task enqueue  | enqueue(internal) | internal_hold     -> external_queue
 * 5. output port user task dequeue | dequeue(external) | external_queue    -> external_hold
 * 6. user get task as output       |                   |
 * 7. output port user release task | enqueue(external) | external_hold     -> external_queue
 * loop to 2
 *
 */
typedef enum MppTaskStatus_e {
    MPP_EXTERNAL_QUEUE,         /* in external queue and ready for external dequeue */
    MPP_EXTERNAL_HOLD,          /* dequeued and hold by external user, user will config */
    MPP_INTERNAL_QUEUE,         /* in mpp internal work queue and ready for mpp dequeue */
    MPP_INTERNAL_HOLD,          /* dequeued and hold by mpp internal worker, mpp is processing */
    MPP_TASK_BUTT,
} MppTaskStatus;

typedef void* MppPort;

typedef struct MppTaskImpl_t {
    const char          *name;
    struct list_head    list;
    MppPort             *port;
    RK_S32              index;
    MppTaskStatus       status;

    MppMeta             meta;
} MppTaskImpl;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET check_mpp_task_name(MppTask task);

/*
 * mpp_port_init:
 * initialize port with task count and initial status
 * group        - return port pointer
 * type         - initial queue for all tasks
 * task_count   - total task count for this task group
 */
MPP_RET mpp_port_init(MppPort *port, MppPortType type, RK_S32 task_count);
MPP_RET mpp_port_deinit(MppPort port);

MPP_RET mpp_port_can_dequeue(MppPort port);
MPP_RET mpp_port_can_enqueue(MppPort port);

MPP_RET mpp_port_dequeue(MppPort port, MppTask *task);
MPP_RET mpp_port_enqueue(MppPort port, MppTask task);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TASK_IMPL_H__*/
