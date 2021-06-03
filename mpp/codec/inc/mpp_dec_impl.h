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

#ifndef __MPP_DEC_IMPL_H__
#define __MPP_DEC_IMPL_H__

#include "mpp_time.h"
#include "hal_info.h"

#include "mpp.h"
#include "mpp_dec_cfg.h"
#include "mpp_callback.h"

#include "mpp_parser.h"
#include "mpp_hal.h"

// for timing record
typedef enum MppDecTimingType_e {
    DEC_PRS_TOTAL,
    DEC_PRS_WAIT,
    DEC_PRS_PROC,
    DEC_PRS_PREPARE,
    DEC_PRS_PARSE,
    DEC_HAL_GEN_REG,
    DEC_HW_START,

    DEC_HAL_TOTAL,
    DEC_HAL_WAIT,
    DEC_HAL_PROC,
    DEC_HW_WAIT,
    DEC_TIMING_BUTT,
} MppDecTimingType;

typedef struct MppDecImpl_t {
    MppCodingType       coding;

    Parser              parser;
    MppHal              hal;

    // worker thread
    MppThread           *thread_parser;
    MppThread           *thread_hal;

    // common resource
    MppBufSlots         frame_slots;
    MppBufSlots         packet_slots;
    MppCbCtx            dec_cb;
    const MppDecHwCap   *hw_info;
    MppDev              dev;
    HalInfo             hal_info;
    RK_U32              info_updated;

    HalTaskGroup        tasks;
    HalTaskGroup        vproc_tasks;

    // runtime configure set
    MppDecCfgSet        cfg;

    /* control process */
    Mutex               *cmd_lock;
    RK_U32              cmd_send;
    RK_U32              cmd_recv;
    MpiCmd              cmd;
    void                *param;
    MPP_RET             *cmd_ret;
    sem_t               cmd_start;
    sem_t               cmd_done;

    // status flags
    RK_U32              parser_work_count;
    RK_U32              parser_wait_count;
    RK_U32              parser_status_flag;
    RK_U32              parser_wait_flag;
    RK_U32              parser_notify_flag;
    RK_U32              hal_notify_flag;

    // reset process:
    // 1. mpp_dec set reset flag and signal parser
    // 2. mpp_dec wait on parser_reset sem
    // 3. parser wait hal reset done
    // 4. hal wait vproc reset done
    // 5. vproc do reset and signal hal
    // 6. hal do reset and signal parser
    // 7. parser do reset and signal mpp_dec
    // 8. mpp_dec reset done
    RK_U32              reset_flag;

    RK_U32              hal_reset_post;
    RK_U32              hal_reset_done;
    sem_t               parser_reset;
    sem_t               hal_reset;

    // work mode flags
    RK_U32              parser_fast_mode;
    RK_U32              disable_error;
    RK_U32              enable_deinterlace;
    RK_U32              batch_mode;

    // dec parser thread runtime resource context
    MppPacket           mpp_pkt_in;
    void                *mpp;
    void                *vproc;

    // statistics data
    RK_U32              statistics_en;
    MppClock            clocks[DEC_TIMING_BUTT];

    // query data
    RK_U32              dec_in_pkt_count;
    RK_U32              dec_hw_run_count;
    RK_U32              dec_out_frame_count;
} MppDecImpl;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /*__MPP_DEC_IMPL_H__*/
