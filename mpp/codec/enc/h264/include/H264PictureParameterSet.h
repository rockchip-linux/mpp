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
--  Abstract  :
--
------------------------------------------------------------------------------*/

#ifndef __H264_PICTURE_PARAMETER_SET_H__
#define __H264_PICTURE_PARAMETER_SET_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
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
    i32 picParameterSetId;
    i32 seqParameterSetId;
    true_e entropyCodingMode;
    true_e picOrderPresent;
    i32 numSliceGroupsMinus1;
    i32 numRefIdxL0ActiveMinus1;
    i32 numRefIdxL1ActiveMinus1;
    true_e weightedPred;
    i32 weightedBipredIdc;
    i32 picInitQpMinus26;
    i32 picInitQsMinus26;
    i32 chromaQpIndexOffset;
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
