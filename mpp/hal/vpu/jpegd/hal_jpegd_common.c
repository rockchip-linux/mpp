/*
 *
 * Copyright 2017 Rockchip Electronics Co. LTD
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
#define MODULE_TAG "HAL_JPEGD_COMMON"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bitput.h"

#include "hal_task.h"
#include "jpegd_syntax.h"
#include "jpegd_api.h"
#include "hal_jpegd_common.h"

static PpRgbCfg pp_rgb_cfgs[PP_RGB_CFG_LENTH] = {
    //ffmpeg: rgb565be, bin(rrrr,rggg, gggb,bbbb) mem MSB-->LSB(gggb,bbbb, rrrr,rggg)
    {
        .fmt = MPP_FMT_RGB565, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 1, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 0, .g_padd = 5, .b_padd = 11,
        .r_dither = 2, .g_dither = 3, .b_dither = 2,
        .r_mask = 0xf800f800, .g_mask = 0x07e007e0, .b_mask = 0x001f001f
    },
    //ffmpeg: bgr565be, bin(bbbb,bggg, gggr,rrrr) mem MSB-->LSB(gggr,rrrr, bbbb,bggg)
    {
        .fmt = MPP_FMT_BGR565, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 1, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 11, .g_padd = 5, .b_padd = 0,
        .r_dither = 2, .g_dither = 3, .b_dither = 2,
        .r_mask = 0x001f001f, .g_mask = 0x07e007e0, .b_mask = 0xf800f800
    },
    //ffmpeg: rgb555be, bin(0rrr,rrgg, gggb,bbbb) mem MSB-->LSB(gggb,bbbb, 0rrr,rrgg)
    {
        .fmt = MPP_FMT_RGB555, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 1, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 1, .g_padd = 6, .b_padd = 11,
        .r_dither = 2, .g_dither = 2, .b_dither = 2,
        .r_mask = 0x7c007c00, .g_mask = 0x03e003e0, .b_mask = 0x001f001f
    },
    //ffmpeg: bgr555be, bin(0bbb,bbgg, gggr,rrrr) mem MSB-->LSB(gggr,rrrr, 0bbb,bbgg)
    {
        .fmt = MPP_FMT_BGR555, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 1, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 11, .g_padd = 6, .b_padd = 1,
        .r_dither = 2, .g_dither = 2, .b_dither = 2,
        .r_mask = 0x001f001f, .g_mask = 0x03e003e0, .b_mask = 0x7c007c00
    },
    //ffmpeg: rgb444be, bin(0000,rrrr, gggg,bbbb) mem MSB-->LSB(gggg,bbbb, 0000,rrrr)
    {
        .fmt = MPP_FMT_RGB444, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 0, .swap_16 = 1, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 12, .g_padd = 0, .b_padd = 4,
        .r_dither = 1, .g_dither = 1, .b_dither = 1,
        .r_mask = 0x000f000f, .g_mask = 0xf000f000, .b_mask = 0x0f000f00
    },
    //ffmpeg: bgr444be, bin(0000,bbbb, gggg,rrrr) mem MSB-->LSB(gggg,rrrr, 0000,bbbb)
    {
        .fmt = MPP_FMT_BGR444, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 0, .swap_16 = 1, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 4, .g_padd = 0, .b_padd = 12, .r_mask = 0x0f000f00,
        .r_dither = 1, .g_dither = 1, .b_dither = 1,
        .g_mask = 0xf000f000, .b_mask = 0x000f000f
    },
    //ffmpeg: argb, bin(aaaa,aaaa, rrrr,rrrr, gggg,gggg, bbbb,bbbb)
    {
        .fmt = MPP_FMT_ARGB8888, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 16, .g_padd = 8, .b_padd = 0,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0x0000ff00 | 0xff,
        .g_mask = 0x00ff0000 | 0xff,
        .b_mask = 0xff000000 | 0xff
    },
    //ffmepg: rgba, bin(aaaa,aaaa, bbbb,bbbb, gggg,gggg, rrrr,rrrr)
    {
        .fmt = MPP_FMT_ABGR8888, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 0, .g_padd = 8, .b_padd = 16,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0xff000000 | 0xff,
        .g_mask = 0x00ff0000 | 0xff,
        .b_mask = 0x0000ff00 | 0xff
    },
    //ffmpeg: bgra, bin(bbbb,bbbb, gggg,gggg, rrrr,rrrr, aaaa,aaaa)
    {
        .fmt = MPP_FMT_BGRA8888, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 8, .g_padd = 16, .b_padd = 24,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0x00ff0000 | (0xff << 24),
        .g_mask = 0x0000ff00 | (0xff << 24),
        .b_mask = 0x000000ff | (0xff << 24)
    },
    //ffmpeg: rgba, bin(rrrr,rrrr, gggg,gggg, bbbb,bbbb, aaaa,aaaa)
    {
        .fmt = MPP_FMT_RGBA8888, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 24, .g_padd = 16, .b_padd = 8,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0x000000ff | (0xff << 24),
        .g_mask = 0x0000ff00 | (0xff << 24),
        .b_mask = 0x00ff0000 | (0xff << 24)
    },
};

static PpRgbCfg pp_rgb_le_cfgs[PP_RGB_CFG_LENTH] = {
    //ffmpeg: rgb565le, bin(gggb,bbbb, rrrr,rggg) mem MSB-->LSB(rrrr,rggg, gggb,bbbb)
    {
        .fmt = MPP_FMT_RGB565 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 0, .swap_16 = 1, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 0, .g_padd = 5, .b_padd = 11,
        .r_dither = 2, .g_dither = 3, .b_dither = 2,
        .r_mask = 0xf800f800, .g_mask = 0x07e007e0, .b_mask = 0x001f001f
    },
    //ffmpeg: bgr565le, bin(gggr,rrrr, bbbb,bggg) mem MSB-->LSB(bbbb,bggg, gggr,rrrr)
    {
        .fmt = MPP_FMT_BGR565 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 0, .swap_16 = 1, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 11, .g_padd = 5, .b_padd = 0,
        .r_dither = 2, .g_dither = 3, .b_dither = 2,
        .r_mask = 0x001f001f, .g_mask = 0x07e007e0, .b_mask = 0xf800f800
    },
    //ffmpeg: rgb555le, bin(gggb,bbbb, 0rrr,rrgg) mem MSB-->LSB(0rrr,rrgg, gggb,bbbb)
    {
        .fmt = MPP_FMT_RGB555 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 0, .swap_16 = 1, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 1, .g_padd = 6, .b_padd = 11,
        .r_dither = 2, .g_dither = 2, .b_dither = 2,
        .r_mask = 0x7c007c00, .g_mask = 0x03e003e0, .b_mask = 0x001f001f
    },
    //ffmpeg: bgr555le, bin(gggr,rrrr, 0bbb,bbgg) mem MSB-->LSB(0bbb,bbgg, gggr,rrrr)
    {
        .fmt = MPP_FMT_BGR555 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 0, .swap_16 = 1, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 11, .g_padd = 6, .b_padd = 1,
        .r_dither = 2, .g_dither = 2, .b_dither = 2,
        .r_mask = 0x001f001f, .g_mask = 0x03e003e0, .b_mask = 0x7c007c00
    },
    //ffmpeg: rgb444le, bin(gggg,bbbb, 0000,rrrr) mem MSB-->LSB(0000,rrrr, gggg,bbbb)
    {
        .fmt = MPP_FMT_RGB444 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 1, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 12, .g_padd = 0, .b_padd = 4,
        .r_dither = 1, .g_dither = 1, .b_dither = 1,
        .r_mask = 0x000f000f, .g_mask = 0xf000f000, .b_mask = 0x0f000f00
    },
    //ffmpeg: bgr444le, bin(gggg,rrrr, 0000,bbbb) mem MSB-->LSB(0000,bbbb, gggg,rrrr)
    {
        .fmt = MPP_FMT_BGR444 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_RGB565,
        .out_endian = 1, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 1,
        .r_padd = 4, .g_padd = 0, .b_padd = 12, .r_mask = 0x0f000f00,
        .r_dither = 1, .g_dither = 1, .b_dither = 1,
        .g_mask = 0xf000f000, .b_mask = 0x000f000f
    },
    //in memory: [31:0] A:R:G:B 8:8:8:8 little endian
    {
        .fmt = MPP_FMT_ARGB8888 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 8, .g_padd = 16, .b_padd = 24,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0x00ff0000 | (0xff << 24),
        .g_mask = 0x0000ff00 | (0xff << 24),
        .b_mask = 0x000000ff | (0xff << 24)
    },
    //in memory: [31:0] A:B:G:R 8:8:8:8 little endian
    {
        .fmt = MPP_FMT_ABGR8888 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 24, .g_padd = 16, .b_padd = 8,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0x000000ff | (0xff << 24),
        .g_mask = 0x0000ff00 | (0xff << 24),
        .b_mask = 0x00ff0000 | (0xff << 24)
    },
    //in memory: [31:0] B:G:R:A 8:8:8:8 little endian
    {
        .fmt = MPP_FMT_BGRA8888 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 16, .g_padd = 8, .b_padd = 0,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0x0000ff00 | 0xff,
        .g_mask = 0x00ff0000 | 0xff,
        .b_mask = 0xff000000 | 0xff
    },
    //in memory: [31:0] R:G:B:A 8:8:8:8 little endian
    {
        .fmt = MPP_FMT_RGBA8888 | MPP_FRAME_FMT_LE_MASK, .pp_out_fmt = PP_OUT_FORMAT_ARGB,
        .out_endian = 0, .swap_16 = 0, .swap_32 = 1, .rgb_in_32 = 0,
        .r_padd = 0, .g_padd = 8, .b_padd = 16,
        .r_dither = 0, .g_dither = 0, .b_dither = 0,
        .r_mask = 0xff000000 | 0xff,
        .g_mask = 0x00ff0000 | 0xff,
        .b_mask = 0x0000ff00 | 0xff
    },
};

PpRgbCfg* get_pp_rgb_Cfg(MppFrameFormat fmt)
{
    PpRgbCfg* cfg = NULL;
    PpRgbCfg* cfg_array = NULL;
    RK_U8 i = 0;

    if (MPP_FRAME_FMT_IS_LE(fmt))
        cfg_array = pp_rgb_le_cfgs;
    else
        cfg_array = pp_rgb_cfgs;

    for (i = 0; i < PP_RGB_CFG_LENTH; i++) {
        if (cfg_array[i].fmt ==  fmt) {
            cfg = &cfg_array[i];
            break;
        }
    }

    return cfg;
}

RK_U32 jpegd_vdpu_tail_0xFF_patch(MppBuffer stream, RK_U32 length)
{
    RK_U8 *p = mpp_buffer_get_ptr(stream);
    RK_U8 *end = p + length;

    if (end[-1] == 0xD9 && end[-2] == 0xFF) {
        end -= 2;

        do {
            if (end[-1] == 0xFF) {
                end--;
                length--;
                continue;
            }
            break;
        } while (1);

        end[0] = 0xff;
        end[1] = 0xD9;
    }

    return length;
}

void jpegd_write_qp_ac_dc_table(JpegdHalCtx *ctx,
                                JpegdSyntax*syntax)
{
    jpegd_dbg_func("enter\n");
    JpegdSyntax *s = syntax;
    RK_U32 *base = (RK_U32 *)mpp_buffer_get_ptr(ctx->pTableBase);
    RK_U8 table_tmp[QUANTIZE_TABLE_LENGTH] = {0};
    RK_U32 idx, table_word = 0, table_value = 0;
    RK_U32 shifter = 32;
    AcTable *ac_ptr0 = NULL, *ac_ptr1 = NULL;
    DcTable *dc_ptr0 = NULL, *dc_ptr1 = NULL;
    RK_U32 i, j = 0;

    /* Quantize tables for all components
     * length = 64 * 3  (Bytes)
     */
    for (j = 0; j < s->qtable_cnt; j++) {
        idx = s->quant_index[j]; /* quantize table index used by j component */

        for (i = 0; i < QUANTIZE_TABLE_LENGTH; i++) {
            table_tmp[zzOrder[i]] = (RK_U8) s->quant_matrixes[idx][i];
        }

        /* could memcpy be OK?? */
        for (i = 0; i < QUANTIZE_TABLE_LENGTH; i += 4) {
            /* transfer to big endian */
            table_word = (table_tmp[i] << 24) |
                         (table_tmp[i + 1] << 16) |
                         (table_tmp[i + 2] << 8) |
                         table_tmp[i + 3];
            *base = table_word;
            base++;
        }
    }

    /* write AC and DC tables
     * memory:  AC(Y) - AC(UV) - DC(Y) - DC(UV)
     * length = 162   + 162    + 12    + 12   (Bytes)
     */
    {
        /* this trick is done because hardware always wants
         * luma table as ac hardware table 0 */
        if (s->ac_index[0] == HUFFMAN_TABLE_ID_ZERO) {
            /* Luma's AC uses Huffman table zero */
            ac_ptr0 = &(s->ac_table[HUFFMAN_TABLE_ID_ZERO]);
            ac_ptr1 = &(s->ac_table[HUFFMAN_TABLE_ID_ONE]);
        } else {
            ac_ptr0 = &(s->ac_table[HUFFMAN_TABLE_ID_ONE]);
            ac_ptr1 = &(s->ac_table[HUFFMAN_TABLE_ID_ZERO]);
        }

        /* write luma AC table */
        for (i = 0; i < MAX_AC_HUFFMAN_TABLE_LENGTH; i++) {
            if (i < ac_ptr0->actual_length)
                table_value = (RK_U8) ac_ptr0->vals[i];
            else
                table_value = 0;

            /* transfer to big endian */
            if (shifter == 32)
                table_word = (table_value << (shifter - 8));
            else
                table_word |= (table_value << (shifter - 8));

            shifter -= 8;
            if (shifter == 0) {
                /* write 4 Bytes(32 bit) */
                *base = table_word;
                base++;
                shifter = 32;
            }
        }

        /* write chroma AC table */
        for (i = 0; i < MAX_AC_HUFFMAN_TABLE_LENGTH; i++) {
            /* chroma's AC table must be zero for YUV400 */
            if ((s->yuv_mode != JPEGDEC_YUV400) && (i < ac_ptr1->actual_length))
                table_value = (RK_U8) ac_ptr1->vals[i];
            else
                table_value = 0;

            /* transfer to big endian */
            if (shifter == 32)
                table_word = (table_value << (shifter - 8));
            else
                table_word |= (table_value << (shifter - 8));

            shifter -= 8;
            if (shifter == 0) {
                /* write 4 Bytes(32 bit) */
                *base = table_word;
                base++;
                shifter = 32;
            }
        }

        /* this trick is done because hardware always wants
         * luma table as dc hardware table 0 */
        if (s->dc_index[0] == HUFFMAN_TABLE_ID_ZERO) {
            /* Luma's DC uses Huffman table zero */
            dc_ptr0 = &(s->dc_table[HUFFMAN_TABLE_ID_ZERO]);
            dc_ptr1 = &(s->dc_table[HUFFMAN_TABLE_ID_ONE]);
        } else {
            dc_ptr0 = &(s->dc_table[HUFFMAN_TABLE_ID_ONE]);
            dc_ptr1 = &(s->dc_table[HUFFMAN_TABLE_ID_ZERO]);
        }

        /* write luma DC table */
        for (i = 0; i < MAX_DC_HUFFMAN_TABLE_LENGTH; i++) {
            if (i < dc_ptr0->actual_length)
                table_value = (RK_U8) dc_ptr0->vals[i];
            else
                table_value = 0;

            /* transfer to big endian */
            if (shifter == 32)
                table_word = (table_value << (shifter - 8));
            else
                table_word |= (table_value << (shifter - 8));

            shifter -= 8;
            if (shifter == 0) {
                /* write 4 Bytes(32 bit) */
                *base = table_word;
                base++;
                shifter = 32;
            }
        }

        /* write chroma DC table */
        for (i = 0; i < MAX_DC_HUFFMAN_TABLE_LENGTH; i++) {
            /* chroma's DC table must be zero for YUV400 */
            if ((s->yuv_mode != JPEGDEC_YUV400) && (i < dc_ptr1->actual_length))
                table_value = (RK_U8) dc_ptr1->vals[i];
            else
                table_value = 0;

            /* transfer to big endian */
            if (shifter == 32)
                table_word = (table_value << (shifter - 8));
            else
                table_word |= (table_value << (shifter - 8));

            shifter -= 8;
            if (shifter == 0) {
                /* write 4 Bytes(32 bit) */
                *base = table_word;
                base++;
                shifter = 32;
            }
        }
    }

    /* four-byte padding zero */
    for (i = 0; i < 4; i++) {
        table_value = 0;

        if (shifter == 32)
            table_word = (table_value << (shifter - 8));
        else
            table_word |= (table_value << (shifter - 8));

        shifter -= 8;

        if (shifter == 0) {
            *base = table_word;
            base++;
            shifter = 32;
        }
    }
    jpegd_dbg_func("exit\n");
    return;
}

