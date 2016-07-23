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
    1. Include headers
------------------------------------------------------------------------------*/
#include "mpp_log.h"  // add by lance 2016.05.06
#include "enccommon.h"
#include "ewl.h"
#include "H264CodeFrame.h"
#include "h264e_macro.h"
//#include <cutils/properties.h>  // mask by lance 2016.05.05

#ifdef SYNTAX_DATA_IN_FILE
extern FILE *fp_syntax_in;
#endif

/*------------------------------------------------------------------------------
    2. External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

static const u32 h264Intra16Favor[52] = {
    24, 24, 24, 26, 27, 30, 32, 35, 39, 43, 48, 53, 58, 64, 71, 78,
    85, 93, 102, 111, 121, 131, 142, 154, 167, 180, 195, 211, 229,
    248, 271, 296, 326, 361, 404, 457, 523, 607, 714, 852, 1034,
    1272, 1588, 2008, 2568, 3318, 4323, 5672, 7486, 9928, 13216,
    17648
};

static const u32 h264InterFavor[52] = {
    40, 40, 41, 42, 43, 44, 45, 48, 51, 53, 55, 60, 62, 67, 69, 72,
    78, 84, 90, 96, 110, 120, 135, 152, 170, 189, 210, 235, 265,
    297, 335, 376, 420, 470, 522, 572, 620, 670, 724, 770, 820,
    867, 915, 970, 1020, 1076, 1132, 1180, 1230, 1275, 1320, 1370
};

static const u32 h264SkipSadPenalty[52] = {     //rk29
    255, 255, 255, 255, 255, 255, 255, 255, 255, 224,
    208, 192, 176, 160, 144, 128, 112, 96, 80, 64,
    56, 48, 44, 40, 36, 32, 28, 24, 22, 20,
    18, 16, 12, 11, 10, 9, 8, 7, 5, 5,
    4, 4, 3, 3, 2, 2, 1, 1, 1, 1,
    0, 0
};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void H264SetNewFrame(h264Instance_s * inst);


//static struct timeval tv1, tv2;  // mask by lance 2016.05.05
//static int64_t total = 0;  // mask by lance 2016.05.05
static RK_S64 encNum = 0;

/*------------------------------------------------------------------------------

    H264CodeFrame

------------------------------------------------------------------------------*/
void H264CodeFrame(h264Instance_s * inst, h264e_syntax *syntax_data)
{
    H264SetNewFrame(inst);
    if (!inst->time_debug_init) {
        inst->time_debug_init = 1;
        encNum = 0;
    }

    EncAsicFrameStart((void*)inst,/* inst->asic.ewl,*/ &inst->asic.regs, syntax_data);  // mask by lance 2016.05.12
#ifdef SYNTAX_DATA_IN_FILE
    if (NULL != fp_syntax_in) {
        regValues_s *syn = &inst->asic.regs;
        int k = 0;
        FILE *fp = fp_syntax_in;
        //VPU_DEBUG("on2_syntax_in.txt open");
        fprintf(fp, "#FRAME %d:\n", inst->frameCnt);

        fprintf(fp, "%-16d %s\n", syn->frameCodingType, "frame_coding_type");
        fprintf(fp, "%-16d %s\n", syn->picInitQp, "pic_init_qp");
        fprintf(fp, "%-16d %s\n", syn->sliceAlphaOffset, "slice_alpha_offset");
        fprintf(fp, "%-16d %s\n", syn->sliceBetaOffset, "slice_beta_offset");
        fprintf(fp, "%-16d %s\n", syn->chromaQpIndexOffset, "chroma_qp_index_offset");
        fprintf(fp, "%-16d %s\n", syn->filterDisable, "filter_disable");
        fprintf(fp, "%-16d %s\n", syn->idrPicId, "idr_pic_id");
        fprintf(fp, "%-16d %s\n", syn->ppsId, "pps_id");
        fprintf(fp, "%-16d %s\n", syn->frameNum, "frame_num");
        fprintf(fp, "%-16d %s\n", syn->sliceSizeMbRows, "slice_size_mb_rows");
        fprintf(fp, "%-16d %s\n", syn->h264Inter4x4Disabled, "h264_inter4x4_disabled");
        fprintf(fp, "%-16d %s\n", syn->enableCabac, "enable_cabac");
        fprintf(fp, "%-16d %s\n", syn->transform8x8Mode, "transform8x8_mode");
        fprintf(fp, "%-16d %s\n", syn->cabacInitIdc, "cabac_init_idc");
        fprintf(fp, "%-16d %s\n", syn->qp, "qp");
        fprintf(fp, "%-16d %s\n", syn->madQpDelta, "mad_qp_delta");
        fprintf(fp, "%-16d %s\n", syn->madThreshold, "mad_threshold");
        fprintf(fp, "%-16d %s\n", syn->qpMin, "qp_min");
        fprintf(fp, "%-16d %s\n", syn->qpMax, "qp_max");
        fprintf(fp, "%-16d %s\n", syn->cpDistanceMbs, "cp_distance_mbs");
        for (k = 0; k < 10; k++)
            fprintf(fp, "%-16d cp_target[%d]\n", syn->cpTarget[k], k);
        for (k = 0; k < 7; k++)
            fprintf(fp, "%-16d target_error[%d]\n", syn->targetError[k], k);
        for (k = 0; k < 7; k++)
            fprintf(fp, "%-16d delta_qp[%d]\n", syn->deltaQp[k], k);
        fprintf(fp, "%-16d %s\n", syn->outputStrmSize, "output_strm_limit_size");
        fprintf(fp, "%-16d %s\n", inst->preProcess.lumWidthSrc, "pic_luma_width");
        fprintf(fp, "%-16d %s\n", inst->preProcess.lumHeightSrc, "pic_luma_height");
        fprintf(fp, "%-16d %s\n", syn->inputImageFormat, "input_image_format");
        fprintf(fp, "%-16d %s\n", syn->colorConversionCoeffA, "color_conversion_coeff_a");
        fprintf(fp, "%-16d %s\n", syn->colorConversionCoeffB, "color_conversion_coeff_b");
        fprintf(fp, "%-16d %s\n", syn->colorConversionCoeffC, "color_conversion_coeff_c");
        fprintf(fp, "%-16d %s\n", syn->colorConversionCoeffE, "color_conversion_coeff_e");
        fprintf(fp, "%-16d %s\n", syn->colorConversionCoeffF, "color_conversion_coeff_f");
        fprintf(fp, "%-16d %s\n", syn->rMaskMsb, "color_conversion_r_mask_msb");
        fprintf(fp, "%-16d %s\n", syn->gMaskMsb, "color_conversion_g_mask_msb");
        fprintf(fp, "%-16d %s\n", syn->bMaskMsb, "color_conversion_b_mask_msb");

        fprintf(fp, "\n");
        fflush(fp);
    }
#endif
    /* Reset the favor values for next frame */
    inst->asic.regs.intra16Favor = 0;
    inst->asic.regs.interFavor = 0;
    inst->asic.regs.skipPenalty = 1;

    return;
}

