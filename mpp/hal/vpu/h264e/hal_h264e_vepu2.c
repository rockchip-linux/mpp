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

#define MODULE_TAG "hal_h264e_vpu"

#include <string.h>
#include <limits.h>
#include "vpu.h"

#include "rk_mpi.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "mpp_common.h"
#include "mpp_rc.h"

#include "hal_h264e_com.h"
#include "hal_h264e_vepu2.h"
#include "hal_h264e_vpu_tbl.h"

//#define H264E_DUMP_DATA_TO_FILE

typedef struct h264e_hal_vpu_buffers_t {
    MppBufferGroup hw_buf_grp;

    MppBuffer hw_rec_buf[2];
    MppBuffer hw_cabac_table_buf;
    MppBuffer hw_nal_size_table_buf;
} h264e_hal_vpu_buffers;

#define HAL_VPU_H264E_DBG_FUNCTION          (0x00000001)
#define HAL_VPU_H264E_DBG_QP                (0x00000010)

#define hal_vpu_h264e_dbg(flag, fmt, ...)   _mpp_dbg(hal_vpu_h264e_debug, flag, fmt, ## __VA_ARGS__)
#define hal_vpu_h264e_dbg_f(flag, fmt, ...) _mpp_dbg_f(hal_vpu_h264e_debug, flag, fmt, ## __VA_ARGS__)

#define hal_vpu_h264e_dbg_func(fmt, ...)    hal_vpu_h264e_dbg_f(HAL_VPU_H264E_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define hal_vpu_h264e_dbg_qp(fmt, ...)      hal_vpu_h264e_dbg(HAL_VPU_H264E_DBG_QP, fmt, ## __VA_ARGS__)

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
        for (k = 0; k < VEPU_H264E_NUM_REGS; k++) {
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
        for (k = 0; k < VEPU_H264E_NUM_REGS; k++) {
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

static MPP_RET h264e_vpu_free_buffers(H264eHalContext *ctx)
{
    MPP_RET ret = MPP_OK;
    RK_S32 k = 0;
    h264e_hal_vpu_buffers *p = (h264e_hal_vpu_buffers *)ctx->buffers;
    h264e_hal_enter();

    if (p->hw_cabac_table_buf) {
        ret = mpp_buffer_put(p->hw_cabac_table_buf);
        if (ret)
            mpp_err("hw_cabac_table_buf put failed ret %d\n", ret);

        p->hw_cabac_table_buf = NULL;
    }

    if (p->hw_nal_size_table_buf) {
        ret = mpp_buffer_put(p->hw_nal_size_table_buf);
        if (ret)
            mpp_err("hw_nal_size_table_buf put failed ret %d\n", ret);

        p->hw_nal_size_table_buf = NULL;
    }


    for (k = 0; k < 2; k++) {
        if (p->hw_rec_buf[k]) {
            ret = mpp_buffer_put(p->hw_rec_buf[k]);
            if (ret)
                mpp_err("hw_rec_buf[%d] put failed ret %d\n", k, ret);

            p->hw_rec_buf[k] = NULL;
        }
    }

    if (p->hw_buf_grp) {
        ret = mpp_buffer_group_put(p->hw_buf_grp);
        if (ret)
            mpp_err("buf group put failed ret %d\n", ret);

        p->hw_buf_grp = NULL;
    }

    h264e_hal_leave();

    return ret;
}

static void h264e_vpu_swap_endian(RK_U32 *buf, RK_S32 size_bytes)
{
    RK_U32 i = 0;
    RK_S32 words = size_bytes / 4;
    RK_U32 val, val2, tmp, tmp2;

    mpp_assert((size_bytes % 8) == 0);

    while (words > 0) {
        val = buf[i];
        tmp = 0;

        tmp |= (val & 0xFF) << 24;
        tmp |= (val & 0xFF00) << 8;
        tmp |= (val & 0xFF0000) >> 8;
        tmp |= (val & 0xFF000000) >> 24;

        {
            val2 = buf[i + 1];
            tmp2 = 0;

            tmp2 |= (val2 & 0xFF) << 24;
            tmp2 |= (val2 & 0xFF00) << 8;
            tmp2 |= (val2 & 0xFF0000) >> 8;
            tmp2 |= (val2 & 0xFF000000) >> 24;

            buf[i] = tmp2;
            words--;
            i++;

        }
        buf[i] = tmp;
        words--;
        i++;
    }
}

static void h264e_vpu_write_cabac_table(MppBuffer hw_cabac_tab_buf, RK_S32 cabac_init_idc)
{
    const RK_S32(*context)[460][2];
    RK_S32 i, j, qp;

    RK_U8 table[H264E_CABAC_TABLE_BUF_SIZE] = {0};

    h264e_hal_enter();

    for (qp = 0; qp < 52; qp++) { /* All QP values */
        for (j = 0; j < 2; j++) { /* Intra/Inter */
            if (j == 0)
                /*lint -e(545) */
                context = &h264_context_init_intra;
            else
                /*lint -e(545) */
                context = &h264_context_init[cabac_init_idc];

            for (i = 0; i < 460; i++) {
                RK_S32 m = (RK_S32)(*context)[i][0];
                RK_S32 n = (RK_S32)(*context)[i][1];

                RK_S32 pre_ctx_state = H264E_HAL_CLIP3(((m * (RK_S32)qp) >> 4) + n, 1, 126);

                if (pre_ctx_state <= 63)
                    table[qp * 464 * 2 + j * 464 + i] = (RK_U8)((63 - pre_ctx_state) << 1);
                else
                    table[qp * 464 * 2 + j * 464 + i] = (RK_U8)(((pre_ctx_state - 64) << 1) | 1);
            }
        }
    }
    h264e_vpu_swap_endian((RK_U32 *)table, H264E_CABAC_TABLE_BUF_SIZE);
    mpp_buffer_write(hw_cabac_tab_buf, 0, table, H264E_CABAC_TABLE_BUF_SIZE);

    h264e_hal_leave();
}

static MPP_RET h264e_vpu_allocate_buffers(H264eHalContext *ctx, H264eHwCfg *syn)
{
    MPP_RET ret = MPP_OK;
    RK_S32 k = 0;
    h264e_hal_vpu_buffers *buffers = (h264e_hal_vpu_buffers *)ctx->buffers;
    RK_U32 frame_size = ((syn->width + 15) & (~15)) * ((syn->height + 15) & (~15)) * 3 / 2;

    h264e_hal_enter();
    ret = mpp_buffer_group_get_internal(&buffers->hw_buf_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("buf group get failed ret %d\n", ret);
        return ret;
    }

    ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_cabac_table_buf, H264E_CABAC_TABLE_BUF_SIZE);
    if (ret) {
        mpp_err("hw_cabac_table_buf get failed\n");
        return ret;
    }

    ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_nal_size_table_buf, (sizeof(RK_U32) * (syn->height + 1) + 7) & (~7));
    if (ret) {
        mpp_err("hw_nal_size_table_buf get failed\n");
        return ret;
    }

    for (k = 0; k < 2; k++) {
        ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_rec_buf[k], frame_size + 4096);
        if (ret) {
            mpp_err("hw_rec_buf[%d] get failed\n", k);
            return ret;
        }
    }

    h264e_vpu_write_cabac_table(buffers->hw_cabac_table_buf, syn->cabac_init_idc);

    h264e_hal_leave();
    return MPP_OK;
}


