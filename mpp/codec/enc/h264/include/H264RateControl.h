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
    RK_S32  a1;               /* model parameter */
    RK_S32  a2;               /* model parameter */
    RK_S32  qp_prev;          /* previous QP */
    RK_S32  qs[15];           /* quantization step size */
    RK_S32  bits[15];         /* Number of bits needed to code residual */
    RK_S32  pos;              /* current position */
    RK_S32  len;              /* current lenght */
    RK_S32  zero_div;         /* a1 divisor is 0 */
} linReg_s;

typedef struct {
    RK_S32 wordError[CTRL_LEVELS]; /* Check point error bit */
    RK_S32 qpChange[CTRL_LEVELS];  /* Check point qp difference */
    RK_S32 wordCntTarget[CHECK_POINTS_MAX];    /* Required bit count */
    RK_S32 wordCntPrev[CHECK_POINTS_MAX];  /* Real bit count */
    RK_S32 checkPointDistance;
    RK_S32 checkPoints;
} h264QpCtrl_s;

/* Virtual buffer */
typedef struct {
    RK_S32 bufferSize;          /* size of the virtual buffer */
    RK_S32 bitRate;             /* input bit rate per second */
    RK_S32 bitPerPic;           /* average number of bits per picture */
    RK_S32 picTimeInc;          /* timeInc since last coded picture */
    RK_S32 timeScale;           /* input frame rate numerator */
    RK_S32 unitsInTic;          /* input frame rate denominator */
    RK_S32 virtualBitCnt;       /* virtual (channel) bit count */
    RK_S32 realBitCnt;          /* real bit count */
    RK_S32 bufferOccupancy;     /* number of bits in the buffer */
    RK_S32 skipFrameTarget;     /* how many frames should be skipped in a row */
    RK_S32 skippedFrames;       /* how many frames have been skipped in a row */
    RK_S32 nonZeroTarget;
    RK_S32 bucketFullness;      /* Leaky Bucket fullness */
    /* new rate control */
    RK_S32 gopRem;
    RK_S32 windowRem;
} h264VirtualBuffer_s;

typedef struct {
    true_e picRc;
    true_e mbRc;             /* Mb header qp can vary, check point rc */
    true_e picSkip;          /* Frame Skip enable */
    true_e hrd;              /* HRD restrictions followed or not */
    RK_U32 fillerIdx;
    RK_S32 mbPerPic;            /* Number of macroblock per picture */
    RK_S32 mbRows;              /* MB rows in picture */
    RK_S32 coeffCntMax;         /* Number of coeff per picture */
    RK_S32 nonZeroCnt;
    RK_S32 srcPrm;              /* Source parameter */
    RK_S32 qpSum;               /* Qp sum counter */
    RK_U32 sliceTypeCur;
    RK_U32 sliceTypePrev;
    true_e frameCoded;       /* Pic coded information */
    RK_S32 fixedQp;             /* Pic header qp when fixed */
    RK_S32 qpHdr;               /* Pic header qp of current voded picture */
    RK_S32 qpMin;               /* Pic header minimum qp, user set */
    RK_S32 qpMax;               /* Pic header maximum qp, user set */
    RK_S32 qpHdrPrev;           /* Pic header qp of previous coded picture */
    RK_S32 qpLastCoded;         /* Quantization parameter of last coded mb */
    RK_S32 qpTarget;            /* Target quantrization parameter */
    RK_U32 estTimeInc;
    RK_S32 outRateNum;
    RK_S32 outRateDenom;
    RK_S32 gDelaySum;
    RK_S32 gInitialDelay;
    RK_S32 gInitialDoffs;
    h264QpCtrl_s qpCtrl;
    h264VirtualBuffer_s virtualBuffer;
    sei_s sei;
    RK_S32 gBufferMin, gBufferMax;
    /* new rate control */
    linReg_s linReg;       /* Data for R-Q model */
    linReg_s overhead;
    linReg_s rError;       /* Rate prediction error (bits) */
    RK_S32 targetPicSize;
    RK_S32 sad;
    RK_S32 frameBitCnt;
    /* for gop rate control */
    RK_S32 gopQpSum;
    RK_S32 gopQpDiv;
    RK_U32 frameCnt;
    RK_S32 gopLen;
    true_e roiRc;
    RK_S32 roiQpHdr;
    RK_S32 roiQpDelta;
    RK_S32 roiStart;
    RK_S32 roiLength;
    RK_S32 intraQpDelta;
    RK_U32 fixedIntraQp;
    RK_S32 mbQpAdjustment;     /* QP delta for MAD macroblock QP adjustment */

    //add for rk30 new ratecontrol
    linReg_s intra;        /* Data for intra frames */
    linReg_s intraError;   /* Prediction error for intra frames */
    linReg_s gop;          /* Data for GOP */

    RK_S32 gopBitCnt;          /* Current GOP bit count so far */
    RK_S32 gopAvgBitCnt;       /* Previous GOP average bit count */

    RK_S32 windowLen;          /* Bitrate window which tries to match target */
    RK_S32 intraInterval;      /* Distance between two previous I-frames */
    RK_S32 intraIntervalCtr;
} h264RateControl_s;

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

RK_U32 H264FillerRc(h264RateControl_s * rc, RK_U32 frameCnt);
RK_S32 H264Calculate(RK_S32 a, RK_S32 b, RK_S32 c);

bool_e H264InitRc(h264RateControl_s * rc);
void H264BeforePicRc(h264RateControl_s * rc, RK_U32 timeInc, RK_U32 sliceType);
RK_S32 H264AfterPicRc(h264RateControl_s * rc, RK_U32 nonZeroCnt, RK_U32 byteCnt,
                   RK_U32 qpSum);
#endif /* H264_RATE_CONTROL_H */

