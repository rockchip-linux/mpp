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

#define MODULE_TAG "hal_m2vd_vdpu2"

#include <string.h>
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "hal_m2vd_base.h"
#include "hal_m2vd_vdpu2_reg.h"
#include "hal_m2vd_vpu2.h"
#include "hal_m2vd_api.h"

MPP_RET hal_m2vd_vdpu2_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    M2vdHalCtx *p = (M2vdHalCtx *)hal;
    M2vdVdpu2Reg *reg = NULL;
    m2vh_dbg_func("FUN_I");

    reg = mpp_calloc(M2vdVdpu2Reg, 1);
    if (NULL == reg) {
        mpp_err_f("failed to malloc register ret\n");
        ret = MPP_ERR_MALLOC;
        goto __ERR_RET;
    }

    p->reg_len = M2VD_VDPU2_REG_NUM;

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_DEC,              /* type */
        .coding = MPP_VIDEO_CodingMPEG2,  /* coding */
        .platform = 0,                    /* platform */
        .pp_enable = 0,                   /* pp_enable */
    };
    ret = mpp_device_init(&p->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        goto __ERR_RET;
    }

    if (p->group == NULL) {
        ret = mpp_buffer_group_get_internal(&p->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("m2v_hal mpp_buffer_group_get failed\n");
            goto __ERR_RET;
        }
    }

    ret = mpp_buffer_get(p->group, &p->qp_table, M2VD_BUF_SIZE_QPTAB);
    if (ret) {
        mpp_err("m2v_hal qtable_base get buffer failed\n");
        goto __ERR_RET;
    }

    if (M2VH_DBG_DUMP_REG & m2vh_debug) {
        p->fp_reg_in = fopen("/sdcard/m2vd_dbg_reg_in.txt", "wb");
        if (p->fp_reg_in == NULL) {
            mpp_log("file open error: %s", "/sdcard/m2vd_dbg_reg_in.txt");
        }
        p->fp_reg_out = fopen("/sdcard/m2vd_dbg_reg_out.txt", "wb");
        if (p->fp_reg_out == NULL) {
            mpp_log("file open error: %s", "/sdcard/m2vd_dbg_reg_out.txt");
        }
    } else {
        p->fp_reg_in = NULL;
        p->fp_reg_out = NULL;
    }

    //configure
    p->packet_slots = cfg->packet_slots;
    p->frame_slots = cfg->frame_slots;
    p->int_cb = cfg->hal_int_cb;
    p->regs = (void*)reg;

    m2vh_dbg_func("FUN_O");

    return ret;

__ERR_RET:
    if (reg) {
        mpp_free(reg);
        reg = NULL;
    }

    if (p) {
        hal_m2vd_vdpu2_deinit(p);
    }

    return ret;
}

