/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_vpu_h263d"

#include <stdio.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"

#include "h263d_syntax.h"
#include "hal_h263d_api.h"
#include "hal_h263d_vdpu2.h"
#include "hal_h263d_vdpu2_reg.h"
#include "hal_h263d_base.h"

static void vpu2_h263d_setup_regs_by_syntax(hal_h263_ctx *ctx, MppSyntax syntax)
{
    Vpu2H263dRegSet_t *regs = (Vpu2H263dRegSet_t*)ctx->regs;
    DXVA2_DecodeBufferDesc **data = syntax.data;
    DXVA_PicParams_H263 *pp = NULL;
    RK_U32 stream_length = 0;
    RK_U32 stream_used = 0;
    RK_U32 i;

    for (i = 0; i < syntax.number; i++) {
        DXVA2_DecodeBufferDesc *desc = data[i];
        switch (desc->CompressedBufferType) {
        case DXVA2_PictureParametersBufferType : {
            pp = (DXVA_PicParams_H263 *)desc->pvPVPState;
        } break;
        case DXVA2_BitStreamDateBufferType : {
            stream_length = desc->DataSize;
            stream_used = desc->DataOffset;
        } break;
        default : {
            mpp_err_f("found invalid buffer descriptor type %d\n", desc->CompressedBufferType);
        } break;
        }
    }

    mpp_assert(pp);
    mpp_assert(stream_length);
    mpp_assert(stream_used);

    regs->reg120.sw_pic_mb_width = (pp->vop_width  + 15) >> 4;
    regs->reg120.sw_pic_mb_hight_p = (pp->vop_height + 15) >> 4;
    regs->reg120.sw_mb_width_off = pp->vop_width & 0xf;
    regs->reg120.sw_mb_height_off = pp->vop_height & 0xf;

    regs->reg53_dec_mode = 2;
    regs->reg50_dec_ctrl.sw_filtering_dis = 1;
    regs->reg136.sw_rounding = 0;
    regs->reg51_stream_info.sw_init_qp = pp->vop_quant;
    regs->reg122.sw_sync_markers_en = 1;

    {
        /*
         * update stream base address here according to consumed bit length
         * 1. hardware start address has to be 64 bit align
         * 2. hardware need to know which is the start bit in
         * 2. pass (10bit fd + (offset << 10)) register value to kernel
         */
        RK_U32 val = regs->reg64_input_stream_base;
        RK_U32 consumed_bytes = stream_used >> 3;
        RK_U32 consumed_bytes_align = consumed_bytes & (~0x7);
        RK_U32 start_bit_offset = stream_used & 0x3F;
        RK_U32 left_bytes = stream_length - consumed_bytes_align;

        if (consumed_bytes_align) {
            mpp_dev_set_reg_offset(ctx->dev, 64, consumed_bytes_align);
        }
        regs->reg64_input_stream_base = val;
        regs->reg122.sw_stream_start_word = start_bit_offset;
        regs->reg51_stream_info.sw_stream_len = left_bytes;
    }

    regs->reg122.sw_vop_time_incr = pp->vop_time_increment_resolution;

    switch (pp->vop_coding_type) {
    case H263_P_VOP : {
        regs->reg57_enable_ctrl.sw_pic_inter_e = 1;

        if (ctx->fd_ref0 >= 0) {
            regs->reg131_ref0_base = (RK_U32)ctx->fd_ref0;
            regs->reg148_ref1_base = (RK_U32)ctx->fd_ref0;
        } else {
            regs->reg131_ref0_base = (RK_U32)ctx->fd_curr;
            regs->reg148_ref1_base = (RK_U32)ctx->fd_curr;
        }
    } break;
    case H263_I_VOP : {
        regs->reg57_enable_ctrl.sw_pic_inter_e = 0;

        regs->reg131_ref0_base = (RK_U32)ctx->fd_curr;
        regs->reg148_ref1_base = (RK_U32)ctx->fd_curr;
    } break;
    default : {
        /* no nothing */
    } break;
    }

    regs->reg136.sw_hrz_bit_of_fwd_mv = 1;
    regs->reg136.sw_vrz_bit_of_fwd_mv = 1;
    regs->reg136.sw_prev_pic_type = (pp->prev_coding_type == H263_P_VOP);
}

MPP_RET hal_vpu2_h263d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    Vpu2H263dRegSet_t *regs = NULL;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;

    mpp_assert(hal);

    regs = mpp_calloc(Vpu2H263dRegSet_t, 1);
    if (NULL == regs) {
        mpp_err_f("failed to malloc register ret\n");
        ret = MPP_ERR_MALLOC;
        goto ERR_RET;
    }

    ret = mpp_dev_init(&ctx->dev, VPU_CLIENT_VDPU2);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        ret = MPP_ERR_UNKNOW;
        goto ERR_RET;
    }

    ctx->frm_slots  = cfg->frame_slots;
    ctx->pkt_slots  = cfg->packet_slots;
    ctx->dec_cb     = cfg->dec_cb;
    ctx->regs       = (void*)regs;

    return ret;
