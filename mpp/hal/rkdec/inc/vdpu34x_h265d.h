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

#ifndef __VDPU34X_H265D_H__
#define __VDPU34X_H265D_H__

#include "vdpu34x_com.h"

typedef struct Vdpu34xRegH265d_t {
    struct SWREG64_H26X_SET {
        RK_U32      h26x_frame_orslice      : 1;
        RK_U32      h26x_rps_mode           : 1;
        RK_U32      h26x_stream_mode        : 1;
        RK_U32      h26x_stream_lastpacket  : 1;
        RK_U32      h264_firstslice_flag    : 1;
        RK_U32      reserve                 : 27;
    } h26x_set;

    struct SWREG65_CUR_POC {
        RK_U32      cur_top_poc : 32;
    } cur_poc;

    struct SWREG66_H264_CUR_POC1 {
        RK_U32      cur_bot_poc : 32;
    } cur_poc1;

    struct SWREG67_82_H26x_REF_POC {
        RK_U32      ref_poc : 32;
    } ref0_15_poc[16];


    struct SWREG83_98_H264_REF_POC {
        RK_U32      ref_poc : 32;
    } ref_poc_no_use[16];

    /*     struct SWREG99_HEVC_REF_VALID{
            RK_U32      hevc_ref_valid  : 15;
            RK_U32      reserve         : 17;
        }hevc_ref_valid; */

    struct SWREG99_HEVC_REF_VALID {
        RK_U32      hevc_ref_valid_0    : 1;
        RK_U32      hevc_ref_valid_1    : 1;
        RK_U32      hevc_ref_valid_2    : 1;
        RK_U32      hevc_ref_valid_3    : 1;
        RK_U32      reserve0            : 4;
        RK_U32      hevc_ref_valid_4    : 1;
        RK_U32      hevc_ref_valid_5    : 1;
        RK_U32      hevc_ref_valid_6    : 1;
        RK_U32      hevc_ref_valid_7    : 1;
        RK_U32      reserve1            : 4;
        RK_U32      hevc_ref_valid_8    : 1;
        RK_U32      hevc_ref_valid_9    : 1;
        RK_U32      hevc_ref_valid_10   : 1;
        RK_U32      hevc_ref_valid_11   : 1;
        RK_U32      reserve2            : 4;
        RK_U32      hevc_ref_valid_12   : 1;
        RK_U32      hevc_ref_valid_13   : 1;
        RK_U32      hevc_ref_valid_14   : 1;
        RK_U32      reserve3            : 5;
    } hevc_ref_valid;

    RK_U32  reg100_102_no_use[3];

    struct SWREG103_HEVC_MVC0 {
        RK_U32      ref_pic_layer_same_with_cur : 16;
        RK_U32      reserve                     : 16;
    } hevc_mvc0;

    struct SWREG104_HEVC_MVC1 {
        RK_U32      poc_lsb_not_present_flag        : 1;
        RK_U32      num_direct_ref_layers           : 6;
        RK_U32      reserve0                        : 1;

        RK_U32      num_reflayer_pics               : 6;
        RK_U32      default_ref_layers_active_flag  : 1;
        RK_U32      max_one_active_ref_layer_flag   : 1;

        RK_U32      poc_reset_info_present_flag     : 1;
        RK_U32      vps_poc_lsb_aligned_flag        : 1;
        RK_U32      mvc_poc15_valid_flag            : 1;
        RK_U32      reserve1                        : 13;
    } hevc_mvc1;

    struct SWREG105_111_NO_USE_REGS {
        RK_U32  no_use_regs[7];
    } no_use_regs;

    struct SWREG112_ERROR_REF_INFO {
        RK_U32      avs2_ref_error_field        : 1;
        RK_U32      avs2_ref_error_topfield     : 1;
        RK_U32      ref_error_topfield_used     : 1;
        RK_U32      ref_error_botfield_used     : 1;
        RK_U32      reserve                     : 28;
    } err_ref_info;

} Vdpu34xRegH265d;

typedef struct Vdpu34xRegH265dAddr_t {
    struct SWREG160_VP9_DELTA_PROB_BASE {
        RK_U32 vp9_delta_prob_base  : 32;
    } reg160_no_use;

    struct SWREG161_PPS_BASE {
        RK_U32      pps_base    : 32;
    } pps_base;

    RK_U32 reg162_no_use;

    struct SWREG163_RPS_BASE {
        RK_U32      rps_base    : 32;
    } rps_base;

    struct SWREG164_179_H26x_REF_BASE {
        RK_U32      ref_base    : 32;
    } ref0_15_base[16];

    struct SWREG180_H26x_SCANLIST_BASE {
        RK_U32      scanlist_addr   : 32;
    } h26x_scanlist_base;

    struct SWREG181_196_H26x_COLMV_REF_BASE {
        RK_U32      colmv_base  : 32;
    } ref0_15_colmv_base[16];

    struct SWREG197_H26x_CABACTBL_BASE {
        RK_U32      cabactbl_base   : 32;
    } cabactbl_base;
} Vdpu34xRegH265dAddr;

typedef struct Vdpu34xH265dRegSet_t {
    Vdpu34xRegCommon        common;
    Vdpu34xRegH265d         h265d_param;
    Vdpu34xRegCommonAddr    common_addr;
    Vdpu34xRegH265dAddr     h265d_addr;
    Vdpu34xRegIrqStatus     irq_status;
} Vdpu34xH265dRegSet;

#endif /* __VDPU34X_H265D_H__ */
