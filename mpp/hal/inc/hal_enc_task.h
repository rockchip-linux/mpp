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

#ifndef __HAL_ENC_TASK__
#define __HAL_ENC_TASK__

#include "hal_task_defs.h"
#include "mpp_rc_defs.h"

#define HAL_ENC_TASK_ERR_INIT         0x00000001
#define HAL_ENC_TASK_ERR_ALLOC        0x00000010
#define HAL_ENC_TASK_ERR_EXTRAINFO    0x00000100
#define HAL_ENC_TASK_ERR_GENREG       0x00001000
#define HAL_ENC_TASK_ERR_START        0x00010000
#define HAL_ENC_TASK_ERR_WAIT         0x00100000

typedef struct HalEncTaskFlag_t {
    RK_U32          err;
} HalEncTaskFlag;

typedef struct HalEncTask_t {
    RK_U32          valid;

    // rate control data channel
    EncRcTask       *rc_task;

    // current tesk protocol syntax information
    MppSyntax       syntax;
    MppSyntax       hal_ret;

    // current tesk output stream buffer
    MppPacket       packet;
    MppBuffer       output;
    RK_U32          header_length;
    RK_U32          sei_length;
    RK_U32          hw_length;
    RK_U32          length;

    // current tesk input slot buffer
    MppFrame        frame;
    MppBuffer       input;

    // current mv info output buffer
    MppBuffer       mv_info;

    RK_U32          is_intra;
    RK_S32          temporal_id;

    HalEncTaskFlag  flags;
} HalEncTask;

#endif /* __HAL_ENC_TASK__ */
