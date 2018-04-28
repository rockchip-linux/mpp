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

#define MODULE_TAG "hal_m2vd_reg"
#include <string.h>
#include "mpp_device.h"
#include "hal_m2vd_reg.h"
#include "mpp_log.h"
#include "mpp_env.h"

RK_U32 m2vh_debug = 0;

#define M2VD_BUF_SIZE_QPTAB            (256)

#define DEC_RKV_LITTLE_ENDIAN                     1
#define DEC_RKV_BIG_ENDIAN                        0
#define DEC_RKV_BUS_BURST_LENGTH_UNDEFINED        0
#define DEC_RKV_BUS_BURST_LENGTH_4                4
#define DEC_RKV_BUS_BURST_LENGTH_8                8
#define DEC_RKV_BUS_BURST_LENGTH_16               16

#define FUN_T(tag) \
    do {\
        if (M2VH_DBG_FUNCTION & m2vh_debug)\
            { mpp_log("%s: line(%d), func(%s)", tag, __LINE__, __FUNCTION__); }\
    } while (0)

typedef enum M2VDPicCodingType_t {
    M2VD_CODING_TYPE_I = 1,
    M2VD_CODING_TYPE_P = 2,
    M2VD_CODING_TYPE_B = 3,
    M2VD_CODING_TYPE_D = 4
} M2VDPicCodingType;

typedef enum M2VDPicStruct_t {
    M2VD_PIC_STRUCT_TOP_FIELD    = 1,
    M2VD_PIC_STRUCT_BOTTOM_FIELD = 2,
    M2VD_PIC_STRUCT_FRAME        = 3
} M2VDPicStruct;

MPP_RET hal_m2vd_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    M2VDHalContext *p = (M2VDHalContext *)hal;
    M2VDRegSet *reg = &p->regs;
    FUN_T("FUN_I");

    //configure
    p->packet_slots = cfg->packet_slots;
    p->frame_slots = cfg->frame_slots;
    p->int_cb = cfg->hal_int_cb;

    mpp_env_get_u32("m2vh_debug", &m2vh_debug, 0);
    //get vpu socket
#ifdef RKPLATFORM
    MppDevCfg dev_cfg = {
        .type = MPP_CTX_DEC,              /* type */
        .coding = MPP_VIDEO_CodingMPEG2,  /* coding */
        .platform = 0,                    /* platform */
        .pp_enable = 0,                   /* pp_enable */
    };
    ret = mpp_device_init(&p->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }
#endif
    if (p->group == NULL) {

#ifdef RKPLATFORM
        ret = mpp_buffer_group_get_internal(&p->group, MPP_BUFFER_TYPE_ION);
#else
        ret = mpp_buffer_group_get_internal(&p->group, MPP_BUFFER_TYPE_NORMAL);
#endif
        if (MPP_OK != ret) {
            mpp_err("m2v_hal mpp_buffer_group_get failed\n");
            return ret;
        }
    }
    ret = mpp_buffer_get(p->group, &p->qp_table, M2VD_BUF_SIZE_QPTAB);
    if (MPP_OK != ret) {
        mpp_err("m2v_hal qtable_base get buffer failed\n");
        return ret;
    }


    //init regs
    memset(&p->regs, 0, sizeof(M2VDRegSet));
    reg->config3.sw_dec_axi_rn_id = 0;
    reg->control.sw_dec_timeout_e = 1;
    reg->config2.sw_dec_strswap32_e = 1;     //change
    reg->config2.sw_dec_strendian_e = DEC_RKV_LITTLE_ENDIAN;
    reg->config2.sw_dec_inswap32_e = 1;      //change
    reg->config2.sw_dec_outswap32_e = 1; //change


    reg->control.sw_dec_clk_gate_e = 1;      //change
    reg->config2.sw_dec_in_endian = DEC_RKV_LITTLE_ENDIAN;  //change
    reg->config2.sw_dec_out_endian = DEC_RKV_LITTLE_ENDIAN;

    reg->config1.sw_dec_out_tiled_e = 0;
    reg->config3.sw_dec_max_burst = DEC_RKV_BUS_BURST_LENGTH_16;
    reg->config1.sw_dec_scmd_dis = 0;
    reg->config1.sw_dec_adv_pre_dis = 0;
    reg->error_position.sw_apf_threshold = 8;

    reg->config1.sw_dec_latency = 0;
    reg->config3.sw_dec_data_disc_e  = 0;

    reg->interrupt.sw_dec_irq = 0;
    reg->config3.sw_dec_axi_rn_id = 0;
    reg->config3.sw_dec_axi_wr_id = 0;

    reg->sw_dec_mode = 8;

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

    FUN_T("FUN_O");

    return ret;
}

MPP_RET hal_m2vd_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    FUN_T("FUN_I");
    M2VDHalContext *p = (M2VDHalContext *)hal;

#ifdef RKPLATFORM
    if (p->dev_ctx) {
        ret = mpp_device_deinit(p->dev_ctx);
        if (MPP_OK != ret) {
            mpp_err("mpp_device_deinit failed ret: %d\n", ret);
        }
    }