static MPP_RET h264e_vpu_stream_buffer_status(H264eVpuStream *stream)
{
    if (stream->byte_cnt + 5 > stream->size) {
        stream->overflow = 1;
        return MPP_NOK;
    }

    return MPP_OK;
}

static MPP_RET h264e_vpu_stream_buffer_reset(H264eVpuStream *strmbuf)
{
    strmbuf->stream = strmbuf->buffer;
    strmbuf->byte_cnt = 0;
    strmbuf->overflow = 0;
    strmbuf->byte_buffer = 0;
    strmbuf->buffered_bits = 0;
    strmbuf->zero_bytes = 0;
    strmbuf->emul_cnt = 0;

    return MPP_OK;
}

static MPP_RET h264e_vpu_stream_buffer_init(H264eVpuStream *strmbuf, RK_S32 size)
{
    strmbuf->buffer = mpp_calloc(RK_U8, size);

    if (strmbuf->buffer == NULL) {
        mpp_err("allocate stream buffer failed\n");
        return MPP_NOK;
    }
    strmbuf->stream = strmbuf->buffer;
    strmbuf->size = size;
    strmbuf->byte_cnt = 0;
    strmbuf->overflow = 0;
    strmbuf->byte_buffer = 0;
    strmbuf->buffered_bits = 0;
    strmbuf->zero_bytes = 0;
    strmbuf->emul_cnt = 0;

    if (MPP_OK != h264e_vpu_stream_buffer_status(strmbuf)) {
        mpp_err("stream buffer is overflow, while init");
        return MPP_NOK;
    }

    return MPP_OK;
}


void h264e_vpu_stream_put_bits(H264eVpuStream *buffer, RK_S32 value, RK_S32 number,
                               const char *name)
{
    RK_S32 bits;
    RK_U32 byte_buffer = buffer->byte_buffer;
    RK_U8*stream = buffer->stream;

    if (h264e_vpu_stream_buffer_status(buffer) != 0)
        return;

    h264e_hal_dbg(H264E_DBG_HEADER, "assemble %s value %x, bits %d\n", name, value, number);

    mpp_assert(value < (1 << number)); //opposite to 'BUG_ON' in kernel
    mpp_assert(number < 25);

    bits = number + buffer->buffered_bits;
    value <<= (32 - bits);
    byte_buffer = byte_buffer | value;

    while (bits > 7) {
        *stream = (RK_U8)(byte_buffer >> 24);

        bits -= 8;
        byte_buffer <<= 8;
        stream++;
        buffer->byte_cnt++;
    }

    buffer->byte_buffer = byte_buffer;
    buffer->buffered_bits = (RK_U8)bits;
    buffer->stream = stream;

    return;
}

