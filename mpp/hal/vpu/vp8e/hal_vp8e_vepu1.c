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

#define MODULE_TAG "hal_vp8e_vepu1"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_rc.h"

#include "vp8e_syntax.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_vepu1.h"
#include "hal_vp8e_vepu1_reg.h"
#include "hal_vp8e_debug.h"

#define SWREG_AMOUNT_VEPU1  (164)
#define HW_STATUS_MASK 0x58
#define HW_STATUS_BUFFER_FULL 0x20
#define HW_STATUS_FRAME_READY 0x04

static MPP_RET vp8e_vpu_frame_start(void *hal)
{
    RK_S32 i = 0;
    HalVp8eCtx *ctx = (HalVp8eCtx *) hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    Vp8eVepu1Reg_t *regs = (Vp8eVepu1Reg_t *) ctx->regs;

    memset(regs, 0, sizeof(Vp8eVepu1Reg_t));

    regs->sw1.val = hw_cfg->irq_disable ? (regs->sw1.val | 0x02) :
                    (regs->sw1.val & 0xfffffffd);


    if (hw_cfg->input_format < INPUT_RGB565)
        regs->sw2.val = 0xd00f;

    else if (hw_cfg->input_format < INPUT_RGB888)
        regs->sw2.val = 0xd00e;
    else
        regs->sw2.val = 0x900e;

    regs->sw5.base_stream = hw_cfg->output_strm_base;

    regs->sw6.base_control = hw_cfg->size_tbl_base;
    regs->sw14.nal_size_write = (hw_cfg->size_tbl_base != 0);
    regs->sw14.mv_write = (hw_cfg->mv_output_base != 0);

    regs->sw7.base_ref_lum = hw_cfg->internal_img_lum_base_r[0];
    regs->sw8.base_ref_chr = hw_cfg->internal_img_chr_base_r[0];
    regs->sw9.base_rec_lum = hw_cfg->internal_img_lum_base_w;
    regs->sw10.base_rec_chr = hw_cfg->internal_img_chr_base_w;

    regs->sw11.base_in_lum = hw_cfg->input_lum_base;
    regs->sw12.base_in_cb = hw_cfg->input_cb_base;
    regs->sw13.base_in_cr = hw_cfg->input_cr_base;

    regs->sw14.int_timeout = 1;
    regs->sw14.int_slice_ready = hw_cfg->int_slice_ready;
    regs->sw14.rec_write_disable = hw_cfg->rec_write_disable;
    regs->sw14.width = hw_cfg->mbs_in_row;
    regs->sw14.height = hw_cfg->mbs_in_col;
    regs->sw14.picture_type = hw_cfg->frame_coding_type;
    regs->sw14.encoding_mode = hw_cfg->coding_type;

    regs->sw15.chr_offset = hw_cfg->input_chroma_base_offset;
    regs->sw15.lum_offset = hw_cfg->input_luma_base_offset;
    regs->sw15.row_length = hw_cfg->pixels_on_row;
    regs->sw15.x_fill = hw_cfg->x_fill;
    regs->sw15.y_fill = hw_cfg->y_fill;
    regs->sw15.input_format = hw_cfg->input_format;
    regs->sw15.input_rot = hw_cfg->input_rotation;

    regs->sw18.cabac_enable = hw_cfg->enable_cabac;
    regs->sw18.ip_intra16_favor = hw_cfg->intra_16_favor;
    regs->sw21.inter_favor = hw_cfg->inter_favor;
    regs->sw18.disable_qp_mv = hw_cfg->disable_qp_mv;
    regs->sw18.deblocking = hw_cfg->filter_disable;
    regs->sw21.skip_penalty = hw_cfg->skip_penalty;
    regs->sw19.split_mv = hw_cfg->split_mv_mode;
    regs->sw20.split_penalty_16x8 = hw_cfg->split_penalty[0];
    regs->sw20.split_penalty_8x8 = hw_cfg->split_penalty[1];
    regs->sw20.split_penalty_8x4 = hw_cfg->split_penalty[2];
    regs->sw62.split_penalty4x4 = hw_cfg->split_penalty[3];
    regs->sw62.zero_mv_favor = hw_cfg->zero_mv_favor;

    regs->sw22.strm_hdr_rem1 = hw_cfg->strm_start_msb;
    regs->sw23.strm_hdr_rem2 = hw_cfg->strm_start_lsb;
    regs->sw24.strm_buf_limit = hw_cfg->output_strm_size;

    regs->sw16.base_ref_lum2 = hw_cfg->internal_img_lum_base_r[1];
    regs->sw17.base_ref_chr2 = hw_cfg->internal_img_chr_base_r[1];

    regs->sw27.y1_quant_dc = hw_cfg->y1_quant_dc[0];
    regs->sw28.y1_quant_ac = hw_cfg->y1_quant_ac[0];
    regs->sw29.y2_quant_dc = hw_cfg->y2_quant_dc[0];
    regs->sw30.y2_quant_ac = hw_cfg->y2_quant_ac[0];
    regs->sw31.ch_quant_dc = hw_cfg->ch_quant_dc[0];
    regs->sw32.ch_quant_ac = hw_cfg->ch_quant_ac[0];

    regs->sw27.y1_zbin_dc = hw_cfg->y1_zbin_dc[0];
    regs->sw28.y1_zbin_ac = hw_cfg->y1_zbin_ac[0];
    regs->sw29.y2_zbin_dc = hw_cfg->y2_zbin_dc[0];
    regs->sw30.y2_zbin_ac = hw_cfg->y2_zbin_ac[0];
    regs->sw31.ch_zbin_dc = hw_cfg->ch_zbin_dc[0];
    regs->sw32.ch_zbin_ac = hw_cfg->ch_zbin_ac[0];

    regs->sw27.y1_round_dc = hw_cfg->y1_round_dc[0];
    regs->sw28.y1_round_ac = hw_cfg->y1_round_ac[0];
    regs->sw29.y2_round_dc = hw_cfg->y2_round_dc[0];
    regs->sw30.y2_round_ac = hw_cfg->y2_round_ac[0];
    regs->sw31.ch_round_dc = hw_cfg->ch_round_dc[0];
    regs->sw32.ch_round_ac = hw_cfg->ch_round_ac[0];

    regs->sw33.y1_dequant_dc = hw_cfg->y1_dequant_dc[0];
    regs->sw33.y1_dequant_ac = hw_cfg->y1_dequant_ac[0];
    regs->sw33.y2_dequant_dc = hw_cfg->y2_dequant_dc[0];
    regs->sw34.y2_dequant_ac = hw_cfg->y2_dequant_ac[0];
    regs->sw34.ch_dequant_dc = hw_cfg->ch_dequant_dc[0];
    regs->sw34.ch_dequant_ac = hw_cfg->ch_dequant_ac[0];

    regs->sw33.mv_ref_idx = hw_cfg->mv_ref_idx[0];
    regs->sw34.mv_ref_idx2 = hw_cfg->mv_ref_idx[1];
    regs->sw34.ref2_enable = hw_cfg->ref2_enable;

    regs->sw35.bool_enc_value = hw_cfg->bool_enc_value;
    regs->sw36.bool_enc_value_bits = hw_cfg->bool_enc_value_bits;
    regs->sw36.bool_enc_range = hw_cfg->bool_enc_range;

    regs->sw36.filter_level = hw_cfg->filter_level[0];
    regs->sw36.golden_penalty = hw_cfg->golden_penalty;
    regs->sw36.filter_sharpness = hw_cfg->filter_sharpness;
    regs->sw36.dct_partition_count = hw_cfg->dct_partitions;

    regs->sw37.start_offset = hw_cfg->first_free_bit;

    regs->sw39.base_next_lum = hw_cfg->vs_next_luma_base;
    regs->sw40.stab_mode = hw_cfg->vs_mode;

    regs->sw19.dmv_penalty4p = hw_cfg->diff_mv_penalty[0];
    regs->sw19.dmv_penalty1p = hw_cfg->diff_mv_penalty[1];
    regs->sw19.dmv_penaltyqp = hw_cfg->diff_mv_penalty[2];

    regs->sw51.base_cabac_ctx = hw_cfg->cabac_tbl_base;
    regs->sw52.base_mv_write = hw_cfg->mv_output_base;

    regs->sw53.rgb_coeff_a = hw_cfg->rgb_coeff_a;
    regs->sw53.rgb_coeff_b = hw_cfg->rgb_coeff_b;
    regs->sw54.rgb_coeff_c = hw_cfg->rgb_coeff_a;
    regs->sw54.rgb_coeff_e = hw_cfg->rgb_coeff_e;
    regs->sw55.rgb_coeff_f = hw_cfg->rgb_coeff_f;

    regs->sw55.r_mask_msb = hw_cfg->r_mask_msb;
    regs->sw55.g_mask_msb = hw_cfg->g_mask_msb;
    regs->sw55.b_mask_msb = hw_cfg->b_mask_msb;

    regs->sw57.cir_start = hw_cfg->cir_start;
    regs->sw57.cir_interval = hw_cfg->cir_interval;

    regs->sw56.intra_area_left = hw_cfg->intra_area_left;
    regs->sw56.intra_area_right = hw_cfg->intra_area_right;
    regs->sw56.intra_area_top = hw_cfg->intra_area_top;
    regs->sw56.intra_area_bottom = hw_cfg->intra_area_bottom;
    regs->sw60.roi1_left = hw_cfg->roi1_left;
    regs->sw60.roi1_right = hw_cfg->roi1_right;
    regs->sw60.roi1_top = hw_cfg->roi1_top;
    regs->sw60.roi1_bottom = hw_cfg->roi1_bottom;

    regs->sw61.roi2_left = hw_cfg->roi2_left;
    regs->sw61.roi2_right = hw_cfg->roi2_right;
    regs->sw61.roi2_top = hw_cfg->roi2_top;
    regs->sw61.roi2_bottom = hw_cfg->roi2_bottom;

    regs->sw58.base_partition1 = hw_cfg->partition_Base[0];
    regs->sw59.base_partition2 = hw_cfg->partition_Base[1];
    regs->sw26.base_prob_count = hw_cfg->prob_count_base;

    regs->sw64.mode0_penalty = hw_cfg->intra_mode_penalty[0];
    regs->sw64.mode1_penalty = hw_cfg->intra_mode_penalty[1];
    regs->sw65.mode2_penalty = hw_cfg->intra_mode_penalty[2];
    regs->sw65.mode3_penalty = hw_cfg->intra_mode_penalty[3];

    for (i = 0; i < 5; i++) {
        regs->sw66_70[i].b_mode_0_penalty = hw_cfg->intra_b_mode_penalty[2 * i];
        regs->sw66_70[i].b_mode_1_penalty = hw_cfg->intra_b_mode_penalty[2 * i + 1];
    }

    regs->sw34.segment_enable = hw_cfg->segment_enable;
    regs->sw34.segment_map_update = hw_cfg->segment_map_update;
    regs->sw71.base_segment_map = hw_cfg->segment_map_base;

    for (i = 0; i < 3; i++) {
        regs->sw72_95[0 + i * 8].num_0.y1_quant_dc = hw_cfg->y1_quant_dc[1 + i];
        regs->sw72_95[0 + i * 8].num_0.y1_zbin_dc = hw_cfg->y1_zbin_dc[1 + i];
        regs->sw72_95[0 + i * 8].num_0.y1_round_dc = hw_cfg->y1_round_dc[1 + i];

        regs->sw72_95[1 + i * 8].num_1.y1_quant_ac = hw_cfg->y1_quant_ac[1 + i];
        regs->sw72_95[1 + i * 8].num_1.y1_zbin_ac = hw_cfg->y1_zbin_ac[1 + i];
        regs->sw72_95[1 + i * 8].num_1.y1_round_ac = hw_cfg->y1_round_ac[1 + i];

        regs->sw72_95[2 + i * 8].num_2.y2_quant_dc = hw_cfg->y2_quant_dc[1 + i];
        regs->sw72_95[2 + i * 8].num_2.y2_zbin_dc = hw_cfg->y2_zbin_dc[1 + i];
        regs->sw72_95[2 + i * 8].num_2.y2_round_dc = hw_cfg->y2_round_dc[1 + i];

        regs->sw72_95[3 + i * 8].num_3.y2_quant_ac = hw_cfg->y2_quant_ac[1 + i];
        regs->sw72_95[3 + i * 8].num_3.y2_zbin_ac = hw_cfg->y2_zbin_ac[1 + i];
        regs->sw72_95[3 + i * 8].num_3.y2_round_ac = hw_cfg->y2_round_ac[1 + i];

        regs->sw72_95[4 + i * 8].num_4.ch_quant_dc = hw_cfg->ch_quant_dc[1 + i];
        regs->sw72_95[4 + i * 8].num_4.ch_zbin_dc = hw_cfg->ch_zbin_dc[1 + i];
        regs->sw72_95[4 + i * 8].num_4.ch_round_dc = hw_cfg->ch_round_dc[1 + i];

        regs->sw72_95[5 + i * 8].num_5.ch_quant_ac = hw_cfg->ch_quant_ac[1 + i];
        regs->sw72_95[5 + i * 8].num_5.ch_zbin_ac = hw_cfg->ch_zbin_ac[1 + i];
        regs->sw72_95[5 + i * 8].num_5.ch_round_ac = hw_cfg->ch_round_ac[1 + i];

        regs->sw72_95[6 + i * 8].num_6.y1_dequant_dc = hw_cfg->y1_dequant_dc[1 + i];
        regs->sw72_95[6 + i * 8].num_6.y1_dequant_ac = hw_cfg->y1_dequant_ac[1 + i];
        regs->sw72_95[6 + i * 8].num_6.y2_dequant_dc = hw_cfg->y2_dequant_dc[1 + i];

        regs->sw72_95[7 + i * 8].num_7.y2_dequant_ac = hw_cfg->y2_dequant_ac[1 + i];
        regs->sw72_95[7 + i * 8].num_7.ch_dequant_dc = hw_cfg->ch_dequant_dc[1 + i];
        regs->sw72_95[7 + i * 8].num_7.ch_dequant_ac = hw_cfg->ch_dequant_ac[1 + i];
        regs->sw72_95[7 + i * 8].num_7.filter_level = hw_cfg->filter_level[1 + i];

    }

    regs->sw162.lf_ref_delta0 = hw_cfg->lf_ref_delta[0] & mask_7b;
    regs->sw162.lf_ref_delta1 = hw_cfg->lf_ref_delta[1] & mask_7b;
    regs->sw162.lf_ref_delta2 = hw_cfg->lf_ref_delta[2] & mask_7b;
    regs->sw162.lf_ref_delta3 = hw_cfg->lf_ref_delta[3] & mask_7b;
    regs->sw163.lf_mode_delta0 = hw_cfg->lf_mode_delta[0] & mask_7b;
    regs->sw163.lf_mode_delta1 = hw_cfg->lf_mode_delta[1] & mask_7b;
    regs->sw163.lf_mode_delta2 = hw_cfg->lf_mode_delta[2] & mask_7b;
    regs->sw163.lf_mode_delta3 = hw_cfg->lf_mode_delta[3] & mask_7b;

    RK_S32 j = 0;

    for (j = 0; j < 32; j++) {
        regs->sw96_127[j].penalty_0 = hw_cfg->dmv_penalty[j * 4 + 3];
        regs->sw96_127[j].penalty_1 = hw_cfg->dmv_penalty[j * 4 + 2];
        regs->sw96_127[j].penalty_2 = hw_cfg->dmv_penalty[j * 4 + 1];
        regs->sw96_127[j].penalty_3 = hw_cfg->dmv_penalty[j * 4];

        regs->sw128_159[j].qpel_penalty_0 = hw_cfg->dmv_qpel_penalty[j * 4 + 3];
        regs->sw128_159[j].qpel_penalty_1 = hw_cfg->dmv_qpel_penalty[j * 4 + 2];
        regs->sw128_159[j].qpel_penalty_2 = hw_cfg->dmv_qpel_penalty[j * 4 + 1];
        regs->sw128_159[j].qpel_penalty_3 = hw_cfg->dmv_qpel_penalty[j * 4];
    }

    regs->sw14.enable = 1;

    return MPP_OK;
}

