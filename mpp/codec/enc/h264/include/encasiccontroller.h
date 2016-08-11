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

#ifndef __ENC_ASIC_CONTROLLER_H__
#define __ENC_ASIC_CONTROLLER_H__

#include "mpp_mem.h"
#include "mpp_buffer.h"
#include "h264e_syntax.h"

/* HW status register bits */
#define ASIC_STATUS_BUFF_FULL           0x020
#define ASIC_STATUS_HW_RESET            0x100
#define ASIC_STATUS_ERROR               0x010
#define ASIC_STATUS_FRAME_READY         0x002

#define ASIC_STATUS_ENABLE              0x001

#define ASIC_H264_BYTE_STREAM           0x00
#define ASIC_H264_NAL_UNIT              0x01

#define ASIC_INPUT_YUV420PLANAR         0x00
#define ASIC_INPUT_YUV420SEMIPLANAR     0x01
#define ASIC_INPUT_YUYV422INTERLEAVED   0x02
#define ASIC_INPUT_UYVY422INTERLEAVED   0x03
#define ASIC_INPUT_RGB565               0x04
#define ASIC_INPUT_RGB555               0x05
#define ASIC_INPUT_RGB444               0x06
#define ASIC_INPUT_RGB888               0x07
#define ASIC_INPUT_RGB101010            0x08

typedef enum {
    IDLE = 0,   /* Initial state, both HW and SW disabled */
    HWON_SWOFF, /* HW processing, SW waiting for HW */
    HWON_SWON,  /* Both HW and SW processing */
    HWOFF_SWON, /* HW is paused or disabled, SW is processing */
    DONE
} bufferState_e;

typedef enum {
    ASIC_MPEG4 = 0,
    ASIC_H263 = 1,
    ASIC_JPEG = 2,
    ASIC_H264 = 3
} asicCodingType_e;

typedef enum {
    ASIC_P_16x16 = 0,
    ASIC_P_16x8 = 1,
    ASIC_P_8x16 = 2,
    ASIC_P_8x8 = 3,
    ASIC_I_4x4 = 4,
    ASIC_I_16x16 = 5
} asicMbType_e;

typedef enum {
    ASIC_INTER = 0,
    ASIC_INTRA = 1
} asicFrameCodingType_e;

typedef struct {
    RK_U32 frameCodingType;
    RK_S32 sliceAlphaOffset;
    RK_S32 sliceBetaOffset;
    RK_U32 filterDisable;
    RK_U32 inputImageFormat;
    RK_U32 inputImageRotation;
    RK_U32 outputStrmBase;
    RK_U32 outputStrmSize;

    RK_U32 inputLumBase;
    RK_U32 inputCbBase;
    RK_U32 inputCrBase;
    RK_U32 cpDistanceMbs;
    RK_U32 *cpTargetResults;
    const RK_U32 *cpTarget;
    const RK_S32 *targetError;
    const RK_S32 *deltaQp;
    RK_U32 rlcCount;
    RK_U32 qpSum;
    RK_U32 h264StrmMode;   /* 0 - byte stream, 1 - NAL units */
    RK_U32 inputLumaBaseOffset;
    RK_U32 inputChromaBaseOffset;
    RK_U32 h264Inter4x4Disabled;
    RK_S32 madQpDelta;
    RK_U32 madThreshold;
    RK_U32 madCount;
    RK_U32 colorConversionCoeffA;
    RK_U32 colorConversionCoeffB;
    RK_U32 colorConversionCoeffC;
    RK_U32 colorConversionCoeffE;
    RK_U32 colorConversionCoeffF;
    RK_U32 rMaskMsb;
    RK_U32 gMaskMsb;
    RK_U32 bMaskMsb;
    RK_U32 hw_status;
} regValues_s;

typedef struct {
    regValues_s regs;
} asicData_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
RK_S32 EncAsicControllerInit(asicData_s * asic);

/* Functions for controlling ASIC */
void EncAsicFrameStart(void * inst, regValues_s * val, h264e_syntax *syntax_data);
#endif
