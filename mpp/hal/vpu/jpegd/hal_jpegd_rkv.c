/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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
#define MODULE_TAG "hal_jpegd_rkv"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "mpp_common.h"

#include "jpegd_syntax.h"
#include "hal_jpegd_common.h"
#include "hal_jpegd_rkv.h"
#include "hal_jpegd_rkv_reg.h"

// Support 8-bit precision only
#define NB_COMPONENTS (3)
#define RKD_QUANTIZATION_TBL_SIZE (8*8*2*NB_COMPONENTS)
#define RKD_HUFFMAN_MINCODE_TBL_SIZE (16*3*2*NB_COMPONENTS)
#define RKD_HUFFMAN_VALUE_TBL_SIZE (16*12*NB_COMPONENTS)
#define RKD_HUFFMAN_MINCODE_TBL_OFFSET (RKD_QUANTIZATION_TBL_SIZE)
#define RKD_HUFFMAN_VALUE_TBL_OFFSET (RKD_HUFFMAN_MINCODE_TBL_OFFSET + MPP_ALIGN(RKD_HUFFMAN_MINCODE_TBL_SIZE, 64))
#define RKD_TABLE_SIZE (RKD_HUFFMAN_VALUE_TBL_OFFSET + RKD_HUFFMAN_VALUE_TBL_SIZE)

MPP_RET jpegd_write_rkv_qtbl(JpegdHalCtx *ctx, JpegdSyntax *syntax)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdSyntax *s = syntax;
    RK_U16 *base = (RK_U16 *)mpp_buffer_get_ptr(ctx->pTableBase);
    RK_U16 table_tmp[QUANTIZE_TABLE_LENGTH] = {0};
    RK_U32 i, j , idx;

    for (j = 0; j < s->nb_components; j++) {
        idx = s->quant_index[j];

        //turn zig-zag order to raster-scan order
        for (i = 0; i < QUANTIZE_TABLE_LENGTH; i++) {
            table_tmp[zzOrder[i]] = s->quant_matrixes[idx][i];
        }

        memcpy(base + j * QUANTIZE_TABLE_LENGTH, table_tmp, sizeof(RK_U16) * QUANTIZE_TABLE_LENGTH);
    }

    if (jpegd_debug & JPEGD_DBG_HAL_TBL) {
        RK_U8 *data = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase);

        mpp_log("--------------Quant tbl----------------------\n");
        for (i = 0; i < RKD_QUANTIZATION_TBL_SIZE; i += 8) {
            mpp_log("%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    data[i + 7], data[i + 6], data[i + 5], data[i + 4],
                    data[i + 3], data[i + 2], data[i + 1], data[i + 0]);
        }
    }

    jpegd_dbg_func("exit\n");
    return ret;

}

