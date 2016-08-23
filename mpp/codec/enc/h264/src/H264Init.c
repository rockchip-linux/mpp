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
#include "H264Init.h"
#include "enccommon.h"
#include "mpp_log.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
#define H264ENC_MIN_ENC_WIDTH       96
#define H264ENC_MAX_ENC_WIDTH       1920
#define H264ENC_MIN_ENC_HEIGHT      96
#define H264ENC_MAX_ENC_HEIGHT      1920

#define H264ENC_MAX_MBS_PER_PIC     8160    /* 1920x1088 */

#define H264ENC_MAX_LEVEL           41

#define H264ENC_DEFAULT_QP          26

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static bool_e SetParameter(H264ECtx * inst,
                           const H264EncConfig * pEncCfg);
static bool_e CheckParameter(const H264ECtx * inst);

/*------------------------------------------------------------------------------

    H264Init

    Function initializes the Encoder and create new encoder instance.

    Input   pEncCfg     Encoder configuration.
            instAddr    Pointer to instance will be stored in this address

    Return  H264ENC_OK
            H264ENC_MEMORY_ERROR
            H264ENC_EWL_ERROR
            H264ENC_EWL_MEMORY_ERROR
            H264ENC_INVALID_ARGUMENT

------------------------------------------------------------------------------*/
H264EncRet H264Init(H264ECtx * pinst)
{
    H264ECtx *inst = pinst;

    H264EncRet ret = H264ENC_OK;

    /* Default values */
    H264SeqParameterSetInit(&inst->seqParameterSet);
    H264PicParameterSetInit(&inst->picParameterSet);
    H264SliceInit(&inst->slice);

    /* Initialize ASIC */
    EncAsicControllerInit(&inst->asic);

    return ret;
}

