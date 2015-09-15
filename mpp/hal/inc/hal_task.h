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

#include "rk_mpi.h"

#define MAX_DEC_REF_NUM     17

/*
 * modified by parser
 *
 * number   : the number of the data pointer array element
 * data     : the address of the pointer array, parser will add its data here
 */
typedef struct MppSyntax_t {
    RK_U32              number;
    void                *data;
} MppSyntax;

/*
 *  HalTask memory layout:
 *
 *  +----^----+ +----------------------+ +----^----+
 *       |      |     context type     |      |
 *       |      +----------------------+      |
 *       +      |      coding type     |      |
 *     header   +----------------------+      |
 *       +      |         size         |      |
 *       |      +----------------------+      |
 *       |      |     pointer count    |      |
 *  +----v----+ +----------------------+      |
 *              |                      |      |
 *              |       pointers       |      |
 *              |                      |      +
 *              +----------------------+    size
 *              |                      |      +
 *              |        data_0        |      |
 *              |                      |      |
 *              +----------------------+      |
 *              |                      |      |
 *              |        data_1        |      |
 *              |                      |      |
 *              +----------------------+      |
 *              |                      |      |
 *              |                      |      |
 *              |        data_2        |      |
 *              |                      |      |
 *              |                      |      |
 *              +----------------------+ +----v----+
 */
typedef struct HalDecTask_t {
    // set by parser to signal that it is valid
    RK_U32          valid;

    // current tesk protocol syntax information
    MppSyntax       syntax;

    // for test purpose
    // current tesk output slot index
    RK_S32          output;
    // current task reference slot index, -1 for unused
    RK_S32          refer[MAX_DEC_REF_NUM];
} HalDecTask;

typedef struct HalEncTask_t {
    RK_U32          valid;

    // current tesk protocol syntax information
    MppSyntax       syntax;

    // current tesk output stream buffer index
    RK_S32          output;

    // current tesk input slot buffer index
    RK_S32          input;
    // current task reference index, -1 for unused
    RK_S32          refer;
    // current task recon index
    RK_S32          recon;
} HalEncTask;

typedef union HalTask_u {
    HalDecTask      dec;
    HalEncTask      enc;
} HalTask;

typedef void* HalTaskHnd;
typedef void* HalTaskGroup;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * group init / deinit will be called by hal
 *
 * NOTE: use mpp_list to implement
 *       the count means the max task waiting for process
 */
MPP_RET hal_task_group_init(HalTaskGroup *group, MppCtxType type, RK_U32 count);
MPP_RET hal_task_group_deinit(HalTaskGroup group);

/*
 * normal working flow:
 *
 * dec:
 *
 * hal_task_can_put(group)      - dec test whether can send task to hal
 * parser->parse(task)          - parser write a local task
 * hal_task_put(group, task)    - dec send the task to hal
 *
 * hal:
 *
 * hal_task_can_get(group)      - hal test whether there is task waiting for process
 * hal_task_get(group, task)    - hal get the task to process
 *
 */
MPP_RET hal_task_can_put(HalTaskGroup group);
MPP_RET hal_task_can_get(HalTaskGroup group);

MPP_RET hal_task_init(HalTask *task, MppCtxType type);
MPP_RET hal_task_put(HalTaskGroup group, HalTask *task);
MPP_RET hal_task_get(HalTaskGroup group, HalTask *task);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_TASK__*/

