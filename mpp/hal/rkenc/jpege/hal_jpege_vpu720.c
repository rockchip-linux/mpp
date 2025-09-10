/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_jpege_vpu720"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_buffer_impl.h"
#include "mpp_enc_hal.h"

#include "jpege_syntax.h"
#include "hal_jpege_hdr.h"
#include "hal_jpege_debug.h"
#include "hal_jpege_vpu720.h"
#include "hal_jpege_vpu720_reg.h"

typedef enum JpegeVpu720InFmt_e {
    JPEGE_VPU720_IN_FMT_TILE_400,
    JPEGE_VPU720_IN_FMT_TILE_420,
    JPEGE_VPU720_IN_FMT_TILE_422,
    JPEGE_VPU720_IN_FMT_TILE_444,
    JPEGE_VPU720_IN_FMT_YUV422SP,
    JPEGE_VPU720_IN_FMT_YUV422P,
    JPEGE_VPU720_IN_FMT_YUV420SP,
    JPEGE_VPU720_IN_FMT_YUV420P,
    JPEGE_VPU720_IN_FMT_YUYV,
    JPEGE_VPU720_IN_FMT_UYVY,
    JPEGE_VPU720_IN_FMT_YUV400,
    JPEGE_VPU720_IN_FMT_RESERVED,
    JPEGE_VPU720_IN_FMT_YUV444SP,
    JPEGE_VPU720_IN_FMT_YUV444P,
} JpegeVpu720InFmt;

typedef enum JpegeVpu720OutFmt_e {
    JPEGE_VPU720_OUT_FMT_400 = 0,
    JPEGE_VPU720_OUT_FMT_420 = 1,
    JPEGE_VPU720_OUT_FMT_422 = 2,
    JPEGE_VPU720_OUT_FMT_444 = 3,
} JpegeVpu720OutFmt;

typedef enum JpegeVpu720EncCmd_e {
    JPEG_VPU720_ENC_MODE_NONE,
    JPEG_VPU720_ENC_MODE_ONE_FRAME,
    JPEG_VPU720_ENC_MODE_MULTI_FRAME_START,
    JPEG_VPU720_ENC_MODE_MULTI_FRAME_UPDATE,
    JPEG_VPU720_ENC_MODE_LKT_FORCE_PAUSE,
    JPEG_VPU720_ENC_MODE_LKT_CONTINUE,
    JPEG_VPU720_ENC_MODE_SAFE_CLR,
    JPEG_VPU720_ENC_MODE_FORCE_CLR,
    JPEG_VPU720_ENC_MODE_FORCE_BUTT,
} JpegeVpu720EncCmd;

typedef enum JPEGVpu720ColorRangeTrans_t {
    JPEG_VPU720_COLOR_RANGE_FULL_TO_LIMIT,
    JPEG_VPU720_COLOR_RANGE_LIMIT_TO_FULL,
} JPEGVpu720ColorRangeTrans;

typedef enum JPEGVpu720ChromaDownSampleMode_t {
    JPEG_VPU720_CHROMA_DOWN_SAMPLE_MODE_Average,
    JPEG_VPU720_CHROMA_DOWN_SAMPLE_MODE_Discard,
} JPEGVpu720ChromaDownSampleMode;

typedef struct JpegeVpu720FmtCfg_t {
    JpegeVpu720InFmt input_format;
    JpegeVpu720OutFmt out_format;
    MppFrameColorRange src_range;
    MppFrameColorRange dst_range;
    JPEGVpu720ChromaDownSampleMode chroma_ds_mode;
    RK_U32 uv_swap;
    RK_U32 mirror;
    RK_U32 fix_chroma_en;
    RK_U32 fix_chroma_u;
    RK_U32 fix_chroma_v;
    RK_U32 out_nb_comp;
    RK_U32 y_stride;
    RK_U32 uv_stride;
    RK_U32 u_offset;
    RK_U32 v_offset;
} JpegeVpu720FmtCfg;

typedef struct JpegeVpu720HalCtx_t {
    MppEncHalApi        api;
    MppDev              dev;
    void                *regs;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_cnt;
    MppEncCfgSet        *cfg;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_S32              max_buf_cnt;
    RK_S32              hdr_status;
    JpegeVpu720FmtCfg   fmt_cfg;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    RK_U32              frame_num;

    JpegeBits           bits;
    JpegeSyntax         syntax;
    RK_S32              hal_start_pos;

    MppBufferGroup      group;
    MppBuffer           qtbl_buffer;
    RK_U16              *qtbl_sw_buf;
    HalJpegeRc          hal_rc;
} JpegeVpu720HalCtx;

