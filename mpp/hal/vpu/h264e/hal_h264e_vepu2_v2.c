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

#define MODULE_TAG "hal_h264e_vepu2_v2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_rc.h"
#include "mpp_packet_impl.h"

#include "mpp_enc_hal.h"
#include "h264e_debug.h"
#include "h264e_sps.h"
#include "h264e_pps.h"
#include "h264e_slice.h"

#include "hal_h264e_debug.h"
#include "hal_h264e_vpu_tbl.h"
#include "hal_h264e_vepu_v2.h"
#include "hal_h264e_stream_amend.h"

#include "hal_h264e_vepu2_reg_tbl.h"

typedef struct HalH264eVepu2Ctx_t {
    MppEncCfgSet            *cfg;

    MppDev                  dev;
    RK_S32                  frame_cnt;

    /* buffers management */
    HalH264eVepuBufs        hw_bufs;

    /* preprocess config */
    HalH264eVepuPrep        hw_prep;

    /* input / recon / refer address config */
    HalH264eVepuAddr        hw_addr;
    VepuOffsetCfg           hw_offset;

    /* macroblock ratecontrol config */
    HalH264eVepuMbRc        hw_mbrc;

    /* syntax for input from enc_impl */
    RK_U32                  updated;
    H264eSps                *sps;
    H264ePps                *pps;
    H264eSlice              *slice;
    H264eFrmInfo            *frms;
    H264eReorderInfo        *reorder;
    H264eMarkingInfo        *marking;
    H264ePrefixNal          *prefix;

    /* special TSVC stream header fixup */
    HalH264eVepuStreamAmend amend;

    /* vepu2 macroblock ratecontrol context */
    HalH264eVepuMbRcCtx     rc_ctx;

    H264eVpu2RegSet         regs_set;
    H264eVpu2RegSet         regs_get;
} HalH264eVepu2Ctx;

static MPP_RET hal_h264e_vepu2_deinit_v2(void *hal)
{
    HalH264eVepu2Ctx *p = (HalH264eVepu2Ctx *)hal;

    hal_h264e_dbg_func("enter %p\n", p);

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }

    h264e_vepu_buf_deinit(&p->hw_bufs);

    if (p->rc_ctx) {
        h264e_vepu_mbrc_deinit(p->rc_ctx);
        p->rc_ctx = NULL;
    }

    h264e_vepu_stream_amend_deinit(&p->amend);

    hal_h264e_dbg_func("leave %p\n", p);

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu2_init_v2(void *hal, MppEncHalCfg *cfg)
{
    HalH264eVepu2Ctx *p = (HalH264eVepu2Ctx *)hal;
    MPP_RET ret = MPP_OK;

    hal_h264e_dbg_func("enter %p\n", p);

    p->cfg = cfg->cfg;

    /* update output to MppEnc */
    cfg->type = VPU_CLIENT_VEPU2;

    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed ret: %d\n", ret);
        goto DONE;
    }
    p->dev = cfg->dev;

    ret = h264e_vepu_buf_init(&p->hw_bufs);
    if (ret) {
        mpp_err_f("init vepu buffer failed ret: %d\n", ret);
        goto DONE;
    }

    ret = h264e_vepu_mbrc_init(&p->rc_ctx, &p->hw_mbrc);
    if (ret) {
        mpp_err_f("init mb rate control failed ret: %d\n", ret);
        goto DONE;
    }

    /* create buffer to TSVC stream */
    h264e_vepu_stream_amend_init(&p->amend);

DONE:
    if (ret)
        hal_h264e_vepu2_deinit_v2(hal);

    hal_h264e_dbg_func("leave %p\n", p);
    return ret;
}

