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

#define MODULE_TAG "HAL_JPEGE_VDPU2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "mpp_hal.h"

#include "jpege_syntax.h"
#include "hal_jpege_api.h"
#include "hal_jpege_hdr.h"
#include "hal_jpege_vepu2.h"
#include "hal_jpege_base.h"

#include "mpp_device.h"
#include "mpp_platform.h"


#define VEPU_JPEGE_VEPU2_NUM_REGS   184

typedef struct jpege_vepu2_reg_set_t {
    RK_U32  val[VEPU_JPEGE_VEPU2_NUM_REGS];
} jpege_vepu2_reg_set;

static const RK_U32 qp_reorder_table[64] = {
    0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
    2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
    4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
    6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

MPP_RET hal_jpege_vepu2_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);
    hal_jpege_dbg_func("enter hal %p cfg %p\n", hal, cfg);

    ctx->int_cb = cfg->hal_int_cb;

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,              /* type */
        .coding = MPP_VIDEO_CodingMJPEG,  /* coding */
        .platform = 0,                    /* platform */
        .pp_enable = 0,                   /* pp_enable */
    };

    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err_f("failed to open vpu client\n");
        return ret;
    }

    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);

    memset(&(ctx->ioctl_info), 0, sizeof(ctx->ioctl_info));
    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

    ctx->ioctl_info.regs = mpp_calloc(RK_U32, VEPU_JPEGE_VEPU2_NUM_REGS);
    if (!ctx->ioctl_info.regs) {
        mpp_err_f("failed to malloc vdpu2 regs\n");
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

    if (ctx->dev_ctx) {
        mpp_device_deinit(ctx->dev_ctx);
        ctx->dev_ctx = NULL;
    }

    mpp_free(ctx->ioctl_info.regs);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

static MPP_RET hal_jpege_vepu2_set_extra_info(RK_U32 *regs,
                                              JpegeIocExtInfo *info,
                                              JpegeSyntax *syntax)
{
    MppFrameFormat fmt  = syntax->format;
    RK_U32 hor_stride   = syntax->hor_stride;
    RK_U32 ver_stride   = syntax->ver_stride;

    if (hor_stride * ver_stride * 5 / 4 >= SZ_4M) {
        JpegeIocExtInfoSlot *slot = NULL;

        info->magic = EXTRA_INFO_MAGIC;
        info->cnt = 2;

        if (fmt == MPP_FMT_YUV420P) {
            slot = &(info->slots[0]);
            slot->reg_idx = 49;
            slot->offset = hor_stride * ver_stride;

            slot = &(info->slots[1]);
            slot->reg_idx = 50;
            slot->offset = hor_stride * ver_stride * 5 / 4;
        } else if (fmt == MPP_FMT_YUV420SP) {
            slot = &(info->slots[0]);
            slot->reg_idx = 49;
            slot->offset = hor_stride * ver_stride;

            slot = &(info->slots[1]);
            slot->reg_idx = 50;
            slot->offset = hor_stride * ver_stride;
        } else {
            mpp_log_f("other format(%d)\n", fmt);
        }
    } else {
        if (fmt == MPP_FMT_YUV420P) {
            regs[49] += (hor_stride * ver_stride) << 10;
            regs[50] += (hor_stride * ver_stride * 5 / 4) << 10;
        } else if (fmt == MPP_FMT_YUV420SP) {
            regs[49] += (hor_stride * ver_stride) << 10;
            regs[50] += (hor_stride * ver_stride) << 10;
        }
    }
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_gen_regs(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    HalEncTask *info = &task->enc;
    MppBuffer input  = info->input;
    MppBuffer output = info->output;
    JpegeSyntax *syntax = &ctx->syntax;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    MppEncCodecCfg *codec = &ctx->cfg->codec;
    RK_U32 width        = prep->width;
    RK_U32 height       = prep->height;
    MppFrameFormat fmt  = prep->format;
    RK_U32 hor_stride   = prep->hor_stride;
    RK_U32 ver_stride   = prep->ver_stride;
    JpegeBits bits      = ctx->bits;
    RK_U32 *regs = ctx->ioctl_info.regs;
    JpegeIocExtInfo *extra_info = &(ctx->ioctl_info.extra_info);
    RK_U8  *buf = mpp_buffer_get_ptr(output);
    size_t size = mpp_buffer_get_size(output);
    const RK_U8 *qtable[2];
    RK_U32 val32;
    RK_S32 bitpos;
    RK_S32 bytepos;
    RK_U32 r_mask = 0;
    RK_U32 g_mask = 0;
    RK_U32 b_mask = 0;
    RK_U32 x_fill = 0;

    syntax->width   = width;
    syntax->height  = height;
    syntax->hor_stride = prep->hor_stride;
    syntax->ver_stride = prep->ver_stride;
    syntax->format  = fmt;
    syntax->quality = codec->jpeg.quant;

    x_fill = (hor_stride - width) / 4;
    if (x_fill > 3)
        mpp_err_f("right fill is illegal, hor_stride = %d, width = %d\n", hor_stride, width);

    hal_jpege_dbg_func("enter hal %p\n", hal);

    /* write header to output buffer */
    jpege_bits_setup(bits, buf, (RK_U32)size);
    /* NOTE: write header will update qtable */
    write_jpeg_header(bits, syntax, qtable);

    memset(regs, 0, sizeof(RK_U32) * VEPU_JPEGE_VEPU2_NUM_REGS);
    // input address setup
    regs[48] = mpp_buffer_get_fd(input);
    regs[49] = mpp_buffer_get_fd(input);
    regs[50] = regs[49];
    hal_jpege_vepu2_set_extra_info(regs, extra_info, syntax);

    // output address setup
    bitpos = jpege_bits_get_bitpos(bits);
    bytepos = (bitpos + 7) >> 3;
    buf = jpege_bits_get_buf(bits);
    {
        RK_S32 left_byte = bytepos & 0x7;
        RK_U8 *tmp = buf + (bytepos & (~0x7));

        // clear the rest bytes in 64bit
        if (left_byte) {
            RK_U32 i;

            for (i = left_byte; i < 8; i++)
                tmp[i] = 0;
        }

        val32 = (tmp[0] << 24) |
                (tmp[1] << 16) |
                (tmp[2] <<  8) |
                (tmp[3] <<  0);

        regs[51] = val32;

        if (left_byte > 4) {
            val32 = (tmp[4] << 24) |
                    (tmp[5] << 16) |
                    (tmp[6] <<  8);
        } else
            val32 = 0;

        regs[52] = val32;
    }

    regs[53] = size - bytepos;

    // bus config
    regs[54] = 16 << 8;

    regs[60] = (((bytepos & 7) * 8) << 16) |
               (x_fill << 4) |
               (ver_stride - height);
    regs[61] = hor_stride;

    switch (fmt) {
    case MPP_FMT_YUV420P : {
        val32 = 0;
        r_mask = g_mask = b_mask = 0;
    } break;
    case MPP_FMT_YUV420SP : {
        val32 = 1;
        r_mask = g_mask = b_mask = 0;
    } break;
    case MPP_FMT_YUV422_YUYV : {
        val32 = 2;
        r_mask = g_mask = b_mask = 0;
    } break;
    case MPP_FMT_YUV422_UYVY : {
        val32 = 3;
        r_mask = g_mask = b_mask = 0;
    } break;
    case MPP_FMT_RGB565 : {
        val32 = 4;
        r_mask = 15;
        g_mask = 10;
        b_mask = 4;
    } break;
    case MPP_FMT_RGB444 : {
        val32 = 6;
        r_mask = 11;
        g_mask = 7;
        b_mask = 3;
    } break;
    case MPP_FMT_RGB888 : {
        val32 = 7;
        r_mask = 7;
        g_mask = 15;
        b_mask = 23;
    } break;
    case MPP_FMT_BGR888 : {
        val32 = 7;
        r_mask = 23;
        g_mask = 15;
        b_mask = 7;
    } break;
    case MPP_FMT_RGB101010 : {
        val32 = 8;
        r_mask = 29;
        g_mask = 19;
        b_mask = 9;
    } break;
    default : {
        mpp_err_f("invalid input format %d\n", fmt);
        val32 = 0;
    } break;
    }
    regs[74] = val32 << 4;

    regs[77] = mpp_buffer_get_fd(output) + (bytepos << 10);

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

    /* TODO: 98 RGB bit mask */
    regs[98] = (r_mask & 0x1f) << 16 |
               (g_mask & 0x1f) << 8  |
               (b_mask & 0x1f);

    regs[103] = (hor_stride >> 4) << 8  |
                (ver_stride >> 4) << 20 |
                (1 << 6) |  /* intra coding  */
                (2 << 4) |  /* format jpeg   */
                1;          /* encoder start */

    /* input byte swap configure */
    regs[105] = 7 << 26;
    if (fmt < MPP_FMT_RGB565) {
        // YUV format
        regs[105] |= (7 << 29);
    } else if (fmt < MPP_FMT_RGB888) {
        // 16bit RGB
        regs[105] |= (2 << 29);
    } else {
        // 32bit RGB
        regs[105] |= (0 << 29);
    }

    /* encoder interrupt */
    regs[109] = 1 << 12 |   /* clock gating */
                1 << 10;    /* enable timeout interrupt */

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

MPP_RET hal_jpege_vepu2_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    RK_U32 *cache = NULL;
    RK_U32 reg_size = sizeof(jpege_vepu2_reg_set);
    RK_U32 extra_size = (ctx->ioctl_info.extra_info.cnt) ?
                        (sizeof(JpegeIocExtInfo)) : (0);
    RK_U32 reg_num = reg_size / sizeof(RK_U32);
    RK_U32 extra_num = extra_size / sizeof(RK_U32);

    hal_jpege_dbg_func("enter hal %p\n", hal);

    cache = mpp_malloc(RK_U32, reg_num + extra_num);
    if (!cache) {
        mpp_err_f("failed to malloc reg cache\n");
        return MPP_NOK;
    }

    memcpy(cache, ctx->ioctl_info.regs, reg_size);
    memcpy(cache + reg_num, &(ctx->ioctl_info.extra_info), extra_size);

    if (ctx->dev_ctx) {
        ret = mpp_device_send_reg(ctx->dev_ctx, cache, reg_num + extra_num);
    }

    mpp_free(cache);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    (void)ctx;
    (void)task;

    return ret;
}

MPP_RET hal_jpege_vepu2_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeBits bits = ctx->bits;
    RK_U32 *regs = ctx->ioctl_info.regs;
    JpegeFeedback feedback;
    RK_U32 val;
    RK_U32 sw_bit;
    RK_U32 hw_bit;
    (void)task;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx->dev_ctx)
        ret = mpp_device_wait_reg(ctx->dev_ctx, regs, sizeof(jpege_vepu2_reg_set) / sizeof(RK_U32));

    val = regs[109];
    hal_jpege_dbg_output("hw_status %08x\n", val);
    feedback.hw_status = val & 0x70;
    val = regs[53];

    sw_bit = jpege_bits_get_bitpos(bits);
    hw_bit = val;

    // NOTE: hardware will return 64 bit access byte count
    feedback.stream_length = ((sw_bit / 8) & (~0x7)) + hw_bit / 8;
    task->enc.length = feedback.stream_length;
    hal_jpege_dbg_output("stream bit: sw %d hw %d total %d\n",
                         sw_bit, hw_bit, feedback.stream_length);

    ctx->int_cb.callBack(ctx->int_cb.opaque, &feedback);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_vepu2_reset(void *hal)
{
    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_flush(void *hal)
{
    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_control(void *hal, RK_S32 cmd, void *param)
{
    (void)hal;
    MPP_RET ret = MPP_OK;

    hal_jpege_dbg_func("enter hal %p cmd %x param %p\n", hal, cmd, param);

    switch (cmd) {
    case MPP_ENC_SET_PREP_CFG : {
        MppEncPrepCfg *cfg = (MppEncPrepCfg *)param;
        if (cfg->width < 16 && cfg->width > 8192) {
            mpp_err("jpege: invalid width %d is not in range [16..8192]\n", cfg->width);
            ret = MPP_NOK;
        }

        if (cfg->height < 16 && cfg->height > 8192) {
            mpp_err("jpege: invalid height %d is not in range [16..8192]\n", cfg->height);
            ret = MPP_NOK;
        }

        if (cfg->format != MPP_FMT_YUV420SP     &&
            cfg->format != MPP_FMT_YUV420P      &&
            cfg->format != MPP_FMT_YUV422SP_VU  &&
            cfg->format != MPP_FMT_YUV422_YUYV  &&
            cfg->format != MPP_FMT_YUV422_UYVY  &&
            cfg->format != MPP_FMT_RGB888       &&
            cfg->format != MPP_FMT_BGR888) {
            mpp_err("jpege: invalid format %d is not supportted\n", cfg->format);
            ret = MPP_NOK;
        }
    } break;
    case MPP_ENC_GET_PREP_CFG:
    case MPP_ENC_GET_CODEC_CFG:
    case MPP_ENC_SET_IDR_FRAME:
    case MPP_ENC_SET_OSD_PLT_CFG:
    case MPP_ENC_SET_OSD_DATA_CFG:
    case MPP_ENC_GET_OSD_CFG:
    case MPP_ENC_SET_EXTRA_INFO:
    case MPP_ENC_GET_EXTRA_INFO:
    case MPP_ENC_GET_SEI_DATA:
    case MPP_ENC_SET_SEI_CFG:
    case MPP_ENC_SET_RC_CFG : {
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
        MppEncJpegCfg *src = &ctx->set->codec.jpeg;
        MppEncJpegCfg *dst = &ctx->cfg->codec.jpeg;
        RK_U32 change = src->change;

        if (change & MPP_ENC_JPEG_CFG_CHANGE_QP) {
            if (src->quant < 0 || src->quant > 10) {
                mpp_err("jpege: invalid quality level %d is not in range [0..10] set to default 8\n");
                src->quant = 8;
            }
            dst->quant = src->quant;
        }

        dst->change = 0;
        src->change = 0;
    } break;
    default:
        mpp_err("No correspond cmd(%08x) found, and can not config!", cmd);
        ret = MPP_NOK;
        break;
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}
