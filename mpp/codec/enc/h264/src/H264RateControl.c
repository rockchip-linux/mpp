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

/*
 * Using only one leaky bucket (Multible buckets is supported by std).
 * Constant bit rate (CBR) operation, ie. leaky bucket drain rate equals
 * average rate of the stream, is enabled if RC_CBR_HRD = 1. Frame skiping and
 * filler data are minimum requirements for the CBR conformance.
 *
 * Constant HRD parameters:
 *   low_delay_hrd_flag = 0, assumes constant delay mode.
 *       cpb_cnt_minus1 = 0, only one leaky bucket used.
 *         (cbr_flag[0] = RC_CBR_HRD, CBR mode.)
 */
#include "H264RateControl.h"
#include "H264Slice.h"
#ifdef ROI_SUPPORT
#include "H264RoiModel.h"
#endif

/*------------------------------------------------------------------------------
  Module defines
------------------------------------------------------------------------------*/

#ifdef TRACE_RC
#include <stdio.h>
FILE *fpRcTrc = NULL;
/* Select debug output: fpRcTrc or stdout */
#define DBGOUTPUT fpRcTrc
/* Select debug level: 0 = minimum, 2 = maximum */
#define DBG_LEVEL 2
#define DBG(l, str) if (l <= DBG_LEVEL) fprintf str
#else
#define DBG(l, str)
#endif

#define INITIAL_BUFFER_FULLNESS   60    /* Decoder Buffer in procents */
#define MIN_PIC_SIZE              50    /* Procents from picPerPic */

#define CLIP3(min, max, val)    ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
#define DIV(a, b)       (((a) + (SIGN(a) * (b)) / 2) / (b))
#define DSCY                      32 /* n * 32 */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define QP_DELTA          2
#define INTRA_QP_DELTA    (0)
#define WORD_CNT_MAX      65535

/*------------------------------------------------------------------------------
  Local structures
------------------------------------------------------------------------------*/
/* q_step values scaled up by 4 and evenly rounded */
static const i32 q_step[53] = { 3, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11,
                                13, 14, 16, 18, 20, 23, 25, 28, 32, 36, 40, 45, 51, 57, 64, 72, 80, 90,
                                101, 114, 128, 144, 160, 180, 203, 228, 256, 288, 320, 360, 405, 456,
                                513, 577, 640, 720, 810, 896
                              };

/*------------------------------------------------------------------------------
  Local function prototypes
------------------------------------------------------------------------------*/

//static i32 InitialQp(i32 bits, i32 pels);  // mask by lance 2016.05.12
//static void MbQuant(h264RateControl_s * rc);  // mask by lance 2016.05.12
//static void LinearModel(h264RateControl_s * rc);  // maks by lance 2016.05.12
//static void AdaptiveModel(h264RateControl_s * rc);  // mask by lance 2016.05.12
//static void SourceParameter(h264RateControl_s * rc, i32 nonZeroCnt);  // mask by lance 2016.05.12
//static void PicSkip(h264RateControl_s * rc);  // mask by lance 2016.05.12
//static void PicQuantLimit(h264RateControl_s * rc);  // mask by lance 2016.05.12
//static i32 VirtualBuffer(h264VirtualBuffer_s *vb, i32 timeInc, true_e hrd);  // mask by lance 2016.05.12
//static void PicQuant(h264RateControl_s * rc);  // mask by lance 2016.05.12
//static i32 avg_rc_error(linReg_s *p);  // mask by lance 2016.05.12
//static void update_rc_error(linReg_s *p, i32 bits);  // mask by lance 2016.05.12
//static i32 avg_overhead(linReg_s *p);  // mask by lance 2016.05.12
//static void update_overhead(linReg_s *p, i32 bits, i32 clen);  // mask by lance 2016.05.12
//static i32 gop_avg_qp(h264RateControl_s *rc);  // mask by lance 2016.05.12
//static i32 new_pic_quant(linReg_s *p, i32 bits, true_e useQpDeltaLimit);  // mask by lance 2016.05.12
//static void update_tables(linReg_s *p, i32 qp, i32 bits);  // mask by lance 2016.05.12
//static void update_model(linReg_s *p);  // mask by lance 2016.05.12
//static i32 lin_sy(i32 *qp, i32 *r, i32 n);  // mask by lance 2016.05.12
//static i32 lin_sx(i32 *qp, i32 n);  // mask by lance 2016.05.12
//static i32 lin_sxy(i32 *qp, i32 *r, i32 n);  // mask by lance 2016.05.12
//static i32 lin_nsxx(i32 *qp, i32 n);  // mask by lance 2016.05.12