void h264e_vpu_stream_put_bits_with_detect(H264eVpuStream * buffer, RK_S32 value, RK_S32 number, const char *name)
{
    RK_S32 bits;
    RK_U8 *stream = buffer->stream;
    RK_U32 byte_buffer = buffer->byte_buffer;

    h264e_hal_dbg(H264E_DBG_HEADER, "assemble %s value %x, bits %d\n", name, value, number);

    if (value) {
        mpp_assert(value < (1 << number));
        mpp_assert(number < 25);
    }

    bits = number + buffer->buffered_bits;
    byte_buffer = byte_buffer | ((RK_U32) value << (32 - bits));

    while (bits > 7) {
        RK_S32 zeroBytes = buffer->zero_bytes;
        RK_S32 byteCnt = buffer->byte_cnt;

        if (h264e_vpu_stream_buffer_status(buffer) != MPP_OK)
            return;

        *stream = (RK_U8) (byte_buffer >> 24);
        byteCnt++;

        if ((zeroBytes == 2) && (*stream < 4)) {
            *stream++ = 3;
            *stream = (RK_U8) (byte_buffer >> 24);
            byteCnt++;
            zeroBytes = 0;
            buffer->emul_cnt++;
        }

        if (*stream == 0)
            zeroBytes++;
        else
            zeroBytes = 0;

        bits -= 8;
        byte_buffer <<= 8;
        stream++;
        buffer->zero_bytes = zeroBytes;
        buffer->byte_cnt = byteCnt;
        buffer->stream = stream;
    }

    buffer->buffered_bits = (RK_U8) bits;
    buffer->byte_buffer = byte_buffer;
}


void h264e_vpu_rbsp_trailing_bits(H264eVpuStream * stream)
{
    h264e_vpu_stream_put_bits_with_detect(stream, 1, 1, "rbsp_stop_one_bit");
    if (stream->buffered_bits > 0) {
        h264e_vpu_stream_put_bits_with_detect(stream, 0, 8 - stream->buffered_bits, "bsp_alignment_zero_bit(s)");
    }
}

static void h264e_vpu_write_ue(H264eVpuStream *fifo, RK_U32 val,
                               const char *name)
{
    RK_U32 num_bits = 0;

    val++;
    while (val >> ++num_bits);

    if (num_bits > 12) {
        RK_U32 tmp;

        tmp = num_bits - 1;

        if (tmp > 24) {
            tmp -= 24;
            h264e_vpu_stream_put_bits_with_detect(fifo, 0, 24, name);
        }

        h264e_vpu_stream_put_bits_with_detect(fifo, 0, tmp, name);

        if (num_bits > 24) {
            num_bits -= 24;
            h264e_vpu_stream_put_bits_with_detect(fifo, val >> num_bits, 24, name);
            val = val >> num_bits;
        }

        h264e_vpu_stream_put_bits_with_detect(fifo, val, num_bits, name);
    } else {
        h264e_vpu_stream_put_bits_with_detect(fifo, val, 2 * num_bits - 1, name);
    }
}

static void h264e_vpu_write_se(H264eVpuStream *fifo, RK_S32 val, const char *name)
{
    RK_U32 tmp;

    if (val > 0)
        tmp = (RK_U32)(2 * val - 1);
    else
        tmp = (RK_U32)(-2 * val);

    h264e_vpu_write_ue(fifo, tmp, name);
}

static MPP_RET h264e_vpu_nal_start(H264eVpuStream * stream, RK_S32 nalRefIdc, H264eNalUnitType nalUnitType)
{
    h264e_vpu_stream_put_bits(stream, 0,                    8, "leadin_zero_8bits");
    h264e_vpu_stream_put_bits(stream, 0,                    8, "start_code_prefix");
    h264e_vpu_stream_put_bits(stream, 0,                    8, "start_code_prefix");
    h264e_vpu_stream_put_bits(stream, 1,                    8, "start_code_prefix");
    h264e_vpu_stream_put_bits(stream, 0,                    1, "forbidden_zero_bit");
    h264e_vpu_stream_put_bits(stream, nalRefIdc,            2, "nal_ref_idc");
    h264e_vpu_stream_put_bits(stream, (RK_S32)nalUnitType,  5, "nal_unit_type");
    stream->zero_bytes = 0; /* we start new counter for zero bytes */

    return MPP_OK;
}



