/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_h264e_com.h"
#include "hal_h264e_vepu.h"
#include "hal_h264e_rc.h"
#include "hal_h264e_vpu_tbl.h"

const RK_S32 h264_q_step[] = {
    3,   3,   3,   4,   4,   5,   5,   6,   7,   7,
    8,   9,   10,  11,  13,  14,  16,  18,  20,  23,
    25,  28,  32,  36,  40,  45,  51,  57,  64,  72,
    80,  90,  101, 114, 128, 144, 160, 180, 203, 228,
    256, 288, 320, 360, 405, 456, 513, 577, 640, 720,
    810, 896
};

static RK_S32 find_best_qp(MppLinReg *ctx, MppEncH264Cfg *codec,
                           RK_S32 qp_start, RK_S32 bits)
{
    RK_S32 qp = qp_start;
    RK_S32 qp_best = qp_start;
    RK_S32 qp_min = codec->qp_min;
    RK_S32 qp_max = codec->qp_max;
    RK_S32 diff_best = INT_MAX;

    if (ctx->a == 0 && ctx->b == 0)
        return qp_best;

    if (bits <= 0) {
        qp_best = mpp_clip(qp_best + codec->qp_max_step, qp_min, qp_max);
    } else {
        do {
            RK_S32 est_bits = mpp_quadreg_calc(ctx, h264_q_step[qp]);
            RK_S32 diff = est_bits - bits;
            if (MPP_ABS(diff) < MPP_ABS(diff_best)) {
                diff_best = MPP_ABS(diff);
                qp_best = qp;
                if (diff > 0)
                    qp++;
                else
                    qp--;
            } else
                break;
        } while (qp <= qp_max && qp >= qp_min);
    }

    return qp_best;
}

#define WORD_CNT_MAX      65535

