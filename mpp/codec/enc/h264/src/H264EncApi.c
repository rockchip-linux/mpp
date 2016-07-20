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
       Version Information
------------------------------------------------------------------------------*/

#define H264ENC_MAJOR_VERSION 1
#define H264ENC_MINOR_VERSION 0

#define H264ENC_BUILD_MAJOR 1
#define H264ENC_BUILD_MINOR 33
#define H264ENC_BUILD_REVISION 0
#define H264ENC_SW_BUILD ((H264ENC_BUILD_MAJOR * 1000000) + \
(H264ENC_BUILD_MINOR * 1000) + H264ENC_BUILD_REVISION)

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include <string.h>

#include "mpp_log.h"
#include "h264encapi.h"
#include "enccommon.h"
#include "H264Instance.h"
#include "H264Init.h"
#include "H264PutBits.h"
#include "H264CodeFrame.h"
#include "H264Sei.h"
#include "H264RateControl.h"
#define LOG_TAG "H264_ENC"
//#include <utils/Log.h>  // mask by lance 2016.05.05
#ifdef INTERNAL_TEST
#include "H264TestId.h"
#endif

#ifdef VIDEOSTAB_ENABLED
#include "vidstabcommon.h"
#endif

#include "ewl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

#define EVALUATION_LIMIT 300    max number of frames to encode

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* Parameter limits */
#define H264ENCSTRMSTART_MIN_BUF        64
#define H264ENCSTRMENCODE_MIN_BUF       4096
#define H264ENC_MAX_PP_INPUT_WIDTH      4096
#define H264ENC_MAX_PP_INPUT_HEIGHT     4096
#define H264ENC_MAX_BITRATE             (50000*1200)    /* Level 4.1 limit */
#define H264ENC_MAX_USER_DATA_SIZE      2048

#define H264ENC_IDR_ID_MODULO           16

#define H264_BUS_ADDRESS_VALID(bus_address)  (((bus_address) != 0)/* &&*/ \
                                              /*((bus_address & 0x07) == 0)*/)

/* HW ID check. H264EncInit() will fail if HW doesn't match. */
#define HW_ID_MASK  0xFFFF0000
#define HW_ID       0x82700000

//#define H264ENC_TRACE

void H264EncTrace(const char *msg)
{
    /* static FILE *fp = NULL;

     if(fp == NULL)
         fp = fopen("api.trc", "wt");

     if(fp)
         fprintf(fp, "%s\n", msg);*/
    mpp_log("%s\n", msg);
}

#define H264ENC_TRACE 1  // add for debuging modify by lance 2016.07.02
/* Tracing macro */
#ifdef H264ENC_TRACE
#define APITRACE(str) H264EncTrace(str)
#else
#define APITRACE(str)
#endif

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

static void H264GetNalUnitSizes(const h264Instance_s * pEncInst,
                                u32 * pNaluSizeBuf);

static i32 VSCheckSize(u32 inputWidth, u32 inputHeight, u32 stabilizedWidth,
                       u32 stabilizedHeight);

/*------------------------------------------------------------------------------

    Function name : H264EncGetApiVersion
    Description   : Return the API version info

    Return type   : H264EncApiVersion
    Argument      : void
------------------------------------------------------------------------------*/
H264EncApiVersion H264EncGetApiVersion(void)
{
    H264EncApiVersion ver;

    ver.major = H264ENC_MAJOR_VERSION;
    ver.minor = H264ENC_MINOR_VERSION;

    APITRACE("H264EncGetApiVersion# OK");
    return ver;
}

/*------------------------------------------------------------------------------
    Function name : H264EncGetBuild
    Description   : Return the SW and HW build information

    Return type   : H264EncBuild
    Argument      : void
------------------------------------------------------------------------------*/
H264EncBuild H264EncGetBuild(void)
{
    H264EncBuild ver;

    ver.swBuild = H264ENC_SW_BUILD;
    ver.hwBuild = 0x82701110;//EWLReadAsicID();

    APITRACE("H264EncGetBuild# OK");

    return (ver);
}