/*------------------------------------------------------------------------------

  InitialQp()  Returns sequence initial quantization parameter.

------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 InitialQp(i32 bits, i32 pels)
{
    const i32 qp_tbl[2][9] = {
        {27, 44, 72, 119, 192, 314, 453, 653, 0x7FFFFFFF},
        /*{26, 38, 59, 96, 173, 305, 545, 0x7FFFFFFF},*/
        {49, 45, 41, 37, 33, 29, 25, 21, 17}
    };
    const i32 upscale = 1000;
    i32 i = -1;

    /* prevents overflow, QP would anyway be 17 with this high bitrate
       for all resolutions under and including 1920x1088 */
    if (bits > 1000000)
        return 17;

    /* Make room for multiplication */
    pels >>= 8;
    bits >>= 2;

    /* Adjust the bits value for the current resolution */
    bits *= pels + 250;
    ASSERT(pels > 0);
    ASSERT(bits > 0);
    bits /= 350 + (3 * pels) / 4;
    bits = H264Calculate(bits, upscale, pels << 6);

    while (qp_tbl[0][++i] < bits);

    return qp_tbl[1][i];
}
#endif

/*------------------------------------------------------------------------------

      H264FillerRc

      Stream watermarking. Insert filler NAL unit of certain size after each
      Nth frame.

------------------------------------------------------------------------------*/
u32 H264FillerRc(h264RateControl_s * rc, u32 frameCnt)
{
    const u8 filler[] = { 0, 9, 0, 9, 9, 9, 0, 2, 2, 0 };
    u32 idx;

    if (rc->fillerIdx == (u32) (-1)) {
        rc->fillerIdx = sizeof(filler) / sizeof(*filler) - 1;
    }

    idx = rc->fillerIdx;
    if (frameCnt != 0 && ((frameCnt % 128) == 0)) {
        idx++;
    }
    idx %= sizeof(filler) / sizeof(*filler);

    if (idx != rc->fillerIdx) {
        rc->fillerIdx = idx;
        return filler[idx] + 1;
    }
    return 0;
}
/*------------------------------------------------------------------------------
  VirtualBuffer()  Return difference of target and real buffer fullness.
  Virtual buffer and real bit count grow until one second.  After one second
  output bit rate per second is removed from virtualBitCnt and realBitCnt. Bit
  drifting has been taken care.

  If the leaky bucket in VBR mode becomes empty (e.g. underflow), those R * T_e
  bits are lost and must be decremented from virtualBitCnt. (NOTE: Drift
  calculation will mess virtualBitCnt up, so the loss is added to realBitCnt)
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 VirtualBuffer(h264VirtualBuffer_s *vb, i32 timeInc, true_e hrd)
{
    i32 drift, target, bitPerPic = vb->bitPerPic;
    if (hrd) {
#if RC_CBR_HRD
        /* In CBR mode, bucket _must not_ underflow. Insert filler when
         * needed. */
        vb->bucketFullness -= bitPerPic;
#else
        if (vb->bucketFullness >= bitPerPic) {
            vb->bucketFullness -= bitPerPic;
        } else {
            vb->realBitCnt += (bitPerPic - vb->bucketFullness);
            vb->bucketFullness = 0;
        }
