/*
 *
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
#include "mpp_env.h"
#include "mpp_buffer.h"

#include "vpu.h"
#include "mpp_dec.h"
#include "h263d_syntax.h"
#include "hal_h263d_api.h"
#include "hal_h263d_reg.h"

typedef struct h263d_reg_context {
    MppBufSlots         frm_slots;
    MppBufSlots         pkt_slots;
    IOInterruptCB       int_cb;

    // save fd for curr/ref0/ref1 for reg_gen
    RK_S32              vpu_fd;
    RK_S32              fd_curr;
    RK_S32              fd_ref0;

    VpuH263dRegSet_t*   regs;
} hal_h263_ctx;

RK_U32 h263d_hal_debug = 0;

static void vpu_h263d_get_buffer_by_index(hal_h263_ctx *ctx, RK_S32 index, MppBuffer *buffer)
{
    if (index >= 0) {
        mpp_buf_slot_get_prop(ctx->frm_slots, index, SLOT_BUFFER, buffer);
        mpp_assert(*buffer);
    }
}

static void vpu_h263d_setup_regs_by_syntax(hal_h263_ctx *ctx, MppSyntax syntax)
{
    VpuH263dRegSet_t *regs = ctx->regs;
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

        val += (consumed_bytes_align << 10);
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

MPP_RET hal_vpu_h263d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    VpuH263dRegSet_t *regs = NULL;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    RK_S32 vpu_fd = -1;

    mpp_assert(hal);

    regs = mpp_calloc(VpuH263dRegSet_t, 1);
    if (NULL == regs) {
        mpp_err_f("failed to malloc register ret\n");
        ret = MPP_ERR_MALLOC;
        goto ERR_RET;
    }

#ifdef RKPLATFORM
    vpu_fd = VPUClientInit(VPU_DEC);
    if (vpu_fd < 0) {
        mpp_err_f("failed to open vpu client\n");
        ret = MPP_ERR_UNKNOW;
        goto ERR_RET;
    }
#endif

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

    ctx->frm_slots  = cfg->frame_slots;
    ctx->pkt_slots  = cfg->packet_slots;
    ctx->int_cb     = cfg->hal_int_cb;
    ctx->vpu_fd     = vpu_fd;
    ctx->regs       = regs;

    mpp_env_get_u32("h263d_hal_debug", &h263d_hal_debug, 0);

    return ret;
ERR_RET:
    if (regs) {
        mpp_free(regs);
        regs = NULL;
    }

    return ret;
}

MPP_RET hal_vpu_h263d_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;

    mpp_assert(hal);

    if (ctx->regs) {
        mpp_free(ctx->regs);
        ctx->regs = NULL;
    }

#ifdef RKPLATFORM
    if (ctx->vpu_fd >= 0) {
        VPUClientRelease(ctx->vpu_fd);
        ctx->vpu_fd = -1;
    }
#endif

    return ret;
}

MPP_RET hal_vpu_h263d_gen_regs(void *hal,  HalTaskInfo *syn)
{
    MPP_RET ret = MPP_OK;
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    HalDecTask *task = &syn->dec;
    MppBuffer buf_frm_curr = NULL;
    MppBuffer buf_frm_ref0 = NULL;
    MppBuffer buf_pkt = NULL;
    VpuH263dRegSet_t *regs = ctx->regs;

    mpp_assert(task->valid);
    mpp_assert(task->input >= 0);
    mpp_assert(task->output >= 0);

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
    vpu_h263d_setup_regs_by_syntax(ctx, task->syntax);

    return ret;
}

MPP_RET hal_vpu_h263d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
#ifdef RKPLATFORM
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    RK_U32 reg_count = (sizeof(*ctx->regs) / sizeof(RK_U32));
    RK_U32* regs = (RK_U32 *)ctx->regs;

    if (h263d_hal_debug & H263D_HAL_DBG_REG_PUT) {
        RK_U32 i = 0;
        for (i = 0; i < reg_count; i++) {
            mpp_log("reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    ret = VPUClientSendReg(ctx->vpu_fd, regs, reg_count);
#endif
    (void)hal;
    (void)task;
    return ret;
}

MPP_RET hal_vpu_h263d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
#ifdef RKPLATFORM
    hal_h263_ctx *ctx = (hal_h263_ctx *)hal;
    VpuH263dRegSet_t reg_out;
    RK_U32* regs = (RK_U32 *)&reg_out;
    RK_U32 reg_count = (sizeof(reg_out) / sizeof(RK_U32));
    VPU_CMD_TYPE cmd = 0;
    RK_S32 length = 0;

    ret = VPUClientWaitResult(ctx->vpu_fd, regs, (sizeof(reg_out) / sizeof(RK_U32)),
                              &cmd, &length);

    if (h263d_hal_debug & H263D_HAL_DBG_REG_GET) {
        RK_U32 i = 0;

        for (i = 0; i < reg_count; i++) {
            mpp_log("reg[%03d]: %08x\n", i, regs[i]);
        }
    }
#endif
    (void)hal;
    (void)task;
    return ret;
}

MPP_RET hal_vpu_h263d_reset(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET hal_vpu_h263d_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET hal_vpu_h263d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    (void)cmd_type;
    (void)param;
    return ret;
}

const MppHalApi hal_api_h263d = {
    "h263d_vpu",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingH263,
    sizeof(hal_h263_ctx),
    0,
    hal_vpu_h263d_init,
    hal_vpu_h263d_deinit,
    hal_vpu_h263d_gen_regs,
    hal_vpu_h263d_start,
    hal_vpu_h263d_wait,
    hal_vpu_h263d_reset,
    hal_vpu_h263d_flush,
    hal_vpu_h263d_control,
};

