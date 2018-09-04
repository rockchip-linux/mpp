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

#define MODULE_TAG "hal_m4vd_vdpu1"

#include <stdio.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_buffer.h"
#include "mpp_common.h"

#include "mpp_device.h"
#include "mpp_dec.h"
#include "mpg4d_syntax.h"
#include "hal_mpg4d_api.h"
#include "hal_m4vd_com.h"
#include "hal_m4vd_vdpu1.h"
#include "hal_m4vd_vdpu1_reg.h"

static void vdpu1_mpg4d_setup_regs_by_syntax(hal_mpg4_ctx *ctx, MppSyntax syntax)
{
    M4vdVdpu1Regs_t *regs = ctx->regs;
    DXVA2_DecodeBufferDesc **data = syntax.data;
    DXVA_PicParams_MPEG4_PART2 *pp = NULL;
    DXVA_QmatrixData *qm = NULL;
    RK_S32 mv_buf_fd = mpp_buffer_get_fd(ctx->mv_buf);
    RK_U32 stream_length = 0;
    RK_U32 stream_used = 0;
    RK_U32 i;

    for (i = 0; i < syntax.number; i++) {
        DXVA2_DecodeBufferDesc *desc = data[i];
        switch (desc->CompressedBufferType) {
        case DXVA2_PictureParametersBufferType : {
            pp = (DXVA_PicParams_MPEG4_PART2 *)desc->pvPVPState;
        } break;
        case DXVA2_InverseQuantizationMatrixBufferType : {
            qm = (DXVA_QmatrixData *)desc->pvPVPState;
        } break;
        case DXVA2_BitStreamDateBufferType : {
            stream_length = desc->DataSize;
            stream_used = desc->DataOffset;
            ctx->bitstrm_len = stream_length;
        } break;
        default : {
            mpp_err_f("found invalid buffer descriptor type %d\n", desc->CompressedBufferType);
        } break;
        }
    }

    mpp_assert(pp);
    mpp_assert(qm);
    mpp_assert(stream_length);
    mpp_assert(stream_used);

    // copy qp table to buffer
    {
        RK_U8 *dst = (RK_U8 *)mpp_buffer_get_ptr(ctx->qp_table);
        RK_U8 *src = (qm->bNewQmatrix[0]) ? (qm->Qmatrix[0]) : (default_intra_matrix);

        memcpy(dst, src, 64);
        dst += 64;

        src = (qm->bNewQmatrix[1]) ? (qm->Qmatrix[1]) : (default_inter_matrix);
        memcpy(dst, src, 64);
    }

    regs->SwReg04.sw_pic_mb_width = (pp->vop_width  + 15) >> 4;
    regs->SwReg04.sw_pic_mb_hight_p = (pp->vop_height + 15) >> 4;

    if (pp->custorm_version == 4) {
        regs->SwReg04.sw_mb_width_off = pp->vop_width & 0xf;
        regs->SwReg04.sw_mb_height_off = pp->vop_height & 0xf;
    } else {
        regs->SwReg04.sw_mb_width_off = 0;
        regs->SwReg04.sw_mb_height_off = 0;
    }

    regs->SwReg03.sw_dec_mode = 1;

    /* note: When comparing bit 19 of reg136(that is sw_alt_scan_flag_e) with bit 6 of
    **       reg120(that is sw_alt_scan_e), we may be confused about the function of
    **       these two bits. According to C Model, just sw_alt_scan_flag_e is set,
    **       but not sw_alt_scan_e.
    */
    regs->SwReg18.sw_alt_scan_flag_e = pp->alternate_vertical_scan_flag;
    regs->SwReg48.sw_startmb_x = 0;
    regs->SwReg48.sw_startmb_y = 0;
    regs->SwReg03.sw_filtering_dis = 1;
    regs->SwReg18.sw_mpeg4_vc1_rc = pp->vop_rounding_type;
    regs->SwReg05.sw_intradc_vlc_thr = pp->intra_dc_vlc_thr;
    regs->SwReg06.sw_init_qp = pp->vop_quant;
    regs->SwReg05.sw_sync_markers_e = 1;

    {
        /*
         * update stream base address here according to consumed bit length
         * 1. hardware start address has to be 64 bit align
         * 2. hardware need to know which is the start bit in
         * 2. pass (10bit fd + (offset << 10)) register value to kernel
         */
        RK_U32 val = regs->SwReg12.sw_rlc_vlc_base;
        RK_U32 consumed_bytes = stream_used >> 3;
        RK_U32 consumed_bytes_align = consumed_bytes & (~0x7);
        RK_U32 start_bit_offset = stream_used & 0x3F;
        RK_U32 left_bytes = stream_length - consumed_bytes_align;

        val += (consumed_bytes_align << 10);
        regs->SwReg12.sw_rlc_vlc_base = val;
        regs->SwReg05.sw_strm_start_bit = start_bit_offset;
        regs->SwReg06.sw_stream_len = left_bytes;
    }
    regs->SwReg05.sw_vop_time_incr = pp->vop_time_increment_resolution;

    switch (pp->vop_coding_type) {
    case MPEG4_B_VOP : {
        RK_U32 time_bp = pp->time_bp;
        RK_U32 time_pp = pp->time_pp;

        RK_U32 trb_per_trd_d0  = MPP_DIV((((RK_S64)(1 * time_bp + 0)) << 27) + 1 * (time_pp - 1), time_pp);
        RK_U32 trb_per_trd_d1  = MPP_DIV((((RK_S64)(2 * time_bp + 1)) << 27) + 2 * (time_pp - 0), 2 * time_pp + 1);
        RK_U32 trb_per_trd_dm1 = MPP_DIV((((RK_S64)(2 * time_bp - 1)) << 27) + 2 * (time_pp - 1), 2 * time_pp - 1);

        regs->SwReg03.sw_pic_b_e = 1;
        regs->SwReg03.sw_pic_inter_e = 1;
        regs->SwReg18.sw_mpeg4_vc1_rc = 0;
        regs->SwReg14.sw_refer0_base = 1;

        mpp_assert(ctx->fd_ref1 >= 0);
        if (ctx->fd_ref1 >= 0) {
            regs->SwReg14.sw_refer0_base = (RK_U32)ctx->fd_ref1;
            regs->SwReg15.sw_refer1_base = (RK_U32)ctx->fd_ref1;
        } else {
            regs->SwReg14.sw_refer0_base = (RK_U32)ctx->fd_curr;
            regs->SwReg15.sw_refer1_base = (RK_U32)ctx->fd_curr;
        }

        mpp_assert(ctx->fd_ref0 >= 0);
        if (ctx->fd_ref0 >= 0) {
            regs->SwReg16.sw_refer2_base = (RK_U32)ctx->fd_ref0;
            regs->SwReg17.sw_refer3_base = (RK_U32)ctx->fd_ref0;
        } else {
            regs->SwReg16.sw_refer2_base = (RK_U32)ctx->fd_curr;
            regs->SwReg17.sw_refer3_base = (RK_U32)ctx->fd_curr;
        }

        regs->SwReg18.sw_fcode_fwd_hor = pp->vop_fcode_forward;
        regs->SwReg18.sw_fcode_fwd_ver = pp->vop_fcode_forward;
        regs->SwReg18.sw_fcode_bwd_hor = pp->vop_fcode_backward;
        regs->SwReg18.sw_fcode_bwd_ver = pp->vop_fcode_backward;
        regs->SwReg03.sw_write_mvs_e = 0;
        regs->SwReg41.sw_dir_mv_base = mv_buf_fd;
        regs->SwReg19.sw_refer5_base = trb_per_trd_d0;
        regs->SwReg21.sw_refer7_base = trb_per_trd_d1;
        regs->SwReg20.sw_refer6_base = trb_per_trd_dm1;
    } break;
    case MPEG4_P_VOP : {
        regs->SwReg03.sw_pic_b_e = 0;
        regs->SwReg03.sw_pic_inter_e = 1;

        if (ctx->fd_ref0 >= 0) {
            regs->SwReg14.sw_refer0_base = (RK_U32)ctx->fd_ref0;
            regs->SwReg15.sw_refer1_base = (RK_U32)ctx->fd_ref0;
        } else {
            regs->SwReg14.sw_refer0_base = (RK_U32)ctx->fd_curr;
            regs->SwReg15.sw_refer1_base = (RK_U32)ctx->fd_curr;
        }
        regs->SwReg16.sw_refer2_base = (RK_U32)ctx->fd_curr;
        regs->SwReg17.sw_refer3_base = (RK_U32)ctx->fd_curr;

        regs->SwReg18.sw_fcode_fwd_hor = pp->vop_fcode_forward;
        regs->SwReg18.sw_fcode_fwd_ver = pp->vop_fcode_forward;
        regs->SwReg03.sw_write_mvs_e = 1;
        regs->SwReg41.sw_dir_mv_base = mv_buf_fd;
    } break;
    case MPEG4_I_VOP : {
        regs->SwReg03.sw_pic_b_e = 0;
        regs->SwReg03.sw_pic_inter_e = 0;

        regs->SwReg14.sw_refer0_base = (RK_U32)ctx->fd_curr;
        regs->SwReg15.sw_refer1_base = (RK_U32)ctx->fd_curr;
        regs->SwReg16.sw_refer2_base = (RK_U32)ctx->fd_curr;
        regs->SwReg17.sw_refer3_base = (RK_U32)ctx->fd_curr;

        regs->SwReg03.sw_write_mvs_e = 0;
        regs->SwReg41.sw_dir_mv_base = mv_buf_fd;

        regs->SwReg18.sw_fcode_fwd_hor = 1;
        regs->SwReg18.sw_fcode_fwd_ver = 1;
    } break;
    default : {
        /* no nothing */
    } break;
    }

    if (pp->interlaced) {
        regs->SwReg03.sw_pic_interlace_e = 1;
        regs->SwReg03.sw_pic_fieldmode_e = 0;
        regs->SwReg04.sw_topfieldfirst_e = pp->top_field_first;
    }

    regs->SwReg18.sw_prev_anc_type = pp->prev_coding_type;
    regs->SwReg05.sw_type1_quant_e = pp->quant_type;
    regs->SwReg40.sw_qtable_base = mpp_buffer_get_fd(ctx->qp_table);
    regs->SwReg18.sw_mv_accuracy_fwd = pp->quarter_sample;

}

