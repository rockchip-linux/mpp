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
#include "ewl.h"
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

void H264SetNewFrame(H264ECtx * inst)
{
    regValues_s *regs = &inst->asic.regs;

    regs->outputStrmSize -= inst->stream.byteCnt;

    /* 64-bit aligned stream base address */
    regs->outputStrmBase += (inst->stream.byteCnt & (~0x07));
    regs->frameNum = inst->slice.frameNum;
    regs->idrPicId = inst->slice.idrPicId;

    /* Store the final register values in the register structure */
    regs->sliceSizeMbRows = inst->slice.sliceSize / inst->mbPerRow;
    regs->chromaQpIndexOffset = inst->picParameterSet.chromaQpIndexOffset;

    regs->picInitQp = (RK_U32) (inst->picParameterSet.picInitQpMinus26 + 26);

    regs->qp = inst->rateControl.qpHdr;
    regs->qpMin = inst->rateControl.qpMin;
    regs->qpMax = inst->rateControl.qpMax;

    if (inst->rateControl.mbRc) {
        regs->cpTarget = (RK_U32 *) inst->rateControl.qpCtrl.wordCntTarget;
        regs->targetError = inst->rateControl.qpCtrl.wordError;
        regs->deltaQp = inst->rateControl.qpCtrl.qpChange;

        regs->cpDistanceMbs = inst->rateControl.qpCtrl.checkPointDistance;

        regs->cpTargetResults = (RK_U32 *) inst->rateControl.qpCtrl.wordCntPrev;
    } else {
        regs->cpTarget = NULL;
    }

    regs->filterDisable = inst->slice.disableDeblocking;
    if (inst->slice.disableDeblocking != 1) {
        regs->sliceAlphaOffset = inst->slice.filterOffsetA / 2;
        regs->sliceBetaOffset = inst->slice.filterOffsetB / 2;
    } else {
        regs->sliceAlphaOffset = 0;
        regs->sliceBetaOffset = 0;
    }
    regs->transform8x8Mode = inst->picParameterSet.transform8x8Mode;
    regs->enableCabac = inst->picParameterSet.entropyCodingMode;
    if (inst->picParameterSet.entropyCodingMode) {
        regs->cabacInitIdc = inst->slice.cabacInitIdc;
    }
    regs->frameCodingType =
        (inst->slice.sliceType == ISLICE) ? ASIC_INTRA : ASIC_INTER;

    /* MAD threshold range [0, 63*256] register 6-bits range [0,63] */
    regs->madThreshold = inst->mad.threshold / 256;
    regs->madQpDelta = inst->rateControl.mbQpAdjustment;
}

