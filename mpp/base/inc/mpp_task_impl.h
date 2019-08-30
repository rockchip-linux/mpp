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
#include "mpp_task.h"

typedef void* MppPort;
typedef void* MppTaskQueue;

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
    MPP_INPUT_PORT,             /* in external queue and ready for external dequeue */
    MPP_INPUT_HOLD,             /* dequeued and hold by external user, user will config */
    MPP_OUTPUT_PORT,            /* in mpp internal work queue and ready for mpp dequeue */
    MPP_OUTPUT_HOLD,            /* dequeued and hold by mpp internal worker, mpp is processing */
    MPP_TASK_STATUS_BUTT,
} MppTaskStatus;

typedef struct MppTaskImpl_t {
    const char          *name;
    struct list_head    list;
    MppTaskQueue        queue;
    RK_S32              index;
    MppTaskStatus       status;

    MppMeta             meta;
} MppTaskImpl;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET check_mpp_task_name(MppTask task);

/*
 * Mpp task queue function:
 *
 * mpp_task_queue_init      - create  task queue structure
 * mpp_task_queue_deinit    - destory task queue structure
 * mpp_task_queue_get_port  - return input or output port of task queue
 *
 * Typical work flow, task mpp_dec for example:
 *
 * 1. Mpp layer creates one task queue in order to connect mpp input and mpp_dec input.
 * 2. Mpp layer setups the task count in task queue input port.
 * 3. Get input port from the task queue and assign to mpp input as mpp_input_port.
 * 4. Get output port from the task queue and assign to mpp_dec input as dec_input_port.
 * 5. Let the loop start.
 *    a. mpi user will dequeue task from mpp_input_port.
 *    b. mpi user will setup task.
 *    c. mpi user will enqueue task back to mpp_input_port.
 *    d. task will automatically transfer to dec_input_port.
 *    e. mpp_dec will dequeue task from dec_input_port.
 *    f. mpp_dec will process task.
 *    g. mpp_dec will enqueue task back to dec_input_port.
 *    h. task will automatically transfer to mpp_input_port.
 * 6. Stop the loop. All tasks must be return to input port with idle status.
 * 6. Mpp layer destory the task queue.
 */
MPP_RET mpp_task_queue_init(MppTaskQueue *queue);
MPP_RET mpp_task_queue_setup(MppTaskQueue queue, RK_S32 task_count);
MPP_RET mpp_task_queue_deinit(MppTaskQueue queue);
MppPort mpp_task_queue_get_port(MppTaskQueue queue, MppPortType type);

#define mpp_port_poll(port, timeout) _mpp_port_poll(__FUNCTION__, port, timeout)
#define mpp_port_dequeue(port, task) _mpp_port_dequeue(__FUNCTION__, port, task)
#define mpp_port_enqueue(port, task) _mpp_port_enqueue(__FUNCTION__, port, task)
#define mpp_port_awake(port) _mpp_port_awake(__FUNCTION__, port)

MPP_RET _mpp_port_poll(const char *caller, MppPort port, MppPollType timeout);
MPP_RET _mpp_port_dequeue(const char *caller, MppPort port, MppTask *task);
MPP_RET _mpp_port_enqueue(const char *caller, MppPort port, MppTask task);
MPP_RET _mpp_port_awake(const char *caller, MppPort port);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TASK_IMPL_H__*/