/*------------------------------------------------------------------------------

    Set encoding parameters at the beginning of a new frame.

------------------------------------------------------------------------------*/
void H264SetNewFrame(h264Instance_s * inst)
{
    regValues_s *regs = &inst->asic.regs;

    regs->outputStrmSize -= inst->stream.byteCnt;
    regs->outputStrmSize /= 8;  /* 64-bit addresses */
    regs->outputStrmSize &= (~0x07);    /* 8 multiple size */

    /* 64-bit aligned stream base address */
    regs->outputStrmBase += (inst->stream.byteCnt & (~0x07));

    /* bit offset in the last 64-bit word */
    regs->firstFreeBit = (inst->stream.byteCnt & 0x07) * 8;

    /* header remainder is byte aligned, max 7 bytes = 56 bits */
    if (regs->firstFreeBit != 0) {
        /* 64-bit aligned stream pointer */
        u8 *pTmp = (u8 *) ((size_t) (inst->stream.stream) & (u32) (~0x07));
        u32 val;

        /* Clear remaining bits */
        for (val = 6; val >= regs->firstFreeBit / 8; val--)
            pTmp[val] = 0;

        val = pTmp[0] << 24;
        val |= pTmp[1] << 16;
        val |= pTmp[2] << 8;
        val |= pTmp[3];

        regs->strmStartMSB = val;  /* 32 bits to MSB */

        if (regs->firstFreeBit > 32) {
            val = pTmp[4] << 24;
            val |= pTmp[5] << 16;
            val |= pTmp[6] << 8;

            regs->strmStartLSB = val;
        } else
            regs->strmStartLSB = 0;
    } else {
        regs->strmStartMSB = regs->strmStartLSB = 0;
    }

    regs->frameNum = inst->slice.frameNum;
    regs->idrPicId = inst->slice.idrPicId;

    /* Store the final register values in the register structure */
    regs->sliceSizeMbRows = inst->slice.sliceSize / inst->mbPerRow;
    regs->chromaQpIndexOffset = inst->picParameterSet.chromaQpIndexOffset;

    regs->picInitQp = (u32) (inst->picParameterSet.picInitQpMinus26 + 26);

    regs->qp = inst->rateControl.qpHdr;
    regs->qpMin = inst->rateControl.qpMin;
    regs->qpMax = inst->rateControl.qpMax;

    if (inst->rateControl.mbRc) {
        regs->cpTarget = (u32 *) inst->rateControl.qpCtrl.wordCntTarget;
        regs->targetError = inst->rateControl.qpCtrl.wordError;
        regs->deltaQp = inst->rateControl.qpCtrl.qpChange;

        regs->cpDistanceMbs = inst->rateControl.qpCtrl.checkPointDistance;

        regs->cpTargetResults = (u32 *) inst->rateControl.qpCtrl.wordCntPrev;
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
    regs->constrainedIntraPrediction =
        (inst->picParameterSet.constIntraPred == ENCHW_YES) ? 1 : 0;

    regs->frameCodingType =
        (inst->slice.sliceType == ISLICE) ? ASIC_INTRA : ASIC_INTER;

    /* If favor has not been set earlier by testId use default */
    if (regs->intra16Favor == 0)
        regs->intra16Favor = h264Intra16Favor[regs->qp];
    if (regs->interFavor == 0)
        regs->interFavor = h264InterFavor[regs->qp];
    if (regs->skipPenalty == 1)
        regs->skipPenalty = h264SkipSadPenalty[regs->qp];

    /* MAD threshold range [0, 63*256] register 6-bits range [0,63] */
    regs->madThreshold = inst->mad.threshold / 256;
    regs->madQpDelta = inst->rateControl.mbQpAdjustment;


}
