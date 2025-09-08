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

#define MODULE_TAG "hal_vp8e_vepu1_v2"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_rc.h"
#include "mpp_common.h"
#include "vp8e_syntax.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_vepu1_v2.h"
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
        regs->sw2.val = 0xd00f;
    else
        regs->sw2.val = 0x900e;

    regs->sw5.base_stream = hw_cfg->output_strm_base;
    mpp_dev_set_reg_offset(ctx->dev, 5, hw_cfg->output_strm_offset);

    regs->sw6.base_control = hw_cfg->size_tbl_base;
    regs->sw14.nal_size_write = (hw_cfg->size_tbl_base != 0);
    regs->sw14.mv_write = (hw_cfg->mv_output_base != 0);

    regs->sw7.base_ref_lum = hw_cfg->internal_img_lum_base_r[0];
    regs->sw8.base_ref_chr = hw_cfg->internal_img_chr_base_r[0];
    regs->sw9.base_rec_lum = hw_cfg->internal_img_lum_base_w;
    regs->sw10.base_rec_chr = hw_cfg->internal_img_chr_base_w;

    regs->sw11.base_in_lum = hw_cfg->input_lum_base;
    if (hw_cfg->input_lum_offset)
        mpp_dev_set_reg_offset(ctx->dev, 11, hw_cfg->input_lum_offset);

    regs->sw12.base_in_cb = hw_cfg->input_cb_base;
    if (hw_cfg->input_cb_offset)
        mpp_dev_set_reg_offset(ctx->dev, 12, hw_cfg->input_cb_offset);

    regs->sw13.base_in_cr = hw_cfg->input_cr_base;
    if (hw_cfg->input_cr_offset)
        mpp_dev_set_reg_offset(ctx->dev, 13, hw_cfg->input_cr_offset);

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
    regs->sw54.rgb_coeff_c = hw_cfg->rgb_coeff_c;
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
    mpp_dev_set_reg_offset(ctx->dev, 58, hw_cfg->partition_offset[0]);
    regs->sw59.base_partition2 = hw_cfg->partition_Base[1];
    mpp_dev_set_reg_offset(ctx->dev, 59, hw_cfg->partition_offset[1]);
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

static MPP_RET hal_vp8e_vepu1_init_v2(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;

    ctx->cfg = cfg->cfg;

    /* update output to MppEnc */
    cfg->type = VPU_CLIENT_VEPU1;
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
    ctx->buffer_ready = 0;
    ctx->frame_cnt = 0;
    ctx->frame_type = VP8E_FRM_KEY;
    ctx->prev_frame_lost = 0;
    ctx->frame_size = 0;
    ctx->ivf_hdr_rdy = 0;
    ctx->reg_size = SWREG_AMOUNT_VEPU1;

    hw_cfg->irq_disable = 0;

    hw_cfg->rounding_ctrl = 0;
    hw_cfg->cp_distance_mbs = 0;
    hw_cfg->recon_img_id = 0;
    hw_cfg->input_lum_base = 0;
    hw_cfg->input_cb_base = 0;
    hw_cfg->input_cr_base = 0;

    hal_vp8e_init_qp_table(hal);

    return ret;
}

static MPP_RET hal_vp8e_vepu1_deinit_v2(void *hal)
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

static MPP_RET hal_vp8e_vepu1_gen_regs_v2(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    ctx->rc->qp_hdr = MPP_CLIP3(0, 127, task->rc_task->info.quality_target);

    if (!ctx->buffer_ready) {
        ret = hal_vp8e_setup(hal);
        if (ret) {
            mpp_err("failed to init hal vp8e\n");
            return ret;
        } else {
            ctx->buffer_ready = 1;
        }
    }

    memset(ctx->stream_size, 0, sizeof(ctx->stream_size));

    hal_vp8e_enc_strm_code(ctx, task);
    vp8e_vpu_frame_start(ctx);

    return MPP_OK;
}

static MPP_RET hal_vp8e_vepu1_start_v2(void *hal, HalEncTask *task)
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
    hw_cfg->split_penalty[0] = 0;
    hw_cfg->split_penalty[1] = 0;
    hw_cfg->split_penalty[3] = 0;

}

static MPP_RET hal_vp8e_vepu1_wait_v2(void *hal, HalEncTask *task)
{
    MPP_RET ret;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eFeedback *fb = &ctx->feedback;
    Vp8eVepu1Reg_t *regs = (Vp8eVepu1Reg_t *)ctx->regs;
    RK_S32 sw_length = task->length;

    if (NULL == ctx->dev) {
        mpp_err_f("invalid dev ctx\n");
        return MPP_NOK;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    fb->hw_status = regs->sw1.val & HW_STATUS_MASK;
    if (regs->sw1.val & HW_STATUS_FRAME_READY)
        vp8e_update_hw_cfg(ctx);
    else if (regs->sw1.val & HW_STATUS_BUFFER_FULL)
        ctx->bitbuf[1].size = 0;

    hal_vp8e_update_buffers(ctx, task);

    ctx->last_frm_intra = task->rc_task->frm.is_intra;
    ctx->frame_cnt++;

    task->rc_task->info.bit_real = ctx->frame_size << 3;
    task->hw_length = task->length - sw_length;

    return ret;
}

static MPP_RET hal_vp8e_vepu1_get_task_v2(void *hal, HalEncTask *task)
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

    if (!ctx->cfg->vp8.disable_ivf && !ctx->ivf_hdr_rdy) {
        RK_U8 *p_out = mpp_buffer_get_ptr(task->output);

        write_ivf_header(hal, p_out);
        task->length += IVF_HDR_BYTES;

        ctx->ivf_hdr_rdy = 1;
    }

    return MPP_OK;
}

static MPP_RET hal_vp8e_vepu1_ret_task_v2(void *hal, HalEncTask *task)
{
    (void)hal;
    (void)task;

    return MPP_OK;
}

const MppEncHalApi hal_vp8e_vepu1 = {
    .name       = "hal_vp8e_vepu1",
    .coding     = MPP_VIDEO_CodingVP8,
    .ctx_size   = sizeof(HalVp8eCtx),
    .flag       = 0,
    .init       = hal_vp8e_vepu1_init_v2,
    .deinit     = hal_vp8e_vepu1_deinit_v2,
    .prepare    = NULL,
    .get_task   = hal_vp8e_vepu1_get_task_v2,
    .gen_regs   = hal_vp8e_vepu1_gen_regs_v2,
    .start      = hal_vp8e_vepu1_start_v2,
    .wait       = hal_vp8e_vepu1_wait_v2,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_vp8e_vepu1_ret_task_v2,
};
