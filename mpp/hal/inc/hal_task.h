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
 */
MPP_RET hal_task_group_init(HalTaskGroup *group, MppCtxType type, RK_U32 count);
MPP_RET hal_task_group_deinit(HalTaskGroup group);

/*
 * normal working flow:
 *
 * dec:
 *
 * get_hnd(group, 0, &hnd)      - dec get a unused handle first
 * parser->parse                - parser write a local info in dec
 * set_info(hnd, info)          - dec write the local info to handle
 * set_used(hnd, 1)             - decoder set handle to used
 *
 * hal:
 * get_hnd(group, 1, &hnd)
 * read_info(hnd, info)
 * set_used(hnd, 0)
 *
 * these calls do not own syntax handle but just get its reference
 * so there is not need to free or destory the handle
 *
 */
MPP_RET hal_task_get_hnd(HalTaskGroup group, RK_U32 used, HalTaskHnd *hnd);
MPP_RET hal_task_set_used(HalTaskHnd hnd, RK_U32 used);

MPP_RET hal_task_get_info(HalTaskHnd hnd, HalTask *task);
MPP_RET hal_task_set_info(HalTaskHnd hnd, HalTask *task);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_TASK__*/

