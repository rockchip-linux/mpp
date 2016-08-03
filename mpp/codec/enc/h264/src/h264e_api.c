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

#define MODULE_TAG "H264E_API"

#include "mpp_log.h"
#include "mpp_common.h"

#include "H264Instance.h"
#include "h264e_api.h"
#include "h264e_codec.h"
#include "h264e_syntax.h"

#include "h264encapi.h"
#include "mpp_controller.h"
#include "h264e_utils.h"
#include "h264e_macro.h"

#ifdef SYNTAX_DATA_IN_FILE
FILE *fp_syntax_in = NULL;
#endif

MPP_RET h264e_init(void *ctx, ControllerCfg *ctrlCfg)
{
    h264Instance_s * pEncInst = (h264Instance_s*)ctx;
    MPP_RET ret = (MPP_RET)H264EncInit(pEncInst);

    if (ret) {
        mpp_err_f("H264EncInit() failed ret %d", ret);
    }

    (void)ctrlCfg;

    return ret;
}

MPP_RET h264e_deinit(void *ctx)
{
    H264EncInst encInst = (H264EncInst)ctx;
    h264Instance_s * pEncInst = (h264Instance_s*)ctx;
    H264EncRet ret/* = MPP_OK*/;
    H264EncIn *encIn = &(pEncInst->encIn);
    H264EncOut *encOut = &(pEncInst->encOut);

    /* End stream */
    ret = H264EncStrmEnd(encInst, encIn, encOut);
    if (ret != H264ENC_OK) {
        mpp_err("H264EncStrmEnd() failed, ret %d.", ret);
    }

    if ((ret = H264EncRelease(encInst)) != H264ENC_OK) {
        mpp_err("H264EncRelease() failed, ret %d.", ret);
        return MPP_NOK;
    }

#ifdef SYNTAX_DATA_IN_FILE
    if (NULL != fp_syntax_in) {
        fclose(fp_syntax_in);
        fp_syntax_in = NULL;
    }
#endif
    return MPP_OK;
}

