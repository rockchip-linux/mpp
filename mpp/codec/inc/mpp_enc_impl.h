/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __MPP_ENC_IMPL_H__
#define __MPP_ENC_IMPL_H__

#include "enc_impl.h"
#include "mpp_enc_hal.h"
#include "mpp_enc_ref.h"
#include "mpp_enc_refs.h"
#include "mpp_device.h"

#include "rc.h"
#include "hal_info.h"

#define HDR_ADDED_MASK  0xe

typedef struct MppPskipMode_t {
    RK_S32 pskip_is_ref;
    RK_S32 pskip_is_non_ref;
} MppPskipMode;

typedef union MppEncHeaderStatus_u {
    RK_U32 val;
    struct {
        RK_U32          ready           : 1;

        RK_U32          added_by_ctrl   : 1;
        RK_U32          added_by_mode   : 1;
        RK_U32          added_by_change : 1;
    };
} MppEncHeaderStatus;

typedef struct RcApiStatus_t {
    RK_U32              rc_api_inited   : 1;
    RK_U32              rc_api_updated  : 1;
    RK_U32              rc_api_user_cfg : 1;
} RcApiStatus;

typedef struct MppEncImpl_t {
    MppCodingType       coding;
    EncImpl             impl;
    MppEncHal           enc_hal;

    /* device from hal */
    MppDev              dev;
    HalInfo             hal_info;
    RK_S64              time_base;
    RK_S64              time_end;
    RK_S32              frame_count;
    RK_S32              hal_info_updated;

    /*
     * Rate control plugin parameters
     */
    RcApiStatus         rc_status;
    RK_S32              rc_api_updated;
    RK_S32              rc_cfg_updated;
    RcApiBrief          rc_brief;
    RcCtx               rc_ctx;

    /*
     * thread input / output context
     */
    MppThread           *thread_enc;
    void                *mpp;

    MppPort             input;
    MppPort             output;
    MppTask             task_in;
    MppTask             task_out;
    MppFrame            frame;
    MppPacket           packet;
    RK_U32              low_delay_part_mode;
    RK_U32              low_delay_output;
    /* output callback for slice output */
    MppCbCtx            output_cb;

    /* base task information */
    HalTaskGroup        tasks;
    HalTaskHnd          hnd;
    EncAsyncTaskInfo    *async;
    RK_U32              task_idx;
    RK_S64              task_pts;
    MppBuffer           frm_buf;
    MppBuffer           pkt_buf;
    MppBuffer           md_info;

    // internal status and protection
    MppMutex            lock;
    RK_U32              reset_flag;
    sem_t               enc_reset;

    RK_U32              wait_count;
    RK_U32              work_count;
    RK_U32              status_flag;
    RK_U32              notify_flag;
    RK_U32              enc_failed_drop;

    /* control process */
    RK_U32              cmd_send;
    RK_U32              cmd_recv;
    MpiCmd              cmd;
    void                *param;
    MPP_RET             *cmd_ret;
    sem_t               cmd_start;
    sem_t               cmd_done;

    // legacy support for MPP_ENC_GET_EXTRA_INFO
    MppPacket           hdr_pkt;
    void                *hdr_buf;
    RK_U32              hdr_len;
    MppEncHeaderStatus  hdr_status;
    MppEncHeaderMode    hdr_mode;
    MppEncSeiMode       sei_mode;

    /* information for debug prefix */
    const char          *version_info;
    RK_S32              version_length;
    char                *rc_cfg_info;
    RK_S32              rc_cfg_pos;
    RK_S32              rc_cfg_length;
    RK_S32              rc_cfg_size;

    /* cpb parameters */
    MppEncRefs          refs;
    MppEncRefFrmUsrCfg  frm_cfg;

    /* two-pass deflicker parameters */
    RK_U32              support_hw_deflicker;
    EncRcTaskInfo       rc_info_prev;

    /* encoder config object for external setup */
    MppEncCfg           set_obj;
    MppEncCfgSet        *set;
    /* encoder config object for internal usage */
    MppEncCfg           cfg_obj;
    MppEncCfgSet        *cfg;
} MppEncImpl;

#ifdef __cplusplus
extern "C" {
#endif

void *mpp_enc_thread(void *data);
void *mpp_enc_async_thread(void *data);
MPP_RET mpp_enc_callback(const char *caller, void *ctx, RK_S32 cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_IMPL_H__*/
