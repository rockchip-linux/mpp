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

#define MODULE_TAG "hal_h264e_vepu2"

#include <string.h>
#include "mpp_device.h"

#include "rk_mpi.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "mpp_common.h"
#include "mpp_rc.h"

#include "hal_h264e_com.h"
#include "hal_h264e_header.h"
#include "hal_h264e_vpu_tbl.h"
#include "hal_h264e_vepu.h"
#include "hal_h264e_vepu2.h"
#include "hal_h264e_vepu2_reg_tbl.h"

static RK_U32 hal_vpu_h264e_debug = 0;

#ifdef H264E_DUMP_DATA_TO_FILE
static MPP_RET h264e_vpu_open_dump_files(void *dump_files)
{
    if (h264e_hal_log_mode & H264E_HAL_LOG_FILE) {
        char base_path[512];
        char full_path[512];
        H264eVpuDumpFiles *files = (H264eVpuDumpFiles *)dump_files;
        strcpy(base_path, "./");

        sprintf(full_path, "%s%s", base_path, "mpp_syntax_in.txt");
        files->fp_mpp_syntax_in = fopen(full_path, "wb");
        if (!files->fp_mpp_syntax_in) {
            mpp_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_reg_in.txt");
        files->fp_mpp_reg_in = fopen(full_path, "wb");
        if (!files->fp_mpp_reg_in) {
            mpp_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_reg_out.txt");
        files->fp_mpp_reg_out = fopen(full_path, "wb");
        if (!files->fp_mpp_reg_out) {
            mpp_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_feedback.txt");
        files->fp_mpp_feedback = fopen(full_path, "wb");
        if (!files->fp_mpp_feedback) {
            mpp_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_strm_out.bin");
        files->fp_mpp_strm_out = fopen(full_path, "wb");
        if (!files->fp_mpp_strm_out) {
            mpp_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }
    }

    return MPP_OK;
}


static MPP_RET h264e_vpu_close_dump_files(void *dump_files)
{
    H264eVpuDumpFiles *files = (H264eVpuDumpFiles *)dump_files;
    H264E_HAL_FCLOSE(files->fp_mpp_syntax_in);
    H264E_HAL_FCLOSE(files->fp_mpp_reg_in);
    H264E_HAL_FCLOSE(files->fp_mpp_reg_out);
    H264E_HAL_FCLOSE(files->fp_mpp_strm_out);
    H264E_HAL_FCLOSE(files->fp_mpp_feedback);
    return MPP_OK;
}

static void h264e_vpu_dump_mpp_syntax_in(H264eHwCfg *syn, H264eHalContext *ctx)
{
    H264eVpuDumpFiles *dump_files = (H264eVpuDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_syntax_in;
    if (fp) {
        RK_S32 k = 0;
        fprintf(fp, "#FRAME %d:\n", ctx->frame_cnt);
        fprintf(fp, "%-16d %s\n", syn->frame_type, "frame_coding_type");
        fprintf(fp, "%-16d %s\n", syn->slice_alpha_offset, "slice_alpha_offset");
        fprintf(fp, "%-16d %s\n", syn->slice_beta_offset, "slice_beta_offset");
        fprintf(fp, "%-16d %s\n", syn->filter_disable, "filter_disable");
        fprintf(fp, "%-16d %s\n", syn->idr_pic_id, "idr_pic_id");
        fprintf(fp, "%-16d %s\n", syn->frame_num, "frame_num");
        fprintf(fp, "%-16d %s\n", syn->slice_size_mb_rows, "slice_size_mb_rows");
        fprintf(fp, "%-16d %s\n", syn->inter4x4_disabled, "h264_inter4x4_disabled");
        fprintf(fp, "%-16d %s\n", syn->qp, "qp");
        fprintf(fp, "%-16d %s\n", syn->mad_qp_delta, "mad_qp_delta");
        fprintf(fp, "%-16d %s\n", syn->mad_threshold, "mad_threshold");
        fprintf(fp, "%-16d %s\n", syn->qp_min, "qp_min");
        fprintf(fp, "%-16d %s\n", syn->qp_max, "qp_max");
        fprintf(fp, "%-16d %s\n", syn->cp_distance_mbs, "cp_distance_mbs");
        for (k = 0; k < 10; k++)
            fprintf(fp, "%-16d cp_target[%d]\n", syn->cp_target[k], k);
        for (k = 0; k < 7; k++)
            fprintf(fp, "%-16d target_error[%d]\n", syn->target_error[k], k);
        for (k = 0; k < 7; k++)
            fprintf(fp, "%-16d delta_qp[%d]\n", syn->delta_qp[k], k);
        fprintf(fp, "%-16d %s\n", syn->output_strm_limit_size, "output_strm_limit_size");
        fprintf(fp, "%-16d %s\n", syn->width, "pic_luma_width");
        fprintf(fp, "%-16d %s\n", syn->height, "pic_luma_height");
        fprintf(fp, "0x%-14x %s\n", syn->input_luma_addr, "input_luma_addr");
        fprintf(fp, "0x%-14x %s\n", syn->input_cb_addr, "input_cb_addr");
        fprintf(fp, "0x%-16x %s\n", syn->input_cr_addr, "input_cr_addr");
        fprintf(fp, "%-16d %s\n", syn->input_format, "input_image_format");

        fprintf(fp, "%-16d %s\n", syn->color_conversion_coeff_a, "color_conversion_coeff_a");
        fprintf(fp, "%-16d %s\n", syn->color_conversion_coeff_b, "color_conversion_coeff_b");
        fprintf(fp, "%-16d %s\n", syn->color_conversion_coeff_c, "color_conversion_coeff_c");
        fprintf(fp, "%-16d %s\n", syn->color_conversion_coeff_e, "color_conversion_coeff_e");
        fprintf(fp, "%-16d %s\n", syn->color_conversion_coeff_f, "color_conversion_coeff_f");

        fprintf(fp, "\n");
        fflush(fp);
    } else {
        mpp_log("try to dump data to mpp_syntax_in.txt, but file is not opened");
    }
}

static void h264e_vpu_dump_mpp_reg_in(H264eHalContext *ctx)
{
    H264eVpuDumpFiles *dump_files = (H264eVpuDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_reg_in;
    if (fp) {
        RK_S32 k = 0;
        RK_U32 *reg = (RK_U32 *)ctx->regs;
        fprintf(fp, "#FRAME %d:\n", ctx->frame_cnt);
        for (k = 0; k < VEPU2_H264E_NUM_REGS; k++) {
            fprintf(fp, "reg[%03d/%03x]: %08x\n", k, k * 4, reg[k]);
            //mpp_log("reg[%03d/%03x]: %08x", k, k*4, reg[k]);
        }
        fprintf(fp, "\n");
    } else {
        mpp_log("try to dump data to mpp_reg_in.txt, but file is not opened");
    }
}

static void h264e_vpu_dump_mpp_reg_out(H264eHalContext *ctx)
{
    H264eVpuDumpFiles *dump_files = (H264eVpuDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_reg_out;
    if (fp) {
        RK_S32 k = 0;
        RK_U32 *reg = (RK_U32 *)ctx->regs;
        fprintf(fp, "#FRAME %d:\n", ctx->frame_cnt - 1);
        for (k = 0; k < VEPU2_H264E_NUM_REGS; k++) {
            fprintf(fp, "reg[%03d/%03x]: %08x\n", k, k * 4, reg[k]);
            //mpp_log("reg[%03d/%03x]: %08x", k, k*4, reg[k]);
        }
        fprintf(fp, "\n");
    } else {
        mpp_log("try to dump data to mpp_reg_in.txt, but file is not opened");
    }
}

static void h264e_vpu_dump_mpp_feedback(H264eHalContext *ctx)
{
    H264eVpuDumpFiles *dump_files = (H264eVpuDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_feedback;
    if (fp) {
        RK_S32 k = 0;
        h264e_feedback *fb = &ctx->feedback;
        fprintf(fp, "#FRAME %d:\n", ctx->frame_cnt - 1);
        fprintf(fp, "%-16d %s\n", fb->hw_status, "hw_status");
        fprintf(fp, "%-16d %s\n", fb->out_strm_size, "out_strm_size");
        fprintf(fp, "%-16d %s\n", fb->qp_sum, "qp_sum");
        for (k = 0; k < 10; k++)
            fprintf(fp, "%-16d cp[%d]\n", fb->cp[k], k);
        fprintf(fp, "%-16d %s\n", fb->mad_count, "mad_count");
        fprintf(fp, "%-16d %s\n", fb->rlc_count, "rlc_count");

        fprintf(fp, "\n");
        fflush(fp);
    } else {
        mpp_log("try to dump data to mpp_feedback.txt, but file is not opened");
    }
}

static void h264e_vpu_dump_mpp_strm_out_header(H264eHalContext *ctx, MppPacket packet)
{
    H264eVpuDumpFiles *dump_files = (H264eVpuDumpFiles *)ctx->dump_files;
    void *ptr   = mpp_packet_get_data(packet);
    size_t len  = mpp_packet_get_length(packet);
    FILE *fp = dump_files->fp_mpp_strm_out;

    if (fp) {
        fwrite(ptr, 1, len, fp);
        fflush(fp);
    } else {
        mpp_log("try to dump strm header to mpp_strm_out.txt, but file is not opened");
    }
}

void h264e_vpu_dump_mpp_strm_out(H264eHalContext *ctx, MppBuffer hw_buf)
{
    H264eVpuDumpFiles *dump_files = (H264eVpuDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_strm_out;
    if (fp && hw_buf) {
        RK_U32 *reg_val = (RK_U32 *)ctx->regs;
        RK_U32 strm_size = reg_val[VEPU_REG_STR_BUF_LIMIT / 4] / 8;

        RK_U8 *hw_buf_vir_addr = (RK_U8 *)mpp_buffer_get_ptr(hw_buf);

        mpp_log("strm_size: %d", strm_size);

        fwrite(hw_buf_vir_addr, 1, strm_size, fp);
        fflush(fp);
    } else {
        mpp_log("try to dump data to mpp_strm_out.txt, but file is not opened");
    }
}
#endif

MPP_RET hal_h264e_vepu2_init(void *hal, MppHalCfg *cfg)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MPP_RET ret = MPP_OK;

    h264e_hal_enter();

    ctx->int_cb     = cfg->hal_int_cb;
    ctx->regs       = mpp_calloc(H264eVpu2RegSet, 1);
    ctx->buffers    = mpp_calloc(h264e_hal_vpu_buffers, 1);
    ctx->extra_info = mpp_calloc(H264eVpuExtraInfo, 1);
    ctx->dump_files = mpp_calloc(H264eVpuDumpFiles, 1);
    ctx->param_buf  = mpp_calloc_size(void,  H264E_EXTRA_INFO_BUF_SIZE);
    mpp_packet_init(&ctx->packeted_param, ctx->param_buf, H264E_EXTRA_INFO_BUF_SIZE);

    h264e_vpu_init_extra_info(ctx->extra_info);
#ifdef H264E_DUMP_DATA_TO_FILE
    h264e_vpu_open_dump_files(ctx->dump_files);
#endif

    ctx->vpu_fd = -1;

#ifdef RKPLATFORM
    ctx->vpu_fd = mpp_device_init(&ctx->dev_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
    if (ctx->vpu_fd <= 0) {
        mpp_err("get vpu_socket(%d) <=0, failed. \n", ctx->vpu_fd);
        ret = MPP_ERR_UNKNOW;
    }
#endif
    ctx->hw_cfg.qp_prev = ctx->cfg->codec.h264.qp_init;
    mpp_env_get_u32("hal_vpu_h264e_debug", &hal_vpu_h264e_debug, 0);

    h264e_hal_leave();
    return ret;
}

MPP_RET hal_h264e_vepu2_deinit(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->param_buf);

    if (ctx->extra_info) {
        h264e_vpu_deinit_extra_info(ctx->extra_info);
        MPP_FREE(ctx->extra_info);
    }

    if (ctx->packeted_param) {
        mpp_packet_deinit(&ctx->packeted_param);
        ctx->packeted_param = NULL;
    }

    if (ctx->buffers) {
        h264e_vpu_free_buffers(ctx);
        MPP_FREE(ctx->buffers);
    }

    if (ctx->intra_qs) {
        mpp_linreg_deinit(ctx->intra_qs);
        ctx->intra_qs = NULL;
    }

    if (ctx->inter_qs) {
        mpp_linreg_deinit(ctx->inter_qs);
        ctx->inter_qs = NULL;
    }

#ifdef H264E_DUMP_DATA_TO_FILE
    if (ctx->dump_files) {
        h264e_vpu_close_dump_files(ctx->dump_files);
        MPP_FREE(ctx->dump_files);
    }
#endif

#ifdef RKPLATFORM
    if (ctx->vpu_fd <= 0) {
        mpp_err("invalid vpu socket: %d", ctx->vpu_fd);
        return MPP_NOK;
    }

    if (mpp_device_deinit(ctx->vpu_fd)) {
        mpp_err("mpp_device_deinit failed");
        return MPP_ERR_VPUHW;
    }
#endif

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu2_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eVpuExtraInfo *extra_info = (H264eVpuExtraInfo *)ctx->extra_info;
    H264ePps *pps = &extra_info->pps;
    h264e_hal_vpu_buffers *bufs = (h264e_hal_vpu_buffers *)ctx->buffers;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_U32 *reg = (RK_U32 *)ctx->regs;
    HalEncTask *enc_task = &task->enc;

    RK_S32 scaler = 0, i = 0;
    RK_U32 val = 0, skip_penalty = 0;
    RK_U32 overfill_r = 0, overfill_b = 0;

    RK_U32 mb_w;
    RK_U32 mb_h;

    h264e_hal_enter();

    // generate parameter from config
    h264e_vpu_update_hw_cfg(ctx, enc_task, hw_cfg);
#ifdef H264E_DUMP_DATA_TO_FILE
    h264e_vpu_dump_mpp_syntax_in(hw_cfg, ctx);
#endif

    // prepare buffer
    if (!ctx->buffer_ready) {
        ret = h264e_vpu_allocate_buffers(ctx, hw_cfg);
        if (ret) {
            h264e_hal_err("allocate buffers failed\n");
            h264e_vpu_free_buffers(ctx);
            return ret;
        } else
            ctx->buffer_ready = 1;
    }

    mb_w = (prep->width  + 15) / 16;
    mb_h = (prep->height + 15) / 16;

    memset(reg, 0, sizeof(H264eVpu2RegSet));

    h264e_hal_dbg(H264E_DBG_DETAIL, "frame %d generate regs now", ctx->frame_cnt);

    /* output buffer size is 64 bit address then 8 multiple size */
    val = mpp_buffer_get_size(enc_task->output);
    val >>= 3;
    val &= ~7;
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_BUF_LIMIT, val);

    /*
     * The hardware needs only the value for luma plane, because
     * values of other planes are calculated internally based on
     * format setting.
     */
    val = VEPU_REG_INTRA_AREA_TOP(mb_h)
          | VEPU_REG_INTRA_AREA_BOTTOM(mb_h)
          | VEPU_REG_INTRA_AREA_LEFT(mb_w)
          | VEPU_REG_INTRA_AREA_RIGHT(mb_w);
    H264E_HAL_SET_REG(reg, VEPU_REG_INTRA_AREA_CTRL, val); //FIXED
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_HDR_REM_MSB, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_STR_HDR_REM_LSB, 0);


    val = VEPU_REG_AXI_CTRL_READ_ID(0);
    val |= VEPU_REG_AXI_CTRL_WRITE_ID(0);
    val |= VEPU_REG_AXI_CTRL_BURST_LEN(16);
    val |= VEPU_REG_AXI_CTRL_INCREMENT_MODE(0);
    val |= VEPU_REG_AXI_CTRL_BIRST_DISCARD(0);
    H264E_HAL_SET_REG(reg, VEPU_REG_AXI_CTRL, val);

    H264E_HAL_SET_REG(reg, VEPU_QP_ADJUST_MAD_DELTA_ROI, hw_cfg->mad_qp_delta);

    val = 0;
    if (mb_w * mb_h > 3600)
        val = VEPU_REG_DISABLE_QUARTER_PIXEL_MV;
    val |= VEPU_REG_CABAC_INIT_IDC(hw_cfg->cabac_init_idc);
    if (pps->b_cabac)
        val |= VEPU_REG_ENTROPY_CODING_MODE;
    if (pps->b_transform_8x8_mode)
        val |= VEPU_REG_H264_TRANS8X8_MODE;
    if (hw_cfg->inter4x4_disabled)
        val |= VEPU_REG_H264_INTER4X4_MODE;
    /*reg |= VEPU_REG_H264_STREAM_MODE;*/
    val |= VEPU_REG_H264_SLICE_SIZE(hw_cfg->slice_size_mb_rows);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL0, val);

    scaler = H264E_HAL_MAX(1, 200 / (mb_w + mb_h));
    skip_penalty = H264E_HAL_MIN(255, h264_skip_sad_penalty[hw_cfg->qp] * scaler);
    skip_penalty = 0xff;
    if (prep->width & 0x0f)
        overfill_r = (16 - (prep->width & 0x0f) ) / 4;
    if (prep->height & 0x0f)
        overfill_b = 16 - (prep->height & 0x0f);
    val = VEPU_REG_STREAM_START_OFFSET(0) | /* first_free_bit */
          VEPU_REG_SKIP_MACROBLOCK_PENALTY(skip_penalty) |
          VEPU_REG_IN_IMG_CTRL_OVRFLR_D4(overfill_r) |
          VEPU_REG_IN_IMG_CTRL_OVRFLB(overfill_b);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_OVER_FILL_STRM_OFFSET, val);


    val = VEPU_REG_IN_IMG_CHROMA_OFFSET(0)
          | VEPU_REG_IN_IMG_LUMA_OFFSET(0)
          | VEPU_REG_IN_IMG_CTRL_ROW_LEN(prep->width);
    H264E_HAL_SET_REG(reg, VEPU_REG_INPUT_LUMA_INFO, val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[0])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(0), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[2])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[3]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(1), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[4])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[5]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(2), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[6])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[7]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(3), val);

    val = VEPU_REG_CHECKPOINT_CHECK1(hw_cfg->cp_target[8])
          | VEPU_REG_CHECKPOINT_CHECK0(hw_cfg->cp_target[9]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHECKPOINT(4), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_cfg->target_error[0])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_cfg->target_error[1]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(0), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_cfg->target_error[2])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_cfg->target_error[3]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(1), val);

    val = VEPU_REG_CHKPT_WORD_ERR_CHK1(hw_cfg->target_error[4])
          | VEPU_REG_CHKPT_WORD_ERR_CHK0(hw_cfg->target_error[5]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_WORD_ERR(2), val);

    val = VEPU_REG_CHKPT_DELTA_QP_CHK6(hw_cfg->delta_qp[6])
          | VEPU_REG_CHKPT_DELTA_QP_CHK5(hw_cfg->delta_qp[5])
          | VEPU_REG_CHKPT_DELTA_QP_CHK4(hw_cfg->delta_qp[4])
          | VEPU_REG_CHKPT_DELTA_QP_CHK3(hw_cfg->delta_qp[3])
          | VEPU_REG_CHKPT_DELTA_QP_CHK2(hw_cfg->delta_qp[2])
          | VEPU_REG_CHKPT_DELTA_QP_CHK1(hw_cfg->delta_qp[1])
          | VEPU_REG_CHKPT_DELTA_QP_CHK0(hw_cfg->delta_qp[0]);
    H264E_HAL_SET_REG(reg, VEPU_REG_CHKPT_DELTA_QP, val);

    val = VEPU_REG_MAD_THRESHOLD(hw_cfg->mad_threshold)
          | VEPU_REG_IN_IMG_CTRL_FMT(hw_cfg->input_format)
          | VEPU_REG_IN_IMG_ROTATE_MODE(0)
          | VEPU_REG_SIZE_TABLE_PRESENT; //FIXED
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL1, val);

    val = VEPU_REG_INTRA16X16_MODE(h264_intra16_favor[hw_cfg->qp])
          | VEPU_REG_INTER_MODE(h264_inter_favor[hw_cfg->qp]);
    H264E_HAL_SET_REG(reg, VEPU_REG_INTRA_INTER_MODE, val);

    val = VEPU_REG_PPS_INIT_QP(pps->i_pic_init_qp)
          | VEPU_REG_SLICE_FILTER_ALPHA(hw_cfg->slice_alpha_offset)
          | VEPU_REG_SLICE_FILTER_BETA(hw_cfg->slice_beta_offset)
          | VEPU_REG_CHROMA_QP_OFFSET(pps->i_chroma_qp_index_offset)
          | VEPU_REG_IDR_PIC_ID(hw_cfg->idr_pic_id);

    if (hw_cfg->filter_disable)
        val |= VEPU_REG_FILTER_DISABLE;

    if (pps->b_constrained_intra_pred)
        val |= VEPU_REG_CONSTRAINED_INTRA_PREDICTION;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL2, val);

    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_NEXT_PIC, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_MV_OUT, 0);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_CABAC_TBL, mpp_buffer_get_fd(bufs->hw_cabac_table_buf));

    val = VEPU_REG_ROI1_TOP_MB(mb_h)
          | VEPU_REG_ROI1_BOTTOM_MB(mb_h)
          | VEPU_REG_ROI1_LEFT_MB(mb_w)
          | VEPU_REG_ROI1_RIGHT_MB(mb_w);
    H264E_HAL_SET_REG(reg, VEPU_REG_ROI1, val); //FIXED

    val = VEPU_REG_ROI2_TOP_MB(mb_h)
          | VEPU_REG_ROI2_BOTTOM_MB(mb_h)
          | VEPU_REG_ROI2_LEFT_MB(mb_w)
          | VEPU_REG_ROI2_RIGHT_MB(mb_w);
    H264E_HAL_SET_REG(reg, VEPU_REG_ROI2, val); //FIXED
    H264E_HAL_SET_REG(reg, VEPU_REG_STABLILIZATION_OUTPUT, 0);

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFB(hw_cfg->color_conversion_coeff_b)
          | VEPU_REG_RGB2YUV_CONVERSION_COEFA(hw_cfg->color_conversion_coeff_a);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF1, val); //FIXED

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFE(hw_cfg->color_conversion_coeff_e)
          | VEPU_REG_RGB2YUV_CONVERSION_COEFC(hw_cfg->color_conversion_coeff_c);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF2, val); //FIXED

    val = VEPU_REG_RGB2YUV_CONVERSION_COEFF(hw_cfg->color_conversion_coeff_f);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB2YUV_CONVERSION_COEF3, val); //FIXED

    val = VEPU_REG_RGB_MASK_B_MSB(hw_cfg->b_mask_msb)
          | VEPU_REG_RGB_MASK_G_MSB(hw_cfg->g_mask_msb)
          | VEPU_REG_RGB_MASK_R_MSB(hw_cfg->r_mask_msb);
    H264E_HAL_SET_REG(reg, VEPU_REG_RGB_MASK_MSB, val); //FIXED

    {
        RK_U32 diff_mv_penalty[3] = {0};
        diff_mv_penalty[0] = h264_diff_mv_penalty4p[hw_cfg->qp];
        diff_mv_penalty[1] = h264_diff_mv_penalty[hw_cfg->qp];
        diff_mv_penalty[2] = h264_diff_mv_penalty[hw_cfg->qp];

        val = VEPU_REG_1MV_PENALTY(diff_mv_penalty[1])
              | VEPU_REG_QMV_PENALTY(diff_mv_penalty[2])
              | VEPU_REG_4MV_PENALTY(diff_mv_penalty[0]);
    }

    val |= VEPU_REG_SPLIT_MV_MODE_EN;
    H264E_HAL_SET_REG(reg, VEPU_REG_MV_PENALTY, val);

    val = VEPU_REG_H264_LUMA_INIT_QP(hw_cfg->qp)
          | VEPU_REG_H264_QP_MAX(hw_cfg->qp_max)
          | VEPU_REG_H264_QP_MIN(hw_cfg->qp_min)
          | VEPU_REG_H264_CHKPT_DISTANCE(hw_cfg->cp_distance_mbs);
    H264E_HAL_SET_REG(reg, VEPU_REG_QP_VAL, val);

    val = VEPU_REG_ZERO_MV_FAVOR_D2(10);
    H264E_HAL_SET_REG(reg, VEPU_REG_MVC_RELATE, val);

    if (hw_cfg->input_format < H264E_VPU_CSP_RGB565) {
        val = VEPU_REG_OUTPUT_SWAP32
              | VEPU_REG_OUTPUT_SWAP16
              | VEPU_REG_OUTPUT_SWAP8
              | VEPU_REG_INPUT_SWAP8
              | VEPU_REG_INPUT_SWAP16
              | VEPU_REG_INPUT_SWAP32;
    } else if (hw_cfg->input_format == H264E_VPU_CSP_ARGB8888) {
        val = VEPU_REG_OUTPUT_SWAP32
              | VEPU_REG_OUTPUT_SWAP16
              | VEPU_REG_OUTPUT_SWAP8
              | VEPU_REG_INPUT_SWAP32;
    } else {
        val = VEPU_REG_OUTPUT_SWAP32
              | VEPU_REG_OUTPUT_SWAP16
              | VEPU_REG_OUTPUT_SWAP8
              | VEPU_REG_INPUT_SWAP16
              | VEPU_REG_INPUT_SWAP32;
    }
    H264E_HAL_SET_REG(reg, VEPU_REG_DATA_ENDIAN, val);

    val = VEPU_REG_PPS_ID(pps->i_id)
          | VEPU_REG_INTRA_PRED_MODE(h264_prev_mode_favor[hw_cfg->qp])
          | VEPU_REG_FRAME_NUM(hw_cfg->frame_num);
    H264E_HAL_SET_REG(reg, VEPU_REG_ENC_CTRL3, val);

    val = VEPU_REG_INTERRUPT_TIMEOUT_EN;
    H264E_HAL_SET_REG(reg, VEPU_REG_INTERRUPT, val);

    {
        RK_U8 dmv_penalty[128] = {0};
        RK_U8 dmv_qpel_penalty[128] = {0};

        for (i = 0; i < 128; i++) {
            dmv_penalty[i] = i;
            dmv_qpel_penalty[i] = H264E_HAL_MIN(255, exp_golomb_signed(i));
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
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_LUMA, hw_cfg->input_luma_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_CB, hw_cfg->input_cb_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_IN_CR, hw_cfg->input_cr_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_OUTPUT_STREAM, hw_cfg->output_strm_addr);
    H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_OUTPUT_CTRL, mpp_buffer_get_fd(bufs->hw_nal_size_table_buf));

    {
        RK_U32 buf2_idx = ctx->frame_cnt & 1;
        RK_S32 recon_chroma_addr = 0, ref_chroma_addr = 0;
        RK_U32 frame_luma_size = mb_h * mb_w * 256;
        RK_S32 recon_luma_addr = mpp_buffer_get_fd(bufs->hw_rec_buf[buf2_idx]);
        RK_S32 ref_luma_addr = mpp_buffer_get_fd(bufs->hw_rec_buf[1 - buf2_idx]);

        recon_chroma_addr = recon_luma_addr | (frame_luma_size << 10);
        ref_chroma_addr   = ref_luma_addr   | (frame_luma_size << 10);

        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REC_LUMA, recon_luma_addr);
        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REC_CHROMA, recon_chroma_addr);
        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REF_LUMA, ref_luma_addr);
        H264E_HAL_SET_REG(reg, VEPU_REG_ADDR_REF_CHROMA , ref_chroma_addr);
    }


    /* set important encode mode info */
    val = VEPU_REG_MB_HEIGHT(mb_h)
          | VEPU_REG_MB_WIDTH(mb_w)
          | VEPU_REG_PIC_TYPE(hw_cfg->frame_type)
          | VEPU_REG_ENCODE_FORMAT(3)
          | VEPU_REG_ENCODE_ENABLE;
    H264E_HAL_SET_REG(reg, VEPU_REG_ENCODE_START, val);