/*------------------------------------------------------------------------------
    Function name : H264EncInit
    Description   : Initialize an encoder instance and returns it to application

    Return type   : H264EncRet
    Argument      : pEncCfg - initialization parameters
                    instAddr - where to save the created instance
------------------------------------------------------------------------------*/
H264EncRet H264EncInit(const H264EncConfig * pEncCfg, h264Instance_s * instAddr)
{
    H264EncRet ret;
    h264Instance_s *pEncInst = instAddr;

    //APITRACE("H264EncInit#");
    /* check that right shift on negative numbers is performed signed */
    /*lint -save -e* following check causes multiple lint messages */
#if (((-1) >> 1) != (-1))
#error Right bit-shifting (>>) does not preserve the sign
#endif
    /*lint -restore */
    /* Check for illegal inputs */
    if (pEncCfg == NULL || instAddr == NULL) {
        APITRACE("H264EncInit: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }
    /* Check for correct HW */
    if ((0x82701110 & HW_ID_MASK) != HW_ID) {
        APITRACE("H264EncInit: ERROR Invalid HW ID");
        return H264ENC_ERROR;
    }

    /* Check that configuration is valid */
    if (H264CheckCfg(pEncCfg) == ENCHW_NOK) {
        APITRACE("H264EncInit: ERROR Invalid configuration");
        return H264ENC_INVALID_ARGUMENT;
    }
    /* Initialize encoder instance and allocate memories */
    ret = H264Init(pEncCfg, pEncInst);
    if (ret != H264ENC_OK) {
        mpp_log/*APITRACE*/("H264EncInit: ERROR Initialization failed");
        return ret;
    }

    /* Status == INIT   Initialization succesful */
    pEncInst->encStatus = H264ENCSTAT_INIT;

    pEncInst->inst = pEncInst;  /* used as checksum */

//    *instAddr = (H264EncInst) pEncInst;

    mpp_log/*APITRACE*/("H264EncInit: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncRelease
    Description   : Releases encoder instance and all associated resource

    Return type   : H264EncRet
    Argument      : inst - the instance to be released
------------------------------------------------------------------------------*/
H264EncRet H264EncRelease(H264EncInst inst)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    APITRACE("H264EncRelease#");

    /* Check for illegal inputs */
    if (pEncInst == NULL) {
        APITRACE("H264EncRelease: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncRelease: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

#ifdef TRACE_STREAM
    EncCloseStreamTrace();
#endif

    H264Shutdown(pEncInst);

    APITRACE("H264EncRelease: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncSetCodingCtrl
    Description   : Sets encoding parameters

    Return type   : H264EncRet
    Argument      : inst - the instance in use
                    pCodeParams - user provided parameters
------------------------------------------------------------------------------*/
H264EncRet H264EncSetCodingCtrl(H264EncInst inst,
                                const H264EncCodingCtrl * pCodeParams)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    APITRACE("H264EncSetCodingCtrl#");

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pCodeParams == NULL)) {
        APITRACE("H264EncSetCodingCtrl: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncSetCodingCtrl: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    /* Check for invalid values */
    if (pCodeParams->sliceSize > pEncInst->mbPerCol) {
        APITRACE("H264EncSetCodingCtrl: ERROR Invalid sliceSize");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Check status, only slice size allowed to be altered after start */
    if (pEncInst->encStatus != H264ENCSTAT_INIT) {
        goto set_slice_size;
    }

    if (pCodeParams->constrainedIntraPrediction > 1 ||
        pCodeParams->disableDeblockingFilter > 2 ||
        pCodeParams->enableCabac > 1 ||
        pCodeParams->transform8x8Mode > 2 ||
        pCodeParams->seiMessages > 1 || pCodeParams->videoFullRange > 1) {
        APITRACE("H264EncSetCodingCtrl: ERROR Invalid enable/disable");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pCodeParams->sampleAspectRatioWidth > 65535 ||
        pCodeParams->sampleAspectRatioHeight > 65535) {
        APITRACE("H264EncSetCodingCtrl: ERROR Invalid sampleAspectRatio");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pCodeParams->cabacInitIdc > 2) {
        APITRACE("H264EncSetCodingCtrl: ERROR Invalid cabacInitIdc");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Configure the values */
    if (pCodeParams->constrainedIntraPrediction == 0)
        pEncInst->picParameterSet.constIntraPred = ENCHW_NO;
    else
        pEncInst->picParameterSet.constIntraPred = ENCHW_YES;

    /* filter control header always present */
    pEncInst->picParameterSet.deblockingFilterControlPresent = ENCHW_YES;
    pEncInst->slice.disableDeblocking = pCodeParams->disableDeblockingFilter;

    if (pCodeParams->enableCabac == 0) {
        pEncInst->picParameterSet.entropyCodingMode = ENCHW_NO;
    } else {
        pEncInst->picParameterSet.entropyCodingMode = ENCHW_YES;
        pEncInst->slice.cabacInitIdc = pCodeParams->cabacInitIdc;
    }

    /* 8x8 mode: 2=enable, 1=adaptive (720p or bigger frame) */
    if ((pCodeParams->transform8x8Mode == 2) ||
        ((pCodeParams->transform8x8Mode == 1) &&
         (pEncInst->mbPerFrame >= 3600))) {
        pEncInst->picParameterSet.transform8x8Mode = ENCHW_YES;
    }

    H264SpsSetVuiAspectRatio(&pEncInst->seqParameterSet,
                             pCodeParams->sampleAspectRatioWidth,
                             pCodeParams->sampleAspectRatioHeight);

    H264SpsSetVuiVideoInfo(&pEncInst->seqParameterSet,
                           pCodeParams->videoFullRange);

    /* SEI messages are written in the beginning of each frame */
    if (pCodeParams->seiMessages) {
        pEncInst->rateControl.sei.enabled = ENCHW_YES;
    } else {
        pEncInst->rateControl.sei.enabled = ENCHW_NO;
    }

set_slice_size:
    /* Slice size is set in macroblock rows => convert to macroblocks */
    pEncInst->slice.sliceSize = pCodeParams->sliceSize * pEncInst->mbPerRow;

    APITRACE("H264EncSetCodingCtrl: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncGetCodingCtrl
    Description   : Returns current encoding parameters

    Return type   : H264EncRet
    Argument      : inst - the instance in use
                    pCodeParams - palce where parameters are returned
------------------------------------------------------------------------------*/
H264EncRet H264EncGetCodingCtrl(H264EncInst inst,
                                H264EncCodingCtrl * pCodeParams)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    APITRACE("H264EncGetCodingCtrl#");

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pCodeParams == NULL)) {
        APITRACE("H264EncGetCodingCtrl: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncGetCodingCtrl: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    /* Get values */
    pCodeParams->constrainedIntraPrediction =
        (pEncInst->picParameterSet.constIntraPred == ENCHW_NO) ? 0 : 1;

    /* Slice size from macroblocks to macroblock rows */
    pCodeParams->sliceSize = pEncInst->slice.sliceSize / pEncInst->mbPerRow;

    pCodeParams->seiMessages =
        (pEncInst->rateControl.sei.enabled == ENCHW_YES) ? 1 : 0;
    pCodeParams->videoFullRange =
        (pEncInst->seqParameterSet.vui.videoFullRange == ENCHW_YES) ? 1 : 0;
    pCodeParams->sampleAspectRatioWidth =
        pEncInst->seqParameterSet.vui.sarWidth;
    pCodeParams->sampleAspectRatioHeight =
        pEncInst->seqParameterSet.vui.sarHeight;
    pCodeParams->disableDeblockingFilter = pEncInst->slice.disableDeblocking;
    pCodeParams->enableCabac = pEncInst->picParameterSet.entropyCodingMode;
    pCodeParams->cabacInitIdc = pEncInst->slice.cabacInitIdc;
    pCodeParams->transform8x8Mode = pEncInst->picParameterSet.transform8x8Mode;

    APITRACE("H264EncGetCodingCtrl: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncSetRateCtrl
    Description   : Sets rate control parameters

    Return type   : H264EncRet
    Argument      : inst - the instance in use
                    pRateCtrl - user provided parameters
------------------------------------------------------------------------------*/
H264EncRet H264EncSetRateCtrl(H264EncInst inst,
                              const H264EncRateCtrl * pRateCtrl)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;
    h264RateControl_s *rc;

    u32 i, tmp;

    APITRACE("H264EncSetRateCtrl#");

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pRateCtrl == NULL)) {
        APITRACE("H264EncSetRateCtrl: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncSetRateCtrl: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    rc = &pEncInst->rateControl;

    /* after stream was started with HRD ON,
     * it is not allowed to change RC params */
    if (pEncInst->encStatus == H264ENCSTAT_START_FRAME && rc->hrd == ENCHW_YES) {
        APITRACE
        ("H264EncSetRateCtrl: ERROR Stream started with HRD ON. Not allowed to change any aprameters");
        return H264ENC_INVALID_STATUS;
    }

    /* Check for invalid input values */
    if (pRateCtrl->pictureRc > 1 ||
        pRateCtrl->mbRc > 1 || pRateCtrl->pictureSkip > 1 || pRateCtrl->hrd > 1) {
        APITRACE("H264EncSetRateCtrl: ERROR Invalid enable/disable value");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pRateCtrl->qpHdr > 51 ||
        pRateCtrl->qpMin > 51 ||
        pRateCtrl->qpMax > 51 ||
        pRateCtrl->qpMax < pRateCtrl->qpMin) {
        APITRACE("H264EncSetRateCtrl: ERROR Invalid QP");
        return H264ENC_INVALID_ARGUMENT;
    }

    if ((pRateCtrl->qpHdr != -1) &&
        (pRateCtrl->qpHdr < (i32)pRateCtrl->qpMin ||
         pRateCtrl->qpHdr > (i32)pRateCtrl->qpMax)) {
        APITRACE("H264EncSetRateCtrl: ERROR QP out of range");
        return H264ENC_INVALID_ARGUMENT;
    }
    if ((u32)(pRateCtrl->intraQpDelta + 12) > 24) {
        APITRACE("H264EncSetRateCtrl: ERROR intraQpDelta out of range");
        return H264ENC_INVALID_ARGUMENT;
    }
    if (pRateCtrl->fixedIntraQp > 51) {
        APITRACE("H264EncSetRateCtrl: ERROR fixedIntraQp out of range");
        return H264ENC_INVALID_ARGUMENT;
    }
    if (pRateCtrl->gopLen < 1 || pRateCtrl->gopLen > 150) {
        APITRACE("H264EncSetRateCtrl: ERROR Invalid GOP length");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Bitrate affects only when rate control is enabled */
    if ((pRateCtrl->pictureRc || pRateCtrl->mbRc ||
         pRateCtrl->pictureSkip || pRateCtrl->hrd) &&
        (pRateCtrl->bitPerSecond < 10000 ||
         pRateCtrl->bitPerSecond > H264ENC_MAX_BITRATE)) {
        APITRACE("H264EncSetRateCtrl: ERROR Invalid bitPerSecond");
        return H264ENC_INVALID_ARGUMENT;
    }

    {
        u32 cpbSize = pRateCtrl->hrdCpbSize;
        u32 bps = pRateCtrl->bitPerSecond;
        u32 level = pEncInst->seqParameterSet.levelIdx;

        /* Saturates really high settings */
        /* bits per unpacked frame */
        tmp = 3 * 8 * pEncInst->mbPerFrame * 256 / 2;
        /* bits per second */
        tmp = H264Calculate(tmp, rc->outRateNum, rc->outRateDenom);
        if (bps > (tmp / 2))
            bps = tmp / 2;

        if (cpbSize == 0) {
            cpbSize = H264MaxCPBS[level];
        } else if (cpbSize == (u32) (-1)) {
            cpbSize = bps;
        }


        /* Limit minimum CPB size based on average bits per frame */
        tmp = H264Calculate(bps, rc->outRateDenom, rc->outRateNum);
        cpbSize = MAX(cpbSize, tmp);

        /* cpbSize must be rounded so it is exactly the size written in stream */
        i = 0;
        tmp = cpbSize;
        while (4095 < (tmp >> (4 + i++)));

        cpbSize = (tmp >> (4 + i)) << (4 + i);

        /* if HRD is ON we have to obay all its limits */
        if (pRateCtrl->hrd != 0) {
            if (cpbSize > H264MaxCPBS[level]) {
                APITRACE
                ("H264EncSetRateCtrl: ERROR. HRD is ON. hrdCpbSize higher than maximum allowed for stream level");
                return H264ENC_INVALID_ARGUMENT;
            }

            if (bps > H264MaxBR[level]) {
                APITRACE
                ("H264EncSetRateCtrl: ERROR. HRD is ON. bitPerSecond higher than maximum allowed for stream level");
                return H264ENC_INVALID_ARGUMENT;
            }
        }

        rc->virtualBuffer.bufferSize = cpbSize;

        /* Set the parameters to rate control */
        if (pRateCtrl->pictureRc != 0)
            rc->picRc = ENCHW_YES;
        else
            rc->picRc = ENCHW_NO;

        /* ROI and MB RC can't be used at the same time */
        if ((pRateCtrl->mbRc != 0) && (rc->roiRc == ENCHW_NO)) {
            rc->mbRc = ENCHW_YES;
        } else
            rc->mbRc = ENCHW_NO;

        if (pRateCtrl->pictureSkip != 0)
            rc->picSkip = ENCHW_YES;
        else
            rc->picSkip = ENCHW_NO;

        if (pRateCtrl->hrd != 0) {
            rc->hrd = ENCHW_YES;
            rc->picRc = ENCHW_YES;
        } else
            rc->hrd = ENCHW_NO;

        rc->qpHdr = pRateCtrl->qpHdr;
        rc->qpMin = pRateCtrl->qpMin;
        rc->qpMax = pRateCtrl->qpMax;
        rc->virtualBuffer.bitRate = bps;
        rc->gopLen = pRateCtrl->gopLen;
    }
    rc->intraQpDelta = pRateCtrl->intraQpDelta;
    rc->fixedIntraQp = pRateCtrl->fixedIntraQp;
    rc->mbQpAdjustment = pRateCtrl->mbQpAdjustment;
    (void) H264InitRc(rc);  /* new parameters checked already */

    APITRACE("H264EncSetRateCtrl: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncGetRateCtrl
    Description   : Return current rate control parameters

    Return type   : H264EncRet
    Argument      : inst - the instance in use
                    pRateCtrl - place where parameters are returned
------------------------------------------------------------------------------*/
H264EncRet H264EncGetRateCtrl(H264EncInst inst, H264EncRateCtrl * pRateCtrl)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;
    h264RateControl_s *rc;

    APITRACE("H264EncGetRateCtrl#");

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pRateCtrl == NULL)) {
        APITRACE("H264EncGetRateCtrl: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncGetRateCtrl: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    /* Get the values */
    rc = &pEncInst->rateControl;

    pRateCtrl->pictureRc = rc->picRc == ENCHW_NO ? 0 : 1;
    pRateCtrl->mbRc = rc->mbRc == ENCHW_NO ? 0 : 1;
    pRateCtrl->pictureSkip = rc->picSkip == ENCHW_NO ? 0 : 1;
    pRateCtrl->qpHdr = rc->qpHdr;
    pRateCtrl->qpMin = rc->qpMin;
    pRateCtrl->qpMax = rc->qpMax;
    pRateCtrl->bitPerSecond = rc->virtualBuffer.bitRate;
    pRateCtrl->hrd = rc->hrd == ENCHW_NO ? 0 : 1;
    pRateCtrl->gopLen = rc->gopLen;

    pRateCtrl->hrdCpbSize = (u32) rc->virtualBuffer.bufferSize;
    pRateCtrl->intraQpDelta = rc->intraQpDelta;
    pRateCtrl->fixedIntraQp = rc->fixedIntraQp;

    APITRACE("H264EncGetRateCtrl: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VSCheckSize
    Description     :
    Return type     : i32
    Argument        : u32 inputWidth
    Argument        : u32 inputHeight
    Argument        : u32 stabilizedWidth
    Argument        : u32 stabilizedHeight
------------------------------------------------------------------------------*/
i32 VSCheckSize(u32 inputWidth, u32 inputHeight, u32 stabilizedWidth,
                u32 stabilizedHeight)
{
    /* Input picture minimum dimensions */
    if ((inputWidth < 104) || (inputHeight < 104))
        return 1;

    /* Stabilized picture minimum  values */
    if ((stabilizedWidth < 96) || (stabilizedHeight < 96))
        return 1;

    /* Stabilized dimensions multiple of 4 */
    if (((stabilizedWidth & 3) != 0) || ((stabilizedHeight & 3) != 0))
        return 1;

    /* Edge >= 4 pixels, not checked because stabilization can be
     * used without cropping for scene detection
    if((inputWidth < (stabilizedWidth + 8)) ||
       (inputHeight < (stabilizedHeight + 8)))
        return 1; */

    return 0;
}

/*------------------------------------------------------------------------------
    Function name   : H264EncSetPreProcessing
    Description     : Sets the preprocessing parameters
    Return type     : H264EncRet
    Argument        : inst - encoder instance in use
    Argument        : pPreProcCfg - user provided parameters
------------------------------------------------------------------------------*/
H264EncRet H264EncSetPreProcessing(H264EncInst inst,
                                   const H264EncPreProcessingCfg * pPreProcCfg)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;
    preProcess_s pp_tmp;

    APITRACE("H264EncSetPreProcessing#");

    /* Check for illegal inputs */
    if ((inst == NULL) || (pPreProcCfg == NULL)) {
        APITRACE("H264EncSetPreProcessing: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncSetPreProcessing: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

#if 0
    /* check HW limitations */
    {
        VPUHwEncConfig_t cfg;

        while (VPUClientGetHwCfg(pEncInst->asic.regs.socket, (RK_U32*)&cfg, sizeof(cfg))) {
            usleep(1);
        }

        /* is video stabilization supported? */
        if (cfg.vsEnabled == EWL_HW_CONFIG_NOT_SUPPORTED &&
            pPreProcCfg->videoStabilization != 0) {
            APITRACE("H264EncSetPreProcessing: ERROR Stabilization not supported");
            return H264ENC_INVALID_ARGUMENT;
        }
        if (cfg.rgbEnabled == EWL_HW_CONFIG_NOT_SUPPORTED &&
            pPreProcCfg->inputType > 3) {
            APITRACE("H264EncSetPreProcessing: ERROR RGB input not supported");
            return H264ENC_INVALID_ARGUMENT;
        }
    }
#endif

    if (pPreProcCfg->origWidth > H264ENC_MAX_PP_INPUT_WIDTH ||
        pPreProcCfg->origHeight > H264ENC_MAX_PP_INPUT_HEIGHT) {
        APITRACE("H264EncSetPreProcessing: ERROR Too big input image");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->inputType > H264ENC_BGR101010) {
        APITRACE("H264EncSetPreProcessing: ERROR Invalid YUV type");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->rotation > H264ENC_ROTATE_90L) {
        APITRACE("H264EncSetPreProcessing: ERROR Invalid rotation");
        return H264ENC_INVALID_ARGUMENT;
    }

    pp_tmp.lumHeightSrc = pPreProcCfg->origHeight;
    pp_tmp.lumWidthSrc = pPreProcCfg->origWidth;

    if (pPreProcCfg->videoStabilization == 0) {
        pp_tmp.horOffsetSrc = pPreProcCfg->xOffset;
        pp_tmp.verOffsetSrc = pPreProcCfg->yOffset;
    } else {
        pp_tmp.horOffsetSrc = pp_tmp.verOffsetSrc = 0;
    }

    pp_tmp.lumWidth = pEncInst->preProcess.lumWidth;
    pp_tmp.lumHeight = pEncInst->preProcess.lumHeight;

    pp_tmp.rotation = pPreProcCfg->rotation;
    pp_tmp.inputFormat = pPreProcCfg->inputType;

    pp_tmp.videoStab = (pPreProcCfg->videoStabilization != 0) ? 1 : 0;

    /* Check for invalid values */
    if (EncPreProcessCheck(&pp_tmp) != ENCHW_OK) {
        APITRACE("H264EncSetPreProcessing: ERROR Invalid cropping values");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Set cropping parameters if required */
    if ( pEncInst->preProcess.lumWidth % 16 || pEncInst->preProcess.lumHeight % 16 ) {
        u32 fillRight = (pEncInst->preProcess.lumWidth + 15) / 16 * 16 -
                        pEncInst->preProcess.lumWidth;
        u32 fillBottom = (pEncInst->preProcess.lumHeight + 15) / 16 * 16 -
                         pEncInst->preProcess.lumHeight;

        pEncInst->seqParameterSet.frameCropping = ENCHW_YES;
        pEncInst->seqParameterSet.frameCropLeftOffset = 0;
        pEncInst->seqParameterSet.frameCropRightOffset = 0;
        pEncInst->seqParameterSet.frameCropTopOffset = 0;
        pEncInst->seqParameterSet.frameCropBottomOffset = 0;

        if (pPreProcCfg->rotation == 0) {   /* No rotation */
            pEncInst->seqParameterSet.frameCropRightOffset = fillRight / 2;
            pEncInst->seqParameterSet.frameCropBottomOffset = fillBottom / 2;
        } else if (pPreProcCfg->rotation == 1) {    /* Rotate right */
            pEncInst->seqParameterSet.frameCropLeftOffset = fillRight / 2;
            pEncInst->seqParameterSet.frameCropBottomOffset = fillBottom / 2;
        } else {    /* Rotate left */
            pEncInst->seqParameterSet.frameCropRightOffset = fillRight / 2;
            pEncInst->seqParameterSet.frameCropTopOffset = fillBottom / 2;
        }
    }

    if (pp_tmp.videoStab != 0) {
        u32 width = pp_tmp.lumWidth;
        u32 height = pp_tmp.lumHeight;

        if (pp_tmp.rotation) {
            u32 tmp;

            tmp = width;
            width = height;
            height = tmp;
        }

        if (VSCheckSize(pp_tmp.lumWidthSrc, pp_tmp.lumHeightSrc, width, height)
            != 0) {
            APITRACE
            ("H264EncSetPreProcessing: ERROR Invalid size for stabilization");
            return H264ENC_INVALID_ARGUMENT;
        }

#ifdef VIDEOSTAB_ENABLED
        VSAlgInit(&pEncInst->vsSwData, pp_tmp.lumWidthSrc, pp_tmp.lumHeightSrc,
                  width, height);

        VSAlgGetResult(&pEncInst->vsSwData, &pp_tmp.horOffsetSrc,
                       &pp_tmp.verOffsetSrc);
#endif
    }

    pp_tmp.colorConversionType = pPreProcCfg->colorConversion.type;
    pp_tmp.colorConversionCoeffA = pPreProcCfg->colorConversion.coeffA;
    pp_tmp.colorConversionCoeffB = pPreProcCfg->colorConversion.coeffB;
    pp_tmp.colorConversionCoeffC = pPreProcCfg->colorConversion.coeffC;
    pp_tmp.colorConversionCoeffE = pPreProcCfg->colorConversion.coeffE;
    pp_tmp.colorConversionCoeffF = pPreProcCfg->colorConversion.coeffF;
    EncSetColorConversion(&pp_tmp, &pEncInst->asic);

    {
        preProcess_s *pp = &pEncInst->preProcess;

        if (memcpy(pp, &pp_tmp, sizeof(preProcess_s)) != pp) {
            APITRACE("H264EncSetPreProcessing: memcpy failed");
            return H264ENC_SYSTEM_ERROR;
        }
    }

    APITRACE("H264EncSetPreProcessing: OK");

    return H264ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : H264EncGetPreProcessing
    Description     : Returns current preprocessing parameters
    Return type     : H264EncRet
    Argument        : inst - encoder instance
    Argument        : pPreProcCfg - place where the parameters are returned
------------------------------------------------------------------------------*/
H264EncRet H264EncGetPreProcessing(H264EncInst inst,
                                   H264EncPreProcessingCfg * pPreProcCfg)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;
    preProcess_s *pPP;

    APITRACE("H264EncGetPreProcessing#");

    /* Check for illegal inputs */
    if ((inst == NULL) || (pPreProcCfg == NULL)) {
        APITRACE("H264EncGetPreProcessing: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncGetPreProcessing: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    pPP = &pEncInst->preProcess;

    pPreProcCfg->origHeight = pPP->lumHeightSrc;
    pPreProcCfg->origWidth = pPP->lumWidthSrc;
    pPreProcCfg->xOffset = pPP->horOffsetSrc;
    pPreProcCfg->yOffset = pPP->verOffsetSrc;

    pPreProcCfg->rotation = (H264EncPictureRotation) pPP->rotation;
    pPreProcCfg->inputType = (H264EncPictureFormat) pPP->inputFormat;

    pPreProcCfg->videoStabilization = pPP->videoStab;

    pPreProcCfg->colorConversion.type =
        (H264EncColorConversionType) pPP->colorConversionType;
    pPreProcCfg->colorConversion.coeffA = pPP->colorConversionCoeffA;
    pPreProcCfg->colorConversion.coeffB = pPP->colorConversionCoeffB;
    pPreProcCfg->colorConversion.coeffC = pPP->colorConversionCoeffC;
    pPreProcCfg->colorConversion.coeffE = pPP->colorConversionCoeffE;
    pPreProcCfg->colorConversion.coeffF = pPP->colorConversionCoeffF;
    mpp_log(" %d  %d  %d  %d ... %d  %d  %d  %d  ... %d %d %d ... %d %d  modify by lance 2016.04.26",
            pPreProcCfg->origHeight, pPreProcCfg->origWidth, pPreProcCfg->xOffset, pPreProcCfg->yOffset,
            pPreProcCfg->rotation, pPreProcCfg->inputType, pPreProcCfg->videoStabilization, pPreProcCfg->colorConversion.type,
            pPreProcCfg->colorConversion.coeffA, pPreProcCfg->colorConversion.coeffB, pPreProcCfg->colorConversion.coeffC,
            pPreProcCfg->colorConversion.coeffE, pPreProcCfg->colorConversion.coeffF);

    APITRACE("H264EncGetPreProcessing: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name : H264EncSetSeiUserData
    Description   : Sets user data SEI messages
    Return type   : H264EncRet
    Argument      : inst - the instance in use
                    pUserData - pointer to userData, this is used by the
                                encoder so it must not be released before
                                disabling user data
                    userDataSize - size of userData, minimum size 16,
                                   maximum size H264ENC_MAX_USER_DATA_SIZE
                                   not valid size disables userData sei messages
------------------------------------------------------------------------------*/
H264EncRet H264EncSetSeiUserData(H264EncInst inst, const u8 * pUserData,
                                 u32 userDataSize)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (userDataSize != 0 && pUserData == NULL)) {
        APITRACE("H264EncSetSeiUserData: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncSetSeiUserData: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    /* Disable user data */
    if ((userDataSize < 16) || (userDataSize > H264ENC_MAX_USER_DATA_SIZE)) {
        pEncInst->rateControl.sei.userDataEnabled = ENCHW_NO;
        pEncInst->rateControl.sei.pUserData = NULL;
        pEncInst->rateControl.sei.userDataSize = 0;
    } else {
        pEncInst->rateControl.sei.userDataEnabled = ENCHW_YES;
        pEncInst->rateControl.sei.pUserData = pUserData;
        pEncInst->rateControl.sei.userDataSize = userDataSize;
    }

    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncStrmStart
    Description   : Starts a new stream
    Return type   : H264EncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
H264EncRet H264EncStrmStart(H264EncInst inst, const H264EncIn * pEncIn,
                            H264EncOut * pEncOut)
{
    h264Instance_s *pEncInst = (h264Instance_s *)inst;
    h264RateControl_s *rc;

    APITRACE("H264EncStrmStart#");

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACE("H264EncStrmStart: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncStrmStart: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    rc = &pEncInst->rateControl;

    /* Check status */
    if (pEncInst->encStatus != H264ENCSTAT_INIT) {
        APITRACE("H264EncStrmStart: ERROR Invalid status");
        return H264ENC_INVALID_STATUS;
    }

    /* Check for invalid input values */
    if ((pEncIn->pOutBuf == NULL) ||
        (pEncIn->outBufSize < H264ENCSTRMSTART_MIN_BUF)) {
        APITRACE("H264EncStrmStart: ERROR Invalid input. Stream buffer");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pEncIn->pNaluSizeBuf != NULL &&
        pEncIn->naluSizeBufSize < (3 * sizeof(u32))) {
        APITRACE("H264EncStrmStart: ERROR Invalid input. NAL size buffer");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Set stream buffer, the size has been checked */
    (void) H264SetBuffer(&pEncInst->stream, (u8 *) pEncIn->pOutBuf,
                         (u32) pEncIn->outBufSize);

#ifdef INTERNAL_TEST
    /* Configure the encoder instance according to the test vector */
    H264ConfigureTestBeforeStream(pEncInst);
#endif

#ifdef TRACE_STREAM
    /* Open stream tracing */
    EncOpenStreamTrace("stream.trc");

    traceStream.frameNum = pEncInst->frameCnt;
    traceStream.id = 0; /* Stream generated by SW */
    traceStream.bitCnt = 0;  /* New frame */
#endif

    /* Set the profile to be used */
    if (pEncInst->seqParameterSet.profileIdc != 66 && pEncInst->seqParameterSet.profileIdc != 77
        && pEncInst->seqParameterSet.profileIdc != 100)
        pEncInst->seqParameterSet.profileIdc = 66; /* base profile */

    /* CABAC => main profile */
    if (pEncInst->picParameterSet.entropyCodingMode == ENCHW_YES)
        pEncInst->seqParameterSet.profileIdc = 77;

    /* 8x8 transform enabled => high profile */
    if (pEncInst->picParameterSet.transform8x8Mode == ENCHW_YES)
        pEncInst->seqParameterSet.profileIdc = 100;

    /* update VUI */
    if (rc->sei.enabled == ENCHW_YES) {
        H264SpsSetVuiPictStructPresentFlag(&pEncInst->seqParameterSet, 1);
    }

    if (rc->hrd == ENCHW_YES) {
        H264SpsSetVuiHrd(&pEncInst->seqParameterSet, 1);

        H264SpsSetVuiHrdBitRate(&pEncInst->seqParameterSet,
                                rc->virtualBuffer.bitRate);

        H264SpsSetVuiHrdCpbSize(&pEncInst->seqParameterSet,
                                rc->virtualBuffer.bufferSize);
    }

    /* Use the first frame QP in the PPS */
    pEncInst->picParameterSet.picInitQpMinus26 = (i32) (rc->qpHdr) - 26;

    /* Init SEI */
    H264InitSei(&rc->sei, pEncInst->seqParameterSet.byteStream,
                rc->hrd, rc->outRateNum, rc->outRateDenom);

    H264SeqParameterSet(&pEncInst->stream, &pEncInst->seqParameterSet);
    if (pEncIn->pNaluSizeBuf != NULL) {
        pEncIn->pNaluSizeBuf[0] = pEncInst->stream.byteCnt;
    }

    H264PicParameterSet(&pEncInst->stream, &pEncInst->picParameterSet);
    if (pEncIn->pNaluSizeBuf != NULL) {
        pEncIn->pNaluSizeBuf[1] =
            pEncInst->stream.byteCnt - pEncIn->pNaluSizeBuf[0];
        pEncIn->pNaluSizeBuf[2] = 0;
    }

    if (pEncInst->stream.overflow == ENCHW_YES) {
        pEncOut->streamSize = 0;
        APITRACE("H264EncStrmStart: ERROR Output buffer too small");
        return H264ENC_OUTPUT_BUFFER_OVERFLOW;
    }

    /* Bytes generated */
    pEncOut->streamSize = pEncInst->stream.byteCnt;

    /* Status == START_STREAM   Stream started */
    pEncInst->encStatus = H264ENCSTAT_START_STREAM;

    pEncInst->slice.frameNum = 0;
    pEncInst->rateControl.fillerIdx = (u32) (-1);

    if (rc->hrd == ENCHW_YES) {
        /* Update HRD Parameters to RC if needed */
        u32 bitrate = H264SpsGetVuiHrdBitRate(&pEncInst->seqParameterSet);
        u32 cpbsize = H264SpsGetVuiHrdCpbSize(&pEncInst->seqParameterSet);

        if ((rc->virtualBuffer.bitRate != (i32)bitrate) ||
            (rc->virtualBuffer.bufferSize != (i32)cpbsize)) {
            rc->virtualBuffer.bitRate = bitrate;
            rc->virtualBuffer.bufferSize = cpbsize;
            (void) H264InitRc(rc);
        }
    }

#ifdef VIDEOSTAB_ENABLED
    /* new stream so reset the stabilization */
    VSAlgReset(&pEncInst->vsSwData);
#endif

    APITRACE("H264EncStrmStart: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264EncStrmEncode
    Description   : Encodes a new picture
    Return type   : H264EncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
H264EncRet H264EncStrmEncode(H264EncInst inst, const H264EncIn * pEncIn,
                             H264EncOut * pEncOut, h264e_syntax *syntax_data)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;
    slice_s *pSlice;
    regValues_s *regs;
    /*h264EncodeFrame_e ret;*/  // mask by lance 2016.05.12

    APITRACE("H264EncStrmEncode#");

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACE("H264EncStrmEncode: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncStrmEncode: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    {
        u32 nals;

        if (pEncInst->slice.sliceSize != 0) {
            /* how many  picture slices we have */
            nals = (pEncInst->mbPerFrame + pEncInst->slice.sliceSize - 1) /
                   pEncInst->slice.sliceSize;
        } else {
            nals = 1;   /* whole pic in one slice */
        }

        nals += 3;  /* SEI, Filler and "0-end" */

        if (pEncIn->pNaluSizeBuf != NULL &&
            pEncIn->naluSizeBufSize < (nals * sizeof(u32))) {
            APITRACE("H264EncStrmEncode: ERROR Invalid input. NAL size buffer");
            return H264ENC_INVALID_ARGUMENT;
        }
    }

    /* Clear the output structure */
    pEncOut->codingType = H264ENC_NOTCODED_FRAME;
    pEncOut->streamSize = 0;

    /* Clear the NAL unit size table */
    if (pEncIn->pNaluSizeBuf != NULL)
        pEncIn->pNaluSizeBuf[0] = 0;

#ifdef EVALUATION_LIMIT
    /* Check for evaluation limit */
    if (pEncInst->frameCnt >= EVALUATION_LIMIT) {
        APITRACE("H264EncStrmEncode: OK Evaluation limit exceeded");
        return H264ENC_OK;
    }
#endif

    /* Check status, INIT and ERROR not allowed */
    if ((pEncInst->encStatus != H264ENCSTAT_START_STREAM) &&
        (pEncInst->encStatus != H264ENCSTAT_START_FRAME)) {
        APITRACE("H264EncStrmEncode: ERROR Invalid status");
        return H264ENC_INVALID_STATUS;
    }

    /* Check for invalid input values */
    if ((!H264_BUS_ADDRESS_VALID(pEncIn->busOutBuf)) ||
        (pEncIn->pOutBuf == NULL) ||
        (pEncIn->outBufSize < H264ENCSTRMENCODE_MIN_BUF) ||
        (pEncIn->codingType > H264ENC_PREDICTED_FRAME)) {
        APITRACE("H264EncStrmEncode: ERROR Invalid input. Output buffer");
        return H264ENC_INVALID_ARGUMENT;
    }

    switch (pEncInst->preProcess.inputFormat) {
    case H264ENC_YUV420_PLANAR:
        if (!H264_BUS_ADDRESS_VALID(pEncIn->busChromaV)) {
            APITRACE("H264EncStrmEncode: ERROR Invalid input busChromaU");
            return H264ENC_INVALID_ARGUMENT;
        }
        /* fall through */
    case H264ENC_YUV420_SEMIPLANAR:
        if (!H264_BUS_ADDRESS_VALID(pEncIn->busChromaU)) {
            APITRACE("H264EncStrmEncode: ERROR Invalid input busChromaU");
            return H264ENC_INVALID_ARGUMENT;
        }
        /* fall through */
    case H264ENC_YUV422_INTERLEAVED_YUYV:
    case H264ENC_YUV422_INTERLEAVED_UYVY:
    case H264ENC_RGB565:
    case H264ENC_BGR565:
    case H264ENC_RGB555:
    case H264ENC_BGR555:
    case H264ENC_RGB444:
    case H264ENC_BGR444:
    case H264ENC_RGB888:
    case H264ENC_BGR888:
    case H264ENC_RGB101010:
    case H264ENC_BGR101010:
        if (!H264_BUS_ADDRESS_VALID(pEncIn->busLuma)) {
            APITRACE("H264EncStrmEncode: ERROR Invalid input busLuma");
            return H264ENC_INVALID_ARGUMENT;
        }
        break;
    default:
        APITRACE("H264EncStrmEncode: ERROR Invalid input format");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pEncInst->preProcess.videoStab) {
        if (!H264_BUS_ADDRESS_VALID(pEncIn->busLumaStab)) {
            APITRACE("H264EncStrmEncode: ERROR Invalid input busLumaStab");
            return H264ENC_INVALID_ARGUMENT;
        }
    }

    /* some shortcuts */
    pSlice = &pEncInst->slice;
    regs = &pEncInst->asic.regs;

    /* Set stream buffer, the size has been checked */
    if (H264SetBuffer(&pEncInst->stream, (u8 *) pEncIn->pOutBuf,
                      (i32) pEncIn->outBufSize) == ENCHW_NOK) {
        APITRACE("H264EncStrmEncode: ERROR Invalid output buffer");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Try to reserve the HW resource */
    /*if(EWLReserveHw(pEncInst->asic.ewl) == EWL_ERROR)
    {
        APITRACE("H264EncStrmEncode: ERROR HW unavailable");
        return H264ENC_HW_RESERVED;
    }*/
    /* update in/out buffers */
    regs->inputLumBase = pEncIn->busLuma;
    regs->inputCbBase = pEncIn->busChromaU;
    regs->inputCrBase = pEncIn->busChromaV;

    regs->outputStrmBase = pEncIn->busOutBuf;
    regs->outputStrmSize = pEncIn->outBufSize;

    /* setup stabilization */
    if (pEncInst->preProcess.videoStab) {
        regs->vsNextLumaBase = pEncIn->busLumaStab;
    }

    {
        H264EncPictureCodingType ct = pEncIn->codingType;

        /* Status may affect the frame coding type */
        if (pEncInst->encStatus == H264ENCSTAT_START_STREAM) {
            ct = H264ENC_INTRA_FRAME;
        }
#ifdef VIDEOSTAB_ENABLED
        if (pEncInst->vsSwData.sceneChange) {
            pEncInst->vsSwData.sceneChange = 0;
            ct = H264ENC_INTRA_FRAME;
        }
#endif
        pSlice->prevFrameNum = pSlice->frameNum;

        /* Frame coding type defines the NAL unit type */
        switch (ct) {
        case H264ENC_INTRA_FRAME:
            /* IDR-slice */
            pSlice->nalUnitType = IDR;
            pSlice->sliceType = ISLICE;
            pSlice->frameNum = 0;
            H264MadInit(&pEncInst->mad, pEncInst->mbPerFrame);
            break;
        case H264ENC_PREDICTED_FRAME:
        default:
            /* non-IDR slice */
            pSlice->nalUnitType = NONIDR;
            pSlice->sliceType = PSLICE;
            break;
        }
    }

    /* Rate control */
    H264BeforePicRc(&pEncInst->rateControl, pEncIn->timeIncrement,
                    pSlice->sliceType);


    /* time stamp updated */
    H264UpdateSeiTS(&pEncInst->rateControl.sei, pEncIn->timeIncrement);

    /* Rate control may choose to skip the frame */
    if (pEncInst->rateControl.frameCoded == ENCHW_NO) {
        APITRACE("H264EncStrmEncode: OK, frame skipped");
        pSlice->frameNum = pSlice->prevFrameNum;    /* restore frame_num */

#if 0
        /* Write previous reconstructed frame when frame is skipped */
        EncAsicRecycleInternalImage(&pEncInst->asic.regs);
        EncDumpRecon(&pEncInst->asic);
        EncAsicRecycleInternalImage(&pEncInst->asic.regs);
#endif

//        EWLReleaseHw(pEncInst->asic.ewl);

        return H264ENC_FRAME_READY;
    }

#ifdef TRACE_STREAM
    traceStream.frameNum = pEncInst->frameCnt;
    traceStream.id = 0; /* Stream generated by SW */
    traceStream.bitCnt = 0;  /* New frame */
#endif

#ifdef INTERNAL_TEST
    /* Configure the encoder instance according to the test vector */
    H264ConfigureTestBeforeFrame(pEncInst);
#endif

    /* update any cropping/rotation/filling */
    EncPreProcess(&pEncInst->asic, &pEncInst->preProcess);

    /* SEI message */
    {
        sei_s *sei = &pEncInst->rateControl.sei;

        if (sei->enabled == ENCHW_YES || sei->userDataEnabled == ENCHW_YES) {

            H264NalUnitHdr(&pEncInst->stream, 0, SEI, sei->byteStream);

            if (sei->enabled == ENCHW_YES) {
                if (pSlice->nalUnitType == IDR) {
                    H264BufferingSei(&pEncInst->stream, sei);
                }

                H264PicTimingSei(&pEncInst->stream, sei);
            }

            if (sei->userDataEnabled == ENCHW_YES) {
                H264UserDataUnregSei(&pEncInst->stream, sei);
            }

            H264RbspTrailingBits(&pEncInst->stream);

            sei->nalUnitSize = pEncInst->stream.byteCnt;
        }
    }

    /* Code one frame */
    H264CodeFrame(pEncInst, syntax_data);  // mask by lance 2016.05.12

    // need to think about it also    modify by lance 2016.05.07
    return H264ENC_FRAME_READY;
}

// get the hw status of encoder after encode frame
RK_S32 EncAsicCheckHwStatus(asicData_s *asic)
{
    RK_S32 ret = ASIC_STATUS_FRAME_READY;
    RK_U32 status = asic->regs.hw_status;

    if (status & ASIC_STATUS_ERROR) {
        ret = ASIC_STATUS_ERROR;
    } else if (status & ASIC_STATUS_HW_RESET) {
        ret = ASIC_STATUS_HW_RESET;
    } else if (status & ASIC_STATUS_FRAME_READY) {
        ret = ASIC_STATUS_FRAME_READY;
    } else {
        ret = ASIC_STATUS_BUFF_FULL;
        /* we do not support recovery from buffer full situation so */
        /* ASIC has to be stopped                                   */
        EncAsicStop(asic->ewl);
    }

    return ret;
}

H264EncRet H264EncStrmEncodeAfter(H264EncInst inst, const H264EncIn * pEncIn,
                                  H264EncOut * pEncOut, MPP_RET vpuWaitResult)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;  // add by lance 2016.05.07
    slice_s *pSlice;  // add by lance 2016.05.07
    regValues_s *regs;  // add by lance 2016.05.07
    h264EncodeFrame_e ret = H264ENCODE_OK;  // add by lance 2016.05.07
    asicData_s *asic = &pEncInst->asic;
    // add by lance 2016.05.07
    /* some shortcuts */
    pSlice = &pEncInst->slice;
    regs = &pEncInst->asic.regs;
    if (vpuWaitResult != MPP_OK) {
        if (vpuWaitResult == EWL_ERROR) {
            /* IRQ error => Stop and release HW */
            ret = H264ENCODE_SYSTEM_ERROR;
        } else { /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
            /* IRQ Timeout => Stop and release HW */
            ret = H264ENCODE_TIMEOUT;
        }
        EncAsicStop(asic->ewl);
        /* Release HW so that it can be used by other codecs */
        //EWLReleaseHw(asic->ewl);
    } else {
        i32 status = EncAsicCheckHwStatus(asic);
        switch (status) {
        case ASIC_STATUS_ERROR:
            ret = H264ENCODE_HW_ERROR;
            break;
        case ASIC_STATUS_BUFF_FULL:
            ret = H264ENCODE_OK;
            pEncInst->stream.overflow = ENCHW_YES;
            break;
        case ASIC_STATUS_HW_RESET:
            ret = H264ENCODE_HW_RESET;
            break;
        case ASIC_STATUS_FRAME_READY: {
            /* last not full 64-bit counted in HW data */
            const u32 hw_offset = pEncInst->stream.byteCnt & (0x07U);
            pEncInst->stream.byteCnt +=
                (asic->regs.outputStrmSize - hw_offset);
            pEncInst->stream.stream +=
                (asic->regs.outputStrmSize - hw_offset);
            ret = H264ENCODE_OK;
            break;
        }
        default:
            /* should never get here */
            ASSERT(0);
            ret = H264ENCODE_HW_ERROR;
        }
    }
    {
        // mask by lance 2016.05.12
        /*u32 i, j, k, m;
        u8 *p, *pp, *pptmp;*/

#if 0
        if (pEncInst->asic.regs.regMirror[9] == pEncInst->asic.internalImageLuma[0].phy_addr) {
            p = pEncInst->asic.internalImageLuma[0].vir_addr;//(char *)pEncInst->asic.regs.regMirror[9];
            //VPUMemInvalidate(&pEncInst->asic.internalImageLuma[0]);
            //VPUMemInvalidate(&pEncInst->asic.internalImageChroma[0]);
            VPUMemClean(&pEncInst->asic.internalImageLuma[0]);
            VPUMemClean(&pEncInst->asic.internalImageChroma[0]);
        } else {
            p = pEncInst->asic.internalImageLuma[1].vir_addr;//(char *)pEncInst->asic.regs.regMirror[9];
            VPUMemClean(&pEncInst->asic.internalImageLuma[1]);
            VPUMemClean(&pEncInst->asic.internalImageChroma[1]);
        }
//         fwrite(p,1,800*608,fpp);
#endif
    }

//    while(1);

#ifdef TRACE_RECON
    EncDumpRecon(&pEncInst->asic);
#endif

    if (ret != H264ENCODE_OK) {
        /* Error has occured and the frame is invalid */
        H264EncRet to_user;

        switch (ret) {
        case H264ENCODE_TIMEOUT:
            APITRACE("H264EncStrmEncode: ERROR HW timeout");
            to_user = H264ENC_HW_TIMEOUT;
            break;
        case H264ENCODE_HW_RESET:
            APITRACE("H264EncStrmEncode: ERROR HW reset detected");
            to_user = H264ENC_HW_RESET;
            break;
        case H264ENCODE_HW_ERROR:
            APITRACE("H264EncStrmEncode: ERROR HW bus access error");
            to_user = H264ENC_HW_BUS_ERROR;
            break;
        case H264ENCODE_SYSTEM_ERROR:
        default:
            /* System error has occured, encoding can't continue */
            pEncInst->encStatus = H264ENCSTAT_ERROR;
            APITRACE("H264EncStrmEncode: ERROR Fatal system error");
            to_user = H264ENC_SYSTEM_ERROR;
        }
        return to_user;
    }

#ifdef VIDEOSTAB_ENABLED
    /* Finalize video stabilization */
    if (pEncInst->preProcess.videoStab) {
        u32 no_motion;

        VSReadStabData(pEncInst->asic.regs.regMirror, &pEncInst->vsHwData);

        no_motion = VSAlgStabilize(&pEncInst->vsSwData, &pEncInst->vsHwData);
        if (no_motion) {
            VSAlgReset(&pEncInst->vsSwData);
        }

        /* update offset after stabilization */
        VSAlgGetResult(&pEncInst->vsSwData, &pEncInst->preProcess.horOffsetSrc,
                       &pEncInst->preProcess.verOffsetSrc);
    }
#endif

    /* Filler data if needed */
    if (0) {
        u32 s = H264FillerRc(&pEncInst->rateControl, pEncInst->frameCnt);

        if (s != 0) {
            s = H264FillerNALU(&pEncInst->stream,
                               (i32) s, pEncInst->seqParameterSet.byteStream);
        }
        pEncInst->fillerNalSize = s;
    }

    /* After stream buffer overflow discard the coded frame */
    if (pEncInst->stream.overflow == ENCHW_YES) {
        /* Error has occured and the frame is invalid */
        /* pEncOut->codingType = H264ENC_NOTCODED_FRAME; */
        APITRACE("H264EncStrmEncode: ERROR Output buffer too small");
        return H264ENC_OUTPUT_BUFFER_OVERFLOW;
    }

    /* Rate control action after vop */
    {
        i32 stat;

        stat = H264AfterPicRc(&pEncInst->rateControl, regs->rlcCount,
                              pEncInst->stream.byteCnt, regs->qpSum);
        H264MadThreshold(&pEncInst->mad, regs->madCount);

        /* After HRD overflow discard the coded frame and go back old time,
         * just like not coded frame */
        if (stat == H264RC_OVERFLOW) {
            /* pEncOut->codingType = H264ENC_NOTCODED_FRAME; */
            pSlice->frameNum = pSlice->prevFrameNum;    /* revert frame_num */
            APITRACE("H264EncStrmEncode: OK, Frame discarded (HRD overflow)");
            return H264ENC_FRAME_READY;
        }
    }

    /* Use the reconstructed frame as the reference for the next frame */
    EncAsicRecycleInternalImage(&pEncInst->asic.regs);

    /* Store the stream size and frame coding type in output structure */
    pEncOut->streamSize = pEncInst->stream.byteCnt;

    if (pSlice->nalUnitType == IDR) {
        pEncOut->codingType = H264ENC_INTRA_FRAME;
        pSlice->idrPicId += 1;
        if (pSlice->idrPicId == H264ENC_IDR_ID_MODULO)
            pSlice->idrPicId = 0;
    } else {
        pEncOut->codingType = H264ENC_PREDICTED_FRAME;
    }

    /* Return NAL units' size table if requested */
    if (pEncIn->pNaluSizeBuf != NULL) {
        H264GetNalUnitSizes(pEncInst, pEncIn->pNaluSizeBuf);
    }

    /* Frame was encoded so increment frame number */
    pEncInst->frameCnt++;
    pSlice->frameNum++;
    pSlice->frameNum %= (1U << pSlice->frameNumBits);

    pEncInst->encStatus = H264ENCSTAT_START_FRAME;
    APITRACE("H264EncStrmEncode: OK");
    return H264ENC_FRAME_READY;
}

/*------------------------------------------------------------------------------

    Function name : H264EncStrmEnd
    Description   : Ends a stream
    Return type   : H264EncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
H264EncRet H264EncStrmEnd(H264EncInst inst, const H264EncIn * pEncIn,
                          H264EncOut * pEncOut)
{
    h264Instance_s *pEncInst = (h264Instance_s *)inst;

    APITRACE("H264EncStrmEnd#");
    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACE("H264EncStrmEnd: ERROR Null argument");
        return H264ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACE("H264EncStrmEnd: ERROR Invalid instance");
        return H264ENC_INSTANCE_ERROR;
    }

    /* Check status, this also makes sure that the instance is valid */
    if ((pEncInst->encStatus != H264ENCSTAT_START_FRAME) &&
        (pEncInst->encStatus != H264ENCSTAT_START_STREAM)) {
        APITRACE("H264EncStrmEnd: ERROR Invalid status");
        return H264ENC_INVALID_STATUS;
    }

    pEncOut->streamSize = 0;

    /* Check for invalid input values */
    if (pEncIn->pOutBuf == NULL ||
        (pEncIn->outBufSize < H264ENCSTRMSTART_MIN_BUF)) {
        APITRACE("H264EncStrmEnd: ERROR Invalid input. Stream buffer");
        return H264ENC_INVALID_ARGUMENT;
    }

    if (pEncIn->pNaluSizeBuf != NULL &&
        pEncIn->naluSizeBufSize < (2 * sizeof(u32))) {
        APITRACE("H264EncStrmEnd: ERROR Invalid input. NAL size buffer");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Set stream buffer and check the size */
    if (H264SetBuffer(&pEncInst->stream, (u8 *) pEncIn->pOutBuf,
                      (u32) pEncIn->outBufSize) != ENCHW_OK) {
        APITRACE("H264EncStrmEnd: ERROR Output buffer too small");
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Write end-of-stream code */
//    H264EndOfSequence(&pEncInst->stream, &pEncInst->seqParameterSet);

    /* Bytes generated */
    pEncOut->streamSize = pEncInst->stream.byteCnt;

    if (pEncIn->pNaluSizeBuf != NULL) {
        pEncIn->pNaluSizeBuf[0] = pEncOut->streamSize;
        pEncIn->pNaluSizeBuf[1] = 0;
    }

    /* Status == INIT   Stream ended, next stream can be started */
    pEncInst->encStatus = H264ENCSTAT_INIT;

    APITRACE("H264EncStrmEnd: OK");
    return H264ENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : H264GetNalUnitSizes
    Description   : Writes each NAL unit size to the user supplied buffer.
        Takes sizes from the buffer written by HW. List ends with a zero value.

    Return type   : void
    Argument      : pEncInst - encoder instance
    Argument      : pNaluSizeBuf - where to write the NAL sizes
------------------------------------------------------------------------------*/
void H264GetNalUnitSizes(const h264Instance_s * pEncInst, u32 * pNaluSizeBuf)
{
    const sei_s *sei = &pEncInst->rateControl.sei;
    const u32 *pTmp;
    i32 nals = 0;

    if (sei->enabled == ENCHW_YES || sei->userDataEnabled == ENCHW_YES) {
        *pNaluSizeBuf++ = sei->nalUnitSize; /* first NAL is the SEI message */
    }

    if (pEncInst->slice.sliceSize != 0)
        nals += (pEncInst->mbPerFrame + pEncInst->slice.sliceSize - 1) /
                pEncInst->slice.sliceSize;

    //pTmp = (u32 *) pEncInst->asic.sizeTbl.nal.vir_addr;  // mask by lance 2016.05.05
    pTmp = (u32 *)mpp_buffer_get_ptr(pEncInst->asic.sizeTbl.nal);  // add by lance 2016.05.05

    /* whole frame can be in one NAL unit */
    do {
        *pNaluSizeBuf++ = (*pTmp++) / 8;    /* bits to bytes */
    } while ((--nals > 0) && (*pTmp != 0));

    if (pEncInst->fillerNalSize)
        *pNaluSizeBuf++ = pEncInst->fillerNalSize;

    *pNaluSizeBuf = 0;  /* END of table */
}

/*------------------------------------------------------------------------------
    Function name : H264EncSetTestId
    Description   : Sets the encoder configuration according to a test vector
    Return type   : H264EncRet
    Argument      : inst - encoder instance
    Argument      : testId - test vector ID
------------------------------------------------------------------------------*/
H264EncRet H264EncSetTestId(H264EncInst inst, u32 testId)
{
    h264Instance_s *pEncInst = (h264Instance_s *) inst;
    (void) pEncInst;
    (void) testId;

    APITRACE("H264EncSetTestId#");

#ifdef INTERNAL_TEST
    pEncInst->testId = testId;

    APITRACE("H264EncSetTestId# OK");
    return H264ENC_OK;
#else
    /* Software compiled without testing support, return error always */
    APITRACE("H264EncSetTestId# ERROR, testing disabled at compile time");
    return H264ENC_ERROR;
#endif
}

