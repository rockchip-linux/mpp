/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "hal_jpege_v511"

#include <linux/string.h>

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_soc.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"

#include "hal_jpege_debug.h"
#include "jpege_syntax.h"
#include "hal_bufs.h"
#include "rkv_enc_def.h"
#include "vepu5xx_common.h"
#include "vepu511_common.h"
#include "hal_jpege_vepu511.h"
#include "hal_jpege_vepu511_reg.h"
#include "hal_jpege_hdr.h"

typedef struct JpegeV511HalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    void                *regs;
    void                *reg_out;

    void                *dump_files;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_cnt;
    void                *roi_data;
    MppEncCfgSet        *cfg;
    Vepu511OsdCfg       osd_cfg;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_S32              max_buf_cnt;
    RK_S32              hdr_status;
    void                *input_fmt;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    RK_U32              frame_num;
    RK_S32              fbc_header_len;
    RK_U32              title_num;

    JpegeBits           bits;
    JpegeSyntax         syntax;
    HalJpegeRc          hal_rc;
} JpegeV511HalContext;

MPP_RET hal_jpege_vepu511_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);
    hal_jpege_enter();

    ctx->reg_out  = mpp_calloc(JpegV511Status, 1);
    ctx->regs           = mpp_calloc(JpegV511RegSet, 1);
    ctx->input_fmt      = mpp_calloc(VepuFmtCfg, 1);
    ctx->cfg            = cfg->cfg;
    ctx->frame_cnt = 0;
    ctx->enc_mode = 1;
    cfg->type = VPU_CLIENT_RKVENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }

    ctx->dev = cfg->dev;
    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);
    hal_jpege_rc_init(&ctx->hal_rc);

    hal_jpege_leave();
    return ret;
}

MPP_RET hal_jpege_vepu511_deinit(void *hal)
{
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;

    hal_jpege_enter();
    jpege_bits_deinit(ctx->bits);

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->reg_out);
    MPP_FREE(ctx->input_fmt);

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }
    hal_jpege_leave();
    return MPP_OK;
}

static MPP_RET hal_jpege_vepu511_prepare(void *hal)
{
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;

    hal_jpege_dbg_func("enter %p\n", hal);
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    vepu5xx_set_fmt(fmt, ctx->cfg->prep.format);

    hal_jpege_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET vepu511_jpeg_set_patch_info(MppDev dev, JpegeSyntax *syn,
                                           VepuFmt input_fmt,
                                           HalEncTask *task)
{
    RK_U32 hor_stride = syn->hor_stride;
    RK_U32 ver_stride = syn->ver_stride ? syn->ver_stride : syn->height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    MPP_RET ret = MPP_OK;

    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
        u_offset = mpp_frame_get_fbc_offset(task->frame);
        v_offset = u_offset;
    } else {
        switch (input_fmt) {
        case VEPU5xx_FMT_YUV420P: {
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        } break;
        case VEPU5xx_FMT_YUV420SP:
        case VEPU5xx_FMT_YUV422SP: {
            u_offset = frame_size;
            v_offset = frame_size;
        } break;
        case VEPU5xx_FMT_YUV422P: {
            u_offset = frame_size;
            v_offset = frame_size * 3 / 2;
        } break;
        case VEPU5xx_FMT_YUV400 :
        case VEPU5xx_FMT_YUYV422:
        case VEPU5xx_FMT_UYVY422: {
            u_offset = 0;
            v_offset = 0;
        } break;
        case VEPU5xx_FMT_YUV444SP : {
            u_offset = frame_size;
            v_offset = frame_size;
        } break;
        case VEPU5xx_FMT_YUV444P : {
            u_offset = frame_size;
            v_offset = frame_size * 2;
        } break;
        case VEPU5xx_FMT_BGR565:
        case VEPU5xx_FMT_BGR888:
        case VEPU5xx_FMT_BGRA8888: {
            u_offset = 0;
            v_offset = 0;
        } break;
        default: {
            mpp_err("unknown color space: %d\n", input_fmt);
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        }
        }
    }

    /* input cb addr */
    if (u_offset)
        mpp_dev_set_reg_offset(dev, 265, u_offset);

    /* input cr addr */
    if (v_offset)
        mpp_dev_set_reg_offset(dev, 266, v_offset);

    return ret;
}

