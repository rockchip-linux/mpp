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

#include "hal_task.h"
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
    RK_S32          drop_by_fps;
    RK_S32          reg_idx;
    /* hal buf index */
    RK_S32          curr_idx;
    RK_S32          refr_idx;
} HalEncTaskFlag;

typedef struct MppSyntax_t {
    RK_U32              number;
    void                *data;
} MppSyntax;

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

    // current md info output buffer
    MppBuffer       md_info;

    // low delay mode part output information
    RK_U32          part_first;
    RK_U32          part_last;
    RK_U32          part_count;
    RK_U8           *part_pos;
    size_t          part_length;

    HalEncTaskFlag  flags;
} HalEncTask;

/* encoder internal work flow */
typedef union EncAsyncStatus_u {
    RK_U32          val;
    struct {
        RK_U32      task_hnd_rdy        : 1;
        RK_U32      task_in_rdy         : 1;
        RK_U32      task_out_rdy        : 1;

        RK_U32      frm_pkt_rdy         : 1;

        RK_U32      hal_task_reset_rdy  : 1;    // reset hal task to start
        RK_U32      rc_check_frm_drop   : 1;    // rc  stage
        RK_U32      pkt_buf_rdy         : 1;    // prepare pkt buf

        RK_U32      enc_start           : 1;    // enc stage
        RK_U32      refs_force_update   : 1;    // enc stage
        RK_U32      low_delay_again     : 1;    // enc stage low delay output again

        RK_U32      enc_backup          : 1;    // enc stage
        RK_U32      enc_restore         : 1;    // reenc flow start point
        RK_U32      enc_proc_dpb        : 1;    // enc stage
        RK_U32      rc_frm_start        : 1;    // rc  stage
        RK_U32      check_type_reenc    : 1;    // flow checkpoint if reenc -> enc_restore
        RK_U32      enc_proc_hal        : 1;    // enc stage
        RK_U32      hal_get_task        : 1;    // hal stage
        RK_U32      rc_hal_start        : 1;    // rc  stage
        RK_U32      hal_gen_reg         : 1;    // hal stage
        RK_U32      hal_start           : 1;    // hal stage
        RK_U32      hal_wait            : 1;    // hal stage NOTE: special in low delay mode
        RK_U32      rc_hal_end          : 1;    // rc  stage
        RK_U32      hal_ret_task        : 1;    // hal stage
        RK_U32      enc_update_hal      : 1;    // enc stage
        RK_U32      rc_frm_end          : 1;    // rc  stage
        RK_U32      check_rc_reenc      : 1;    // flow checkpoint if reenc -> enc_restore
        RK_U32      enc_done            : 1;    // done stage
        RK_U32      slice_out_done      : 1;
    };
} EncAsyncStatus;

typedef struct EncAsyncTaskInfo_t {
    RK_S32              seq_idx;
    EncAsyncStatus      status;
    RK_S64              pts;

    HalEncTask          task;
    EncRcTask           rc;
    MppEncRefFrmUsrCfg  usr;
} EncAsyncTaskInfo;

#endif /* __HAL_ENC_TASK__ */
