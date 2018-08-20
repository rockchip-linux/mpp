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

typedef enum HalTaskStatus_e {
    TASK_IDLE,
    TASK_PREPARE,
    TASK_WAIT_PROC,
    TASK_PROCESSING,
    TASK_PROC_DONE,
    TASK_BUTT,
} HalTaskStatus;

typedef struct IOInterruptCB {
    MPP_RET (*callBack)(void*, void*);
    void   *opaque;
} IOInterruptCB;

typedef struct IOCallbackCtx_t {
    RK_U32      device_id;
    void        *task;
    RK_U32      *regs;
    RK_U32       hard_err;
} IOCallbackCtx;

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

#define HAL_ENC_TASK_ERR_INIT         0x00000001
#define HAL_ENC_TASK_ERR_ALLOC        0x00000010
#define HAL_ENC_TASK_ERR_EXTRAINFO    0x00000100
#define HAL_ENC_TASK_ERR_GENREG       0x00001000
#define HAL_ENC_TASK_ERR_START        0x00010000
#define HAL_ENC_TASK_ERR_WAIT         0x00100000


typedef union HalDecTaskFlag_t {
    RK_U32          val;
    struct {
        RK_U32      eos              : 1;
        RK_U32      info_change      : 1;

        /*
         * Different error flags for task
         *
         * parse_err :
         * When set it means fatal error happened at parsing stage
         * This task should not enable hardware just output a empty frame with
         * error flag.
         *
         * ref_err :
         * When set it means current task is ok but it contains reference frame
         * with error which will introduce error pixels to this frame.
         *
         * used_for_ref :
         * When set it means this output frame will be used as reference frame
         * for further decoding. When there is error on decoding this frame
         * if used_for_ref is set then the frame will set errinfo flag
         * if used_for_ref is cleared then the frame will set discard flag.
         */
        RK_U32      parse_err        : 1;
        RK_U32      ref_err          : 1;
        RK_U32      used_for_ref     : 1;

        RK_U32      wait_done        : 1;
    };
} HalDecTaskFlag;

typedef struct HalEncTaskFlag_t {
    RK_U32 err;
} HalEncTaskFlag;

typedef struct HalDecTask_t {
    // set by parser to signal that it is valid
    RK_U32          valid;
    HalDecTaskFlag  flags;

    // previous task hardware working status
    // when hardware error happen status is not zero
    RK_U32          prev_status;
    // current tesk protocol syntax information
    MppSyntax       syntax;

    // packet need to be copied to hardware buffer
    // parser will create this packet and mpp_dec will copy it to hardware bufffer
    MppPacket       input_packet;

    // current task input slot index
    RK_S32          input;

    RK_S32          reg_index;
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

    // current tesk output stream buffer
    MppBuffer       output;
    RK_U32          length;

    // current tesk input slot buffer
    MppBuffer       input;

    // current mv info output buffer
    MppBuffer       mv_info;

    RK_U32          is_intra;

    HalEncTaskFlag  flags;

} HalEncTask;


typedef struct HalTask_u {
    union {
        HalDecTask  dec;
        HalEncTask  enc;
    };
} HalTaskInfo;

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
MPP_RET hal_task_group_init(HalTaskGroup *group, MppCtxType type, RK_S32 count);
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