static RK_U32 update_vepu2_syntax(HalH264eVepu2Ctx *ctx, MppSyntax *syntax)
{
    H264eSyntaxDesc *desc = syntax->data;
    RK_S32 syn_num = syntax->number;
    RK_U32 updated = 0;
    RK_S32 i;

    for (i = 0; i < syn_num; i++, desc++) {
        switch (desc->type) {
        case H264E_SYN_CFG : {
            hal_h264e_dbg_detail("update cfg");
            ctx->cfg = desc->p;
        } break;
        case H264E_SYN_SPS : {
            hal_h264e_dbg_detail("update sps");
            ctx->sps = desc->p;
        } break;
        case H264E_SYN_PPS : {
            hal_h264e_dbg_detail("update pps");
            ctx->pps = desc->p;
        } break;
        case H264E_SYN_DPB : {
            hal_h264e_dbg_detail("update dpb");
        } break;
        case H264E_SYN_SLICE : {
            hal_h264e_dbg_detail("update slice");
            ctx->slice = desc->p;
        } break;
        case H264E_SYN_FRAME : {
            hal_h264e_dbg_detail("update frames");
            ctx->frms = desc->p;
        } break;
        case H264E_SYN_PREFIX : {
            hal_h264e_dbg_detail("update prefix nal");
            ctx->prefix = desc->p;
        } break;
        default : {
            mpp_log_f("invalid syntax type %d\n", desc->type);
        } break;
        }

        updated |= SYN_TYPE_FLAG(desc->type);
    }

    return updated;
}

static MPP_RET hal_h264e_vepu2_get_task_v2(void *hal, HalEncTask *task)
{
    HalH264eVepu2Ctx *ctx = (HalH264eVepu2Ctx *)hal;
    RK_U32 updated = update_vepu2_syntax(ctx, &task->syntax);
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    HalH264eVepuPrep *hw_prep = &ctx->hw_prep;
    HalH264eVepuAddr *hw_addr = &ctx->hw_addr;
    HalH264eVepuBufs *hw_bufs = &ctx->hw_bufs;
    VepuOffsetCfg *hw_offset = &ctx->hw_offset;
    H264eFrmInfo *frms = ctx->frms;

    hal_h264e_dbg_func("enter %p\n", hal);

    if (updated & SYN_TYPE_FLAG(H264E_SYN_CFG)) {
        h264e_vepu_buf_set_frame_size(hw_bufs, prep->width, prep->height);

        /* preprocess setup */
        if (h264e_vepu_prep_setup(hw_prep, prep))
            return MPP_NOK;

        h264e_vepu_mbrc_setup(ctx->rc_ctx, ctx->cfg);
    }

    if (updated & SYN_TYPE_FLAG(H264E_SYN_SLICE)) {
        H264eSlice *slice = ctx->slice;

        h264e_vepu_buf_set_cabac_idc(hw_bufs, slice->cabac_init_idc);
    }

    h264e_vepu_prep_get_addr(hw_prep, task->input, &hw_addr->orig);

    MppBuffer recn = h264e_vepu_buf_get_frame_buffer(hw_bufs, frms->curr_idx);
    MppBuffer refr = h264e_vepu_buf_get_frame_buffer(hw_bufs, frms->refr_idx);

    hw_addr->recn[0] = mpp_buffer_get_fd(recn);
    hw_addr->refr[0] = mpp_buffer_get_fd(refr);
    hw_addr->recn[1] = hw_addr->recn[0];
    hw_addr->refr[1] = hw_addr->refr[0];

    hw_offset->fmt = prep->format;
    hw_offset->width = prep->width;
    hw_offset->height = prep->height;
    hw_offset->hor_stride = prep->hor_stride;
    hw_offset->ver_stride = prep->ver_stride;
    hw_offset->offset_x = mpp_frame_get_offset_x(task->frame);
    hw_offset->offset_y = mpp_frame_get_offset_y(task->frame);

    get_vepu_offset_cfg(hw_offset);

    h264e_vepu_stream_amend_config(&ctx->amend, task->packet, ctx->cfg,
                                   ctx->slice, ctx->prefix);

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static RK_S32 setup_output_packet(HalH264eVepu2Ctx *ctx, RK_U32 *reg, MppBuffer buf, RK_U32 offset)
{
    RK_U32 offset8 = offset & (~0x7);
    RK_S32 fd = mpp_buffer_get_fd(buf);
    RK_U32 hdr_rem_msb = 0;
    RK_U32 hdr_rem_lsb = 0;
    RK_U32 limit = 0;

    if (offset) {
        RK_U8 *buf32 = (RK_U8 *)mpp_buffer_get_ptr(buf) + offset8;

        hdr_rem_msb = MPP_RB32(buf32);
        hdr_rem_lsb = MPP_RB32(buf32 + 4);
    }

    hal_h264e_dbg_detail("offset %d offset8 %d\n", offset, offset8);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_OUTPUT_STREAM, fd);
    mpp_dev_set_reg_offset(ctx->dev, VEPU_REG_ADDR_OUTPUT_STREAM >> 2, offset8);

    /* output buffer size is 64 bit address then 8 multiple size */
    limit = mpp_buffer_get_size(buf);
    limit -= offset8;
    limit >>= 3;
    limit &= ~7;
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_BUF_LIMIT, limit);

    hal_h264e_dbg_detail("msb %08x lsb %08x", hdr_rem_msb, hdr_rem_lsb);

    H264E_HAL_SET_REG(reg, VEPU_REG_STR_HDR_REM_MSB, hdr_rem_msb);
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_HDR_REM_LSB, hdr_rem_lsb);

    return (offset - offset8) * 8;
}

