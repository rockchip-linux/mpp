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

#define MODULE_TAG "hal_vp8e_vepu2"

#include <string.h>

#include "mpp_mem.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_vepu2.h"
#include "hal_vp8e_vepu2_reg.h"

#include "mpp_rc.h"
#include "vp8e_syntax.h"
#include "hal_vp8e_debug.h"

#define SWREG_AMOUNT_VEPU2  (184)
#define HW_STATUS_MASK 0x250
#define HW_STATUS_BUFFER_FULL 0x20
#define HW_STATUS_FRAME_READY 0x02

static MPP_RET vp8e_vpu_frame_start(void *hal)
{
    RK_S32 i;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    Vp8eVepu2Reg_t *regs = (Vp8eVepu2Reg_t *)ctx->regs;

    memset(regs, 0, sizeof(Vp8eVepu2Reg_t));

    regs->sw109.val = hw_cfg->irq_disable ? (regs->sw109.val | 0x0100) :
                      (regs->sw109.val & 0xfeff);

    //((0 & (255)) << 24) | ((0 & (255)) << 16) | ((16 & (63)) << 8) | ((0 & (1)) << 2) | ((0 & (1)) << 1);
    regs->sw54.val = 0x1000;

    if (hw_cfg->input_format < INPUT_RGB565) {
        regs->sw105.val = 0xfc000000;
    } else if (hw_cfg->input_format < INPUT_RGB888) {
        regs->sw105.val = 0x7c000000;
    } else {
        regs->sw105.val = 0x3c000000;
    }

    regs->sw77.base_stream =  hw_cfg->output_strm_base;

    regs->sw78.base_control =  hw_cfg->size_tbl_base;
    regs->sw74.nal_size_write =  hw_cfg->size_tbl_base != 0;
    regs->sw109.mv_write =  hw_cfg->mv_output_base != 0;

    regs->sw56.base_ref_lum = hw_cfg->internal_img_lum_base_r[0];
    regs->sw57.base_ref_chr = hw_cfg->internal_img_chr_base_r[0];
    regs->sw63.base_rec_lum = hw_cfg->internal_img_lum_base_w;
    regs->sw64.base_rec_chr = hw_cfg->internal_img_chr_base_w;

    regs->sw48.base_in_lum = hw_cfg->input_lum_base;
    regs->sw49.base_in_cb = hw_cfg->input_cb_base;
    regs->sw50.base_in_cr = hw_cfg->input_cr_base;

//    regs->sw109.int_timeout =  1 & 1;
    regs->sw109.val |= 0x0400;
    regs->sw109.int_slice_ready =  hw_cfg->int_slice_ready;
    regs->sw109.rec_write_disable =  hw_cfg->rec_write_disable;
    regs->sw103.width =  hw_cfg->mbs_in_row;
    regs->sw103.height =  hw_cfg->mbs_in_col;
    regs->sw103.picture_type =  hw_cfg->frame_coding_type;
    regs->sw103.encoding_mode =  hw_cfg->coding_type;

    regs->sw61.chr_offset = hw_cfg->input_chroma_base_offset;
    regs->sw61.lum_offset = hw_cfg->input_luma_base_offset;
    regs->sw61.row_length = hw_cfg->pixels_on_row;
    regs->sw60.x_fill = hw_cfg->x_fill;
    regs->sw60.y_fill = hw_cfg->y_fill;
    regs->sw74.input_format = hw_cfg->input_format;
    regs->sw74.input_rot = hw_cfg->input_rotation;

    regs->sw59.cabac_enable = hw_cfg->enable_cabac;
    regs->sw75.ip_intra_16_favor = hw_cfg->intra_16_favor;
    regs->sw75.inter_favor = hw_cfg->inter_favor;
    regs->sw59.disable_qp_mv = hw_cfg->disable_qp_mv;
    regs->sw59.deblocking = hw_cfg->filter_disable;
    regs->sw60.skip_penalty = hw_cfg->skip_penalty;
    regs->sw99.split_mv = hw_cfg->split_mv_mode;
    regs->sw107.split_penalty_16x8 = hw_cfg->split_penalty[0];
    regs->sw107.split_penalty_8x8 = hw_cfg->split_penalty[1];
    regs->sw107.split_penalty_8x4 = hw_cfg->split_penalty[2];
    regs->sw102.split_penalty_4x4 = hw_cfg->split_penalty[3];
    regs->sw102.zero_mv_favor = hw_cfg->zero_mv_favor;

    regs->sw51.strm_hdr_rem1 =  hw_cfg->strm_start_msb;
    regs->sw52.strm_hdr_rem2 =  hw_cfg->strm_start_lsb;
    regs->sw53.strm_buf_limit =  hw_cfg->output_strm_size;

    regs->sw76.base_ref_lum2 =  hw_cfg->internal_img_lum_base_r[1];
    regs->sw106.base_ref_chr2 =  hw_cfg->internal_img_lum_base_r[1];

    regs->sw100.y1_quant_dc =  hw_cfg->y1_quant_dc[0];
    regs->sw65.y1_quant_ac =  hw_cfg->y1_quant_ac[0];
    regs->sw66.y2_quant_dc =  hw_cfg->y2_quant_dc[0];
    regs->sw67.y2_quant_ac =  hw_cfg->y2_quant_ac[0];
    regs->sw68.ch_quant_dc =  hw_cfg->ch_quant_dc[0];
    regs->sw69.ch_quant_ac =  hw_cfg->ch_quant_ac[0];

    regs->sw100.y1_zbin_dc =  hw_cfg->y1_zbin_dc[0];
    regs->sw65.y1_zbin_ac =  hw_cfg->y1_zbin_ac[0];
    regs->sw66.y2_zbin_dc =  hw_cfg->y2_zbin_dc[0];
    regs->sw67.y2_zbin_ac =  hw_cfg->y2_zbin_ac[0];
    regs->sw68.ch_zbin_dc =  hw_cfg->ch_zbin_dc[0];
    regs->sw69.ch_zbin_ac =  hw_cfg->ch_zbin_ac[0];

    regs->sw100.y1_round_dc =  hw_cfg->y1_round_dc[0];
    regs->sw65.y1_round_ac =  hw_cfg->y1_round_ac[0];
    regs->sw66.y2_round_dc =  hw_cfg->y2_round_dc[0];
    regs->sw67.y2_round_ac =  hw_cfg->y2_round_ac[0];
    regs->sw68.ch_round_dc =  hw_cfg->ch_round_dc[0];
    regs->sw69.ch_round_ac =  hw_cfg->ch_round_ac[0];

    regs->sw70.y1_dequant_dc =  hw_cfg->y1_dequant_dc[0];
    regs->sw70.y1_dequant_ac =  hw_cfg->y1_dequant_ac[0];
    regs->sw70.y2_dequant_dc =  hw_cfg->y2_dequant_dc[0];
    regs->sw71.y2_dequant_ac =  hw_cfg->y2_dequant_ac[0];
    regs->sw71.ch_dequant_dc =  hw_cfg->ch_dequant_dc[0];
    regs->sw71.ch_dequant_ac =  hw_cfg->ch_dequant_ac[0];

    regs->sw70.mv_ref_idx =  hw_cfg->mv_ref_idx[0];
    regs->sw71.mv_ref_idx2 =  hw_cfg->mv_ref_idx[1];
    regs->sw71.ref2_enable =  hw_cfg->ref2_enable;

    regs->sw72.bool_enc_value =  hw_cfg->bool_enc_value;
    regs->sw73.bool_enc_value_bits =  hw_cfg->bool_enc_value_bits;
    regs->sw73.bool_enc_range =  hw_cfg->bool_enc_range;

    regs->sw73.filter_level =  hw_cfg->filter_level[0];
    regs->sw73.golden_penalty =  hw_cfg->golden_penalty;
    regs->sw73.filter_sharpness =  hw_cfg->filter_sharpness;
    regs->sw73.dct_partition_count =  hw_cfg->dct_partitions;

    regs->sw60.start_offset =  hw_cfg->first_free_bit;

    regs->sw79.base_next_lum =  hw_cfg->vs_next_luma_base;
    regs->sw94.stab_mode =  hw_cfg->vs_mode;

    regs->sw99.dmv_penalty_4p =  hw_cfg->diff_mv_penalty[0];
    regs->sw99.dmv_penalty_1p =  hw_cfg->diff_mv_penalty[1];
    regs->sw99.dmv_penalty_qp =  hw_cfg->diff_mv_penalty[2];

    regs->sw81.base_cabac_ctx =  hw_cfg->cabac_tbl_base;
    regs->sw80.base_mv_write =  hw_cfg->mv_output_base;

    regs->sw95.rgb_coeff_a =  hw_cfg->rgb_coeff_a;
    regs->sw95.rgb_coeff_b =  hw_cfg->rgb_coeff_b;
    regs->sw96.rgb_coeff_c =  hw_cfg->rgb_coeff_a;
    regs->sw96.rgb_coeff_e =  hw_cfg->rgb_coeff_e;
    regs->sw97.rgb_coeff_f =  hw_cfg->rgb_coeff_f;

    regs->sw98.r_mask_msb =  hw_cfg->r_mask_msb;
    regs->sw98.g_mask_msb =  hw_cfg->g_mask_msb;
    regs->sw98.b_mask_msb =  hw_cfg->b_mask_msb;

    regs->sw47.cir_start =  hw_cfg->cir_start;
    regs->sw47.cir_interval =  hw_cfg->cir_interval;

    regs->sw46.intra_area_left =  hw_cfg->intra_area_left;
    regs->sw46.intra_area_right =  hw_cfg->intra_area_right;
    regs->sw46.intra_area_top =  hw_cfg->intra_area_top;
    regs->sw46.intra_area_bottom =  hw_cfg->intra_area_bottom   ;
    regs->sw82.roi1_left =  hw_cfg->roi1_left;
    regs->sw82.roi1_right =  hw_cfg->roi1_right;
    regs->sw82.roi1_top =  hw_cfg->roi1_top;
    regs->sw82.roi1_bottom =  hw_cfg->roi1_bottom;

    regs->sw83.roi2_left =  hw_cfg->roi2_left;
    regs->sw83.roi2_right =  hw_cfg->roi2_right;
    regs->sw83.roi2_top =  hw_cfg->roi2_top;
    regs->sw83.roi2_bottom =  hw_cfg->roi2_bottom;

    regs->sw44.base_partition1 =  hw_cfg->partition_Base[0];
    regs->sw45.base_partition2 =  hw_cfg->partition_Base[1];
    regs->sw108.base_prob_count =  hw_cfg->prob_count_base;

    regs->sw33.mode0_penalty =  hw_cfg->intra_mode_penalty[0];
    regs->sw33.mode1_penalty =  hw_cfg->intra_mode_penalty[1];
    regs->sw34.mode2_penalty =  hw_cfg->intra_mode_penalty[2];
    regs->sw34.mode3_penalty =  hw_cfg->intra_mode_penalty[3];

    for (i = 0 ; i < 5; i++) {
        regs->sw28_32[i].b_mode_0_penalty = hw_cfg->intra_b_mode_penalty[2 * i];
        regs->sw28_32[i].b_mode_1_penalty = hw_cfg->intra_b_mode_penalty[2 * i + 1];
    }

    regs->sw71.segment_enable =  hw_cfg->segment_enable;
    regs->sw71.segment_map_update =  hw_cfg->segment_map_update;
    regs->sw27.base_segment_map =  hw_cfg->segment_map_base;

    for (i = 0; i < 3; i++) {
        regs->sw0_26[0 + i * 9].num_0.y1_quant_dc = hw_cfg->y1_quant_dc[1 + i];
        regs->sw0_26[0 + i * 9].num_0.y2_quant_dc = hw_cfg->y2_quant_dc[1 + i];

        regs->sw0_26[1 + i * 9].num_1.ch_quant_dc = hw_cfg->ch_quant_dc[1 + i];
        regs->sw0_26[1 + i * 9].num_1.y1_quant_ac = hw_cfg->y1_quant_ac[1 + i];

        regs->sw0_26[2 + i * 9].num_2.y2_quant_ac = hw_cfg->y2_quant_ac[1 + i];
        regs->sw0_26[2 + i * 9].num_2.ch_quant_ac = hw_cfg->ch_quant_ac[1 + i];

        regs->sw0_26[3 + i * 9].num_3.y1_zbin_dc = hw_cfg->y1_zbin_dc[1 + i];
        regs->sw0_26[3 + i * 9].num_3.y2_zbin_dc = hw_cfg->y2_zbin_dc[1 + i];
        regs->sw0_26[3 + i * 9].num_3.ch_zbin_dc = hw_cfg->ch_zbin_dc[1 + i];

        regs->sw0_26[4 + i * 9].num_4.y1_zbin_ac = hw_cfg->y1_zbin_ac[1 + i];
        regs->sw0_26[4 + i * 9].num_4.y2_zbin_ac = hw_cfg->y2_zbin_ac[1 + i];
        regs->sw0_26[4 + i * 9].num_4.ch_zbin_ac = hw_cfg->ch_zbin_ac[1 + i];

        regs->sw0_26[5 + i * 9].num_5.y1_round_dc = hw_cfg->y1_round_dc[1 + i];
        regs->sw0_26[5 + i * 9].num_5.y2_round_dc = hw_cfg->y2_round_dc[1 + i];
        regs->sw0_26[5 + i * 9].num_5.ch_round_dc = hw_cfg->ch_round_dc[1 + i];

        regs->sw0_26[6 + i * 9].num_6.y1_round_ac = hw_cfg->y1_round_ac[1 + i];
        regs->sw0_26[6 + i * 9].num_6.y2_round_ac = hw_cfg->y2_round_ac[1 + i];
        regs->sw0_26[6 + i * 9].num_6.ch_round_ac = hw_cfg->ch_round_ac[1 + i];

        regs->sw0_26[7 + i * 9].num_7.y1_dequant_dc = hw_cfg->y1_dequant_dc[1 + i];
        regs->sw0_26[7 + i * 9].num_7.y2_dequant_dc = hw_cfg->y2_dequant_dc[1 + i];
        regs->sw0_26[7 + i * 9].num_7.ch_dequant_dc = hw_cfg->ch_dequant_dc[1 + i];
        regs->sw0_26[7 + i * 9].num_7.filter_level = hw_cfg->filter_level[1 + i];

        regs->sw0_26[8 + i * 9].num_8.y1_dequant_ac = hw_cfg->y1_dequant_ac[1 + i];
        regs->sw0_26[8 + i * 9].num_8.y2_dequant_ac = hw_cfg->y2_dequant_ac[1 + i];
        regs->sw0_26[8 + i * 9].num_8.ch_dequant_ac = hw_cfg->ch_dequant_ac[1 + i];
    }

    regs->sw40.lf_ref_delta0 =  hw_cfg->lf_ref_delta[0] & mask_7b;
    regs->sw42.lf_ref_delta1 =  hw_cfg->lf_ref_delta[1] & mask_7b;
    regs->sw42.lf_ref_delta2 =  hw_cfg->lf_ref_delta[2] & mask_7b;
    regs->sw42.lf_ref_delta3 =  hw_cfg->lf_ref_delta[3] & mask_7b;
    regs->sw40.lf_mode_delta0 =  hw_cfg->lf_mode_delta[0] & mask_7b;
    regs->sw43.lf_mode_delta1 =  hw_cfg->lf_mode_delta[1] & mask_7b;
    regs->sw43.lf_mode_delta2 =  hw_cfg->lf_mode_delta[2] & mask_7b;
    regs->sw43.lf_mode_delta3 =  hw_cfg->lf_mode_delta[3] & mask_7b;

    RK_S32 j = 0;
    for (j = 0; j < 32; j++) {
        regs->sw120_183[j].penalty_0 = hw_cfg->dmv_penalty[j * 4 + 3];
        regs->sw120_183[j].penalty_1 = hw_cfg->dmv_penalty[j * 4 + 2];
        regs->sw120_183[j].penalty_2 = hw_cfg->dmv_penalty[j * 4 + 1];
        regs->sw120_183[j].penalty_3 = hw_cfg->dmv_penalty[j * 4];

        regs->sw120_183[j + 32].penalty_0 = hw_cfg->dmv_qpel_penalty[j * 4 + 3];
        regs->sw120_183[j + 32].penalty_1 = hw_cfg->dmv_qpel_penalty[j * 4 + 2];
        regs->sw120_183[j + 32].penalty_2 = hw_cfg->dmv_qpel_penalty[j * 4 + 1];
        regs->sw120_183[j + 32].penalty_3 = hw_cfg->dmv_qpel_penalty[j * 4];
    }

    regs->sw103.enable = 0x1;

    return MPP_OK;
}

