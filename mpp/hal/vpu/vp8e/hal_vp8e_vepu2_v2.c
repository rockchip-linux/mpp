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

#define MODULE_TAG "hal_vp8e_vepu2_v2"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_enc_hal.h"
#include "hal_vp8e_base.h"
#include "hal_vp8e_vepu2_v2.h"
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
    mpp_dev_set_reg_offset(ctx->dev, 77, hw_cfg->output_strm_offset);
    regs->sw78.base_control =  hw_cfg->size_tbl_base;
    regs->sw74.nal_size_write =  hw_cfg->size_tbl_base != 0;
    regs->sw109.mv_write =  hw_cfg->mv_output_base != 0;

    regs->sw56.base_ref_lum = hw_cfg->internal_img_lum_base_r[0];
    regs->sw57.base_ref_chr = hw_cfg->internal_img_chr_base_r[0];
    regs->sw63.base_rec_lum = hw_cfg->internal_img_lum_base_w;
    regs->sw64.base_rec_chr = hw_cfg->internal_img_chr_base_w;

    regs->sw48.base_in_lum = hw_cfg->input_lum_base;
    regs->sw49.base_in_cb = hw_cfg->input_cb_base;
    mpp_dev_set_reg_offset(ctx->dev, 49, hw_cfg->input_cb_offset);
    regs->sw50.base_in_cr = hw_cfg->input_cr_base;
    mpp_dev_set_reg_offset(ctx->dev, 50, hw_cfg->input_cr_offset);

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
    regs->sw106.base_ref_chr2 =  hw_cfg->internal_img_chr_base_r[1];

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
    regs->sw96.rgb_coeff_c =  hw_cfg->rgb_coeff_c;
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
    mpp_dev_set_reg_offset(ctx->dev, 44, hw_cfg->partition_offset[0]);
    regs->sw45.base_partition2 =  hw_cfg->partition_Base[1];
    mpp_dev_set_reg_offset(ctx->dev, 45, hw_cfg->partition_offset[1]);
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

static MPP_RET hal_vp8e_vepu2_init_v2(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;

    ctx->cfg = cfg->cfg;

    /* update output to MppEnc */
    cfg->type = VPU_CLIENT_VEPU2;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }
    ctx->dev = cfg->dev;

    vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_dev_init success.\n");

    ctx->buffers = mpp_calloc(Vp8eVpuBuf, 1);
    if (ctx->buffers == NULL) {
        mpp_err("failed to malloc buffers");
        return MPP_ERR_NOMEM;
    }
    //memset(ctx->buffers, 0, sizeof(Vp8eVpuBuf));

    ctx->buffer_ready = 0;
    ctx->frame_cnt = 0;
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

    return ret;
}

static MPP_RET hal_vp8e_vepu2_deinit_v2(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    hal_vp8e_buf_free(ctx);

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->buffers);

    vp8e_hal_dbg(VP8E_DBG_HAL_FUNCTION, "mpp_dev_deinit success.\n");

    return ret;
}

static MPP_RET hal_vp8e_vepu2_gen_regs_v2(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    ctx->rc->qp_hdr = MPP_CLIP3(0, 127, task->rc_task->info.quality_target);

    if (!ctx->buffer_ready) {
        ret = hal_vp8e_setup(hal);
        if (ret) {
            hal_vp8e_vepu2_deinit_v2(hal);
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

static MPP_RET hal_vp8e_vepu2_start_v2(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    if (VP8E_DBG_HAL_DUMP_REG & vp8e_hal_debug) {
        RK_U32 i = 0;
        RK_U32 *tmp = (RK_U32 *)ctx->regs;

        for (; i < ctx->reg_size; i++)
            mpp_log("reg[%d]:%x\n", i, tmp[i]);
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = ctx->reg_size * sizeof(RK_U32);

        wr_cfg.reg = ctx->regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = ctx->regs;
        rd_cfg.size = reg_size;
        rd_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

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
    hw_cfg->split_penalty[0] = 0;
    hw_cfg->split_penalty[1] = 0;
    hw_cfg->split_penalty[3] = 0;
}

static MPP_RET hal_vp8e_vepu2_wait_v2(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eFeedback *fb = &ctx->feedback;
    Vp8eVepu2Reg_t *regs = (Vp8eVepu2Reg_t *) ctx->regs;

    if (NULL == ctx->dev) {
        mpp_err_f("invalid dev ctx\n");
        return MPP_NOK;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    fb->hw_status = regs->sw109.val & HW_STATUS_MASK;
    if (regs->sw109.val & HW_STATUS_FRAME_READY)
        vp8e_update_hw_cfg(ctx);
    else if (regs->sw109.val & HW_STATUS_BUFFER_FULL)
        ctx->bitbuf[1].size = 0;

    hal_vp8e_update_buffers(ctx, task);

    ctx->last_frm_intra = task->rc_task->frm.is_intra;
    ctx->frame_cnt++;

    task->rc_task->info.bit_real = ctx->frame_size << 3;
    task->hw_length = task->length;
    return ret;
}

static MPP_RET hal_vp8e_vepu2_get_task_v2(void *hal, HalEncTask *task)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eSyntax* syntax = (Vp8eSyntax*)task->syntax.data;
    //ctx->cfg = syntax->cfg;
    RK_U32 i;

    for (i = 0; i < task->syntax.number; i++) {
        if (syntax[i].type == VP8E_SYN_CFG) {
            ctx->cfg = (MppEncCfgSet*)syntax[i].data;
        }
        if (syntax[i].type == VP8E_SYN_RC) {
            ctx->rc = (Vp8eRc*) syntax[i].data;
        }
    }

    ctx->frame_type = task->rc_task->frm.is_intra ? VP8E_FRM_KEY : VP8E_FRM_P;
    return MPP_OK;
}

static MPP_RET hal_vp8e_vepu2_ret_task_v2(void *hal, HalEncTask *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}

const MppEncHalApi hal_vp8e_vepu2 = {
    .name       = "hal_vp8e_vepu2",
    .coding     = MPP_VIDEO_CodingVP8,
    .ctx_size   = sizeof(HalVp8eCtx),
    .flag       = 0,
    .init       = hal_vp8e_vepu2_init_v2,
    .deinit     = hal_vp8e_vepu2_deinit_v2,
    .prepare    = NULL,
    .get_task   = hal_vp8e_vepu2_get_task_v2,
    .gen_regs   = hal_vp8e_vepu2_gen_regs_v2,
    .start      = hal_vp8e_vepu2_start_v2,
    .wait       = hal_vp8e_vepu2_wait_v2,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_vp8e_vepu2_ret_task_v2,
};
