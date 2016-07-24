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

#include "mpp_log.h"
#include "enccommon.h"
#include "encasiccontroller.h"
#include "H264Instance.h"
#include "encpreprocess.h"
#include "ewl.h"
#include <stdio.h>

/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

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

//#define WRITE_VAL_INFO_FOR_COMPARE 1
#ifdef WRITE_VAL_INFO_FOR_COMPARE
FILE *valCompareFile = NULL;
int oneFrameFlagTest = 0;
#endif

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

    /* User must set these */
    asic->regs.inputLumBase = 0;
    asic->regs.inputCbBase = 0;
    asic->regs.inputCrBase = 0;

    /* we do NOT reset hardware at this point because */
    /* of the multi-instance support                  */
#ifdef WRITE_VAL_INFO_FOR_COMPARE
    oneFrameFlagTest = 0;
    if (valCompareFile == NULL) {
        if ((valCompareFile = fopen("/data/test/val_cmp.file", "w")) == NULL)
            mpp_err("val original File open failed!");
    }
#endif
    return ENCHW_OK;
}

void EncAsicFrameStart(void * inst, regValues_s * val, h264e_syntax *syntax_data)
{
    h264Instance_s *instH264Encoder = (h264Instance_s *)inst;
    int iCount = 0;
#ifdef WRITE_VAL_INFO_FOR_COMPARE
    int iTest = 0;
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
        fprintf(valCompareFile, "val->madThreshold                   0x%08X\n", val->madThreshold);
        fprintf(valCompareFile, "val->inputImageFormat               0x%08X\n", val->inputImageFormat);
        fprintf(valCompareFile, "val->inputImageRotation             0x%08X\n", val->inputImageRotation);
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
        fprintf(valCompareFile, "val->irqDisable                     0x%08X\n", val->irqDisable);
        fflush(valCompareFile);
        ++oneFrameFlagTest;
        if (valCompareFile != NULL && (oneFrameFlagTest == 239)) {
            fclose(valCompareFile);
            valCompareFile = NULL;
        }
    }
#endif

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
    syntax_data->pic_luma_width = instH264Encoder->preProcess.lumWidth;
    syntax_data->pic_luma_height = instH264Encoder->preProcess.lumHeight;
    syntax_data->input_luma_addr = val->inputLumBase;
    syntax_data->input_cb_addr = val->inputCbBase;
    syntax_data->input_cr_addr = val->inputCrBase;
    syntax_data->input_image_format = val->inputImageFormat;
    syntax_data->color_conversion_coeff_a = val->colorConversionCoeffA;
    syntax_data->color_conversion_coeff_b = val->colorConversionCoeffB;
    syntax_data->color_conversion_coeff_c = val->colorConversionCoeffC;
    syntax_data->color_conversion_coeff_e = val->colorConversionCoeffE;
    syntax_data->color_conversion_coeff_f = val->colorConversionCoeffF;
    syntax_data->color_conversion_r_mask_msb = val->rMaskMsb;
    syntax_data->color_conversion_g_mask_msb = val->gMaskMsb;
    syntax_data->color_conversion_b_mask_msb = val->bMaskMsb;
    // ----------------
}