#endif
    }

    /* Saturate realBitCnt, this is to prevent overflows caused by much greater
       bitrate setting than is really possible to reach */
    if (vb->realBitCnt > 0x1FFFFFFF)
        vb->realBitCnt = 0x1FFFFFFF;
    if (vb->realBitCnt < -0x1FFFFFFF)
        vb->realBitCnt = -0x1FFFFFFF;

    vb->picTimeInc    += timeInc;
    vb->virtualBitCnt += H264Calculate(vb->bitRate, timeInc, vb->timeScale);
    target = vb->virtualBitCnt - vb->realBitCnt;

    /* Saturate target, prevents rc going totally out of control.
       This situation should never happen. */
    if (target > 0x1FFFFFFF)
        target = 0x1FFFFFFF;
    if (target < -0x1FFFFFFF)
        target = -0x1FFFFFFF;

    /* picTimeInc must be in range of [0, timeScale) */
    while (vb->picTimeInc >= vb->timeScale) {
        vb->picTimeInc    -= vb->timeScale;
        vb->virtualBitCnt -= vb->bitRate;
        vb->realBitCnt    -= vb->bitRate;
    }
    drift = H264Calculate(vb->bitRate, vb->picTimeInc, vb->timeScale);
    drift -= vb->virtualBitCnt;
    vb->virtualBitCnt += drift;

    DBG(1, (DBGOUTPUT, "virtualBitCnt:\t\t%6i  realBitCnt: %i  ",
            vb->virtualBitCnt, vb->realBitCnt));
    DBG(1, (DBGOUTPUT, "target: %i  timeInc: %i\n", target, timeInc));
    return target;
}
#endif
/*------------------------------------------------------------------------------

  MbQuant()

------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void MbQuant(h264RateControl_s * rc)
{
    i32 nonZeroTarget;

    /* Disable Mb Rc for Intra Slices, because coeffTarget will be wrong */
    if (rc->sliceTypeCur == ISLICE || rc->sliceTypeCur == ISLICES ||
        rc->srcPrm == 0) {
        return;
    }

    /* Required zero cnt */
    nonZeroTarget = H264Calculate(rc->targetPicSize, 256, rc->srcPrm);
    nonZeroTarget = MIN(rc->coeffCntMax, MAX(0, nonZeroTarget));

    nonZeroTarget = MIN(0x7FFFFFFFU / 1024U, (u32)nonZeroTarget);

    rc->virtualBuffer.nonZeroTarget = nonZeroTarget;

    /* Use linear model when previous frame can't be used for prediction */
    if ((rc->sliceTypeCur != rc->sliceTypePrev) || (rc->nonZeroCnt == 0)) {
        LinearModel(rc);
    } else {
        AdaptiveModel(rc);
    }
}
#endif

/*------------------------------------------------------------------------------

  LinearModel()

------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void LinearModel(h264RateControl_s * rc)
{
    const i32 sscale = 1024;
    h264QpCtrl_s *qc = &rc->qpCtrl;
    i32 scaler;
    i32 i;
    i32 tmp, nonZeroTarget = rc->virtualBuffer.nonZeroTarget;

    ASSERT(nonZeroTarget < (0x7FFFFFFF / sscale));

    if (nonZeroTarget > 0) {
        scaler = H264Calculate(nonZeroTarget, sscale, (i32) rc->mbPerPic);
    } else {
        return;
    }

    DBG(1, (DBGOUTPUT, " Linear Target: %8d prevCnt:\t %6d Scaler:\t %6d\n",
            nonZeroTarget, rc->nonZeroCnt, scaler / sscale));

    for (i = 0; i < rc->qpCtrl.checkPoints; i++) {
        tmp = (scaler * (qc->checkPointDistance * (i + 1) + 1)) / sscale;
        tmp = MIN(WORD_CNT_MAX, tmp / 4 + 1);
        qc->wordCntTarget[i] = tmp;
    }

    /* calculate nz count for avg. bits per frame */
    tmp = H264Calculate(rc->virtualBuffer.bitPerPic, 256, rc->srcPrm);

    DBG(1, (DBGOUTPUT, "Error Limit:\t %8d SrcPrm:\t %6d\n",
            tmp, rc->srcPrm / 256));

    qc->wordError[0] = -tmp * 3;
    qc->qpChange[0] = -3;
    qc->wordError[1] = -tmp * 2;
    qc->qpChange[1] = -2;
    qc->wordError[2] = -tmp * 1;
    qc->qpChange[2] = -1;
    qc->wordError[3] = tmp * 1;
    qc->qpChange[3] = 0;
    qc->wordError[4] = tmp * 2;
    qc->qpChange[4] = 1;
    qc->wordError[5] = tmp * 3;
    qc->qpChange[5] = 2;
    qc->wordError[6] = tmp * 4;
    qc->qpChange[6] = 3;

    for (i = 0; i < CTRL_LEVELS; i++) {
        tmp = qc->wordError[i];
        tmp = CLIP3(-32768, 32767, tmp / 4);
        qc->wordError[i] = tmp;
    }
}
#endif