#ifdef H264E_DUMP_DATA_TO_FILE
    h264e_vpu_dump_mpp_reg_in(ctx);
#endif

    ctx->frame_cnt++;
    hw_cfg->frame_num++;
    if (hw_cfg->frame_type == H264E_VPU_FRAME_I)
        hw_cfg->idr_pic_id++;
    if (hw_cfg->idr_pic_id == 16)
        hw_cfg->idr_pic_id = 0;

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu2_start(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    (void)task;
    h264e_hal_enter();
#ifdef RKPLATFORM
    if (ctx->vpu_fd > 0) {
        RK_U32 *p_regs = (RK_U32 *)ctx->regs;
        h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client is sending %d regs", VEPU2_H264E_NUM_REGS);
        if (MPP_OK != mpp_device_send_reg(ctx->vpu_fd, p_regs, VEPU2_H264E_NUM_REGS)) {
            mpp_err("mpp_device_send_reg Failed!!!");
            return MPP_ERR_VPUHW;
        } else {
            h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_send_reg successfully!");
        }
    } else {
        mpp_err("invalid vpu socket: %d", ctx->vpu_fd);
        return MPP_NOK;
    }
#endif
    (void)ctx;
    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_vpu_set_feedback(h264e_feedback *fb, H264eVpu2RegSet *reg)
{
    RK_S32 i = 0;
    RK_U32 cpt_prev = 0, overflow = 0;
    RK_U32 cpt_idx = VEPU_REG_CHECKPOINT(0) / 4;
    RK_U32 *reg_val = (RK_U32 *)reg;
    fb->hw_status = reg_val[VEPU_REG_INTERRUPT / 4];
    fb->qp_sum = ((reg_val[VEPU_REG_QP_SUM_DIV2 / 4] >> 11) & 0x001fffff) * 2;
    fb->mad_count = (reg_val[VEPU_REG_MB_CTRL / 4] >> 16) & 0xffff;
    fb->rlc_count = reg_val[VEPU_REG_RLC_SUM / 4] & 0x3fffff;
    fb->out_strm_size = reg_val[VEPU_REG_STR_BUF_LIMIT / 4] / 8;
    for (i = 0; i < 10; i++) {
        RK_U32 cpt = VEPU_REG_CHECKPOINT_RESULT(reg_val[cpt_idx]);
        if (cpt < cpt_prev)
            overflow += (1 << 21);
        fb->cp[i] = cpt + overflow;
        cpt_idx += (i & 1);
    }

    return MPP_OK;
}

MPP_RET hal_h264e_vepu2_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eVpu2RegSet *reg_out = (H264eVpu2RegSet *)ctx->regs;
    MppEncPrepCfg *prep = &ctx->set->prep;
    IOInterruptCB int_cb = ctx->int_cb;
    h264e_feedback *fb = &ctx->feedback;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16) * MPP_ALIGN(prep->height, 16) / 16 / 16;

    h264e_hal_enter();