MPP_RET hal_m2vd_vdpu2_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    m2vh_dbg_func("FUN_I");
    M2vdHalCtx *p = (M2vdHalCtx *)hal;

    if (p->dev_ctx) {
        ret = mpp_device_deinit(p->dev_ctx);
        p->dev_ctx = NULL;
        if (ret) {
            mpp_err("mpp_device_deinit failed ret: %d\n", ret);
        }
    }

    if (p->qp_table) {
        ret = mpp_buffer_put(p->qp_table);
        p->qp_table = NULL;
        if (ret) {
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
    if (p->fp_reg_in) {
        fclose(p->fp_reg_in);
        p->fp_reg_in = NULL;
    }
    if (p->fp_reg_out) {
        fclose(p->fp_reg_out);
        p->fp_reg_out = NULL;
    }

    m2vh_dbg_func("FUN_O");
    return ret;
}

static MPP_RET hal_m2vd_vdpu2_init_hwcfg(M2vdHalCtx *ctx)
{

    M2vdVdpu2Reg *p_regs = (M2vdVdpu2Reg *)ctx->regs;

    memset(p_regs, 0, sizeof(M2vdVdpu2Reg));

    p_regs->sw56.dec_axi_rn_id = 0;
    p_regs->sw57.dec_timeout_e = 1;
    p_regs->sw54.dec_strswap32_e = 1;     //change
    p_regs->sw54.dec_strendian_e = DEC_LITTLE_ENDIAN;
    p_regs->sw54.dec_inswap32_e = 1;      //change
    p_regs->sw54.dec_outswap32_e = 1; //change


    p_regs->sw57.dec_clk_gate_e = 1;      //change
    p_regs->sw54.dec_in_endian = DEC_LITTLE_ENDIAN;  //change
    p_regs->sw54.dec_out_endian = DEC_LITTLE_ENDIAN;

    p_regs->sw50.dec_out_tiled_e = 0;
    p_regs->sw56.dec_max_burst = DEC_BUS_BURST_LENGTH_16;
    p_regs->sw50.dec_scmd_dis = 0;
    p_regs->sw50.dec_adv_pre_dis = 0;
    p_regs->sw52.apf_threshold = 8;

    p_regs->sw50.dec_latency = 0;
    p_regs->sw56.dec_data_disc_e  = 0;

    p_regs->sw55.dec_irq = 0;
    p_regs->sw56.dec_axi_rn_id = 0;
    p_regs->sw56.dec_axi_wr_id = 0;

    p_regs->sw53.sw_dec_mode = 8;

    p_regs->ppReg[0] = 0;
    p_regs->sw136.mv_accuracy_fwd = 1;
    p_regs->sw136.mv_accuracy_bwd = 1;

    return MPP_OK;
}

MPP_RET hal_m2vd_vdpu2_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    if (task->dec.valid) {
        void *q_table = NULL;
        MppBuffer streambuf = NULL;
        MppBuffer framebuf = NULL;
        M2vdHalCtx *ctx = (M2vdHalCtx *)hal;
        M2VDDxvaParam *dx = (M2VDDxvaParam *)task->dec.syntax.data;
        M2vdVdpu2Reg *p_regs = ctx->regs;

        task->dec.valid = 0;
        q_table = mpp_buffer_get_ptr(ctx->qp_table);
        memcpy(q_table, dx->qp_tab, M2VD_BUF_SIZE_QPTAB);

        hal_m2vd_vdpu2_init_hwcfg(ctx);

        if (dx->seq_ext_head_dec_flag) {
            p_regs->sw53.sw_dec_mode = 5;
            p_regs->sw136.fcode_fwd_hor = dx->pic.full_pel_forward_vector;
            p_regs->sw136.fcode_fwd_ver = dx->pic.forward_f_code;
            p_regs->sw136.fcode_bwd_hor = dx->pic.full_pel_backward_vector;
            p_regs->sw136.fcode_bwd_ver = dx->pic.backward_f_code;

        } else {
            p_regs->sw53.sw_dec_mode = 6;
            p_regs->sw136.fcode_fwd_hor = dx->pic.forward_f_code;
            p_regs->sw136.fcode_fwd_ver = dx->pic.forward_f_code;
            p_regs->sw136.fcode_bwd_hor = dx->pic.backward_f_code;
            p_regs->sw136.fcode_bwd_ver = dx->pic.backward_f_code;
            if (dx->pic.full_pel_forward_vector)
                p_regs->sw136.mv_accuracy_fwd = 0;
            if (dx->pic.full_pel_backward_vector)
                p_regs->sw136.mv_accuracy_bwd = 0;
        }

        p_regs->sw120.pic_mb_width = (dx->seq.decode_width + 15) >> 4;
        p_regs->sw120.pic_mb_height_p = (dx->seq.decode_height + 15) >> 4;
        p_regs->sw57.pic_interlace_e = 1 - dx->seq_ext.progressive_sequence;
        if (dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_FRAME)
            p_regs->sw57.pic_fieldmode_e = 0;
        else {
            p_regs->sw57.pic_fieldmode_e = 1;
            p_regs->sw57.pic_topfield_e = dx->pic_code_ext.picture_structure == 1;
        }
        if (dx->pic.picture_coding_type == M2VD_CODING_TYPE_B)
            p_regs->sw57.pic_b_e = 1;
        else
            p_regs->sw57.pic_b_e = 0;
        if (dx->pic.picture_coding_type == M2VD_CODING_TYPE_I)
            p_regs->sw57.pic_inter_e = 0;
        else
            p_regs->sw57.pic_inter_e = 1;

        p_regs->sw120.topfieldfirst_e = dx->pic_code_ext.top_field_first;
        p_regs->sw57.fwd_interlace_e = 0;
        p_regs->sw57.write_mvs_e = 0;//concealment_motion_vectors;
        p_regs->sw120.alt_scan_e = dx->pic_code_ext.alternate_scan;
        p_regs->sw136.alt_scan_flag_e = dx->pic_code_ext.alternate_scan;

        p_regs->sw122.qscale_type = dx->pic_code_ext.q_scale_type;
        p_regs->sw122.intra_dc_prec = dx->pic_code_ext.intra_dc_precision;
        p_regs->sw122.con_mv_e = dx->pic_code_ext.concealment_motion_vectors;
        p_regs->sw122.intra_vlc_tab = dx->pic_code_ext.intra_vlc_format;
        p_regs->sw122.frame_pred_dct = dx->pic_code_ext.frame_pred_frame_dct;
        p_regs->sw51.init_qp = 1;

        mpp_buf_slot_get_prop(ctx->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
        p_regs->sw64.VLC_base = mpp_buffer_get_fd(streambuf);
        p_regs->sw64.VLC_base |= (dx->bitstream_offset << 10);

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->CurrPic.Index7Bits, SLOT_BUFFER, &framebuf);


        if ((dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) ||
            (dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_FRAME)) {
            p_regs->sw63.cur_pic_base = mpp_buffer_get_fd(framebuf); //just index need map
        } else {
            p_regs->sw63.cur_pic_base = mpp_buffer_get_fd(framebuf) | (MPP_ALIGN(dx->seq.decode_width, 16) << 10);
        }

        //ref & qtable config
        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[0].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw131.ref0  = mpp_buffer_get_fd(framebuf); //just index need map

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[1].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw148.ref1  = mpp_buffer_get_fd(framebuf); //just index need map

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[2].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw134.ref2  = mpp_buffer_get_fd(framebuf); //just index need map

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[3].Index7Bits, SLOT_BUFFER, &framebuf);
        p_regs->sw135.ref3  = mpp_buffer_get_fd(framebuf); //just index need map

        p_regs->sw61.slice_table = mpp_buffer_get_fd(ctx->qp_table);

        p_regs->sw52.startmb_x = 0;
        p_regs->sw52.startmb_y = 0;
        p_regs->sw57.dec_out_dis = 0;
        p_regs->sw50.filtering_dis = 1;

        p_regs->sw51.stream_len = dx->bitstream_length;
        p_regs->sw122.stream_start_bit = dx->bitstream_start_bit;
        p_regs->sw57.dec_e = 1;

        if (M2VH_DBG_REG & m2vh_debug) {
            RK_U32 j = 0;
            RK_U32 *p_reg = (RK_U32 *)p_regs;
            for (j = 50; j < 159; j++) {
                mpp_log("reg[%d] = 0x%08x", j, p_reg[j]);
            }
        }
        if (ctx->fp_reg_in) {
            int k = 0;
            RK_U32 *p_reg = (RK_U32*)p_regs;
            mpp_log("fwrite regs start");
            fprintf(ctx->fp_reg_in, "Frame #%d\n", ctx->dec_frame_cnt);
            for (k = 0; k < M2VD_VDPU2_REG_NUM; k++)
                fprintf(ctx->fp_reg_in, "[(D)%03d, (X)%03x]  %08x\n", k, k, p_reg[k]);
            fflush(ctx->fp_reg_in);
        }

        task->dec.valid = 1;
        ctx->dec_frame_cnt++;
    }
    return ret;

}

