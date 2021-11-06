/*
*
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

#ifndef __HAL_TASK__
#define __HAL_TASK__

#include "mpp_err.h"
#include "hal_dec_task.h"

typedef union HalDecVprocTaskFlag_t {
    RK_U32          val;

    struct {
        RK_U32      eos              : 1;
        RK_U32      info_change      : 1;
    };
} HalDecVprocTaskFlag;

typedef struct HalDecVprocTask_t {
    // input slot index for post-process
    HalDecVprocTaskFlag     flags;

    RK_S32                  input;
} HalDecVprocTask;

typedef union HalTask_u {
    HalDecTask              dec;
    HalDecVprocTask         dec_vproc;
} HalTaskInfo;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * group init / deinit will be called by hal
 *
 * NOTE: use mpp_list to implement
 *       the count means the max task waiting for process
 */
MPP_RET hal_task_group_init(HalTaskGroup *group, RK_S32 count);
MPP_RET hal_task_group_deinit(HalTaskGroup group);

/*
 * normal working flow:
 *
 * dec:
 *
 * - codec
 * hal_task_get_hnd(group, idle, hnd)       - dec try get idle task to work
 * hal_task_hnd_set_status(hnd, prepare)    - dec prepare the task
 * codec prepare task
 * hal_task_hnd_set_status(hnd, wait_proc)  - dec send the task to hardware queue
 *
 * - hal
 * hal_task_get_hnd(group, wait_proc, hnd)  - hal get task on wait_proc status
 * hal start task
 * hal_task_set_hnd(hnd, processing)        - hal send task to hardware for process
 * hal wait task done
 * hal_task_set_hnd(hnd, proc_done)         - hal mark task is finished
 *
 * - codec
 * hal_task_get_hnd(group, task_done, hnd)  - codec query the previous finished task
 * codec do error process on task
 * hal_task_set_hnd(hnd, idle)              - codec mark task is idle
 *
 */
MPP_RET hal_task_get_hnd(HalTaskGroup group, HalTaskStatus status, HalTaskHnd *hnd);
MPP_RET hal_task_get_count(HalTaskGroup group, HalTaskStatus status, RK_U32 *count);
MPP_RET hal_task_hnd_set_status(HalTaskHnd hnd, HalTaskStatus status);
MPP_RET hal_task_hnd_set_info(HalTaskHnd hnd, HalTaskInfo *task);
MPP_RET hal_task_hnd_get_info(HalTaskHnd hnd, HalTaskInfo *task);
MPP_RET hal_task_info_init(HalTaskInfo *task, MppCtxType type);
MPP_RET hal_task_check_empty(HalTaskGroup group, HalTaskStatus status);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_TASK__*/