#ifdef RKPLATFORM
    if (ctx->vpu_fd > 0) {
        RK_S32 hw_ret = mpp_device_wait_reg(ctx->vpu_fd, (RK_U32 *)reg_out,
                                            VEPU2_H264E_NUM_REGS);

        h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_wait_reg: ret %d\n", hw_ret);

        if (hw_ret != MPP_OK) {
            mpp_err("hardware returns error:%d", hw_ret);
            return MPP_ERR_VPUHW;
        }
    } else {
        mpp_err("invalid vpu socket: %d", ctx->vpu_fd);
        return MPP_NOK;
    }
#endif

    h264e_vpu_set_feedback(fb, reg_out);
#ifdef H264E_DUMP_DATA_TO_FILE
    h264e_vpu_dump_mpp_feedback(ctx);
#endif
    task->enc.length = fb->out_strm_size;
    if (int_cb.callBack) {
        RcSyntax *syn = (RcSyntax *)task->enc.syntax.data;
        RcHalResult result;
        RK_S32 avg_qp = fb->qp_sum / num_mb;

        mpp_assert(avg_qp >= 0);
        mpp_assert(avg_qp <= 51);

        result.bits = fb->out_strm_size * 8;
        result.type = syn->type;
        fb->result = &result;

        mpp_save_regdata((syn->type == INTRA_FRAME) ?
                         (ctx->intra_qs) :
                         (ctx->inter_qs),
                         h264_q_step[avg_qp], result.bits);
        mpp_linreg_update((syn->type == INTRA_FRAME) ?
                          (ctx->intra_qs) :
                          (ctx->inter_qs));

        int_cb.callBack(int_cb.opaque, fb);
    }

