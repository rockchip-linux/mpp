/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Rockchip Products .                             --
--                                                                            --
--                   (C) COPYRIGHT 2014 ROCKCHIP PRODUCTS                     --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--
--------------------------------------------------------------------------------
--
--
--
------------------------------------------------------------------------------*/

#ifndef __H264_SEQUENCE_PARAMETER_SET_h__
#define __H264_SEQUENCE_PARAMETER_SET_h__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
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
    u32 timeScale;
    u32 numUnitsInTick;
    u32 bitStreamRestrictionFlag;
    u32 videoFullRange;
    u32 sarWidth;
    u32 sarHeight;
    u32 nalHrdParametersPresentFlag;
    u32 vclHrdParametersPresentFlag;
    u32 pictStructPresentFlag;
    u32 initialCpbRemovalDelayLength;
    u32 cpbRemovalDelayLength;
    u32 dpbOutputDelayLength;
    u32 timeOffsetLength;
    u32 bitRate;
    u32 cpbSize;
} vui_t;

typedef struct {
    true_e byteStream;
    u32 profileIdc;
    true_e constraintSet0;
    true_e constraintSet1;
    true_e constraintSet2;
    true_e constraintSet3;
    u32 levelIdc;
    u32 levelIdx;
    u32 seqParameterSetId;
    i32 log2MaxFrameNumMinus4;
    u32 picOrderCntType;
    u32 numRefFrames;
    true_e gapsInFrameNumValueAllowed;
    i32 picWidthInMbsMinus1;
    i32 picHeightInMapUnitsMinus1;
    true_e frameMbsOnly;
    true_e direct8x8Inference;
    true_e frameCropping;
    true_e vuiParametersPresent;
    vui_t vui;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;
} sps_s;

extern const u32 H264LevelIdc[];
extern const u32 H264MaxCPBS[];
extern const u32 H264MaxFS[];
extern const u32 H264SqrtMaxFS8[];
extern const u32 H264MaxMBPS[];
extern const u32 H264MaxBR[];

#define INVALID_LEVEL 0xFFFF

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264SeqParameterSetInit(sps_s * sps);
void H264SeqParameterSet(stream_s * stream, sps_s * sps);

void H264EndOfSequence(stream_s * stream, sps_s * sps);
void H264EndOfStream(stream_s * stream, sps_s * sps);

u32 H264GetLevelIndex(u32 levelIdc);

bool_e H264CheckLevel(sps_s * sps, i32 bitRate, i32 frameRateNum,
                      i32 frameRateDenom);

void H264SpsSetVuiTimigInfo(sps_s * sps, u32 timeScale, u32 numUnitsInTick);
void H264SpsSetVuiVideoInfo(sps_s * sps, u32 videoFullRange);
void H264SpsSetVuiAspectRatio(sps_s * sps, u32 sampleAspectRatioWidth,
                              u32 sampleAspectRatioHeight);
void H264SpsSetVuiPictStructPresentFlag(sps_s * sps, u32 flag);
void H264SpsSetVuiHrd(sps_s * sps, u32 present);
void H264SpsSetVuiHrdBitRate(sps_s * sps, u32 bitRate);
void H264SpsSetVuiHrdCpbSize(sps_s * sps, u32 cpbSize);
u32 H264SpsGetVuiHrdBitRate(sps_s * sps);
u32 H264SpsGetVuiHrdCpbSize(sps_s * sps);

#endif
