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

#ifndef __HAL_H265E_RKV_CTX_H__
#define __HAL_H265E_RKV_CTX_H__

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "rkv_enc_def.h"
#include "mpp_enc_hal.h"
#include "hal_bufs.h"

typedef struct h265e_feedback_t {
    RK_U32 hw_status; /* 0:corret, 1:error */
    RK_U32 qp_sum;
    RK_U32 out_strm_size;
    RK_U32 out_hw_strm_size;
    RK_S64 sse_sum;
    RK_U32 st_lvl64_inter_num;
    RK_U32 st_lvl32_inter_num;
    RK_U32 st_lvl16_inter_num;
    RK_U32 st_lvl8_inter_num;
    RK_U32 st_lvl32_intra_num;
    RK_U32 st_lvl16_intra_num;
    RK_U32 st_lvl8_intra_num;
    RK_U32 st_lvl4_intra_num;
    RK_U32 st_cu_num_qp[52];
    RK_U32 st_madp;
    RK_U32 st_madi;
} h265e_feedback;

typedef struct H265eRkvHalContext_t {
    MppEncHalApi        api;
    IOInterruptCB       int_cb;
    MppDevCtx           dev_ctx;
    void                *regs;
    void                *l2_regs;
    void                *ioctl_input;
    void                *ioctl_output;
    void                *rc_hal_cfg;

    h265e_feedback      feedback;
    void                *buffers;
    void                *dump_files;
    RK_U32              frame_cnt_gen_ready;
    RK_U32              frame_cnt_send_ready;
    RK_U32              num_frames_to_send;

    /*qp decision*/
    RK_S32              cur_scale_qp;
    RK_S32              pre_i_qp;
    RK_S32              pre_p_qp;
    RK_S32              start_qp;
    RK_S32              frame_type;
    RK_S32              last_frame_type;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_cnt;
    RK_S32              osd_plt_type;
    MppEncOSDData       *osd_data;
    MppEncROICfg        *roi_data;
    void                *roi_buf;
    MppEncCfgSet        *set;
    MppEncCfgSet        *cfg;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_U32              alloc_flg;
    RK_S32              hdr_status;
    void                *input_fmt;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    RK_U32              frame_num;
    HalBufs             dpb_bufs;
} H265eRkvHalContext;

#endif