static MPP_RET setup_intra_refresh(HalH264eVepu2Ctx *ctx, EncFrmStatus *frm)
{
    MPP_RET ret = MPP_OK;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 refresh_num = ctx->cfg->rc.refresh_num;
    MppEncCfgSet *cfg = ctx->cfg;
    RK_U32 *reg = ctx->regs_set.val;
    RK_U32 val = 0;
    RK_S32 top = 0;
    RK_S32 left = 0;
    RK_S32 right = 0;
    RK_S32 bottom = 0;
    RK_U32 refresh_idx = frm->seq_idx % cfg->rc.gop;

    hal_h264e_dbg_func("enter\n");

    if (!ctx->cfg->rc.refresh_en || !frm->is_i_refresh) {
        goto RET;
    }

    if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_ROW) {
        left = 0;
        right = mb_w;
        top = refresh_idx * refresh_num - 2;
        bottom = (refresh_idx + 1) * refresh_num - 1;
        top = mpp_clip(top, 0, mb_h);
        bottom = mpp_clip(bottom, 0, mb_h);
    } else if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_COL) {
        top = 0;
        bottom = mb_h;
        left = refresh_idx * refresh_num - 2;
        right = (refresh_idx + 1) * refresh_num - 1;
        left = mpp_clip(left, 0, mb_w);
        right = mpp_clip(right, 0, mb_w);
    }

RET:
    val = VEPU_REG_INTRA_AREA_TOP(top)
          | VEPU_REG_INTRA_AREA_BOTTOM(bottom)
          | VEPU_REG_INTRA_AREA_LEFT(left)
          | VEPU_REG_INTRA_AREA_RIGHT(right);
    H264E_HAL_SET_REG(reg, VEPU_REG_INTRA_AREA_CTRL, val);

    hal_h264e_dbg_func("leave, ret %d\n", ret);

    return ret;
}