#define JPEGE_VPU720_QTABLE_SIZE (64 * 3)

static MPP_RET hal_jpege_vpu720_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *)hal;

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);

    hal_jpege_enter();

    ctx->regs   = mpp_calloc(JpegeVpu720Reg, 1);
    ctx->cfg    = cfg->cfg;

    ctx->frame_cnt = 0;
    ctx->enc_mode = JPEG_VPU720_ENC_MODE_ONE_FRAME;
    cfg->type = VPU_CLIENT_JPEG_ENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }

    ctx->dev = cfg->dev;
    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);
    if (ctx->group == NULL) {
        ret = mpp_buffer_group_get_internal(&ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err_f("mpp_buffer_group_get failed ret %d\n", ret);
            return ret;
        }
    }

    ret = mpp_buffer_get(ctx->group, &ctx->qtbl_buffer, JPEGE_VPU720_QTABLE_SIZE * sizeof(RK_U16));
    mpp_buffer_attach_dev(ctx->qtbl_buffer, ctx->dev);
    ctx->qtbl_sw_buf = (RK_U16 *)mpp_calloc(RK_U16, JPEGE_VPU720_QTABLE_SIZE);
    hal_jpege_rc_init(&ctx->hal_rc);

    hal_jpege_leave();
    return ret;
}

static MPP_RET hal_jpege_vpu720_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *)hal;

    hal_jpege_enter();
    jpege_bits_deinit(ctx->bits);

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->qtbl_sw_buf);

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    if (ctx->qtbl_buffer) {
        ret = mpp_buffer_put(ctx->qtbl_buffer);
        if (ret) {
            mpp_err_f("put qtbl buffer failed\n");
        }
    }

    if (ctx->group) {
        ret = mpp_buffer_group_put(ctx->group);
        if (ret) {
            mpp_err_f("group free buffer failed\n");
        }
    }

    hal_jpege_leave();
    return MPP_OK;
}

