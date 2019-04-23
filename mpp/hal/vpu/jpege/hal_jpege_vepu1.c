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

#define MODULE_TAG "HAL_JPEGE_VDPU1"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "mpp_hal.h"

#include "jpege_syntax.h"
#include "hal_jpege_api.h"
#include "hal_jpege_hdr.h"
#include "hal_jpege_base.h"
#include "hal_jpege_vepu1.h"

#include "mpp_device.h"
#include "mpp_platform.h"

#include "rk_type.h"

#define VEPU_JPEGE_VEPU1_NUM_REGS 164

typedef struct jpege_vepu1_reg_set_t {
    RK_U32  val[VEPU_JPEGE_VEPU1_NUM_REGS];
} jpege_vepu1_reg_set;

static const RK_U32 qp_reorder_table[64] = {
    0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
    2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
    4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
    6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

MPP_RET hal_jpege_vepu1_init(void *hal, MppHalCfg *cfg)
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
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }

    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);

    memset(&(ctx->ioctl_info), 0, sizeof(ctx->ioctl_info));
    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

    ctx->ioctl_info.regs = mpp_calloc(RK_U32, VEPU_JPEGE_VEPU1_NUM_REGS);
    if (!ctx->ioctl_info.regs) {
        mpp_err_f("failed to malloc vdpu1 regs\n");
        return MPP_NOK;
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu1_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx->bits) {
        jpege_bits_deinit(ctx->bits);
        ctx->bits = NULL;
    }

    if (ctx->dev_ctx) {
        ret = mpp_device_deinit(ctx->dev_ctx);
        if (ret) {
            mpp_err("mpp_device_deinit failed. ret: %d\n", ret);
        }
    }

    mpp_free(ctx->ioctl_info.regs);
    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}

static MPP_RET hal_jpege_vepu1_set_extra_info(RK_U32 *regs,
                                              JpegeIocExtInfo *info,
                                              JpegeSyntax *syntax)
{
    if (info == NULL || syntax == NULL) {
        mpp_err_f("invalid input parameter!");
        return MPP_NOK;
    }

    MppFrameFormat fmt  = syntax->format;
    RK_U32 hor_stride   = syntax->hor_stride;
    RK_U32 ver_stride   = syntax->ver_stride;

    if (hor_stride * ver_stride * 5 / 4 >= SZ_4M) {
        JpegeIocExtInfoSlot *slot = NULL;

        info->magic = EXTRA_INFO_MAGIC;
        info->cnt = 2;

        if (fmt == MPP_FMT_YUV420P) {
            slot = &(info->slots[0]);
            slot->reg_idx = 12;
            slot->offset = hor_stride * ver_stride;

            slot = &(info->slots[1]);
            slot->reg_idx = 13;
            slot->offset = hor_stride * ver_stride * 5 / 4;
        } else if (fmt == MPP_FMT_YUV420SP) {
            slot = &(info->slots[0]);
            slot->reg_idx = 12;
            slot->offset = hor_stride * ver_stride;

            slot = &(info->slots[1]);
            slot->reg_idx = 13;
            slot->offset = hor_stride * ver_stride;
        } else {
            mpp_log_f("other format(%d)\n", fmt);
        }
    } else {
        if (fmt == MPP_FMT_YUV420P) {
            regs[12] += (hor_stride * ver_stride) << 10;
            regs[13] += (hor_stride * ver_stride * 5 / 4) << 10;
        } else if (fmt == MPP_FMT_YUV420SP) {
            regs[12] += (hor_stride * ver_stride) << 10;
            regs[13] += (hor_stride * ver_stride) << 10;
        }
    }

    return MPP_OK;
}