MPP_RET vepu511_set_jpeg_reg(Vepu511JpegCfg *cfg)
{
    HalEncTask *task = ( HalEncTask *)cfg->enc_task;
    JpegeSyntax *syn = (JpegeSyntax *)task->syntax.data;
    Vepu511JpegReg *regs = (Vepu511JpegReg *)cfg->jpeg_reg_base;
    VepuFmtCfg *fmt = (VepuFmtCfg *)cfg->input_fmt;
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    pic_width_align8 = (syn->width + 7) & (~7);
    pic_height_align8 = (syn->height + 7) & (~7);

    regs->adr_src0 =  mpp_buffer_get_fd(task->input);
    regs->adr_src1 = regs->adr_src0;
    regs->adr_src2 = regs->adr_src0;

    vepu511_jpeg_set_patch_info(cfg->dev, syn, (VepuFmt)fmt->format, task);

    regs->adr_bsbt = mpp_buffer_get_fd(task->output);
    regs->adr_bsbb = regs->adr_bsbt;
    regs->adr_bsbs = regs->adr_bsbt;
    regs->adr_bsbr = regs->adr_bsbt;

    mpp_dev_set_reg_offset(cfg->dev, 258, mpp_packet_get_length(task->packet));
    mpp_dev_set_reg_offset(cfg->dev, 256, mpp_buffer_get_size(task->output));

    regs->enc_rsl.pic_wd8_m1    = pic_width_align8 / 8 - 1;
    regs->src_fill.pic_wfill    = (syn->width & 0x7)
                                  ? (8 - (syn->width & 0x7)) : 0;
    regs->enc_rsl.pic_hd8_m1    = pic_height_align8 / 8 - 1;
    regs->src_fill.pic_hfill    = (syn->height & 0x7)
                                  ? (8 - (syn->height & 0x7)) : 0;

    regs->src_fmt.src_cfmt = fmt->format;
    regs->src_fmt.alpha_swap = fmt->alpha_swap;
    regs->src_fmt.rbuv_swap  = fmt->rbuv_swap;
    regs->src_fmt.src_range_trns_en  = 0;
    regs->src_fmt.src_range_trns_sel = 0;
    regs->src_fmt.chroma_ds_mode     = 0;
    regs->src_proc.src_mirr = syn->mirroring > 0;
    regs->src_proc.src_rot = syn->rotation;

    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
        regs->src_proc.rkfbcd_en = 1;

        stridey = mpp_frame_get_fbc_hdr_stride(task->frame);
        if (!stridey)
            stridey = MPP_ALIGN(syn->hor_stride, 16) >> 2;
    } else if (syn->hor_stride) {
        stridey = syn->hor_stride;
    } else {
        if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_BGRA8888)
            stridey = syn->width * 4;
        else if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_BGR888 ||
                 regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV444P ||
                 regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV444SP)
            stridey = syn->width * 3;
        else if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_BGR565 ||
                 regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUYV422 ||
                 regs->src_fmt.src_cfmt == VEPU5xx_FMT_UYVY422)
            stridey = syn->width * 2;
    }

    stridec = (regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV422SP ||
               regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV420SP ||
               regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV444P) ?
              stridey : stridey / 2;

    if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV444SP)
        stridec = stridey * 2;

    if (regs->src_fmt.src_cfmt < VEPU5xx_FMT_ARGB1555) {
        regs->src_udfy.csc_wgt_r2y = 66;
        regs->src_udfy.csc_wgt_g2y = 129;
        regs->src_udfy.csc_wgt_b2y = 25;

        regs->src_udfu.csc_wgt_r2u = -38;
        regs->src_udfu.csc_wgt_g2u = -74;
        regs->src_udfu.csc_wgt_b2u = 112;

        regs->src_udfv.csc_wgt_r2v = 112;
        regs->src_udfv.csc_wgt_g2v = -94;
        regs->src_udfv.csc_wgt_b2v = -18;

        regs->src_udfo.csc_ofst_y = 16;
        regs->src_udfo.csc_ofst_u = 128;
        regs->src_udfo.csc_ofst_v = 128;
    }

    regs->src_strd0.src_strd0  = stridey;
    regs->src_strd1.src_strd1  = stridec;
    regs->pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);

    regs->y_cfg.bias_y = 0;
    regs->u_cfg.bias_u = 0;
    regs->v_cfg.bias_v = 0;

    regs->base_cfg.ri  = syn->restart_ri;
    regs->base_cfg.out_mode = 0;
    regs->base_cfg.start_rst_m = 0;
    regs->base_cfg.pic_last_ecs = 1;
    regs->base_cfg.stnd = 1;

    regs->uvc_cfg.uvc_partition0_len = 0;
    regs->uvc_cfg.uvc_partition_len = 0;
    regs->uvc_cfg.uvc_skip_len = 0;
    return MPP_OK;
}