void jpegd_check_have_pp(JpegdHalCtx *ctx)
{
    ctx->codec_type = mpp_get_vcodec_type();
    ctx->have_pp = ((ctx->dev_type == VPU_CLIENT_VDPU1) &&
                    (ctx->codec_type & (1 << VPU_CLIENT_VDPU1_PP))) ||
                   ((ctx->dev_type == VPU_CLIENT_VDPU2) &&
                    (ctx->codec_type & (1 << VPU_CLIENT_VDPU2_PP)));
}

MPP_RET jpegd_setup_output_fmt(JpegdHalCtx *ctx, JpegdSyntax *s, RK_S32 output)
{
    jpegd_dbg_func("enter\n");
    RK_U32 pp_in_fmt = 0;
    RK_U32 stride = 0;
    PPInfo *pp_info = &ctx->pp_info;
    MppClientType dev_type = ctx->dev_type;
    MppFrame frm = NULL;
    MPP_RET ret = MPP_OK;

    if (ctx->have_pp && ctx->set_output_fmt_flag &&
        ctx->output_fmt != s->output_fmt) {
        MppFrameFormat fmt = MPP_FMT_BUTT;

        /* Using pp to convert all format to yuv420sp */
        switch (s->output_fmt) {
        case MPP_FMT_YUV400:
            pp_in_fmt = PP_IN_FORMAT_YUV400;
            break;
        case MPP_FMT_YUV420SP:
            pp_in_fmt = PP_IN_FORMAT_YUV420SEMI;
            break;
        case MPP_FMT_YUV422SP:
            pp_in_fmt = PP_IN_FORMAT_YUV422SEMI;
            break;
        case MPP_FMT_YUV440SP:
            pp_in_fmt = PP_IN_FORMAT_YUV440SEMI;
            break;
        case MPP_FMT_YUV411SP:
            pp_in_fmt = PP_IN_FORMAT_YUV411_SEMI;
            break;
        case MPP_FMT_YUV444SP:
            pp_in_fmt = PP_IN_FORMAT_YUV444_SEMI;
            break;
        default:
            jpegd_dbg_hal("other output format %d\n", s->output_fmt);
            break;
        }

        pp_info->pp_enable = 1;
        pp_info->pp_in_fmt = pp_in_fmt;

        fmt = ctx->output_fmt;

        if (MPP_FRAME_FMT_IS_LE(fmt)) {
            fmt &= MPP_FRAME_FMT_MASK;
        }

        switch (fmt) {
        case MPP_FMT_RGB565 :
        case MPP_FMT_BGR565 :
        case MPP_FMT_RGB555 :
        case MPP_FMT_BGR555 :
        case MPP_FMT_RGB444 :
        case MPP_FMT_BGR444 : {
            pp_info->pp_out_fmt = PP_OUT_FORMAT_RGB565;
            stride = s->hor_stride * 2;
        } break;
        case MPP_FMT_ARGB8888 :
        case MPP_FMT_ABGR8888 :
        case MPP_FMT_RGBA8888 :
        case MPP_FMT_BGRA8888 : {
            pp_info->pp_out_fmt = PP_OUT_FORMAT_ARGB;
            stride = s->hor_stride * 4;
        } break;
        default : {
            pp_info->pp_out_fmt = PP_OUT_FORMAT_YUV420INTERLAVE;
        } break;
        }

        jpegd_dbg_hal("Post Process! pp_in_fmt:%d, pp_out_fmt:%d",
                      pp_in_fmt, pp_info->pp_out_fmt);

        /* check and switch to dev with pp */
        if (ctx->dev_type == VPU_CLIENT_VDPU1)
            dev_type = VPU_CLIENT_VDPU1_PP;
        else if (ctx->dev_type == VPU_CLIENT_VDPU2)
            dev_type = VPU_CLIENT_VDPU2_PP;
    } else {
        /* keep original output format */
        ctx->output_fmt = s->output_fmt;
        pp_info->pp_enable = 0;

        /* check and switch to dev without pp */
        if (ctx->dev_type == VPU_CLIENT_VDPU1_PP)
            dev_type = VPU_CLIENT_VDPU1;
        else if (ctx->dev_type == VPU_CLIENT_VDPU2_PP)
            dev_type = VPU_CLIENT_VDPU2;
    }

    mpp_assert(ctx->dev);
    if (ctx->dev_type != dev_type && ctx->dev) {
        MppDev dev = NULL;

        ret = mpp_dev_init(&dev, dev_type);
        if (ret) {
            mpp_err_f("dev type %x -> %x switch failed ret %d\n",
                      ctx->dev_type, dev_type, ret);
            return ret;
        }

        mpp_dev_deinit(ctx->dev);
        ctx->dev = dev;
        ctx->dev_type = dev_type;

        jpegd_dbg_hal("mpp_dev_init success.\n");
    }

    mpp_buf_slot_get_prop(ctx->frame_slots, output,
                          SLOT_FRAME_PTR, &frm);
    mpp_frame_set_fmt(frm, ctx->output_fmt);
    mpp_frame_set_hor_stride_pixel(frm, s->hor_stride);
    /* update RGBX format byte stride and pixel stride */
    if (stride)
        mpp_frame_set_hor_stride(frm, stride);

    jpegd_dbg_func("exit\n");
    return ret;
}

MPP_RET jpeg_image_check_size(RK_U32 hor_stride, RK_U32 ver_stride)
{
    MPP_RET ret = MPP_OK;

    if (hor_stride > MAX_WIDTH || ver_stride > MAX_HEIGHT ||
        hor_stride < MIN_WIDTH || ver_stride < MIN_HEIGHT ||
        hor_stride * ver_stride > MAX_STREAM_LENGTH) {
        mpp_err_f("unsupported resolution: %dx%d\n", hor_stride, ver_stride);
        ret = MPP_NOK;
    }

    return ret;
}