MPP_RET h264e_encode(void *ctx, /*HalEncTask **/void *task)
{
    H264EncRet ret;
    H264EncInst encInst = (H264EncInst)ctx;
    H264EncInst encoderOpen = (H264EncInst)ctx;
    h264Instance_s *pEncInst = (h264Instance_s *)ctx;    // add by lance 2016.05.31
    H264EncIn *encIn = &(pEncInst->encIn);  // TODO modify by lance 2016.05.19
    H264EncOut *encOut = &(pEncInst->encOut);  // TODO modify by lance 2016.05.19
    RK_U32 srcLumaWidth = pEncInst->lumWidthSrc;
    RK_U32 srcLumaHeight = pEncInst->lumHeightSrc;

    encIn->pOutBuf = (u32*)mpp_buffer_get_ptr(((EncTask*)task)->ctrl_pkt_buf_out);
    encIn->busOutBuf = mpp_buffer_get_fd(((EncTask*)task)->ctrl_pkt_buf_out);
    encIn->outBufSize = mpp_buffer_get_size(((EncTask*)task)->ctrl_pkt_buf_out);

    /* Start stream */
    if (pEncInst->encStatus == H264ENCSTAT_INIT) {
        ret = H264EncStrmStart(encoderOpen, encIn, encOut);
        if (ret != H264ENC_OK) {
            mpp_err("H264EncStrmStart() failed, ret %d.", ret);
            return -1;
        }


        /* First frame is always intra with time increment = 0 */
        encIn->codingType = H264ENC_INTRA_FRAME;
        encIn->timeIncrement = 0;
    }

    /* Setup encoder input */
    {
        u32 w = (srcLumaWidth + 15) & (~0x0f);  // TODO    352 need to be modify by lance 2016.05.31
        // mask by lance 2016.07.04
        //MppBufferInfo inInfo;   // add by lance 2016.05.06
        //mpp_buffer_info_get(((EncTask*)task)->ctrl_frm_buf_in, &inInfo);
        encIn->busLuma = mpp_buffer_get_fd(((EncTask*)task)->ctrl_frm_buf_in)/*pictureMem.phy_addr*/;

        encIn->busChromaU = encIn->busLuma | ((w * srcLumaHeight) << 10);    // TODO    288 need to be modify by lance 2016.05.31
        encIn->busChromaV = encIn->busChromaU +
                            ((((w + 1) >> 1) * ((srcLumaHeight + 1) >> 1)) << 10);    // TODO    288 need to be modify by lance 2016.05.31
        // mask by lance 2016.07.04
        /*mpp_log("enc->busChromaU %15x V %15x w %d srcLumaHeight %d size %d",
                encIn->busChromaU, encIn->busChromaV, w, srcLumaHeight, inInfo.size);*/
    }



    //encIn.busLumaStab = mpp_buffer_get_fd(pictureStabMem)/*pictureStabMem.phy_addr*/;

    /* Select frame type */
    if (pEncInst->intraPicRate != 0 &&
        (pEncInst->intraPeriodCnt >= pEncInst->intraPicRate)) {
        encIn->codingType = H264ENC_INTRA_FRAME;
    } else {
        encIn->codingType = H264ENC_PREDICTED_FRAME;
    }

    if (encIn->codingType == H264ENC_INTRA_FRAME)
        pEncInst->intraPeriodCnt = 0;

    // TODO syntax_data need to be assigned    modify by lance 2016.05.19
    ret = H264EncStrmEncode(encInst, encIn, encOut, &(((EncTask*)task)->syntax_data));
    if (ret != H264ENC_FRAME_READY) {
        mpp_err("H264EncStrmEncode() failed, ret %d.", ret);  // TODO    need to be modified by lance 2016.05.31
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET h264e_reset(void *ctx)
{
    (void)ctx;
    return MPP_OK;
}

MPP_RET h264e_flush(void *ctx)
{
    (void)ctx;
    return MPP_OK;
}


static const H264Profile h264e_supported_profile[] =
{
    H264_PROFILE_BASELINE,
    H264_PROFILE_MAIN,
    H264_PROFILE_HIGH,
};

static const H264Level h264e_supported_level[] =
{
    H264_LEVEL_1,
    H264_LEVEL_1_b,
    H264_LEVEL_1_1,
    H264_LEVEL_1_2,
    H264_LEVEL_1_3,
    H264_LEVEL_2,
    H264_LEVEL_2_1,
    H264_LEVEL_2_2,
    H264_LEVEL_3,
    H264_LEVEL_3_1,
    H264_LEVEL_3_2,
    H264_LEVEL_4_0,
    H264_LEVEL_4_1,
    H264_LEVEL_4_2,
};

static MPP_RET h264e_check_mpp_cfg(MppEncConfig *mpp_cfg)
{
    MPP_RET ret = MPP_NOK;
    RK_U32 i, count;

    count = MPP_ARRAY_ELEMS(h264e_supported_profile);
    for (i = 0; i < count; i++) {
        if (h264e_supported_profile[i] == (H264Profile)mpp_cfg->profile) {
            break;
        }
    }

    if (i >= count) {
        mpp_log_f("invalid profile %d set to default baseline\n", mpp_cfg->profile);
        mpp_cfg->profile = H264_PROFILE_BASELINE;
    }

    count = MPP_ARRAY_ELEMS(h264e_supported_level);
    for (i = 0; i < count; i++) {
        if (h264e_supported_level[i] == (H264Level)mpp_cfg->level) {
            break;
        }
    }

    if (i >= count) {
        mpp_log_f("invalid level %d set to default 4.0\n", mpp_cfg->level);
        mpp_cfg->level = H264_LEVEL_3;
    }

    if (mpp_cfg->fps_in <= 0) {
        mpp_log_f("invalid input fps %d will be set to default 30\n", mpp_cfg->fps_in);
        mpp_cfg->fps_in = 30;
    }

    if (mpp_cfg->fps_out <= 0) {
        mpp_log_f("invalid output fps %d will be set to fps_in 30\n", mpp_cfg->fps_out, mpp_cfg->fps_in);
        mpp_cfg->fps_out = mpp_cfg->fps_in;
    }

    if (mpp_cfg->gop <= 0) {
        mpp_log_f("invalid gop %d will be set to fps_out 30\n", mpp_cfg->gop, mpp_cfg->fps_out);
        mpp_cfg->gop = mpp_cfg->fps_out;
    }

    if (mpp_cfg->rc_mode < 0 || mpp_cfg->rc_mode > 2) {
        mpp_err_f("invalid rc_mode %d\n", mpp_cfg->rc_mode);
        return ret;
    }

    if (mpp_cfg->qp <= 0 || mpp_cfg->qp > 51) {
        mpp_log_f("invalid qp %d set to default 26\n", mpp_cfg->qp);
        mpp_cfg->qp = 26;
    }

    if (mpp_cfg->bps <= 0) {
        mpp_err_f("invalid bit rate %d\n", mpp_cfg->bps);
        return ret;
    }

    if (mpp_cfg->width <= 0 || mpp_cfg->height <= 0) {
        mpp_err_f("invalid width %d height %d\n", mpp_cfg->width, mpp_cfg->height);
        return ret;
    }

    if (mpp_cfg->hor_stride <= 0) {
        mpp_err_f("invalid hor_stride %d will be set to %d\n", mpp_cfg->hor_stride, MPP_ALIGN(mpp_cfg->width, 16));
        mpp_cfg->hor_stride = MPP_ALIGN(mpp_cfg->width, 16);
    }

    if (mpp_cfg->ver_stride <= 0) {
        mpp_err_f("invalid ver_stride %d will be set to %d\n", mpp_cfg->ver_stride, MPP_ALIGN(mpp_cfg->height, 16));
        mpp_cfg->ver_stride = MPP_ALIGN(mpp_cfg->height, 16);
    }

    return MPP_OK;
}

MPP_RET h264e_config(void *ctx, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_NOK;
    h264Instance_s *pEncInst = (h264Instance_s *)ctx;    // add by lance 2016.05.31

    h264e_control_debug_enter();

    switch (cmd) {
    case CHK_ENC_CFG : {
        ret = h264e_check_mpp_cfg((MppEncConfig *)param);
    } break;
    case SET_ENC_CFG : {
        const H264EncConfig *encCfg = (const H264EncConfig *)param;

        H264EncInst encoder = (H264EncInst)ctx;
        H264EncCodingCtrl oriCodingCfg;
        H264EncPreProcessingCfg oriPreProcCfg;

        ret = H264EncCfg(encoder, encCfg);

        /* Encoder setup: coding control */
        ret = H264EncGetCodingCtrl(encoder, &oriCodingCfg);
        if (ret) {
            mpp_err("H264EncGetCodingCtrl() failed, ret %d.", ret);
            h264e_deinit((void*)encoder);
            break;
        } else {
            // will be replaced  modify by lance 2016.05.20
            // ------------
            oriCodingCfg.sliceSize = 0;
            oriCodingCfg.constrainedIntraPrediction = 0;
            oriCodingCfg.disableDeblockingFilter = 0;
            oriCodingCfg.enableCabac = 0;
            oriCodingCfg.cabacInitIdc = 0;
            oriCodingCfg.transform8x8Mode = 0;
            oriCodingCfg.videoFullRange = 0;
            oriCodingCfg.seiMessages = 0;
            ret = H264EncSetCodingCtrl(encoder, &oriCodingCfg);
            if (ret) {
                mpp_err("H264EncSetCodingCtrl() failed, ret %d.", ret);
                h264e_deinit((void*)encoder);
                break;
            }
        }

        /* PreP setup */
        ret = H264EncGetPreProcessing(encoder, &oriPreProcCfg);
        if (ret) {
            mpp_err("H264EncGetPreProcessing() failed, ret %d.\n", ret);
            h264e_deinit((void*)encoder);
            break;
        } else {
            h264e_control_log("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
                              ": stab %d : cc %d\n",
                              oriPreProcCfg.origWidth, oriPreProcCfg.origHeight, oriPreProcCfg.xOffset,
                              oriPreProcCfg.yOffset, oriPreProcCfg.inputType, oriPreProcCfg.rotation,
                              oriPreProcCfg.videoStabilization, oriPreProcCfg.colorConversion.type);
            // ----------------
            // will be replaced  modify by lance 2016.05.20
            oriPreProcCfg.inputType = H264ENC_YUV420_SEMIPLANAR;//H264ENC_YUV420_PLANAR;
            oriPreProcCfg.rotation = H264ENC_ROTATE_0;
            oriPreProcCfg.origWidth = encCfg->width;
            oriPreProcCfg.origHeight = encCfg->height;
            oriPreProcCfg.xOffset = 0;
            oriPreProcCfg.yOffset = 0;
            oriPreProcCfg.videoStabilization = 0;
            oriPreProcCfg.colorConversion.type = H264ENC_RGBTOYUV_BT601;
            if (oriPreProcCfg.colorConversion.type == H264ENC_RGBTOYUV_USER_DEFINED) {
                oriPreProcCfg.colorConversion.coeffA = 20000;
                oriPreProcCfg.colorConversion.coeffB = 44000;
                oriPreProcCfg.colorConversion.coeffC = 5000;
                oriPreProcCfg.colorConversion.coeffE = 35000;
                oriPreProcCfg.colorConversion.coeffF = 38000;
            }

            ret = H264EncSetPreProcessing(encoder, &oriPreProcCfg);
            if (ret) {
                mpp_err("H264EncSetPreProcessing() failed.", ret);
                h264e_deinit((void*)encoder);
                break;
            }
        }

        // add by lance 2016.06.16
#ifdef SYNTAX_DATA_IN_FILE
        if (NULL == fp_syntax_in) {
            fp_syntax_in = fopen("/data/test/vpu_syntax_in.txt", "ab");
            if (NULL == fp_syntax_in)
                mpp_err("open fp_syntax_in failed!");
        }
#endif
    } break;
    case SET_ENC_RC_CFG : {
        const H264EncRateCtrl *encCfg = (const H264EncRateCtrl *)param;
        H264EncInst encoder = (H264EncInst)ctx;
        H264EncRateCtrl oriRcCfg;

        mpp_assert(pEncInst);
        pEncInst->intraPicRate = encCfg->intraPicRate;
        pEncInst->intraPeriodCnt = encCfg->intraPicRate;

        /* Encoder setup: rate control */
        ret = H264EncGetRateCtrl(encoder, &oriRcCfg);
        if (ret) {
            mpp_err("H264EncGetRateCtrl() failed, ret %d.", ret);
        } else {
            h264e_control_log("Get rate control: qp %2d [%2d, %2d] bps %8d\n",
                              oriRcCfg.qpHdr, oriRcCfg.qpMin, oriRcCfg.qpMax, oriRcCfg.bitPerSecond);

            h264e_control_log("pic %d mb %d skip %d hrd %d cpbSize %d gopLen %d\n",
                              oriRcCfg.pictureRc, oriRcCfg.mbRc, oriRcCfg.pictureSkip, oriRcCfg.hrd,
                              oriRcCfg.hrdCpbSize, oriRcCfg.gopLen);

            // will be replaced  modify by lance 2016.05.20
            // ------------
            if (encCfg->qpHdr)
                oriRcCfg.qpHdr = encCfg->qpHdr;

            if (encCfg->qpMin)
                oriRcCfg.qpMin = encCfg->qpMin;

            if (encCfg->qpMax)
                oriRcCfg.qpMax = encCfg->qpMax;

            oriRcCfg.pictureSkip = encCfg->pictureSkip;
            oriRcCfg.pictureRc = encCfg->pictureRc;
            oriRcCfg.mbRc = encCfg->mbRc;
            oriRcCfg.bitPerSecond = encCfg->bitPerSecond;
            oriRcCfg.hrd = encCfg->hrd;
            oriRcCfg.hrdCpbSize = encCfg->hrdCpbSize;
            oriRcCfg.gopLen = encCfg->gopLen;
            oriRcCfg.intraQpDelta = encCfg->intraQpDelta;
            oriRcCfg.fixedIntraQp = encCfg->fixedIntraQp;
            oriRcCfg.mbQpAdjustment = encCfg->mbQpAdjustment;

            h264e_control_log("Set rate control: qp %2d [%2d, %2d] bps %8d\n",
                              oriRcCfg.qpHdr, oriRcCfg.qpMin, oriRcCfg.qpMax, oriRcCfg.bitPerSecond);

            h264e_control_log("pic %d mb %d skip %d hrd %d cpbSize %d gopLen %d\n",
                              oriRcCfg.pictureRc, oriRcCfg.mbRc, oriRcCfg.pictureSkip, oriRcCfg.hrd,
                              oriRcCfg.hrdCpbSize, oriRcCfg.gopLen);

            ret = H264EncSetRateCtrl(encoder, &oriRcCfg);
            if (ret) {
                mpp_err("H264EncSetRateCtrl() failed, ret %d.", ret);
            }
        }
    } break;
    case GET_OUTPUT_STREAM_SIZE:
        *((RK_U32*)param) = getOutputStreamSize(pEncInst);
        break;
    default:
        mpp_err("No correspond cmd found, and can not config!");
        break;
    }
    h264e_control_debug_leave();

    return ret;
}

MPP_RET h264e_callback(void *ctx, void *feedback)
{
    h264Instance_s *pEncInst = (h264Instance_s *)ctx;
    regValues_s *val = &(pEncInst->asic.regs);
    h264e_feedback *fb = (h264e_feedback *)feedback;
    int i = 0;

    H264EncRet ret;
    H264EncInst encInst = (H264EncInst)ctx;
    H264EncOut *encOut = &(pEncInst->encOut);  // TODO modify by lance 2016.05.19
    MPP_RET vpuWaitResult = MPP_OK;

    /* HW output stream size, bits to bytes */
    val->outputStrmSize = fb->out_strm_size;

    if (val->cpTarget != NULL) {
        /* video coding with MB rate control ON */
        for (i = 0; i < 10; i++) {
            val->cpTargetResults[i] = fb->cp[i];
        }
    }

    /* QP sum div2 */
    val->qpSum = fb->qp_sum;

    /* MAD MB count*/
    val->madCount = fb->mad_count;

    /* Non-zero coefficient count*/
    val->rlcCount = fb->rlc_count;

    /*hw status*/
    val->hw_status = fb->hw_status;

    // vpuWaitResult should be given from hal part, and here assume it is OK  // TODO  modify by lance 2016.06.01
    ret = H264EncStrmEncodeAfter(encInst, encOut, vpuWaitResult);    // add by lance 2016.05.07
    switch (ret) {
    case H264ENC_FRAME_READY:
        h264e_control_log("after encode frame ready");  // add by lance 2016.06.01


        if (encOut->codingType != H264ENC_NOTCODED_FRAME) {
            pEncInst->intraPeriodCnt++;
        }
#if 0
        mpp_log("%5i | %3i | %2i | %s | %8u | %7i %6i | ",
                next, frameCnt, rc.qpHdr,
                encOut->codingType == H264ENC_INTRA_FRAME ? " I  " :
                encOut->codingType == H264ENC_PREDICTED_FRAME ? " P  " : "skip",
                bitrate, streamSize, encOut->streamSize);
        if (cml->psnr)
            psnr = PrintPSNR(/*(u8 *)
                                 (((h264Instance_s *)encoderInst)->asic.regs.inputLumBase +
                                  ((h264Instance_s *)encoderInst)->asic.regs.inputLumaBaseOffset),
                                 (u8 *)
                                 (((h264Instance_s *)encoderInst)->asic.regs.internalImageLumBaseR),
                                 cml->lumWidthSrc, cml->width, cml->height*/);  // mask by lance 2016.05.12
        if (psnr) {
            psnrSum += psnr;
            psnrCnt++;
        }
        // mask and add by lance 2016.05.06
        /*PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.vir_addr,
          encOut.streamSize, cml->byteStream);*/
        PrintNalSizes(encIn->pNaluSizeBuf/*, (u8 *) mpp_buffer_get_ptr(outbufMem),
                                              encOut.streamSize, cml->byteStream*/);  // mask by lance 2016.05.12
        mpp_log("\n");

        // mask and add by lance 2016.05.06
        //WriteStrm(fout, outbufMem.vir_addr, encOut.streamSize, 0);
        WriteStrm(fout, (u32*)mpp_buffer_get_ptr(outbufMem), encOut->streamSize, 0);

        if (cml->byteStream == 0) {
            WriteNalSizesToFile(nal_sizes_file, encIn->pNaluSizeBuf,
                                encIn->naluSizeBufSize);
        }

        if (pUserData) {
            /* We want the user data to be written only once so
             * we disable the user data and free the memory after
             * first frame has been encoded. */
            H264EncSetSeiUserData(encoderInst, NULL, 0);
            free(pUserData);
            pUserData = NULL;
        }
#endif
        break;

    case H264ENC_OUTPUT_BUFFER_OVERFLOW:
        mpp_log("output buffer overflow!");
#if 0
        mpp_log("%5i | %3i | %2i | %s | %8u | %7i %6i | \n",
                next, frameCnt, rc.qpHdr, "lost",
                bitrate, streamSize, encOut->streamSize);
#endif
        break;

    default:
        mpp_log("afterencode default!");
#if 0
        PrintErrorValue("H264EncStrmEncode() failed.", ret);
        /* For debugging, can be removed */
        // mask and add by lance 2016.05.06
        //WriteStrm(fout, outbufMem.vir_addr, encOut.streamSize, 0);
        WriteStrm(fout, (u32*)mpp_buffer_get_ptr(outbufMem), encOut->streamSize, 0);
        /* We try to continue encoding the next frame */
#endif
        break;
    }

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/
const ControlApi api_h264e_controller = {
    "h264e_control",
    MPP_VIDEO_CodingAVC,
    sizeof(h264Instance_s),
    0,
    h264e_init,
    h264e_deinit,
    h264e_encode,
    h264e_reset,
    h264e_flush,
    h264e_config,
    h264e_callback,
};