MPP_RET hal_vp8e_vepu1_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;

    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

    ctx->int_cb = cfg->hal_int_cb;

    ctx->buffers = mpp_calloc(Vp8eVpuBuf, 1);
    if (ctx->buffers == NULL) {
        mpp_err("failed to malloc buffers");
        return MPP_ERR_NOMEM;
    }
    ctx->buffer_ready = 0;
    ctx->frame_cnt = 0;
    ctx->gop_len = 0;
    ctx->frame_type = VP8E_FRM_KEY;
    ctx->prev_frame_lost = 0;
    ctx->frame_size = 0;
    ctx->reg_size = SWREG_AMOUNT_VEPU1;

    hw_cfg->irq_disable = 0;

    hw_cfg->rounding_ctrl = 0;
    hw_cfg->cp_distance_mbs = 0;
    hw_cfg->recon_img_id = 0;
    hw_cfg->input_lum_base = 0;
    hw_cfg->input_cb_base = 0;
    hw_cfg->input_cr_base = 0;

    hal_vp8e_init_qp_table(hal);

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,            /* type */
        .coding = MPP_VIDEO_CodingVP8,  /* coding */
        .platform = 0,                  /* platform */
        .pp_enable = 0,                 /* pp_enable */
    };
    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);

    if (ret) {
        mpp_err("mpp_device_init failed ret %d\n", ret);
        return ret;
    } else {
        vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_device_init success.\n");
        return ret;
    }
}

