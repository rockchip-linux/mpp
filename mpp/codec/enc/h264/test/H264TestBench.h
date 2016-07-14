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

#ifndef _H264TESTBENCH_H_
#define _H264TESTBENCH_H_

#include "basetype.h"
#include "h264encapi.h"

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define DEFAULT -100
#define MAX_BPS_ADJUST 20

/* Structure for command line options */
typedef struct {
    char *input;
    char *output;
    char *userData;
    i32 firstPic;
    i32 lastPic;
    i32 width;
    i32 height;
    i32 lumWidthSrc;
    i32 lumHeightSrc;
    i32 horOffsetSrc;
    i32 verOffsetSrc;
    i32 outputRateNumer;
    i32 outputRateDenom;
    i32 inputRateNumer;
    i32 inputRateDenom;
    i32 level;
    i32 hrdConformance;
    i32 cpbSize;
    i32 intraPicRate;
    i32 constIntraPred;
    i32 disableDeblocking;
    i32 mbPerSlice;
    i32 qpHdr;
    i32 qpMin;
    i32 qpMax;
    i32 bitPerSecond;
    i32 picRc;
    i32 mbRc;
    i32 picSkip;
    i32 rotation;
    i32 inputFormat;
    i32 colorConversion;
    i32 videoBufferSize;
    i32 videoRange;
    i32 chromaQpOffset;
    i32 filterOffsetA;
    i32 filterOffsetB;
    i32 trans8x8;
    i32 enableCabac;
    i32 cabacInitIdc;
    i32 testId;
    i32 burst;
    i32 bursttype;
    i32 quarterPixelMv;
    i32 sei;
    i32 byteStream;
    i32 videoStab;
    i32 gopLength;
    i32 intraQpDelta;
    i32 fixedIntraQp;
    i32 mbQpAdjustment;
    i32 testParam;
    i32 psnr;
    i32 bpsAdjustFrame[MAX_BPS_ADJUST];
    i32 bpsAdjustBitrate[MAX_BPS_ADJUST];
    i32 framerateout;
} commandLine_s;

void TestNaluSizes(i32 * pNaluSizes, u8 * stream, u32 strmSize, u32 picBytes);


#endif
