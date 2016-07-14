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

/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/
#include <string.h>

#include "mpp_log.h"  // add by lance 2016.05.06
#include "enccommon.h"
#include "encasiccontroller.h"
#include "H264Instance.h"  // add by lance 2016.05.08
#include "encpreprocess.h"
#include "ewl.h"
#include <stdio.h>

/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

#ifdef ASIC_WAVE_TRACE_TRIGGER
extern i32 trigger_point;    /* picture which will be traced */
#endif

/* Mask fields */
#define mask_2b         (u32)0x00000003
#define mask_3b         (u32)0x00000007
#define mask_4b         (u32)0x0000000F
#define mask_5b         (u32)0x0000001F
#define mask_6b         (u32)0x0000003F
#define mask_11b        (u32)0x000007FF
#define mask_14b        (u32)0x00003FFF
#define mask_16b        (u32)0x0000FFFF

#define HSWREG(n)       ((n)*4)

/* MPEG-4 motion estimation parameters */
static const i32 mpeg4InterFavor[32] = { 0,
                                         0, 120, 140, 160, 200, 240, 280, 340, 400, 460, 520, 600, 680,
                                         760, 840, 920, 1000, 1080, 1160, 1240, 1320, 1400, 1480, 1560,
                                         1640, 1720, 1800, 1880, 1960, 2040, 2120
                                       };

static const u32 mpeg4DiffMvPenalty[32] = { 0,
                                            4, 5, 6, 7, 8, 9, 10, 11, 14, 17, 20, 23, 27, 31, 35, 38, 41,
                                            44, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59
                                          };

/* H.264 motion estimation parameters */
static const u32 h264PrevModeFavor[52] = {
    7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 24, 25, 27, 29, 30, 32, 34, 36, 38, 41, 43, 46,
    49, 51, 55, 58, 61, 65, 69, 73, 78, 82, 87, 93, 98, 104, 110,
    117, 124, 132, 140
};

static const u32 h264DiffMvPenalty[52] = {  //rk29
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 9, 10,
    11, 13, 14, 16, 18, 20, 23, 25, 29, 32, 36, 40, 45, 51, 57, 64,
    72, 81, 91
};

/* sqrt(2^((qp-12)/3))*8 */
static const u32 h264DiffMvPenalty_rk30[52] = { //rk30
    2, 2, 3, 3, 3, 4, 4, 4, 5, 6,
    6, 7, 8, 9, 10, 11, 13, 14, 16, 18,
    20, 23, 26, 29, 32, 36, 40, 45, 51, 57,
    64, 72, 81, 91, 102, 114, 128, 144, 161, 181,
    203, 228, 256, 287, 323, 362, 406, 456, 512, 575,
    645, 724
};

/* 31*sqrt(2^((qp-12)/3))/4 */
static const u32 h264DiffMvPenalty4p_rk30[52] = { //rk30
    2, 2, 2, 3, 3, 3, 4, 4, 5, 5,
    6, 7, 8, 9, 10, 11, 12, 14, 16, 17,
    20, 22, 25, 28, 31, 35, 39, 44, 49, 55,
    62, 70, 78, 88, 98, 110, 124, 139, 156, 175,
    197, 221, 248, 278, 312, 351, 394, 442, 496, 557,
    625, 701
};


/* JPEG QUANT table order */
static const u32 qpReorderTable[64] = {
    0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
    2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
    4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
    6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

extern u32 h264SkipSadPenalty_rk30[52];

// add by lance 2016.04.29 for compare
//#define WRITE_VAL_INFO_FOR_COMPARE 1
#ifdef WRITE_VAL_INFO_FOR_COMPARE
FILE *valCompareFile = NULL;
int oneFrameFlagTest = 0;
#endif

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Initialize empty structure with default values.
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s * asic)
{
    ASSERT(asic != NULL);

    /* Initialize default values from defined configuration */
    asic->regs.irqDisable = ENC6820_IRQ_DISABLE;

    asic->regs.asicCfgReg =
        ((ENC6820_OUTPUT_SWAP_32 & (1)) << 26) |
        ((ENC6820_OUTPUT_SWAP_16 & (1)) << 27) |
        ((ENC6820_OUTPUT_SWAP_8 & (1)) << 28);

    /* Initialize default values */
    asic->regs.roundingCtrl = 0;
    asic->regs.cpDistanceMbs = 0;
    asic->regs.riceEnable = 0;

    /* User must set these */
    asic->regs.inputLumBase = 0;
    asic->regs.inputCbBase = 0;
    asic->regs.inputCrBase = 0;

    asic->asicDataBufferGroup = NULL;  // add by lance 2016.05.05
    asic->internalImageLuma[0] = NULL;  // modify by lance 2016.05.05
    asic->internalImageChroma[0] = NULL;  // modify by lance 2016.05.05
    asic->internalImageLuma[1] = NULL;  // modify by lance 2016.05.05
    asic->internalImageChroma[1] = NULL;
    asic->cabacCtx = NULL;
    asic->riceRead = NULL;
    asic->riceWrite = NULL;

#ifdef ASIC_WAVE_TRACE_TRIGGER
    asic->regs.vop_count = 0;
#endif

    /* get ASIC ID value */
    asic->regs.asicHwId = EncAsicGetId(/*asic->ewl*/);  // mask by lance 2016.05.12

    /* we do NOT reset hardware at this point because */
    /* of the multi-instance support                  */
    // add by lance 2016.04.29
#ifdef WRITE_VAL_INFO_FOR_COMPARE
    oneFrameFlagTest = 0;
    if (valCompareFile == NULL) {
        if ((valCompareFile = fopen("/data/test/val_cmp.file", "w")) == NULL)
            mpp_err("val original File open failed!");
    }
#endif
    return ENCHW_OK;
}


