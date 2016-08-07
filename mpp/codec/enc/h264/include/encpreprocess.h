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

#ifndef __ENC_PRE_PROCESS_H__
#define __ENC_PRE_PROCESS_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define    ROTATE_0     0U
#define    ROTATE_90R   1U  /* Rotate 90 degrees clockwise */
#define    ROTATE_90L   2U  /* Rotate 90 degrees counter-clockwise */

/* maximum input picture width set by the available bits in ASIC regs */
#define    MAX_INPUT_IMAGE_WIDTH   (8192)

typedef struct {
    RK_U32 lumWidthSrc;    /* Source input image width */
    RK_U32 lumHeightSrc;   /* Source input image height */
    RK_U32 lumWidth;   /* Encoded image width */
    RK_U32 lumHeight;  /* Encoded image height */
    RK_U32 horOffsetSrc;   /* Encoded frame offset, reference is ... */
    RK_U32 verOffsetSrc;   /* ...top  left corner of source image */
    RK_U32 inputFormat;
    RK_U32 rotation;
    RK_U32 videoStab;
    RK_U32 colorConversionType;    /* 0 = bt601, 1 = bt709, 2 = user defined */
    RK_U32 colorConversionCoeffA;
    RK_U32 colorConversionCoeffB;
    RK_U32 colorConversionCoeffC;
    RK_U32 colorConversionCoeffE;
    RK_U32 colorConversionCoeffF;
} preProcess_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
RK_S32 EncPreProcessCheck(const preProcess_s * preProcess);
void EncPreProcess(asicData_s * asic, const preProcess_s * preProcess);
void EncSetColorConversion(preProcess_s * preProcess, asicData_s * asic);

#endif
