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

#include "mpp_buffer.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_dec.h"
#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_bitput.h"

#include "jpegd_syntax.h"
#include "jpegd_api.h"
#include "hal_jpegd_common.h"

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
            if (end[-1] == 0x00 && end [-2] == 0xFF) {
                end -= 2;
                length -= 2;
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

void jpegd_setup_output_fmt(JpegdHalCtx *ctx, JpegdSyntax *s, RK_S32 output)
{
    jpegd_dbg_func("enter\n");
    RK_U32 pp_in_fmt = 0;
    PPInfo *pp_info = &(ctx->pp_info);
    MppFrame frm = NULL;

    if (ctx->set_output_fmt_flag && (ctx->output_fmt != s->output_fmt)) {
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
        pp_info->pp_out_fmt = PP_OUT_FORMAT_YUV420INTERLAVE;

        jpegd_dbg_hal("Post Process! pp_in_fmt:%d, pp_out_fmt:%d",
                      pp_in_fmt, pp_info->pp_out_fmt);
    } else {
        /* keep original output format */
        ctx->output_fmt = s->output_fmt;
        pp_info->pp_enable = 0;
    }

    mpp_buf_slot_get_prop(ctx->frame_slots, output,
                          SLOT_FRAME_PTR, &frm);
    mpp_frame_set_fmt(frm, ctx->output_fmt);

    jpegd_dbg_func("exit\n");
    return;
}