/*------------------------------------------------------------------------------

  AdaptiveModel()

------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void AdaptiveModel(h264RateControl_s * rc)
{
    const i32 sscale = 1024;
    h264QpCtrl_s *qc = &rc->qpCtrl;
    i32 i;
    i32 tmp, nonZeroTarget = rc->virtualBuffer.nonZeroTarget;
    i32 scaler;

    ASSERT(nonZeroTarget < (0x7FFFFFFF / sscale));

    if ((nonZeroTarget > 0) && (rc->nonZeroCnt > 0)) {
        scaler = H264Calculate(nonZeroTarget, sscale, rc->nonZeroCnt);
    } else {
        return;
    }
    DBG(1, (DBGOUTPUT, "Adaptive Target: %8d prevCnt:\t %6d Scaler:\t %6d\n",
            nonZeroTarget, rc->nonZeroCnt, scaler / sscale));

    for (i = 0; i < rc->qpCtrl.checkPoints; i++) {
        tmp = (i32) (qc->wordCntPrev[i] * scaler) / sscale;
        tmp = MIN(WORD_CNT_MAX, tmp / 4 + 1);
        qc->wordCntTarget[i] = tmp;
        DBG(2, (DBGOUTPUT, " CP %i  wordCntPrev %6i  wordCntTarget %6i\n",
                i, qc->wordCntPrev[i], qc->wordCntTarget[i]));
    }

    /* Qp change table */

    /* calculate nz count for avg. bits per frame */
    tmp = H264Calculate(rc->virtualBuffer.bitPerPic, 256, (rc->srcPrm * 3));

    DBG(1, (DBGOUTPUT, "Error Limit:\t %8d SrcPrm:\t %6d\n",
            tmp, rc->srcPrm / 256));

    qc->wordError[0] = -tmp * 3;
    qc->qpChange[0] = -3;
    qc->wordError[1] = -tmp * 2;
    qc->qpChange[1] = -2;
    qc->wordError[2] = -tmp * 1;
    qc->qpChange[2] = -1;
    qc->wordError[3] = tmp * 1;
    qc->qpChange[3] = 0;
    qc->wordError[4] = tmp * 2;
    qc->qpChange[4] = 1;
    qc->wordError[5] = tmp * 3;
    qc->qpChange[5] = 2;
    qc->wordError[6] = tmp * 4;
    qc->qpChange[6] = 3;

    for (i = 0; i < CTRL_LEVELS; i++) {
        tmp = qc->wordError[i];
        tmp = CLIP3(-32768, 32767, tmp / 4);
        qc->wordError[i] = tmp;
    }
}
#endif

/*------------------------------------------------------------------------------

  SourceParameter()  Source parameter of last coded frame. Parameters
  has been scaled up by factor 256.

------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void SourceParameter(h264RateControl_s * rc, i32 nonZeroCnt)
{
    ASSERT(rc->qpSum <= 51 * rc->mbPerPic);
    ASSERT(nonZeroCnt <= rc->coeffCntMax);
    ASSERT(nonZeroCnt >= 0 && rc->coeffCntMax >= 0);

    /* AVOID division by zero */
    if (nonZeroCnt == 0) {
        nonZeroCnt = 1;
    }

    rc->srcPrm = H264Calculate(rc->frameBitCnt, 256, nonZeroCnt);

    DBG(1, (DBGOUTPUT, "nonZeroCnt %6i, srcPrm %i\n",
            nonZeroCnt, rc->srcPrm / 256));

}
#endif

/*------------------------------------------------------------------------------
  PicSkip()  Decrease framerate if not enough bits available.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void PicSkip(h264RateControl_s * rc)
{
    h264VirtualBuffer_s *vb = &rc->virtualBuffer;
    i32 bitAvailable = vb->virtualBitCnt - vb->realBitCnt;
    i32 skipIncLimit = -vb->bitPerPic / 3;
    i32 skipDecLimit = vb->bitPerPic / 3;

    /* When frameRc is enabled, skipFrameTarget is not allowed to be > 1
     * This makes sure that not too many frames is skipped and lets
     * the frameRc adjust QP instead of skipping many frames */
    if (((rc->picRc == ENCHW_NO) || (vb->skipFrameTarget == 0)) &&
        (bitAvailable < skipIncLimit)) {
        vb->skipFrameTarget++;
    }

    if ((bitAvailable > skipDecLimit) && vb->skipFrameTarget > 0) {
        vb->skipFrameTarget--;
    }

    if (vb->skippedFrames < vb->skipFrameTarget) {
        vb->skippedFrames++;
        rc->frameCoded = ENCHW_NO;
    } else {
        vb->skippedFrames = 0;
    }
}
#endif