MPP_RET jpegd_write_rkv_htbl(JpegdHalCtx *ctx, JpegdSyntax *jpegd_syntax)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;

    JpegdSyntax *s = jpegd_syntax;
    AcTable *ac_ptr0 = NULL, *ac_ptr1 = NULL;
    DcTable *dc_ptr0 = NULL, *dc_ptr1 = NULL;
    void * htbl_ptr[6] = {NULL};
    RK_U32 i, j, k = 0;
    RK_U8 *p_htbl_value = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_VALUE_TBL_OFFSET;
    RK_U16 *p_htbl_mincode = (RK_U16 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_MINCODE_TBL_OFFSET  / 2;
    RK_U16 min_code_ac[16] = {0};
    RK_U16 min_code_dc[16] = {0};
    RK_U16 acc_addr_ac[16] = {0};
    RK_U16 acc_addr_dc[16] = {0};
    RK_U8 htbl_value[192] = {0};
    RK_U16 code = 0;
    RK_S32 addr = 0;
    RK_U32 len = 0;
    AcTable *ac_ptr;
    DcTable *dc_ptr;

    if (s->ac_index[0] == HUFFMAN_TABLE_ID_ZERO) {
        /* Luma's AC uses Huffman table zero */
        ac_ptr0 = &(s->ac_table[HUFFMAN_TABLE_ID_ZERO]);
        ac_ptr1 = &(s->ac_table[HUFFMAN_TABLE_ID_ONE]);
    } else {
        ac_ptr0 = &(s->ac_table[HUFFMAN_TABLE_ID_ONE]);
        ac_ptr1 = &(s->ac_table[HUFFMAN_TABLE_ID_ZERO]);
    }

    if (s->dc_index[0] == HUFFMAN_TABLE_ID_ZERO) {
        /* Luma's DC uses Huffman table zero */
        dc_ptr0 = &(s->dc_table[HUFFMAN_TABLE_ID_ZERO]);
        dc_ptr1 = &(s->dc_table[HUFFMAN_TABLE_ID_ONE]);
    } else {
        dc_ptr0 = &(s->dc_table[HUFFMAN_TABLE_ID_ONE]);
        dc_ptr1 = &(s->dc_table[HUFFMAN_TABLE_ID_ZERO]);
    }

    htbl_ptr[0] = dc_ptr0;
    htbl_ptr[1] = ac_ptr0;
    htbl_ptr[2] = dc_ptr1;
    htbl_ptr[3] = ac_ptr1;
    htbl_ptr[4] = dc_ptr1;
    htbl_ptr[5] = ac_ptr1;

    for (k = 0; k < s->nb_components; k++) {
        dc_ptr = (DcTable *)htbl_ptr[k * 2];
        ac_ptr = (AcTable *)htbl_ptr[k * 2 + 1];

        len = dc_ptr->bits[0];
        code = addr = 0;
        min_code_dc[0] = 0;
        acc_addr_dc[0] = 0;

        for (j = 0; j < 16; j++) {
            len = dc_ptr->bits[j];

            if (len == 0 && j > 0) {
                if (code > (min_code_dc[j - 1] << 1))
                    min_code_dc[j] = code;
                else
                    min_code_dc[j] = min_code_dc[j - 1] << 1;
            } else {
                min_code_dc[j] = code;
            }

            code += len;
            addr += len;
            acc_addr_dc[j] = addr;
            code <<= 1;

        }

        if (dc_ptr->bits[15])
            min_code_dc[0] = min_code_dc[15] + dc_ptr->bits[15] - 1;
        else
            min_code_dc[0] = min_code_dc[15];

        len = ac_ptr->bits[0];
        code = addr = 0;
        min_code_ac[0] = 0;
        acc_addr_ac[0] = 0;
        for (j = 0; j < 16; j++) {
            len = ac_ptr->bits[j];

            if (len == 0 && j > 0) {
                if (code > (min_code_ac[j - 1] << 1))
                    min_code_ac[j] = code;
                else
                    min_code_ac[j] = min_code_ac[j - 1] << 1;
            } else {
                min_code_ac[j] = code;
            }

            code += len;
            addr += len;
            acc_addr_ac[j] = addr;
            code <<= 1;
        }

        if (ac_ptr->bits[15])
            min_code_ac[0] = min_code_ac[15] + ac_ptr->bits[15] - 1;
        else
            min_code_ac[0] = min_code_ac[15];

        for (i = 0; i < 16; i++) {
            *p_htbl_mincode++ = min_code_dc[i];
        }

        for (i = 0; i < 8; i++) {
            *p_htbl_mincode++ = acc_addr_dc[2 * i] | acc_addr_dc[2 * i + 1] << 8;
        }

        for (i = 0; i < 16; i++) {
            *p_htbl_mincode++ = min_code_ac[i];
        }

        for (i = 0; i < 8; i++) {
            *p_htbl_mincode++ = acc_addr_ac[2 * i] | acc_addr_ac[2 * i + 1] << 8;
        }

        for (i = 0; i < MAX_DC_HUFFMAN_TABLE_LENGTH; i++) {
            htbl_value[i] = dc_ptr->vals[i];
        }

        for (i = 0; i < MAX_AC_HUFFMAN_TABLE_LENGTH; i++) {
            htbl_value[i + 16] = ac_ptr->vals[i];
        }

        for (i = 0; i < 12 * 16; i++) {
            *p_htbl_value++ = htbl_value[i];
        }
    }

    if (jpegd_debug & JPEGD_DBG_HAL_TBL) {
        RK_U8 *data = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_VALUE_TBL_OFFSET;

        mpp_log("--------------huffman value tbl----------------------\n");
        for (i = 0; i < RKD_HUFFMAN_VALUE_TBL_SIZE; i += 8) {
            mpp_log("%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    data[i + 7], data[i + 6], data[i + 5], data[i + 4],
                    data[i + 3], data[i + 2], data[i + 1], data[i + 0]);
        }

        data = NULL;
        data = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_MINCODE_TBL_OFFSET;

        mpp_log("--------------huffman mincode tbl----------------------\n");
        for (i = 0; i < RKD_HUFFMAN_MINCODE_TBL_SIZE; i += 8) {
            mpp_log("%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    data[i + 7], data[i + 6], data[i + 5], data[i + 4],
                    data[i + 3], data[i + 2], data[i + 1], data[i + 0]);
        }
    }

    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET hal_jpegd_rkv_init(void *hal, MppHalCfg *cfg)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;
    if (NULL == ctx) {
        ctx = (JpegdHalCtx *)mpp_calloc(JpegdHalCtx, 1);
        if (NULL == ctx) {
            mpp_err_f("NULL pointer");
            return MPP_ERR_NULL_PTR;
        }
    }

    ctx->packet_slots = cfg->packet_slots;
    ctx->frame_slots = cfg->frame_slots;

    ret = mpp_dev_init(&ctx->dev, VPU_CLIENT_JPEG_DEC);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }

    /* allocate regs buffer */
    if (ctx->regs == NULL) {
        ctx->regs = mpp_calloc_size(void, sizeof(JpegRegSet));
        if (ctx->regs == NULL) {
            mpp_err("hal jpegd reg alloc failed\n");

            jpegd_dbg_func("exit\n");
            return MPP_ERR_NOMEM;
        }
    }

    if (ctx->group == NULL) {
        ret = mpp_buffer_group_get_internal(&ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err_f("mpp_buffer_group_get failed ret %d\n", ret);
            return ret;
        }
    }

    ret = mpp_buffer_get(ctx->group, &ctx->frame_buf,
                         JPEGD_STREAM_BUFF_SIZE);
    if (ret) {
        mpp_err_f("Get frame buffer failed ret %d\n", ret);
        return ret;
    }

    ret = mpp_buffer_get(ctx->group, &ctx->pTableBase, RKD_TABLE_SIZE);
    if (ret) {
        mpp_err_f("Get table buffer failed, ret %d\n", ret);
    }

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET setup_output_fmt(JpegdHalCtx *ctx, JpegdSyntax *syntax, RK_S32 out_idx)
{

    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdSyntax *s = syntax;
    JpegRegSet *regs = (JpegRegSet *)ctx->regs;
    MppFrame frm = NULL;

    mpp_buf_slot_get_prop(ctx->frame_slots, out_idx, SLOT_FRAME_PTR, &frm);

    if (ctx->scale) {
        if (ctx->scale == 2)
            regs->reg2_sys.scaledown_mode = SCALEDOWN_HALF;
        if (ctx->scale == 4)
            regs->reg2_sys.scaledown_mode = SCALEDOWN_QUARTER;
        if (ctx->scale == 8)
            regs->reg2_sys.scaledown_mode = SCALEDOWN_ONE_EIGHTS;
    } else {
        regs->reg2_sys.scaledown_mode = SCALEDOWN_DISABLE;
    }

    if (ctx->set_output_fmt_flag && (ctx->output_fmt != s->output_fmt)) {   // PP enable
        if (MPP_FRAME_FMT_IS_YUV(ctx->output_fmt) && s->output_fmt != MPP_FMT_YUV400) {
            if (ctx->output_fmt == MPP_FMT_YUV420SP)
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_NV12;
            else if (ctx->output_fmt == MPP_FMT_YUV422_YUYV)
                /* Only support yuv422 and yuv444.
                 * Other format transformation won't report hardware irq error,
                 * and won't get a correct YUV image.
                */
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_YUYV;
            else if (ctx->output_fmt == MPP_FMT_YUV422_YVYU) {
                regs->reg2_sys.out_cbcr_swap = 1;
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_YUYV;
            } else if (ctx->output_fmt == MPP_FMT_YUV420SP_VU) {
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_NV12;
                regs->reg2_sys.out_cbcr_swap = 1;
            }
        } else if (MPP_FRAME_FMT_IS_RGB(ctx->output_fmt)) {
            if (ctx->output_fmt == MPP_FMT_RGB888) {
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_RGB888;
            } else if (ctx->output_fmt == MPP_FMT_BGR565) { //bgr565le
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_RGB565;
            } else {
                mpp_err_f("unsupported output format %d\n", ctx->output_fmt);
                ret = MPP_NOK;
            }
            MppFrameColorRange color_range = MPP_FRAME_RANGE_UNSPECIFIED;
            color_range = mpp_frame_get_color_range(frm);
            if (color_range != MPP_FRAME_RANGE_MPEG)
                regs->reg2_sys.yuv2rgb_range = YUV_TO_RGB_FULL_RANGE;
            else
                regs->reg2_sys.yuv2rgb_range = YUV_TO_RGB_LIMIT_RANGE;
        }
    } else {    //keep original format
        regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_NO_TRANS;
    }

    jpegd_dbg_hal("convert format %d to format %d\n", s->output_fmt, ctx->output_fmt);

    regs->reg2_sys.fill_down_e = s->fill_bottom;
    regs->reg2_sys.fill_right_e = s->fill_right;

    mpp_frame_set_fmt(frm, ctx->output_fmt);

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET jpegd_gen_regs(JpegdHalCtx *ctx, JpegdSyntax *syntax)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdSyntax *s = syntax;
    JpegRegSet *regs = (JpegRegSet *)ctx->regs;

    regs->reg1_int.dec_e = 1;
    regs->reg1_int.dec_timeout_e = 1;
    regs->reg1_int.buf_empty_e = 1;

    regs->reg3_pic_size.pic_width_m1 = s->width - 1;
    regs->reg3_pic_size.pic_height_m1 = s->height - 1;

    if (s->sample_precision != DCT_SAMPLE_PRECISION_8 || s->htbl_entry > TBL_ENTRY_3
        || s->qtbl_entry > TBL_ENTRY_3)
        return MPP_NOK;

    regs->reg4_pic_fmt.pixel_depth = BIT_DEPTH_8;
    if (s->nb_components == 1) {
        regs->reg4_pic_fmt.htables_sel = TBL_ENTRY_1;
    }

    if (s->nb_components > 1) {
        regs->reg4_pic_fmt.qtables_sel = (s->qtbl_entry > 1) ? s->qtbl_entry : TBL_ENTRY_2;
        regs->reg4_pic_fmt.htables_sel = (s->htbl_entry > 1) ? s->htbl_entry : TBL_ENTRY_2;
    } else {
        regs->reg4_pic_fmt.qtables_sel = TBL_ENTRY_1;
        regs->reg4_pic_fmt.htables_sel = TBL_ENTRY_1;
    }

    if (s->restart_interval) {
        regs->reg4_pic_fmt.dri_e = RST_ENABLE;
        regs->reg4_pic_fmt.dri_mcu_num_m1 = s->restart_interval - 1;
    }

    switch (s->yuv_mode) {
    case JPEGDEC_YUV400:
    case JPEGDEC_YUV420:
    case JPEGDEC_YUV422:
        regs->reg4_pic_fmt.jpeg_mode = s->yuv_mode;
        break;
    case JPEGDEC_YUV411:
        regs->reg4_pic_fmt.jpeg_mode = YUV_MODE_411;
        break;
    case JPEGDEC_YUV440:
        regs->reg4_pic_fmt.jpeg_mode = YUV_MODE_440;
        break;
    case JPEGDEC_YUV444:
        regs->reg4_pic_fmt.jpeg_mode = YUV_MODE_444;
        break;
    default:
        mpp_err_f("unsupported yuv mode %d\n", s->yuv_mode);
        break;
    }

    RK_U32 out_width = MPP_ALIGN(s->width, 16);
    RK_U32 out_height = s->height;
    out_width = out_width >> regs->reg2_sys.scaledown_mode;
    out_width = MPP_ALIGN(out_width, 16);
    out_height = regs->reg2_sys.fill_down_e ? MPP_ALIGN(out_height, 16) : MPP_ALIGN(out_height, 8);
    out_height = out_height >> regs->reg2_sys.scaledown_mode;
    jpegd_dbg_hal("output scale %d, width %d, height %d\n", regs->reg2_sys.scaledown_mode, out_width, out_height);

    RK_U32 y_hor_stride = out_width >> 4;
    RK_U32 y_virstride = 0;
    RK_U32 uv_hor_virstride = 0;

    switch (regs->reg2_sys.yuv_out_format) {
    case YUV_OUT_FMT_2_RGB888:
        y_hor_stride *= 3;
        break;
    case YUV_OUT_FMT_2_RGB565:
    case YUV_OUT_FMT_2_YUYV:
        y_hor_stride *= 2;
        break;
    case YUV_OUT_FMT_NO_TRANS:
        switch (regs->reg4_pic_fmt.jpeg_mode) {
        case YUV_MODE_440:
        case YUV_MODE_444:
            uv_hor_virstride = y_hor_stride * 2;
            break;
        case YUV_MODE_411:
            uv_hor_virstride = y_hor_stride >> 1;
            break;
        case YUV_MODE_400:
            uv_hor_virstride = 0;
            break;
        default:
            uv_hor_virstride = y_hor_stride;
            break;
        }
        break;
    case YUV_OUT_FMT_2_NV12:
        uv_hor_virstride = y_hor_stride;
        break;
    }

    y_virstride = y_hor_stride * out_height;
    if (regs->reg2_sys.dec_out_sequence == OUTPUT_TILE) {
        y_hor_stride <<= 3;
        uv_hor_virstride <<= 3;
    }

    regs->reg5_hor_virstride.y_hor_virstride = y_hor_stride & 0xffff;
    regs->reg5_hor_virstride.uv_hor_virstride = uv_hor_virstride & 0xffff;
    regs->reg6_y_virstride.y_virstride = y_virstride;

    regs->reg7_tbl_len.y_hor_virstride_h = (y_hor_stride >> 16) & 1;

    if (s->qtable_cnt)
        regs->reg7_tbl_len.qtbl_len = regs->reg4_pic_fmt.qtables_sel * 8 - 1;
    else
        regs->reg7_tbl_len.qtbl_len = 0;

    // 8 bit precision 12, 12 bit precision 16;
    regs->reg7_tbl_len.htbl_value_len = regs->reg4_pic_fmt.htables_sel * (regs->reg4_pic_fmt.pixel_depth ? 16 : 12) - 1;

    switch (regs->reg4_pic_fmt.htables_sel) {
    case TBL_ENTRY_0 :
        regs->reg7_tbl_len.htbl_mincode_len = 0;
        regs->reg7_tbl_len.htbl_value_len = 0;
        break;
    case TBL_ENTRY_2 :
        regs->reg7_tbl_len.htbl_mincode_len = (s->nb_components - 1) * 6 - 1;
        break;
    case TBL_ENTRY_1 :
    case TBL_ENTRY_3 :
        regs->reg7_tbl_len.htbl_mincode_len = s->nb_components * 6 - 1;
        break;
    default :
        mpp_err_f("unsupported htable_sel %d\n", regs->reg4_pic_fmt.htables_sel);
        break;
    }

    RK_U32 strm_offset = 0;
    RK_U32 hw_strm_offset = 0;
    RK_U8 start_byte = 0;
    RK_U32 table_fd = mpp_buffer_get_fd(ctx->pTableBase);

    strm_offset = s->strm_offset;
    hw_strm_offset = strm_offset - strm_offset % 16;
    start_byte = strm_offset % 16;

    regs->reg8_strm_len.stream_len = (MPP_ALIGN((s->pkt_len - hw_strm_offset), 16) - 1) >> 4;
    regs->reg8_strm_len.strm_start_byte = start_byte;

    regs->reg9_qtbl_base = table_fd;
    regs->reg10_htbl_mincode_base = table_fd;
    regs->reg11_htbl_value_base = table_fd;
    regs->reg13_dec_out_base = ctx->frame_fd;
    regs->reg12_strm_base = ctx->pkt_fd;

    MppDevRegOffsetCfg trans_cfg_10;
    MppDevRegOffsetCfg trans_cfg_11;
    MppDevRegOffsetCfg trans_cfg_12;

    trans_cfg_12.reg_idx = 12;
    trans_cfg_12.offset = hw_strm_offset;
    mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFSET, &trans_cfg_12);

    trans_cfg_10.reg_idx = 10;
    trans_cfg_10.offset = RKD_HUFFMAN_MINCODE_TBL_OFFSET;
    mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFSET, &trans_cfg_10);

    trans_cfg_11.reg_idx = 11;
    trans_cfg_11.offset = RKD_HUFFMAN_VALUE_TBL_OFFSET;
    mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFSET, &trans_cfg_11);

    regs->reg14_strm_error.error_prc_mode = 1;
    regs->reg14_strm_error.strm_ffff_err_mode = 2;
    regs->reg14_strm_error.strm_other_mark_mode = 2;
    regs->reg14_strm_error.strm_dri_seq_err_mode = 0;

    regs->reg16_clk_gate.val = 0xff;

    regs->reg30_perf_latency_ctrl0.axi_per_work_e = 1;
    regs->reg30_perf_latency_ctrl0.axi_per_clr_e = 1;
    regs->reg30_perf_latency_ctrl0.axi_cnt_type = 1;
    regs->reg30_perf_latency_ctrl0.rd_latency_id = 0xa;

    jpegd_write_rkv_htbl(ctx, s);

    jpegd_write_rkv_qtbl(ctx, s);

    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET hal_jpegd_rkv_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;

    jpegd_dbg_func("enter\n");

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    if (ctx->frame_buf) {
        ret = mpp_buffer_put(ctx->frame_buf);
        if (ret) {
            mpp_err_f("put buffer failed\n");
            return ret;
        }
    }

    if (ctx->pTableBase) {
        ret = mpp_buffer_put(ctx->pTableBase);
        if (ret) {
            mpp_err_f("put buffer failed\n");
            return ret;
        }
    }

    if (ctx->group) {
        ret = mpp_buffer_group_put(ctx->group);
        if (ret) {
            mpp_err_f("group free buffer failed\n");
            return ret;
        }
    }

    if (ctx->regs) {
        mpp_free(ctx->regs);
        ctx->regs = NULL;
    }

    ctx->output_fmt = MPP_FMT_YUV420SP;
    ctx->set_output_fmt_flag = 0;
    ctx->hal_debug_enable = 0;
    ctx->frame_count = 0;
    ctx->output_yuv_count = 0;

    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET hal_jpegd_rkv_gen_regs(void *hal,  HalTaskInfo *syn)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;
    JpegdSyntax *s = (JpegdSyntax *)syn->dec.syntax.data;

    MppBuffer strm_buf = NULL;
    MppBuffer output_buf = NULL;

    mpp_buf_slot_get_prop(ctx->packet_slots, syn->dec.input, SLOT_BUFFER, & strm_buf);
    mpp_buf_slot_get_prop(ctx->frame_slots, syn->dec.output, SLOT_BUFFER, &output_buf);

    ctx->pkt_fd = mpp_buffer_get_fd(strm_buf);
    ctx->frame_fd = mpp_buffer_get_fd(output_buf);

    memset(ctx->regs, 0, sizeof(JpegRegSet));

    setup_output_fmt(ctx, s, syn->dec.output);

    ret = jpegd_gen_regs(ctx, s);

    if (ret != MPP_OK) {
        mpp_err_f("generate registers failed\n");
        return ret;
    }

    syn->dec.valid = 1;
    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET hal_jpegd_rkv_start(void *hal, HalTaskInfo *task)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdHalCtx * ctx = (JpegdHalCtx *)hal;
    RK_U32 *regs = (RK_U32 *)ctx->regs;

    MppDevRegWrCfg wr_cfg;
    MppDevRegRdCfg rd_cfg;
    RK_U32 reg_size = JPEGD_REG_NUM * sizeof(RK_U32);
    RK_U8 i = 0;

    wr_cfg.reg = regs;
    wr_cfg.size = reg_size;
    wr_cfg.offset = 0;

    if (jpegd_debug & JPEGD_DBG_HAL_INFO) {
        for (i = 0; i < JPEGD_REG_NUM; i++) {
            mpp_log_f("send reg[%d]=0x%08x\n", i, regs[i]);
        }
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);

    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        goto __RETURN;
    }

    rd_cfg.reg = regs;
    rd_cfg.size = reg_size;
    rd_cfg.offset = 0;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);

    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        goto __RETURN;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);

    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
        goto __RETURN;
    }

