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

#include "basetype.h"
#include "enccfg.h"
#include "ewl.h"
#include "mpp_mem.h"
#include "mpp_buffer.h"
#include "h264e_syntax.h"  // add by lance 2016.05.07

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
    u32 irqDisable;
    u32 irqInterval;
    u32 mbsInCol;
    u32 mbsInRow;
    u32 qp;
    u32 qpMin;
    u32 qpMax;
    u32 constrainedIntraPrediction;
    u32 roundingCtrl;
    u32 frameCodingType;
    u32 codingType;
    u32 pixelsOnRow;
    u32 xFill;
    u32 yFill;
    u32 ppsId;
    u32 idrPicId;
    u32 frameNum;
    u32 picInitQp;
    i32 sliceAlphaOffset;
    i32 sliceBetaOffset;
    u32 filterDisable;
    u32 transform8x8Mode;
    u32 enableCabac;
    u32 cabacInitIdc;
    i32 chromaQpIndexOffset;
    u32 sliceSizeMbRows;
    u32 inputImageFormat;
    u32 inputImageRotation;
    u32 outputStrmBase;
    u32 outputStrmSize;
    u32 firstFreeBit;
    u32 strmStartMSB;
    u32 strmStartLSB;
    u32 rlcBase;
    u32 rlcLimitSpace;
    u32 socket;   // vpu socket    // now it will be inited by hal part, so mask it    by lance 2016.05.09

    u32 inputLumBase;
    u32 inputCbBase;
    u32 inputCrBase;
    u32 cpDistanceMbs;
    u32 *cpTargetResults;
    const u32 *cpTarget;
    const i32 *targetError;
    const i32 *deltaQp;
    u32 rlcCount;
    u32 qpSum;
    u32 h264StrmMode;   /* 0 - byte stream, 1 - NAL units */
    u32 gobHeaderMask;
    u32 gobFrameId;
    u8 quantTable[8 * 8 * 2];
    u32 jpegMode;
    u32 jpegSliceEnable;
    u32 jpegRestartInterval;
    u32 jpegRestartMarker;
    u32 inputLumaBaseOffset;
    u32 inputChromaBaseOffset;
    u32 h264Inter4x4Disabled;
    u32 disableQuarterPixelMv;
    u32 vsNextLumaBase;
    u32 vsMode;
    u32 vpSize;
    u32 vpMbBits;
    u32 hec;
    u32 moduloTimeBase;
    u32 intraDcVlcThr;
    u32 vopFcode;
    u32 timeInc;
    u32 timeIncBits;
    u32 asicCfgReg;
    u32 intra16Favor;
    u32 interFavor;
    u32 skipPenalty;
    i32 madQpDelta;
    u32 madThreshold;
    u32 madCount;
    u32 colorConversionCoeffA;
    u32 colorConversionCoeffB;
    u32 colorConversionCoeffC;
    u32 colorConversionCoeffE;
    u32 colorConversionCoeffF;
    u32 rMaskMsb;
    u32 gMaskMsb;
    u32 bMaskMsb;

    u32 splitMvMode;
    u32 diffMvPenalty[3];
    u32 splitPenalty[4];
    u32 zeroMvFavorDiv2;

    u32 intraAreaTop;
    u32 intraAreaLeft;
    u32 intraAreaBottom;
    u32 intraAreaRight;
    u32 roi1Top;
    u32 roi1Left;
    u32 roi1Bottom;
    u32 roi1Right;
    u32 roi2Top;
    u32 roi2Left;
    u32 roi2Bottom;
    u32 roi2Right;
    u32 hw_status;
} regValues_s;

typedef struct {
    regValues_s regs;
} asicData_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s * asic);

/* Functions for controlling ASIC */
void EncAsicFrameStart(void * inst,/* const void *ewl,*/ regValues_s * val, h264e_syntax *syntax_data);  // mask by lance 2016.05.12
#endif
