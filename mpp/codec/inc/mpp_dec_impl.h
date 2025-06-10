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
#include "mpp_mem_pool.h"
#include "mpp_lock.h"
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


typedef enum MppDecMode_e {
    MPP_DEC_MODE_DEFAULT,
    MPP_DEC_MODE_NO_THREAD,

    MPP_DEC_MODE_BUTT,
} MppDecMode;

typedef struct MppDecImpl_t MppDecImpl;

typedef struct MppDecModeApi_t {
    MPP_RET (*start)(MppDecImpl *dec);
    MPP_RET (*stop)(MppDecImpl *dec);
    MPP_RET (*reset)(MppDecImpl *dec);
    MPP_RET (*notify)(MppDecImpl *dec, RK_U32 flag);
    MPP_RET (*control)(MppDecImpl *dec, MpiCmd cmd, void *param);
} MppDecModeApi;

struct MppDecImpl_t {
    MppCodingType       coding;

    MppDecMode          mode;
    MppDecModeApi       *api;

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
    MppDecCfg           cfg_obj;
    MppDecCfgSet        *cfg;

    /* control process */
    MppMutexCond        cmd_lock;
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
    RK_U32              dis_err_clr_mark;
    RK_U32              enable_deinterlace;

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

    MppMemPool          ts_pool;
    struct list_head    ts_link;
    spinlock_t          ts_lock;
    void                *task_single;
};

/* external wait state */
#define MPP_DEC_WAIT_PKT_IN             (0x00000001)    /* input packet not ready */
#define MPP_DEC_WAIT_FRM_OUT            (0x00000002)    /* frame output queue full */

#define MPP_DEC_WAIT_INFO_CHG           (0x00000020)    /* wait info change ready */
#define MPP_DEC_WAIT_BUF_RDY            (0x00000040)    /* wait valid frame buffer */
#define MPP_DEC_WAIT_TSK_ALL_DONE       (0x00000080)    /* wait all task done */

#define MPP_DEC_WAIT_TSK_HND_RDY        (0x00000100)    /* wait task handle ready */
#define MPP_DEC_WAIT_TSK_PREV_DONE      (0x00000200)    /* wait previous task done */
#define MPP_DEC_WAIT_BUF_GRP_RDY        (0x00000200)    /* wait buffer group change ready */

/* internal wait state */
#define MPP_DEC_WAIT_BUF_SLOT_RDY       (0x00001000)    /* wait buffer slot ready */
#define MPP_DEC_WAIT_PKT_BUF_RDY        (0x00002000)    /* wait packet buffer ready */
#define MPP_DEC_WAIT_BUF_SLOT_KEEP      (0x00004000)    /* wait buffer slot reservation */

typedef union PaserTaskWait_u {
    RK_U32          val;
    struct {
        RK_U32      dec_pkt_in      : 1;   // 0x0001 MPP_DEC_NOTIFY_PACKET_ENQUEUE
        RK_U32      dis_que_full    : 1;   // 0x0002 MPP_DEC_NOTIFY_FRAME_DEQUEUE
        RK_U32      reserv0004      : 1;   // 0x0004
        RK_U32      reserv0008      : 1;   // 0x0008

        RK_U32      ext_buf_grp     : 1;   // 0x0010 MPP_DEC_NOTIFY_EXT_BUF_GRP_READY
        RK_U32      info_change     : 1;   // 0x0020 MPP_DEC_NOTIFY_INFO_CHG_DONE
        RK_U32      dec_pic_unusd   : 1;   // 0x0040 MPP_DEC_NOTIFY_BUFFER_VALID
        RK_U32      dec_all_done    : 1;   // 0x0080 MPP_DEC_NOTIFY_TASK_ALL_DONE

        RK_U32      task_hnd        : 1;   // 0x0100 MPP_DEC_NOTIFY_TASK_HND_VALID
        RK_U32      prev_task       : 1;   // 0x0200 MPP_DEC_NOTIFY_TASK_PREV_DONE
        RK_U32      dec_pic_match   : 1;   // 0x0400 MPP_DEC_NOTIFY_BUFFER_MATCH
        RK_U32      reserv0800      : 1;   // 0x0800

        RK_U32      dec_pkt_idx     : 1;   // 0x1000
        RK_U32      dec_pkt_buf     : 1;   // 0x2000
        RK_U32      dec_slot_idx    : 1;   // 0x4000 MPP_DEC_NOTIFY_SLOT_VALID
    };
} PaserTaskWait;

typedef union DecTaskStatus_u {
    RK_U32          val;
    struct {
        RK_U32      task_hnd_rdy      : 1;
        RK_U32      mpp_pkt_in_rdy    : 1;
        RK_U32      dec_pkt_idx_rdy   : 1;
        RK_U32      dec_pkt_buf_rdy   : 1;
        RK_U32      task_valid_rdy    : 1;
        RK_U32      dec_pkt_copy_rdy  : 1;
        RK_U32      prev_task_rdy     : 1;
        RK_U32      info_task_gen_rdy : 1;
        RK_U32      curr_task_rdy     : 1;
        RK_U32      task_parsed_rdy   : 1;
        RK_U32      mpp_in_frm_at_pkt : 1;
    };
} DecTaskStatus;

typedef struct MppPktTimestamp_t {
    struct list_head link;
    RK_S64  pts;
    RK_S64  dts;
} MppPktTs;

typedef struct DecTask_t {
    HalTaskHnd      hnd;

    DecTaskStatus   status;
    PaserTaskWait   wait;

    HalTaskInfo     info;
    MppPktTs        ts_cur;

    MppBuffer       hal_pkt_buf_in;
    MppBuffer       hal_frm_buf_out;
} DecTask;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET dec_task_info_init(HalTaskInfo *task);
void dec_task_init(DecTask *task);

MPP_RET mpp_dec_proc_cfg(MppDecImpl *dec, MpiCmd cmd, void *param);

MPP_RET update_dec_hal_info(MppDecImpl *dec, MppFrame frame);
void mpp_dec_put_frame(Mpp *mpp, RK_S32 index, HalDecTaskFlag flags);
RK_S32 mpp_dec_push_display(Mpp *mpp, HalDecTaskFlag flags);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_DEC_IMPL_H__*/