static MPP_RET jpege_vpu720_setup_format(void *hal, HalEncTask *task)
{
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *) hal;
    MppFrameFormat in_fmt = ctx->cfg->prep.format & MPP_FRAME_FMT_MASK;
    JpegeVpu720FmtCfg *fmt_cfg = &ctx->fmt_cfg;
    JpegeSyntax *syntax = &ctx->syntax;
    MppFrameChromaFormat out_fmt = syntax->format_out;
    RK_U32 hor_stride = mpp_frame_get_hor_stride(task->frame);
    RK_U32 ver_stride = mpp_frame_get_ver_stride(task->frame);

    hal_jpege_enter();

    memset(fmt_cfg, 0, sizeof(JpegeVpu720FmtCfg));

    if (MPP_FRAME_FMT_IS_TILE(ctx->cfg->prep.format)) {
        switch (in_fmt) {
        case MPP_FMT_YUV400:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_TILE_400;
            fmt_cfg->y_stride = hor_stride * 4;
            fmt_cfg->out_format = JPEGE_VPU720_OUT_FMT_400;
            break;
        case MPP_FMT_YUV420P:
        case MPP_FMT_YUV420SP:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_TILE_420;
            fmt_cfg->y_stride = hor_stride * 4 * 3 / 2;
            break;
        case MPP_FMT_YUV422P:
        case MPP_FMT_YUV422SP:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_TILE_422;
            fmt_cfg->y_stride = hor_stride * 4 * 2;
            break;
        case MPP_FMT_YUV444P:
        case MPP_FMT_YUV444SP:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_TILE_444;
            fmt_cfg->y_stride = hor_stride * 4 * 3;
            break;
        default:
            mpp_err("Unsupported input format 0x%08x, with TILE mask.\n", in_fmt);
            return MPP_ERR_VALUE;
            break;
        }
    } else {
        switch (in_fmt) {
        case MPP_FMT_YUV400:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV400;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->out_format = JPEGE_VPU720_OUT_FMT_400;
            break;
        case MPP_FMT_YUV420P:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV420P;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride >> 1;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            fmt_cfg->v_offset = fmt_cfg->u_offset + fmt_cfg->uv_stride * (ver_stride >> 1);
            break;
        case MPP_FMT_YUV420SP:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV420SP;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            break;
        case MPP_FMT_YUV420SP_VU:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV420SP;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            fmt_cfg->uv_swap = 1;
            break;
        case MPP_FMT_YUV422P:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV422P;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride >> 1;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            fmt_cfg->v_offset = fmt_cfg->u_offset + fmt_cfg->uv_stride * ver_stride;
            break;
        case MPP_FMT_YUV422SP:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV422SP;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            break;
        case MPP_FMT_YUV422SP_VU:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV422SP;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            fmt_cfg->uv_swap = 1;
            break;
        case MPP_FMT_YUV422_YUYV:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUYV;
            fmt_cfg->y_stride = hor_stride;
            break;
        case MPP_FMT_YUV422_UYVY:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_UYVY;
            fmt_cfg->y_stride = hor_stride;
            break;
        case MPP_FMT_YUV422_YVYU:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUYV;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_swap = 1;
            break;
        case MPP_FMT_YUV422_VYUY:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_UYVY;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_swap = 1;
            break;
        case MPP_FMT_YUV444SP:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV444SP;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride << 1;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            break;
        case MPP_FMT_YUV444P:
            fmt_cfg->input_format = JPEGE_VPU720_IN_FMT_YUV444P;
            fmt_cfg->y_stride = hor_stride;
            fmt_cfg->uv_stride = hor_stride;
            fmt_cfg->u_offset = hor_stride * ver_stride;
            fmt_cfg->v_offset = fmt_cfg->u_offset  << 1;
            break;
        default :
            mpp_err("Unsupported input format 0x%08x\n", in_fmt);
            return MPP_ERR_VALUE;
            break;
        }
    }

    switch (out_fmt) {
    case MPP_CHROMA_400:
        ctx->fmt_cfg.out_format = JPEGE_VPU720_OUT_FMT_400;
        break;
    case MPP_CHROMA_420:
        ctx->fmt_cfg.out_format = JPEGE_VPU720_OUT_FMT_420;
        break;
    case MPP_CHROMA_422:
        ctx->fmt_cfg.out_format = JPEGE_VPU720_OUT_FMT_422;
        break;
    case MPP_CHROMA_444:
        ctx->fmt_cfg.out_format = JPEGE_VPU720_OUT_FMT_444;
        break;
    default:
        ctx->fmt_cfg.out_format = JPEGE_VPU720_OUT_FMT_420;
        break;
    }
    ctx->fmt_cfg.out_nb_comp = ctx->syntax.nb_components;

    switch (ctx->cfg->prep.chroma_ds_mode) {
    case MPP_FRAME_CHORMA_DOWN_SAMPLE_MODE_AVERAGE:
        ctx->fmt_cfg.chroma_ds_mode = JPEG_VPU720_CHROMA_DOWN_SAMPLE_MODE_Average;
        break;
    case MPP_FRAME_CHORMA_DOWN_SAMPLE_MODE_DISCARD:
        ctx->fmt_cfg.chroma_ds_mode = JPEG_VPU720_CHROMA_DOWN_SAMPLE_MODE_Discard;
        break;
    default:
        ctx->fmt_cfg.chroma_ds_mode = JPEG_VPU720_CHROMA_DOWN_SAMPLE_MODE_Average;
        break;
    }

    hal_jpege_dbg_detail("JPEG format: in 0x%x out 0x%x, hw in_fmt %d, out_fmt %d, ds_mode %d\n",
                         ctx->cfg->prep.format, ctx->cfg->prep.format_out,
                         ctx->fmt_cfg.input_format, ctx->fmt_cfg.out_format,
                         ctx->fmt_cfg.chroma_ds_mode);

    if (ctx->cfg->prep.mirroring)
        ctx->fmt_cfg.mirror = 1;

    if (ctx->cfg->prep.fix_chroma_en) {
        ctx->fmt_cfg.fix_chroma_en = 1;
        ctx->fmt_cfg.fix_chroma_u = ctx->cfg->prep.fix_chroma_u & 0xff;
        ctx->fmt_cfg.fix_chroma_v = ctx->cfg->prep.fix_chroma_v & 0xff;
    }

    ctx->fmt_cfg.src_range = (ctx->cfg->prep.range == MPP_FRAME_RANGE_UNSPECIFIED) ?
                             MPP_FRAME_RANGE_JPEG : ctx->cfg->prep.range;
    ctx->fmt_cfg.dst_range = (ctx->cfg->prep.range_out == MPP_FRAME_RANGE_UNSPECIFIED) ?
                             MPP_FRAME_RANGE_JPEG : ctx->cfg->prep.range_out;
    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_vpu720_gen_regs(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *) hal;
    JpegeVpu720Reg *regs = ctx->regs;
    JpegeVpu720BaseReg *reg_base = &regs->reg_base;
    JpegeBits bits = ctx->bits;
    size_t length = mpp_packet_get_length(task->packet);
    RK_U8 *buf = mpp_buffer_get_ptr(task->output);
    size_t size = mpp_buffer_get_size(task->output);
    JpegeSyntax *syntax = &ctx->syntax;
    RK_U8 *qtbl_base = (RK_U8 *)mpp_buffer_get_ptr(ctx->qtbl_buffer);
    RK_S32 bitpos;
    RK_U32 i, j;
    RK_U32 encode_width;
    RK_U32 encode_height;

    hal_jpege_enter();

    jpege_vpu720_setup_format(hal, task);

    memset(regs, 0, sizeof(JpegeVpu720Reg));

    mpp_buffer_sync_begin(task->output);

    if (syntax->q_mode == JPEG_QFACTOR) {
        syntax->q_factor = 100 - task->rc_task->info.quality_target;
        hal_jpege_rc_update(&ctx->hal_rc, syntax);
    }

    jpege_bits_setup(bits, buf, (RK_U32)size);
    jpege_seek_bits(bits, length << 3);
    write_jpeg_header(bits, syntax, &ctx->hal_rc);
    mpp_buffer_sync_end(task->output);

    bitpos = jpege_bits_get_bitpos(bits);
    task->length = (bitpos + 7) >> 3;

    mpp_packet_set_length(task->packet, task->length);

    reg_base->reg001_enc_strt.lkt_num = 0;
    reg_base->reg001_enc_strt.vepu_cmd = ctx->enc_mode;

    // interupt
    reg_base->reg004_int_en.fenc_done_en = 1;
    reg_base->reg004_int_en.lkt_node_done_en = 1;
    reg_base->reg004_int_en.sclr_done_en = 1;
    reg_base->reg004_int_en.vslc_done_en = 1;
    reg_base->reg004_int_en.vbsb_oflw_en = 1;
    reg_base->reg004_int_en.vbsb_sct_en = 1;
    reg_base->reg004_int_en.fenc_err_en = 1;
    reg_base->reg004_int_en.wdg_en = 1;
    reg_base->reg004_int_en.lkt_oerr_en = 1;
    reg_base->reg004_int_en.lkt_estp_en = 1;
    reg_base->reg004_int_en.lkt_fstp_en = 1;
    reg_base->reg004_int_en.lkt_note_stp_en = 1;
    reg_base->reg004_int_en.lkt_data_error_en = 1;

    reg_base->reg005_int_msk.fenc_done_msk = 1;
    reg_base->reg005_int_msk.lkt_node_done_msk = 1;
    reg_base->reg005_int_msk.sclr_done_msk = 1;
    reg_base->reg005_int_msk.vslc_done_msk = 1;
    reg_base->reg005_int_msk.vbsb_oflw_msk = 1;
    reg_base->reg005_int_msk.vbsb_sct_msk = 1;
    reg_base->reg005_int_msk.fenc_err_msk = 1;
    reg_base->reg005_int_msk.wdg_msk = 1;
    reg_base->reg005_int_msk.lkt_oerr_msk = 1;
    reg_base->reg005_int_msk.lkt_estp_msk = 1;
    reg_base->reg005_int_msk.lkt_fstp_msk = 1;
    reg_base->reg005_int_msk.lkt_note_stp_msk = 1;
    reg_base->reg005_int_msk.lkt_data_error_msk = 1;

    reg_base->reg008_cru_ctrl.resetn_hw_en = 1;
    reg_base->reg008_cru_ctrl.sram_ckg_en = 1;
    reg_base->reg008_cru_ctrl.cke = 1;

    reg_base->reg042_dbus_endn.jbsw_bus_edin = 0xf;
    reg_base->reg042_dbus_endn.vsl_bus_edin = 0;
    reg_base->reg042_dbus_endn.ecs_len_edin = 0xf;
    reg_base->reg042_dbus_endn.sw_qtbl_edin = 0;

    reg_base->reg011_wdg_jpeg = syntax->mcu_cnt * 1000;

    reg_base->reg026_axi_perf_ctrl0.perf_work_e = 1;
    reg_base->reg026_axi_perf_ctrl0.perf_clr_e = 1;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            ctx->qtbl_sw_buf[i * 8 + j] = 0x8000 / ctx->hal_rc.qtables[0][j * 8 + i];
            ctx->qtbl_sw_buf[64 + i * 8 + j] = 0x8000 / ctx->hal_rc.qtables[1][j * 8 + i];
        }
    }

    memcpy(&ctx->qtbl_sw_buf[64 * 2], &ctx->qtbl_sw_buf[64], sizeof(RK_U16) * 64);

    encode_width = MPP_ALIGN(syntax->width, syntax->mcu_width);
    encode_height = MPP_ALIGN(syntax->height, syntax->mcu_height);
    reg_base->reg029_sw_enc_rsl.pic_wd8_m1 = encode_width / 8 - 1;
    reg_base->reg029_sw_enc_rsl.pic_hd8_m1 = encode_height / 8 - 1;
    reg_base->reg030_sw_src_fill.pic_wfill_jpeg = encode_width - syntax->width;
    reg_base->reg030_sw_src_fill.pic_hfill_jpeg = encode_height - syntax->height;

    reg_base->reg032_sw_src_fmt.src_fmt = ctx->fmt_cfg.input_format;
    reg_base->reg032_sw_src_fmt.out_fmt = ctx->fmt_cfg.out_format;
    reg_base->reg032_sw_src_fmt.rbuv_swap_jpeg = ctx->fmt_cfg.uv_swap;
    reg_base->reg032_sw_src_fmt.chroma_ds_mode = ctx->fmt_cfg.chroma_ds_mode;
    reg_base->reg032_sw_src_fmt.src_mirr_jpeg = ctx->fmt_cfg.mirror;
    reg_base->reg032_sw_src_fmt.chroma_force_en = ctx->fmt_cfg.fix_chroma_en;
    reg_base->reg032_sw_src_fmt.u_force_value = ctx->fmt_cfg.fix_chroma_u;
    reg_base->reg032_sw_src_fmt.v_force_value = ctx->fmt_cfg.fix_chroma_v;

    if (ctx->fmt_cfg.src_range != ctx->fmt_cfg.dst_range) {
        reg_base->reg032_sw_src_fmt.src_range_trns_en = 1;
        if (ctx->fmt_cfg.src_range == MPP_FRAME_RANGE_MPEG)
            reg_base->reg032_sw_src_fmt.src_range_trns_sel = JPEG_VPU720_COLOR_RANGE_LIMIT_TO_FULL;
        else
            reg_base->reg032_sw_src_fmt.src_range_trns_sel = JPEG_VPU720_COLOR_RANGE_FULL_TO_LIMIT;
    }

    reg_base->reg033_sw_pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);
    reg_base->reg033_sw_pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);

    reg_base->reg034_sw_src_strd_0.src_strd_0 = ctx->fmt_cfg.y_stride;
    reg_base->reg035_sw_src_strd_1.src_strd_1 = ctx->fmt_cfg.uv_stride;

    reg_base->reg036_sw_jpeg_enc_cfg.rst_intv = syntax->restart_ri;
    reg_base->reg036_sw_jpeg_enc_cfg.rst_m = 0;
    reg_base->reg036_sw_jpeg_enc_cfg.pic_last_ecs = 1;

    reg_base->reg022_adr_src0 = mpp_buffer_get_fd(task->input);
    reg_base->reg023_adr_src1 = reg_base->reg022_adr_src0;
    reg_base->reg024_adr_src2 = reg_base->reg022_adr_src0;

    reg_base->reg017_adr_bsbt = mpp_buffer_get_fd(task->output);
    reg_base->reg018_adr_bsbb = reg_base->reg017_adr_bsbt;
    reg_base->reg019_adr_bsbr = reg_base->reg017_adr_bsbt;
    reg_base->reg020_adr_bsbs = reg_base->reg017_adr_bsbt;

    reg_base->reg016_adr_qtbl = mpp_buffer_get_fd(ctx->qtbl_buffer);
    memcpy(qtbl_base, ctx->qtbl_sw_buf, JPEGE_VPU720_QTABLE_SIZE * sizeof(RK_U16));
    mpp_buffer_sync_end(ctx->qtbl_buffer);

    mpp_dev_set_reg_offset(ctx->dev, 20, mpp_packet_get_length(task->packet));
    mpp_dev_set_reg_offset(ctx->dev, 17, mpp_buffer_get_size(task->output));
    mpp_dev_set_reg_offset(ctx->dev, 23, ctx->fmt_cfg.u_offset);
    mpp_dev_set_reg_offset(ctx->dev, 24, ctx->fmt_cfg.v_offset);

    ctx->frame_num++;

    hal_jpege_leave();
    return ret;
}

