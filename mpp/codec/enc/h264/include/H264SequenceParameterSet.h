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

#ifndef __H264_SEQUENCE_PARAMETER_SET_h__
#define __H264_SEQUENCE_PARAMETER_SET_h__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "H264PutBits.h"
#include "H264Slice.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

typedef struct {
    RK_U32 timeScale;
    RK_U32 numUnitsInTick;
    RK_U32 bitStreamRestrictionFlag;
    RK_U32 videoFullRange;
    RK_U32 sarWidth;
    RK_U32 sarHeight;
    RK_U32 nalHrdParametersPresentFlag;
    RK_U32 vclHrdParametersPresentFlag;
    RK_U32 pictStructPresentFlag;
    RK_U32 initialCpbRemovalDelayLength;
    RK_U32 cpbRemovalDelayLength;
    RK_U32 dpbOutputDelayLength;
    RK_U32 timeOffsetLength;
    RK_U32 bitRate;
    RK_U32 cpbSize;
} vui_t;

typedef struct {
    true_e byteStream;
    RK_U32 profileIdc;
    true_e constraintSet0;
    true_e constraintSet1;
    true_e constraintSet2;
    true_e constraintSet3;
    RK_U32 levelIdc;
    RK_U32 levelIdx;
    RK_U32 seqParameterSetId;
    RK_S32 log2MaxFrameNumMinus4;
    RK_U32 picOrderCntType;
    RK_U32 numRefFrames;
    true_e gapsInFrameNumValueAllowed;
    RK_S32 picWidthInMbsMinus1;
    RK_S32 picHeightInMapUnitsMinus1;
    true_e frameMbsOnly;
    true_e direct8x8Inference;
    true_e frameCropping;
    true_e vuiParametersPresent;
    vui_t vui;
    RK_U32 frameCropLeftOffset;
    RK_U32 frameCropRightOffset;
    RK_U32 frameCropTopOffset;
    RK_U32 frameCropBottomOffset;
} sps_s;

extern const RK_U32 H264LevelIdc[];
extern const RK_U32 H264MaxCPBS[];
extern const RK_U32 H264MaxFS[];
extern const RK_U32 H264SqrtMaxFS8[];
extern const RK_U32 H264MaxMBPS[];
extern const RK_U32 H264MaxBR[];

#define INVALID_LEVEL 0xFFFF

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264SeqParameterSetInit(sps_s * sps);
void H264SeqParameterSet(stream_s * stream, sps_s * sps);

void H264EndOfSequence(stream_s * stream, sps_s * sps);
void H264EndOfStream(stream_s * stream, sps_s * sps);

RK_U32 H264GetLevelIndex(RK_U32 levelIdc);

bool_e H264CheckLevel(sps_s * sps, RK_S32 bitRate, RK_S32 frameRateNum,
                      RK_S32 frameRateDenom);

void H264SpsSetVuiTimigInfo(sps_s * sps, RK_U32 timeScale, RK_U32 numUnitsInTick);
void H264SpsSetVuiVideoInfo(sps_s * sps, RK_U32 videoFullRange);
void H264SpsSetVuiAspectRatio(sps_s * sps, RK_U32 sampleAspectRatioWidth,
                              RK_U32 sampleAspectRatioHeight);
void H264SpsSetVuiPictStructPresentFlag(sps_s * sps, RK_U32 flag);
void H264SpsSetVuiHrd(sps_s * sps, RK_U32 present);
void H264SpsSetVuiHrdBitRate(sps_s * sps, RK_U32 bitRate);
void H264SpsSetVuiHrdCpbSize(sps_s * sps, RK_U32 cpbSize);
RK_U32 H264SpsGetVuiHrdBitRate(sps_s * sps);
RK_U32 H264SpsGetVuiHrdCpbSize(sps_s * sps);

#endif