static MPP_RET hal_h264e_vepu2_gen_regs_v2(void *hal, HalEncTask *task)
{
    //MPP_RET ret = MPP_OK;
    HalH264eVepu2Ctx *ctx = (HalH264eVepu2Ctx *)hal;
    HalH264eVepuBufs *hw_bufs = &ctx->hw_bufs;
    HalH264eVepuPrep *hw_prep = &ctx->hw_prep;
    HalH264eVepuAddr *hw_addr = &ctx->hw_addr;
    HalH264eVepuMbRc *hw_mbrc = &ctx->hw_mbrc;
    VepuOffsetCfg *hw_offset = &ctx->hw_offset;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    EncFrmStatus *frm = &task->rc_task->frm;
    H264eSps *sps = ctx->sps;
    H264ePps *pps = ctx->pps;
    H264eSlice *slice = ctx->slice;
    RK_U32 *reg = ctx->regs_set.val;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 offset = mpp_packet_get_length(task->packet);
    RK_U32 first_free_bit = 0;
    RK_U32 val = 0;
    RK_S32 i = 0;

    if (hw_prep->rotation) {
        mb_w = ctx->sps->pic_height_in_mbs;
        mb_h = ctx->sps->pic_width_in_mbs;
    }

    hw_mbrc->qp_init = rc_info->quality_target;
    hw_mbrc->qp_max = rc_info->quality_max;
    hw_mbrc->qp_min = rc_info->quality_min;

    hal_h264e_dbg_func("enter %p\n", hal);

    hal_h264e_dbg_detail("frame %d generate regs now", frm->seq_idx);

    // prepare mb rc config
    h264e_vepu_mbrc_prepare(ctx->rc_ctx, &ctx->hw_mbrc, task->rc_task);
    h264e_vepu_slice_split_cfg(ctx->slice, &ctx->hw_mbrc, task->rc_task, ctx->cfg);

    /* setup output address with offset */
    first_free_bit = setup_output_packet(ctx, reg, task->output, offset);

    /* set extra byte for header */
    hw_mbrc->hdr_strm_size = offset;
    hw_mbrc->hdr_free_size = first_free_bit / 8;
    hw_mbrc->out_strm_size = 0;

    /*
     * The hardware needs only the value for luma plane, because
     * values of other planes are calculated internally based on
     * format setting.
     */
    setup_intra_refresh(ctx, frm);

    val = VEPU_REG_AXI_CTRL_READ_ID(0);
    val |= VEPU_REG_AXI_CTRL_WRITE_ID(0);
    val |= VEPU_REG_AXI_CTRL_BURST_LEN(16);
    val |= VEPU_REG_AXI_CTRL_INCREMENT_MODE(0);
    val |= VEPU_REG_AXI_CTRL_BIRST_DISCARD(0);
    H264E_HAL_SET_REG(reg, VEPU_REG_AXI_CTRL, val);

    H264E_HAL_SET_REG(reg, VEPU_QP_ADJUST_MAD_DELTA_ROI,
                      hw_mbrc->mad_qp_change);

    val = 0;
    if (mb_w * mb_h > 3600)
        val = VEPU_REG_DISABLE_QUARTER_PIXEL_MV;
    val |= VEPU_REG_CABAC_INIT_IDC(slice->cabac_init_idc);
    if (pps->entropy_coding_mode)
        val |= VEPU_REG_ENTROPY_CODING_MODE;
    if (pps->transform_8x8_mode)
        val |= VEPU_REG_H264_TRANS8X8_MODE;
    if (sps->profile_idc > 31)
        val |= VEPU_REG_H264_INTER4X4_MODE;
    /*reg |= VEPU_REG_H264_STREAM_MODE;*/
    val |= VEPU_REG_H264_SLICE_SIZE(hw_mbrc->slice_size_mb_rows);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL0, val);

    RK_U32 scaler = MPP_MAX(1, 200 / (mb_w + mb_h));

    RK_U32 skip_penalty = MPP_MIN(255, h264_skip_sad_penalty[hw_mbrc->qp_init] * scaler);

    RK_U32 overfill_r = (hw_prep->src_w & 0x0f) ?
                        ((16 - (hw_prep->src_w & 0x0f)) / 4) : 0;

    RK_U32 overfill_b = (hw_prep->src_h & 0x0f) ?
                        (16 - (hw_prep->src_h & 0x0f)) : 0;

    val = VEPU_REG_STREAM_START_OFFSET(first_free_bit) |
          VEPU_REG_SKIP_MACROBLOCK_PENALTY(skip_penalty) |
          VEPU_REG_IN_IMG_CTRL_OVRFLR_D4(overfill_r) |
          VEPU_REG_IN_IMG_CTRL_OVRFLB(overfill_b);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_OVER_FILL_STRM_OFFSET, val);

    // When offset is zero row length should be total 16 aligned width
    val = VEPU_REG_IN_IMG_CHROMA_OFFSET(0)
          | VEPU_REG_IN_IMG_LUMA_OFFSET(0)
          | VEPU_REG_IN_IMG_CTRL_ROW_LEN(hw_prep->pixel_stride);

    H264E_HAL_SET_REG(reg, VEPU_REG_INPUT_LUMA_INFO, val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_mbrc->cp_target[0])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_mbrc->cp_target[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(0), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_mbrc->cp_target[2])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_mbrc->cp_target[3]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(1), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_mbrc->cp_target[4])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_mbrc->cp_target[5]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(2), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_mbrc->cp_target[6])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_mbrc->cp_target[7]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(3), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_mbrc->cp_target[8])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_mbrc->cp_target[9]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(4), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_mbrc->cp_error[0])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_mbrc->cp_error[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(0), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_mbrc->cp_error[2])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_mbrc->cp_error[3]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(1), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_mbrc->cp_error[4])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_mbrc->cp_error[5]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(2), val);

    val = VEPU_REG_CHKPT_DELTA_QP_CHK6(hw_mbrc->cp_delta_qp[6])
          | VEPU_REG_CHKPT_DELTA_QP_CHK5(hw_mbrc->cp_delta_qp[5])
          | VEPU_REG_CHKPT_DELTA_QP_CHK4(hw_mbrc->cp_delta_qp[4])
          | VEPU_REG_CHKPT_DELTA_QP_CHK3(hw_mbrc->cp_delta_qp[3])
          | VEPU_REG_CHKPT_DELTA_QP_CHK2(hw_mbrc->cp_delta_qp[2])
          | VEPU_REG_CHKPT_DELTA_QP_CHK1(hw_mbrc->cp_delta_qp[1])
          | VEPU_REG_CHKPT_DELTA_QP_CHK0(hw_mbrc->cp_delta_qp[0]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_DELTA_QP, val);

    val = VEPU_REG_MAD_THRESHOLD(hw_mbrc->mad_threshold)
          | VEPU_REG_IN_IMG_CTRL_FMT(hw_prep->src_fmt)
          | VEPU_REG_IN_IMG_ROTATE_MODE(hw_prep->rotation)
          | VEPU_REG_SIZE_TABLE_PRESENT; //FIXED
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL1, val);

    val = VEPU_REG_INTRA16X16_MODE(h264_intra16_favor[hw_mbrc->qp_init])
          | VEPU_REG_INTER_MODE(h264_inter_favor[hw_mbrc->qp_init]);
    H264E_HAL_SET_REG(reg, VEPU_REG_INTRA_INTER_MODE, val);

    val = VEPU_REG_PPS_INIT_QP(pps->pic_init_qp)
          | VEPU_REG_SLICE_FILTER_ALPHA(slice->slice_alpha_c0_offset_div2)
          | VEPU_REG_SLICE_FILTER_BETA(slice->slice_beta_offset_div2)
          | VEPU_REG_CHROMA_QP_OFFSET(pps->chroma_qp_index_offset)
          | VEPU_REG_IDR_PIC_ID(slice->idr_pic_id);

    if (slice->disable_deblocking_filter_idc)
        val |= VEPU_REG_FILTER_DISABLE;

    if (pps->constrained_intra_pred)
        val |= VEPU_REG_CONSTRAINED_INTRA_PREDICTION;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL2, val);

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_NEXT_PIC, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_MV_OUT, 0);

    MppBuffer cabac_table = hw_bufs->cabac_table;
    RK_S32 cabac_table_fd = cabac_table ? mpp_buffer_get_fd(cabac_table) : 0;

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_CABAC_TBL, cabac_table_fd);

    val = VEPU_REG_ROI1_TOP_MB(mb_h)
          | VEPU_REG_ROI1_BOTTOM_MB(mb_h)
          | VEPU_REG_ROI1_LEFT_MB(mb_w)
          | VEPU_REG_ROI1_RIGHT_MB(mb_w);
    H264E_HAL_SET_REG(reg, VEPU_REG_ROI1, val);

    val = VEPU_REG_ROI2_TOP_MB(mb_h)
          | VEPU_REG_ROI2_BOTTOM_MB(mb_h)
          | VEPU_REG_ROI2_LEFT_MB(mb_w)
          | VEPU_REG_ROI2_RIGHT_MB(mb_w);
    H264E_HAL_SET_REG(reg, VEPU_REG_ROI2, val);
    H264E_HAL_SET_REG(reg, VEPU_REG_STABLILIZATION_OUTPUT, 0);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFB(hw_prep->color_conversion_coeff_b)
          | VEPU_REG_RGB2YUV_CONVERSION_COEFA(hw_prep->color_conversion_coeff_a);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF1, val);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFE(hw_prep->color_conversion_coeff_e)
          | VEPU_REG_RGB2YUV_CONVERSION_COEFC(hw_prep->color_conversion_coeff_c);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF2, val);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFF(hw_prep->color_conversion_coeff_f);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF3, val);

    val = VEPU_REG_RGB_MASK_B_MSB(hw_prep->b_mask_msb)
          | VEPU_REG_RGB_MASK_G_MSB(hw_prep->g_mask_msb)
          | VEPU_REG_RGB_MASK_R_MSB(hw_prep->r_mask_msb);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB_MASK_MSB, val); //FIXED

    {
        RK_U32 diff_mv_penalty[3] = {0};
        diff_mv_penalty[0] = h264_diff_mv_penalty4p[hw_mbrc->qp_init];
        diff_mv_penalty[1] = h264_diff_mv_penalty[hw_mbrc->qp_init];
        diff_mv_penalty[2] = h264_diff_mv_penalty[hw_mbrc->qp_init];

        val = VEPU_REG_1MV_PENALTY(diff_mv_penalty[1])
              | VEPU_REG_QMV_PENALTY(diff_mv_penalty[2])
              | VEPU_REG_4MV_PENALTY(diff_mv_penalty[0]);
    }

    val |= VEPU_REG_SPLIT_MV_MODE_EN;
    H264E_HAL_SET_REG(reg, VEPU_REG_MV_PENALTY, val);

    val = VEPU_REG_H264_LUMA_INIT_QP(hw_mbrc->qp_init)
          | VEPU_REG_H264_QP_MAX(hw_mbrc->qp_max)
          | VEPU_REG_H264_QP_MIN(hw_mbrc->qp_min)
          | VEPU_REG_H264_CHKPT_DISTANCE(hw_mbrc->cp_distance_mbs);
    H264E_HAL_SET_REG(reg, VEPU_REG_QP_VAL, val);

    val = VEPU_REG_ZERO_MV_FAVOR_D2(10);
    H264E_HAL_SET_REG(reg, VEPU_REG_MVC_RELATE, val);

    val = VEPU_REG_OUTPUT_SWAP32
          | VEPU_REG_OUTPUT_SWAP16
          | VEPU_REG_OUTPUT_SWAP8
          | VEPU_REG_INPUT_SWAP8_(hw_prep->swap_8_in)
          | VEPU_REG_INPUT_SWAP16_(hw_prep->swap_16_in)
          | VEPU_REG_INPUT_SWAP32_(hw_prep->swap_32_in);
    H264E_HAL_SET_REG(reg, VEPU_REG_DATA_ENDIAN, val);

    val = VEPU_REG_PPS_ID(pps->pps_id)
          | VEPU_REG_INTRA_PRED_MODE(h264_prev_mode_favor[hw_mbrc->qp_init])
          | VEPU_REG_FRAME_NUM(slice->frame_num);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL3, val);

    val = VEPU_REG_INTERRUPT_TIMEOUT_EN;
    H264E_HAL_SET_REG(reg, VEPU_REG_INTERRUPT, val);

    {
        RK_U8 dmv_penalty[128] = {0};
        RK_U8 dmv_qpel_penalty[128] = {0};

        for (i = 0; i < 128; i++) {
            dmv_penalty[i] = i;
            dmv_qpel_penalty[i] = MPP_MIN(255, exp_golomb_signed(i));
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
    }

    /* set buffers addr */
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_LUMA, hw_addr->orig[0]);
    if (hw_offset->offset_byte[0])
        mpp_dev_set_reg_offset(ctx->dev, VEPU_REG_ADDR_IN_LUMA >> 2,
                               hw_offset->offset_byte[0]);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_CB, hw_addr->orig[1]);
    if (hw_offset->offset_byte[1])
        mpp_dev_set_reg_offset(ctx->dev, VEPU_REG_ADDR_IN_CB >> 2,
                               hw_offset->offset_byte[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_CR, hw_addr->orig[2]);
    if (hw_offset->offset_byte[2])
        mpp_dev_set_reg_offset(ctx->dev, VEPU_REG_ADDR_IN_CR >> 2,
                               hw_offset->offset_byte[2]);

    MppBuffer nal_size_table = h264e_vepu_buf_get_nal_size_table(hw_bufs);
    RK_S32 nal_size_table_fd = nal_size_table ? mpp_buffer_get_fd(nal_size_table) : 0;

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_OUTPUT_CTRL, nal_size_table_fd);

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REC_LUMA,   hw_addr->recn[0]);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REC_CHROMA, hw_addr->recn[1]);
    mpp_dev_set_reg_offset(ctx->dev, VEPU_REG_ADDR_REC_CHROMA >> 2, hw_bufs->yuv_size);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REF_LUMA,   hw_addr->refr[0]);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REF_CHROMA, hw_addr->refr[1]);
    mpp_dev_set_reg_offset(ctx->dev, VEPU_REG_ADDR_REF_CHROMA >> 2, hw_bufs->yuv_size);

    /* set important encode mode info */
    val = VEPU_REG_MB_HEIGHT(mb_h)
          | VEPU_REG_MB_WIDTH(mb_w)
          | VEPU_REG_PIC_TYPE(slice->idr_flag)
          | VEPU_REG_ENCODE_FORMAT(3)
          | VEPU_REG_ENCODE_ENABLE;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENCODE_START, val);

    ctx->frame_cnt++;

    hal_h264e_dbg_func("leave %p\n", hal);
    return MPP_OK;
}

