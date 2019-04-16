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

#ifndef __MPP_ENC_REFS_H__
#define __MPP_ENC_REFS_H__

#include "rk_type.h"

#include "mpp_err.h"
#include "mpp_list.h"

/*
 * GOP structure intermediate format string definition
 *
 * Mpp use string to explicitly specify the frame property in a gop.
 * It contains some fields to be parsed:
 *
 * 1. Frame type
 * I    - IDR frame
 * i    - I frame but not IDR frame
 * P/p  - P frame
 * B/b  - B frame
 *
 * 2. Display order
 * Integer value frame 0 to gop size - 1
 *
 * 3. Reference type
 * N/n  - Non-reference frame
 * S/s  - short-term reference frame
 * L/l  - long-term reference frame
 *
 * 4. Encode order
 * Integer value frame 0 to gop size - 1
 * Note: not supported
 *
 * 5. Temporal layer id
 * Start with T/t then Integer value frame 0 to max temporal layer - 1
 *
 * Example for TSVC4:
 *
 * I0LT0   P1NT3 P2ST2 P3NT3 P4ST1   P5NT3 P6ST2 P7NT3 P8ST0
 *
 *       /-> P1      /-> P3        /-> P5      /-> P7            30fps
 *      /           /             /           /
 *     //--------> P2            //--------> P6                  15fps
 *    //                        //
 *   ///---------------------> P4                               7.5fps
 *  ///
 *  P0 ------------------------------------------------> P8    3.75fps
 *
 *  0        1     2     3     4       5     6     7     8
 *
 */

#define MAX_GOP_SIZE            8
#define MAX_GOP_FMT_CNT         (MAX_GOP_SIZE+1)
#define MAX_GOP_FMT_SIZE        5
#define MAX_GOP_FMT_BUF_STUFF   16
#define MAX_GOP_FMT_BUF_SIZE    (MAX_GOP_FMT_CNT * MAX_GOP_FMT_SIZE + MAX_GOP_FMT_BUF_STUFF)

/*
 * For controller and hal communication
 */
typedef struct MppEncHierCfg_t {
    RK_U32  change;

    // hierarchy pattern loop length unit in frames
    // NOTE: need one extra size to close the gop loop
    // For example the P8 reference P0 or P4 is quite different
    RK_S32  length;
    RK_S32  ref_idx[MAX_GOP_FMT_CNT];
    char    ref_fmt[MAX_GOP_FMT_BUF_SIZE];
} MppEncHierCfg;

typedef struct MppEncFrmStatus_t {
    /*
     * 0 - inter frame
     * 1 - intra frame
     */
    RK_U32  is_intra    : 1;

    /*
     * Valid when is_intra is true
     * 0 - normal intra frame
     * 1 - IDR frame
     */
    RK_U32  is_idr      : 1;

    /*
     * 0 - mark as reference frame
     * 1 - mark as non-refernce frame
     */
    RK_U32  is_non_ref  : 1;

    /*
     * Valid when is_non_ref is false
     * 0 - mark as short-term reference frame
     * 1 - mark as long-term refernce frame
     */
    RK_U32  is_lt_ref   : 1;
    RK_U32  lt_idx      : 4;
    RK_U32  temporal_id : 4;
    RK_U32  stuff       : 20;
} MppEncFrmStatus;

typedef struct MppEncFrmRefInfo_t {
    MppEncFrmStatus     status;

    /* Reference relationship management */
    /* Only one reference frame is supported */
    RK_S32              ref_gop_idx;

    /* Referenced count. When zero it should be mark as non-reference */
    /* Generated at beginning of gop */
    RK_S32              valid_refer_cnt;
    RK_S32              refers[MAX_GOP_FMT_CNT];
} MppEncFrmRefInfo;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_enc_get_hier_info(MppEncFrmRefInfo **info, MppEncHierCfg *cfg);
MPP_RET mpp_enc_put_hier_info(MppEncFrmRefInfo *info);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_ENC_REFS_H__ */