static MPP_RET h264e_vpu_write_sps(H264eVpuStream *stream, H264eSps *sps)
{
    h264e_hal_enter();

    h264e_vpu_nal_start(stream, 1, H264E_NAL_SPS);

    h264e_vpu_stream_put_bits_with_detect(stream, sps->i_profile_idc, 8, "profile_idc"); //FIXED: 77, 42
    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_constraint_set0, 1, "constraint_set0_flag"); //E0
    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_constraint_set1, 1, "constraint_set1_flag");
    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_constraint_set2, 1, "constraint_set2_flag");
    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_constraint_set3, 1, "constraint_set3_flag");

    h264e_vpu_stream_put_bits_with_detect(stream, 0, 4, "reserved_zero_4bits");
    h264e_vpu_stream_put_bits_with_detect(stream, sps->i_level_idc, 8, "level_idc"); //28

    h264e_vpu_write_ue(stream, sps->i_id, "seq_parameter_set_id"); //8D

    if (sps->i_profile_idc >= 100) { //High profile
        h264e_vpu_write_ue(stream, sps->i_chroma_format_idc, "chroma_format_idc");
        h264e_vpu_write_ue(stream, H264_BIT_DEPTH - 8, "bit_depth_luma_minus8");
        h264e_vpu_write_ue(stream, H264_BIT_DEPTH - 8, "bit_depth_chroma_minus8");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->b_qpprime_y_zero_transform_bypass, 1, "qpprime_y_zero_transform_bypass_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, 0, 1, "seq_scaling_matrix_present_flag");
    }

    h264e_vpu_write_ue(stream, sps->i_log2_max_frame_num - 4, "log2_max_frame_num_minus4");

    h264e_vpu_write_ue(stream, sps->i_poc_type, "pic_order_cnt_type"); //68 16

    h264e_vpu_write_ue(stream, sps->i_num_ref_frames, "num_ref_frames");

    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_gaps_in_frame_num_value_allowed, 1, "gaps_in_frame_num_value_allowed_flag");

    h264e_vpu_write_ue(stream, sps->i_mb_width - 1, "pic_width_in_mbs_minus1");

    h264e_vpu_write_ue(stream, sps->i_mb_height - 1, "pic_height_in_map_units_minus1"); //09 64

    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_frame_mbs_only, 1, "frame_mbs_only_flag");

    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_direct8x8_inference, 1, "direct_8x8_inference_flag");

    h264e_vpu_stream_put_bits_with_detect(stream, sps->b_crop, 1, "frame_cropping_flag");
    if (sps->b_crop) {
        h264e_vpu_write_ue(stream, sps->crop.i_left / 2, "frame_crop_left_offset");
        h264e_vpu_write_ue(stream, sps->crop.i_right / 2, "frame_crop_right_offset");
        h264e_vpu_write_ue(stream, sps->crop.i_top / 2, "frame_crop_top_offset");
        h264e_vpu_write_ue(stream, sps->crop.i_bottom / 2, "frame_crop_bottom_offset");
    }

    h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_vui,     1, "vui_parameters_present_flag");
    if (sps->vui.b_vui) {
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_aspect_ratio_info_present,         1, "aspect_ratio_info_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_overscan_info_present,             1, "overscan_info_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_signal_type_present,               1, "video_signal_type_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_chroma_loc_info_present,           1, "chroma_loc_info_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_timing_info_present,               1, "timing_info_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.i_num_units_in_tick >> 16,           16, "num_units_in_tick msb");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.i_num_units_in_tick & 0xffff,        16, "num_units_in_tick lsb");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.i_time_scale >> 16,                  16, "time_scale msb");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.i_time_scale & 0xffff,               16, "time_scale lsb");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_fixed_frame_rate,                  1, "fixed_frame_rate_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_nal_hrd_parameters_present,        1, "nal_hrd_parameters_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_vcl_hrd_parameters_present,        1, "vcl_hrd_parameters_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_pic_struct_present,                1, "pic_struct_present_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_bitstream_restriction,             1, "bit_stream_restriction_flag");
        if (sps->vui.b_bitstream_restriction) {
            h264e_vpu_stream_put_bits_with_detect(stream, sps->vui.b_motion_vectors_over_pic_boundaries, 1, "motion_vectors_over_pic_boundaries");
            h264e_vpu_write_ue(stream, sps->vui.i_max_bytes_per_pic_denom,            "max_bytes_per_pic_denom");
            h264e_vpu_write_ue(stream, sps->vui.i_max_bits_per_mb_denom,              "max_bits_per_mb_denom");
            h264e_vpu_write_ue(stream, sps->vui.i_log2_max_mv_length_horizontal,      "log2_mv_length_horizontal");
            h264e_vpu_write_ue(stream, sps->vui.i_log2_max_mv_length_vertical,        "log2_mv_length_vertical");
            h264e_vpu_write_ue(stream, sps->vui.i_num_reorder_frames,                 "num_reorder_frames");
            h264e_vpu_write_ue(stream, sps->vui.i_max_dec_frame_buffering,            "max_dec_frame_buffering");
        }
    }

    h264e_vpu_rbsp_trailing_bits(stream);

    h264e_hal_dbg(H264E_DBG_HEADER, "sps write size: %d bytes", stream->byte_cnt);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_vpu_write_pps(H264eVpuStream *stream, H264ePps *pps)
{
    h264e_hal_enter();

    h264e_vpu_nal_start(stream, 1, H264E_NAL_PPS);

    h264e_vpu_write_ue(stream, pps->i_id, "pic_parameter_set_id");
    h264e_vpu_write_ue(stream, pps->i_sps_id, "seq_parameter_set_id");

    h264e_vpu_stream_put_bits_with_detect(stream, pps->b_cabac, 1, "entropy_coding_mode_flag");
    h264e_vpu_stream_put_bits_with_detect(stream, pps->b_pic_order, 1, "pic_order_present_flag");

    h264e_vpu_write_ue(stream, pps->i_num_slice_groups - 1, "num_slice_groups_minus1");
    h264e_vpu_write_ue(stream, pps->i_num_ref_idx_l0_default_active - 1, "num_ref_idx_l0_active_minus1");
    h264e_vpu_write_ue(stream, pps->i_num_ref_idx_l1_default_active - 1, "num_ref_idx_l1_active_minus1");

    h264e_vpu_stream_put_bits_with_detect(stream, pps->b_weighted_pred, 1, "weighted_pred_flag");
    h264e_vpu_stream_put_bits_with_detect(stream, pps->i_weighted_bipred_idc, 2, "weighted_bipred_idc");

    h264e_vpu_write_se(stream, pps->i_pic_init_qp - 26, "pic_init_qp_minus26");
    h264e_vpu_write_se(stream, pps->i_pic_init_qs - 26, "pic_init_qs_minus26");
    h264e_vpu_write_se(stream, pps->i_chroma_qp_index_offset, "chroma_qp_index_offset");

    h264e_vpu_stream_put_bits_with_detect(stream, pps->b_deblocking_filter_control, 1, "deblocking_filter_control_present_flag");
    h264e_vpu_stream_put_bits_with_detect(stream, pps->b_constrained_intra_pred, 1, "constrained_intra_pred_flag");

    h264e_vpu_stream_put_bits_with_detect(stream, pps->b_redundant_pic_cnt, 1, "redundant_pic_cnt_present_flag");

    if (pps->b_transform_8x8_mode) {
        h264e_vpu_stream_put_bits_with_detect(stream, pps->b_transform_8x8_mode, 1, "transform_8x8_mode_flag");
        h264e_vpu_stream_put_bits_with_detect(stream, pps->b_cqm_preset, 1, "pic_scaling_matrix_present_flag");
        h264e_vpu_write_se(stream, pps->i_chroma_qp_index_offset, "chroma_qp_index_offset");
    }

    h264e_vpu_rbsp_trailing_bits(stream);

    h264e_hal_dbg(H264E_DBG_HEADER, "pps write size: %d bytes", stream->byte_cnt);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_vpu_write_sei(H264eVpuStream *s, RK_U8 *payload, RK_S32 payload_size, RK_S32 payload_type)
{
    RK_S32 i = 0;

    h264e_hal_enter();

    h264e_vpu_nal_start(s, H264E_NAL_PRIORITY_DISPOSABLE, H264E_NAL_SEI);

    for (i = 0; i <= payload_type - 255; i += 255)
        h264e_vpu_stream_put_bits_with_detect(s, 0xff, 8, "sei_payload_type_ff_byte");
    h264e_vpu_stream_put_bits_with_detect(s, payload_type - i, 8, "sei_last_payload_type_byte");

    for (i = 0; i <= payload_size - 255; i += 255)
        h264e_vpu_stream_put_bits_with_detect(s, 0xff, 8, "sei_payload_size_ff_byte");
    h264e_vpu_stream_put_bits_with_detect( s, payload_size - i, 8, "sei_last_payload_size_byte");

    for (i = 0; i < payload_size; i++)
        h264e_vpu_stream_put_bits_with_detect(s, (RK_U32)payload[i], 8, "sei_payload_data");

    h264e_vpu_rbsp_trailing_bits(s);

    h264e_hal_leave();

    return MPP_OK;
}


MPP_RET h264e_vpu_sei_encode(H264eHalContext *ctx)
{
    H264eVpuExtraInfo *info = (H264eVpuExtraInfo *)ctx->extra_info;
    H264eVpuStream *sei_stream = &info->sei_stream;
    char *str = (char *)info->sei_buf;
    RK_S32 str_len = 0;

    h264e_sei_pack2str(str + H264E_UUID_LENGTH, ctx);

    str_len = strlen(str) + 1;
    if (str_len > H264E_SEI_BUF_SIZE) {
        h264e_hal_err("SEI actual string length %d exceed malloced size %d", str_len, H264E_SEI_BUF_SIZE);
        return MPP_NOK;
    } else {
        h264e_vpu_write_sei(sei_stream, (RK_U8 *)str, str_len, H264E_SEI_USER_DATA_UNREGISTERED);
    }

    return MPP_OK;
}

static MPP_RET h264e_vpu_init_extra_info(void *extra_info)
{
    static const RK_U8 h264e_sei_uuid[H264E_UUID_LENGTH] = {
        0x63, 0xfc, 0x6a, 0x3c, 0xd8, 0x5c, 0x44, 0x1e,
        0x87, 0xfb, 0x3f, 0xab, 0xec, 0xb3, 0xb6, 0x77
    };
    H264eVpuExtraInfo *info = (H264eVpuExtraInfo *)extra_info;
    H264eVpuStream *sps_stream = &info->sps_stream;
    H264eVpuStream *pps_stream = &info->pps_stream;
    H264eVpuStream *sei_stream = &info->sei_stream;

    if (MPP_OK != h264e_vpu_stream_buffer_init(sps_stream, 128)) {
        mpp_err("sps stream sw buf init failed");
        return MPP_NOK;
    }
    if (MPP_OK != h264e_vpu_stream_buffer_init(pps_stream, 128)) {
        mpp_err("pps stream sw buf init failed");
        return MPP_NOK;
    }
    if (MPP_OK != h264e_vpu_stream_buffer_init(sei_stream, H264E_SEI_BUF_SIZE)) {
        mpp_err("sei stream sw buf init failed");
        return MPP_NOK;
    }
    info->sei_buf = mpp_calloc_size(RK_U8, H264E_SEI_BUF_SIZE);
    memcpy(info->sei_buf, h264e_sei_uuid, H264E_UUID_LENGTH);

    return MPP_OK;
}

static MPP_RET h264e_vpu_deinit_extra_info(void *extra_info)
{
    H264eVpuExtraInfo *info = (H264eVpuExtraInfo *)extra_info;
    H264eVpuStream *sps_stream = &info->sps_stream;
    H264eVpuStream *pps_stream = &info->pps_stream;
    H264eVpuStream *sei_stream = &info->sei_stream;

    MPP_FREE(sps_stream->buffer);
    MPP_FREE(pps_stream->buffer);
    MPP_FREE(sei_stream->buffer);
    MPP_FREE(info->sei_buf);

    return MPP_OK;
}

static MPP_RET h264e_vpu_set_extra_info(H264eHalContext *ctx)
{
    H264eVpuExtraInfo *info = (H264eVpuExtraInfo *)ctx->extra_info;
    H264eVpuStream *sps_stream = &info->sps_stream;
    H264eVpuStream *pps_stream = &info->pps_stream;
    H264eSps *sps = &info->sps;
    H264ePps *pps = &info->pps;

    h264e_hal_enter();

    h264e_vpu_stream_buffer_reset(sps_stream);
    h264e_vpu_stream_buffer_reset(pps_stream);

    h264e_set_sps(ctx, sps);
    h264e_set_pps(ctx, pps, sps);

    h264e_vpu_write_sps(sps_stream, sps);
    h264e_vpu_write_pps(pps_stream, pps);

    if (ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_SEQ || ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_FRAME) {
        info->sei_change_flg |= H264E_SEI_CHG_SPSPPS;
        h264e_vpu_sei_encode(ctx);
    }


    h264e_hal_leave();

    return MPP_OK;
}

static RK_S32 exp_golomb_signed(RK_S32 val)
{
    RK_S32 tmp = 0;

    if (val > 0)
        val = 2 * val;
    else
        val = -2 * val + 1;

    while (val >> ++tmp)
        ;

    return tmp * 2 - 1;
}


MPP_RET hal_h264e_vpu_init(void *hal, MppHalCfg *cfg)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MPP_RET ret = MPP_OK;
    VPU_CLIENT_TYPE type = VPU_ENC;

    h264e_hal_enter();

    ctx->int_cb     = cfg->hal_int_cb;
    ctx->regs       = mpp_calloc(H264eVpuRegSet, 1);
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
    h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client: %d", type);

#ifdef RKPLATFORM
    ctx->vpu_fd = VPUClientInit(type);
    if (ctx->vpu_fd <= 0) {
        mpp_err("get vpu_socket(%d) <=0, failed. \n", ctx->vpu_fd);
        ret = MPP_ERR_UNKNOW;
    } else {
        VPUHwEncConfig_t hwCfg;
        h264e_hal_dbg(H264E_DBG_DETAIL, "get vpu_socket(%d), success. \n", ctx->vpu_fd);
        memset(&hwCfg, 0, sizeof(VPUHwEncConfig_t));
        if (VPUClientGetHwCfg(ctx->vpu_fd, (RK_U32*)&hwCfg, sizeof(hwCfg))) {
            mpp_err("h264enc # Get HwCfg failed, release vpu\n");
            VPUClientRelease(ctx->vpu_fd);
            ctx->vpu_fd = -1;
            ret = MPP_NOK;
        }
    }
#endif
    ctx->hw_cfg.qp_prev = ctx->cfg->codec.h264.qp_init;
    mpp_env_get_u32("hal_vpu_h264e_debug", &hal_vpu_h264e_debug, 0);

    h264e_hal_leave();
    return ret;
}