MPP_RET vdpu1_mpg4d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    M4vdVdpu1Regs_t *regs = NULL;
    MppBufferGroup group = NULL;
    MppBuffer mv_buf = NULL;
    MppBuffer qp_table = NULL;
    hal_mpg4_ctx *ctx = (hal_mpg4_ctx *)hal;

    mpp_assert(hal);

    ret = mpp_buffer_group_get_internal(&group, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err_f("failed to get buffer group ret %d\n", ret);
        goto ERR_RET;
    }

    ret = mpp_buffer_get(group, &mv_buf, MPEG4_MAX_MV_BUF_SIZE);
    if (ret) {
        mpp_err_f("failed to get mv buffer ret %d\n", ret);
        goto ERR_RET;
    }

    ret = mpp_buffer_get(group, &qp_table, 64 * 2 * sizeof(RK_U8));
    if (ret) {
        mpp_err_f("failed to get qp talbe buffer ret %d\n", ret);
        goto ERR_RET;
    }

    regs = mpp_calloc(M4vdVdpu1Regs_t, 1);
    if (NULL == regs) {
        mpp_err_f("failed to malloc register ret\n");
        ret = MPP_ERR_MALLOC;
        goto ERR_RET;
    }

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_DEC,              /* type */
        .coding = MPP_VIDEO_CodingMPEG4,  /* coding */
        .platform = 0,                    /* platform */
        .pp_enable = 0,                   /* pp_enable */
    };

    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err_f("mpp_device_init failed. ret: %d\n", ret);
        goto ERR_RET;
    }

    /*
     * basic register configuration setup here
     */
    regs->SwReg02.sw_dec_out_endian = 1;
    regs->SwReg02.sw_dec_in_endian = 1;
    regs->SwReg02.sw_dec_inswap32_e = 1;
    regs->SwReg02.sw_dec_outswap32_e = 1;
    regs->SwReg02.sw_dec_strswap32_e = 1;
    regs->SwReg02.sw_dec_strendian_e = 1;
    regs->SwReg02.sw_dec_max_burst = 16;
    regs->SwReg55.sw_apf_threshold = 1;
    regs->SwReg02.sw_dec_timeout_e = 1;
    regs->SwReg02.sw_dec_clk_gate_e = 1;
    regs->SwReg01.sw_dec_en = 1;
    regs->SwReg49.sw_pred_bc_tap_0_0 = -1;
    regs->SwReg49.sw_pred_bc_tap_0_1 = 3;
    regs->SwReg49.sw_pred_bc_tap_0_2 = -6;
    regs->SwReg34.sw_pred_bc_tap_0_3 = 20;

    ctx->frm_slots  = cfg->frame_slots;
    ctx->pkt_slots  = cfg->packet_slots;
    ctx->int_cb     = cfg->hal_int_cb;
    ctx->group      = group;
    ctx->mv_buf     = mv_buf;
    ctx->qp_table   = qp_table;
    ctx->regs       = regs;

    mpp_env_get_u32("mpg4d_hal_debug", &mpg4d_hal_debug, 0);

    return ret;
