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

#define MODULE_TAG "hal_jpege_vepu2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_platform.h"

#include "mpp_enc_hal.h"
#include "vcodec_service.h"

#include "hal_jpege_debug.h"
#include "hal_jpege_api_v2.h"
#include "hal_jpege_base.h"

#define VEPU_JPEGE_VEPU2_NUM_REGS   184
#define VEPU2_REG_INPUT_Y           48
#define VEPU2_REG_INPUT_U           49
#define VEPU2_REG_INPUT_V           50

typedef struct jpege_vepu2_reg_set_t {
    RK_U32  val[VEPU_JPEGE_VEPU2_NUM_REGS];
} jpege_vepu2_reg_set;

MPP_RET hal_jpege_vepu2_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);
    hal_jpege_dbg_func("enter hal %p cfg %p\n", hal, cfg);

    /* update output to MppEnc */
    cfg->type = VPU_CLIENT_VEPU2;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }
    ctx->dev = cfg->dev;

    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);

    memset(&ctx->hal_rc, 0, sizeof(ctx->hal_rc));
    ctx->cfg = cfg->cfg;
    ctx->reg_size = sizeof(RK_U32) * VEPU_JPEGE_VEPU2_NUM_REGS;
    ctx->regs = mpp_calloc_size(void, ctx->reg_size + EXTRA_INFO_SIZE);
    if (NULL == ctx->regs) {
        mpp_err_f("failed to malloc vepu2 regs\n");
        return MPP_NOK;
    }

    ctx->regs_out = mpp_calloc_size(void, ctx->reg_size + EXTRA_INFO_SIZE);
    if (NULL == ctx->regs_out) {
        mpp_err_f("failed to malloc vepu2 regs\n");
        return MPP_NOK;
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_deinit(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx->bits) {
        jpege_bits_deinit(ctx->bits);
        ctx->bits = NULL;
    }

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->regs_out);
    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_get_task(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeSyntax *syntax = (JpegeSyntax *)task->syntax.data;
    hal_jpege_dbg_func("enter hal %p\n", hal);

    memcpy(&ctx->syntax, syntax, sizeof(ctx->syntax));
    /* Set rc paramters */
    hal_jpege_dbg_input("rc_mode %d\n", ctx->cfg->rc.rc_mode);
    if (ctx->cfg->rc.rc_mode != MPP_ENC_RC_MODE_FIXQP) {
        if (!ctx->hal_rc.q_factor) {
            task->rc_task->info.quality_target = syntax->q_factor ? (100 - syntax->q_factor) : 80;
            task->rc_task->info.quality_min = 100 - syntax->qf_max;
            task->rc_task->info.quality_max = 100 - syntax->qf_min;
            task->rc_task->frm.is_intra = 1;
        } else {
            task->rc_task->info.quality_target = ctx->hal_rc.last_quality;
            task->rc_task->info.quality_min = 100 - syntax->qf_max;
            task->rc_task->info.quality_max = 100 - syntax->qf_min;
        }
    }
    ctx->hal_start_pos = mpp_packet_get_length(task->packet);

    /* prepare for part encoding */
    ctx->mcu_y = 0;
    ctx->mcu_h = syntax->mcu_h;
    ctx->sw_bit = 0;
    ctx->part_bytepos = 0;
    ctx->part_x_fill = 0;
    ctx->part_y_fill = 0;
    ctx->rst_marker_idx = 0;
    task->part_first = 1;
    task->part_last = 0;

    hal_jpege_dbg_func("leave hal %p\n", hal);

    return MPP_OK;
}

