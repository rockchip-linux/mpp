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

#ifndef H264_RATE_CONTROL_H
#define H264_RATE_CONTROL_H

#include "enccommon.h"
#include "H264Sei.h"

enum
{ H264RC_OVERFLOW = -1 };

#define RC_CBR_HRD  0   /* 1 = Constant bit rate model. Must use filler
                         * data to conform */

#define CTRL_LEVELS          7  /* DO NOT CHANGE THIS */
#define CHECK_POINTS_MAX    10  /* DO NOT CHANGE THIS */
#define RC_TABLE_LENGTH     10  /* DO NOT CHANGE THIS */

typedef struct {
    i32  a1;               /* model parameter */
    i32  a2;               /* model parameter */
    i32  qp_prev;          /* previous QP */
    i32  qs[15];           /* quantization step size */
    i32  bits[15];         /* Number of bits needed to code residual */
    i32  pos;              /* current position */
    i32  len;              /* current lenght */
    i32  zero_div;         /* a1 divisor is 0 */
} linReg_s;

typedef struct {
    i32 wordError[CTRL_LEVELS]; /* Check point error bit */
    i32 qpChange[CTRL_LEVELS];  /* Check point qp difference */
    i32 wordCntTarget[CHECK_POINTS_MAX];    /* Required bit count */
    i32 wordCntPrev[CHECK_POINTS_MAX];  /* Real bit count */
    i32 checkPointDistance;
    i32 checkPoints;
} h264QpCtrl_s;

/* Virtual buffer */
typedef struct {
    i32 bufferSize;          /* size of the virtual buffer */
    i32 bitRate;             /* input bit rate per second */
    i32 bitPerPic;           /* average number of bits per picture */
    i32 picTimeInc;          /* timeInc since last coded picture */
    i32 timeScale;           /* input frame rate numerator */
    i32 unitsInTic;          /* input frame rate denominator */
    i32 virtualBitCnt;       /* virtual (channel) bit count */
    i32 realBitCnt;          /* real bit count */
    i32 bufferOccupancy;     /* number of bits in the buffer */
    i32 skipFrameTarget;     /* how many frames should be skipped in a row */
    i32 skippedFrames;       /* how many frames have been skipped in a row */
    i32 nonZeroTarget;
    i32 bucketFullness;      /* Leaky Bucket fullness */
    /* new rate control */
    i32 gopRem;
    i32 windowRem;
} h264VirtualBuffer_s;

typedef struct {
    true_e picRc;
    true_e mbRc;             /* Mb header qp can vary, check point rc */
    true_e picSkip;          /* Frame Skip enable */
    true_e hrd;              /* HRD restrictions followed or not */
    u32 fillerIdx;
    i32 mbPerPic;            /* Number of macroblock per picture */
    i32 mbRows;              /* MB rows in picture */
    i32 coeffCntMax;         /* Number of coeff per picture */
    i32 nonZeroCnt;
    i32 srcPrm;              /* Source parameter */
    i32 qpSum;               /* Qp sum counter */
    u32 sliceTypeCur;
    u32 sliceTypePrev;
    true_e frameCoded;       /* Pic coded information */
    i32 fixedQp;             /* Pic header qp when fixed */
    i32 qpHdr;               /* Pic header qp of current voded picture */
    i32 qpMin;               /* Pic header minimum qp, user set */
    i32 qpMax;               /* Pic header maximum qp, user set */
    i32 qpHdrPrev;           /* Pic header qp of previous coded picture */
    i32 qpLastCoded;         /* Quantization parameter of last coded mb */
    i32 qpTarget;            /* Target quantrization parameter */
    u32 estTimeInc;
    i32 outRateNum;
    i32 outRateDenom;
    i32 gDelaySum;
    i32 gInitialDelay;
    i32 gInitialDoffs;
    h264QpCtrl_s qpCtrl;
    h264VirtualBuffer_s virtualBuffer;
    sei_s sei;
    i32 gBufferMin, gBufferMax;
    /* new rate control */
    linReg_s linReg;       /* Data for R-Q model */
    linReg_s overhead;
    linReg_s rError;       /* Rate prediction error (bits) */
    i32 targetPicSize;
    i32 sad;
    i32 frameBitCnt;
    /* for gop rate control */
    i32 gopQpSum;
    i32 gopQpDiv;
    u32 frameCnt;
    i32 gopLen;
    true_e roiRc;
    i32 roiQpHdr;
    i32 roiQpDelta;
    i32 roiStart;
    i32 roiLength;
    i32 intraQpDelta;
    u32 fixedIntraQp;
    i32 mbQpAdjustment;     /* QP delta for MAD macroblock QP adjustment */

    //add for rk30 new ratecontrol
    linReg_s intra;        /* Data for intra frames */
    linReg_s intraError;   /* Prediction error for intra frames */
    linReg_s gop;          /* Data for GOP */

    i32 gopBitCnt;          /* Current GOP bit count so far */
    i32 gopAvgBitCnt;       /* Previous GOP average bit count */

    i32 windowLen;          /* Bitrate window which tries to match target */
    i32 intraInterval;      /* Distance between two previous I-frames */
    i32 intraIntervalCtr;
} h264RateControl_s;

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

u32 H264FillerRc(h264RateControl_s * rc, u32 frameCnt);
i32 H264Calculate(i32 a, i32 b, i32 c);

bool_e H264InitRc(h264RateControl_s * rc);
void H264BeforePicRc(h264RateControl_s * rc, u32 timeInc, u32 sliceType);
i32 H264AfterPicRc(h264RateControl_s * rc, u32 nonZeroCnt, u32 byteCnt,
                   u32 qpSum);
#endif /* H264_RATE_CONTROL_H */