MPP_RET hal_vp8e_vepu1_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    hal_vp8e_buf_free(ctx);

    ret = mpp_device_deinit(&ctx->dev_ctx);

    if (ret) {
        mpp_err("mpp_device_deinit failed ret: %d\n", ret);
    } else {
        vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_device_deinit success.\n");
    }

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->buffers);

    return ret;
}

MPP_RET hal_vp8e_vepu1_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    if (!ctx->buffer_ready) {
        ret = hal_vp8e_setup(hal);
        if (ret) {
            hal_vp8e_vepu1_deinit(hal);
            mpp_err("failed to init hal vp8e\n");
        } else {
            ctx->buffer_ready = 1;
        }
    }

    memset(ctx->stream_size, 0, sizeof(ctx->stream_size));

    hal_vp8e_enc_strm_code(ctx, task);
    vp8e_vpu_frame_start(ctx);

    return MPP_OK;
}

MPP_RET hal_vp8e_vepu1_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    ret = mpp_device_send_reg(ctx->dev_ctx, (RK_U32 *)ctx->regs, ctx->reg_size);
    if (ret) {
        mpp_err("failed to send regs to kernel!!!\n");
    } else {
        vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_device_send_reg success.\n");
    }

    if (VP8E_DBG_HAL_DUMP_REG & vp8e_hal_debug) {
        RK_U32 i = 0;
        RK_U32 *tmp = (RK_U32 *)ctx->regs;

        for (; i < ctx->reg_size; i++)
            mpp_log("reg[%d]:%x\n", i, tmp[i]);
    }

    (void)task;
    return ret;
}

