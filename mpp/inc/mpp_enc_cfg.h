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

#ifndef __MPP_ENC_CFG_H__
#define __MPP_ENC_CFG_H__

#include "rk_venc_cmd.h"

typedef struct EncFrmStatus_t {
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

    /*
     * distance between current frame and reference frame
     */
    RK_S32  ref_dist    : 16;

    /*
     * reencode flag and force pskip flag
     */
    RK_U32  reencode    : 1;
    RK_U32  force_pskip : 1;

    RK_U32  stuff       : 2;
} EncFrmStatus;

/*
 * MppEncCfgSet shows the relationship between different configuration
 * Due to the huge amount of configurable parameters we need to setup
 * only minimum amount of necessary parameters.
 *
 * For normal user rc and prep config are enough.
 */
typedef struct MppEncCfgSet_t {
    // esential config
    MppEncPrepCfg       prep;
    MppEncRcCfg         rc;

    // codec detail config
    MppEncCodecCfg      codec;

    MppEncSliceSplit    split;
    MppEncGopRef        gop_ref;
    MppEncROICfg        roi;
    MppEncOSDData       osd_data;
    MppEncOSDPlt        osd_plt;
} MppEncCfgSet;

#endif /*__MPP_ENC_H__*/
