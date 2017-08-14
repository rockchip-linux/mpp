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

#ifndef __VP8E_SYNTAX_H__
#define __VP8E_SYNTAX_H__

#include "rk_type.h"

typedef struct {
    RK_S32  a1;
    RK_S32  a2;
    RK_S32  qp_prev;
    RK_S32  qs[15];
    RK_S32  bits[15];
    RK_S32  pos;
    RK_S32  len;
    RK_S32  zero_div;
} Vp8eLinReg;


typedef struct vp8e_feedback_t {
    /* rc */
    void *result;
    RK_U32 hw_status; /* 0:corret, 1:error */

    RK_S32 qp_sum;
    RK_S32 cp[10];
    RK_S32 mad_count;
    RK_S32 rlc_count;
    RK_U32 out_strm_size;
    RK_U32 out_hw_strm_size;
    RK_S64 sse_sum;

} Vp8eFeedback;

typedef struct vp8e_virture_buffer_t {
    RK_S32 buffer_size;
    RK_S32 bit_rate;     //MppEncRcCfg.bps_target
    RK_S32 bps_max;
    RK_S32 bps_min;
    RK_S32 bit_per_pic;
    RK_S32 pic_time_inc;
    RK_S32 time_scale;
    RK_S32 units_in_tic;
    RK_S32 virtual_bit_cnt;
    RK_S32 real_bit_cnt;
    RK_S32 buffer_occupancy;
    RK_S32 skip_frame_target;
    RK_S32 skipped_frames;
    RK_S32 bucket_fullness;
    RK_S32 gop_rem;
} Vp8eVirBuf;

typedef struct {
    RK_U8  pic_rc_enable;
    RK_U8  pic_skip;
    RK_U8  frame_coded;
    RK_S32 mb_per_pic;
    RK_S32 mb_rows;
    RK_S32 curr_frame_intra;
    RK_S32 prev_frame_intra;
    RK_S32 fixed_qp;
    RK_S32 qp_hdr;
    RK_S32 qp_min;
    RK_S32 qp_max;
    RK_S32 qp_hdr_prev;
    RK_S32 fps_out_num;     //MppEncRcCfg.fps_out_num
    RK_S32 fps_out_denorm;  //MppEncRcCfg.fps_out_denorm
    RK_S32 fps_out;

    RK_S32 target_pic_size;
    RK_S32 frame_bit_cnt;
    RK_S32 gop_qp_sum;
    RK_S32 gop_qp_div;
    RK_S32 frame_cnt;
    RK_S32 gop_len;
    RK_S32 intra_qp_delta;
    RK_S32 fixed_intra_qp;
    RK_S32 mb_qp_adjustment;
    RK_S32 intra_picture_rate;
    RK_S32 golden_picture_rate;
    RK_S32 altref_picture_rate;
    RK_U32 time_inc;

    Vp8eLinReg lin_reg;
    Vp8eLinReg r_error;
    Vp8eVirBuf virbuf;
} Vp8eRc;

#endif /*__VP8E_SYNTAX_H__*/