MPP_RET hal_h264e_vpu_deinit(void *hal)
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

    if (VPU_SUCCESS != VPUClientRelease(ctx->vpu_fd)) {
        mpp_err("VPUClientRelease failed");
        return MPP_ERR_VPUHW;
    }
#endif

    h264e_hal_leave();
    return MPP_OK;
}

static RK_S32 find_best_qp(MppLinReg *ctx, MppEncH264Cfg *codec, RK_S32 qp_start, RK_S32 bits)
{
    RK_S32 qp = qp_start;
    RK_S32 qp_best = qp_start;
    RK_S32 qp_min = codec->qp_min;
    RK_S32 qp_max = codec->qp_max;
    RK_S32 diff_best = INT_MAX;

    if (ctx->a == 0 && ctx->b == 0)
        return qp_best;

    hal_vpu_h264e_dbg_qp("RC: qp est target bit %d\n", bits);
    if (bits <= 0) {
        qp_best = mpp_clip(qp_best + codec->qp_max_step, qp_min, qp_max);
    } else {
        do {
            RK_S32 est_bits = mpp_quadreg_calc(ctx, h264_q_step[qp]);
            RK_S32 diff = est_bits - bits;
            hal_vpu_h264e_dbg_qp("RC: qp est qp %d bit %d diff %d best %d\n",
                                 qp, bits, diff, diff_best);
            if (MPP_ABS(diff) < MPP_ABS(diff_best)) {
                diff_best = MPP_ABS(diff);
                qp_best = qp;
                if (diff > 0)
                    qp++;
                else
                    qp--;
            } else
                break;
        } while (qp <= qp_max && qp >= qp_min);
    }

    return qp_best;
}