MPP_RET h264e_vpu_mb_rc_cfg(H264eHalContext *ctx, RcSyntax *rc_syn, H264eHwCfg *hw_cfg)
{
    const RK_S32 sscale = 256;
    VepuQpCtrl *qc = &hw_cfg->qpCtrl;
    RK_S32 scaler, srcPrm;
    RK_S32 i;
    RK_S32 tmp, nonZeroTarget;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    int intraQpDelta = 3;
    MppEncRcCfg *rc = &cfg->rc;
    RK_S32 mbPerPic = (hw_cfg->width + 15) / 16 * (hw_cfg->height + 15) / 16;
    RK_S32 coeffCntMax = mbPerPic * 24 * 16;
    RK_S32 bits_per_pic = axb_div_c(rc->bps_target,
                                    rc->fps_out_denorm,
                                    rc->fps_out_num);

    if (hw_cfg->qp <= 0) {
        RK_S32 qp_tbl[2][13] = {
            {
                26, 36, 48, 63, 85, 110, 152, 208, 313, 427, 936,
                1472, 0x7fffffff
            },
            {42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12, 9, 6}
        };
        RK_S32 pels = ctx->cfg->prep.width * ctx->cfg->prep.height;
        if (pels) {
            RK_S32 upscale = 8000;
            if (bits_per_pic > 1000000)
                hw_cfg->qp = codec->qp_min;
            else {
                RK_S32 j = -1;

                pels >>= 8;
                bits_per_pic >>= 5;

                bits_per_pic *= pels + 250;
                bits_per_pic /= 350 + (3 * pels) / 4;
                bits_per_pic = axb_div_c(bits_per_pic, upscale, pels << 6);

                while (qp_tbl[0][++j] < bits_per_pic);

                hw_cfg->qp = qp_tbl[1][j];
                hw_cfg->qp_prev = hw_cfg->qp;
            }
        }
        //first frame init
    }
    if (ctx->frame_cnt == 0) {
        RK_S32 mbRows = ctx->cfg->prep.height / 16;
        hw_cfg->mad_qp_delta = 2;
        hw_cfg->mad_threshold = 256 * 6;
        hw_cfg->qpCtrl.checkPoints = MPP_MIN(mbRows - 1, CHECK_POINTS_MAX);
        if (rc->rc_mode == MPP_ENC_RC_MODE_CBR) {
            hw_cfg->qpCtrl.checkPointDistance =
                mbPerPic / (hw_cfg->qpCtrl.checkPoints + 1);
        } else {
            hw_cfg->qpCtrl.checkPointDistance = 0;
        }
    }
    /* frame type and rate control setup */
    {
        hw_cfg->pre_frame_type = hw_cfg->frame_type;
        if (rc_syn->type == INTRA_FRAME) {
            hw_cfg->frame_type = H264E_VPU_FRAME_I;
            hw_cfg->frame_num = 0;
            if (ctx->frame_cnt > 0) {
                hw_cfg->qp = mpp_data_avg(ctx->qp_p, -1, 1, 1);
                hw_cfg->qp += intraQpDelta;
            }
            /*
             * Previous frame is inter then intra frame can not
             * have a big qp step between these two frames
             */
            if (hw_cfg->pre_frame_type  == H264E_VPU_FRAME_P)
                hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 4,
                                      hw_cfg->qp_prev + 4);
            else
                hw_cfg->qp = find_best_qp(ctx->intra_qs, codec, hw_cfg->qp_prev,
                                          rc_syn->bit_target);
        } else {
            hw_cfg->frame_type = H264E_VPU_FRAME_P;

            hw_cfg->qp = find_best_qp(ctx->inter_qs, codec, hw_cfg->qp_prev,
                                      rc_syn->bit_target);

            if (hw_cfg->pre_frame_type == H264E_VPU_FRAME_I)
                hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 4,
                                      hw_cfg->qp_prev + 4);
            else {
                if (hw_cfg->pre_bit_diff < 0 && hw_cfg->qp >= hw_cfg->qp_prev) {
                    hw_cfg->pre_bit_diff = abs(hw_cfg->pre_bit_diff);
                    if (hw_cfg->pre_bit_diff <=  rc_syn->bit_target * 1 / 5) {
                        hw_cfg->qp = hw_cfg->qp_prev;
                    } else if (hw_cfg->pre_bit_diff >  rc_syn->bit_target * 1 / 5)
                        hw_cfg->qp  = hw_cfg->qp_prev - 1;
                    else if (hw_cfg->pre_bit_diff >  rc_syn->bit_target * 2 / 3)
                        hw_cfg->qp  = hw_cfg->qp_prev - 2;
                    else
                        hw_cfg->qp  = hw_cfg->qp_prev - 3;
                }
            }
        }
    }
    hw_cfg->qp = mpp_clip(hw_cfg->qp,
                          hw_cfg->qp_prev - codec->qp_max_step,
                          hw_cfg->qp_prev + codec->qp_max_step);

    hw_cfg->qp = mpp_clip(hw_cfg->qp, codec->qp_min, codec->qp_max);


    if (qc->nonZeroCnt == 0) {
        qc->nonZeroCnt = 1;
    }

    srcPrm = axb_div_c(qc->frameBitCnt, 256, qc->nonZeroCnt);
    /* Disable Mb Rc for Intra Slices, because coeffTarget will be wrong */
    if (hw_cfg->frame_type == INTRA_FRAME || srcPrm == 0) {
        return 0;
    }

    /* Required zero cnt */
    nonZeroTarget = axb_div_c(rc_syn->bit_target, 256, srcPrm);
    nonZeroTarget = MPP_MIN(coeffCntMax, MPP_MAX(0, nonZeroTarget));
    nonZeroTarget = MPP_MIN(0x7FFFFFFFU / 1024U, (RK_U32)nonZeroTarget);

    if (nonZeroTarget > 0) {
        scaler = axb_div_c(nonZeroTarget, sscale, (RK_S32) mbPerPic);
    } else {
        return 0;
    }

    if ((hw_cfg->frame_type != hw_cfg->pre_frame_type) || (qc->nonZeroCnt == 0)) {

        for (i = 0; i < qc->checkPoints; i++) {
            tmp = (scaler * (qc->checkPointDistance * (i + 1) + 1)) / sscale;
            tmp = MPP_MIN(WORD_CNT_MAX, tmp / 32 + 1);
            if (tmp < 0) tmp = WORD_CNT_MAX;    /* Detect overflow */
            hw_cfg->cp_target[i] = tmp; /* div32 for regs */
        }
        tmp = axb_div_c(bits_per_pic, 256, srcPrm);
    } else {
        for (i = 0; i < qc->checkPoints; i++) {
            tmp = (RK_S32) (qc->wordCntPrev[i] * scaler) / sscale;
            tmp = MPP_MIN(WORD_CNT_MAX, tmp / 32 + 1);
            if (tmp < 0) tmp = WORD_CNT_MAX;    /* Detect overflow */
            hw_cfg->cp_target[i] = tmp; /* div32 for regs */
        }
        tmp = axb_div_c(bits_per_pic, 256, srcPrm);
    }

    hw_cfg->target_error[0] = -tmp * 3;
    hw_cfg->delta_qp[0] = -3;
    hw_cfg->target_error[1] = -tmp * 2;
    hw_cfg->delta_qp[1] = -2;
    hw_cfg->target_error[2] = -tmp * 1;
    hw_cfg->delta_qp[2] = -1;
    hw_cfg->target_error[3] = tmp * 1;
    hw_cfg->delta_qp[3] = 0;
    hw_cfg->target_error[4] = tmp * 2;
    hw_cfg->delta_qp[4] = 1;
    hw_cfg->target_error[5] = tmp * 3;
    hw_cfg->delta_qp[5] = 2;
    hw_cfg->target_error[6] = tmp * 4;
    hw_cfg->delta_qp[6] = 3;

    for (i = 0; i < CTRL_LEVELS; i++) {
        tmp =  hw_cfg->cp_target[i];
        tmp = mpp_clip(tmp / 4, -32768, 32767);
        hw_cfg->cp_target[i] = tmp;
    }
    hw_cfg->cp_distance_mbs = hw_cfg->qpCtrl.checkPointDistance;
    return 0;
}

