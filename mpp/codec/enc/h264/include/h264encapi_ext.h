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

#ifndef __H264ENCAPI_EXT_H__
#define __H264ENCAPI_EXT_H__

#include "basetype.h"
#include "h264encapi.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    u32 disableDeblocking;
    i32 filterOffsetA;
    i32 filterOffsetB;
} H264EncFilter;

H264EncRet H264EncGetFilter(H264EncInst inst, H264EncFilter * pEncCfg);
H264EncRet H264EncSetFilter(H264EncInst inst,
                            const H264EncFilter * pEncCfg);

H264EncRet H264EncSetFilter(H264EncInst inst,
                            const H264EncFilter * pEncCfg);

H264EncRet H264EncSetChromaQpIndexOffset(H264EncInst inst, i32 offset);

H264EncRet H264EncSetHwBurstSize(H264EncInst inst, u32 burst);

H264EncRet H264EncSetHwBurstType(H264EncInst inst, u32 burstType);

H264EncRet H264EncSetQuarterPixelMv(H264EncInst inst, u32 enable);

#ifdef __cplusplus
}
#endif

#endif /*__H264ENCAPI_EXT_H__*/
