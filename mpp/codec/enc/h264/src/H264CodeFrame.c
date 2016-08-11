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

#include "mpp_log.h"
#include "enccommon.h"
#include "h264encapi.h"
#include "H264CodeFrame.h"

static void H264SetNewFrame(H264ECtx * inst);

static RK_S64 encNum = 0;

void H264CodeFrame(H264ECtx * inst, h264e_syntax *syntax_data)
{
    H264SetNewFrame(inst);
    if (!inst->time_debug_init) {
        inst->time_debug_init = 1;
        encNum = 0;
    }

    EncAsicFrameStart((void*)inst, &inst->asic.regs, syntax_data);
}

void H264SetNewFrame(H264ECtx * ctx)
{
    regValues_s *regs = &ctx->asic.regs;

    regs->outputStrmSize -= ctx->stream.byteCnt;

    /* 64-bit aligned stream base address */
    regs->outputStrmBase += (ctx->stream.byteCnt & (~0x07));

    if (ctx->rateControl.mbRc) {
        regs->cpTarget = (RK_U32 *) ctx->rateControl.qpCtrl.wordCntTarget;
        regs->targetError = ctx->rateControl.qpCtrl.wordError;
        regs->deltaQp = ctx->rateControl.qpCtrl.qpChange;

        regs->cpDistanceMbs = ctx->rateControl.qpCtrl.checkPointDistance;

        regs->cpTargetResults = (RK_U32 *) ctx->rateControl.qpCtrl.wordCntPrev;
    } else {
        regs->cpTarget = NULL;
    }

    regs->filterDisable = ctx->slice.disableDeblocking;
    if (ctx->slice.disableDeblocking != 1) {
        regs->sliceAlphaOffset = ctx->slice.filterOffsetA / 2;
        regs->sliceBetaOffset = ctx->slice.filterOffsetB / 2;
    } else {
        regs->sliceAlphaOffset = 0;
        regs->sliceBetaOffset = 0;
    }

    regs->frameCodingType =
        (ctx->slice.sliceType == ISLICE) ? ASIC_INTRA : ASIC_INTER;

    /* MAD threshold range [0, 63*256] register 6-bits range [0,63] */
    regs->madThreshold = ctx->mad.threshold / 256;
    regs->madQpDelta = ctx->rateControl.mbQpAdjustment;
}