MPP_RET h264e_vpu_mad_threshold(H264eHwCfg *hw_cfg, MppLinReg *mad, RK_U32 madCount)
{
    RK_S32 mbPerPic = (hw_cfg->width + 15) / 16 * (hw_cfg->height + 15) / 16;
    RK_U32 targetCount = 30 * mbPerPic / 100;
    RK_S32 threshold = hw_cfg->mad_threshold;
    RK_S32 lowLimit, highLimit;

    mpp_save_regdata(mad, hw_cfg->mad_threshold, madCount);
    mpp_linreg_update(mad);
//    mpp_log("hw_cfg->mad_threshold = %d",hw_cfg->mad_threshold);
    /* Calculate new threshold for next frame using either linear regression
     * model or adjustment based on current setting */
    if (mad->a)
        threshold = mad->a * targetCount / 32 + mad->b;
    else if (madCount < targetCount)
        threshold = MPP_MAX(hw_cfg->mad_threshold * 5 / 4, hw_cfg->mad_threshold + 256);
    else
        threshold = MPP_MIN(hw_cfg->mad_threshold * 3 / 4, hw_cfg->mad_threshold - 256);

    /* For small count, ensure that we increase the threshold minimum 1 step */
    if (madCount < targetCount / 2)
        threshold = MPP_MAX(threshold, hw_cfg->mad_threshold + 256);

    /* If previous frame had zero count, ensure that we increase threshold */
    if (!madCount)
        threshold = MPP_MAX(threshold, hw_cfg->mad_threshold + 256 * 4);

    /* Limit how much the threshold can change between two frames */
    lowLimit = hw_cfg->mad_threshold / 2;
    highLimit = MPP_MAX(hw_cfg->mad_threshold * 2, 256 * 4);
    hw_cfg->mad_threshold = MPP_MIN(highLimit, MPP_MAX(lowLimit, threshold));

    /* threshold_div256 has 6-bits range [0,63] */
    hw_cfg->mad_threshold = ((hw_cfg->mad_threshold + 128) / 256) * 256;
    hw_cfg->mad_threshold = MPP_MAX(0, MPP_MIN(63 * 256, hw_cfg->mad_threshold));
    return 0;
}

