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
#ifndef __H265E_CODEC_H__
#define __H265E_CODEC_H__

#include "mpp_log.h"

#include "mpp_common.h"
#include "mpp_rc.h"

#include "h265e_syntax.h"
#include "h265e_dpb.h"
#include "enc_impl_api.h"
#include "rc_api.h"
#include "rc.h"

#define H265E_DBG_FUNCTION          (0x00000001)
#define H265E_DBG_INPUT             (0x00000010)
#define H265E_DBG_OUTPUT            (0x00000020)
#define H265E_DBG_PS                (0x00000040)
#define H265E_DBG_DPB               (0x00000080)
#define H265E_DBG_SLICE             (0x00000100)
#define H265E_DBG_HEADER            (0x00000200)
#define H265E_DBG_API               (0x00000400)


#define H265E_PS_BUF_SIZE           512
#define H265E_SEI_BUF_SIZE          1024
#define H265E_EXTRA_INFO_BUF_SIZE   (H265E_PS_BUF_SIZE + H265E_SEI_BUF_SIZE)


extern RK_U32 h265e_debug;

#define h265e_dbg(flag, fmt, ...)   _mpp_dbg(h265e_debug, flag, fmt, ## __VA_ARGS__)
#define h265e_dbg_f(flag, fmt, ...) _mpp_dbg_f(h265e_debug, flag, fmt, ## __VA_ARGS__)

#define h265e_dbg_func(fmt, ...)    h265e_dbg_f(H265E_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define h265e_dbg_input(fmt, ...)   h265e_dbg(H265E_DBG_INPUT, fmt, ## __VA_ARGS__)
#define h265e_dbg_output(fmt, ...)  h265e_dbg(H265E_DBG_OUTPUT, fmt, ## __VA_ARGS__)

#define h265e_dbg_ps(fmt, ...)      h265e_dbg(H265E_DBG_PS, fmt, ## __VA_ARGS__)
#define h265e_dbg_dpb(fmt, ...)     h265e_dbg(H265E_DBG_DPB, fmt, ## __VA_ARGS__)
#define h265e_dbg_slice(fmt, ...)   h265e_dbg(H265E_DBG_SLICE, fmt, ## __VA_ARGS__)

/*
 * Split reference frame configure to two parts
 * The first part is slice depended info like poc / frame_num, and frame
 * type and flags.
 * The other part is gop structure depended info like gop index, ref_status
 * and ref_frm_index. This part is inited from dpb gop hierarchy info.
 */
typedef struct H265eFrmInfo_s {
    RK_S32              seq_idx;

    RK_S32              curr_idx;
    RK_S32              refr_idx;

    // current frame rate control and dpb status info
    RK_S32              mb_per_frame;
    RK_S32              mb_raw;
    RK_S32              mb_wid;
    RK_S32              frame_type;
    RK_S32              last_frame_type;
    RK_S32              cur_scale_qp;
    RK_S32              pre_i_qp;
    RK_S32              pre_p_qp;
    RK_S32              start_qp;

    RcHalCfg            rc_cfg;
    EncFrmStatus        status;

    RK_S32              usage[MAX_REFS + 1];
} H265eFrmInfo;

typedef struct H265eCtx_t {
    MppEncCfgSet        *cfg;
    MppEncCfgSet        *set;
    MppDeviceId         dev_id;
    MppRateControl      *rc;
    RcCtx               rc_ctx;
    RK_U32              rc_ready;
    RK_S32              idr_request;

    H265eVps            vps;
    H265eSps            sps;
    H265ePps            pps;
    H265eSlice          *slice;
    H265eDpbCfg         dpbcfg;
    H265eDpb            *dpb;
    H265eDpb            *dpb_bak;
    H265eFrmInfo        frms;
    RK_U32              plt_flag;

    void                *extra_info;
    void                *param_buf;
    MppPacket           packeted_param;
    H265eSyntax         syntax;
    H265eFeedback       feedback;
    struct list_head    rc_list;
} H265eCtx;

#endif
