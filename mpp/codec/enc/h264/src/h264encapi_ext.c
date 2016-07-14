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

#include "h264encapi.h"
#include "h264encapi_ext.h"
#include "H264Instance.h"

H264EncRet H264EncSetFilter(H264EncInst inst, const H264EncFilter * pEncCfg)
{

    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    if (pEncInst->picParameterSet.deblockingFilterControlPresent == ENCHW_NO)
        return H264ENC_INVALID_STATUS;

    pEncInst->slice.disableDeblocking = pEncCfg->disableDeblocking;

#if 0
    if (pEncCfg->disableDeblocking != 1) {
        pEncInst->slice.filterOffsetA = pEncCfg->filterOffsetA;
        pEncInst->slice.filterOffsetB = pEncCfg->filterOffsetB;
    } else {
        pEncInst->slice.filterOffsetA = 0;
        pEncInst->slice.filterOffsetB = 0;
    }
#else
    pEncInst->slice.filterOffsetA = pEncCfg->filterOffsetA;
    pEncInst->slice.filterOffsetB = pEncCfg->filterOffsetB;
#endif

    return H264ENC_OK;
}

H264EncRet H264EncGetFilter(H264EncInst inst, H264EncFilter * pEncCfg)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    pEncCfg->disableDeblocking = pEncInst->slice.disableDeblocking;
    pEncCfg->filterOffsetA = pEncInst->slice.filterOffsetA;
    pEncCfg->filterOffsetB = pEncInst->slice.filterOffsetB;

    return H264ENC_OK;
}

H264EncRet H264EncSetChromaQpIndexOffset(H264EncInst inst, i32 offset)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    ASSERT(inst != NULL);

    if (offset < -12 || offset > 12) {
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Check status, only INIT is allowed */
    if (pEncInst->encStatus != H264ENCSTAT_INIT) {
        return H264ENC_INVALID_STATUS;
    }

    pEncInst->picParameterSet.chromaQpIndexOffset = offset;

    return H264ENC_OK;
}

H264EncRet H264EncSetHwBurstSize(H264EncInst inst, u32 burst)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    ASSERT(inst != NULL);
    ASSERT(burst < 64);

    pEncInst->asic.regs.asicCfgReg &=  ~(63 << 8);
    pEncInst->asic.regs.asicCfgReg |=  ((burst & (63)) << 8);

    return H264ENC_OK;
}

H264EncRet H264EncSetHwBurstType(H264EncInst inst, u32 bursttype)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    ASSERT(inst != NULL);

    pEncInst->asic.regs.asicCfgReg &=  ~(1 << 6);
    pEncInst->asic.regs.asicCfgReg |=  ((bursttype & (1)) << 6);

    return H264ENC_OK;
}

H264EncRet H264EncSetQuarterPixelMv(H264EncInst inst, u32 enable)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    ASSERT(inst != NULL);

    pEncInst->asic.regs.disableQuarterPixelMv = enable ? 0 : 1;

    return H264ENC_OK;
}