static MPP_RET hal_h264e_vepu2_start_v2(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalH264eVepu2Ctx *ctx = (HalH264eVepu2Ctx *)hal;
    (void)task;

    hal_h264e_dbg_func("enter %p\n", hal);

    if (ctx->dev) {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = sizeof(ctx->regs_set);

        do {
            wr_cfg.reg = &ctx->regs_set;
            wr_cfg.size = reg_size;
            wr_cfg.offset = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
            if (ret) {
                mpp_err_f("set register write failed %d\n", ret);
                break;
            }

            rd_cfg.reg = &ctx->regs_get;
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
    } else
        mpp_err("invalid NULL device ctx\n");

    hal_h264e_dbg_func("leave %p\n", hal);

    return ret;
}

static void h264e_vepu2_get_mbrc(HalH264eVepuMbRc *mb_rc, H264eVpu2RegSet *reg)
{
    RK_S32 i = 0;
    RK_U32 cpt_prev = 0;
    RK_U32 overflow = 0;
    RK_U32 cpt_idx = VEPU_REG_CHECKPOINT(0) / 4;
    RK_U32 *reg_val = reg->val;

    mb_rc->hw_status        = reg_val[VEPU_REG_INTERRUPT / 4];
    mb_rc->out_strm_size    = reg_val[VEPU_REG_STR_BUF_LIMIT / 4] / 8 - mb_rc->hdr_free_size;
    mb_rc->qp_sum           = ((reg_val[VEPU_REG_QP_SUM_DIV2 / 4] >> 11) & 0x001fffff) * 2;
    mb_rc->less_mad_count   = (reg_val[VEPU_REG_MB_CTRL / 4] >> 16) & 0xffff;
    mb_rc->rlc_count        = reg_val[VEPU_REG_RLC_SUM / 4] & 0x3fffff;

    for (i = 0; i < VEPU_CHECK_POINTS_MAX; i++) {
        RK_U32 cpt = VEPU_REG_CHECKPOINT_RESULT(reg_val[cpt_idx]);

        if (cpt < cpt_prev)
            overflow += (1 << 21);

        cpt_prev = cpt;
        mb_rc->cp_usage[i] = cpt + overflow;
        cpt_idx += (i & 1);
    }
}

static MPP_RET hal_h264e_vepu2_wait_v2(void *hal, HalEncTask *task)
{
    HalH264eVepu2Ctx *ctx = (HalH264eVepu2Ctx *)hal;
    HalH264eVepuMbRc *hw_mbrc = &ctx->hw_mbrc;
    H264NaluType type = task->rc_task->frm.is_idr ?  H264_NALU_TYPE_IDR : H264_NALU_TYPE_SLICE;
    MppPacket pkt = task->packet;
    RK_S32 offset = mpp_packet_get_length(pkt);
    MPP_RET ret = MPP_NOK;

    hal_h264e_dbg_func("enter %p\n", hal);

    if (ctx->dev) {
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);
    } else {
        mpp_err("invalid NULL device ctx\n");
        return ret;
    }

    h264e_vepu2_get_mbrc(hw_mbrc, &ctx->regs_get);
    h264e_vepu_mbrc_update(ctx->rc_ctx, hw_mbrc);

    mpp_packet_add_segment_info(pkt, type, offset, hw_mbrc->out_strm_size);

    {
        HalH264eVepuStreamAmend *amend = &ctx->amend;

        if (amend->enable) {
            amend->old_length = hw_mbrc->out_strm_size;
            h264e_vepu_stream_amend_proc(amend, &ctx->cfg->h264.hw_cfg);
            ctx->hw_mbrc.out_strm_size = amend->new_length;
        } else if (amend->prefix) {
            /* check prefix value */
            amend->old_length = hw_mbrc->out_strm_size;
            h264e_vepu_stream_amend_sync_ref_idc(amend);
        }
    }

    task->hw_length += ctx->hw_mbrc.out_strm_size;

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu2_ret_task_v2(void *hal, HalEncTask *task)
{
    HalH264eVepu2Ctx *ctx = (HalH264eVepu2Ctx *)hal;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 mbs = mb_w * mb_h;

    hal_h264e_dbg_func("enter %p\n", hal);

    task->length += task->hw_length;

    rc_info->bit_real = task->hw_length * 8;
    rc_info->quality_real = ctx->hw_mbrc.qp_sum / mbs;

    hal_h264e_dbg_rc("real bit %d quality %d\n", rc_info->bit_real, rc_info->quality_real);

    task->hal_ret.data   = rc_info;
    task->hal_ret.number = 1;

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

const MppEncHalApi hal_h264e_vepu2 = {
    .name       = "hal_h264e_vepu2",
    .coding     = MPP_VIDEO_CodingAVC,
    .ctx_size   = sizeof(HalH264eVepu2Ctx),
    .flag       = 0,
    .init       = hal_h264e_vepu2_init_v2,
    .deinit     = hal_h264e_vepu2_deinit_v2,
    .prepare    = NULL,
    .get_task   = hal_h264e_vepu2_get_task_v2,
    .gen_regs   = hal_h264e_vepu2_gen_regs_v2,
    .start      = hal_h264e_vepu2_start_v2,
    .wait       = hal_h264e_vepu2_wait_v2,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_h264e_vepu2_ret_task_v2,
};
