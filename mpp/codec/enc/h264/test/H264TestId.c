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
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "H264TestId.h"

#include <stdio.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void H264FrameQuantizationTest(h264Instance_s *inst);
static void H264SliceTest(h264Instance_s *inst);
static void H264StreamBufferLimitTest(h264Instance_s *inst);
static void H264MbQuantizationTest(h264Instance_s *inst);
static void H264FilterTest(h264Instance_s *inst);
static void H264UserDataTest(h264Instance_s *inst);
static void H264Intra16FavorTest(h264Instance_s *inst);
static void H264InterFavorTest(h264Instance_s *inst);
static void H264CroppingTest(h264Instance_s *inst);
static void H264DisableQPME(h264Instance_s *inst);
static void H264DisableRICE(h264Instance_s *inst);
static void H264RgbInputMaskTest(h264Instance_s *inst);
static void H264MadTest(h264Instance_s *inst);

/*------------------------------------------------------------------------------

    TestID defines a test configuration for the encoder. If the encoder control
    software is compiled with INTERNAL_TEST flag the test ID will force the
    encoder operation according to the test vector.

    TestID  Description
    0       No action, normal encoder operation
    1       Frame quantization test, adjust qp for every frame, qp = 0..51
    2       Slice test, adjust slice amount for each frame
    4       Stream buffer limit test, limit=500 (4kB) for first frame
    6       Checkpoint quantization test.
            Checkpoints with minimum and maximum values. Gradual increase of
            checkpoint values.
    7       Filter test, set disableDeblocking and filterOffsets A and B
    12      User data test
    15      Intra16Favor test, set to maximum value
    16      Cropping test, set cropping values for every frame
    17      Disable quarter pixel motion search
    18      Disable RICE
    19      RGB input mask test, set all values
    20      MAD test, test all MAD QP change values
    21      InterFavor test, set to maximum value

------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------

    H264ConfigureTestBeforeStream

    Function configures the encoder instance before starting stream encoding

------------------------------------------------------------------------------*/
void H264ConfigureTestBeforeStream(h264Instance_s * inst)
{
    ASSERT(inst);

    switch (inst->testId) {
    default:
        break;
    }

}

/*------------------------------------------------------------------------------

    H264ConfigureTestBeforeFrame

    Function configures the encoder instance before starting frame encoding

------------------------------------------------------------------------------*/
void H264ConfigureTestBeforeFrame(h264Instance_s * inst)
{
    ASSERT(inst);

    switch (inst->testId) {
    case 1:
        H264FrameQuantizationTest(inst);
        break;
    case 2:
        H264SliceTest(inst);
        break;
    case 4:
        H264StreamBufferLimitTest(inst);
        break;
    case 6:
        H264MbQuantizationTest(inst);
        break;
    case 7:
        H264FilterTest(inst);
        break;
    case 12:
        H264UserDataTest(inst);
        break;
    case 15:
        H264Intra16FavorTest(inst);
        break;
    case 16:
        H264CroppingTest(inst);
        break;
    case 17:
        H264DisableQPME(inst);
        break;
    case 18:
        H264DisableRICE(inst);
        break;
    case 19:
        H264RgbInputMaskTest(inst);
        break;
    case 20:
        H264MadTest(inst);
        break;
    case 21:
        H264InterFavorTest(inst);
        break;
    default:
        break;
    }

}

/*------------------------------------------------------------------------------
  H264QuantizationTest
------------------------------------------------------------------------------*/
void H264FrameQuantizationTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    /* Inter frame qp start zero */
    inst->rateControl.qpHdr = MIN(51, MAX(0, (vopNum - 1) % 52));

    ALOGV("H264FrameQuantTest# qpHdr %d\n", inst->rateControl.qpHdr);
}

/*------------------------------------------------------------------------------
  H264SliceTest
------------------------------------------------------------------------------*/
void H264SliceTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    inst->slice.sliceSize = (inst->mbPerRow * vopNum) % inst->mbPerFrame;

    ALOGV("H264SliceTest# sliceSize %d\n", inst->slice.sliceSize);
}

