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
#define mask_2b         (RK_U32)0x00000003
#define mask_3b         (RK_U32)0x00000007
#define mask_4b         (RK_U32)0x0000000F
#define mask_5b         (RK_U32)0x0000001F
#define mask_6b         (RK_U32)0x0000003F
#define mask_11b        (RK_U32)0x000007FF
#define mask_14b        (RK_U32)0x00003FFF
#define mask_16b        (RK_U32)0x0000FFFF

#define HSWREG(n)       ((n)*4)

RK_S32 EncAsicControllerInit(asicData_s * asic)
{
    ASSERT(asic != NULL);

    /* Initialize default values */
    asic->regs.cpDistanceMbs = 0;

    /* User must set these */
    asic->regs.inputLumBase = 0;
    asic->regs.inputCbBase = 0;
    asic->regs.inputCrBase = 0;

    /* we do NOT reset hardware at this point because */
    /* of the multi-instance support                  */
    return ENCHW_OK;
}

void EncAsicFrameStart(void * inst, regValues_s * val, h264e_syntax *syntax_data)
{
    H264ECtx *instH264Encoder = (H264ECtx *)inst;
    int iCount = 0;
    // TODO for rkv
    syntax_data->pic_order_cnt_lsb = 2 * val->frameNum;
    syntax_data->second_chroma_qp_index_offset = val->chromaQpIndexOffset;
    syntax_data->slice_type = instH264Encoder->slice.sliceType;

    syntax_data->frame_coding_type = val->frameCodingType;
    syntax_data->pic_init_qp = val->picInitQp;
    syntax_data->slice_alpha_offset = val->sliceAlphaOffset;
    syntax_data->slice_beta_offset = val->sliceBetaOffset;
    syntax_data->chroma_qp_index_offset = val->chromaQpIndexOffset;
    syntax_data->filter_disable = val->filterDisable;
    syntax_data->idr_pic_id = val->idrPicId;
    syntax_data->pps_id = 0;
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

