/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_h264e_vepu1"
#include <string.h>

#include "mpp_device.h"

#include "mpp_rc.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "mpp_common.h"

#include "hal_h264e_com.h"
#include "hal_h264e_header.h"
#include "hal_h264e_vpu_tbl.h"

#include "hal_h264e_vepu.h"
#include "hal_h264e_rc.h"
#include "hal_h264e_vepu1.h"
#include "hal_h264e_vepu1_reg_tbl.h"

static RK_U32 hal_vpu_h264e_debug = 0;

MPP_RET hal_h264e_vepu1_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    ctx->int_cb = cfg->hal_int_cb;
    ctx->regs = mpp_calloc(h264e_vepu1_reg_set, 1);
    ctx->buffers = mpp_calloc(h264e_hal_vpu_buffers, 1);
    ctx->extra_info = mpp_calloc(H264eVpuExtraInfo, 1);
    ctx->param_buf  = mpp_calloc_size(void, H264E_EXTRA_INFO_BUF_SIZE);
    mpp_packet_init(&ctx->packeted_param, ctx->param_buf,
                    H264E_EXTRA_INFO_BUF_SIZE);

    h264e_vpu_init_extra_info(ctx->extra_info);

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,            /* type */
        .coding = MPP_VIDEO_CodingAVC,  /* coding */
        .platform = 0,                  /* platform */
        .pp_enable = 0,                 /* pp_enable */
    };

    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }

    ctx->hw_cfg.qp_prev = ctx->cfg->codec.h264.qp_init;
    mpp_env_get_u32("hal_vpu_h264e_debug", &hal_vpu_h264e_debug, 0);

    ret = h264e_vpu_allocate_buffers(ctx);
    if (ret != MPP_OK) {
        h264e_hal_err("allocate buffers failed\n");
        h264e_vpu_free_buffers(ctx);
    }

    h264e_hal_leave();
    return ret;
}

MPP_RET hal_h264e_vepu1_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    MPP_FREE(ctx->regs);

    if (ctx->extra_info) {
        h264e_vpu_deinit_extra_info(ctx->extra_info);
        MPP_FREE(ctx->extra_info);
    }

    if (ctx->packeted_param) {
        mpp_packet_deinit(&ctx->packeted_param);
        ctx->packeted_param = NULL;
    }

    if (ctx->buffers) {
        h264e_vpu_free_buffers(ctx);
        MPP_FREE(ctx->buffers);
    }

    if (ctx->param_buf) {
        mpp_free(ctx->param_buf);
        ctx->param_buf = NULL;
    }

    if (ctx->inter_qs) {
        mpp_linreg_deinit(ctx->inter_qs);
        ctx->inter_qs = NULL;
    }

    if (ctx->intra_qs) {
        mpp_linreg_deinit(ctx->intra_qs);
        ctx->intra_qs = NULL;
    }

    if (ctx->mad) {
        mpp_linreg_deinit(ctx->mad);
        ctx->mad = NULL;
    }

    if (ctx->qp_p) {
        mpp_data_deinit(ctx->qp_p);
        ctx->qp_p = NULL;
    }

    ret = mpp_device_deinit(ctx->dev_ctx);
    if (ret)
        mpp_err("mpp_device_deinit failed ret: %d", ret);

    h264e_hal_leave();
    return ret;
}