/*------------------------------------------------------------------------------

  PicQuant()  Calculate quantization parameter for next frame. In the beginning
                of GOP use previous GOP average QP and otherwise find new QP
                using the target size and previous frames QPs and bit counts.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void PicQuant(h264RateControl_s * rc)
{
    i32 tmp = 0;
    i32 avgOverhead, tmpValue, avgRcError;
    true_e useQpDeltaLimit = ENCHW_YES;

    if (rc->picRc != ENCHW_YES) {
        rc->qpHdr = rc->fixedQp;
        DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: xx xx D:  xxxx newQP: xx\n"));
        return;
    }

    /* If HRD is enabled we must make sure this frame fits in buffer */
    if (rc->hrd == ENCHW_YES) {
        i32 bitsAvailable =
            (rc->virtualBuffer.bufferSize - rc->virtualBuffer.bucketFullness);

        /* If the previous frame didn't fit the buffer we don't limit QP change */
        if (rc->frameBitCnt > bitsAvailable) {
            useQpDeltaLimit = ENCHW_NO;
        }
    }

    /* determine initial quantization parameter for current picture */
    if (rc->sliceTypeCur == ISLICE || rc->sliceTypeCur == ISLICES) {
        avgOverhead = avg_overhead(&rc->overhead);
        /* If all frames or every other frame is intra we calculate new QP
         * for intra the same way as for inter */
        if (rc->gopLen == 1 || rc->gopLen == 2) {
            tmp = new_pic_quant(&rc->linReg, H264Calculate(
                                    rc->targetPicSize - avgOverhead,
                                    131072, rc->sad), useQpDeltaLimit);
        } else {
            DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: xx xx D:  xxxx newQP: xx\n"));
            tmp = gop_avg_qp(rc);
        }
        if (tmp) {
            rc->qpHdr = tmp;
        }
    } else if (rc->sliceTypePrev == ISLICE || rc->sliceTypePrev == ISLICES) {
        /* Previous frame was intra, use the same QP */
        DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: == == D:  ==== newQP: ==\n"));
        rc->qpHdr = rc->qpHdrPrev;
    } else {
        /* Calculate new QP by matching to previous frames R-Q curve */
        avgRcError = avg_rc_error(&rc->rError);
        avgOverhead = avg_overhead(&rc->overhead);
        tmpValue = H264Calculate(rc->targetPicSize  - avgRcError -
                                 avgOverhead, 131072, rc->sad);
        rc->qpHdr = new_pic_quant(&rc->linReg, tmpValue, useQpDeltaLimit);
    }
}
#endif

/*------------------------------------------------------------------------------

  PicQuantLimit()

------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void PicQuantLimit(h264RateControl_s * rc)
{
    rc->qpHdr = MIN(rc->qpMax, MAX(rc->qpMin, rc->qpHdr));
}
#endif

/*------------------------------------------------------------------------------

  Calculate()  I try to avoid overflow and calculate good enough result of a*b/c

------------------------------------------------------------------------------*/
i32 H264Calculate(i32 a, i32 b, i32 c)
{
    u32 left = 32;
    u32 right = 0;
    u32 shift;
    i32 sign = 1;
    i32 tmp;

    if (a == 0 || b == 0) {
        return 0;
    } else if ((a * b / b) == a && c != 0) {
        return (a * b / c);
    }
    if (a < 0) {
        sign = -1;
        a = -a;
    }
    if (b < 0) {
        sign *= -1;
        b = -b;
    }
    if (c < 0) {
        sign *= -1;
        c = -c;
    }

    if (c == 0 ) {
        return 0x7FFFFFFF * sign;
    }

    if (b > a) {
        tmp = b;
        b = a;
        a = tmp;
    }

    for (--left; (((u32)a << left) >> left) != (u32)a; --left);
    left--; /* unsigned values have one more bit on left,
               we want signed accuracy. shifting signed values gives
               lint warnings */

    while (((u32)b >> right) > (u32)c) {
        right++;
    }

    if (right > left) {
        return 0x7FFFFFFF * sign;
    } else {
        shift = left - right;
        return (i32)((((u32)a << shift) / (u32)c * (u32)b) >> shift) * sign;
    }
}

