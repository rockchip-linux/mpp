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
#include "ewl.h"
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
static bool_e SetParameter(h264Instance_s * inst,
                           const H264EncConfig * pEncCfg);
static bool_e CheckParameter(const h264Instance_s * inst);

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
H264EncRet H264Init(const H264EncConfig * pEncCfg, h264Instance_s * pinst)
{
    h264Instance_s *inst = pinst;

    H264EncRet ret = H264ENC_OK;

    /* Default values */
    H264SeqParameterSetInit(&inst->seqParameterSet);
    H264PicParameterSetInit(&inst->picParameterSet);
    H264SliceInit(&inst->slice);

    /* Set parameters depending on user config */
    if (SetParameter(inst, pEncCfg) != ENCHW_OK) {
        ret = H264ENC_INVALID_ARGUMENT;
        goto err;
    }
    /* Check and init the rest of parameters */
    if (CheckParameter(inst) != ENCHW_OK) {
        ret = H264ENC_INVALID_ARGUMENT;
        goto err;
    }

    if (H264InitRc(&inst->rateControl) != ENCHW_OK) {
        return H264ENC_INVALID_ARGUMENT;
    }
    /* Initialize ASIC */
    inst->asic.ewl = NULL;//ewl;
    (void) EncAsicControllerInit(&inst->asic);

    /* Allocate internal SW/HW shared memories */
    if (EncAsicMemAlloc_V2(&inst->asic,
                           (u32) inst->preProcess.lumWidth,
                           (u32) inst->preProcess.lumHeight,
                           ASIC_H264) != ENCHW_OK) {

        ret = H264ENC_EWL_MEMORY_ERROR;
        goto err;
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

    /* When resolution larger than 720p = 3600 macroblocks
     * there is not enough time to do 1/4 pixel ME */
    if (inst->mbPerFrame > 3600)
        inst->asic.regs.disableQuarterPixelMv = 1;
    else
        inst->asic.regs.disableQuarterPixelMv = 0;

    inst->asic.regs.skipPenalty = 1;

    return ret;

err:
    /*if(inst != NULL)
        free(inst);*/
    /*if(ewl != NULL)
        (void) EWLRelease(ewl);*/

    return ret;
}

/*------------------------------------------------------------------------------

    H264Shutdown

    Function frees the encoder instance.

    Input   h264Instance_s *    Pointer to the encoder instance to be freed.
                            After this the pointer is no longer valid.

------------------------------------------------------------------------------*/
void H264Shutdown(h264Instance_s * data)
{
    ASSERT(data);

    EncAsicMemFree_V2(&data->asic);
}

/*------------------------------------------------------------------------------

    SetParameter

    Set all parameters in instance to valid values depending on user config.

------------------------------------------------------------------------------*/
bool_e SetParameter(h264Instance_s * inst, const H264EncConfig * pEncCfg)
{
    i32 width, height, tmp, bps;
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

    /* Disable intra and ROI areas by default */
    inst->asic.regs.intraAreaTop = inst->asic.regs.intraAreaBottom = inst->mbPerCol;
    inst->asic.regs.intraAreaLeft = inst->asic.regs.intraAreaRight = inst->mbPerRow;
    inst->asic.regs.roi1Top = inst->asic.regs.roi1Bottom = inst->mbPerCol;
    inst->asic.regs.roi1Left = inst->asic.regs.roi1Right = inst->mbPerRow;
    inst->asic.regs.roi2Top = inst->asic.regs.roi2Bottom = inst->mbPerCol;
    inst->asic.regs.roi2Left = inst->asic.regs.roi2Right = inst->mbPerRow;

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
    if (pEncCfg->level == H264ENC_LEVEL_1_b) {
        inst->seqParameterSet.levelIdc = 11;
        inst->seqParameterSet.constraintSet3 = ENCHW_YES;
    }

    /* Get the index for the table of level maximum values */
    tmp = H264GetLevelIndex(inst->seqParameterSet.levelIdc);
    if (tmp == INVALID_LEVEL)
        return ENCHW_NOK;

    inst->seqParameterSet.levelIdx = tmp;

#if 1   /* enforce maximum frame size in level */
    if (inst->mbPerFrame > H264MaxFS[inst->seqParameterSet.levelIdx]) {
        return ENCHW_NOK;
    }
#endif

#if 0   /* enforce macroblock rate limit in level */
    {
        u32 mb_rate =
            (pEncCfg->frameRateNum * inst->mbPerFrame) /
            pEncCfg->frameRateDenom;

        if (mb_rate > H264MaxMBPS[inst->seqParameterSet.levelIdx]) {
            return ENCHW_NOK;
        }
    }
#endif

    /* Picture parameter set */
    inst->picParameterSet.picInitQpMinus26 = (i32) H264ENC_DEFAULT_QP - 26;

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
        H264GetAllowedWidth(pEncCfg->width, H264ENC_YUV420_PLANAR);

    inst->preProcess.lumHeight = pEncCfg->height;
    inst->preProcess.lumHeightSrc = pEncCfg->height;

    inst->preProcess.horOffsetSrc = 0;
    inst->preProcess.verOffsetSrc = 0;

    inst->preProcess.rotation = ROTATE_0;
    inst->preProcess.inputFormat = H264ENC_YUV420_PLANAR;
    inst->preProcess.videoStab = 0;

    inst->preProcess.colorConversionType = 0;
    EncSetColorConversion(&inst->preProcess, &inst->asic);

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    CheckParameter

------------------------------------------------------------------------------*/
bool_e CheckParameter(const h264Instance_s * inst)
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
i32 H264GetAllowedWidth(i32 width, H264EncPictureFormat inputType)
{
    if (inputType == H264ENC_YUV420_PLANAR) {
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

H264EncRet H264Cfg(const H264EncConfig * pEncCfg, h264Instance_s * pinst)
{
    h264Instance_s *inst = pinst;

    H264EncRet ret = H264ENC_OK;

    /* Set parameters depending on user config */
    if (SetParameter(inst, pEncCfg) != ENCHW_OK) {
        ret = H264ENC_INVALID_ARGUMENT;
    }

    return ret;
}