#endif
    if (p->qp_table) {
        ret = mpp_buffer_put(p->qp_table);
        if (MPP_OK != ret) {
            mpp_err("m2v_hal qp_table put buffer failed\n");
            return ret;
        }
    }

    if (p->group) {
        ret = mpp_buffer_group_put(p->group);
        if (MPP_OK != ret) {
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

    FUN_T("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    if (task->dec.valid) {
        void *q_table = NULL;
#ifdef RKPLATFORM
        MppBuffer streambuf = NULL;
        MppBuffer framebuf = NULL;
#endif
        M2VDHalContext *ctx = (M2VDHalContext *)hal;
        M2VDDxvaParam *dx = (M2VDDxvaParam *)task->dec.syntax.data;
        M2VDRegSet *pRegs = &ctx->regs;
        task->dec.valid = 0;
        q_table = mpp_buffer_get_ptr(ctx->qp_table);
        memcpy(q_table, dx->qp_tab, M2VD_BUF_SIZE_QPTAB);

        pRegs->ppReg[0] = 0;
        pRegs->dec_info.sw_mv_accuracy_fwd = 1;
        pRegs->dec_info.sw_mv_accuracy_bwd = 1;

        if (dx->seq_ext_head_dec_flag) {
            pRegs->sw_dec_mode = 5;
            pRegs->dec_info.sw_fcode_fwd_hor = dx->pic.full_pel_forward_vector;
            pRegs->dec_info.sw_fcode_fwd_ver = dx->pic.forward_f_code;
            pRegs->dec_info.sw_fcode_bwd_hor = dx->pic.full_pel_backward_vector;
            pRegs->dec_info.sw_fcode_bwd_ver = dx->pic.backward_f_code;

        } else {
            pRegs->sw_dec_mode = 6;
            pRegs->dec_info.sw_fcode_fwd_hor = dx->pic.forward_f_code;
            pRegs->dec_info.sw_fcode_fwd_ver = dx->pic.forward_f_code;
            pRegs->dec_info.sw_fcode_bwd_hor = dx->pic.backward_f_code;
            pRegs->dec_info.sw_fcode_bwd_ver = dx->pic.backward_f_code;
            if (dx->pic.full_pel_forward_vector)
                pRegs->dec_info.sw_mv_accuracy_fwd = 0;
            if (dx->pic.full_pel_backward_vector)
                pRegs->dec_info.sw_mv_accuracy_bwd = 0;
        }

        pRegs->pic_params.sw_pic_mb_width = (dx->seq.decode_width + 15) >> 4;
        pRegs->pic_params.sw_pic_mb_height_p = (dx->seq.decode_height + 15) >> 4;
        pRegs->control.sw_pic_interlace_e = 1 - dx->seq_ext.progressive_sequence;
        if (dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_FRAME)
            pRegs->control.sw_pic_fieldmode_e = 0;
        else {
            pRegs->control.sw_pic_fieldmode_e = 1;
            pRegs->control.sw_pic_topfield_e = dx->pic_code_ext.picture_structure == 1;
        }
        if (dx->pic.picture_coding_type == M2VD_CODING_TYPE_B)
            pRegs->control.sw_pic_b_e = 1;
        else
            pRegs->control.sw_pic_b_e = 0;
        if (dx->pic.picture_coding_type == M2VD_CODING_TYPE_I)
            pRegs->control.sw_pic_inter_e = 0;
        else
            pRegs->control.sw_pic_inter_e = 1;

        pRegs->pic_params.sw_topfieldfirst_e = dx->pic_code_ext.top_field_first;
        pRegs->control.sw_fwd_interlace_e = 0;
        pRegs->control.sw_write_mvs_e = 0;//concealment_motion_vectors;
        pRegs->pic_params.sw_alt_scan_e = dx->pic_code_ext.alternate_scan;
        pRegs->dec_info.sw_alt_scan_flag_e = dx->pic_code_ext.alternate_scan;

        pRegs->stream_bitinfo.sw_qscale_type = dx->pic_code_ext.q_scale_type;
        pRegs->stream_bitinfo.sw_intra_dc_prec = dx->pic_code_ext.intra_dc_precision;
        pRegs->stream_bitinfo.sw_con_mv_e = dx->pic_code_ext.concealment_motion_vectors;
        pRegs->stream_bitinfo.sw_intra_vlc_tab = dx->pic_code_ext.intra_vlc_format;
        pRegs->stream_bitinfo.sw_frame_pred_dct = dx->pic_code_ext.frame_pred_frame_dct;
        pRegs->stream_buffinfo.sw_init_qp = 1;
#ifdef RKPLATFORM //current strem in frame out config
        mpp_buf_slot_get_prop(ctx->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
        pRegs->VLC_base = mpp_buffer_get_fd(streambuf);
        pRegs->VLC_base |= (dx->bitstream_offset << 10);

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->CurrPic.Index7Bits, SLOT_BUFFER, &framebuf);


        if ((dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) ||
            (dx->pic_code_ext.picture_structure == M2VD_PIC_STRUCT_FRAME)) {
            pRegs->cur_pic_base = mpp_buffer_get_fd(framebuf); //just index need map
        } else {
            pRegs->cur_pic_base = mpp_buffer_get_fd(framebuf) | (((dx->seq.decode_width + 15) & (~15)) << 10);
        }

        //ref & qtable config
        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[0].Index7Bits, SLOT_BUFFER, &framebuf);
        pRegs->ref0  = mpp_buffer_get_fd(framebuf); //just index need map

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[1].Index7Bits, SLOT_BUFFER, &framebuf);
        pRegs->ref1  = mpp_buffer_get_fd(framebuf); //just index need map

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[2].Index7Bits, SLOT_BUFFER, &framebuf);
        pRegs->ref2  = mpp_buffer_get_fd(framebuf); //just index need map

        mpp_buf_slot_get_prop(ctx->frame_slots, dx->frame_refs[3].Index7Bits, SLOT_BUFFER, &framebuf);
        pRegs->ref3  = mpp_buffer_get_fd(framebuf); //just index need map

        pRegs->slice_table = mpp_buffer_get_fd(ctx->qp_table);