#ifdef H264E_DUMP_DATA_TO_FILE
    h264e_vpu_dump_mpp_reg_out(ctx);
    h264e_vpu_dump_mpp_strm_out(ctx, task->enc.output);
#endif
    //h264e_vpu_dump_mpp_strm_out(ctx, NULL);

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_vepu2_reset(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu2_flush(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vepu2_control(void *hal, RK_S32 cmd_type, void *param)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    h264e_hal_dbg(H264E_DBG_DETAIL, "h264e_vpu_control cmd 0x%x, info %p", cmd_type, param);
    switch (cmd_type) {
    case MPP_ENC_SET_EXTRA_INFO: {
        break;
    }
    case MPP_ENC_GET_EXTRA_INFO: {
        MppPacket  pkt      = ctx->packeted_param;
        MppPacket *pkt_out  = (MppPacket *)param;

        H264eVpuExtraInfo *src = (H264eVpuExtraInfo *)ctx->extra_info;
        H264eVpuStream *sps_stream = &src->sps_stream;
        H264eVpuStream *pps_stream = &src->pps_stream;
        H264eVpuStream *sei_stream = &src->sei_stream;

        size_t offset = 0;

        h264e_vpu_set_extra_info(ctx);

        mpp_packet_write(pkt, offset, sps_stream->buffer, sps_stream->byte_cnt);
        offset += sps_stream->byte_cnt;

        mpp_packet_write(pkt, offset, pps_stream->buffer, pps_stream->byte_cnt);
        offset += pps_stream->byte_cnt;

        mpp_packet_write(pkt, offset, sei_stream->buffer, sei_stream->byte_cnt);
        offset += sei_stream->byte_cnt;

        mpp_packet_set_length(pkt, offset);

        *pkt_out = pkt;
#ifdef H264E_DUMP_DATA_TO_FILE
        h264e_vpu_dump_mpp_strm_out_header(ctx, pkt);
#endif
        break;
    }

    case MPP_ENC_SET_PREP_CFG : {
        MppEncPrepCfg *set = &ctx->set->prep;
        RK_U32 change = set->change;
        MPP_RET ret = MPP_NOK;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            if ((set->width < 0 || set->width > 1920) ||
                (set->height < 0 || set->height > 3840) ||
                (set->hor_stride < 0 || set->hor_stride > 3840) ||
                (set->ver_stride < 0 || set->ver_stride > 3840)) {
                mpp_err("invalid input w:h [%d:%d] [%d:%d]\n",
                        set->width, set->height,
                        set->hor_stride, set->ver_stride);
                return ret;
            }
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            if ((set->format < MPP_FRAME_FMT_RGB &&
                 set->format >= MPP_FMT_YUV_BUTT) ||
                set->format >= MPP_FMT_RGB_BUTT) {
                mpp_err("invalid format %d\n", set->format);
                return ret;
            }
        }

        /* Update SPS/PPS information */
        h264e_vpu_set_extra_info(ctx);
        break;
    }
    case MPP_ENC_SET_RC_CFG : {
        // TODO: do rate control check here
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncH264Cfg *src = &ctx->set->codec.h264;
        MppEncH264Cfg *dst = &ctx->cfg->codec.h264;
        RK_U32 change = src->change;

        // TODO: do codec check first

        if (change & MPP_ENC_H264_CFG_STREAM_TYPE)
            dst->stream_type = src->stream_type;
        if (change & MPP_ENC_H264_CFG_CHANGE_PROFILE) {
            dst->profile = src->profile;
            dst->level = src->level;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_ENTROPY) {
            dst->entropy_coding_mode = src->entropy_coding_mode;
            dst->cabac_init_idc = src->cabac_init_idc;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_TRANS_8x8)
            dst->transform8x8_mode = src->transform8x8_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CONST_INTRA)
            dst->constrained_intra_pred_mode = src->constrained_intra_pred_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CHROMA_QP) {
            dst->chroma_cb_qp_offset = src->chroma_cb_qp_offset;
            dst->chroma_cr_qp_offset = src->chroma_cr_qp_offset;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_DEBLOCKING) {
            dst->deblock_disable = src->deblock_disable;
            dst->deblock_offset_alpha = src->deblock_offset_alpha;
            dst->deblock_offset_beta = src->deblock_offset_beta;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_LONG_TERM)
            dst->use_longterm = src->use_longterm;
        if (change & MPP_ENC_H264_CFG_CHANGE_QP_LIMIT) {
            dst->qp_init = src->qp_init;
            dst->qp_max = src->qp_max;
            dst->qp_min = src->qp_min;
            dst->qp_max_step = src->qp_max_step;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH) {
            dst->intra_refresh_mode = src->intra_refresh_mode;
            dst->intra_refresh_arg = src->intra_refresh_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SLICE_MODE) {
            dst->slice_mode = src->slice_mode;
            dst->slice_arg = src->slice_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_VUI) {
            dst->vui = src->vui;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SEI) {
            dst->sei = src->sei;
        }

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    } break;

    case MPP_ENC_SET_OSD_PLT_CFG:
    case MPP_ENC_SET_OSD_DATA_CFG: {
        break;
    }
    case MPP_ENC_SET_SEI_CFG: {
        ctx->sei_mode = *((MppEncSeiMode *)param);
        break;
    }
    default : {
        mpp_err("unrecognizable cmd type %x", cmd_type);
    } break;
    }

    h264e_hal_leave();
    return MPP_OK;
}
