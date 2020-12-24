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

#include "mpp_time.h"

#include "hal_task_defs.h"
#include "mpp_rc_defs.h"
#include "mpp_enc_refs.h"

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

    // cpb reference force config
    MppEncRefFrmUsrCfg  *frm_cfg;

    // current tesk protocol syntax information
    MppSyntax       syntax;
    MppSyntax       hal_ret;

    /*
     * Current tesk output stream buffer
     *
     * Usage and flow of changing task length and packet length
     *
     * 1. length is runtime updated for each stage.
     *    header_length / sei_length / hw_length are for recording.
     *
     * 2. When writing vps/sps/pps encoder should update length.
     *    Then length will be kept before next stage is done.
     *    For example when vps/sps/pps were inserted and slice data need
     *    reencoding the hal should update length at the final loop.
     *
     * 3. length in task and length in packet should be updated at the same
     *    time. Encoder flow need to check these two length between stages.
     */
    MppPacket       packet;
    MppBuffer       output;
    RK_S32          header_length;
    RK_S32          sei_length;
    RK_S32          hw_length;
    RK_U32          length;

    // current tesk input slot buffer
    MppFrame        frame;
    MppBuffer       input;

    // task stopwatch for timing
    MppStopwatch    stopwatch;

    // current mv info output buffer (not used)
    MppBuffer       mv_info;

    // low delay mode part output information
    RK_U32          part_first;
    RK_U32          part_last;
    RK_U32          part_count;
    RK_U8           *part_pos;
    size_t          part_length;

    HalEncTaskFlag  flags;
} HalEncTask;

#endif /* __HAL_ENC_TASK__ */