#endif
        pRegs->error_position.sw_startmb_x = 0;
        pRegs->error_position.sw_startmb_y = 0;
        pRegs->control.sw_dec_out_dis = 0;
        pRegs->config1.sw_filtering_dis = 1;

        pRegs->stream_buffinfo.sw_stream_len = dx->bitstream_length;
        pRegs->stream_bitinfo.sw_stream_start_bit = dx->bitstream_start_bit;
        pRegs->control.sw_dec_e = 1;

        if (M2VH_DBG_REG & m2vh_debug) {
            RK_U32 j = 0;
            RK_U32 *p_reg = (RK_U32 *)pRegs;
            for (j = 50; j < 159; j++) {
                mpp_log("reg[%d] = 0x%08x", j, p_reg[j]);
            }
        }
        if (ctx->fp_reg_in) {
            int k = 0;
            RK_U32 *p_reg = (RK_U32*)pRegs;
            mpp_log("fwrite regs start");
            fprintf(ctx->fp_reg_in, "Frame #%d\n", ctx->dec_frame_cnt);
            for (k = 0; k < M2VD_REG_NUM; k++)
                fprintf(ctx->fp_reg_in, "[(D)%03d, (X)%03x]  %08x\n", k, k, p_reg[k]);
            fflush(ctx->fp_reg_in);
        }

        task->dec.valid = 1;
        ctx->dec_frame_cnt++;
    }
    return ret;

}

MPP_RET hal_m2vd_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
#ifdef RKPLATFORM
    M2VDHalContext *ctx = (M2VDHalContext *)hal;
    RK_U32 *p_regs = (RK_U32 *)&ctx->regs;
    FUN_T("FUN_I");
    ret = mpp_device_send_reg(ctx->dev_ctx, p_regs, M2VD_REG_NUM);
    if (ret != 0) {
        mpp_err("mpp_device_send_reg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }
#endif
    (void)task;
    (void)hal;
    FUN_T("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
#ifdef RKPLATFORM
    M2VDRegSet reg_out;
    M2VDHalContext *ctx = (M2VDHalContext *)hal;

    FUN_T("FUN_I");
    memset(&reg_out, 0, sizeof(M2VDRegSet));
    ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)&reg_out, M2VD_REG_NUM);
    if (ctx->fp_reg_out) {
        int k = 0;
        RK_U32 *p_reg = (RK_U32*)&reg_out;
        fprintf(ctx->fp_reg_out, "Frame #%d\n", ctx->dec_frame_cnt);
        for (k = 0; k < M2VD_REG_NUM; k++)
            fprintf(ctx->fp_reg_out, "[(D)%03d, (X)%03x]  %08x\n", k, k, p_reg[k]);
        fflush(ctx->fp_reg_out);
    }
    if (reg_out.interrupt.sw_dec_error_int | reg_out.interrupt.sw_dec_buffer_int) {
        if (ctx->int_cb.callBack)
            ctx->int_cb.callBack(ctx->int_cb.opaque, NULL);
    }
    if (M2VH_DBG_IRQ & m2vh_debug)
        mpp_log("mpp_device_wait_reg return interrupt:%08x", reg_out.interrupt);
#endif
    if (ret) {
        mpp_log("mpp_device_wait_reg return error:%d", ret);
    }
    (void)hal;
    (void)task;
    FUN_T("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    FUN_T("FUN_I");
    (void)hal;
    FUN_T("FUN_O");
    return ret;
}

MPP_RET hal_m2vd_flush(void *hal)
{
    MPP_RET ret = MPP_OK;
    FUN_T("FUN_I");
    (void)hal;
    FUN_T("FUN_O");
    return ret;
}
MPP_RET hal_m2vd_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    FUN_T("FUN_I");
    (void)hal;
    (void)cmd_type;
    (void)param;
    FUN_T("FUN_O");
    return ret;
}