__RETURN:
    (void)task;
    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET hal_jpegd_rkv_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;
    JpegRegSet *reg_out = ctx->regs;
    RK_U32 errinfo = 0;
    MppFrame tmp = NULL;
    RK_U8 i = 0;

    jpegd_dbg_func("enter\n");

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);

    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    if (jpegd_debug & JPEGD_DBG_HAL_INFO) {
        for (i = 0; i < JPEGD_REG_NUM; i++) {
            mpp_log_f("read regs[%d]=0x%08x\n", i, ((RK_U32*)reg_out)[i]);
        }
    }

    jpegd_dbg_hal("decode one frame in cycles: %d\n", reg_out->reg39_perf_working_cnt);

    if (!reg_out->reg1_int.dec_irq || !reg_out->reg1_int.dec_rdy_sta
        || reg_out->reg1_int.dec_bus_sta || reg_out->reg1_int.dec_error_sta
        || reg_out->reg1_int.dec_timeout_sta
        || reg_out->reg1_int.dec_buf_empty_sta) {
        mpp_err("decode result: failed, irq 0x%08x\n", ((RK_U32 *)reg_out)[1]);
        errinfo = 1;
    }

    mpp_buf_slot_get_prop(ctx->frame_slots, task->dec.output, SLOT_FRAME_PTR, &tmp);
    mpp_frame_set_errinfo(tmp, errinfo);

    if (jpegd_debug & JPEGD_DBG_IO) {
        FILE *jpg_file;
        char name[32];
        MppBuffer outputBuf = NULL;
        void *base = NULL;
        mpp_buf_slot_get_prop(ctx->frame_slots, task->dec.output, SLOT_BUFFER, &outputBuf);
        base = mpp_buffer_get_ptr(outputBuf);

        snprintf(name, sizeof(name), "/data/tmp/output%02d.yuv", ctx->output_yuv_count);
        jpg_file = fopen(name, "wb+");
        if (jpg_file) {
            JpegdSyntax *s = (JpegdSyntax *) task->dec.syntax.data;
            RK_U32 width = s->hor_stride;
            RK_U32 height = s->ver_stride;

            fwrite(base, width * height * 3, 1, jpg_file);
            jpegd_dbg_io("frame_%02d output YUV(%d*%d) saving to %s\n", ctx->output_yuv_count,
                         width, height, name);
            fclose(jpg_file);
            ctx->output_yuv_count++;
        }
    }

    memset(&reg_out->reg1_int, 0, sizeof(RK_U32));

    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET hal_jpegd_rkv_control(void *hal, MpiCmd cmd_type, void *param)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *JpegHalCtx = (JpegdHalCtx *)hal;

    if (NULL == JpegHalCtx) {
        mpp_err_f("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd_type) {
    case MPP_DEC_SET_OUTPUT_FORMAT: {
        JpegHalCtx->output_fmt = *((MppFrameFormat *)param);
        JpegHalCtx->set_output_fmt_flag = 1;
        jpegd_dbg_hal("output_format:%d\n", JpegHalCtx->output_fmt);
    } break;
    //TODO support scale and tile output
    default :
        ret = MPP_NOK;
        break;
    }

    jpegd_dbg_func("exit\n");
    return ret;
}