MPP_RET hal_h264e_vepu1_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32 scaler = 0, i = 0;
    RK_U32 val = 0, skip_penalty = 0;
    RK_U32 overfill_r = 0, overfill_b = 0;
    RK_U32 first_free_bit = 0, constrained_intra_prediction = 0;
    RK_U8 dmv_penalty[128] = {0};
    RK_U8 dmv_qpel_penalty[128] = {0};
    RK_U32 diff_mv_penalty[3] = {0};

    H264eHalContext *ctx = (H264eHalContext *)hal;
    RK_U32 *reg = (RK_U32 *)ctx->regs;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    H264eVpuExtraInfo *extra_info =
        (H264eVpuExtraInfo *)ctx->extra_info;
    H264ePps *pps = &extra_info->pps;
    HalEncTask *enc_task = &task->enc;
    h264e_hal_vpu_buffers *bufs = (h264e_hal_vpu_buffers *)ctx->buffers;

    RK_U32 mbs_in_row = 0;
    RK_U32 mbs_in_col = 0;

    h264e_hal_enter();
    h264e_vpu_update_hw_cfg(ctx, enc_task, hw_cfg);
    h264e_vpu_update_buffers(ctx, hw_cfg);

    mbs_in_row = (prep->width + 15) / 16;
    mbs_in_col = (prep->height + 15) / 16;

    memset(reg, 0, sizeof(h264e_vepu1_reg_set));

    h264e_hal_dbg(H264E_DBG_DETAIL, "frame %d generate regs now", ctx->frame_cnt);

    /* If frame encode type for current frame is intra, write sps pps to
       the output buffer */
    val = mpp_buffer_get_size(enc_task->output);
    val >>= 3;
    val &= ~7;
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_BUF_LIMIT, val);

    /*
     * The hardware needs only the value for luma plane, because
     * values of other planes are calculated internally based on
     * format setting.
     */
    val = VEPU_REG_INTRA_AREA_TOP(mbs_in_col)
          | VEPU_REG_INTRA_AREA_BOTTOM(mbs_in_col)
          | VEPU_REG_INTRA_AREA_LEFT(mbs_in_row)
          | VEPU_REG_INTRA_AREA_RIGHT(mbs_in_row);
    H264E_HAL_SET_REG(reg, VEPU_REG_INTRA_AREA_CTRL, val); //FIXED
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_HDR_REM_MSB, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_HDR_REM_LSB, 0);

    val = VEPU_REG_AXI_CTRL_WRITE_ID(0)
          | VEPU_REG_AXI_CTRL_READ_ID(0)
          | VEPU_REG_OUTPUT_SWAP16
          | VEPU_REG_INPUT_SWAP16
          | VEPU_REG_AXI_CTRL_BURST_LEN(16)
          | VEPU_REG_OUTPUT_SWAP32
          | VEPU_REG_INPUT_SWAP32
          | VEPU_REG_OUTPUT_SWAP8
          | VEPU_REG_INPUT_SWAP8;
    H264E_HAL_SET_REG(reg, VEPU_REG_AXI_CTRL, val);

    val = VEPU_REG_MAD_QP_ADJUSTMENT (hw_cfg->mad_qp_delta)
          | VEPU_REG_MAD_THRESHOLD(hw_cfg->mad_threshold);
    H264E_HAL_SET_REG(reg, VEPU_REG_MAD_CTRL, val);

    val = 0;
    if (mbs_in_row * mbs_in_col > 3600)
        val = VEPU_REG_DISABLE_QUARTER_PIXEL_MV;
    val |= VEPU_REG_CABAC_INIT_IDC(hw_cfg->cabac_init_idc);
    if (pps->b_cabac)
        val |= VEPU_REG_ENTROPY_CODING_MODE;
    if (pps->b_transform_8x8_mode)
        val |= VEPU_REG_H264_TRANS8X8_MODE;
    if (hw_cfg->inter4x4_disabled)
        val |= VEPU_REG_H264_INTER4X4_MODE;
    /*reg |= VEPU_REG_H264_STREAM_MODE;*/
    val |= VEPU_REG_H264_SLICE_SIZE(hw_cfg->slice_size_mb_rows)
           | VEPU_REG_INTRA16X16_MODE(h264_intra16_favor[hw_cfg->qp]);

    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL2, val);

    scaler = H264E_HAL_MAX(1, 200 / (mbs_in_row + mbs_in_col));
    skip_penalty = H264E_HAL_MIN(255, h264_skip_sad_penalty[hw_cfg->qp]
                                 * scaler);
    val = VEPU_REG_SKIP_MACROBLOCK_PENALTY(skip_penalty)
          | VEPU_REG_INTER_MODE(h264_inter_favor[hw_cfg->qp]);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL_4, val);

    val = VEPU_REG_STREAM_START_OFFSET(first_free_bit);
    H264E_HAL_SET_REG(reg, VEPU_REG_RLC_CTRL, val);

    if (prep->width & 0x0f)
        overfill_r = (16 - (prep->width & 0x0f) ) / 4;
    if (prep->height & 0x0f)
        overfill_b = 16 - (prep->height & 0x0f);

    // When offset is zero row length should be total 16 aligned width
    val = VEPU_REG_IN_IMG_CHROMA_OFFSET(0)
          | VEPU_REG_IN_IMG_LUMA_OFFSET(0)
          | VEPU_REG_IN_IMG_CTRL_ROW_LEN(mbs_in_row * 16)
          | VEPU_REG_IN_IMG_CTRL_OVRFLR_D4(overfill_r)
          | VEPU_REG_IN_IMG_CTRL_OVRFLB(overfill_b)
          | VEPU_REG_IN_IMG_CTRL_FMT(hw_cfg->input_format)
          | VEPU_REG_IN_IMG_ROTATE_MODE(0);

    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_INPUT_IMAGE_CTRL, val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[0])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(0), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[2])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[3]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(1), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[4])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[5]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(2), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[6])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[7]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(3), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[8])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[9]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(4), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_cfg->target_error[0])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_cfg->target_error[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(0), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_cfg->target_error[2])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_cfg->target_error[3]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(1), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_cfg->target_error[4])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_cfg->target_error[5]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(2), val);

    val = VEPU_REG_CHKPT_DELTA_QP_CHK6(hw_cfg->delta_qp[6])
          | VEPU_REG_CHKPT_DELTA_QP_CHK5(hw_cfg->delta_qp[5])
          | VEPU_REG_CHKPT_DELTA_QP_CHK4(hw_cfg->delta_qp[4])
          | VEPU_REG_CHKPT_DELTA_QP_CHK3(hw_cfg->delta_qp[3])
          | VEPU_REG_CHKPT_DELTA_QP_CHK2(hw_cfg->delta_qp[2])
          | VEPU_REG_CHKPT_DELTA_QP_CHK1(hw_cfg->delta_qp[1])
          | VEPU_REG_CHKPT_DELTA_QP_CHK0(hw_cfg->delta_qp[0]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_DELTA_QP, val);

    val = VEPU_REG_PPS_INIT_QP(pps->i_pic_init_qp)
          | VEPU_REG_SLICE_FILTER_ALPHA(hw_cfg->slice_alpha_offset)
          | VEPU_REG_SLICE_FILTER_BETA(hw_cfg->slice_beta_offset)
          | VEPU_REG_CHROMA_QP_OFFSET(pps->i_chroma_qp_index_offset)
          | VEPU_REG_IDR_PIC_ID(hw_cfg->idr_pic_id);

    if (constrained_intra_prediction)
        val |= VEPU_REG_CONSTRAINED_INTRA_PREDICTION;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL0, val);

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_NEXT_PIC, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_MV_OUT, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_CABAC_TBL,
                      mpp_buffer_get_fd(bufs->hw_cabac_table_buf));

    val = VEPU_REG_ROI1_TOP_MB(mbs_in_col)
          | VEPU_REG_ROI1_BOTTOM_MB(mbs_in_col)
          | VEPU_REG_ROI1_LEFT_MB(mbs_in_row)
          | VEPU_REG_ROI1_RIGHT_MB(mbs_in_row);
    H264E_HAL_SET_REG(reg, VEPU_REG_ROI1, val);

    val = VEPU_REG_ROI2_TOP_MB(mbs_in_col)
          | VEPU_REG_ROI2_BOTTOM_MB(mbs_in_col)
          | VEPU_REG_ROI2_LEFT_MB(mbs_in_row)
          | VEPU_REG_ROI2_RIGHT_MB(mbs_in_row);
    H264E_HAL_SET_REG(reg, VEPU_REG_ROI2, val);
    H264E_HAL_SET_REG(reg, VEPU_REG_STABLILIZATION_OUTPUT, 0);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFB(hw_cfg->color_conversion_coeff_b)
          | VEPU_REG_RGB2YUV_CONVERSION_COEFA(hw_cfg->color_conversion_coeff_a);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF1, val);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFE(hw_cfg->color_conversion_coeff_e)
          | VEPU_REG_RGB2YUV_CONVERSION_COEFC(hw_cfg->color_conversion_coeff_c);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF2, val);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFF(hw_cfg->color_conversion_coeff_f)
          | VEPU_REG_RGB_MASK_B_MSB(hw_cfg->b_mask_msb)
          | VEPU_REG_RGB_MASK_G_MSB(hw_cfg->g_mask_msb)
          | VEPU_REG_RGB_MASK_R_MSB(hw_cfg->r_mask_msb);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB_MASK_MSB, val);

    diff_mv_penalty[0] = h264_diff_mv_penalty4p[hw_cfg->qp];
    diff_mv_penalty[1] = h264_diff_mv_penalty[hw_cfg->qp];
    diff_mv_penalty[2] = h264_diff_mv_penalty[hw_cfg->qp];

    val = VEPU_REG_1MV_PENALTY(diff_mv_penalty[1])
          | VEPU_REG_QMV_PENALTY(diff_mv_penalty[2])
          | VEPU_REG_4MV_PENALTY(diff_mv_penalty[0]);

    val |= VEPU_REG_SPLIT_MV_MODE_EN;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL3, val);

    val = VEPU_REG_H264_LUMA_INIT_QP(hw_cfg->qp)
          | VEPU_REG_H264_QP_MAX(hw_cfg->qp_max)
          | VEPU_REG_H264_QP_MIN(hw_cfg->qp_min)
          | VEPU_REG_H264_CHKPT_DISTANCE(hw_cfg->cp_distance_mbs);
    H264E_HAL_SET_REG(reg, VEPU_REG_QP_VAL, val);

    val = VEPU_REG_ZERO_MV_FAVOR_D2(10);
    H264E_HAL_SET_REG(reg, VEPU_REG_MVC_RELATE, val);

    val = VEPU_REG_PPS_ID(pps->i_id)
          | VEPU_REG_INTRA_PRED_MODE(h264_prev_mode_favor[hw_cfg->qp])
          | VEPU_REG_FRAME_NUM(hw_cfg->frame_num);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL1, val);

    for (i = 0; i < 128; i++) {
        dmv_penalty[i] = i;
        dmv_qpel_penalty[i] = H264E_HAL_MIN(255, exp_golomb_signed(i));
    }

    for (i = 0; i < 128; i += 4) {
        val = VEPU_REG_DMV_PENALTY_TABLE_BIT(dmv_penalty[i], 3);
        val |= VEPU_REG_DMV_PENALTY_TABLE_BIT(dmv_penalty[i + 1], 2);
        val |= VEPU_REG_DMV_PENALTY_TABLE_BIT(dmv_penalty[i + 2], 1);
        val |= VEPU_REG_DMV_PENALTY_TABLE_BIT(dmv_penalty[i + 3], 0);
        H264E_HAL_SET_REG(reg, VEPU_REG_DMV_PENALTY_TBL(i / 4), val);

        val = VEPU_REG_DMV_Q_PIXEL_PENALTY_TABLE_BIT(
                  dmv_qpel_penalty[i], 3);
        val |= VEPU_REG_DMV_Q_PIXEL_PENALTY_TABLE_BIT(
                   dmv_qpel_penalty[i + 1], 2);
        val |= VEPU_REG_DMV_Q_PIXEL_PENALTY_TABLE_BIT(
                   dmv_qpel_penalty[i + 2], 1);
        val |= VEPU_REG_DMV_Q_PIXEL_PENALTY_TABLE_BIT(
                   dmv_qpel_penalty[i + 3], 0);
        H264E_HAL_SET_REG(reg, VEPU_REG_DMV_Q_PIXEL_PENALTY_TBL(i / 4), val);
    }

    /* set buffers addr */
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_LUMA, hw_cfg->input_luma_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_CB, hw_cfg->input_cb_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_CR, hw_cfg->input_cr_addr);

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_OUTPUT_STREAM,
                      hw_cfg->output_strm_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_OUTPUT_CTRL,
                      mpp_buffer_get_fd(bufs->hw_nal_size_table_buf));

    {
        RK_U32 buf2_idx = ctx->frame_cnt % 2;
        RK_S32 recon_chroma_addr = 0, ref_chroma_addr = 0;
        RK_U32 frame_luma_size = mbs_in_col * mbs_in_row * 256;
        RK_S32 recon_luma_addr = mpp_buffer_get_fd(bufs->hw_rec_buf[buf2_idx]);
        RK_S32 ref_luma_addr = mpp_buffer_get_fd(bufs->hw_rec_buf[1 - buf2_idx]);

        recon_chroma_addr = recon_luma_addr | (frame_luma_size << 10);
        ref_chroma_addr   = ref_luma_addr   | (frame_luma_size << 10);

        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REC_LUMA, recon_luma_addr);
        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REC_CHROMA, recon_chroma_addr);
        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REF_LUMA, ref_luma_addr);
        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REF_CHROMA , ref_chroma_addr);
    }

    /* set important encode mode info */
    val = VEPU_REG_INTERRUPT_TIMEOUT_EN
          | VEPU_REG_SIZE_TABLE_PRESENT
          | VEPU_REG_MB_HEIGHT(mbs_in_col)
          | VEPU_REG_MB_WIDTH(mbs_in_row)
          | VEPU_REG_PIC_TYPE(hw_cfg->frame_type)
          | VEPU_REG_ENCODE_FORMAT(3)
          | VEPU_REG_ENCODE_ENABLE;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENCODE_CTRL, val);

    ctx->frame_cnt++;
    hw_cfg->frame_num++;

    if (hw_cfg->frame_type == H264E_VPU_FRAME_I)
        hw_cfg->idr_pic_id++;
    if (hw_cfg->idr_pic_id == 16)
        hw_cfg->idr_pic_id = 0;

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu1_start(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    (void)task;
    h264e_hal_enter();

    if (ctx->dev_ctx) {
        RK_U32 *p_regs = (RK_U32 *)ctx->regs;
        h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client is sending %d regs",
                      VEPU_H264E_VEPU1_NUM_REGS);
        if (MPP_OK != mpp_device_send_reg(ctx->dev_ctx, p_regs,
                                          VEPU_H264E_VEPU1_NUM_REGS)) {
            mpp_err("mpp_device_send_reg Failed!!!");
            return MPP_ERR_VPUHW;
        } else {
            h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_send_reg success!");
        }
    } else {
        mpp_err("invalid device ctx: %p", ctx->dev_ctx);
        return MPP_NOK;
    }

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu1_set_feedback(h264e_feedback *fb, h264e_vepu1_reg_set *reg)
{
    RK_S32 i = 0;
    RK_U32 cpt_prev = 0, overflow = 0;
    RK_U32 cpt_idx = VEPU_REG_CHECKPOINT(0) / 4;
    RK_U32 *reg_val = (RK_U32 *)reg;
    fb->hw_status = reg_val[VEPU_REG_INTERRUPT / 4];
    fb->qp_sum = VEPU_REG_QP_SUM(reg_val[VEPU_REG_MAD_CTRL / 4]);
    fb->mad_count = VEPU_REG_MB_CNT_SET(reg_val[VEPU_REG_MB_CTRL / 4]);
    fb->rlc_count = VEPU_REG_RLC_SUM_OUT(reg_val[VEPU_REG_RLC_CTRL / 4]);
    fb->out_strm_size = reg_val[VEPU_REG_STR_BUF_LIMIT / 4] / 8;
    for (i = 0; i < CHECK_POINTS_MAX; i++) {
        RK_U32 cpt = VEPU_REG_CHECKPOINT_RESULT(reg_val[cpt_idx]);
        if (cpt < cpt_prev)
            overflow += (1 << 21);
        fb->cp[i] = cpt + overflow;
        cpt_idx += (i & 1);
    }

    return MPP_OK;
}
static MPP_RET hal_h264e_vpu1_resend(H264eHalContext *ctx, RK_U32 *reg_out, RK_S32 dealt_qp)
{

    RK_U32 *p_regs = (RK_U32 *)ctx->regs;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_U32 val = 0;
    RK_S32 hw_ret = 0;
    hw_cfg->qp += dealt_qp;
    hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_min, hw_cfg->qp_max);
    val = VEPU_REG_H264_LUMA_INIT_QP(hw_cfg->qp)
          | VEPU_REG_H264_QP_MAX(hw_cfg->qp_max)
          | VEPU_REG_H264_QP_MIN(hw_cfg->qp_min)
          | VEPU_REG_H264_CHKPT_DISTANCE(hw_cfg->cp_distance_mbs);

    H264E_HAL_SET_REG(p_regs, VEPU_REG_QP_VAL, val);

    hw_ret = mpp_device_send_reg(ctx->dev_ctx, p_regs, VEPU_H264E_VEPU1_NUM_REGS);
    if (hw_ret)
        mpp_err("mpp_device_send_reg failed ret %d", hw_ret);
    else
        h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_send_reg success!");

    hw_ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)reg_out, VEPU_H264E_VEPU1_NUM_REGS);
    if (hw_ret) {
        h264e_hal_err("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }

    return MPP_OK;
}

MPP_RET hal_h264e_vepu1_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_vepu1_reg_set reg_out_tmp;
    h264e_vepu1_reg_set *reg_out = &reg_out_tmp;
    IOInterruptCB int_cb = ctx->int_cb;
    h264e_feedback *fb = &ctx->feedback;
    MppEncPrepCfg *prep = &ctx->set->prep;
    RcSyntax *rc_syn = (RcSyntax *)task->enc.syntax.data;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16)
                    * MPP_ALIGN(prep->height, 16) / 16 / 16;
    memset(reg_out, 0, sizeof(h264e_vepu1_reg_set));

    h264e_hal_enter();

    if (ctx->dev_ctx) {
        RK_S32 hw_ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)reg_out,
                                            VEPU_H264E_VEPU1_NUM_REGS);

        h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_wait_reg: ret %d\n", hw_ret);

        if (hw_ret != MPP_OK) {
            mpp_err("hardware returns error:%d", hw_ret);
            return MPP_ERR_VPUHW;
        }
    } else {
        mpp_err("invalid device ctx: %p", ctx->dev_ctx);
        return MPP_NOK;
    }

    hal_h264e_vepu1_set_feedback(fb, reg_out);
    task->enc.length = fb->out_strm_size;

    hw_cfg->qp_prev = hw_cfg->qp;
    if (rc_syn->type == INTER_P_FRAME) {
        int dealt_qp = 3;
        int cnt = 3;
        do {
            if (hw_cfg->qp < 30) {
                dealt_qp = 5;
            } else if (hw_cfg->qp < 42) {
                dealt_qp = 3;
            } else {
                dealt_qp = 2;
            }
            if ((fb->out_strm_size * 8 >  (RK_U32)rc_syn->bit_target * 3) && (hw_cfg->qp < hw_cfg->qp_max)) {
                hal_h264e_vpu1_resend(hal, (RK_U32 *)reg_out, dealt_qp);
                hal_h264e_vepu1_set_feedback(fb, reg_out);
                task->enc.length = fb->out_strm_size;
                hw_cfg->qp_prev = fb->qp_sum / num_mb;
                if ((cnt-- <= 0) || (hw_cfg->qp == hw_cfg->qp_max)) {
                    break;
                }
            } else {
                break;
            }
        } while (1);
    }
    if (int_cb.callBack) {
        RcSyntax *syn = (RcSyntax *)task->enc.syntax.data;
        RK_S32 avg_qp = fb->qp_sum / num_mb;
        RcHalResult result;
        RK_S32 i;

        mpp_assert(avg_qp >= 0);
        mpp_assert(avg_qp <= 51);
        avg_qp = hw_cfg->qp;

        result.bits = fb->out_strm_size * 8;
        result.type = syn->type;
        fb->result = &result;

        hw_cfg->qpCtrl.nonZeroCnt = fb->rlc_count;
        hw_cfg->qpCtrl.frameBitCnt = result.bits;
        if (syn->type == INTER_P_FRAME || syn->gop_mode == MPP_GOP_ALL_INTRA) {
            mpp_data_update(ctx->qp_p, avg_qp);
        }

        for (i = 0; i < CHECK_POINTS_MAX; i++) {
            hw_cfg->qpCtrl.wordCntPrev[i] = fb->cp[i];
        }

        mpp_save_regdata((syn->type == INTRA_FRAME) ?
                         (ctx->intra_qs) :
                         (ctx->inter_qs),
                         h264_q_step[avg_qp], result.bits);
        mpp_linreg_update((syn->type == INTRA_FRAME) ?
                          (ctx->intra_qs) :
                          (ctx->inter_qs));

        h264e_vpu_mad_threshold(hw_cfg, ctx->mad, fb->mad_count);

        int_cb.callBack(int_cb.opaque, fb);
    }

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_vepu1_reset(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu1_flush(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu1_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    h264e_hal_dbg(H264E_DBG_DETAIL, "hal_h264e_vpu_control cmd 0x%x, info %p", cmd_type, param);
    switch (cmd_type) {
    case MPP_ENC_SET_EXTRA_INFO: {
    } break;
    case MPP_ENC_GET_EXTRA_INFO: {
        size_t offset = 0;
        MppPacket  pkt      = ctx->packeted_param;
        MppPacket *pkt_out  = (MppPacket *)param;

        H264eVpuExtraInfo *src = (H264eVpuExtraInfo *)ctx->extra_info;
        H264eStream *sps_stream = &src->sps_stream;
        H264eStream *pps_stream = &src->pps_stream;
        H264eStream *sei_stream = &src->sei_stream;

        h264e_vpu_set_extra_info(ctx);

        mpp_packet_write(pkt, offset, sps_stream->buffer, sps_stream->byte_cnt);
        offset += sps_stream->byte_cnt;

        mpp_packet_write(pkt, offset, pps_stream->buffer, pps_stream->byte_cnt);
        offset += pps_stream->byte_cnt;

        mpp_packet_write(pkt, offset, sei_stream->buffer, sei_stream->byte_cnt);
        offset += sei_stream->byte_cnt;

        mpp_packet_set_length(pkt, offset);

        *pkt_out = pkt;
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        MppEncPrepCfg *set = &ctx->set->prep;
        RK_U32 change = set->change;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            if ((set->width < 0 || set->width > 1920) ||
                (set->height < 0 || set->height > 3840) ||
                (set->hor_stride < 0 || set->hor_stride > 3840) ||
                (set->ver_stride < 0 || set->ver_stride > 3840)) {
                mpp_err("invalid input w:h [%d:%d] [%d:%d]\n",
                        set->width, set->height,
                        set->hor_stride, set->ver_stride);
                ret = MPP_NOK;
            }
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            if ((set->format < MPP_FRAME_FMT_RGB &&
                 set->format >= MPP_FMT_YUV_BUTT) ||
                set->format >= MPP_FMT_RGB_BUTT) {
                mpp_err("invalid format %d\n", set->format);
                ret = MPP_NOK;
            }
        }
    } break;
    case MPP_ENC_SET_RC_CFG : {
        // TODO: do rate control check here
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncH264Cfg *src = &ctx->set->codec.h264;
        MppEncH264Cfg *dst = &ctx->cfg->codec.h264;
        RK_U32 change = src->change;

        // TODO: do codec check first

        if (change & MPP_ENC_H264_CFG_STREAM_TYPE)
            dst->stream_type = src->stream_type;
        if (change & MPP_ENC_H264_CFG_CHANGE_PROFILE) {
            dst->profile = src->profile;
            dst->level = src->level;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_ENTROPY) {
            dst->entropy_coding_mode = src->entropy_coding_mode;
            dst->cabac_init_idc = src->cabac_init_idc;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_TRANS_8x8)
            dst->transform8x8_mode = src->transform8x8_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CONST_INTRA)
            dst->constrained_intra_pred_mode = src->constrained_intra_pred_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CHROMA_QP) {
            dst->chroma_cb_qp_offset = src->chroma_cb_qp_offset;
            dst->chroma_cr_qp_offset = src->chroma_cr_qp_offset;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_DEBLOCKING) {
            dst->deblock_disable = src->deblock_disable;
            dst->deblock_offset_alpha = src->deblock_offset_alpha;
            dst->deblock_offset_beta = src->deblock_offset_beta;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_LONG_TERM)
            dst->use_longterm = src->use_longterm;
        if (change & MPP_ENC_H264_CFG_CHANGE_QP_LIMIT) {
            dst->qp_init = src->qp_init;
            dst->qp_max = src->qp_max;
            dst->qp_min = src->qp_min;
            dst->qp_max_step = src->qp_max_step;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH) {
            dst->intra_refresh_mode = src->intra_refresh_mode;
            dst->intra_refresh_arg = src->intra_refresh_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SLICE_MODE) {
            dst->slice_mode = src->slice_mode;
            dst->slice_arg = src->slice_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_VUI) {
            dst->vui = src->vui;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SEI) {
            dst->sei = src->sei;
        }

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    } break;
    case MPP_ENC_SET_OSD_PLT_CFG:
    case MPP_ENC_SET_OSD_DATA_CFG: {
        mpp_err("vepu1 do not support osd cfg\n");
        ret = MPP_NOK;
    } break;
    case MPP_ENC_SET_SEI_CFG: {
        ctx->sei_mode = *((MppEncSeiMode *)param);
    } break;
    case MPP_ENC_SET_ROI_CFG: {
        mpp_err("vepu1 do not support roi cfg\n");
        ret = MPP_NOK;
    } break;
    case MPP_ENC_PRE_ALLOC_BUFF:
        // vepu do not support prealloc buff, ignore cmd
        break;
    default : {
        mpp_err("unrecognizable cmd type %d", cmd_type);
        ret = MPP_NOK;
    } break;
    }

    h264e_hal_leave();
    return ret;
}