MPP_RET h264e_vpu_update_hw_cfg(H264eHalContext *ctx, HalEncTask *task,
                                H264eHwCfg *hw_cfg)
{
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    RcSyntax *rc_syn = (RcSyntax *)task->syntax.data;

    /* preprocess setup */
    if (prep->change) {
        RK_U32 change = prep->change;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            hw_cfg->width   = prep->width;
            hw_cfg->height  = prep->height;

            hw_cfg->hor_stride = prep->hor_stride;
            hw_cfg->ver_stride = prep->ver_stride;
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            hw_cfg->input_format = prep->format;
            h264e_vpu_set_format(hw_cfg, prep);
            switch (prep->color) {
            case MPP_FRAME_SPC_RGB : {
                /* BT.601 */
                /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
                 * Cb = 0.5647 (B - Y) + 128
                 * Cr = 0.7132 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            case MPP_FRAME_SPC_BT709 : {
                /* BT.709 */
                /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
                 * Cb = 0.5389 (B - Y) + 128
                 * Cr = 0.6350 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 13933;
                hw_cfg->color_conversion_coeff_b = 46871;
                hw_cfg->color_conversion_coeff_c = 4732;
                hw_cfg->color_conversion_coeff_e = 35317;
                hw_cfg->color_conversion_coeff_f = 41615;
            } break;
            default : {
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            }
        }

        prep->change = 0;
    }

    if (codec->change) {
        // TODO: setup sps / pps here
        hw_cfg->idr_pic_id = !ctx->idr_pic_id;
        hw_cfg->filter_disable = codec->deblock_disable;
        hw_cfg->slice_alpha_offset = codec->deblock_offset_alpha;
        hw_cfg->slice_beta_offset = codec->deblock_offset_beta;
        hw_cfg->inter4x4_disabled = (codec->profile >= 31) ? (1) : (0);
        hw_cfg->cabac_init_idc = codec->cabac_init_idc;
        hw_cfg->qp = codec->qp_init;

        hw_cfg->qp_prev = hw_cfg->qp;

        codec->change = 0;
    }
    if (NULL == ctx->intra_qs)
        mpp_linreg_init(&ctx->intra_qs, MPP_MIN(rc->gop, 10), 2);
    if (NULL == ctx->inter_qs)
        mpp_linreg_init(&ctx->inter_qs, MPP_MIN(rc->gop, 10), 2);

    if (NULL == ctx->mad)
        mpp_linreg_init(&ctx->mad, 5, 1);

    if (NULL == ctx->qp_p)
        mpp_data_init(&ctx->qp_p, MPP_MIN(rc->gop, 10));

    mpp_assert(ctx->intra_qs);
    mpp_assert(ctx->inter_qs);
    if (rc_syn->type == INTRA_FRAME) {
        hw_cfg->frame_type = H264E_VPU_FRAME_I;
        hw_cfg->frame_num = 0;
    } else {
        hw_cfg->frame_type = H264E_VPU_FRAME_P;
    }

    hw_cfg->keyframe_max_interval = rc->gop;
    hw_cfg->qp_min = codec->qp_min;
    hw_cfg->qp_max = codec->qp_max;

    if (rc->rc_mode == MPP_ENC_RC_MODE_VBR &&
        rc->quality == MPP_ENC_RC_QUALITY_CQP) {
        hw_cfg->qp = codec->qp_init;
    } else {
        /* enable mb rate control*/
        h264e_vpu_mb_rc_cfg(ctx, rc_syn, hw_cfg);
    }
    /* slice mode setup */
    hw_cfg->slice_size_mb_rows = 0; //(prep->height + 15) >> 4;

    /* input and preprocess config, the offset is at [31:10] */
    hw_cfg->input_luma_addr = mpp_buffer_get_fd(task->input);

    switch (prep->format) {
    case MPP_FMT_YUV420SP: {
        RK_U32 offset_uv = hw_cfg->hor_stride * hw_cfg->ver_stride;

        mpp_assert(prep->hor_stride == MPP_ALIGN(prep->width, 8));
        mpp_assert(prep->ver_stride == MPP_ALIGN(prep->height, 8));

        hw_cfg->input_cb_addr = hw_cfg->input_luma_addr + (offset_uv << 10);
        hw_cfg->input_cr_addr = 0;
        break;
    }
    case MPP_FMT_YUV420P: {
        RK_U32 offset_y = hw_cfg->hor_stride * hw_cfg->ver_stride;

        mpp_assert(prep->hor_stride == MPP_ALIGN(prep->width, 8));
        mpp_assert(prep->ver_stride == MPP_ALIGN(prep->height, 8));

        hw_cfg->input_cb_addr = hw_cfg->input_luma_addr + (offset_y << 10);
        hw_cfg->input_cr_addr = hw_cfg->input_cb_addr + (offset_y << 8);
        break;
    }
    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR444:
    case MPP_FMT_BGR888:
    case MPP_FMT_RGB888:
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGR101010:
        hw_cfg->input_cb_addr = 0;
        hw_cfg->input_cr_addr = 0;
        break;
    default: {
        mpp_err_f("invalid input format %d", prep->format);
        return MPP_ERR_VALUE;
    }
    }
    hw_cfg->output_strm_addr = mpp_buffer_get_fd(task->output);
    hw_cfg->output_strm_limit_size = mpp_buffer_get_size(task->output);

    /* context update */
    ctx->idr_pic_id = !ctx->idr_pic_id;
    return MPP_OK;
}
