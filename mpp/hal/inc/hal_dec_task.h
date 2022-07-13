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

#ifndef __HAL_DEC_TASK__
#define __HAL_DEC_TASK__

#include "hal_task.h"
#include "mpp_callback.h"

#define MAX_DEC_REF_NUM     17

/*
 * modified by parser and encoder
 *
 * number   : the number of the data pointer array element
 * data     : the address of the pointer array, parser will add its data here
 */
typedef struct MppSyntax_t {
    RK_U32              number;
    void                *data;
} MppSyntax;

typedef union HalDecTaskFlag_t {
    RK_U64          val;
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
        RK_U32      reserved0        : 2;
        RK_U32      ref_miss         : 8;
        RK_U32      ref_used         : 8;
    };
} HalDecTaskFlag;

typedef struct HalDecTask_t {
    // set by parser to signal that it is valid
    RK_U32          valid;
    HalDecTaskFlag  flags;

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

#endif /* __HAL_DEC_TASK__ */