MPP_RET hal_jpege_vepu1_gen_regs(void *hal, HalTaskInfo *task)
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
    RK_U32 deflt_cfg;
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

    memset(regs, 0, sizeof(RK_U32) * VEPU_JPEGE_VEPU1_NUM_REGS);
    regs[11] = mpp_buffer_get_fd(input);
    regs[12] = mpp_buffer_get_fd(input);
    regs[13] = regs[12];
    hal_jpege_vepu1_set_extra_info(regs, extra_info, syntax);

    bitpos = jpege_bits_get_bitpos(bits);
    bytepos = (bitpos + 7) >> 3;
    buf = jpege_bits_get_buf(bits);

    deflt_cfg =
        ((0  & (255)) << 24) |
        ((0  & (255)) << 16) |
        ((1  & (1)) << 15) |
        ((16 & (63)) << 8) |
        ((0  & (1)) << 6) |
        ((0  & (1)) << 5) |
        ((1  & (1)) << 4) |
        ((1  & (1)) << 3) |
        ((1  & (1)) << 1);

    switch (fmt) {
    case MPP_FMT_YUV420P : {
        val32 = 0;
        r_mask = 0;
        g_mask = 0;
        b_mask = 0;
    } break;
    case MPP_FMT_YUV420SP : {
        val32 = 1;
        r_mask = 0;
        g_mask = 0;
        b_mask = 0;
    } break;
    case MPP_FMT_YUV422_YUYV : {
        val32 = 2;
        r_mask = 0;
        g_mask = 0;
        b_mask = 0;
    } break;
    case MPP_FMT_YUV422_UYVY : {
        val32 = 3;
        r_mask = 0;
        g_mask = 0;
        b_mask = 0;
    } break;
    case MPP_FMT_RGB565 : {
        val32 = 4;
        r_mask = 4;
        g_mask = 10;
        b_mask = 15;
    } break;
    case MPP_FMT_RGB444 : {
        val32 = 5;
        r_mask = 3;
        g_mask = 7;
        b_mask = 11;
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
    } break;
    default : {
        mpp_err_f("invalid input format %d\n", fmt);
        val32 = 0;
    } break;
    }

    if (val32 < 4) {
        regs[2] = deflt_cfg |
                  ((1 & (1)) << 14) |
                  ((1 & (1)) << 2) |
                  (1 & (1));
    } else if (val32 < 7) {
        regs[2] = deflt_cfg |
                  ((1 & (1)) << 14) |
                  ((0 & (1)) << 2) |
                  (0 & (1));
    } else {
        regs[2] = deflt_cfg |
                  ((0 & (1)) << 14) |
                  ((0 & (1)) << 2) |
                  (0 & (1));
    }

    regs[5] = mpp_buffer_get_fd(output) + (bytepos << 10);

    regs[14] = (1 << 31) |
               (0 << 30) |
               (0 << 29) |
               ((hor_stride >> 4) << 19) |
               ((ver_stride >> 4) << 10) |
               (1 << 3) | (2 << 1);

    regs[15] = (0 << 29) |
               (0 << 26) |
               (hor_stride << 12) |
               (x_fill << 10) |
               ((ver_stride - height) << 6) |
               (val32 << 2) | (0);

    {
        RK_S32 left_byte = bytepos & 0x7;
        RK_U8 *tmp = buf + (bytepos & (~0x7));

        if (left_byte) {
            RK_U32 i;

            for (i = left_byte; i < 8; i++)
                tmp[i] = 0;
        }

        val32 = (tmp[0] << 24) |
                (tmp[1] << 16) |
                (tmp[2] <<  8) |
                (tmp[3] <<  0);

        regs[22] = val32;

        if (left_byte > 4) {
            val32 = (tmp[4] << 24) |
                    (tmp[5] << 16) |
                    (tmp[6] <<  8);
        } else
            val32 = 0;

        regs[23] = val32;
    }

    regs[24] = size - bytepos;

    regs[37] = ((bytepos & 7) * 8) << 23;

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

        regs[53] = coeffA | (coeffB << 16);
        regs[54] = coeffC | (coeffE << 16);
        regs[55] = ((r_mask & 0x1f) << 26) |
                   ((g_mask & 0x1f) << 21) |
                   ((b_mask & 0x1f) << 16) | coeffF;
    }

    regs[14] |= 0x001;

    {
        RK_S32 i;

        for (i = 0; i < 16; i++) {
            /* qtable need to reorder in particular order */
            regs[i + 64] = qtable[0][qp_reorder_table[i * 4 + 0]] << 24 |
                           qtable[0][qp_reorder_table[i * 4 + 1]] << 16 |
                           qtable[0][qp_reorder_table[i * 4 + 2]] << 8 |
                           qtable[0][qp_reorder_table[i * 4 + 3]];
        }
        for (i = 0; i < 16; i++) {
            /* qtable need to reorder in particular order */
            regs[i + 80] = qtable[1][qp_reorder_table[i * 4 + 0]] << 24 |
                           qtable[1][qp_reorder_table[i * 4 + 1]] << 16 |
                           qtable[1][qp_reorder_table[i * 4 + 2]] << 8 |
                           qtable[1][qp_reorder_table[i * 4 + 3]];
        }
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu1_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    RK_U32 *cache = NULL;
    RK_U32 reg_size = sizeof(jpege_vepu1_reg_set);
    RK_U32 extra_size = sizeof(JpegeIocExtInfo);
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

MPP_RET hal_jpege_vepu1_wait(void *hal, HalTaskInfo *task)
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
        ret = mpp_device_wait_reg(ctx->dev_ctx, regs, sizeof(jpege_vepu1_reg_set) / sizeof(RK_U32));

    val = regs[1];
    hal_jpege_dbg_output("hw_status %08x\n", val);
    feedback.hw_status = val & 0x70;
    val = regs[24];

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

MPP_RET hal_jpege_vepu1_reset(void *hal)
{
    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu1_flush(void *hal)
{
    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu1_control(void *hal, RK_S32 cmd, void *param)
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
