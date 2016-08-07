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

#ifndef __H264_PICTURE_PARAMETER_SET_H__
#define __H264_PICTURE_PARAMETER_SET_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "H264PutBits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
    true_e byteStream;
    RK_S32 picParameterSetId;
    RK_S32 seqParameterSetId;
    true_e entropyCodingMode;
    true_e picOrderPresent;
    RK_S32 numSliceGroupsMinus1;
    RK_S32 numRefIdxL0ActiveMinus1;
    RK_S32 numRefIdxL1ActiveMinus1;
    true_e weightedPred;
    RK_S32 weightedBipredIdc;
    RK_S32 picInitQpMinus26;
    RK_S32 picInitQsMinus26;
    RK_S32 chromaQpIndexOffset;
    true_e deblockingFilterControlPresent;
    true_e constIntraPred;
    true_e redundantPicCntPresent;
    true_e transform8x8Mode;
} pps_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264PicParameterSetInit(pps_s * pps);
void H264PicParameterSet(stream_s * stream, pps_s * pps);

#endif