/*------------------------------------------------------------------------------
  avg_rc_error()  PI(D)-control for rate prediction error.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 avg_rc_error(linReg_s *p)
{
    return DIV(p->bits[2] * 4 + p->bits[1] * 6 + p->bits[0] * 0, 100);
}
#endif

/*------------------------------------------------------------------------------
  update_overhead()  Update PI(D)-control values
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static void update_rc_error(linReg_s *p, i32 bits)
{
    p->len = 3;

    if (bits == (i32)0x7fffffff) {
        /* RESET */
        p->bits[0] = 0;
        p->bits[1] = 0;
        p->bits[2] = 0;
        return;
    }
    p->bits[0] = bits - p->bits[2]; /* Derivative */
    p->bits[1] = bits + p->bits[1]; /* Integral */
    p->bits[2] = bits;              /* Proportional */
    DBG(1, (DBGOUTPUT, "P %6d I %7d D %7d\n", p->bits[2],  p->bits[1], p->bits[0]));
}
#endif

/*------------------------------------------------------------------------------
  avg_overhead()  Average number of bits used for overhead data.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 avg_overhead(linReg_s *p)
{
    i32 i, tmp ;

    if (p->len) {
        for (i = 0, tmp = 0; i < p->len; i++) {
            tmp += p->bits[i];
        }
        return DIV(tmp, p->len);
    }
    return 0;
}
#endif

/*------------------------------------------------------------------------------
  update_overhead()  Update overhead (non texture data) table. Only statistics
  of PSLICE, please.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static void update_overhead(linReg_s *p, i32 bits, i32 clen)
{
    i32 tmp = p->pos;

    p->bits[tmp] = bits;

    if (++p->pos >= clen) {
        p->pos = 0;
    }
    if (p->len < clen) {
        p->len++;
    }
}
#endif

/*------------------------------------------------------------------------------
  gop_avg_qp()  Average quantization parameter of P frames of the previous GOP.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
i32 gop_avg_qp(h264RateControl_s *rc)
{
    i32 tmp = 0;

    if (rc->gopQpSum) {
        tmp = DIV(rc->gopQpSum, rc->gopQpDiv);
    }
    rc->gopQpSum = 0;
    rc->gopQpDiv = 0;

    return tmp;
}
#endif

/*------------------------------------------------------------------------------
  new_pic_quant()  Calculate new quantization parameter from the 2nd degree R-Q
  equation. Further adjust Qp for "smoother" visual quality.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 new_pic_quant(linReg_s *p, i32 bits, true_e useQpDeltaLimit)
{
    i32 tmp, qp_best = p->qp_prev, qp = p->qp_prev, diff;
    i32 diff_prev = 0, qp_prev = 0, diff_best = 0x7FFFFFFF;

    DBG(1, (DBGOUTPUT, "R/cx:%6d ", bits));

    if (p->a1 == 0 && p->a2 == 0) {
        DBG(1, (DBGOUTPUT, " QP: xx xx D:  ==== newQP: %2d\n", qp));
        return qp;
    }
    if (bits <= 0) {
        if (useQpDeltaLimit)
            qp = MIN(51, MAX(0, qp + QP_DELTA));
        else
            qp = MIN(51, MAX(0, qp + 10));

        DBG(1, (DBGOUTPUT, " QP: xx xx D:  ---- newQP: %2d\n", qp));
        return qp;
    }

    do {
        tmp  = DIV(p->a1, q_step[qp]);
        tmp += DIV(p->a2, q_step[qp] * q_step[qp]);
        diff = ABS(tmp - bits);

        if (diff < diff_best) {
            if (diff_best == 0x7FFFFFFF) {
                diff_prev = diff;
                qp_prev   = qp;
            } else {
                diff_prev = diff_best;
                qp_prev   = qp_best;
            }
            diff_best = diff;
            qp_best   = qp;
            if ((tmp - bits) <= 0) {
                if (qp < 1) {
                    break;
                }
                qp--;
            } else {
                if (qp > 50) {
                    break;
                }
                qp++;
            }
        } else {
            break;
        }
    } while ((qp >= 0) && (qp <= 51));
    qp = qp_best;

    DBG(1, (DBGOUTPUT, " QP: %2d %2d D: %5d", qp, qp_prev, diff_prev - diff_best));

    /* One unit change in Qp changes rate about 12% ca. 1/8. If the
     * difference is less than half use the one closer to previous value */
    if (ABS(diff_prev - diff_best) <= ABS(bits) / 16) {
        qp = qp_prev;
    }
    DBG(1, (DBGOUTPUT, " newQP: %2d\n", qp));

    /* Limit Qp change for smoother visual quality */
    if (useQpDeltaLimit) {
        tmp = qp - p->qp_prev;
        if (tmp > QP_DELTA) {
            qp = p->qp_prev + QP_DELTA;
        } else if (tmp < -QP_DELTA) {
            qp = p->qp_prev - QP_DELTA;
        }
    }

    return qp;
}
#endif