static void hal_jpege_vepu511_set_roi(JpegeV511HalContext *ctx)
{
    MppJpegROICfg *roi_cfg = (MppJpegROICfg *)ctx->roi_data;
    JpegV511RegSet *regs = ctx->regs;
    Vepu511JpegRoiRegion *reg_regions = &regs->reg_base.jpegReg.roi_regions[0];
    MppJpegROIRegion *region;
    RK_U32 frame_width = ctx->cfg->prep.width;
    RK_U32 frame_height = ctx->cfg->prep.height;
    RK_S32 i;

    if (roi_cfg == NULL)
        return;

    if (roi_cfg->non_roi_en) {
        if (roi_cfg->non_roi_level <= MPP_MAX_JPEG_ROI_LEVEL) {
            reg_regions->roi_cfg1.frm_rdoq_en = 1;
            reg_regions->roi_cfg1.frm_rdoq_level = roi_cfg->non_roi_level;
        } else {
            mpp_err_f("none roi level[%d] is invalid\n", roi_cfg->non_roi_level);
        }
    }

    for (i = 0; i < MPP_MAX_JPEG_ROI_NUM; i++) {
        region = &roi_cfg->regions[i];
        if (!region->roi_en)
            continue;

        if (region->w == 0 || region->h == 0 ||
            region->x + region->w > frame_width ||
            region->y + region->h > frame_height) {
            mpp_err_f("region[%d]: x[%d] y[%d] w[%d] h[%d] is invalid, frame width[%d] height[%d]\n",
                      i, region->x, region->y, region->w,
                      region->h, frame_width, frame_height);
            continue;
        }

        if (region->level > MPP_MAX_JPEG_ROI_LEVEL) {
            mpp_err_f("region[%d]: roi level[%d] is invalid\n", i, region->level);
            continue;
        }

        reg_regions->roi_cfg0.roi0_rdoq_en = 1;
        reg_regions->roi_cfg0.roi0_rdoq_start_x = MPP_ALIGN(region->x, 16) >> 3;
        reg_regions->roi_cfg0.roi0_rdoq_start_y = MPP_ALIGN(region->y, 16) >> 3;
        reg_regions->roi_cfg0.roi0_rdoq_level = region->level;
        reg_regions->roi_cfg1.roi0_rdoq_width_m1 = (MPP_ALIGN(region->w, 16) >> 3) - 1;
        reg_regions->roi_cfg1.roi0_rdoq_height_m1 = (MPP_ALIGN(region->h, 16) >> 3) - 1;
        reg_regions++;
    }
}

