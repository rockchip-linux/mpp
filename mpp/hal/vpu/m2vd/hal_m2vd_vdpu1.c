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

#define MODULE_TAG "hal_m2vd_vdpu1"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_m2vd_base.h"
#include "hal_m2vd_vdpu1_reg.h"
#include "hal_m2vd_vpu1.h"

MPP_RET hal_m2vd_vdpu1_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    M2vdHalCtx *ctx = (M2vdHalCtx *)hal;
    M2vdVdpu1Reg_t *regs = NULL;

    regs = mpp_calloc(M2vdVdpu1Reg_t, 1);
    if (NULL == regs) {
        mpp_err_f("failed to malloc register ret\n");
        ret = MPP_ERR_MALLOC;
        goto __ERR_RET;
    }

    ctx->reg_len = M2VD_VDPU1_REG_NUM;

    ret = mpp_dev_init(&ctx->dev, VPU_CLIENT_VDPU1);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        ret = MPP_ERR_UNKNOW;
        goto __ERR_RET;
    }
    if (ctx->group == NULL) {
        ret = mpp_buffer_group_get_internal(&ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("m2v_hal mpp_buffer_group_get failed\n");
            goto __ERR_RET;
        }
    }
    ret = mpp_buffer_get(ctx->group, &ctx->qp_table, M2VD_BUF_SIZE_QPTAB);
    if (ret) {
        mpp_err("m2v_hal_qtable_base get buffer failed\n");
        goto __ERR_RET;
    }

    ctx->packet_slots   = cfg->packet_slots;
    ctx->frame_slots    = cfg->frame_slots;
    ctx->dec_cb         = cfg->dec_cb;
    ctx->regs           = (void*)regs;
    cfg->dev            = ctx->dev;

    return ret;

__ERR_RET:
    if (regs) {
        mpp_free(regs);
        regs = NULL;
    }

    if (ctx) {
        hal_m2vd_vdpu1_deinit(ctx);
    }

    return ret;
}

MPP_RET hal_m2vd_vdpu1_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    M2vdHalCtx *p = (M2vdHalCtx *)hal;

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }

    if (p->qp_table) {
        ret = mpp_buffer_put(p->qp_table);
        p->qp_table = NULL;
        if (MPP_OK !=  ret) {
            mpp_err("m2v_hal qp_table put buffer failed\n");
            return ret;
        }
    }

    if (p->group) {
        ret = mpp_buffer_group_put(p->group);
        p->group = NULL;
        if (ret) {
            mpp_err("m2v_hal group free buffer failed\n");
            return ret;
        }
    }

    return ret;
}

static MPP_RET hal_m2vd_vdpu1_init_hwcfg(M2vdHalCtx *ctx)
{
    M2vdVdpu1Reg_t *p_regs = (M2vdVdpu1Reg_t *)ctx->regs;

    memset(p_regs, 0, sizeof(M2vdVdpu1Reg_t));
    p_regs->sw02.dec_axi_rn_id = 0;
    p_regs->sw02.dec_timeout_e = 1;
    p_regs->sw02.dec_strswap32_e = 1;
    p_regs->sw02.dec_strendian_e = 1;
    p_regs->sw02.dec_inswap32_e = 1;
    p_regs->sw02.dec_outswap32_e = 1;

    p_regs->sw02.dec_clk_gate_e = 1;
    p_regs->sw02.dec_in_endian = 1;
    p_regs->sw02.dec_out_endian = 1;

    p_regs->sw02.dec_out_tiled_e = 0;
    p_regs->sw02.dec_max_burst = DEC_BUS_BURST_LENGTH_16;
    p_regs->sw02.dec_scmd_dis = 0;
    p_regs->sw02.dec_adv_pre_dis = 0;
    p_regs->sw55.apf_threshold = 8;
    p_regs->sw02.dec_latency = 0;
    p_regs->sw02.dec_data_disc_e = 0;
    p_regs->sw01.dec_irq = 0;
    p_regs->sw02.dec_axi_rn_id = 0;
    p_regs->sw03.dec_axi_wr_id = 0;
    p_regs->sw03.dec_mode = 8;

    return MPP_OK;
}

