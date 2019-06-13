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

#include "rk_mpi.h"

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

/*
 * 3. linear module
 */
#define LINEAR_MODEL_STATISTIC_COUNT    15

/*
 * Linear regression
 * C = a * x * r + b * x * x * r
 */
typedef struct linear_model_s {
    RK_S32 size;        /* elements max size */
    RK_S32 n;           /* elements count */
    RK_S32 i;           /* elements index for store */

    double a;           /* coefficient */
    double b;           /* coefficient */
    double c;

    RK_S32 *x;          /* x */
    RK_S32 *r;          /* r */
    RK_S64 *y;          /* y = x * x * r */
    RK_S32 weight_mode; /* different weight ratio*/
} MppLinReg;

/* Virtual buffer */
typedef struct MppVirtualBuffer_s {
    RK_S32 bufferSize;          /* size of the virtual buffer */
    RK_S32 bitRate;             /* input bit rate per second */
    RK_S32 bitPerPic;           /* average number of bits per picture */
    RK_S32 picTimeInc;          /* timeInc since last coded picture */
    RK_S32 timeScale;           /* input frame rate numerator */
    RK_S32 unitsInTic;          /* input frame rate denominator */
    RK_S32 virtualBitCnt;       /* virtual (channel) bit count */
    RK_S32 realBitCnt;          /* real bit count */
    RK_S32 bufferOccupancy;     /* number of bits in the buffer */
    RK_S32 skipFrameTarget;     /* how many frames should be skipped in a row */
    RK_S32 skippedFrames;       /* how many frames have been skipped in a row */
    RK_S32 nonZeroTarget;
    RK_S32 bucketFullness;      /* Leaky Bucket fullness */
    RK_S32 gopRem;
    RK_S32 windowRem;
} MppVirtualBuffer;


typedef enum ENC_FRAME_TYPE_E {
    INTER_P_FRAME = 0,
    INTER_B_FRAME = 1,
    INTRA_FRAME   = 2
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
 * Controller provide step 1 and step 2. Hal provide step 3.
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
    RC_RECORD_LIN_REG,
    RC_RECORD_WIN_LEN
} RC_PARAM_OPS;

typedef struct MppRateControl_s {
    /* control parameter from external config */
    RK_S32 fps_num;
    RK_S32 fps_denom;
    RK_S32 fps_out;
    RK_S32 gop;
    RK_S32 bps_min;
    RK_S32 bps_target;
    RK_S32 bps_max;

    /*
     * derivation parameter
     * bits_per_pic - average bit count in gop
     * bits_per_intra   - target intra frame size
     *                if not intra frame is encoded default set to a certain
     *                times of inter frame according to gop
     * bits_per_inter   - target inter frame size
     */
    MppEncGopMode gop_mode;
    RK_S32 bits_per_pic;
    RK_S32 bits_per_intra;
    RK_S32 bits_per_inter;
    RK_S32 mb_per_frame;

    /* bitrate window which tries to match target */
    RK_S32 window_len;
    RK_S32 intra_to_inter_rate;

    RK_S32 acc_intra_bits_in_fps;
    RK_S32 acc_inter_bits_in_fps;
    RK_S32 acc_total_bits;
    RK_U32 time_in_second;

    RK_S32 acc_intra_count;
    RK_S32 acc_inter_count;
    RK_S32 acc_total_count;
    RK_S32 last_fps_bits;
    float last_intra_percent;

    RK_S32 prev_intra_target;
    RK_S32 prev_inter_target;

    /* runtime status parameter */
    ENC_FRAME_TYPE cur_frmtype;
    ENC_FRAME_TYPE pre_frmtype;

    /*
     * intra     - intra frame bits record
     * inter     - inter frame bits record
     * pid_intra - intra frame bits record
     * pid_inter - inter frame bits record
     * pid_gop   - serial frame bits record
     */
    MppData *intra;
    MppData *inter;
    MppData *gop_bits;
    MppData *intra_percent;
    MppPIDCtx pid_intra;
    MppPIDCtx pid_inter;
    MppPIDCtx pid_fps;
    /*
     * Vbv buffer control
    */
    MppVirtualBuffer vb;
    RK_S32           hrd;

    /*
     * output target bits on current status
     * 0        - do not do rate control
     * non-zero - have rate control
     */
    RK_S32 bits_target;
    RK_S32 pre_gop_left_bit;
    float max_rate;
    float min_rate;

    /* start from ONE */
    RK_U32 frm_cnt;
    RK_S32 real_bps;
    RK_S32 prev_aq_prop_offset;
    RK_S32 quality;

} MppRateControl;

/*
 * Data structure from encoder to hal
 * type         - frame encode type
 * bit_target   - frame target size
 * bit_min      - frame minimum size
 * bit_max      - frame maximum size
 */
typedef struct RcSyntax_s {
    MppEncGopMode    gop_mode;
    ENC_FRAME_TYPE   type;
    RK_S32           bit_target;
    RK_S32           bit_max;
    RK_S32           bit_min;
    RK_S32           aq_prop_offset;

    /* head node of rc parameter list */
    struct list_head *rc_head;
} RcSyntax;

/*
 * Data structure from hal to encoder
 * type         - frame encode type
 * bits         - frame actual byte size
 */
typedef struct HalRcResult_s {
    ENC_FRAME_TYPE  type;
    RK_S32          time;
    RK_S32          bits;
} RcHalResult;

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
    MppLinReg        lin_reg;
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

MPP_RET mpp_rc_init(MppRateControl **ctx);
MPP_RET mpp_rc_deinit(MppRateControl *ctx);

/*
 * Translate MppEncRcCfg struct to internal bitrate setting
 * Called in mpp_control function.
 * If parameter changed mark flag and let encoder recalculate bit allocation.
 */
MPP_RET mpp_rc_update_user_cfg(MppRateControl *ctx, MppEncRcCfg *cfg, RK_S32 force_idr);

/*
 * When one frame is encoded hal will call this function to update paramter
 * from hardware. Hardware will update bits / qp_sum / mad or sse data
 *
 * Then rate control will update the linear regression model
 */
MPP_RET mpp_rc_update_hw_result(MppRateControl *ctx, RcHalResult *result);

/*
 * Use bps/fps config generate bit allocation setting
 * Called in controller loop when parameter changed or get a encoder result.
 * This function will calculation next frames target bits according to current
 * bit rate status.
 *
 * bits[0] - target
 * bits[1] - min
 * bits[1] - max
 */
MPP_RET mpp_rc_bits_allocation(MppRateControl *ctx, RcSyntax *rc_syn);

MPP_RET mpp_rc_record_param(struct list_head *head, MppRateControl *ctx,
                            RcSyntax *rc_syn);

MPP_RET mpp_rc_calc_real_bps(struct list_head *head, MppRateControl *ctx,
                             RK_S32 cur_bits);

MPP_RET mpp_rc_param_ops(struct list_head *head, RK_U32 frm_cnt,
                         RC_PARAM_OPS ops, void *arg);

MPP_RET mpp_linreg_init(MppLinReg **ctx, RK_S32 size, RK_S32 weight_mode);
MPP_RET mpp_linreg_deinit(MppLinReg *ctx);
void mpp_save_regdata(MppLinReg *ctx, RK_S32 x, RK_S32 r);
MPP_RET mpp_linreg_update(MppLinReg *ctx);
MPP_RET mpp_quadreg_update(MppLinReg *ctx, RK_S32 wlen);
RK_S32  mpp_linreg_calc(MppLinReg *ctx, RK_S32 r);
RK_S64  mpp_quadreg_calc(MppLinReg *ctx, RK_S32 r);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_RC__ */
