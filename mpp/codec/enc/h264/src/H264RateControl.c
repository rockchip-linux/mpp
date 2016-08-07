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