ERR_RET:
    if (regs) {
        mpp_free(regs);
        regs = NULL;
    }

    if (qp_table) {
        mpp_buffer_put(qp_table);
        qp_table = NULL;
    }

    if (mv_buf) {
        mpp_buffer_put(mv_buf);
        mv_buf = NULL;
    }

    if (group) {
        mpp_buffer_group_put(group);
        group = NULL;
    }

    return ret;
}

MPP_RET vdpu1_mpg4d_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    hal_mpg4_ctx *ctx = (hal_mpg4_ctx *)hal;

    mpp_assert(hal);

    if (ctx->regs) {
        mpp_free(ctx->regs);
        ctx->regs = NULL;
    }

    if (ctx->qp_table) {
        mpp_buffer_put(ctx->qp_table);
        ctx->qp_table = NULL;
    }

    if (ctx->mv_buf) {
        mpp_buffer_put(ctx->mv_buf);
        ctx->mv_buf = NULL;
    }

    if (ctx->group) {
        mpp_buffer_group_put(ctx->group);
        ctx->group = NULL;
    }

    if (ctx->dev_ctx) {
        ret = mpp_device_deinit(ctx->dev_ctx);
        if (ret)
            mpp_err("mpp_device_deinit failed ret: %d\n", ret);
    }

    return ret;
}