MPP_RET hal_jpege_vpu720_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *) hal;
    JpegeVpu720Reg *regs = ctx->regs;
    MppDevRegWrCfg cfg_base;
    MppDevRegRdCfg cfg_st;

    hal_jpege_enter();

    if (task->flags.err) {
        mpp_err_f("task->flags.err 0x%08x, return early\n", task->flags.err);
        return MPP_NOK;
    }

    if (hal_jpege_debug & HAL_JPEGE_DBG_DETAIL) {
        RK_U32 i = 0;
        RK_U32 *reg = (RK_U32 *)regs;

        for (i = 0; i < 43; i++) {
            mpp_log_f("set reg[%03d] : %04x : 0x%08x\n", i, i * 4, reg[i]);
        }
    }

    cfg_base.reg = &regs->reg_base;
    cfg_base.size = sizeof(JpegeVpu720BaseReg);
    cfg_base.offset = 0;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg_base);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg_st.reg = &regs->int_state;
    cfg_st.size = sizeof(RK_U32);
    cfg_st.offset = JPEGE_VPU720_REG_BASE_INT_STATE;
    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg_st);
    if (ret) {
        mpp_err_f("set register to read int state failed %d\n", ret);
    }
    cfg_st.reg = &regs->reg_st;
    cfg_st.size = sizeof(JpegeVpu720StatusReg);
    cfg_st.offset = JPEGE_VPU720_REG_STATUS_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg_st);

    if (ret) {
        mpp_err_f("set register to read hw status failed %d\n", ret);
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
    }

    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_vpu720_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *) hal;
    JpegeVpu720Reg *regs = (JpegeVpu720Reg *)ctx->regs;
    JpegeVpu720StatusReg *reg_st = &regs->reg_st;
    RK_U32 int_state = regs->int_state;

    hal_jpege_enter();

    if (task->flags.err) {
        mpp_err_f("task->flags.err 0x%08x, return earyl\n", task->flags.err);
        return ret = MPP_NOK;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret) {
        mpp_err_f("poll cmd failed %d\n", ret);
        return ret = MPP_ERR_VPUHW;
    } else {
        if (int_state & 0x170)
            mpp_err_f("JPEG encoder hw error 0x%08x\n", int_state);
        else
            hal_jpege_dbg_simple("JPEG encoder int state 0x%08x\n", int_state);

        hal_jpege_dbg_detail("hw length %d, cycle %d\n",
                             reg_st->st_bsl_l32_jpeg_head_bits,
                             reg_st->st_perf_working_cnt);
        task->hw_length += reg_st->st_bsl_l32_jpeg_head_bits;
    }

    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_vpu720_get_task(void *hal, HalEncTask *task)
{
    JpegeVpu720HalCtx *ctx = (JpegeVpu720HalCtx *) hal;
    JpegeSyntax *syntax = (JpegeSyntax *) task->syntax.data;

    hal_jpege_enter();
    memcpy(&ctx->syntax, syntax, sizeof(ctx->syntax));

    if (ctx->cfg->jpeg.update) {
        hal_jpege_rc_update(&ctx->hal_rc, syntax);
        ctx->cfg->jpeg.update = 0;
    }

    task->rc_task->frm.is_intra = 1;

    // TODO config rc
    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_vpu720_ret_task(void *hal, HalEncTask *task)
{
    (void)hal;
    EncRcTaskInfo * rc_info = &task->rc_task->info;

    hal_jpege_enter();

    task->length += task->hw_length;

    // setup bit length for rate control
    rc_info->bit_real = task->hw_length * 8;
    rc_info->quality_real = rc_info->quality_target;
    mpp_buffer_sync_ro_begin(task->output);

    hal_jpege_leave();
    return MPP_OK;
}
const MppEncHalApi hal_jpege_vpu720 = {
    .name = "hal_jpege_vpu720",
    .coding = MPP_VIDEO_CodingMJPEG,
    .ctx_size = sizeof(JpegeVpu720HalCtx),
    .flag = 0,
    .init = hal_jpege_vpu720_init,
    .deinit = hal_jpege_vpu720_deinit,
    .prepare = NULL,
    .get_task = hal_jpege_vpu720_get_task,
    .gen_regs = hal_jpege_vpu720_gen_regs,
    .start = hal_jpege_vpu720_start,
    .wait = hal_jpege_vpu720_wait,
    .part_start = NULL,
    .part_wait = NULL,
    .ret_task = hal_jpege_vpu720_ret_task,
};