MPP_RET hal_jpege_vepu511_gen_regs(void *hal, HalEncTask *task)
{
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;
    JpegV511RegSet *regs = ctx->regs;
    Vepu511ControlCfg *reg_ctl = &regs->reg_ctl;
    JpegVepu511Base *reg_base = &regs->reg_base;
    JpegeBits bits = ctx->bits;
    size_t length = mpp_packet_get_length(task->packet);
    RK_U8  *buf = mpp_buffer_get_ptr(task->output);
    size_t size = mpp_buffer_get_size(task->output);
    JpegeSyntax *syntax = &ctx->syntax;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    Vepu511JpegCfg cfg;
    RK_S32 bitpos;

    hal_jpege_enter();
    cfg.enc_task = task;
    cfg.jpeg_reg_base = &reg_base->jpegReg;
    cfg.dev = ctx->dev;
    cfg.input_fmt = ctx->input_fmt;

    memset(regs, 0, sizeof(JpegV511RegSet));

    if (syntax->q_mode == JPEG_QFACTOR) {
        syntax->q_factor = 100 - task->rc_task->info.quality_target;
        hal_jpege_rc_update(&ctx->hal_rc, syntax);
    }

    /* write header to output buffer */
    jpege_bits_setup(bits, buf, (RK_U32)size);
    /* seek length bytes data */
    jpege_seek_bits(bits, length << 3);
    /* NOTE: write header will update qtable */
    write_jpeg_header(bits, syntax, &ctx->hal_rc);

    bitpos = jpege_bits_get_bitpos(bits);
    task->length = (bitpos + 7) >> 3;
    mpp_buffer_sync_partial_end(task->output, 0, task->length);
    mpp_packet_set_length(task->packet, task->length);
    reg_ctl->enc_strt.lkt_num      = 0;
    reg_ctl->enc_strt.vepu_cmd     = ctx->enc_mode;
    reg_ctl->enc_clr.safe_clr      = 0x0;
    reg_ctl->enc_clr.force_clr     = 0x0;

    reg_ctl->int_en.enc_done_en         = 1;
    reg_ctl->int_en.lkt_node_done_en    = 1;
    reg_ctl->int_en.sclr_done_en        = 1;
    reg_ctl->int_en.vslc_done_en         = 1;
    reg_ctl->int_en.vbsf_oflw_en         = 1;

    reg_ctl->int_en.jbuf_lens_en        = 1;
    reg_ctl->int_en.enc_err_en          = 1;
    reg_ctl->int_en.vsrc_err_en         = 1;
    reg_ctl->int_en.wdg_en              = 1;
    reg_ctl->int_en.lkt_err_int_en      = 0;
    reg_ctl->int_en.lkt_err_stop_en     = 0;
    reg_ctl->int_en.lkt_force_stop_en   = 0;
    reg_ctl->int_en.jslc_done_en        = 0;
    reg_ctl->int_en.jbsf_oflw_en        = 0;
    reg_ctl->int_en.dvbm_err_en         = 0;

    reg_ctl->dtrns_map.jpeg_bus_edin    = 0x7;
    reg_ctl->dtrns_map.src_bus_edin     = 0x0;
    reg_ctl->dtrns_map.meiw_bus_edin    = 0x0;
    reg_ctl->dtrns_map.bsw_bus_edin     = 0x0;
    reg_ctl->dtrns_map.lktr_bus_edin    = 0x0;
    reg_ctl->dtrns_map.roir_bus_edin    = 0x0;
    reg_ctl->dtrns_map.lktw_bus_edin    = 0x0;
    reg_ctl->dtrns_map.rec_nfbc_bus_edin   = 0x0;
    reg_ctl->dtrns_cfg.jsrc_bus_edin = fmt->src_endian;
    reg_base->common.enc_pic.enc_stnd = 2; // disable h264 or hevc

    reg_ctl->dtrns_cfg.axi_brsp_cke     = 0x0;
    reg_ctl->enc_wdg.vs_load_thd        = 0x1fffff;
    reg_base->common.enc_pic.jpeg_slen_fifo = 0;

    vepu511_set_jpeg_reg(&cfg);
    hal_jpege_vepu511_set_roi(ctx);

    if (ctx->osd_cfg.osd_data3 || ctx->osd_cfg.osd_data)
        vepu511_set_osd(&ctx->osd_cfg, &regs->reg_osd.osd_jpeg_cfg);

    {
        RK_U16 *tbl = &regs->jpeg_table.qua_tab0[0];
        RK_U32 i, j;

        for ( i = 0; i < 8; i++) {
            for ( j = 0; j < 8; j++) {
                tbl[i * 8 + j] = 0x8000 / ctx->hal_rc.qtables[0][j * 8 + i];
            }
        }
        tbl += 64;
        for ( i = 0; i < 8; i++) {
            for ( j = 0; j < 8; j++) {
                tbl[i * 8 + j] = 0x8000 / ctx->hal_rc.qtables[1][j * 8 + i];
            }
        }
        tbl += 64;
        for ( i = 0; i < 8; i++) {
            for ( j = 0; j < 8; j++) {
                tbl[i * 8 + j] = 0x8000 / ctx->hal_rc.qtables[1][j * 8 + i];
            }
        }
    }
    ctx->frame_num++;

    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_vepu511_start(void *hal, HalEncTask *enc_task)
{
    MPP_RET ret = MPP_OK;
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;
    JpegV511RegSet *hw_regs = ctx->regs;
    JpegV511Status *reg_out = ctx->reg_out;
    MppDevRegWrCfg cfg;
    MppDevRegRdCfg cfg1;
    hal_jpege_enter();

    if (enc_task->flags.err) {
        mpp_err_f("enc_task->flags.err %08x, return e arly",
                  enc_task->flags.err);
        return MPP_NOK;
    }

    cfg.reg = (RK_U32*)&hw_regs->reg_ctl;
    cfg.size = sizeof(Vepu511ControlCfg);
    cfg.offset = VEPU511_CTL_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg.reg = &hw_regs->jpeg_table;
    cfg.size = sizeof(JpegVepu511Tab);
    cfg.offset = VEPU511_JPEGTAB_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg.reg = &hw_regs->reg_base;
    cfg.size = sizeof(JpegVepu511Base);
    cfg.offset = VEPU511_FRAME_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg.reg = &hw_regs->reg_osd;
    cfg.size = sizeof(Vepu511OsdRegs);
    cfg.offset = VEPU511_OSD_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->hw_status;
    cfg1.size = sizeof(RK_U32);
    cfg1.offset = VEPU511_REG_BASE_HW_STATUS;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->st;
    cfg1.size = sizeof(JpegV511Status) - 4;
    cfg1.offset = VEPU511_STATUS_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
    }
    hal_jpege_leave();
    return ret;
}