/*------------------------------------------------------------------------------
  H264StreamBufferLimitTest
------------------------------------------------------------------------------*/
void H264StreamBufferLimitTest(h264Instance_s *inst)
{
    static u32 firstFrame = 1;

    if (!firstFrame)
        return;

    firstFrame = 0;
    inst->asic.regs.outputStrmSize = 4000;

    ALOGV("H264StreamBufferLimitTest# streamBufferLimit %d bytes\n",
          inst->asic.regs.outputStrmSize);
}

/*------------------------------------------------------------------------------
  H264QuantizationTest
  NOTE: ASIC resolution for wordCntTarget and wordError is value/4
------------------------------------------------------------------------------*/
void H264MbQuantizationTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;
    h264QpCtrl_s *qc = &inst->rateControl.qpCtrl;
    h264RateControl_s *rc = &inst->rateControl;

    i32 tmp, i;
    i32 bitCnt = 0xffff / 200;

    rc->qpMin = MIN(51, vopNum / 4);
    rc->qpMax = MAX(0, 51 - vopNum / 4);

    rc->qpLastCoded = rc->qpTarget = rc->qpHdr = 26;

    for (i = 0; i < CHECK_POINTS_MAX; i++) {
        if (vopNum < 2) {
            tmp = 1;
        } else if (vopNum < 4) {
            tmp = 0xffff;
        } else {
            tmp = (vopNum *  bitCnt) / 4;
        }
        qc->wordCntTarget[i] = MIN(0xffff, tmp);
    }

    ALOGV("H264MbQuantTest# wordCntTarget[] %d\n", qc->wordCntTarget[0]);

    for (i = 0; i < CTRL_LEVELS; i++) {
        if (vopNum < 2) {
            tmp = -0x8000;
        } else if (vopNum < 4) {
            tmp = 0x7fff;
        } else {
            tmp = (-bitCnt / 2 * vopNum + vopNum * i * bitCnt / 5) / 4;
        }
        qc->wordError[i] = MIN(0x7fff, MAX(-0x8000, tmp));

        tmp = -8 + i * 3;
        if ((vopNum & 4) == 0) {
            tmp = -tmp;
        }
        qc->qpChange[i] = MIN(0x7, MAX(-0x8, tmp));

        ALOGV("                 wordError[%d] %d   qpChange[%d] %d\n",
              i, qc->wordError[i], i, qc->qpChange[i]);
    }

}

/*------------------------------------------------------------------------------
  H264FilterTest
------------------------------------------------------------------------------*/
void H264FilterTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;
    slice_s *slice = &inst->slice;

    slice->disableDeblocking = (vopNum / 2) % 3;
    if (vopNum == 0) {
        slice->filterOffsetA = -12;
        slice->filterOffsetB = 12;
    } else if (vopNum < 77) {
        if (vopNum % 6 == 0) {
            slice->filterOffsetA += 2;
            slice->filterOffsetB -= 2;
        }
    } else if (vopNum == 78) {
        slice->filterOffsetA = -12;
        slice->filterOffsetB = -12;
    } else if (vopNum < 155) {
        if (vopNum % 6 == 0) {
            slice->filterOffsetA += 2;
            slice->filterOffsetB += 2;
        }
    }

    ALOGV("H264FilterTest# disableDeblock = %d, filterOffA = %i filterOffB = %i\n",
          slice->disableDeblocking, slice->filterOffsetA,
          slice->filterOffsetB);

}

/*------------------------------------------------------------------------------
  H264UserDataTest
------------------------------------------------------------------------------*/
void H264UserDataTest(h264Instance_s *inst)
{
    static u8 *userDataBuf = NULL;
    i32 userDataLength = (16 + inst->frameCnt * 11) % 2048;
    i32 i;

    /* Allocate a buffer for user data, encoder reads data from this buffer
     * and writes it to the stream. This is never freed. */
#if 0
    if (!userDataBuf)
        userDataBuf = (u8*)malloc(2048);
#endif

    if (!userDataBuf)
        return;

    for (i = 0; i < userDataLength; i++) {
        /* Fill user data buffer with ASCII symbols from 48 to 125 */
        userDataBuf[i] = 48 + i % 78;
    }

    /* Enable user data insertion */
    inst->rateControl.sei.userDataEnabled = ENCHW_YES;
    inst->rateControl.sei.pUserData = userDataBuf;
    inst->rateControl.sei.userDataSize = userDataLength;

    ALOGV("H264UserDataTest# userDataSize %d\n", userDataLength);
}

