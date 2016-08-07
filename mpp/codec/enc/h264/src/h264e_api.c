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

#define MODULE_TAG "h264e_api"

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "H264Instance.h"
#include "h264e_api.h"
#include "h264e_codec.h"
#include "h264e_syntax.h"

#include "h264encapi.h"
#include "mpp_controller.h"
#include "h264e_utils.h"

RK_U32 h264e_ctrl_debug = 0;

MPP_RET h264e_init(void *ctx, ControllerCfg *ctrlCfg)
{
    h264e_dbg_func("enter\n");

    H264ECtx * pEncInst = (H264ECtx*)ctx;
    MPP_RET ret = (MPP_RET)H264EncInit(pEncInst);

    pEncInst->encStatus = H264ENCSTAT_INIT;
    pEncInst->inst = pEncInst;

    mpp_env_get_u32("h264e_debug", &h264e_debug, 0);

    if (ret) {
        mpp_err_f("H264EncInit() failed ret %d", ret);
    }

    (void)ctrlCfg;

    h264e_dbg_func("leave\n");
    return ret;
}

MPP_RET h264e_deinit(void *ctx)
{
    H264ECtx * pEncInst = (H264ECtx *)ctx;
    H264EncRet ret/* = MPP_OK*/;
    H264EncIn *encIn = &(pEncInst->encIn);
    H264EncOut *encOut = &(pEncInst->encOut);

    /* End stream */
    ret = H264EncStrmEnd(pEncInst, encIn, encOut);
    if (ret != H264ENC_OK) {
        mpp_err("H264EncStrmEnd() failed, ret %d.", ret);
    }

    if ((ret = H264EncRelease(pEncInst)) != H264ENC_OK) {
        mpp_err("H264EncRelease() failed, ret %d.", ret);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET h264e_encode(void *ctx, HalEncTask *task)
{
    H264EncRet ret;
    H264ECtx *p = (H264ECtx *)ctx;
    H264EncIn *encIn = &(p->encIn);
    H264EncOut *encOut = &(p->encOut);
    RK_U32 srcLumaWidth = p->lumWidthSrc;
    RK_U32 srcLumaHeight = p->lumHeightSrc;

    encIn->pOutBuf = (u32*)mpp_buffer_get_ptr(task->output);
    encIn->busOutBuf = mpp_buffer_get_fd(task->output);
    encIn->outBufSize = mpp_buffer_get_size(task->output);

    /* Start stream */
    if (p->encStatus == H264ENCSTAT_INIT) {
        ret = H264EncStrmStart(p, encIn, encOut);
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
        u32 w = srcLumaWidth;
        encIn->busLuma = mpp_buffer_get_fd(task->input);

        encIn->busChromaU = encIn->busLuma | ((w * srcLumaHeight) << 10);
        encIn->busChromaV = encIn->busChromaU +
                            ((((w + 1) >> 1) * ((srcLumaHeight + 1) >> 1)) << 10);
    }

    /* Select frame type */
    if (p->intraPicRate != 0 && (p->intraPeriodCnt >= p->intraPicRate)) {
        encIn->codingType   = H264ENC_INTRA_FRAME;
        task->is_intra      = 1;
    } else {
        encIn->codingType   = H264ENC_PREDICTED_FRAME;
        task->is_intra      = 0;
    }

    if (encIn->codingType == H264ENC_INTRA_FRAME)
        p->intraPeriodCnt = 0;

    memset(&p->syntax, 0, sizeof(p->syntax));
    ret = H264EncStrmEncode(p, encIn, encOut, &p->syntax);
    if (ret != H264ENC_FRAME_READY) {
        mpp_err("H264EncStrmEncode() failed, ret %d.", ret);  // TODO    need to be modified by lance 2016.05.31
        return MPP_NOK;
    }

    task->syntax.data   = &p->syntax;
    task->syntax.number = 1;

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


static const H264Profile h264e_supported_profile[] = {
    H264_PROFILE_BASELINE,
    H264_PROFILE_MAIN,
    H264_PROFILE_HIGH,
};

static const H264Level h264e_supported_level[] = {
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
    H264ECtx *enc = (H264ECtx *)ctx;    // add by lance 2016.05.31

    h264e_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case CHK_ENC_CFG : {
        ret = h264e_check_mpp_cfg((MppEncConfig *)param);
    } break;
    case SET_ENC_CFG : {
        MppEncConfig  *mpp_cfg = (MppEncConfig *)param;
        H264EncConfig *enc_cfg = &enc->enc_cfg;

        H264EncCodingCtrl oriCodingCfg;

        enc_cfg->streamType = H264ENC_BYTE_STREAM;
        enc_cfg->frameRateDenom = 1;
        enc_cfg->profile    = (H264Profile)mpp_cfg->profile;
        enc_cfg->level      = (H264Level)mpp_cfg->level;

        if (mpp_cfg->width && mpp_cfg->height) {
            enc_cfg->width  = mpp_cfg->width;
            enc_cfg->height = mpp_cfg->height;
        } else
            mpp_err("width %d height %d is not available\n", mpp_cfg->width, mpp_cfg->height);

        enc_cfg->frameRateNum = mpp_cfg->fps_in;
        if (mpp_cfg->cabac_en)
            enc_cfg->enable_cabac = mpp_cfg->cabac_en;
        else
            enc_cfg->enable_cabac = 0;

        enc_cfg->transform8x8_mode = (enc_cfg->profile >= H264_PROFILE_HIGH) ? (1) : (0);
        enc_cfg->chroma_qp_index_offset = 2;
        enc_cfg->pic_init_qp = mpp_cfg->qp;
        enc_cfg->second_chroma_qp_index_offset = 2;
        enc_cfg->pps_id = 0;
        enc_cfg->input_image_format = (MppFrameFormat)mpp_cfg->format;

        ret = H264EncCfg(enc, enc_cfg);

        /* Encoder setup: coding control */
        ret = H264EncGetCodingCtrl(enc, &oriCodingCfg);
        if (ret) {
            mpp_err("H264EncGetCodingCtrl() failed, ret %d.", ret);
            h264e_deinit((void*)enc);
            break;
        } else {
            oriCodingCfg.sliceSize = 0;
            oriCodingCfg.constrainedIntraPrediction = 0;
            oriCodingCfg.disableDeblockingFilter = 0;
            oriCodingCfg.cabacInitIdc = 0;
            oriCodingCfg.videoFullRange = 0;
            oriCodingCfg.seiMessages = 0;
            ret = H264EncSetCodingCtrl(enc, &oriCodingCfg);
            if (ret) {
                mpp_err("H264EncSetCodingCtrl() failed, ret %d.", ret);
                h264e_deinit((void*)enc);
                break;
            }
        }
    } break;
    case SET_ENC_RC_CFG : {
        MppEncConfig    *mpp_cfg    = (MppEncConfig *)param;
        H264EncRateCtrl *enc_rc_cfg = &enc->enc_rc_cfg;
        H264EncRateCtrl oriRcCfg;

        mpp_assert(enc);

        if (mpp_cfg->rc_mode) {
            /* VBR / CBR mode */
            RK_S32 max_qp = MPP_MAX(mpp_cfg->qp + 6, 48);
            RK_S32 min_qp = MPP_MIN(mpp_cfg->qp - 6, 16);

            enc_rc_cfg->pictureRc       = 1;
            enc_rc_cfg->mbRc            = 1;
            enc_rc_cfg->qpHdr           = mpp_cfg->qp;
            enc_rc_cfg->qpMax           = max_qp;
            enc_rc_cfg->qpMin           = min_qp;
            enc_rc_cfg->hrd             = 1;
            enc_rc_cfg->intraQpDelta    = 3;
        } else {
            /* CQP mode */
            enc_rc_cfg->pictureRc       = 0;
            enc_rc_cfg->mbRc            = 0;
            enc_rc_cfg->qpHdr           = mpp_cfg->qp;
            enc_rc_cfg->qpMax           = mpp_cfg->qp;
            enc_rc_cfg->qpMin           = mpp_cfg->qp;
            enc_rc_cfg->hrd             = 0;
            enc_rc_cfg->intraQpDelta    = 0;
        }
        enc_rc_cfg->pictureSkip = mpp_cfg->skip_cnt;

        if (mpp_cfg->gop > 0)
            enc_rc_cfg->intraPicRate = mpp_cfg->gop;
        else
            enc_rc_cfg->intraPicRate = 30;

        enc_rc_cfg->keyframe_max_interval = 150;
        enc_rc_cfg->bitPerSecond = mpp_cfg->bps;
        enc_rc_cfg->gopLen = mpp_cfg->gop;
        enc_rc_cfg->fixedIntraQp = 0;
        enc_rc_cfg->mbQpAdjustment = 3;
        enc_rc_cfg->hrdCpbSize = mpp_cfg->bps;

        enc->intraPicRate = enc_rc_cfg->intraPicRate;
        enc->intraPeriodCnt = enc_rc_cfg->intraPicRate;

        /* Encoder setup: rate control */
        ret = H264EncGetRateCtrl(enc, &oriRcCfg);
        if (ret) {
            mpp_err("H264EncGetRateCtrl() failed, ret %d.", ret);
        } else {
            mpp_log_f("Get rate control: qp %2d [%2d, %2d] bps %8d\n",
                      oriRcCfg.qpHdr, oriRcCfg.qpMin, oriRcCfg.qpMax, oriRcCfg.bitPerSecond);

            mpp_log_f("pic %d mb %d skip %d hrd %d cpbSize %d gopLen %d\n",
                      oriRcCfg.pictureRc, oriRcCfg.mbRc, oriRcCfg.pictureSkip, oriRcCfg.hrd,
                      oriRcCfg.hrdCpbSize, oriRcCfg.gopLen);

            oriRcCfg.qpHdr = enc_rc_cfg->qpHdr;
            oriRcCfg.qpMin = enc_rc_cfg->qpMin;
            oriRcCfg.qpMax = enc_rc_cfg->qpMax;

            oriRcCfg.pictureSkip = enc_rc_cfg->pictureSkip;
            oriRcCfg.pictureRc = enc_rc_cfg->pictureRc;
            oriRcCfg.mbRc = enc_rc_cfg->mbRc;
            oriRcCfg.bitPerSecond = enc_rc_cfg->bitPerSecond;
            oriRcCfg.hrd = enc_rc_cfg->hrd;
            oriRcCfg.hrdCpbSize = enc_rc_cfg->hrdCpbSize;
            oriRcCfg.gopLen = enc_rc_cfg->gopLen;
            oriRcCfg.intraQpDelta = enc_rc_cfg->intraQpDelta;
            oriRcCfg.fixedIntraQp = enc_rc_cfg->fixedIntraQp;
            oriRcCfg.mbQpAdjustment = enc_rc_cfg->mbQpAdjustment;

            mpp_log("Set rate control: qp %2d [%2d, %2d] bps %8d\n",
                    oriRcCfg.qpHdr, oriRcCfg.qpMin, oriRcCfg.qpMax, oriRcCfg.bitPerSecond);

            mpp_log("pic %d mb %d skip %d hrd %d cpbSize %d gopLen %d\n",
                    oriRcCfg.pictureRc, oriRcCfg.mbRc, oriRcCfg.pictureSkip, oriRcCfg.hrd,
                    oriRcCfg.hrdCpbSize, oriRcCfg.gopLen);

            ret = H264EncSetRateCtrl(enc, &oriRcCfg);
            if (ret) {
                mpp_err("H264EncSetRateCtrl() failed, ret %d.", ret);
            }
        }
    } break;
    case GET_ENC_EXTRA_INFO : {
        h264e_control_extra_info_cfg **dst  = (h264e_control_extra_info_cfg **)param;
        h264e_control_extra_info_cfg *info  = &enc->info;
        H264EncConfig *enc_cfg              = &enc->enc_cfg;
        H264EncRateCtrl *enc_rc_cfg         = &enc->enc_rc_cfg;

        info->chroma_qp_index_offset        = enc_cfg->chroma_qp_index_offset;
        info->enable_cabac                  = enc_cfg->enable_cabac;
        info->pic_init_qp                   = enc_cfg->pic_init_qp;
        info->pic_luma_height               = enc_cfg->height;
        info->pic_luma_width                = enc_cfg->width;
        info->transform8x8_mode             = enc_cfg->transform8x8_mode;

        info->input_image_format            = enc_cfg->input_image_format;
        info->profile_idc                   = enc_cfg->profile;
        info->level_idc                     = enc_cfg->level;
        info->keyframe_max_interval         = enc_rc_cfg->keyframe_max_interval;
        info->second_chroma_qp_index_offset = enc_cfg->second_chroma_qp_index_offset;
        info->pps_id                        = enc_cfg->pps_id;

        *dst = info;
    } break;
    case GET_OUTPUT_STREAM_SIZE : {
        *((RK_U32*)param) = getOutputStreamSize(enc);
    } break;
    default:
        mpp_err("No correspond cmd found, and can not config!");
        break;
    }

    h264e_dbg_func("leave ret %d\n", ret);

    return ret;
}

MPP_RET h264e_callback(void *ctx, void *feedback)
{
    H264ECtx *enc = (H264ECtx *)ctx;
    regValues_s    *val = &(enc->asic.regs);
    h264e_feedback *fb  = (h264e_feedback *)feedback;
    H264EncOut *encOut  = &(enc->encOut);
    RK_S32 i = 0;
    H264EncRet ret;
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
    ret = H264EncStrmEncodeAfter(enc, encOut, vpuWaitResult);    // add by lance 2016.05.07
    switch (ret) {
    case H264ENC_FRAME_READY:
        if (encOut->codingType != H264ENC_NOTCODED_FRAME) {
            enc->intraPeriodCnt++;
        }
        break;

    case H264ENC_OUTPUT_BUFFER_OVERFLOW:
        mpp_log("output buffer overflow!");
        break;

    default:
        mpp_log("afterencode default!");
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
    sizeof(H264ECtx),
    0,
    h264e_init,
    h264e_deinit,
    h264e_encode,
    h264e_reset,
    h264e_flush,
    h264e_config,
    h264e_callback,
};
