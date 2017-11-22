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

#ifndef __HAL_H264E_RKV_DPB_H__
#define __HAL_H264E_RKV_DPB_H__

#include "hal_h264e_com.h"

#define RKV_H264E_REF_MAX                16

#define RKVENC_CODING_TYPE_AUTO          0x0000  /* Let x264 choose the right type */
#define RKVENC_CODING_TYPE_IDR           0x0001
#define RKVENC_CODING_TYPE_I             0x0002
#define RKVENC_CODING_TYPE_P             0x0003
#define RKVENC_CODING_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define RKVENC_CODING_TYPE_B             0x0005
#define RKVENC_CODING_TYPE_KEYFRAME      0x0006  /* IDR or I depending on b_open_gop option */
#define RKVENC_IS_TYPE_I(x) ((x)==RKVENC_CODING_TYPE_I || (x)==RKVENC_CODING_TYPE_IDR)
#define RKVENC_IS_TYPE_B(x) ((x)==RKVENC_CODING_TYPE_B || (x)==RKVENC_CODING_TYPE_BREF)
#define RKVENC_IS_DISPOSABLE(type) ( type == RKVENC_CODING_TYPE_B )

typedef struct  H264eRkvFrame_t {
    MppBuffer   hw_buf;
    RK_S32      hw_buf_used;
    RK_S32      i_frame_cnt; /* Presentation frame number */
    RK_S32      i_frame_num; /* 7.4.3 frame_num */
    RK_S32      long_term_flag;
    RK_S32      reorder_longterm_flag;
    RK_S32      i_poc;
    RK_S32      i_delta_poc[2];
    RK_S32      i_frame_type;
    RK_S64      i_pts;
    RK_S64      i_dts;
    RK_S32      b_kept_as_ref;
    RK_S32      b_keyframe;
    RK_S32      b_corrupt;
    RK_S32      i_reference_count;
} H264eRkvFrame;

typedef struct H264eRkvDpbCtx_t {
    //H264eRkvFrame *fenc;
    H264eRkvFrame *fdec;
    H264eRkvFrame *fref[2][RKV_H264E_REF_MAX + 1];
    H264eRkvFrame *fref_nearest[2]; //Used for RC
    struct {
        /* Unused frames: 0 = fenc, 1 = fdec */
        H264eRkvFrame **unused;

        /* frames used for reference + sentinels */
        H264eRkvFrame *reference[RKV_H264E_REF_MAX + 1]; //TODO: remove later

        RK_S32 i_last_keyframe;  /* Frame number of the last keyframe */
        RK_S32 i_last_idr;       /* Frame number of the last IDR (not RP)*/
        //RK_S64 i_largest_pts;
        //RK_S64 i_second_largest_pts;
    } frames;

    H264eRkvFrame    frame_buf[RKV_H264E_REF_MAX + 1];

    RK_S32      i_ref[2];
    RK_U32      i_nal_type;
    RK_U32      i_nal_ref_idc;

    RK_S32      i_frame_cnt; /* Presentation frame number */
    RK_S32      i_frame_num; /* 7.4.3 frame_num */

    //move from slice header below
    RK_S32      i_slice_type;
    RK_S32      i_idr_pic_id;   /* -1 if nal_type != 5 */
    RK_S32      i_tmp_idr_pic_id;
    //RK_S32 i_poc;
    //RK_S32 i_delta_poc[2];
    //RK_S32 i_redundant_pic_cnt;
    RK_S32      i_max_ref0;
    RK_S32      i_max_ref1;
    RK_S32      b_ref_pic_list_reordering[2];
    struct {
        RK_S32 idc;
        RK_S32 arg;
    } ref_pic_list_order[2][RKV_H264E_REF_MAX];

    RK_S32 i_mmco_remove_from_end;
    RK_S32 i_mmco_command_count;
    struct { /* struct for future expansion */
        RK_S32 i_difference_of_pic_nums;
        RK_S32 i_poc;
        RK_S32 memory_management_control_operation;
    } mmco[RKV_H264E_REF_MAX];

    RK_S32 i_long_term_reference_flag;
} H264eRkvDpbCtx;

MPP_RET h264e_rkv_reference_frame_set( H264eHalContext *ctx, H264eHwCfg *syn);
MPP_RET h264e_rkv_reference_frame_update( H264eHalContext *ctx);

#endif /* __HAL_H264E_RKV_DPB_H__ */