/*------------------------------------------------------------------------------
  H264Intra16FavorTest
------------------------------------------------------------------------------*/
void H264Intra16FavorTest(h264Instance_s *inst)
{
    /* Force intra16 favor to maximum value */
    inst->asic.regs.intra16Favor = 65535;

    ALOGV("H264Intra16FavorTest# intra16Favor %d\n",
          inst->asic.regs.intra16Favor);
}

/*------------------------------------------------------------------------------
  H264InterFavorTest
------------------------------------------------------------------------------*/
void H264InterFavorTest(h264Instance_s *inst)
{
    /* Force combinations of inter favor and skip penalty values */

    if ((inst->frameCnt % 3) == 0) {
        inst->asic.regs.skipPenalty = 1;    /* default value */
        inst->asic.regs.interFavor = 65535;
    } else if ((inst->frameCnt % 3) == 1) {
        inst->asic.regs.skipPenalty = 0;
        inst->asic.regs.interFavor = 65535;
    } else {
        inst->asic.regs.skipPenalty = 0;
        inst->asic.regs.interFavor = 0;     /* default value */
    }

    ALOGV("H264InterFavorTest# interFavor %d skipPenalty %d\n",
          inst->asic.regs.interFavor, inst->asic.regs.skipPenalty);
}

/*------------------------------------------------------------------------------
  H264CroppingTest
------------------------------------------------------------------------------*/
void H264CroppingTest(h264Instance_s *inst)
{
    inst->preProcess.horOffsetSrc = inst->frameCnt % 8;
    inst->preProcess.verOffsetSrc = inst->frameCnt / 2;

    ALOGV("H264CroppingTest# horOffsetSrc %d  verOffsetSrc %d\n",
          inst->preProcess.horOffsetSrc, inst->preProcess.verOffsetSrc);
}

/*------------------------------------------------------------------------------
  H264DisableQPME
------------------------------------------------------------------------------*/
void H264DisableQPME(h264Instance_s *inst)
{
    inst->asic.regs.disableQuarterPixelMv = 1;

    ALOGV("H264DisableQPME# 1/4 pixel motion search disabled\n");
}

/*------------------------------------------------------------------------------
  H264DisableRICE
------------------------------------------------------------------------------*/
void H264DisableRICE(h264Instance_s *inst)
{
    inst->asic.regs.riceEnable = 0;

    ALOGV("H264DisableRICE# RICE disabled\n");
}

/*------------------------------------------------------------------------------
  H264RgbInputMaskTest
------------------------------------------------------------------------------*/
void H264RgbInputMaskTest(h264Instance_s *inst)
{
    u32 frameNum = (u32)inst->frameCnt;
    static u32 rMsb = 0;
    static u32 gMsb = 0;
    static u32 bMsb = 0;
    static u32 lsMask = 0;  /* Lowest possible mask position */
    static u32 msMask = 0;  /* Highest possible mask position */

    /* First frame normal
     * 1..29 step rMaskMsb values
     * 30..58 step gMaskMsb values
     * 59..87 step bMaskMsb values */
    if (frameNum == 0) {
        rMsb = inst->asic.regs.rMaskMsb;
        gMsb = inst->asic.regs.gMaskMsb;
        bMsb = inst->asic.regs.bMaskMsb;
        lsMask = MIN(rMsb, gMsb);
        lsMask = MIN(bMsb, lsMask);
        msMask = MAX(rMsb, gMsb);
        msMask = MAX(bMsb, msMask);
        if (msMask < 16)
            msMask = 15 - 2;  /* 16bit RGB, 13 mask positions: 3..15  */
        else
            msMask = 31 - 2;  /* 32bit RGB, 29 mask positions: 3..31 */
    } else if (frameNum <= msMask) {
        inst->asic.regs.rMaskMsb = MAX(frameNum + 2, lsMask);
        inst->asic.regs.gMaskMsb = gMsb;
        inst->asic.regs.bMaskMsb = bMsb;
    } else if (frameNum <= msMask * 2) {
        inst->asic.regs.rMaskMsb = rMsb;
        inst->asic.regs.gMaskMsb = MAX(frameNum - msMask + 2, lsMask);
        if (inst->asic.regs.inputImageFormat == 4)  /* RGB 565 special case */
            inst->asic.regs.gMaskMsb = MAX(frameNum - msMask + 2, lsMask + 1);
        inst->asic.regs.bMaskMsb = bMsb;
    } else if (frameNum <= msMask * 3) {
        inst->asic.regs.rMaskMsb = rMsb;
        inst->asic.regs.gMaskMsb = gMsb;
        inst->asic.regs.bMaskMsb = MAX(frameNum - msMask * 2 + 2, lsMask);
    } else {
        inst->asic.regs.rMaskMsb = rMsb;
        inst->asic.regs.gMaskMsb = gMsb;
        inst->asic.regs.bMaskMsb = bMsb;
    }

    ALOGV("H264RgbInputMaskTest#  %d %d %d\n", inst->asic.regs.rMaskMsb,
          inst->asic.regs.gMaskMsb, inst->asic.regs.bMaskMsb);
}