MPP_RET hal_m2vd_vdpu1_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;

    if (task->dec.valid) {
        void *q_table = NULL;
        MppBuffer streambuf = NULL;
        MppBuffer framebuf = NULL;
        M2vdHalCtx *ctx = (M2vdHalCtx *)hal;
        M2VDDxvaParam *dx = (M2VDDxvaParam *)task->dec.syntax.data;
        M2vdVdpu1Reg_t *p_regs = (M2vdVdpu1Reg_t*) ctx->regs;

        task->dec.valid = 0;
        q_table = mpp_buffer_get_ptr(ctx->qp_table);
        memcpy(q_table, dx->qp_tab, M2VD_BUF_SIZE_QPTAB);

        hal_m2vd_vdpu1_init_hwcfg(ctx);

        p_regs->sw18.mv_accuracy_fwd = 1;
        p_regs->sw18.mv_accuracy_bwd = 1;
        if (dx->seq_ext_head_dec_flag) {
            p_regs->sw03.dec_mode = 5;
            p_regs->sw18.fcode_fwd_hor = dx->pic.full_pel_forward_vector;
            p_regs->sw18.fcode_fwd_ver = dx->pic.forward_f_code;
            p_regs->sw18.fcode_bwd_hor = dx->pic.full_pel_backward_vector;
            p_regs->sw18.fcode_bwd_ver = dx->pic.backward_f_code;
        } else {
            p_regs->sw03.dec_mode = 6;
            p_regs->sw18.fcode_fwd_hor = dx->pic.forward_f_code;
            p_regs->sw18.fcode_fwd_ver = dx->pic.forward_f_code;
            p_regs->sw18.fcode_bwd_hor = dx->pic.backward_f_code;
            p_regs->sw18.fcode_bwd_ver = dx->pic.backward_f_code;
            if (dx->pic.full_pel_forward_vector)
                p_regs->sw18.mv_accuracy_fwd = 0;
            if (dx->pic.full_pel_backward_vector)
                p_regs->sw18.mv_accuracy_bwd = 0;
        }

        p_regs->sw04.pic_mb_width = (dx->seq.decode_width + 15) >> 4;
        p_regs->sw04.pic_mb_height_p = (dx->seq.decode_height + 15) >> 4;
        p_regs->sw03.pic_interlace_e = 1 - dx->seq_ext.progressive_sequence;
        if (dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_FRAME)
            p_regs->sw03.pic_fieldmode_e = 0;
        else {
            p_regs->sw03.pic_fieldmode_e = 1;
            p_regs->sw03.pic_topfield_e = dx->pic_code_ext.picture_structure == 1;
        }
        if (dx->pic.picture_coding_type == M2VD_CODING_TYPE_B)
            p_regs->sw03.pic_b_e = 1;
        else
            p_regs->sw03.pic_b_e = 0;
        if (dx->pic.picture_coding_type == M2VD_CODING_TYPE_I)
            p_regs->sw03.pic_inter_e = 0;
        else
            p_regs->sw03.pic_inter_e = 1;

        p_regs->sw04.topfieldfirst_e = dx->pic_code_ext.top_field_first;
        p_regs->sw03.fwd_interlace_e = 0;
        p_regs->sw03.write_mvs_e = 0;
        p_regs->sw04.alt_scan_e = dx->pic_code_ext.alternate_scan;
        p_regs->sw18.alt_scan_flag_e = dx->pic_code_ext.alternate_scan;

        p_regs->sw05.qscale_type = dx->pic_code_ext.q_scale_type;
        p_regs->sw05.intra_dc_prec = dx->pic_code_ext.intra_dc_precision;
        p_regs->sw05.con_mv_e = dx->pic_code_ext.concealment_motion_vectors;
        p_regs->sw05.intra_vlc_tab = dx->pic_code_ext.intra_vlc_format;
        p_regs->sw05.frame_pred_dct = dx->pic_code_ext.frame_pred_frame_dct;
        p_regs->sw06.init_qp = 1;

        mpp_buf_slot_get_prop(ctx->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
        p_regs->sw12.rlc_vlc_base = mpp_buffer_get_fd(streambuf);
        if (dx->bitstream_offset) {
            mpp_dev_set_reg_offset(ctx->dev, 12, dx->bitstream_offset);
        }

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->CurrPic.Index7Bits, SLOT_BUFFER, &framebuf);

        if ((dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) ||
            (dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_FRAME)) {
            p_regs->sw13.dec_out_base = mpp_buffer_get_fd(framebuf);
        } else {
            p_regs->sw13.dec_out_base = mpp_buffer_get_fd(framebuf);
            mpp_dev_set_reg_offset(ctx->dev, 13, MPP_ALIGN(dx->seq.decode_width, 16));
        }

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[0].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw14.refer0_base = mpp_buffer_get_fd(framebuf);

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[1].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw15.refer1_base = mpp_buffer_get_fd(framebuf);

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[2].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw16.refer2_base = mpp_buffer_get_fd(framebuf);

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[3].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw17.refer3_base = mpp_buffer_get_fd(framebuf);

        p_regs->sw40.qtable_base = mpp_buffer_get_fd(ctx->qp_table);

        p_regs->sw48.startmb_x = 0;
        p_regs->sw48.startmb_y = 0;
        p_regs->sw03.dec_out_dis = 0;
        p_regs->sw03.filtering_dis = 1;
        p_regs->sw06.stream_len = dx->bitstream_length;
        p_regs->sw05.stream_start_bit = dx->bitstream_start_bit;
        p_regs->sw01.dec_e = 1;

        task->dec.valid = 1;
        ctx->dec_frame_cnt++;
    }

    return ret;
}

MPP_RET hal_m2vd_vdpu1_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    M2vdHalCtx *ctx = (M2vdHalCtx *)hal;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 *regs = (RK_U32 *)ctx->regs;
        RK_U32 reg_size = sizeof(M2vdVdpu1Reg_t);

        wr_cfg.reg = regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = regs;
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

MPP_RET hal_m2vd_vdpu1_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    M2vdHalCtx *ctx = (M2vdHalCtx *)hal;
    M2vdVdpu1Reg_t* reg_out = (M2vdVdpu1Reg_t * )ctx->regs;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    if (reg_out->sw01.dec_error_int | reg_out->sw01.dec_buffer_int) {
        if (ctx->dec_cb)
            mpp_callback(ctx->dec_cb, NULL);
    }

    (void)task;

    return ret;
}