static MPP_RET hal_jpege_vepu511_status_check(void *hal)
{
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;
    JpegV511Status *elem = (JpegV511Status *)ctx->reg_out;

    RK_U32 hw_status = elem->hw_status;

    if (hw_status & RKV_ENC_INT_LINKTABLE_FINISH)
        mpp_err_f("RKV_ENC_INT_LINKTABLE_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_SLICE_FINISH)
        mpp_err_f("RKV_ENC_INT_ONE_SLICE_FINISH");

    if (hw_status & RKV_ENC_INT_SAFE_CLEAR_FINISH)
        mpp_err_f("RKV_ENC_INT_SAFE_CLEAR_FINISH");

    if (hw_status & RKV_ENC_INT_BIT_STREAM_OVERFLOW)
        mpp_err_f("RKV_ENC_INT_BIT_STREAM_OVERFLOW");

    if (hw_status & RKV_ENC_INT_BUS_WRITE_FULL)
        mpp_err_f("RKV_ENC_INT_BUS_WRITE_FULL");

    if (hw_status & RKV_ENC_INT_BUS_WRITE_ERROR)
        mpp_err_f("RKV_ENC_INT_BUS_WRITE_ERROR");

    if (hw_status & RKV_ENC_INT_BUS_READ_ERROR)
        mpp_err_f("RKV_ENC_INT_BUS_READ_ERROR");

    if (hw_status & RKV_ENC_INT_TIMEOUT_ERROR)
        mpp_err_f("RKV_ENC_INT_TIMEOUT_ERROR");

    return MPP_OK;
}

MPP_RET hal_jpege_vepu511_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;
    HalEncTask *enc_task = task;
    JpegV511Status *elem = (JpegV511Status *)ctx->reg_out;
    hal_jpege_enter();

    if (enc_task->flags.err) {
        mpp_err_f("enc_task->flags.err %08x, return early",
                  enc_task->flags.err);
        return MPP_NOK;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret) {
        mpp_err_f("poll cmd failed %d\n", ret);
        ret = MPP_ERR_VPUHW;
    } else {
        hal_jpege_vepu511_status_check(hal);
        task->hw_length += elem->st.jpeg_head_bits_l32;
    }

    hal_jpege_leave();
    return ret;
}

MPP_RET hal_jpege_vepu511_get_task(void *hal, HalEncTask *task)
{
    JpegeV511HalContext *ctx = (JpegeV511HalContext *)hal;
    MppFrame frame = task->frame;
    EncFrmStatus  *frm_status = &task->rc_task->frm;
    JpegeSyntax *syntax = (JpegeSyntax *)task->syntax.data;

    hal_jpege_enter();

    memcpy(&ctx->syntax, syntax, sizeof(ctx->syntax));
    ctx->last_frame_type = ctx->frame_type;

    if (!frm_status->reencode && mpp_frame_has_meta(task->frame)) {
        MppMeta meta = mpp_frame_get_meta(frame);

        mpp_meta_get_ptr(meta, KEY_JPEG_ROI_DATA, (void **)&ctx->roi_data);
        mpp_meta_get_ptr(meta, KEY_OSD_DATA3, (void **)&ctx->osd_cfg.osd_data3);

    }

    if (ctx->cfg->jpeg.update) {
        hal_jpege_rc_update(&ctx->hal_rc, syntax);
        ctx->cfg->jpeg.update = 0;
    }

    task->rc_task->frm.is_intra = 1;

    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_vepu511_ret_task(void *hal, HalEncTask *task)
{
    (void)hal;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    hal_jpege_enter();

    task->length += task->hw_length;

    // setup bit length for rate control
    rc_info->bit_real = task->hw_length * 8;
    rc_info->quality_real = rc_info->quality_target;

    hal_jpege_leave();
    return MPP_OK;
}

const MppEncHalApi hal_jpege_vepu511 = {
    .name       = "hal_jpege_v511",
    .coding     = MPP_VIDEO_CodingMJPEG,
    .ctx_size   = sizeof(JpegeV511HalContext),
    .flag       = 0,
    .init       = hal_jpege_vepu511_init,
    .deinit     = hal_jpege_vepu511_deinit,
    .prepare    = hal_jpege_vepu511_prepare,
    .get_task   = hal_jpege_vepu511_get_task,
    .gen_regs   = hal_jpege_vepu511_gen_regs,
    .start      = hal_jpege_vepu511_start,
    .wait       = hal_jpege_vepu511_wait,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_jpege_vepu511_ret_task,
};
