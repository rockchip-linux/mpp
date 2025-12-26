/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_jpegd_vpu730"

#include "mpp_common.h"
#include "mpp_debug.h"
#include "mpp_env.h"

#include "mpp_buffer_impl.h"
#include "mpp_dec_cb_param.h"

#include "jpegd_syntax.h"
#include "hal_jpegd_base.h"

#include "hal_jpegd_vpu7xx_com.h"
#include "hal_jpegd_vpu730.h"
#include "hal_jpegd_vpu730_reg.h"

static MPP_RET hal_jpegd_vpu730_init(void *hal, MppHalCfg *cfg)
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

    ctx->dec_cb       = cfg->dec_cb;
    ctx->packet_slots = cfg->packet_slots;
    ctx->frame_slots  = cfg->frame_slots;

    /* allocate regs buffer */
    if (ctx->regs == NULL) {
        ctx->regs = mpp_calloc_size(void, sizeof(JpegdVpu730Reg));

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

    ret = mpp_buffer_get(ctx->group, &ctx->pTableBase, RKD_TABLE_SIZE);

    if (ret)
        mpp_err_f("Get table buffer failed, ret %d\n", ret);

    mpp_buffer_attach_dev(ctx->pTableBase, ctx->dev);

    jpegd_dbg_func("exit\n");

    return ret;
}

static MPP_RET hal_jpegd_vpu730_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;

    jpegd_dbg_func("enter\n");

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
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