MPP_RET hal_m2vd_vdpu2_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    M2vdHalCtx *ctx = (M2vdHalCtx *)hal;
    RK_U32 *p_regs = (RK_U32 *)ctx->regs;

    m2vh_dbg_func("FUN_I");

    ret = mpp_device_send_reg(ctx->dev_ctx, p_regs, ctx->reg_len);
    if (ret) {
        mpp_err("mpp_device_send_reg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }

    (void)task;
    m2vh_dbg_func("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_vdpu2_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    M2vdVdpu2Reg reg_out;
    M2vdHalCtx *ctx = (M2vdHalCtx *)hal;

    m2vh_dbg_func("FUN_I");

    memset(&reg_out, 0, sizeof(M2vdVdpu2Reg));
    ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)&reg_out, ctx->reg_len);
    if (ctx->fp_reg_out) {
        int k = 0;
        RK_U32 *p_reg = (RK_U32*)&reg_out;
        fprintf(ctx->fp_reg_out, "Frame #%d\n", ctx->dec_frame_cnt);
        for (k = 0; k < M2VD_VDPU2_REG_NUM; k++)
            fprintf(ctx->fp_reg_out, "[(D)%03d, (X)%03x]  %08x\n", k, k, p_reg[k]);
        fflush(ctx->fp_reg_out);
    }
    if (reg_out.sw55.dec_error_int | reg_out.sw55.dec_buffer_int) {
        if (ctx->int_cb.callBack)
            ctx->int_cb.callBack(ctx->int_cb.opaque, NULL);
    }

    if (M2VH_DBG_IRQ & m2vh_debug)
        mpp_log("mpp_device_wait_reg return interrupt:%08x", reg_out.sw55);

    if (ret) {
        mpp_log("mpp_device_wait_reg return error:%d", ret);
    }

    (void)task;
    m2vh_dbg_func("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_vdpu2_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    m2vh_dbg_func("FUN_I");
    (void)hal;
    m2vh_dbg_func("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_vdpu2_flush(void *hal)
{
    MPP_RET ret = MPP_OK;
    m2vh_dbg_func("FUN_I");
    (void)hal;
    m2vh_dbg_func("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_vdpu2_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    m2vh_dbg_func("FUN_I");
    (void)hal;
    (void)cmd_type;
    (void)param;
    m2vh_dbg_func("FUN_O");
    return ret;
}