/*------------------------------------------------------------------------------

    SetParameter

    Set all parameters in instance to valid values depending on user config.

------------------------------------------------------------------------------*/
bool_e SetParameter(H264ECtx * inst, const H264EncConfig * pEncCfg)
{
    RK_S32 width, height, tmp, bps;
    ASSERT(inst);

    /* Internal images, next macroblock boundary */
    width = 16 * ((pEncCfg->width + 15) / 16);
    height = 16 * ((pEncCfg->height + 15) / 16);

    /* stream type */
    if (pEncCfg->streamType == H264ENC_BYTE_STREAM) {
        inst->asic.regs.h264StrmMode = ASIC_H264_BYTE_STREAM;
        inst->picParameterSet.byteStream = ENCHW_YES;
        inst->seqParameterSet.byteStream = ENCHW_YES;
        inst->rateControl.sei.byteStream = ENCHW_YES;
        inst->slice.byteStream = ENCHW_YES;
    } else {
        inst->asic.regs.h264StrmMode = ASIC_H264_NAL_UNIT;
        inst->picParameterSet.byteStream = ENCHW_NO;
        inst->seqParameterSet.byteStream = ENCHW_NO;
        inst->rateControl.sei.byteStream = ENCHW_NO;
        inst->slice.byteStream = ENCHW_NO;
    }

    /* Slice */
    inst->slice.sliceSize = 0;

    /*luma width and height*/
    inst->lumWidthSrc = pEncCfg->width;
    inst->lumHeightSrc = pEncCfg->height;

    /* Macroblock */
    inst->mbPerFrame = width / 16 * height / 16;
    inst->mbPerRow = width / 16;
    inst->mbPerCol = height / 16;

    /* Sequence parameter set */
    inst->seqParameterSet.levelIdc = pEncCfg->level;
    inst->seqParameterSet.picWidthInMbsMinus1 = width / 16 - 1;
    inst->seqParameterSet.picHeightInMapUnitsMinus1 = height / 16 - 1;

    /* Set cropping parameters if required */
    if ( pEncCfg->width % 16 || pEncCfg->height % 16 ) {
        inst->seqParameterSet.frameCropping = ENCHW_YES;
        inst->seqParameterSet.frameCropRightOffset = (width - pEncCfg->width) / 2 ;
        inst->seqParameterSet.frameCropBottomOffset = (height - pEncCfg->height) / 2;
    }

    /* Level 1b is indicated with levelIdc == 11 and constraintSet3 */
    if (pEncCfg->level == H264_LEVEL_1_b) {
        inst->seqParameterSet.levelIdc = 11;
        inst->seqParameterSet.constraintSet3 = ENCHW_YES;
    }

    /* Get the index for the table of level maximum values */
    tmp = H264GetLevelIndex(inst->seqParameterSet.levelIdc);
    if (tmp == INVALID_LEVEL) {
        mpp_err("H264GetLevelIndex failed\n");
        return ENCHW_NOK;
    }

    inst->seqParameterSet.levelIdx = tmp;

    /* enforce maximum frame size in level */
    if (inst->mbPerFrame > H264MaxFS[inst->seqParameterSet.levelIdx]) {
        while (inst->mbPerFrame > H264MaxFS[inst->seqParameterSet.levelIdx])
            inst->seqParameterSet.levelIdx++;
    }

    /* enforce macroblock rate limit in level */
    {
        RK_U32 mb_rate = (pEncCfg->frameRateNum * inst->mbPerFrame) /
                         pEncCfg->frameRateDenom;

        if (mb_rate > H264MaxMBPS[inst->seqParameterSet.levelIdx]) {
            mpp_log("input mb rate %d is larger than restriction %d\n",
                    inst->mbPerFrame, H264MaxMBPS[inst->seqParameterSet.levelIdx]);
        }
    }

    /* Picture parameter set */
    inst->picParameterSet.picInitQpMinus26 = (RK_S32) H264ENC_DEFAULT_QP - 26;

    /* Rate control setup */

    /* Maximum bitrate for the specified level */
    bps = H264MaxBR[inst->seqParameterSet.levelIdx];

    {
        h264RateControl_s *rc = &inst->rateControl;

        rc->outRateDenom = pEncCfg->frameRateDenom;
        rc->outRateNum = pEncCfg->frameRateNum;
        rc->mbPerPic = (width / 16) * (height / 16);
        rc->mbRows = height / 16;

        {
            h264VirtualBuffer_s *vb = &rc->virtualBuffer;

            vb->bitRate = bps;
            vb->unitsInTic = pEncCfg->frameRateDenom;
            vb->timeScale = pEncCfg->frameRateNum;
            vb->bufferSize = H264MaxCPBS[inst->seqParameterSet.levelIdx];
        }

        rc->hrd = ENCHW_YES;
        rc->picRc = ENCHW_YES;
        rc->mbRc = ENCHW_YES;
        rc->picSkip = ENCHW_NO;

        rc->qpHdr = H264ENC_DEFAULT_QP;
        rc->qpMin = 10;
        rc->qpMax = 51;

        rc->frameCoded = ENCHW_YES;
        rc->sliceTypeCur = ISLICE;
        rc->sliceTypePrev = PSLICE;
        rc->gopLen = 150;

        /* Default initial value for intra QP delta */
        rc->intraQpDelta = -3;
        rc->fixedIntraQp = 0;
    }

    /* no SEI by default */
    inst->rateControl.sei.enabled = ENCHW_NO;

    /* Pre processing */
    inst->preProcess.lumWidth = pEncCfg->width;
    inst->preProcess.lumWidthSrc =
        H264GetAllowedWidth(pEncCfg->width, MPP_FMT_YUV420P);

    inst->preProcess.lumHeight = pEncCfg->height;
    inst->preProcess.lumHeightSrc = pEncCfg->height;

    inst->preProcess.horOffsetSrc = 0;
    inst->preProcess.verOffsetSrc = 0;

    inst->preProcess.rotation = ROTATE_0;
    inst->preProcess.inputFormat = pEncCfg->input_image_format;
    inst->preProcess.videoStab = 0;

    inst->preProcess.colorConversionType = 0;
    EncSetColorConversion(&inst->preProcess, &inst->asic);

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    CheckParameter

------------------------------------------------------------------------------*/
bool_e CheckParameter(const H264ECtx * inst)
{
    /* Check crop */
    if (EncPreProcessCheck(&inst->preProcess) != ENCHW_OK) {
        return ENCHW_NOK;
    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    Round the width to the next multiple of 8 or 16 depending on YUV type.

------------------------------------------------------------------------------*/
RK_S32 H264GetAllowedWidth(RK_S32 width, MppFrameFormat inputType)
{
    if (inputType == MPP_FMT_YUV420P) {
        /* Width must be multiple of 16 to make
         * chrominance row 64-bit aligned */
        return ((width + 15) / 16) * 16;
    } else {
        /* H264ENC_YUV420_SEMIPLANAR */
        /* H264ENC_YUV422_INTERLEAVED_YUYV */
        /* H264ENC_YUV422_INTERLEAVED_UYVY */
        return ((width + 7) / 8) * 8;
    }
}

H264EncRet H264Cfg(const H264EncConfig * pEncCfg, H264ECtx * pinst)
{
    H264ECtx *inst = pinst;

    H264EncRet ret = H264ENC_OK;

    /* Set parameters depending on user config */
    if (SetParameter(inst, pEncCfg) != ENCHW_OK) {
        ret = H264ENC_INVALID_ARGUMENT;
        mpp_err_f("SetParameter failed\n");
        return ret;
    }

    /* Check and init the rest of parameters */
    if (CheckParameter(inst) != ENCHW_OK) {
        ret = H264ENC_INVALID_ARGUMENT;
        mpp_err_f("CheckParameter failed\n");
        return ret;
    }

    if (H264InitRc(&inst->rateControl) != ENCHW_OK) {
        mpp_err_f("H264InitRc failed\n");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* init VUI */
    {
        const h264VirtualBuffer_s *vb = &inst->rateControl.virtualBuffer;

        H264SpsSetVuiTimigInfo(&inst->seqParameterSet,
                               vb->timeScale, vb->unitsInTic);
    }

    if (inst->seqParameterSet.levelIdc >= 31)
        inst->asic.regs.h264Inter4x4Disabled = 1;
    else
        inst->asic.regs.h264Inter4x4Disabled = 0;

    return ret;
}