static MPP_RET h264e_vpu_update_hw_cfg(H264eHalContext *ctx, HalEncTask *task, H264eHwCfg *hw_cfg)
{
    RK_S32 i;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    RcSyntax *rc_syn = (RcSyntax *)task->syntax.data;

    /* preprocess setup */
    if (prep->change) {
        RK_U32 change = prep->change;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            hw_cfg->width   = prep->width;
            hw_cfg->height  = prep->height;
            hw_cfg->input_format = prep->format;

            mpp_assert(prep->hor_stride == MPP_ALIGN(prep->width, 16));
            mpp_assert(prep->ver_stride == MPP_ALIGN(prep->height, 16));

            hw_cfg->hor_stride = prep->hor_stride;
            hw_cfg->ver_stride = prep->ver_stride;

            h264e_vpu_set_format(hw_cfg, prep);
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            switch (prep->color) {
            case MPP_FRAME_SPC_RGB : {
                /* BT.601 */
                /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
                 * Cb = 0.5647 (B - Y) + 128
                 * Cr = 0.7132 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            case MPP_FRAME_SPC_BT709 : {
                /* BT.709 */
                /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
                 * Cb = 0.5389 (B - Y) + 128
                 * Cr = 0.6350 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 13933;
                hw_cfg->color_conversion_coeff_b = 46871;
                hw_cfg->color_conversion_coeff_c = 4732;
                hw_cfg->color_conversion_coeff_e = 35317;
                hw_cfg->color_conversion_coeff_f = 41615;
            } break;
            default : {
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            }
        }

        prep->change = 0;
    }

    if (codec->change) {
        // TODO: setup sps / pps here
        hw_cfg->idr_pic_id = !ctx->idr_pic_id;
        hw_cfg->filter_disable = codec->deblock_disable;
        hw_cfg->slice_alpha_offset = codec->deblock_offset_alpha;
        hw_cfg->slice_beta_offset = codec->deblock_offset_beta;
        hw_cfg->inter4x4_disabled = (codec->profile >= 31) ? (1) : (0);
        hw_cfg->cabac_init_idc = codec->cabac_init_idc;
        hw_cfg->qp = codec->qp_init;

        hw_cfg->qp_prev = hw_cfg->qp;

        codec->change = 0;
    }

    if (NULL == ctx->intra_qs)
        mpp_linreg_init(&ctx->intra_qs, MPP_MIN(rc->gop, 10), 2);
    if (NULL == ctx->inter_qs)
        mpp_linreg_init(&ctx->inter_qs, MPP_MIN(rc->gop, 10), 2);

    mpp_assert(ctx->intra_qs);
    mpp_assert(ctx->inter_qs);

    /* frame type and rate control setup */
    hal_vpu_h264e_dbg_qp("RC: qp calc ctx %p qp [%d %d] prev %d target bit %d\n",
                         ctx->inter_qs, codec->qp_min, codec->qp_max, hw_cfg->qp_prev,
                         rc_syn->bit_target);
    {
        RK_S32 prev_coding_type = hw_cfg->frame_type;

        if (rc_syn->type == INTRA_FRAME) {
            hw_cfg->frame_type = H264E_VPU_FRAME_I;
            hw_cfg->frame_num = 0;

            hw_cfg->qp = find_best_qp(ctx->intra_qs, codec, hw_cfg->qp_prev, rc_syn->bit_target);

            /*
             * Previous frame is inter then intra frame can not
             * have a big qp step between these two frames
             */
            if (prev_coding_type == 0)
                hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 4, hw_cfg->qp_prev + 4);
        } else {
            hw_cfg->frame_type = H264E_VPU_FRAME_P;

            hw_cfg->qp = find_best_qp(ctx->inter_qs, codec, hw_cfg->qp_prev, rc_syn->bit_target);

            if (prev_coding_type == 1)
                hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 4, hw_cfg->qp_prev + 4);
        }
    }

    hal_vpu_h264e_dbg_qp("RC: qp calc ctx %p qp get %d\n",
                         ctx->inter_qs, hw_cfg->qp);

    hw_cfg->qp = mpp_clip(hw_cfg->qp,
                          hw_cfg->qp_prev - codec->qp_max_step,
                          hw_cfg->qp_prev + codec->qp_max_step);

    hal_vpu_h264e_dbg_qp("RC: qp calc ctx %p qp clip %d prev %d step %d\n",
                         ctx->inter_qs, hw_cfg->qp, hw_cfg->qp_prev, codec->qp_max_step);

    hw_cfg->qp_prev = hw_cfg->qp;

    hw_cfg->mad_qp_delta = 0;
    hw_cfg->mad_threshold = 6;
    hw_cfg->keyframe_max_interval = rc->gop;
    hw_cfg->qp_min = codec->qp_min;
    hw_cfg->qp_max = codec->qp_max;

    /* disable mb rate control first */
    hw_cfg->cp_distance_mbs = 0;
    for (i = 0; i < 10; i++)
        hw_cfg->cp_target[i] = 0;

    for (i = 0; i < 7; i++)
        hw_cfg->target_error[i] = 0;

    for (i = 0; i < 7; i++)
        hw_cfg->delta_qp[i] = 0;

    /* slice mode setup */
    hw_cfg->slice_size_mb_rows = 0; //(prep->height + 15) >> 4;

    /* input and preprocess config */
    {
        RK_U32 offset_uv = hw_cfg->hor_stride * hw_cfg->ver_stride;
        hw_cfg->input_luma_addr = mpp_buffer_get_fd(task->input);
        hw_cfg->input_cb_addr = hw_cfg->input_luma_addr + (offset_uv << 10);
        hw_cfg->input_cr_addr = hw_cfg->input_cb_addr + (offset_uv << 8);
        hw_cfg->output_strm_limit_size = mpp_buffer_get_size(task->output);
        hw_cfg->output_strm_addr = mpp_buffer_get_fd(task->output);
    }

    /* context update */
    ctx->idr_pic_id = !ctx->idr_pic_id;
    return MPP_OK;
}