ERR_RET:
    if (regs) {
        mpp_free(regs);
        regs = NULL;
    }

    return ret;
}

MPP_RET hal_vpu2_h263d_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;

    mpp_assert(hal);

    if (ctx->regs) {
        mpp_free(ctx->regs);
        ctx->regs = NULL;
    }

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    return ret;
}

MPP_RET hal_vpu2_h263d_gen_regs(void *hal,  HalTaskInfo *syn)
{
    MPP_RET ret = MPP_OK;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    HalDecTask *task = &syn->dec;
    MppBuffer buf_frm_curr = NULL;
    MppBuffer buf_frm_ref0 = NULL;
    MppBuffer buf_pkt = NULL;
    Vpu2H263dRegSet_t *regs = (Vpu2H263dRegSet_t*)ctx->regs;

    mpp_assert(task->valid);
    mpp_assert(task->input >= 0);
    mpp_assert(task->output >= 0);

    memset(regs, 0, sizeof(Vpu2H263dRegSet_t));

    /*
     * basic register configuration setup here
     */
    regs->reg54_endian.sw_dec_out_endian = 1;
    regs->reg54_endian.sw_dec_in_endian = 1;
    regs->reg54_endian.sw_dec_inswap32_e = 1;
    regs->reg54_endian.sw_dec_outswap32_e = 1;
    regs->reg54_endian.sw_dec_strswap32_e = 1;
    regs->reg54_endian.sw_dec_strendian_e = 1;
    regs->reg56_axi_ctrl.sw_dec_max_burst = 16;
    regs->reg52_error_concealment.sw_apf_threshold = 1;
    regs->reg57_enable_ctrl.sw_dec_timeout_e = 1;
    regs->reg57_enable_ctrl.sw_dec_clk_gate_e = 1;
    regs->reg57_enable_ctrl.sw_dec_e = 1;
    regs->reg59.sw_pred_bc_tap_0_0 = -1;
    regs->reg59.sw_pred_bc_tap_0_1 = 3;
    regs->reg59.sw_pred_bc_tap_0_2 = -6;
    regs->reg153.sw_pred_bc_tap_0_3 = 20;

    /* setup buffer for input / output / reference */
    mpp_buf_slot_get_prop(ctx->pkt_slots, task->input, SLOT_BUFFER, &buf_pkt);
    mpp_assert(buf_pkt);
    vpu_h263d_get_buffer_by_index(ctx, task->output, &buf_frm_curr);
    vpu_h263d_get_buffer_by_index(ctx, task->refer[0], &buf_frm_ref0);

    /* address registers setup first */
    ctx->fd_curr = mpp_buffer_get_fd(buf_frm_curr);
    ctx->fd_ref0 = (buf_frm_ref0) ? (mpp_buffer_get_fd(buf_frm_ref0)) : (-1);
    regs->reg63_cur_pic_base = (RK_U32)ctx->fd_curr;
    regs->reg64_input_stream_base = mpp_buffer_get_fd(buf_pkt);

    /* setup other registers, here will update packet address */
    vpu2_h263d_setup_regs_by_syntax(ctx, task->syntax);

    return ret;
}

MPP_RET hal_vpu2_h263d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    RK_U32 *regs = (RK_U32 *)ctx->regs;

    if (h263d_hal_debug & H263D_HAL_DBG_REG_PUT) {
        RK_U32 reg_count = (sizeof(Vpu2H263dRegSet_t) / sizeof(RK_U32));
        RK_U32 i = 0;

        for (i = 0; i < reg_count; i++)
            mpp_log("reg[%03d]: %08x\n", i, regs[i]);
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = regs;
        wr_cfg.size = sizeof(Vpu2H263dRegSet_t);
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = regs;
        rd_cfg.size = sizeof(Vpu2H263dRegSet_t);
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

MPP_RET hal_vpu2_h263d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    if (h263d_hal_debug & H263D_HAL_DBG_REG_GET) {
        RK_U32 *regs = (RK_U32 *)ctx->regs;
        RK_U32 reg_count = (sizeof(Vpu2H263dRegSet_t) / sizeof(RK_U32));
        RK_U32 i = 0;

        for (i = 0; i < reg_count; i++)
            mpp_log("reg[%03d]: %08x\n", i, regs[i]);
    }

    (void)task;
    return ret;
}
