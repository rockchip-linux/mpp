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

#define MODULE_TAG "hal_jpege_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp_hal.h"

#include "jpege_syntax.h"
#include "hal_jpege_api.h"
#include "hal_jpege_hdr.h"

#include "vpu.h"

#define VPU2_REG_NUM    184

typedef struct hal_jpege_ctx_s {
    RK_S32          vpu_fd;

    IOInterruptCB   int_cb;
    JpegeBits       bits;
    RK_U32          regs[VPU2_REG_NUM];
} HalJpegeCtx;

#define HAL_JPEGE_DBG_FUNCTION          (0x00000001)
#define HAL_JPEGE_DBG_INPUT             (0x00000010)
#define HAL_JPEGE_DBG_OUTPUT            (0x00000020)

RK_U32 hal_jpege_debug = 0;

#define hal_jpege_dbg(flag, fmt, ...)   _mpp_dbg(hal_jpege_debug, flag, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_f(flag, fmt, ...) _mpp_dbg_f(hal_jpege_debug, flag, fmt, ## __VA_ARGS__)

#define hal_jpege_dbg_func(fmt, ...)    hal_jpege_dbg_f(HAL_JPEGE_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_input(fmt, ...)   hal_jpege_dbg(HAL_JPEGE_DBG_INPUT, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_output(fmt, ...)  hal_jpege_dbg(HAL_JPEGE_DBG_OUTPUT, fmt, ## __VA_ARGS__)

/* JPEG QUANT table order */
static const RK_U32 qp_reorder_table[64] = {
    0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
    2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
    4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
    6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

MPP_RET hal_jpege_init(void *hal, MppHalCfg *cfg)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);
    hal_jpege_dbg_func("enter hal %p cfg %p\n", hal, cfg);

    ctx->int_cb = cfg->hal_int_cb;

#ifdef RKPLATFORM
    ctx->vpu_fd = VPUClientInit(VPU_ENC);
    if (ctx->vpu_fd < 0) {
        mpp_err_f("failed to open vpu client\n");
        return MPP_NOK;
    }
#endif
    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);

    memset(ctx->regs, 0, sizeof(ctx->regs));

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_deinit(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx->bits) {
        jpege_bits_deinit(ctx->bits);
        ctx->bits = NULL;
    }

#ifdef RKPLATFORM
    if (ctx->vpu_fd >= 0) {
        VPUClientRelease(ctx->vpu_fd);
        ctx->vpu_fd = -1;
    }
#endif

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_gen_regs(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    HalEncTask *info = &task->enc;
    MppBuffer input  = info->input;
    MppBuffer output = info->output;
    JpegeSyntax *syntax = (JpegeSyntax *)info->syntax.data;
    RK_U32 width        = syntax->width;
    RK_U32 height       = syntax->height;
    MppFrameFormat fmt  = syntax->format;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    JpegeBits bits      = ctx->bits;
    RK_U32 *regs = ctx->regs;
    RK_U8  *buf = mpp_buffer_get_ptr(output);
    size_t size = mpp_buffer_get_size(output);
    const RK_U8 *qtable[2];
    RK_U32 val32;
    RK_S32 bitpos;
    RK_S32 bytepos;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    /* write header to output buffer */
    jpege_bits_setup(bits, buf, (RK_U32)size);
    /* NOTE: write header will update qtable */
    write_jpeg_header(bits, syntax, qtable);

    // input address setup
    regs[48] = mpp_buffer_get_fd(input);
    regs[49] = mpp_buffer_get_fd(input) + ((hor_stride * height) << 10);
    regs[50] = regs[49];

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
               ((hor_stride - width) << 4) |
                (ver_stride - height);
    regs[61] = hor_stride;

    switch (fmt) {
    case MPP_FMT_YUV420P : {
        val32 = 0;
    } break;
    case MPP_FMT_YUV420SP : {
        val32 = 1;
    } break;
    case MPP_FMT_YUV422_YUYV : {
        val32 = 2;
    } break;
    case MPP_FMT_YUV422_UYVY : {
        val32 = 3;
    } break;
    case MPP_FMT_RGB565 : {
        val32 = 4;
    } break;
    case MPP_FMT_RGB444 : {
        val32 = 5;
    } break;
    case MPP_FMT_RGB888 : {
        val32 = 6;
    } break;
    case MPP_FMT_RGB101010 : {
        val32 = 7;
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
    regs[98] = 0;

    regs[103] = (hor_stride >> 4) << 8  |
                (ver_stride >> 4) << 20 |
                (1 << 6) |  /* intra coding  */
                (2 << 4) |  /* format jpeg   */
                1;          /* encoder start */

    /* input byte swap configure */
    regs[105] = 7 << 26;
    if (val32 < 4) {
        // YUV format
        regs[105] |= (7 << 29);
    } else if (val32 < 7) {
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
            regs[i] = qtable[0][i * 4 + 0] << 24 |
                      qtable[0][i * 4 + 1] << 16 |
                      qtable[0][i * 4 + 2] << 8 |
                      qtable[0][i * 4 + 3];
        }
        for (i = 0; i < 16; i++) {
            regs[i + 16] = qtable[1][i * 4 + 0] << 24 |
                           qtable[1][i * 4 + 1] << 16 |
                           qtable[1][i * 4 + 2] << 8 |
                           qtable[1][i * 4 + 3];
        }
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    RK_U32 *regs = ctx->regs;
    (void)task;

    hal_jpege_dbg_func("enter hal %p\n", hal);

#ifdef RKPLATFORM
    if (ctx->vpu_fd >= 0)
        ret = VPUClientSendReg(ctx->vpu_fd, regs, VPU2_REG_NUM);
#endif

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeBits bits = ctx->bits;
    RK_U32 *regs = ctx->regs;
    JpegeFeedback feedback;
    RK_U32 val;
    (void)task;

    hal_jpege_dbg_func("enter hal %p\n", hal);

#ifdef RKPLATFORM
    if (ctx->vpu_fd >= 0) {
        VPU_CMD_TYPE cmd;
        RK_S32 len;
        ret = VPUClientWaitResult(ctx->vpu_fd, regs, VPU2_REG_NUM, &cmd, &len);
    }
#endif
    val = regs[109];
    hal_jpege_dbg_output("hw_status %x\n", val);
    feedback.hw_status = val & 0x70;
    val = regs[53];
    feedback.stream_length = jpege_bits_get_bitpos(bits) / 8 + val / 8;
    hal_jpege_dbg_output("stream length: sw %d hw %d total %d\n",
            jpege_bits_get_bitpos(bits) / 8, val / 8, feedback.stream_length);

    ctx->int_cb.callBack(ctx->int_cb.opaque, &feedback);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_reset(void *hal)
{
    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_flush(void *hal)
{
    hal_jpege_dbg_func("enter hal %p\n", hal);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_control(void *hal, RK_S32 cmd_type, void *param)
{
    hal_jpege_dbg_func("enter hal %p cmd %x param %p\n", hal, cmd_type, param);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

const MppHalApi hal_api_jpege = {
    "jpege_vpu",
    MPP_CTX_ENC,
    MPP_VIDEO_CodingMJPEG,
    sizeof(HalJpegeCtx),
    0,
    hal_jpege_init,
    hal_jpege_deinit,
    hal_jpege_gen_regs,
    hal_jpege_start,
    hal_jpege_wait,
    hal_jpege_reset,
    hal_jpege_flush,
    hal_jpege_control,
};

