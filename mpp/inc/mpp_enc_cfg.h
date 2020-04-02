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
#include "rc_data.h"

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
    MppEncOSDPltCfg     plt_cfg;
    MppEncOSDPlt        plt_data;
} MppEncCfgSet;

#endif /*__MPP_ENC_H__*/