static MPP_RET setup_output_fmt(JpegdHalCtx *ctx, JpegdSyntax *syntax, RK_S32 out_idx)
{

    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdSyntax *s = syntax;
    JpegdVpu730Reg *regs = (JpegdVpu730Reg *)ctx->regs;
    RK_U32 stride = syntax->hor_stride;
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

    mpp_frame_set_hor_stride_pixel(frm, stride);

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
            if (ctx->output_fmt == MPP_FMT_RGB888 || ctx->output_fmt == MPP_FMT_BGR888) {
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_RGB888;
                regs->reg2_sys.bgr_sequence = (ctx->output_fmt == MPP_FMT_BGR888);
                mpp_frame_set_hor_stride(frm, stride * 3);
            } else if (ctx->output_fmt == MPP_FMT_BGR565 || ctx->output_fmt == MPP_FMT_RGB565) {
                regs->reg2_sys.yuv_out_format = YUV_OUT_FMT_2_RGB565;
                regs->reg2_sys.bgr_sequence = (ctx->output_fmt == MPP_FMT_BGR565);
                mpp_frame_set_hor_stride(frm, stride * 2);
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
        ctx->output_fmt = s->output_fmt;
    }

    if (MPP_FRAME_FMT_IS_TILE(ctx->output_fmt))
        regs->reg2_sys.dec_out_sequence = OUTPUT_TILE;
    else
        regs->reg2_sys.dec_out_sequence = OUTPUT_RASTER;

    jpegd_dbg_hal("convert format %d to format %d\n", s->output_fmt, ctx->output_fmt);

    if ((s->yuv_mode == YUV_MODE_420 && regs->reg2_sys.yuv_out_format == YUV_OUT_FMT_NO_TRANS) ||
        (regs->reg2_sys.yuv_out_format == YUV_OUT_FMT_2_NV12))
        regs->reg2_sys.fill_down_e = 1;
    else
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
    JpegdVpu730Reg *regs = (JpegdVpu730Reg *)ctx->regs;

    regs->reg66_int_en.dec_done_int_en = 1;
    regs->reg66_int_en.safe_reset_int_en = 1;
    regs->reg66_int_en.bus_error_int_en = 1;
    regs->reg66_int_en.timeout_int_en = 1;
    regs->reg66_int_en.buffer_empty_int_en = 1;

    regs->reg67_int_mask.dec_done_int_mask = 1;
    regs->reg67_int_mask.safe_reset_int_mask = 1;
    regs->reg67_int_mask.bus_error_int_mask = 1;
    regs->reg67_int_mask.timeout_int_mask = 1;
    regs->reg67_int_mask.buffer_empty_int_mask = 1;

    regs->reg3_pic_size.pic_width_m1 = s->width - 1;
    regs->reg3_pic_size.pic_height_m1 = s->height - 1;

    if (s->sample_precision != DCT_SAMPLE_PRECISION_8 || s->qtbl_entry > TBL_ENTRY_3)
        return MPP_NOK;

    regs->reg4_pic_fmt.pixel_depth = BIT_DEPTH_8;
    if (s->nb_components == 1) {
        regs->reg4_pic_fmt.htables_sel = TBL_ENTRY_1;
    }

    if (s->nb_components > 1) {
        regs->reg4_pic_fmt.qtables_sel = (s->qtbl_entry > 1) ? TBL_ENTRY_3 : TBL_ENTRY_2;
        regs->reg4_pic_fmt.htables_sel = (s->htbl_entry > 0x0f) ? TBL_ENTRY_3 : TBL_ENTRY_2;
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
        // The new JPEG decoder supports tile 4x4 output by default.
        if (mpp_get_soc_type() >= ROCKCHIP_SOC_RK3576) {
            switch (regs->reg2_sys.yuv_out_format) {
            case YUV_OUT_FMT_2_YUYV:
                y_hor_stride = y_hor_stride * 4 * 2;
                break;
            case YUV_OUT_FMT_NO_TRANS:
                switch (regs->reg4_pic_fmt.jpeg_mode) {
                case YUV_MODE_422:
                case YUV_MODE_440:
                    y_hor_stride = y_hor_stride * 4 * 2;
                    break;
                case YUV_MODE_444:
                    y_hor_stride = y_hor_stride * 4 * 3;
                    break;
                case YUV_MODE_411:
                case YUV_MODE_420:
                    y_hor_stride = y_hor_stride * 4 * 3 / 2;
                    break;
                case YUV_MODE_400:
                    y_hor_stride = y_hor_stride * 4;
                    break;
                default:
                    return MPP_NOK;
                    break;
                }
                break;
            case YUV_OUT_FMT_2_NV12:
                y_hor_stride = y_hor_stride * 4 * 3 / 2;
                break;
            }

            uv_hor_virstride = 0;
        } else {
            y_hor_stride <<= 3;
            uv_hor_virstride <<= 3;
        }
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

    if (table_fd <= 0) {
        mpp_err_f("get table_fd failed\n");
        return MPP_NOK;
    }

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

    mpp_dev_set_reg_offset(ctx->dev, 12, hw_strm_offset);
    mpp_dev_set_reg_offset(ctx->dev, 10, RKD_HUFFMAN_MINCODE_TBL_OFFSET);
    mpp_dev_set_reg_offset(ctx->dev, 11, RKD_HUFFMAN_VALUE_TBL_OFFSET);

    regs->reg14_strm_error.error_prc_mode = 1;
    regs->reg14_strm_error.strm_ffff_err_mode = 2;
    regs->reg14_strm_error.strm_other_mark_mode = 2;
    regs->reg14_strm_error.strm_dri_seq_err_mode = 0;

    regs->reg16_clk_gate.val = 0xff;

    regs->reg30_perf_latency_ctrl0.axi_perf_work_e = 1;
    regs->reg30_perf_latency_ctrl0.axi_perf_clr_e = 1;
    regs->reg30_perf_latency_ctrl0.axi_cnt_type = 1;
    regs->reg30_perf_latency_ctrl0.rd_latency_id = 0xa;

    jpegd_vpu7xx_write_htbl(ctx, s);

    jpegd_vpu7xx_write_qtbl(ctx, s);

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET hal_jpegd_vpu730_gen_regs(void *hal,  HalTaskInfo *syn)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;
    JpegdSyntax *s = (JpegdSyntax *)syn->dec.syntax.data;

    MppBuffer strm_buf = NULL;
    MppBuffer output_buf = NULL;

    if (syn->dec.flags.parse_err)
        goto __RETURN;

    mpp_buf_slot_get_prop(ctx->packet_slots, syn->dec.input, SLOT_BUFFER, & strm_buf);
    mpp_buf_slot_get_prop(ctx->frame_slots, syn->dec.output, SLOT_BUFFER, &output_buf);

    ctx->pkt_fd = mpp_buffer_get_fd(strm_buf);
    if (ctx->pkt_fd <= 0) {
        mpp_err_f("get pkt_fd failed\n");
        goto __RETURN;
    }

    ctx->frame_fd = mpp_buffer_get_fd(output_buf);
    if (ctx->frame_fd <= 0) {
        mpp_err_f("get frame_fd failed\n");
        goto __RETURN;
    }

    memset(ctx->regs, 0, sizeof(JpegdVpu730Reg));

    setup_output_fmt(ctx, s, syn->dec.output);

    ret = jpegd_gen_regs(ctx, s);
    mpp_buffer_sync_end(strm_buf);
    mpp_buffer_sync_end(ctx->pTableBase);

    if (ret != MPP_OK) {
        mpp_err_f("generate registers failed\n");
        goto __RETURN;
    }

    syn->dec.valid = 1;
    jpegd_dbg_func("exit\n");
    return ret;

__RETURN:
    syn->dec.flags.parse_err = 1;
    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET hal_jpegd_vpu730_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    JpegdHalCtx * ctx = (JpegdHalCtx *)hal;
    RK_U32 *regs = (RK_U32 *)ctx->regs;

    jpegd_dbg_func("enter\n");
    if (task->dec.flags.parse_err)
        goto __RETURN;

    MppDevRegWrCfg wr_cfg;
    MppDevRegRdCfg rd_cfg;
    RK_U32 reg_size = JPEGD_VPU730_REG_NUM * sizeof(RK_U32);
    RK_U8 i = 0;

    wr_cfg.reg = regs;
    wr_cfg.size = reg_size;
    wr_cfg.offset = 0;

    if (jpegd_debug & JPEGD_DBG_HAL_INFO) {
        for (i = 0; i < JPEGD_VPU730_REG_NUM; i++) {
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

    jpegd_dbg_func("exit\n");
    return ret;

__RETURN:
    task->dec.flags.parse_err = 1;
    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET hal_jpegd_vpu730_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    JpegdHalCtx *ctx = (JpegdHalCtx *)hal;
    JpegdVpu730Reg *reg_out = ctx->regs;
    RK_U32 errinfo = 0;
    RK_U8 i = 0;

    jpegd_dbg_func("enter\n");
    if (task->dec.flags.parse_err)
        goto __SKIP_HARD;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);

    if (ret) {
        task->dec.flags.parse_err = 1;
        mpp_err_f("poll cmd failed %d\n", ret);
    }

__SKIP_HARD:
    if (ctx->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)reg_out;
        if (!reg_out->reg69_int_sta.dec_done_int_sta || reg_out->reg69_int_sta.bus_error_int_sta
            || reg_out->reg69_int_sta.dec_error_int_sta || reg_out->reg69_int_sta.timeout_int_sta
            || reg_out->reg69_int_sta.buffer_empty_int_sta) {
            mpp_err("decode result: failed, irq 0x%08x\n", ((RK_U32 *)reg_out)[1]);
            errinfo = 1;
        }
        param.hard_err = errinfo;
        mpp_callback(ctx->dec_cb, &param);
    }
    if (jpegd_debug & JPEGD_DBG_HAL_INFO) {
        for (i = 0; i < JPEGD_VPU730_REG_NUM; i++) {
            mpp_log_f("read regs[%d]=0x%08x\n", i, ((RK_U32*)reg_out)[i]);
        }
    }

    jpegd_dbg_hal("decode one frame in cycles: %d\n", reg_out->reg39_perf_working_cnt);

    if (jpegd_debug & JPEGD_DBG_IO) {
        FILE *jpg_file;
        char name[32];
        MppBuffer outputBuf = NULL;
        void *base = NULL;
        mpp_buf_slot_get_prop(ctx->frame_slots, task->dec.output, SLOT_BUFFER, &outputBuf);
        base = mpp_buffer_get_ptr(outputBuf);

        snprintf(name, sizeof(name) - 1, "/data/tmp/output%02d.yuv", ctx->output_yuv_count);
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

    memset(&reg_out->reg69_int_sta, 0, sizeof(RK_U32));

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET hal_jpegd_vpu730_control(void *hal, MpiCmd cmd_type, void *param)
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
        MppFrameFormat output_fmt = *((MppFrameFormat *)param);
        RockchipSocType soc_type = mpp_get_soc_type();

        if (MPP_FRAME_FMT_IS_FBC(output_fmt)) {
            ret = MPP_ERR_VALUE;
        }

        if (MPP_FRAME_FMT_IS_TILE(output_fmt)) {
            if (soc_type < ROCKCHIP_SOC_RK3576 || MPP_FRAME_FMT_IS_RGB(output_fmt)) {
                ret = MPP_ERR_VALUE;
            }
        }

        if (MPP_FRAME_FMT_IS_RGB(output_fmt)) {
            if (soc_type == ROCKCHIP_SOC_RK3572) {
                ret = MPP_ERR_VALUE;
            } else if (soc_type == ROCKCHIP_SOC_RK3538) {
                if (output_fmt != MPP_FMT_BGR888 && output_fmt != MPP_FMT_RGB888 &&
                    output_fmt != MPP_FMT_RGB565 && output_fmt != MPP_FMT_BGR565) {
                    ret = MPP_ERR_VALUE;
                }
            }
        }

        if (ret) {
            mpp_err_f("invalid output format 0x%x\n", output_fmt);
        } else {
            JpegHalCtx->output_fmt = output_fmt;
            JpegHalCtx->set_output_fmt_flag = 1;
            jpegd_dbg_hal("output_format: 0x%x\n", JpegHalCtx->output_fmt);
        }
    } break;
    default :
        break;
    }

    jpegd_dbg_func("exit ret %d\n", ret);
    return ret;
}

const MppHalApi hal_jpegd_vpu730 = {
    .name     = "jpegd_vpu730",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingMJPEG,
    .ctx_size = sizeof(JpegdHalCtx),
    .flag     = 0,
    .init     = hal_jpegd_vpu730_init,
    .deinit   = hal_jpegd_vpu730_deinit,
    .reg_gen  = hal_jpegd_vpu730_gen_regs,
    .start    = hal_jpegd_vpu730_start,
    .wait     = hal_jpegd_vpu730_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = hal_jpegd_vpu730_control,
};