MPP_RET hal_vp8e_vepu2_init(void *hal, MppHalCfg *cfg)
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
    ctx->reg_size = SWREG_AMOUNT_VEPU2;

    hw_cfg->irq_disable = 0;

    hw_cfg->rounding_ctrl  = 0;
    hw_cfg->cp_distance_mbs = 0;
    hw_cfg->recon_img_id  = 0;
    hw_cfg->input_lum_base  = 0;
    hw_cfg->input_cb_base   = 0;
    hw_cfg->input_cr_base   = 0;

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
    } else {
        vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_device_init success.\n");
    }
    return ret;
}

MPP_RET hal_vp8e_vepu2_deinit(void *hal)
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

MPP_RET hal_vp8e_vepu2_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    if (!ctx->buffer_ready) {
        ret = hal_vp8e_setup(hal);
        if (ret) {
            hal_vp8e_vepu2_deinit(hal);
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

MPP_RET hal_vp8e_vepu2_start(void *hal, HalTaskInfo *task)
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
    Vp8eVepu2Reg_t *regs = (Vp8eVepu2Reg_t *) ctx->regs;

    hw_cfg->output_strm_base = regs->sw53.strm_buf_limit / 8;
    hw_cfg->qp_sum = regs->sw58.qp_sum * 2;
    hw_cfg->mad_count = regs->sw104.mad_count;
    hw_cfg->rlc_count = regs->sw62.rlc_sum * 3;

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

MPP_RET hal_vp8e_vepu2_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eFeedback *fb = &ctx->feedback;
    IOInterruptCB int_cb = ctx->int_cb;
    Vp8eVepu2Reg_t *regs = (Vp8eVepu2Reg_t *) ctx->regs;

    if (NULL == ctx->dev_ctx) {
        mpp_err_f("invalid dev ctx\n");
        return MPP_NOK;
    }

    ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)ctx->regs, ctx->reg_size);
    if (ret != MPP_OK) {
        mpp_err("hardware returns error:%d\n", ret);
        return ret;
    } else {
        vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_device_wait_reg success.\n");
    }

    fb->hw_status = regs->sw109.val & HW_STATUS_MASK;
    if (regs->sw109.val & HW_STATUS_FRAME_READY)
        vp8e_update_hw_cfg(ctx);
    else if (regs->sw109.val & HW_STATUS_BUFFER_FULL)
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

MPP_RET hal_vp8e_vepu2_reset(void *hal)
{
    (void)hal;

    return MPP_OK;
}

MPP_RET hal_vp8e_vepu2_flush(void *hal)
{
    (void)hal;

    return MPP_OK;
}

MPP_RET hal_vp8e_vepu2_control(void *hal, RK_S32 cmd, void *param)
{
    (void)hal;
    (void)cmd;
    (void)param;

    return MPP_OK;
}