static MPP_RET hal_jpege_vepu2_set_extra_info(MppDev dev, JpegeSyntax *syntax,
                                              RK_U32 start_mbrow)
{
    VepuOffsetCfg cfg;
    MppDevRegOffsetCfg trans_cfg;

    cfg.fmt = syntax->format;
    cfg.width = syntax->width;
    cfg.height = syntax->height;
    cfg.hor_stride = syntax->hor_stride;
    cfg.ver_stride = syntax->ver_stride;
    cfg.offset_x = syntax->offset_x;
    cfg.offset_y = syntax->offset_y + start_mbrow * 16;

    get_vepu_offset_cfg(&cfg);

    if (cfg.offset_byte[0]) {
        trans_cfg.reg_idx = VEPU2_REG_INPUT_Y;
        trans_cfg.offset = cfg.offset_byte[0];

        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (cfg.offset_byte[1]) {
        trans_cfg.reg_idx = VEPU2_REG_INPUT_U;
        trans_cfg.offset = cfg.offset_byte[1];

        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (cfg.offset_byte[2]) {
        trans_cfg.reg_idx = VEPU2_REG_INPUT_V;
        trans_cfg.offset = cfg.offset_byte[2];

        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_gen_regs(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    MppBuffer input  = task->input;
    MppBuffer output = task->output;
    JpegeSyntax *syntax = &ctx->syntax;
    RK_U32 width        = syntax->width;
    RK_U32 width_align  = MPP_ALIGN(width, 16);
    RK_U32 height       = syntax->height;
    MppFrameFormat fmt  = syntax->format;
    RK_U32 hor_stride   = 0;
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    JpegeBits bits      = ctx->bits;
    RK_U32 *regs = (RK_U32 *)ctx->regs;
    size_t length = mpp_packet_get_length(task->packet);
    RK_U8  *buf = mpp_buffer_get_ptr(output);
    size_t size = mpp_buffer_get_size(output);
    const RK_U8 *qtable[2] = {NULL};
    RK_S32 bitpos;
    RK_S32 bytepos;
    RK_U32 x_fill = 0;
    RK_U32 y_fill = 0;
    VepuFormatCfg fmt_cfg;
    RK_U32 rotation = 0;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (syntax->rotation == MPP_ENC_ROT_90)
        rotation = 1;
    else if (syntax->rotation == MPP_ENC_ROT_270)
        rotation = 2;
    else if (syntax->rotation != MPP_ENC_ROT_0)
        mpp_err_f("Warning: only support 90 or 270 degree rotate, request rotate %d", syntax->rotation);
    if (rotation) {
        MPP_SWAP(RK_U32, width, height);
        MPP_SWAP(RK_U32, width_align, ver_stride);
    }
    hor_stride = get_vepu_pixel_stride(&ctx->stride_cfg, width,
                                       syntax->hor_stride, fmt);

    //hor_stride must be align with 8, and ver_stride mus align with 2
    if ((hor_stride & 0x7) || (ver_stride & 0x1) || (hor_stride >= (1 << 15))) {
        mpp_err_f("illegal resolution, hor_stride %d, ver_stride %d, width %d, height %d\n",
                  syntax->hor_stride, syntax->ver_stride,
                  syntax->width, syntax->height);
    }

    x_fill = (width_align - width) / 4;
    y_fill = (ver_stride - height);
    mpp_assert(x_fill <= 3);
    mpp_assert(y_fill <= 15);
    ctx->part_x_fill = x_fill;
    ctx->part_y_fill = y_fill;

    /* write header to output buffer */
    jpege_bits_setup(bits, buf, (RK_U32)size);
    /* seek length bytes data */
    jpege_seek_bits(bits, length << 3);
    /* NOTE: write header will update qtable */
    if (ctx->cfg->rc.rc_mode != MPP_ENC_RC_MODE_FIXQP) {
        hal_jpege_vepu_rc(ctx, task);
        qtable[0] = ctx->hal_rc.qtable_y;
        qtable[1] = ctx->hal_rc.qtable_c;
    } else {
        qtable[0] = NULL;
        qtable[1] = NULL;
    }
    write_jpeg_header(bits, syntax, qtable);

    memset(regs, 0, sizeof(RK_U32) * VEPU_JPEGE_VEPU2_NUM_REGS);
    // input address setup
    regs[VEPU2_REG_INPUT_Y] = mpp_buffer_get_fd(input);
    regs[VEPU2_REG_INPUT_U] = regs[VEPU2_REG_INPUT_Y];
    regs[VEPU2_REG_INPUT_V] = regs[VEPU2_REG_INPUT_Y];

    // output address setup
    bitpos = jpege_bits_get_bitpos(bits);
    bytepos = (bitpos + 7) >> 3;
    ctx->base = buf;
    ctx->size = size;
    ctx->sw_bit = bitpos;
    ctx->part_bytepos = bytepos;

    get_msb_lsb_at_pos(&regs[51], &regs[52], buf, bytepos);

    regs[53] = size - bytepos;

    // bus config
    regs[54] = 16 << 8;

    regs[60] = (((bytepos & 7) * 8) << 16) |
               (x_fill << 4) |
               (y_fill);
    regs[61] = hor_stride;

    regs[77] = mpp_buffer_get_fd(output);
    if (bytepos)
        mpp_dev_set_reg_offset(ctx->dev, 77, bytepos);

    /* 95 - 97 color conversion parameter */
    {
        RK_U32 coeffA;
        RK_U32 coeffB;
        RK_U32 coeffC;
        RK_U32 coeffE;
        RK_U32 coeffF;

        switch (syntax->color_conversion_type) {
        case 0 : {  /* BT.601 */
            /*
             * Y  = 0.2989 R + 0.5866 G + 0.1145 B
             * Cb = 0.5647 (B - Y) + 128
             * Cr = 0.7132 (R - Y) + 128
             */
            coeffA = 19589;
            coeffB = 38443;
            coeffC = 7504;
            coeffE = 37008;
            coeffF = 46740;
        } break;
        case 1 : {  /* BT.709 */
            /*
             * Y  = 0.2126 R + 0.7152 G + 0.0722 B
             * Cb = 0.5389 (B - Y) + 128
             * Cr = 0.6350 (R - Y) + 128
             */
            coeffA = 13933;
            coeffB = 46871;
            coeffC = 4732;
            coeffE = 35317;
            coeffF = 41615;
        } break;
        case 2 : {
            coeffA = syntax->coeffA;
            coeffB = syntax->coeffB;
            coeffC = syntax->coeffC;
            coeffE = syntax->coeffE;
            coeffF = syntax->coeffF;
        } break;
        default : {
            mpp_err("invalid color conversion type %d\n",
                    syntax->color_conversion_type);
            coeffA = 19589;
            coeffB = 38443;
            coeffC = 7504;
            coeffE = 37008;
            coeffF = 46740;
        } break;
        }

        regs[95] = coeffA | (coeffB << 16);
        regs[96] = coeffC | (coeffE << 16);
        regs[97] = coeffF;
    }

    regs[103] = (width_align >> 4) << 8  |
                (ver_stride >> 4) << 20 |
                (1 << 6) |  /* intra coding  */
                (2 << 4) |  /* format jpeg   */
                1;          /* encoder start */

    if (!get_vepu_fmt(&fmt_cfg, fmt)) {
        regs[74] = (fmt_cfg.format << 4) |
                   (rotation << 2);
        regs[98] = (fmt_cfg.b_mask & 0x1f) << 16 |
                   (fmt_cfg.g_mask & 0x1f) << 8  |
                   (fmt_cfg.r_mask & 0x1f);
        regs[105] = 7 << 26 | (fmt_cfg.swap_32_in & 1) << 29 |
                    (fmt_cfg.swap_16_in & 1) << 30 |
                    (fmt_cfg.swap_8_in & 1) << 31;
    }

    regs[107] = ((syntax->part_rows & 0xff) << 16) |
                jpege_restart_marker[ctx->rst_marker_idx & 7];

    /* encoder interrupt */
    regs[109] = 1 << 12 |   /* clock gating */
                1 << 10;    /* enable timeout interrupt */

    if (syntax->low_delay) {
        /* slice encode end by RST */
        regs[107] |= (1 << 24);
        /* slice interrupt enable */
        regs[109] |= (1 << 16);
    }

    /* 0 ~ 31 quantization tables */
    {
        RK_S32 i;

        for (i = 0; i < 16; i++) {
            /* qtable need to reorder in particular order */
            regs[i] = qtable[0][qp_reorder_table[i * 4 + 0]] << 24 |
                      qtable[0][qp_reorder_table[i * 4 + 1]] << 16 |
                      qtable[0][qp_reorder_table[i * 4 + 2]] << 8 |
                      qtable[0][qp_reorder_table[i * 4 + 3]];
        }
        for (i = 0; i < 16; i++) {
            /* qtable need to reorder in particular order */
            regs[i + 16] = qtable[1][qp_reorder_table[i * 4 + 0]] << 24 |
                           qtable[1][qp_reorder_table[i * 4 + 1]] << 16 |
                           qtable[1][qp_reorder_table[i * 4 + 2]] << 8 |
                           qtable[1][qp_reorder_table[i * 4 + 3]];
        }
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_vepu2_set_extra_info(ctx->dev, &ctx->syntax, 0);

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = ctx->reg_size;

        wr_cfg.reg = ctx->regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = ctx->regs;
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

    hal_jpege_dbg_func("leave hal %p\n", hal);
    (void)task;
    return ret;
}

MPP_RET hal_jpege_vepu2_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeBits bits = ctx->bits;
    RK_U32 *regs = ctx->regs;
    JpegeFeedback *feedback = &ctx->feedback;
    RK_U32 val;
    RK_U32 sw_bit = 0;
    RK_U32 hw_bit = 0;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx->dev) {
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);
    }

    val = regs[109];
    hal_jpege_dbg_output("hw_status %08x\n", val);
    feedback->hw_status = val & 0x70;
    val = regs[53];

    sw_bit = jpege_bits_get_bitpos(bits);
    hw_bit = val;

    // NOTE: hardware will return 64 bit access byte count
    feedback->stream_length = ((sw_bit / 8) & (~0x7)) + hw_bit / 8;
    task->length = feedback->stream_length;
    task->hw_length = task->length - ctx->hal_start_pos;

    hal_jpege_dbg_output("stream bit: sw %d hw %d total %d hw_length %d\n",
                         sw_bit, hw_bit, feedback->stream_length, task->hw_length);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_vepu2_part_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeSyntax *syntax = (JpegeSyntax *)task->syntax.data;
    RK_U32 mcu_w = syntax->mcu_w;
    RK_U32 mcu_h = syntax->mcu_h;
    RK_U32 mcu_y = ctx->mcu_y;
    RK_U32 part_mcu_h = syntax->part_rows;
    RK_U32 *regs = (RK_U32 *)ctx->regs;
    RK_U32 part_enc_h;
    RK_U32 part_enc_mcu_h;
    RK_U32 part_y_fill;
    RK_U32 part_not_end;

    hal_jpege_dbg_func("enter part start %p\n", hal);

    /* Fix register for each part encoding */
    task->part_first = !mcu_y;
    if (mcu_y + part_mcu_h < mcu_h) {
        part_enc_h = part_mcu_h * 16;
        part_enc_mcu_h = part_mcu_h;
        part_y_fill = 0;
        part_not_end = 1;
        task->part_last = 0;
    } else {
        part_enc_h = syntax->height - mcu_y * 16;
        part_enc_mcu_h = MPP_ALIGN(part_enc_h, 16) / 16;;
        part_y_fill = ctx->part_y_fill;
        part_not_end = 0;
        task->part_last = 1;
    }

    hal_jpege_dbg_detail("part first %d last %d\n", task->part_first, task->part_last);

    get_msb_lsb_at_pos(&regs[51], &regs[52], ctx->base, ctx->part_bytepos);

    regs[53] = ctx->size - ctx->part_bytepos;

    regs[60] = (((ctx->part_bytepos & 7) * 8) << 16) |
               (ctx->part_x_fill << 4) |
               (part_y_fill);

    regs[77] = mpp_buffer_get_fd(task->output);
    if (ctx->part_bytepos)
        mpp_dev_set_reg_offset(ctx->dev, 77, ctx->part_bytepos);

    regs[103] = mcu_w << 8  |
                (part_enc_mcu_h) << 20 |
                (1 << 6) |  /* intra coding  */
                (2 << 4) |  /* format jpeg   */
                1;          /* encoder start */

    regs[107] = part_not_end << 24 | jpege_restart_marker[ctx->rst_marker_idx & 7];
    ctx->rst_marker_idx++;

    hal_jpege_vepu2_set_extra_info(ctx->dev, syntax, mcu_y);
    ctx->mcu_y += part_enc_mcu_h;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = ctx->reg_size;

        wr_cfg.reg = ctx->regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = ctx->regs_out;
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

    hal_jpege_dbg_func("leave part start %p\n", hal);
    (void)task;
    return ret;
}

MPP_RET hal_jpege_vepu2_part_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    RK_U32 *regs = ctx->regs_out;
    JpegeFeedback *feedback = &ctx->feedback;
    RK_U32 hw_bit = 0;

    hal_jpege_dbg_func("enter part wait %p\n", hal);

    if (ctx->dev) {
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);
    }

    hal_jpege_dbg_detail("hw_status %08x\n", regs[109]);

    hw_bit = regs[53];

    hal_jpege_dbg_detail("byte pos %d -> %d\n", ctx->part_bytepos,
                         (ctx->part_bytepos & (~7)) + (hw_bit / 8));
    ctx->part_bytepos = (ctx->part_bytepos & (~7)) + (hw_bit / 8);

    feedback->stream_length = ctx->part_bytepos;
    task->length = ctx->part_bytepos;
    task->hw_length = task->length - ctx->hal_start_pos;

    hal_jpege_dbg_detail("stream_length %d, hw_byte %d",
                         feedback->stream_length, hw_bit / 8);

    hal_jpege_dbg_output("stream bit: sw %d hw %d total %d hw_length %d\n",
                         ctx->sw_bit, hw_bit, feedback->stream_length, task->hw_length);

    hal_jpege_dbg_func("leave part wait %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_vepu2_ret_task(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    ctx->hal_rc.last_quality = task->rc_task->info.quality_target;
    task->rc_task->info.bit_real = ctx->feedback.stream_length * 8;
    task->hal_ret.data = &ctx->feedback;
    task->hal_ret.number = 1;

    return MPP_OK;
}

const MppEncHalApi hal_jpege_vepu2 = {
    .name       = "hal_jpege_vepu2",
    .coding     = MPP_VIDEO_CodingMJPEG,
    .ctx_size   = sizeof(HalJpegeCtx),
    .flag       = 0,
    .init       = hal_jpege_vepu2_init,
    .deinit     = hal_jpege_vepu2_deinit,
    .prepare    = NULL,
    .get_task   = hal_jpege_vepu2_get_task,
    .gen_regs   = hal_jpege_vepu2_gen_regs,
    .start      = hal_jpege_vepu2_start,
    .wait       = hal_jpege_vepu2_wait,
    .part_start = hal_jpege_vepu2_part_start,
    .part_wait  = hal_jpege_vepu2_part_wait,
    .ret_task   = hal_jpege_vepu2_ret_task,
};