MPP_RET hal_h264e_vpu_gen_regs(void *hal, HalTaskInfo *task)
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

    memset(reg, 0, sizeof(H264eVpuRegSet));

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

    val = VEPU_REG_OUTPUT_SWAP32
          | VEPU_REG_OUTPUT_SWAP16
          | VEPU_REG_OUTPUT_SWAP8
          | VEPU_REG_INPUT_SWAP8
          | VEPU_REG_INPUT_SWAP16
          | VEPU_REG_INPUT_SWAP32;
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
        if (VPUClientGetIOMMUStatus() > 0) {
            recon_chroma_addr = recon_luma_addr | (frame_luma_size << 10);
            ref_chroma_addr   = ref_luma_addr   | (frame_luma_size << 10);
        } else {
            recon_chroma_addr = recon_luma_addr + frame_luma_size;
            ref_chroma_addr   = ref_luma_addr   + frame_luma_size ;
        }
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

MPP_RET hal_h264e_vpu_start(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    (void)task;
    h264e_hal_enter();
#ifdef RKPLATFORM
    if (ctx->vpu_fd > 0) {
        RK_U32 *p_regs = (RK_U32 *)ctx->regs;
        h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client is sending %d regs", VEPU_H264E_NUM_REGS);
        if (MPP_OK != VPUClientSendReg(ctx->vpu_fd, p_regs, VEPU_H264E_NUM_REGS)) {
            mpp_err("VPUClientSendReg Failed!!!");
            return MPP_ERR_VPUHW;
        } else {
            h264e_hal_dbg(H264E_DBG_DETAIL, "VPUClientSendReg successfully!");
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

static MPP_RET h264e_vpu_set_feedback(h264e_feedback *fb, H264eVpuRegSet *reg)
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

MPP_RET hal_h264e_vpu_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eVpuRegSet *reg_out = (H264eVpuRegSet *)ctx->regs;
    MppEncPrepCfg *prep = &ctx->set->prep;
    IOInterruptCB int_cb = ctx->int_cb;
    h264e_feedback *fb = &ctx->feedback;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16) * MPP_ALIGN(prep->height, 16) / 16 / 16;

    h264e_hal_enter();

#ifdef RKPLATFORM
    if (ctx->vpu_fd > 0) {
        VPU_CMD_TYPE cmd = 0;
        RK_S32 length = 0;
        RK_S32 hw_ret = VPUClientWaitResult(ctx->vpu_fd, (RK_U32 *)reg_out,
                                            VEPU_H264E_NUM_REGS, &cmd, &length);

        h264e_hal_dbg(H264E_DBG_DETAIL, "VPUClientWaitResult: ret %d, cmd %d, len %d\n", hw_ret, cmd, length);


        if ((VPU_SUCCESS != hw_ret) || (cmd != VPU_SEND_CONFIG_ACK_OK))
            mpp_err("hardware wait error");

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

MPP_RET hal_h264e_vpu_reset(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vpu_flush(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_vpu_control(void *hal, RK_S32 cmd_type, void *param)
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
                (set->hor_stride < 0 || set->hor_stride > 1920) ||
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
    } break;
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