static void vp8e_update_hw_cfg(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    Vp8eVepu1Reg_t * regs = (Vp8eVepu1Reg_t *)ctx->regs;

    hw_cfg->output_strm_base = regs->sw24.strm_buf_limit / 8;
    hw_cfg->qp_sum = regs->sw25.qp_sum * 2;
    hw_cfg->mad_count = regs->sw38.mad_count;
    hw_cfg->rlc_count = regs->sw37.rlc_sum * 4;

    hw_cfg->intra_16_favor = -1;
    hw_cfg->inter_favor = -1;
    hw_cfg->diff_mv_penalty[0] = -1;
    hw_cfg->diff_mv_penalty[1] = -1;
    hw_cfg->diff_mv_penalty[2] = -1;
    hw_cfg->skip_penalty = -1;
    hw_cfg->golden_penalty = -1;
    hw_cfg->split_penalty[0] = -1;
    hw_cfg->split_penalty[1] = -1;
    hw_cfg->split_penalty[3] = -1;

}

MPP_RET hal_vp8e_vepu1_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eFeedback *fb = &ctx->feedback;
    IOInterruptCB int_cb = ctx->int_cb;
    Vp8eVepu1Reg_t *regs = (Vp8eVepu1Reg_t *)ctx->regs;

    if (NULL == ctx->dev_ctx) {
        mpp_err_f("invalid dev ctx\n");
        return MPP_NOK;
    }

    ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)ctx->regs, ctx->reg_size);
    if (ret != MPP_OK) {
        mpp_err("hardware returns error: %d\n", ret);
        return ret;
    } else {
        vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_device_wait_reg success.\n");
    }

    fb->hw_status = regs->sw1.val & HW_STATUS_MASK;
    if (regs->sw1.val & HW_STATUS_FRAME_READY)
        vp8e_update_hw_cfg(ctx);
    else if (regs->sw1.val & HW_STATUS_BUFFER_FULL)
        ctx->bitbuf[1].size = 0;

    hal_vp8e_update_buffers(ctx, task);

    RcHalResult result;
    ctx->frame_cnt++;
    if (ctx->frame_cnt % ctx->gop_len == 0) {
        ctx->frame_type = VP8E_FRM_KEY;
        result.type = INTRA_FRAME;
    } else {
        ctx->frame_type = VP8E_FRM_P;
        result.type = INTER_P_FRAME;
    }

    result.bits = ctx->frame_size * 8;

    if (int_cb.callBack) {
        fb->result = &result;
        int_cb.callBack(int_cb.opaque, fb);
    }

    return ret;
}

MPP_RET hal_vp8e_vepu1_reset(void *hal)
{
    vp8e_hal_enter();

    vp8e_hal_leave();
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_vp8e_vepu1_flush(void *hal)
{
    vp8e_hal_enter();

    vp8e_hal_leave();
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_vp8e_vepu1_control(void *hal, RK_S32 cmd, void *param)
{
    (void)hal;
    (void)cmd;
    (void)param;

    return MPP_OK;
}