/*------------------------------------------------------------------------------
  update_tables()  only statistics of PSLICE, please.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static void update_tables(linReg_s *p, i32 qp, i32 bits)
{
    const i32 clen = 10;
    i32 tmp = p->pos;

    p->qp_prev   = qp;
    p->qs[tmp]   = q_step[qp];
    p->bits[tmp] = bits;

    if (++p->pos >= clen) {
        p->pos = 0;
    }
    if (p->len < clen) {
        p->len++;
    }
}
#endif

/*------------------------------------------------------------------------------
            update_model()  Update model parameter by Linear Regression.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static void update_model(linReg_s *p)
{
    i32 *qs = p->qs, *r = p->bits, n = p->len;
    i32 i, a1, a2, sx = lin_sx(qs, n), sy = lin_sy(qs, r, n);

    for (i = 0; i < n; i++) {
        DBG(2, (DBGOUTPUT, "model: qs %i  r %i\n", qs[i], r[i]));
    }

    a1 = lin_sxy(qs, r, n);
    a1 = a1 < I32_MAX / n ? a1 * n : I32_MAX;

    if (sy == 0) {
        a1 = 0;
    } else {
        a1 -= sx < I32_MAX / sy ? sx * sy : I32_MAX;
    }

    a2 = (lin_nsxx(qs, n) - (sx * sx));
    if (a2 == 0) {
        if (p->a1 == 0) {
            /* If encountered in the beginning */
            a1 = 0;
        } else {
            a1 = (p->a1 * 2) / 3;
        }
    } else {
        a1 = H264Calculate(a1, DSCY, a2);
    }

    /* Value of a1 shouldn't be excessive (small) */
    a1 = MAX(a1, -262144);
    a1 = MIN(a1,  262143);

    ASSERT(ABS(a1) * sx >= 0);
    ASSERT(sx * DSCY >= 0);
    a2 = DIV(sy * DSCY, n) - DIV(a1 * sx, n);

    DBG(2, (DBGOUTPUT, "model: a2:%9d  a1:%8d\n", a2, a1));

    if (p->len > 0) {
        p->a1 = a1;
        p->a2 = a2;
    }
}
#endif

/*------------------------------------------------------------------------------
  lin_sy()  calculate value of Sy for n points.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 lin_sy(i32 *qp, i32 *r, i32 n)
{
    i32 sum = 0;

    while (n--) {
        sum += qp[n] * qp[n] * r[n];
        if (sum < 0) {
            return I32_MAX / DSCY;
        }
    }
    return DIV(sum, DSCY);
}
#endif

/*------------------------------------------------------------------------------
  lin_sx()  calculate value of Sx for n points.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 lin_sx(i32 *qp, i32 n)
{
    i32 tmp = 0;

    while (n--) {
        ASSERT(qp[n]);
        tmp += qp[n];
    }
    return tmp;
}
#endif

/*------------------------------------------------------------------------------
  lin_sxy()  calculate value of Sxy for n points.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 lin_sxy(i32 *qp, i32 *r, i32 n)
{
    i32 tmp, sum = 0;

    while (n--) {
        tmp = qp[n] * qp[n] * qp[n];
        if (tmp > r[n]) {
            sum += DIV(tmp, DSCY) * r[n];
        } else {
            sum += tmp * DIV(r[n], DSCY);
        }
        if (sum < 0) {
            return I32_MAX;
        }
    }
    return sum;
}
#endif

/*------------------------------------------------------------------------------
  lin_nsxx()  calculate value of n * Sxy for n points.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
static i32 lin_nsxx(i32 *qp, i32 n)
{
    i32 tmp = 0, sum = 0, d = n ;

    while (n--) {
        tmp = qp[n];
        tmp *= tmp;
        sum += d * tmp;
    }
    return sum;
}
#endif
