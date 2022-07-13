/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __HAL_AVS2D_RKV_H__
#define __HAL_AVS2D_RKV_H__

#include "mpp_device.h"

#include "parser_api.h"
#include "hal_avs2d_api.h"
#include "hal_avs2d_global.h"
#include "avs2d_syntax.h"
#include "vdpu34x_com.h"

#define AVS2D_REGISTERS     (278)

typedef struct Vdpu34xRegAvs2dParam_t {
    struct SWREG64_H26X_SET {
        RK_U32      h26x_frame_orslice      : 1;
        RK_U32      h26x_rps_mode           : 1;
        RK_U32      h26x_stream_mode        : 1;
        RK_U32      h26x_stream_lastpacket  : 1;
        RK_U32      h264_firstslice_flag    : 1;
        RK_U32      reserve                 : 27;
    } reg64;

    RK_U32 reg65_cur_top_poc;
    RK_U32 reg66_cur_bot_poc;

    RK_U32 reg67_098_ref_poc[32];

    struct SWREG99_AVS2_REF0_3_INFO {
        RK_U32 ref0_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref0_botfield_used           : 1;
        RK_U32 ref0_valid_flag              : 1;
        RK_U32                              : 4;
        RK_U32 ref1_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref1_botfield_used           : 1;
        RK_U32 ref1_valid_flag              : 1;
        RK_U32                              : 4;
        RK_U32 ref2_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref2_botfield_used           : 1;
        RK_U32 ref2_valid_flag              : 1;
        RK_U32                              : 4;
        RK_U32 ref3_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref3_botfield_used           : 1;
        RK_U32 ref3_valid_flag              : 1;
        RK_U32                              : 4;
    } reg99;

    struct SWREG100_AVS2_REF4_7_INFO {
        RK_U32 ref4_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref4_botfield_used           : 1;
        RK_U32 ref4_valid_flag              : 1;
        RK_U32                              : 4;
        RK_U32 ref5_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref5_botfield_used           : 1;
        RK_U32 ref5_valid_flag              : 1;
        RK_U32                              : 4;
        RK_U32 ref6_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref6_botfield_used           : 1;
        RK_U32 ref6_valid_flag              : 1;
        RK_U32                              : 4;
        RK_U32 ref7_field                   : 1;
        RK_U32                              : 1;
        RK_U32 ref7_botfield_used           : 1;
        RK_U32 ref7_valid_flag              : 1;
        RK_U32                              : 4;
    } reg100;

    RK_U32 reg101_102[2];

    struct SW103_CTRL_EXTRA {
        // 0 : use default 255, 1 : use fixed 256
        RK_U32 slice_hor_pos_ctrl           : 1;
        RK_U32                              : 31;
    } reg103;

    RK_U32 reg104;
    struct SW105_HEAD_LEN {
        RK_U32 head_len                     : 4;
        RK_U32 count_update_en              : 1;
        RK_U32                              : 27;
    } reg105;

    RK_U32 reg106_111[6];
    struct SW112_ERROR_REF_INFO {
        // 0 : Frame, 1 : field
        RK_U32 ref_error_field              : 1;
        /**
         * @brief Refer error is top field flag.
         * 0 : Bottom field flag,
         * 1 : Top field flag.
         */
        RK_U32 ref_error_topfield           : 1;
        // For inter, 0 : top field is no used, 1 : top field is used.
        RK_U32 ref_error_topfield_used      : 1;
        // For inter, 0 : bottom field is no used, 1 : bottom field is used.
        RK_U32 ref_error_botfield_used      : 1;
        RK_U32                              : 28;
    } reg112;

} Vdpu34xRegAvs2dParam;

typedef struct Vdpu34xRegAvs2dAddr_t {
    /* SWREG160 */
    RK_U32  reg160_no_use;

    /* SWREG161 */
    RK_U32  head_base;

    /* SWREG162 */
    RK_U32  reg162_no_use;

    /* SWREG163 */
    RK_U32  rps_base;

    /* SWREG164~179 */
    RK_U32  ref_base[16];

    /* SWREG180 */
    RK_U32  scanlist_addr;

    /* SWREG181~196 */
    RK_U32  colmv_base[16];

    /* SWREG197 */
    RK_U32  cabactbl_base;
} Vdpu34xRegAvs2dAddr;

typedef struct Vdpu34xAvs2dRegSet_t {
    Vdpu34xRegCommon common;
    Vdpu34xRegAvs2dParam avs2d_param;
    Vdpu34xRegCommonAddr common_addr;
    Vdpu34xRegAvs2dAddr avs2d_addr;
    Vdpu34xRegIrqStatus irq_status;
    Vdpu34xRegStatistic statistic;
} Vdpu34xAvs2dRegSet;


#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET hal_avs2d_rkv_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_avs2d_rkv_deinit  (void *hal);
MPP_RET hal_avs2d_rkv_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_rkv_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_avs2d_rkv_wait    (void *hal, HalTaskInfo *task);

#ifdef  __cplusplus
}
#endif

#endif /*__HAL_AVS2D_RKV_H__*/