/*------------------------------------------------------------------------------
  H264MadTest
------------------------------------------------------------------------------*/
void H264MadTest(h264Instance_s *inst)
{
    u32 frameNum = (u32)inst->frameCnt;

    /* All values in range [-8,7] */
    inst->rateControl.mbQpAdjustment = -8 + (frameNum % 16);
    /* Step 256, range [0,63*256] */
    inst->mad.threshold = 256 * ((frameNum + 1) % 64);

    ALOGV("H264MadTest#  %d %d\n",
          inst->asic.regs.madThreshold,
          inst->asic.regs.madQpDelta);
}

#if 0
/*------------------------------------------------------------------------------
  MbPerInterruptTest
------------------------------------------------------------------------------*/
void MbPerInterruptTest(trace_s *trace)
{
    if (trace->testId != 3) {
        return;
    }

    trace->control.mbPerInterrupt = trace->vopNum;
}

/*------------------------------------------------------------------------------
H264FilterTest
------------------------------------------------------------------------------*/
void H264FilterTest(trace_s *trace, slice_s *slice)
{
    i32 vopNum = trace->vopNum;

    if (trace->testId != 7) {
        return;
    }

    slice->disableDeblocking = (vopNum / 2) % 3;
    if (vopNum == 0) {
        slice->filterOffsetA = -12;
        slice->filterOffsetB = 12;
    } else if (vopNum < 77) {
        if (vopNum % 6 == 0) {
            slice->filterOffsetA += 2;
            slice->filterOffsetB -= 2;
        }
    } else if (vopNum == 78) {
        slice->filterOffsetA = -12;
        slice->filterOffsetB = -12;
    } else if (vopNum < 155) {
        if (vopNum % 6 == 0) {
            slice->filterOffsetA += 2;
            slice->filterOffsetB += 2;
        }
    }
}

/*------------------------------------------------------------------------------
H264SliceQuantTest()  Change sliceQP from min->max->min.
------------------------------------------------------------------------------*/
void H264SliceQuantTest(trace_s *trace, slice_s *slice, mb_s *mb,
                        rateControl_s *rc)
{
    if (trace->testId != 8) {
        return;
    }

    rc->vopRc   = NO;
    rc->mbRc    = NO;
    rc->picSkip = NO;

    if (mb->qpPrev == rc->qpMin) {
        mb->qpLum = rc->qpMax;
    } else if (mb->qpPrev == rc->qpMax) {
        mb->qpLum = rc->qpMin;
    } else {
        mb->qpLum = rc->qpMax;
    }

    mb->qpCh  = qpCh[MIN(51, MAX(0, mb->qpLum + mb->chromaQpOffset))];
    rc->qp = rc->qpHdr = mb->qpLum;
    slice->sliceQpDelta = mb->qpLum - slice->picInitQpMinus26 - 26;
}

#endif

