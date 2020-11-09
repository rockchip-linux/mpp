/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __MPP_RC__
#define __MPP_RC__

#include "rk_venc_cmd.h"

#include "mpp_log.h"
#include "mpp_list.h"

/*
 * mpp rate control contain common caculation methd
 *
 * 1. MppData - data statistic struct
 *    size - max valid data number
 *    len  - valid data number
 *    pos  - current load/store position
 *    val  - buffer array pointer
 */
typedef struct {
    RK_S32  size;
    RK_S32  len;
    RK_S32  pos;
    RK_S32  *val;
} MppData;

/*
 * 2. Proportion Integration Differentiation (PID) control
 */
typedef struct {
    RK_S32  p;
    RK_S32  i;
    RK_S32  d;
    RK_S32  coef_p;
    RK_S32  coef_i;
    RK_S32  coef_d;
    RK_S32  div;
    RK_S32  len;
    RK_S32  count;
} MppPIDCtx;

typedef enum ENC_FRAME_TYPE_E {
    INTER_P_FRAME   = 0,
    INTER_B_FRAME   = 1,
    INTRA_FRAME     = 2,
    INTER_VI_FRAME  = 3,
} ENC_FRAME_TYPE;

/*
 * MppRateControl has three steps work:
 *
 * 1. translate user requirement to bit rate parameters
 * 2. calculate target bit from bit parameters
 * 3. calculate qstep from target bit
 *
 * That is user setting -> target bit -> qstep.
 *
 * This struct will be used in both controller and hal.
 * EncImpl provide step 1 and step 2. Hal provide step 3.
 *
 */
typedef enum MppEncGopMode_e {
    /* gop == 0 */
    MPP_GOP_ALL_INTER,
    /* gop == 1 */
    MPP_GOP_ALL_INTRA,
    /* gop < fps */
    MPP_GOP_SMALL,
    /* gop >= fps */
    MPP_GOP_LARGE,
    MPP_GOP_MODE_BUTT,
} MppEncGopMode;

typedef enum RC_PARAM_OPS {
    RC_RECORD_REAL_BITS,
    RC_RECORD_QP_SUM,
    RC_RECORD_QP_MIN,
    RC_RECORD_QP_MAX,
    RC_RECORD_SET_QP,
    RC_RECORD_REAL_QP,
    RC_RECORD_SSE_SUM,
    RC_RECORD_WIN_LEN
} RC_PARAM_OPS;

typedef struct RecordNode_t {
    struct list_head list;
    ENC_FRAME_TYPE   frm_type;
    /* @frm_cnt starts from ONE */
    RK_U32           frm_cnt;
    RK_U32           bps;
    RK_U32           fps;
    RK_S32           gop;
    RK_S32           bits_per_pic;
    RK_S32           bits_per_intra;
    RK_S32           bits_per_inter;
    RK_U32           tgt_bits;
    RK_U32           bit_min;
    RK_U32           bit_max;
    RK_U32           real_bits;
    RK_S32           acc_intra_bits_in_fps;
    RK_S32           acc_inter_bits_in_fps;
    RK_S32           last_fps_bits;
    float            last_intra_percent;

    /* hardware result */
    RK_S32           qp_sum;
    RK_S64           sse_sum;
    RK_S32           set_qp;
    RK_S32           qp_min;
    RK_S32           qp_max;
    RK_S32           real_qp;
    RK_S32           wlen;
} RecordNode;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_data_init(MppData **p, RK_S32 len);
void mpp_data_deinit(MppData *p);
void mpp_data_update(MppData *p, RK_S32 val);
RK_S32 mpp_data_avg(MppData *p, RK_S32 len, RK_S32 num, RK_S32 denorm);

void mpp_pid_reset(MppPIDCtx *p);
void mpp_pid_set_param(MppPIDCtx *p, RK_S32 coef_p, RK_S32 coef_i, RK_S32 coef_d, RK_S32 div, RK_S32 len);
void mpp_pid_update(MppPIDCtx *p, RK_S32 val);
RK_S32 mpp_pid_calc(MppPIDCtx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_RC__ */