/*------------------------------------------------------------------------------

    EncAsicSetQuantTable

    Set new jpeg quantization table to be used by ASIC

------------------------------------------------------------------------------*/
void EncAsicSetQuantTable(asicData_s * asic,
                          const u8 * lumTable, const u8 * chTable)
{
    i32 i;

    ASSERT(lumTable);
    ASSERT(chTable);

    for (i = 0; i < 64; i++) {
        asic->regs.quantTable[i] = lumTable[qpReorderTable[i]];
    }
    for (i = 0; i < 64; i++) {
        asic->regs.quantTable[64 + i] = chTable[qpReorderTable[i]];
    }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetId(/*const void *ewl*/)  // mask by lance 2016.05.12
{
    return 0x82701110;
}

/*------------------------------------------------------------------------------
    When the frame is successfully encoded the internal image is recycled
    so that during the next frame the current internal image is read and
    the new reconstructed image is written two macroblock rows earlier.
------------------------------------------------------------------------------*/
void EncAsicRecycleInternalImage(regValues_s * val)
{
    /* The next encoded frame will use current reconstructed frame as */
    /* the reference */
    u32 tmp;

    tmp = val->internalImageLumBaseW;
    val->internalImageLumBaseW = val->internalImageLumBaseR;
    val->internalImageLumBaseR = tmp;

    tmp = val->internalImageChrBaseW;
    val->internalImageChrBaseW = val->internalImageChrBaseR;
    val->internalImageChrBaseR = tmp;

    tmp = val->riceReadBase;
    val->riceReadBase = val->riceWriteBase;
    val->riceWriteBase = tmp;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void CheckRegisterValues(regValues_s * val)
{
    u32 i;

    ASSERT(val->irqDisable <= 1);
    ASSERT(val->rlcLimitSpace / 2 < (1 << 20));
    ASSERT(val->mbsInCol <= 511);
    ASSERT(val->mbsInRow <= 511);
    ASSERT(val->filterDisable <= 2);
    ASSERT(val->madThreshold <= 63);
    ASSERT(val->madQpDelta >= -8 && val->madQpDelta <= 7);
    ASSERT(val->qp <= 51);
    ASSERT(val->constrainedIntraPrediction <= 1);
    ASSERT(val->roundingCtrl <= 1);
    ASSERT(val->frameCodingType <= 1);
    ASSERT(val->codingType <= 3);
    ASSERT(val->pixelsOnRow >= 16 && val->pixelsOnRow <= 8192); /* max input for cropping */
    ASSERT(val->xFill <= 3);
    ASSERT(val->yFill <= 14 && ((val->yFill & 0x01) == 0));
    ASSERT(val->sliceAlphaOffset >= -6 && val->sliceAlphaOffset <= 6);
    ASSERT(val->sliceBetaOffset >= -6 && val->sliceBetaOffset <= 6);
    ASSERT(val->chromaQpIndexOffset >= -12 && val->chromaQpIndexOffset <= 12);
    ASSERT(val->sliceSizeMbRows <= 127);
    ASSERT(val->inputImageFormat <= ASIC_INPUT_RGB101010);
    ASSERT(val->inputImageRotation <= 2);
    ASSERT(val->cpDistanceMbs >= 0 && val->cpDistanceMbs <= 2047);
    ASSERT(val->vpMbBits <= 15);
    ASSERT(val->vpSize <= 65535);

    if (val->codingType != ASIC_JPEG && val->cpTarget != NULL) {
        ASSERT(val->cpTargetResults != NULL);

        for (i = 0; i < 10; i++) {
            ASSERT(*val->cpTarget < (1 << 16));
        }

        ASSERT(val->targetError != NULL);

        for (i = 0; i < 7; i++) {
            ASSERT((*val->targetError) >= -32768 &&
                   (*val->targetError) < 32768);
        }

        ASSERT(val->deltaQp != NULL);

        for (i = 0; i < 7; i++) {
            ASSERT((*val->deltaQp) >= -8 && (*val->deltaQp) < 8);
        }
    }

    (void) val;
}

/*------------------------------------------------------------------------------
    Function name   : EncAsicFrameStart
    Description     :
    Return type     : void
    Argument        : const void *ewl
    Argument        : regValues_s * val
------------------------------------------------------------------------------*/
void EncAsicFrameStart(void * inst, /*const void *ewl, */regValues_s * val, h264e_syntax *syntax_data)  // mask by lance 2016.05.12
{
    h264Instance_s *instH264Encoder = (h264Instance_s *)inst;  // add by lance 2016.05.08
    int iCount = 0;  // add by lance 2016.05.08
    u32 /*interFavor = 0, *//*diffMvPenalty = 0,*/ prevModeFavor = 0;  // mask by lance 2016.05.12
    // add by lance 2016.04.29
#ifdef WRITE_VAL_INFO_FOR_COMPARE
    int iTest = 0;
#endif
    /* Set the interrupt interval in macroblock rows (JPEG) or
     * in macroblocks (video) */

    prevModeFavor = h264PrevModeFavor[val->qp];
    //diffMvPenalty = h264DiffMvPenalty[val->qp];  // mask by lance 2016.05.12

    CheckRegisterValues(val);

    memset(val->regMirror, 0, sizeof(val->regMirror));


    //intra area
    val->regMirror[46] = (val->intraAreaTop & 0xff) << 24;
    val->regMirror[46] |= (val->intraAreaBottom & 0xff) << 16;
    val->regMirror[46] |= (val->intraAreaLeft & 0xff) << 8;
    val->regMirror[46] |= (val->intraAreaRight & 0xff);

    /* Input picture buffers */
    val->regMirror[48] = val->inputLumBase;
    val->regMirror[49] = val->inputCbBase;
    val->regMirror[50] = val->inputCrBase;

    /* stream buffer limits */
    {
        val->regMirror[51] = val->strmStartMSB;
        val->regMirror[52] = val->strmStartLSB;
        val->regMirror[53] = val->outputStrmSize;
    }

    val->regMirror[54] =
        ((ENC6820_AXI_READ_ID & (255)) << 24) |
        ((ENC6820_AXI_WRITE_ID & (255)) << 16) |
        ((ENC6820_BURST_LENGTH & (63)) << 8) |
        ((ENC6820_BURST_INCR_TYPE_ENABLED & (1)) << 2) |
        ((ENC6820_BURST_DATA_DISCARD_ENABLED & (1)) << 1);

    val->regMirror[55] = (val->madQpDelta & 0xF);

    /* Video encoding reference picture buffers */
    val->regMirror[56] = val->internalImageLumBaseR;
    val->regMirror[57] = val->internalImageChrBaseR;

    /* Common control register */

    /* PreP control */

    /* If favor has not been set earlier by testId use default */
    {
        i32 scaler = MAX(1, 200 / (val->mbsInRow + val->mbsInCol));
        val->skipPenalty = MIN(255, h264SkipSadPenalty_rk30[val->qp] * scaler);
    }

    val->regMirror[59] =
        (val->disableQuarterPixelMv << 28) |
        (val->cabacInitIdc << 21) |
        (val->enableCabac << 20) |
        (val->transform8x8Mode << 17) |
        (val->h264Inter4x4Disabled << 16) |
        (val->h264StrmMode << 15) |
        (val->sliceSizeMbRows << 8) ;

    /* Stream start offset */
    val->regMirror[60] =
        (val->firstFreeBit << 16) |
        (val->skipPenalty << 8) |
        (val->xFill << 4) |
        val->yFill;

    val->regMirror[61] =
        (val->inputChromaBaseOffset << 20) |
        (val->inputLumaBaseOffset << 16) |
        val->pixelsOnRow;

    val->regMirror[63] = val->internalImageLumBaseW;

    val->regMirror[64] = val->internalImageChrBaseW;

    /* video encoding rate control */


    if (val->cpTarget != NULL) {
        val->regMirror[65] = (val->cpTarget[0] << 16) | (val->cpTarget[1]);
        val->regMirror[66] = (val->cpTarget[2] << 16) | (val->cpTarget[3]);
        val->regMirror[67] = (val->cpTarget[4] << 16) | (val->cpTarget[5]);
        val->regMirror[68] = (val->cpTarget[6] << 16) | (val->cpTarget[7]);
        val->regMirror[69] = (val->cpTarget[8] << 16) | (val->cpTarget[9]);

        val->regMirror[70] = ((val->targetError[0] & mask_16b) << 16) |
                             (val->targetError[1] & mask_16b);
        val->regMirror[71] = ((val->targetError[2] & mask_16b) << 16) |
                             (val->targetError[3] & mask_16b);
        val->regMirror[72] = ((val->targetError[4] & mask_16b) << 16) |
                             (val->targetError[5] & mask_16b);

        val->regMirror[73] = ((val->deltaQp[0] & mask_4b) << 24) |
                             ((val->deltaQp[1] & mask_4b) << 20) |
                             ((val->deltaQp[2] & mask_4b) << 16) |
                             ((val->deltaQp[3] & mask_4b) << 12) |
                             ((val->deltaQp[4] & mask_4b) << 8) |
                             ((val->deltaQp[5] & mask_4b) << 4) |
                             (val->deltaQp[6] & mask_4b);
    }

    /* output stream buffer */
    {
        val->regMirror[77] = val->outputStrmBase;
        /* NAL size buffer or MB control buffer */
        val->regMirror[78] = val->sizeTblBase.nal;
        if (val->sizeTblBase.nal)
            val->sizeTblPresent = 1;
    }

    val->regMirror[74] =
        (val->madThreshold << 24) |
        (val->inputImageFormat << 4) |
        (val->inputImageRotation << 2) |
        val->sizeTblPresent;

    val->regMirror[75] =
        (val->intra16Favor << 16) |
        val->interFavor;

    /* H.264 control */
    val->regMirror[76] =
        (val->picInitQp << 26) |
        ((val->sliceAlphaOffset & mask_4b) << 22) |
        ((val->sliceBetaOffset & mask_4b) << 18) |
        ((val->chromaQpIndexOffset & mask_5b) << 13) |
        (val->filterDisable << 5) |
        (val->idrPicId << 1) | (val->constrainedIntraPrediction);



    val->regMirror[79] = val->vsNextLumaBase;

    val->regMirror[80] = val->riceWriteBase;

    val->regMirror[81] = val->cabacCtxBase;
    //roi1 area
    val->regMirror[82] = (val->roi1Top & 0xff) << 24;
    val->regMirror[82] |= (val->roi1Bottom & 0xff) << 16;
    val->regMirror[82] |= (val->roi1Left & 0xff) << 8;
    val->regMirror[82] |= (val->roi1Right & 0xff);

    //roi2 area
    val->regMirror[83] =  (val->roi2Top & 0xff) << 24;
    val->regMirror[83] |= (val->roi2Bottom & 0xff) << 16;
    val->regMirror[83] |= (val->roi2Left & 0xff) << 8;
    val->regMirror[83] |= (val->roi2Right & 0xff);

    val->regMirror[94] = val->vsMode << 6;

    val->regMirror[95] = ((val->colorConversionCoeffB & mask_16b) << 16) |
                         (val->colorConversionCoeffA & mask_16b);

    val->regMirror[96] = ((val->colorConversionCoeffE & mask_16b) << 16) |
                         (val->colorConversionCoeffC & mask_16b);

    val->regMirror[97] = (val->colorConversionCoeffF & mask_16b);

    val->regMirror[98] = ((val->bMaskMsb & mask_5b) << 16) |
                         ((val->gMaskMsb & mask_5b) << 8) |
                         (val->rMaskMsb & mask_5b);

    val->splitMvMode = 1;
    val->diffMvPenalty[0] = h264DiffMvPenalty4p_rk30[val->qp];
    val->diffMvPenalty[1] = h264DiffMvPenalty_rk30[val->qp];
    val->diffMvPenalty[2] = h264DiffMvPenalty_rk30[val->qp];
    val->splitPenalty[0] = 0;
    val->splitPenalty[1] = 0;
    val->splitPenalty[2] = 0;
    val->splitPenalty[3] = 0;

    val->zeroMvFavorDiv2 = 10;

    val->regMirror[99] = val->diffMvPenalty[1] << 21 |
                         (val->diffMvPenalty[2] << 11) |
                         (val->diffMvPenalty[0] << 1) |
                         (val->splitMvMode); //change

    val->regMirror[100] = (val->qp << 26) | (val->qpMax << 20) |
                          (val->qpMin << 14) | (val->cpDistanceMbs);

    val->regMirror[102] = val->zeroMvFavorDiv2 << 20;

    val->regMirror[103] =
        (val->mbsInRow << 8) |
        (val->mbsInCol << 20) |
        (val->frameCodingType << 6) |
        (val->codingType << 4);

    /* system configuration */
    if (val->inputImageFormat < ASIC_INPUT_RGB565)      /* YUV input */
        val->regMirror[105] = val->asicCfgReg |
                              ((ENC6820_INPUT_SWAP_8_YUV & (1)) << 31 |
                               ((ENC6820_INPUT_SWAP_16_YUV & (1)) << 30) |
                               ((ENC6820_INPUT_SWAP_32_YUV & (1)) << 29));

    else if (val->inputImageFormat < ASIC_INPUT_RGB888) /* 16-bit RGB input */
        val->regMirror[105] = val->asicCfgReg |
                              ((ENC6820_INPUT_SWAP_8_RGB16 & (1)) << 31)  |
                              ((ENC6820_INPUT_SWAP_16_RGB16 & (1)) << 30) |
                              ((ENC6820_INPUT_SWAP_32_RGB16 & (1)) << 29);
    else    /* 32-bit RGB input */
        val->regMirror[105] = val->asicCfgReg |
                              ((ENC6820_INPUT_SWAP_8_RGB32 & (1)) << 31) |
                              ((ENC6820_INPUT_SWAP_16_RGB32 & (1)) << 30) |
                              ((ENC6820_INPUT_SWAP_32_RGB32 & (1)) << 29);

    val->regMirror[106] =
        (val->ppsId << 24) | (prevModeFavor << 16) | (val->frameNum);

    /* encoder interrupt */
    val->regMirror[109] =
        (val->riceEnable << 24) |
        ((ENC6820_ASIC_CLOCK_GATING_ENABLED & (1)) << 12) |
        (ENC6820_TIMEOUT_INTERRUPT << 10) |
        (val->irqDisable << 8);

    i32 i = 0;
    /* DMV penalty tables */
    for (i = 0; i < ASIC_PENALTY_TABLE_SIZE; i++) {
        val->dmvPenalty[i] = i;
        val->dmvQpelPenalty[i] = MIN(255, ExpGolombSigned(i));
    }

    /* Write DMV penalty tables to regs */
    for (i = 0; i < 128; i += 4) {
        /* swreg[96]=0x180 to swreg[127]=0x1FC */
        val->regMirror[120 + (i / 4)] =
            ((val->dmvPenalty[i   ] << 24) |
             (val->dmvPenalty[i + 1] << 16) |
             (val->dmvPenalty[i + 2] << 8) |
             (val->dmvPenalty[i + 3]));
    }
    for (i = 0; i < 128; i += 4) {
        /* swreg[128]=0x200 to swreg[159]=0x27C */
        val->regMirror[152 + (i / 4)] =
            ((val->dmvQpelPenalty[i   ] << 24) |
             (val->dmvQpelPenalty[i + 1] << 16) |
             (val->dmvQpelPenalty[i + 2] << 8) |
             (val->dmvQpelPenalty[i + 3]));
    }

    /* Register with enable bit is written last */
    val->regMirror[103] |= ASIC_STATUS_ENABLE;
    // add by lance 2016.04.29
#ifdef WRITE_VAL_INFO_FOR_COMPARE
    if (oneFrameFlagTest < 240 && valCompareFile != NULL) {
        fprintf(valCompareFile, "val->intraAreaTop                   0x%08X\n", val->intraAreaTop);
        fprintf(valCompareFile, "val->intraAreaBottom                0x%08X\n", val->intraAreaBottom);
        fprintf(valCompareFile, "val->intraAreaLeft                  0x%08X\n", val->intraAreaLeft);
        fprintf(valCompareFile, "val->intraAreaRight                 0x%08X\n", val->intraAreaRight);
        fprintf(valCompareFile, "val->inputLumBase                   0x%08X\n", val->inputLumBase);
        fprintf(valCompareFile, "val->inputCbBase                    0x%08X\n", val->inputCbBase);
        fprintf(valCompareFile, "val->inputCrBase                    0x%08X\n", val->inputCrBase);
        fprintf(valCompareFile, "val->strmStartMSB                   0x%08X\n", val->strmStartMSB);
        fprintf(valCompareFile, "val->strmStartLSB                   0x%08X\n", val->strmStartLSB);
        fprintf(valCompareFile, "val->outputStrmSize                 0x%08X\n", val->outputStrmSize);
        fprintf(valCompareFile, "val->madQpDelta                     0x%08X\n", val->madQpDelta);
        fprintf(valCompareFile, "val->internalImageLumBaseR          0x%08X\n", val->internalImageLumBaseR);
        fprintf(valCompareFile, "val->internalImageChrBaseR          0x%08X\n", val->internalImageChrBaseR);
        fprintf(valCompareFile, "val->mbsInRow                       0x%08X\n", val->mbsInRow);
        fprintf(valCompareFile, "val->mbsInCol                       0x%08X\n", val->mbsInCol);
        fprintf(valCompareFile, "val->qp                             0x%08X\n", val->qp);
        fprintf(valCompareFile, "val->disableQuarterPixelMv          0x%08X\n", val->disableQuarterPixelMv);
        fprintf(valCompareFile, "val->cabacInitIdc                   0x%08X\n", val->cabacInitIdc);
        fprintf(valCompareFile, "val->enableCabac                    0x%08X\n", val->enableCabac);
        fprintf(valCompareFile, "val->transform8x8Mode               0x%08X\n", val->transform8x8Mode);
        fprintf(valCompareFile, "val->h264Inter4x4Disabled           0x%08X\n", val->h264Inter4x4Disabled);
        fprintf(valCompareFile, "val->h264StrmMode                   0x%08X\n", val->h264StrmMode);
        fprintf(valCompareFile, "val->sliceSizeMbRows                0x%08X\n", val->sliceSizeMbRows);
        fprintf(valCompareFile, "val->firstFreeBit                   0x%08X\n", val->firstFreeBit);
        fprintf(valCompareFile, "val->xFill                          0x%08X\n", val->xFill);
        fprintf(valCompareFile, "val->yFill                          0x%08X\n", val->yFill);
        fprintf(valCompareFile, "val->inputChromaBaseOffset          0x%08X\n", val->inputChromaBaseOffset);
        fprintf(valCompareFile, "val->inputLumaBaseOffset            0x%08X\n", val->inputLumaBaseOffset);
        fprintf(valCompareFile, "val->pixelsOnRow                    0x%08X\n", val->pixelsOnRow);
        fprintf(valCompareFile, "val->internalImageLumBaseW          0x%08X\n", val->internalImageLumBaseW);
        fprintf(valCompareFile, "val->internalImageChrBaseW          0x%08X\n", val->internalImageChrBaseW);
        if (val->cpTarget != NULL) {
            for (iTest = 0; iTest < 10; ++iTest) {
                fprintf(valCompareFile, "val->cpTarget[%2d]                   0x%08X\n", iTest, val->cpTarget[iTest]);
            }
            for (iTest = 0; iTest < 6; ++iTest) {
                fprintf(valCompareFile, "val->targetError[%2d]                0x%08X\n", iTest, val->targetError[iTest]);
            }
            for (iTest = 0; iTest < 7; ++iTest) {
                fprintf(valCompareFile, "val->deltaQp[%2d]                    0x%08X\n", iTest, val->deltaQp[iTest]);
            }
        }
        fprintf(valCompareFile, "val->outputStrmBase                 0x%08X\n", val->outputStrmBase);
        fprintf(valCompareFile, "val->sizeTblBase.nal                0x%08X\n", val->sizeTblBase.nal);
        fprintf(valCompareFile, "val->sizeTblPresent                 0x%08X\n", val->sizeTblPresent);
        fprintf(valCompareFile, "val->madThreshold                   0x%08X\n", val->madThreshold);
        fprintf(valCompareFile, "val->inputImageFormat               0x%08X\n", val->inputImageFormat);
        fprintf(valCompareFile, "val->inputImageRotation             0x%08X\n", val->inputImageRotation);
        fprintf(valCompareFile, "val->sizeTblPresent                 0x%08X\n", val->sizeTblPresent);
        fprintf(valCompareFile, "val->intra16Favor                   0x%08X\n", val->intra16Favor);
        fprintf(valCompareFile, "val->interFavor                     0x%08X\n", val->interFavor);
        fprintf(valCompareFile, "val->picInitQp                      0x%08X\n", val->picInitQp);
        fprintf(valCompareFile, "val->sliceAlphaOffset               0x%08X\n", val->sliceAlphaOffset);
        fprintf(valCompareFile, "val->sliceBetaOffset                0x%08X\n", val->sliceBetaOffset);
        fprintf(valCompareFile, "val->chromaQpIndexOffset            0x%08X\n", val->chromaQpIndexOffset);
        fprintf(valCompareFile, "val->filterDisable                  0x%08X\n", val->filterDisable);
        fprintf(valCompareFile, "val->idrPicId                       0x%08X\n", val->idrPicId);
        fprintf(valCompareFile, "val->constrainedIntraPrediction     0x%08X\n", val->constrainedIntraPrediction);
        fprintf(valCompareFile, "val->vsNextLumaBase                 0x%08X\n", val->vsNextLumaBase);
        fprintf(valCompareFile, "val->riceWriteBase                  0x%08X\n", val->riceWriteBase);
        fprintf(valCompareFile, "val->cabacCtxBase                   0x%08X\n", val->cabacCtxBase);
        fprintf(valCompareFile, "val->roi1Top                        0x%08X\n", val->roi1Top);
        fprintf(valCompareFile, "val->roi1Bottom                     0x%08X\n", val->roi1Bottom);
        fprintf(valCompareFile, "val->roi1Left                       0x%08X\n", val->roi1Left);
        fprintf(valCompareFile, "val->roi1Right                      0x%08X\n", val->roi1Right);
        fprintf(valCompareFile, "val->roi2Top                        0x%08X\n", val->roi2Top);
        fprintf(valCompareFile, "val->roi2Bottom                     0x%08X\n", val->roi2Bottom);
        fprintf(valCompareFile, "val->roi2Left                       0x%08X\n", val->roi2Left);
        fprintf(valCompareFile, "val->roi2Right                      0x%08X\n", val->roi2Right);
        fprintf(valCompareFile, "val->vsMode                         0x%08X\n", val->vsMode);
        fprintf(valCompareFile, "val->colorConversionCoeffB          0x%08X\n", val->colorConversionCoeffB);
        fprintf(valCompareFile, "val->colorConversionCoeffA          0x%08X\n", val->colorConversionCoeffA);
        fprintf(valCompareFile, "val->colorConversionCoeffC          0x%08X\n", val->colorConversionCoeffC);
        fprintf(valCompareFile, "val->colorConversionCoeffE          0x%08X\n", val->colorConversionCoeffE);
        fprintf(valCompareFile, "val->colorConversionCoeffF          0x%08X\n", val->colorConversionCoeffF);
        fprintf(valCompareFile, "val->bMaskMsb                       0x%08X\n", val->bMaskMsb);
        fprintf(valCompareFile, "val->gMaskMsb                       0x%08X\n", val->gMaskMsb);
        fprintf(valCompareFile, "val->rMaskMsb                       0x%08X\n", val->rMaskMsb);
        fprintf(valCompareFile, "val->splitMvMode                    0x%08X\n", val->splitMvMode);
        fprintf(valCompareFile, "val->qpMax                          0x%08X\n", val->qpMax);
        fprintf(valCompareFile, "val->qpMin                          0x%08X\n", val->qpMin);
        fprintf(valCompareFile, "val->cpDistanceMbs                  0x%08X\n", val->cpDistanceMbs);
        fprintf(valCompareFile, "val->zeroMvFavorDiv2                0x%08X\n", val->zeroMvFavorDiv2);
        fprintf(valCompareFile, "val->frameCodingType                0x%08X\n", val->frameCodingType);
        fprintf(valCompareFile, "val->codingType                     0x%08X\n", val->codingType);
        fprintf(valCompareFile, "val->asicCfgReg                     0x%08X\n", val->asicCfgReg);
        fprintf(valCompareFile, "val->ppsId                          0x%08X\n", val->ppsId);
        fprintf(valCompareFile, "val->frameNum                       0x%08X\n", val->frameNum);
        fprintf(valCompareFile, "val->riceEnable                     0x%08X\n", val->riceEnable);
        fprintf(valCompareFile, "val->irqDisable                     0x%08X\n", val->irqDisable);
        fflush(valCompareFile);
        ++oneFrameFlagTest;
        if (valCompareFile != NULL && (oneFrameFlagTest == 239)) {
            fclose(valCompareFile);
            valCompareFile = NULL;
        }
    }
#endif
    // ----------------
    // add the h264e syntax data by lance 2016.05.07
    syntax_data->frame_coding_type = val->frameCodingType;
    syntax_data->pic_init_qp = val->picInitQp;
    syntax_data->slice_alpha_offset = val->sliceAlphaOffset;
    syntax_data->slice_beta_offset = val->sliceBetaOffset;
    syntax_data->chroma_qp_index_offset = val->chromaQpIndexOffset;
    syntax_data->filter_disable = val->filterDisable;
    syntax_data->idr_pic_id = val->idrPicId;
    syntax_data->pps_id = val->ppsId;
    syntax_data->frame_num = val->frameNum;
    syntax_data->slice_size_mb_rows = val->sliceSizeMbRows;
    syntax_data->h264_inter4x4_disabled = val->h264Inter4x4Disabled;
    syntax_data->enable_cabac = val->enableCabac;
    syntax_data->transform8x8_mode = val->transform8x8Mode;
    syntax_data->cabac_init_idc = val->cabacInitIdc;
    syntax_data->qp = val->qp;
    syntax_data->mad_qp_delta = val->madQpDelta;
    syntax_data->mad_threshold = val->madThreshold;
    syntax_data->qp_min = val->qpMin;
    syntax_data->qp_max = val->qpMax;
    syntax_data->cp_distance_mbs = val->cpDistanceMbs;
    if (val->cpTarget != NULL) {
        for (iCount = 0; iCount < 10; ++iCount) {
            syntax_data->cp_target[iCount] = val->cpTarget[iCount];
        }
        for (iCount = 0; iCount < 6; ++iCount) {
            syntax_data->target_error[iCount] = val->targetError[iCount];
        }
        for (iCount = 0; iCount < 7; ++iCount) {
            syntax_data->delta_qp[iCount] = val->deltaQp[iCount];
        }
    }
    syntax_data->output_strm_limit_size = val->outputStrmSize;
    syntax_data->output_strm_addr = val->outputStrmBase;
    syntax_data->cabac_table_addr = val->cabacCtxBase;
    syntax_data->pic_luma_width = instH264Encoder->preProcess.lumWidth;
    syntax_data->pic_luma_height = instH264Encoder->preProcess.lumHeight;
    syntax_data->ref_luma_addr = val->internalImageLumBaseR;  // need to talk with kesheng by lance 2016.05.07
    syntax_data->ref_chroma_addr = val->internalImageChrBaseR;  // need to talk with kesheng by lance 2016.05.07
    syntax_data->recon_luma_addr = val->internalImageLumBaseW;
    syntax_data->recon_chroma_addr = val->internalImageChrBaseW;
    syntax_data->input_luma_addr = val->inputLumBase;
    syntax_data->input_cb_addr = val->inputCbBase;
    syntax_data->input_cr_addr = val->inputCrBase;
    syntax_data->input_image_format = val->inputImageFormat;
    syntax_data->nal_size_table_addr = val->sizeTblBase.nal;
    syntax_data->color_conversion_coeff_a = val->colorConversionCoeffA;
    syntax_data->color_conversion_coeff_b = val->colorConversionCoeffB;
    syntax_data->color_conversion_coeff_c = val->colorConversionCoeffC;
    syntax_data->color_conversion_coeff_e = val->colorConversionCoeffE;
    syntax_data->color_conversion_coeff_f = val->colorConversionCoeffF;
    syntax_data->color_conversion_r_mask_msb = val->rMaskMsb;
    syntax_data->color_conversion_g_mask_msb = val->gMaskMsb;
    syntax_data->color_conversion_b_mask_msb = val->bMaskMsb;
    // ----------------
    //H264encFlushRegs(val);  // mask by lance 2016.05.06

}

void H264encFlushRegs(regValues_s * val)
{
    mpp_log("H264encFlushRegs val %p.\n", val);
#ifdef ANDROID
    u32 registerlen = 0;

    {
        int i;
        for (i = 0; i < ENC6820_REGISTERS; i++) {
            mpp_log("val->regMirror[%d]=0x%x\n", i, val->regMirror[i]);
        }
    }

    registerlen = ENC6820_REGISTERS;

    if (VPUClientSendReg(val->socket, val->regMirror, /*ENC6820_REGISTERS*/registerlen))
        mpp_log("H264encFlushRegs fail\n");
    else
        mpp_log("H264encFlushRegs success\n");
#endif
}

void EncAsicGetRegisters(/*const void *ewl, */regValues_s * val)  // mask by lance 2016.05.12
{

    /* HW output stream size, bits to bytes */
    val->outputStrmSize =
        val->regMirror[53] / 8; //EncAsicGetRegisterValue(ewl, val->regMirror, HEncStrmBufLimit) / 8;

    if (val->codingType != ASIC_JPEG && val->cpTarget != NULL) {
        /* video coding with MB rate control ON */
        u32 i, reg = 65;
        u32 cpt_prev = 0;
        u32 overflow = 0;

        for (i = 0; i < 10; i++) {
            u32 cpt;

            /* Checkpoint result div32 */
            cpt = ((val->regMirror[reg] >> (16 - 16 * (i & 1))) & 0xffff) * 32; //EncAsicGetRegisterValue(ewl, val->regMirror,
            //(regName)(HEncCP1WordTarget+i)) * 32;

            /* detect any overflow, overflow div32 is 65536 => overflow at 21 bits */
            if (cpt < cpt_prev)
                overflow += (1 << 21);
            cpt_prev = cpt;

            val->cpTargetResults[i] = cpt + overflow;
            reg = reg + (i & 1);
        }
    }

    /* QP sum div2 */
    val->qpSum = (val->regMirror[58] & 0x001fffff) * 2; //EncAsicGetRegisterValue(ewl, val->regMirror, HEncQpSum) * 2;

    /* MAD MB count*/
    val->madCount = (val->regMirror[104] & 0xffff);//EncAsicGetRegisterValue(ewl, val->regMirror, HEncMadCount);

    /* Non-zero coefficient count*/
    val->rlcCount = (val->regMirror[62] & 0x007fffff) * 4; //EncAsicGetRegisterValue(ewl, val->regMirror, HEncRlcSum) * 4;

    /* get stabilization results if needed */
    if (val->vsMode != 0) {
        //i32 i;  // mask by lance 2016.05.12
#if 0
        for (i = 40; i <= 50; i++) {
            val->regMirror[i] = EWLReadReg(ewl, HSWREG(i));
        }
#endif
    }

#ifdef TRACE_REGS
    EncTraceRegs(ewl, 1, EncAsicGetRegisterValue(ewl, val->regMirror, HEncMbCount));
#endif

}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicStop(/*const void *ewl*/)  // mask by lance 2016.05.12
{
//    EWLDisableHW(ewl, HSWREG(14), 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetStatus(regValues_s *val)
{
    return val->regMirror[109];
}

/*------------------------------------------------------------------------------

        ExpGolombSigned

------------------------------------------------------------------------------*/
i32 ExpGolombSigned(i32 val)
{
    i32 tmp = 0;

    if (val > 0) {
        val = 2 * val;
    } else {
        val = -2 * val + 1;
    }

    while (val >> ++tmp);

    /*
    H264NalBits(stream, val, tmp*2-1);
    */

    return tmp * 2 - 1;
}


