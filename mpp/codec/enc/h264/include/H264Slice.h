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

#ifndef __H264_SLICE_H__
#define __H264_SLICE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "H264PutBits.h"
#include "H264NalUnit.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef enum {
    PSLICE = 0,
    ISLICE = 2,
    PSLICES = 5,
    ISLICES = 7
} sliceType_e;

typedef struct {
    true_e byteStream;
    u32 sliceSize;
    sliceType_e sliceType;
    nalUnitType_e nalUnitType;
    u32 picParameterSetId;
    u32 prevFrameNum;
    u32 frameNum;
    u32 frameNumBits;
    u32 idrPicId;
    u32 nalRefIdc;
    u32 disableDeblocking;
    i32 filterOffsetA;
    i32 filterOffsetB;
    u32 cabacInitIdc;
} slice_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264SliceInit(slice_s * slice);

#endif