MPP_RET vdpu1_mpg4d_gen_regs(void *hal,  HalTaskInfo *syn)
{
    MPP_RET ret = MPP_OK;
    hal_mpg4_ctx *ctx = (hal_mpg4_ctx *)hal;
    HalDecTask *task = &syn->dec;
    MppBuffer buf_frm_curr = NULL;
    MppBuffer buf_frm_ref0 = NULL;
    MppBuffer buf_frm_ref1 = NULL;
    MppBuffer buf_pkt = NULL;
    M4vdVdpu1Regs_t *regs = ctx->regs;

    mpp_assert(task->valid);
    mpp_assert(task->input >= 0);
    mpp_assert(task->output >= 0);

    /* setup buffer for input / output / reference */
    mpp_buf_slot_get_prop(ctx->pkt_slots, task->input, SLOT_BUFFER, &buf_pkt);
    mpp_assert(buf_pkt);
    vpu_mpg4d_get_buffer_by_index(ctx, task->output, &buf_frm_curr);
    vpu_mpg4d_get_buffer_by_index(ctx, task->refer[0], &buf_frm_ref0);
    vpu_mpg4d_get_buffer_by_index(ctx, task->refer[1], &buf_frm_ref1);

    /* address registers setup first */
    ctx->fd_curr = mpp_buffer_get_fd(buf_frm_curr);
    ctx->fd_ref0 = (buf_frm_ref0) ? (mpp_buffer_get_fd(buf_frm_ref0)) : (-1);
    ctx->fd_ref1 = (buf_frm_ref1) ? (mpp_buffer_get_fd(buf_frm_ref1)) : (-1);
    regs->SwReg13.dec_out_st_adr = (RK_U32)ctx->fd_curr;
    regs->SwReg12.sw_rlc_vlc_base = mpp_buffer_get_fd(buf_pkt);

    /* setup other registers, here will update packet address */
    vdpu1_mpg4d_setup_regs_by_syntax(ctx, task->syntax);
    /* memset tails to zero for stream buffer */
    {
        RK_U8 *ptr = (RK_U8 *)mpp_buffer_get_ptr(buf_pkt);
        RK_U32 strm_len = MPP_ALIGN(ctx->bitstrm_len, 16) + 64;
        memset(ptr + ctx->bitstrm_len, 0, strm_len - ctx->bitstrm_len);
    }

    return ret;
}

MPP_RET vdpu1_mpg4d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    hal_mpg4_ctx *ctx = (hal_mpg4_ctx *)hal;
    RK_U32 reg_count = (sizeof(*(M4vdVdpu1Regs_t *)(ctx->regs)) / sizeof(RK_U32));
    RK_U32* regs = (RK_U32 *)ctx->regs;

    if (mpg4d_hal_debug & MPG4D_HAL_DBG_REG_PUT) {
        RK_U32 i = 0;
        for (i = 2; i < reg_count; i++) {
            mpp_log("reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    ret = mpp_device_send_reg(ctx->dev_ctx, regs, reg_count);

    (void)task;
    return ret;
}

MPP_RET vdpu1_mpg4d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    hal_mpg4_ctx *ctx = (hal_mpg4_ctx *)hal;
    M4vdVdpu1Regs_t reg_out;
    RK_U32* regs = (RK_U32 *)&reg_out;
    RK_U32 reg_count = (sizeof(reg_out) / sizeof(RK_U32));

    ret = mpp_device_wait_reg(ctx->dev_ctx, regs, (sizeof(reg_out) / sizeof(RK_U32)));

    if (mpg4d_hal_debug & MPG4D_HAL_DBG_REG_GET) {
        RK_U32 i = 0;

        for (i = 0; i < reg_count; i++) {
            mpp_log("reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    (void)task;
    return ret;
}

MPP_RET vdpu1_mpg4d_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    (void)hal;
    return ret;
}

MPP_RET vdpu1_mpg4d_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET vdpu1_mpg4d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    (void)cmd_type;
    (void)param;
    return  ret;
}
