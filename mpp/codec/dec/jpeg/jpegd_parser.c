/*
 *
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

#define MODULE_TAG "jpegd_parser"

#include "mpp_bitread.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_packet_impl.h"

#include "jpegd_api.h"
#include "jpegd_parser.h"
#include "mpp_platform.h"

RK_U32 jpegd_debug = 0x0;

/* return the 8 bit start code value and update the search
   state. Return -1 if no start code found */
static RK_S32 jpegd_find_marker(const RK_U8 **pbuf_ptr, const RK_U8 *buf_end)
{
    const RK_U8 *buf_ptr;
    unsigned int v, v2;
    int val;
    int skipped = 0;

    buf_ptr = *pbuf_ptr;
    while (buf_end - buf_ptr > 1) {
        v  = *buf_ptr++;
        v2 = *buf_ptr;
        if ((v == 0xff) && (v2 >= 0xc0) && (v2 <= 0xfe) && buf_ptr < buf_end) {
            val = *buf_ptr++;
            goto found;
        } else if ((v == 0x89) && (v2 == 0x50)) {
            mpp_log("input img maybe png format,check it\n");
        }
        skipped++;
    }
    buf_ptr = buf_end;
    val = -1;

found:
    jpegd_dbg_marker("find_marker skipped %d bytes\n", skipped);
    *pbuf_ptr = buf_ptr;
    return val;
}

static
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

static MPP_RET jpeg_judge_yuv_mode(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    JpegdSyntax *s = ctx->syntax;

    /*  check input format */
    if (s->nb_components == 3) {
        if (s->h_count[0] == 2 && s->v_count[0] == 2 &&
            s->h_count[1] == 1 && s->v_count[1] == 1 &&
            s->h_count[2] == 1 && s->v_count[2] == 1) {
            jpegd_dbg_marker("YCbCr Format: YUV420(2*2:1*1:1*1)\n");
            s->yuv_mode = JPEGDEC_YUV420;
            s->output_fmt = MPP_FMT_YUV420SP;
        } else if (s->h_count[0] == 2 && s->v_count[0] == 1 &&
                   s->h_count[1] == 1 && s->v_count[1] == 1 &&
                   s->h_count[2] == 1 && s->v_count[2] == 1) {
            jpegd_dbg_marker("YCbCr Format: YUV422(2*1:1*1:1*1)\n");
            s->yuv_mode = JPEGDEC_YUV422;
            s->output_fmt = MPP_FMT_YUV422SP;

            /* check if fill needed */
            if ((s->height & 0xf) && ((s->height & 0xf) <= 8)) {
                s->fill_bottom = 1;
            }
        } else if (s->h_count[0] == 1 && s->v_count[0] == 2 &&
                   s->h_count[1] == 1 && s->v_count[1] == 1 &&
                   s->h_count[2] == 1 && s->v_count[2] == 1) {
            jpegd_dbg_marker("YCbCr Format: YUV440(1*2:1*1:1*1)\n");
            s->yuv_mode = JPEGDEC_YUV440;
            s->output_fmt = MPP_FMT_YUV440SP;

            /* check if fill needed */
            if ((s->width & 0xf) && ((s->width & 0xf) <= 8)) {
                s->fill_right = 1;
            }
        } else if (s->h_count[0] == 1 && s->v_count[0] == 1 &&
                   s->h_count[1] == 1 && s->v_count[1] == 1 &&
                   s->h_count[2] == 1 && s->v_count[2] == 1) {
            jpegd_dbg_marker("YCbCr Format: YUV444(1*1:1*1:1*1)\n");
            s->yuv_mode = JPEGDEC_YUV444;
            s->output_fmt = MPP_FMT_YUV444SP;

            /* check if fill needed */
            if ((s->width & 0xf) && ((s->width & 0xf) <= 8)) {
                s->fill_right = 1;
            }

            if ((s->height & 0xf) && ((s->height & 0xf) <= 8)) {
                s->fill_bottom = 1;
            }
        } else if (s->h_count[0] == 4 && s->v_count[0] == 1 &&
                   s->h_count[1] == 1 && s->v_count[1] == 1 &&
                   s->h_count[2] == 1 && s->v_count[2] == 1) {
            jpegd_dbg_marker("YCbCr Format: YUV411(4*1:1*1:1*1)\n");
            s->yuv_mode = JPEGDEC_YUV411;
            s->output_fmt = MPP_FMT_YUV411SP;

            /* check if fill needed */
            if ((s->height & 0xf) && ((s->height & 0xf) <= 8)) {
                s->fill_bottom = 1;
            }
        } else {
            mpp_err_f("Unsupported YCbCr Format: (%d*%d:%d*%d:%d*%d)\n",
                      s->h_count[0], s->v_count[0],
                      s->h_count[1], s->v_count[1],
                      s->h_count[2], s->v_count[2]);
            ret = MPP_ERR_STREAM;
        }
    } else if (s->nb_components == 1) {
        if ((s->h_count[0] == 1) || (s->v_count[0] == 1)) {
            s->yuv_mode = JPEGDEC_YUV400;
            s->output_fmt = MPP_FMT_YUV400;

            /* check if fill needed */
            if ((s->width & 0xf) && ((s->width & 0xf) <= 8)) {
                s->fill_right = 1;
            }

            if ((s->height & 0xf) && ((s->height & 0xf) <= 8)) {
                s->fill_bottom = 1;
            }
        } else {
            mpp_err_f("unsupported format(%d*%d)\n", s->h_count[0],
                      s->v_count[0]);
            ret = MPP_ERR_STREAM;
        }
    } else {
        mpp_err_f("unsupported format, nb_components=%d\n", s->nb_components);
        ret = MPP_ERR_STREAM;
    }

    return ret;
}


static MPP_RET jpegd_decode_app(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *syntax = ctx->syntax;
    RK_U32 len, id;

    READ_BITS(gb, 16, &len);
    if (len < 6 || len > gb->bytes_left_) {
        /* too short length or bytes is not enough */
        return MPP_ERR_READ_BIT;
    }

    READ_BITS_LONG(gb, 32, &id);
    len -= 6;

    if (id == JPEG_IDENTIFIER('J', 'F', 'I', 'F')) {
        int t_w, t_h, v1, v2;
        SKIP_BITS(gb, 8); /* the trailing zero-byte */

        /* version */
        READ_BITS(gb, 8, &v1);
        READ_BITS(gb, 8, &v2);

        SKIP_BITS(gb, 8);

        READ_BITS(gb, 16, &syntax->hor_density);
        READ_BITS(gb, 16, &syntax->ver_density);
        if (syntax->hor_density <= 0
            || syntax->ver_density <= 0) {
            syntax->hor_density = 0;
            syntax->ver_density = 1;
        }

        jpegd_dbg_marker("mjpeg: JFIF header found (version: %x.%x) SAR=%d/%d\n",
                         v1, v2, syntax->hor_density, syntax->ver_density);

        len -= 8;
        if (len >= 2) {
            READ_BITS(gb, 8, &t_w);
            READ_BITS(gb, 8, &t_h);
            if (t_w && t_h) {
                /* skip thumbnail */
                if (len - 10 - (t_w * t_h * 3) > 0)
                    len -= t_w * t_h * 3;
            }
            len -= 2;
        }

        ret = MPP_OK;
    }

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error, but it does not matter!\n");

    return ret;
}

static MPP_RET jpegd_decode_dht(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *syntax = ctx->syntax;
    RK_U32 len, num, value;
    RK_U32 table_type, table_id;
    RK_U32  i, code_max;

    READ_BITS(gb, 16, &len);
    len -= 2; /* Huffman Table Length */

    if (len > gb->bytes_left_) {
        mpp_err_f("dht: len %d is too large\n", len);
        return MPP_ERR_STREAM;
    }
    jpegd_dbg_marker("dht: huffman tables length=%d\n", len);

    while (len > 0) {
        if (len < MAX_HUFFMAN_CODE_BIT_LENGTH + 1) {
            mpp_err_f("dht: len %d is too small\n", len);
            return MPP_ERR_STREAM;
        }

        READ_BITS(gb, 4, &table_type); /* 0 - DC; 1 - AC */
        if (table_type >= HUFFMAN_TABLE_TYPE_BUTT) {
            mpp_err_f("table type %d error\n", table_type);
            return MPP_ERR_STREAM;
        }

        READ_BITS(gb, 4, &table_id);
        if (table_id >= HUFFMAN_TABLE_ID_TWO) {
            mpp_err_f("table id %d is unsupported for baseline\n", table_id);
            return MPP_ERR_STREAM;
        }

        num = 0;
        if (table_type == HUFFMAN_TABLE_TYPE_DC) {
            DcTable *ptr = &(syntax->dc_table[table_id]);
            for (i = 0; i < MAX_HUFFMAN_CODE_BIT_LENGTH; i++) {
                READ_BITS(gb, 8, &value);
                ptr->bits[i] = value;
                num += value;
            }

            ptr->actual_length = num;
        } else {
            AcTable *ptr = &(syntax->ac_table[table_id]);
            for (i = 0; i < MAX_HUFFMAN_CODE_BIT_LENGTH; i++) {
                READ_BITS(gb, 8, &value);
                ptr->bits[i] = value;
                num += value;
            }

            ptr->actual_length = num;
        }

        len -= 17;
        if (len < num ||
            (num > MAX_DC_HUFFMAN_TABLE_LENGTH && table_type == HUFFMAN_TABLE_TYPE_DC) ||
            (num > MAX_AC_HUFFMAN_TABLE_LENGTH && table_type == HUFFMAN_TABLE_TYPE_AC)) {
            mpp_err_f("table type %d, code word number %d error\n", table_type, num);
            return MPP_ERR_STREAM;
        }

        code_max = 0;
        if (table_type == HUFFMAN_TABLE_TYPE_DC) {
            DcTable *ptr = &(syntax->dc_table[table_id]);

            for (i = 0; i < num; i++) {
                READ_BITS(gb, 8, &value);
                ptr->vals[i] = value;
                if (ptr->vals[i] > code_max)
                    code_max = ptr->vals[i];
            }
        } else {
            AcTable *ptr = &(syntax->ac_table[table_id]);

            for (i = 0; i < num; i++) {
                READ_BITS(gb, 8, &value);
                ptr->vals[i] = value;
                if (ptr->vals[i] > code_max)
                    code_max = ptr->vals[i];
            }
        }
        len -= num;

        jpegd_dbg_marker("dht: type=%d id=%d code_word_num=%d, code_max=%d, len=%d\n",
                         table_type, table_id, num, code_max, len);
    }
    ret = MPP_OK;

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error!\n");

    return ret;
}

/* quantize tables */
static MPP_RET jpegd_decode_dqt(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *syntax = ctx->syntax;
    RK_U32 len;
    int index, i;
    RK_U16 value;

    READ_BITS(gb, 16, &len);
    len -= 2; /* quantize tables length */

    if (len > gb->bytes_left_) {
        mpp_err_f("dqt: len %d is too large\n", len);
        return MPP_ERR_STREAM;
    }

    while (len >= 65) {
        RK_U16 pr;
        READ_BITS(gb, 4, &pr);
        if (pr > 1) {
            mpp_err_f("dqt: invalid precision\n");
            return MPP_ERR_STREAM;
        }

        READ_BITS(gb, 4, &index);
        if (index >= 4) {
            mpp_err_f("dqt: invalid quantize tables ID\n");
            return -1;
        }
        jpegd_dbg_marker("quantize tables ID=%d\n", index);

        /* read quant table */
        for (i = 0; i < QUANTIZE_TABLE_LENGTH; i++) {
            READ_BITS(gb, pr ? 16 : 8, &value);
            syntax->quant_matrixes[index][i] = value;
        }

        /* debug code */
        jpegd_dbg_table("\n******Start to print quantize table %d******",
                        index);
        if (jpegd_debug & JPEGD_DBG_TABLE) {
            for (i = 0; i < QUANTIZE_TABLE_LENGTH; i++) {
                if (i % 8 == 0)
                    printf("\n");
                printf("0x%02x,", syntax->quant_matrixes[index][i]);
            }
        }
        jpegd_dbg_table("\n******Quantize table %d End******\n\n", index);

        // XXX FIXME fine-tune, and perhaps add dc too
        syntax->qscale[index] = MPP_MAX(syntax->quant_matrixes[index][1],
                                        syntax->quant_matrixes[index][8]) >> 1;

        jpegd_dbg_marker("qscale[%d]: %d\n", index, syntax->qscale[index]);
        len -= 1 + 64 * (1 + pr);
    }
    ret = MPP_OK;

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error!\n");

    return ret;
}

static MPP_RET jpegd_decode_com(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    RK_U32 len;

    READ_BITS(gb, 16, &len);
    if (len >= 2 && len - 2 <= gb->bytes_left_) {
        RK_U32 i;
        RK_U8 *cbuf = mpp_calloc_size(RK_U8, len - 1);
        if (!cbuf) {
            mpp_err_f("allocate buffer failed\n");
            return MPP_ERR_NOMEM;
        }

        for (i = 0; i < len - 2; i++)
            READ_BITS(gb, 8, &cbuf[i]);

        if (i > 0 && cbuf[i - 1] == '\n')
            cbuf[i - 1] = 0;
        else
            cbuf[i] = 0;

        jpegd_dbg_marker("comment: '%s'\n", cbuf);

        mpp_free(cbuf);
        ret = MPP_OK;
    }

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error!\n");

    return ret;
}

static MPP_RET jpegd_decode_sof(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *syntax = ctx->syntax;
    RK_U32 len, bits, i;
    RK_U32 width, height;
    RK_U32 nb_components, value;

    READ_BITS(gb, 16, &len);
    if (len > gb->bytes_left_) {
        mpp_err_f("len %d is too large\n", len);
        return MPP_ERR_STREAM;
    }

    READ_BITS(gb, 8, &bits);
    if (bits > 16 || bits < 1) {
        /* usually bits is 8 */
        mpp_err_f("bits %d is invalid\n", bits);
        return MPP_ERR_STREAM;
    }

    READ_BITS(gb, 16, &height);
    READ_BITS(gb, 16, &width);
    syntax->height = height;
    syntax->width = width;
    syntax->hor_stride = MPP_ALIGN(width, 16);
    syntax->ver_stride = MPP_ALIGN(height, 16);

    jpegd_dbg_marker("sof0: picture: %dx%d, stride: %dx%d\n", width, height,
                     syntax->hor_stride, syntax->ver_stride);
    if (jpeg_image_check_size(syntax->hor_stride, syntax->ver_stride)) {
        mpp_err_f("check size failed\n");
        return MPP_ERR_STREAM;
    }

    READ_BITS(gb, 8 , &nb_components);
    if ((nb_components != 1) && (nb_components != MAX_COMPONENTS)) {
        mpp_err_f("components number %d error\n", nb_components);
        return MPP_ERR_STREAM;
    }

    if (len != (8 + (3 * nb_components)))
        mpp_err_f("decode_sof0: error, len(%d) mismatch nb_components(%d)\n",
                  len, nb_components);

    syntax->nb_components = nb_components;
    syntax->h_max = 1;
    syntax->v_max = 1;
    for (i = 0; i < nb_components; i++) {
        /* component id */
        READ_BITS(gb, 8 , &value);
        syntax->component_id[i] = value - 1; /* start from zero */

        READ_BITS(gb, 4, &value);
        syntax->h_count[i] = value;  /* Horizontal sampling factor */

        READ_BITS(gb, 4, &value);
        syntax->v_count[i] = value; /* Vertical sampling factor */

        if (!syntax->h_count[i] || !syntax->v_count[i]) {
            mpp_err_f("Invalid sampling factor in component %d %d:%d\n",
                      i, syntax->h_count[i], syntax->v_count[i]);
            return MPP_ERR_STREAM;
        }

        /* compute hmax and vmax (only used in interleaved case) */
        if (syntax->h_count[i] > syntax->h_max)
            syntax->h_max = syntax->h_count[i];
        if (syntax->v_count[i] > syntax->v_max)
            syntax->v_max = syntax->v_count[i];

        /* Quantization table destination selector */
        READ_BITS(gb, 8, &value);
        syntax->quant_index[i] = value;

        if (syntax->quant_index[i] >= QUANTIZE_TABLE_ID_BUTT) {
            mpp_err_f("quant_index %d is invalid\n", syntax->quant_index[i]);
            return MPP_ERR_STREAM;
        }

        jpegd_dbg_marker("component %d %d:%d id: %d quant_id:%d\n",
                         i, syntax->h_count[i], syntax->v_count[i],
                         syntax->component_id[i], syntax->quant_index[i]);
    }

    /* judge yuv mode from sampling factor */
    ret = jpeg_judge_yuv_mode(ctx);

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error!\n");

    return ret;
}

static MPP_RET jpegd_decode_sos(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *syntax = ctx->syntax;
    RK_U32 len, nb_components, value;
    RK_U32 id, i, index;

    READ_BITS(gb, 16, &len);
    if (len > gb->bytes_left_) {
        mpp_err_f("len %d is too large\n", len);
        return MPP_ERR_STREAM;
    }
    syntax->sos_len = len; /* used for calculating stream offset */

    READ_BITS(gb, 8, &nb_components);
    if ((nb_components != 1) && (nb_components != MAX_COMPONENTS)) {
        mpp_err_f("decode_sos: nb_components %d unsupported\n", nb_components);
        return MPP_ERR_STREAM;
    }

    if (len != 6 + 2 * nb_components) {
        mpp_err_f("decode_sos: invalid len (%d), nb_components:%d\n",
                  len, nb_components);
        return MPP_ERR_STREAM;
    }
    syntax->qtable_cnt = nb_components;

    for (i = 0; i < nb_components; i++) {
        READ_BITS(gb, 8, &value);
        id = value - 1;
        jpegd_dbg_marker("sos component: %d\n", id);

        /* find component index */
        for (index = 0; index < syntax->nb_components; index++)
            if (id == syntax->component_id[index])
                break;

        if (index == syntax->nb_components) {
            mpp_err_f("decode_sos: index(%d) out of components\n", index);
            return MPP_ERR_STREAM;
        }

        READ_BITS(gb, 4, &value);
        syntax->dc_index[i] = value;

        READ_BITS(gb, 4, &value);
        syntax->ac_index[i] = value;

        jpegd_dbg_marker("component:%d, dc_index:%d, ac_index:%d\n",
                         id, syntax->dc_index[i], syntax->ac_index[i]);

        if (syntax->dc_index[i] > HUFFMAN_TABLE_ID_ONE ||
            syntax->ac_index[i] > HUFFMAN_TABLE_ID_ONE) {
            /* for baseline */
            mpp_err_f("Huffman table id error\n");
            return MPP_ERR_STREAM;
        }
    }

    READ_BITS(gb, 8, &value);
    syntax->scan_start = value;

    READ_BITS(gb, 8, &value);
    syntax->scan_end = value;

    READ_BITS(gb, 4, &value);
    syntax->prev_shift = value;

    READ_BITS(gb, 4, &value);
    syntax->point_transform = value;

    if (syntax->scan_start != 0 || syntax->scan_end != 0x3F ||
        syntax->prev_shift != 0 || syntax->point_transform != 0) {
        /* for baseline */
        mpp_err_f("unsupported sos parameter: scan_start:%d,\n"
                  "\t\tscan_end:%d, prev_shift:%d, point_transform:%d\n",
                  syntax->scan_start, syntax->scan_end,
                  syntax->prev_shift, syntax->point_transform);
        return MPP_ERR_STREAM;
    }
    ret = MPP_OK;

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error!\n");

    return ret;
}

static MPP_RET jpegd_decode_dri(JpegdCtx *ctx)
{
    MPP_RET ret = MPP_NOK;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *s = ctx->syntax;
    RK_U32 len;

    READ_BITS(gb, 16, &len);
    if (len != DRI_MARKER_LENGTH) {
        mpp_err_f("DRI length %d error\n", len);
        return MPP_ERR_STREAM;
    }

    READ_BITS(gb, 16, &s->restart_interval);
    jpegd_dbg_marker("restart interval: %d\n", s->restart_interval);

    ret = MPP_OK;

__BITREAD_ERR:
    if (ret != MPP_OK)
        jpegd_dbg_syntax("bit read error!\n");

    return ret;
}

static MPP_RET jpegd_setup_default_dht(JpegdCtx *ctx)
{
    jpegd_dbg_func("enter\n");
    JpegdSyntax *s = ctx->syntax;
    AcTable *ac_ptr = NULL;
    DcTable *dc_ptr = NULL;
    RK_U8 *bits_tmp = NULL;
    RK_U8 *val_tmp = NULL;
    RK_U32 tmp_len = 0;
    RK_U32 i,  k;

    /* Set up the standard Huffman tables (cf. JPEG standard section K.3)
     * IMPORTANT: these are only valid for 8-bit data precision!
     */
    static RK_U8 bits_dc_luminance[MAX_HUFFMAN_CODE_BIT_LENGTH] =
    {  0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };

    /* luminance and chrominance all use it */
    static RK_U8 val_dc[MAX_DC_HUFFMAN_TABLE_LENGTH] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    static RK_U8 bits_dc_chrominance[MAX_HUFFMAN_CODE_BIT_LENGTH] =
    { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };

    static RK_U8 bits_ac_luminance[MAX_HUFFMAN_CODE_BIT_LENGTH] =
    { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };

    /* 162 Bytes */
    static RK_U8 val_ac_luminance[MAX_AC_HUFFMAN_TABLE_LENGTH] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    };

    static RK_U8 bits_ac_chrominance[MAX_HUFFMAN_CODE_BIT_LENGTH] =
    { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };

    static RK_U8 val_ac_chrominance[MAX_AC_HUFFMAN_TABLE_LENGTH] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    };

    RK_U8 *bits_table[4] = {
        bits_ac_luminance,
        bits_ac_chrominance,
        bits_dc_luminance,
        bits_dc_chrominance
    };

    RK_U8 *val_table[4] = {
        val_ac_luminance,
        val_ac_chrominance,
        val_dc,
        val_dc
    };

    /* AC Table */
    for (k = 0; k < 2; k++) {
        ac_ptr = &(s->ac_table[k]);
        bits_tmp = bits_table[k];
        val_tmp = val_table[k];

        tmp_len = 0;
        for (i = 0; i < MAX_HUFFMAN_CODE_BIT_LENGTH; i++) {
            /* read in the values of list BITS */
            tmp_len += ac_ptr->bits[i] = bits_tmp[i];
        }

        ac_ptr->actual_length = tmp_len;  /* set the table length */
        for (i = 0; i < tmp_len; i++) {
            /* read in the HUFFVALs */
            ac_ptr->vals[i] = val_tmp[i];
        }
    }

    /* DC Table */
    for (k = 0; k < 2; k++) {
        dc_ptr = &(s->dc_table[k]);
        bits_tmp = bits_table[k + 2];
        val_tmp = val_table[k + 2];

        tmp_len = 0;
        for (i = 0; i < MAX_HUFFMAN_CODE_BIT_LENGTH; i++) {
            /* read in the values of list BITS */
            tmp_len += dc_ptr->bits[i] = bits_tmp[i];
        }

        dc_ptr->actual_length = tmp_len;  /* set the table length */
        for (i = 0; i < tmp_len; i++) {
            /* read in the HUFFVALs */
            dc_ptr->vals[i] = val_tmp[i];
        }
    }

    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

static MPP_RET jpegd_decode_frame(JpegdCtx *ctx)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    const RK_U8 *buf = ctx->buffer;
    RK_U32 buf_size = ctx->buf_size;
    const RK_U8 *buf_end, *buf_ptr;
    BitReadCtx_t *gb = ctx->bit_ctx;
    JpegdSyntax *syntax = ctx->syntax;
    RK_S32 start_code;

    buf_ptr = buf;
    buf_end = buf + buf_size;

    while (buf_ptr < buf_end) {
        /* find start marker */
        start_code = jpegd_find_marker(&buf_ptr, buf_end);
        if (start_code < 0) {
            jpegd_dbg_marker("start code not found\n");
            break;
        }

        jpegd_dbg_marker("marker = 0x%x, avail_size_in_buf = %d\n",
                         start_code, buf_end - buf_ptr);
        ctx->start_code = start_code;

        /* setup bit read context */
        mpp_set_bitread_ctx(gb, (RK_U8 *)buf_ptr, buf_end - buf_ptr);

        /* process markers */
        if (start_code >= RST0 && start_code <= RST7) {
            /* nothing to do with RSTn */
            jpegd_dbg_syntax("restart marker: %d\n", start_code & 0x0f);
        } else if (start_code >= APP0 && start_code <= APP15) {
            /* APP fields */
            jpegd_decode_app(ctx);
        }

        switch (start_code) {
        case SOI:
            /* nothing to do on SOI */
            syntax->dht_found = 0;
            syntax->eoi_found = 0;
            break;
        case DHT:
            if ((ret = jpegd_decode_dht(ctx)) != MPP_OK) {
                mpp_err_f("huffman table decode error\n");
                goto fail;
            }
            syntax->dht_found = 1;
            break;
        case DQT:
            if ((ret = jpegd_decode_dqt(ctx)) != MPP_OK) {
                mpp_err_f("quantize tables decode error\n");
                goto fail;
            }
            break;
        case COM:
            if ((ret = jpegd_decode_com(ctx)) != MPP_OK) {
                mpp_err_f("comment decode error\n");
            }
            break;
        case SOF0:
            if ((ret = jpegd_decode_sof(ctx)) != MPP_OK) {
                mpp_err_f("sof0 decode error\n");
                goto fail;
            }
            break;
        case EOI:
            syntax->eoi_found = 1;
            jpegd_dbg_marker("still exists %d bytes behind EOI marker\n",
                             buf_end - buf_ptr);
            goto done;
            break;
        case SOS:
            if ((ret = jpegd_decode_sos(ctx)) != MPP_OK) {
                mpp_err_f("sos decode error\n");
                goto fail;
            }

            /* stream behind SOS is decoded by hardware */
            syntax->strm_offset = buf_ptr - buf + syntax->sos_len;
            syntax->cur_pos = (RK_U8 *)buf + syntax->strm_offset;
            syntax->pkt_len = ctx->buf_size;
            jpegd_dbg_marker("This packet owns %d bytes,\n"
                             "\t\thas been decoded %d bytes by software\n"
                             "\t\tbuf_ptr:%p, buf:%p, sos_len:%d\n"
                             "\t\thardware start address:%p",
                             syntax->pkt_len,
                             syntax->strm_offset, buf_ptr, buf,
                             syntax->sos_len, syntax->cur_pos);

            if (syntax->strm_offset >= ctx->buf_size) {
                mpp_err_f("stream offset %d is larger than buffer size %d\n",
                          syntax->strm_offset, ctx->buf_size);
                ret = MPP_ERR_UNKNOW;
                goto fail;
            }

            if (!ctx->scan_all_marker) {
                jpegd_dbg_marker("just scan parts of markers!\n");
                goto done;
            }

            break;
        case DRI:
            if ((ret = jpegd_decode_dri(ctx)) != MPP_OK) {
                mpp_err_f("dri decode error\n");
                goto fail;
            }
            break;
        case SOF2:
        case SOF3:
        case SOF5:
        case SOF6:
        case SOF7:
        case SOF9:
        case SOF10:
        case SOF11:
        case SOF13:
        case SOF14:
        case SOF15:
        case SOF48:
        case LSE:
        case JPG:
            mpp_log("jpeg: unsupported coding type (0x%x)\n", start_code);
            break;
        default:
            jpegd_dbg_marker("unhandled coding type(0x%x) in switch..case..\n",
                             start_code);
            break;
        }
    }

    if (!syntax->eoi_found) {
        mpp_err_f("EOI marker not found!\n");
    }

done:
    if (!syntax->dht_found) {
        jpegd_dbg_marker("sorry, DHT is not found!\n");
        jpegd_setup_default_dht(ctx);
    }

fail:
    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET
jpegd_split_frame(RK_U8 *src, RK_U32 src_size,
                  RK_U8 *dst, RK_U32 *dst_size)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    if (NULL == src || NULL == dst || src_size <= 0) {
        mpp_err_f("NULL pointer or wrong src_size(%d)", src_size);
        return MPP_ERR_NULL_PTR;
    }
    RK_U8 *end;
    RK_U8 *tmp;
    RK_U32 str_size = (src_size + 255) & (~255);
    end = dst + src_size;

    if (src[6] == 0x41 && src[7] == 0x56 && src[8] == 0x49 && src[9] == 0x31) {
        //distinguish 310 from 210 camera
        RK_U32     i;
        RK_U32 copy_len = 0;
        tmp = src;
        jpegd_dbg_parser("distinguish 310 from 210 camera");

        for (i = 0; i < src_size - 4; i++) {
            if (tmp[i] == 0xff) {
                if (tmp[i + 1] == 0x00 && tmp[i + 2] == 0xff && ((tmp[i + 3] & 0xf0) == 0xd0))
                    i += 2;
            }
            *dst++ = tmp[i];
            copy_len++;
        }
        for (; i < src_size; i++) {
            *dst++ = tmp[i];
            copy_len++;
        }
        if (copy_len < src_size)
            memset(dst, 0, src_size - copy_len);
        *dst_size = copy_len;
    } else {
        memcpy(dst, src, src_size);
        memset(dst + src_size, 0, str_size - src_size);
        *dst_size = src_size;
    }

    /* NOTE: hardware bug, need to remove tailing FF 00 before FF D9 end flag */
    if (end[-1] == 0xD9 && end[-2] == 0xFF) {
        end -= 2;

        do {
            if (end[-1] == 0xFF) {
                end--;
                continue;
            }
            if (end[-1] == 0x00 && end [-2] == 0xFF) {
                jpegd_dbg_parser("remove tailing FF 00 before FF D9 end flag.");
                end -= 2;
                continue;
            }
            break;
        } while (1);


        end[0] = 0xff;
        end[1] = 0xD9;
    }

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET
jpegd_handle_stream(RK_U8 *src, RK_U32 src_size,
                    RK_U8 *dst, RK_U32 *dst_size)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    if (NULL == src || NULL == dst || src_size <= 0) {
        mpp_err_f("NULL pointer or wrong src_size(%d)", src_size);
        return MPP_ERR_NULL_PTR;
    }
    RK_U8 *end;
    *dst_size = 0;  /* no need to copy */
    end = src + src_size;

    /* NOTE: hardware bug, need to remove tailing FF 00 before FF D9 end flag */
    if (end[-1] == 0xD9 && end[-2] == 0xFF) {
        end -= 2;

        do {
            if (end[-1] == 0xFF) {
                end--;
                continue;
            }
            if (end[-1] == 0x00 && end [-2] == 0xFF) {
                jpegd_dbg_parser("remove tailing FF 00 before FF D9 end flag.");
                end -= 2;
                continue;
            }
            break;
        } while (1);

        end[0] = 0xff;
        end[1] = 0xD9;
    }

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET jpegd_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;
    if (!JpegCtx->copy_flag) {
        /* no need to copy stream, handle packet from upper application directly*/
        JpegCtx->input_packet = pkt;
    }

    MppPacket input_packet = JpegCtx->input_packet;
    RK_U32 copy_length = 0;
    void *base = mpp_packet_get_pos(pkt);
    RK_U8 *pos = base;
    RK_U32 pkt_length = (RK_U32)mpp_packet_get_length(pkt);
    RK_U32 eos = (pkt_length) ? (mpp_packet_get_eos(pkt)) : (1);

    JpegCtx->pts = mpp_packet_get_pts(pkt);

    task->valid = 0;
    task->flags.eos = eos;
    JpegCtx->eos = eos;

    jpegd_dbg_parser("pkt_length %d eos %d\n", pkt_length, eos);

    if (!pkt_length) {
        jpegd_dbg_parser("it is end of stream.");
        return ret;
    }

    if (pkt_length > JpegCtx->bufferSize) {
        jpegd_dbg_parser("Huge Frame(%d Bytes)! bufferSize:%d",
                         pkt_length, JpegCtx->bufferSize);
        mpp_free(JpegCtx->recv_buffer);
        JpegCtx->recv_buffer = NULL;

        JpegCtx->recv_buffer = mpp_calloc(RK_U8, pkt_length + 1024);
        if (NULL == JpegCtx->recv_buffer) {
            mpp_err_f("no memory!");
            return MPP_ERR_NOMEM;
        }

        JpegCtx->bufferSize = pkt_length + 1024;
    }

    if (JpegCtx->copy_flag)
        jpegd_split_frame(base, pkt_length, JpegCtx->recv_buffer, &copy_length);
    else
        jpegd_handle_stream(base, pkt_length, JpegCtx->recv_buffer, &copy_length);

    pos += pkt_length;
    mpp_packet_set_pos(pkt, pos);
    if (copy_length != pkt_length) {
        jpegd_dbg_parser("packet prepare, pkt_length:%d, copy_length:%d\n",
                         pkt_length, copy_length);
    }

    /* debug information */
    if (jpegd_debug & JPEGD_DBG_IO) {
        static FILE *jpg_file;
        static char name[32];

        snprintf(name, sizeof(name), "/data/input%02d.jpg",
                 JpegCtx->input_jpeg_count);
        jpg_file = fopen(name, "wb+");
        if (jpg_file) {
            jpegd_dbg_io("frame_%02d input jpeg(%d Bytes) saving to %s\n",
                         JpegCtx->input_jpeg_count, pkt_length, name);
            fwrite(base, pkt_length, 1, jpg_file);
            fclose(jpg_file);
            JpegCtx->input_jpeg_count++;
        }
    }

    if (JpegCtx->copy_flag) {
        mpp_packet_set_data(input_packet, JpegCtx->recv_buffer);
        mpp_packet_set_size(input_packet, pkt_length);
        mpp_packet_set_length(input_packet, pkt_length);
    }

    JpegCtx->streamLength = pkt_length;
    task->input_packet = input_packet;
    task->valid = 1;
    jpegd_dbg_parser("input_packet:%p, recv_buffer:%p, pkt_length:%d",
                     input_packet,
                     JpegCtx->recv_buffer, pkt_length);

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET jpegd_allocate_frame(JpegdCtx *ctx)
{
    jpegd_dbg_func("enter\n");
    JpegdSyntax *s = ctx->syntax;
    MppBufSlots slots = ctx->frame_slots;
    MppFrame output = ctx->output_frame;
    RK_S32 slot_idx = ctx->frame_slot_index;

    if (slot_idx == -1) {
        RK_U32 value;
        MppFrameFormat fmt = MPP_FMT_YUV420SP;

        switch (s->yuv_mode) {
        case JPEGDEC_YUV420: {
            fmt = MPP_FMT_YUV420SP;
        } break;
        case JPEGDEC_YUV422: {
            fmt = MPP_FMT_YUV422SP;
        } break;
        case JPEGDEC_YUV444: {
            fmt = MPP_FMT_YUV444SP;
        } break;
        case JPEGDEC_YUV400: {
            fmt = MPP_FMT_YUV400;
        } break;
        default : {
            fmt = MPP_FMT_YUV420SP;
        } break;
        }

        mpp_frame_set_fmt(output, fmt);
        mpp_frame_set_width(output, s->width);
        mpp_frame_set_height(output, s->height);
        mpp_frame_set_hor_stride(output, s->hor_stride);
        mpp_frame_set_ver_stride(output, s->ver_stride);
        mpp_frame_set_pts(output, ctx->pts);

        if (ctx->eos)
            mpp_frame_set_eos(output, 1);

        mpp_buf_slot_get_unused(slots, &slot_idx);
        ctx->frame_slot_index = slot_idx;
        jpegd_dbg_parser("frame_slot_index:%d\n", slot_idx);

        value = 2;
        mpp_slots_set_prop(slots, SLOTS_NUMERATOR, &value);
        value = 1;
        mpp_slots_set_prop(slots, SLOTS_DENOMINATOR, &value);
        if (mpp_buf_slot_set_prop(slots, slot_idx, SLOT_FRAME, output))
            return MPP_ERR_VALUE;
        mpp_buf_slot_set_flag(slots, slot_idx, SLOT_CODEC_USE);
        mpp_buf_slot_set_flag(slots, slot_idx, SLOT_HAL_OUTPUT);
    }

    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

static MPP_RET jpegd_update_frame(JpegdCtx *ctx)
{
    jpegd_dbg_func("enter\n");

    mpp_buf_slot_clr_flag(ctx->frame_slots, ctx->frame_slot_index,
                          SLOT_CODEC_USE);
    ctx->frame_slot_index = -1;

    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

static MPP_RET jpegd_parse(void *ctx, HalDecTask *task)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;
    task->valid = 0;

    JpegCtx->buffer = (RK_U8 *)mpp_packet_get_data(JpegCtx->input_packet);
    JpegCtx->buf_size = (RK_U32)mpp_packet_get_size(JpegCtx->input_packet);

    ret = jpegd_decode_frame(JpegCtx);
    if (MPP_OK == ret) {
        if (jpegd_allocate_frame(JpegCtx))
            return MPP_ERR_VALUE;

        task->syntax.data = (void *)JpegCtx->syntax;
        task->syntax.number = sizeof(JpegdSyntax);
        task->output = JpegCtx->frame_slot_index;
        task->valid = 1;

        jpegd_update_frame(JpegCtx);
    }

    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET jpegd_deinit(void *ctx)
{
    jpegd_dbg_func("enter\n");
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;

    if (JpegCtx->recv_buffer) {
        mpp_free(JpegCtx->recv_buffer);
        JpegCtx->recv_buffer = NULL;
    }

    if (JpegCtx->output_frame) {
        mpp_free(JpegCtx->output_frame);
    }

    if (JpegCtx->copy_flag) {
        if (JpegCtx->input_packet) {
            mpp_packet_deinit(&JpegCtx->input_packet);
        }
    } else {
        JpegCtx->input_packet = NULL;
    }

    if (JpegCtx->bit_ctx) {
        mpp_free(JpegCtx->bit_ctx);
        JpegCtx->bit_ctx = NULL;
    }

    if (JpegCtx->syntax) {
        mpp_free(JpegCtx->syntax);
        JpegCtx->syntax = NULL;
    }

    JpegCtx->output_fmt = MPP_FMT_YUV420SP;
    JpegCtx->pts = 0;
    JpegCtx->eos = 0;
    JpegCtx->input_jpeg_count = 0;

    jpegd_dbg_func("exit\n");
    return 0;
}

static MPP_RET jpegd_init(void *ctx, ParserCfg *parser_cfg)
{
    jpegd_dbg_func("enter\n");
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;
    if (NULL == JpegCtx) {
        JpegCtx = (JpegdCtx *)mpp_calloc(JpegdCtx, 1);
        if (NULL == JpegCtx) {
            mpp_err_f("NULL pointer");
            return MPP_ERR_NULL_PTR;
        }
    }
    mpp_env_get_u32("jpegd_debug", &jpegd_debug, 0);

    const char* soc_name = NULL;
    soc_name = mpp_get_soc_name();
    if (soc_name && strstr(soc_name, "1108")) {
        /* rv1108: no need to copy stream when decoding jpeg;
         *         just scan parts of markers to reduce CPU's occupancy
         */
        JpegCtx->copy_flag = 0;
        JpegCtx->scan_all_marker = 0;
    } else {
        JpegCtx->copy_flag = 1;
        JpegCtx->scan_all_marker = 1;
    }

    JpegCtx->frame_slots = parser_cfg->frame_slots;
    JpegCtx->packet_slots = parser_cfg->packet_slots;
    JpegCtx->frame_slot_index = -1;
    mpp_buf_slot_setup(JpegCtx->frame_slots, 1);

    JpegCtx->recv_buffer = mpp_calloc(RK_U8, JPEGD_STREAM_BUFF_SIZE);
    if (NULL == JpegCtx->recv_buffer) {
        mpp_err_f("no memory!");
        return MPP_ERR_NOMEM;
    }
    JpegCtx->bufferSize = JPEGD_STREAM_BUFF_SIZE;
    if (JpegCtx->copy_flag) {
        mpp_packet_init(&JpegCtx->input_packet,
                        JpegCtx->recv_buffer, JPEGD_STREAM_BUFF_SIZE);
    } else {
        JpegCtx->input_packet = NULL;
    }

    mpp_frame_init(&JpegCtx->output_frame);
    if (!JpegCtx->output_frame) {
        mpp_err_f("Failed to allocate output frame buffer");
        return MPP_ERR_NOMEM;
    }

    JpegCtx->bit_ctx = mpp_calloc(BitReadCtx_t, 1);
    if (JpegCtx->bit_ctx == NULL) {
        mpp_err_f("allocate bit_ctx failed\n");
        return MPP_ERR_MALLOC;
    }

    JpegCtx->syntax = mpp_calloc(JpegdSyntax, 1);
    if (JpegCtx->syntax == NULL) {
        mpp_err_f("allocate syntax failed\n");
        return MPP_ERR_MALLOC;
    }
    memset(JpegCtx->syntax, 0, sizeof(JpegdSyntax));

    JpegCtx->output_fmt = MPP_FMT_YUV420SP;
    JpegCtx->pts = 0;
    JpegCtx->eos = 0;
    JpegCtx->input_jpeg_count = 0;

    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

static MPP_RET jpegd_flush(void *ctx)
{
    jpegd_dbg_func("enter\n");
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;
    (void)JpegCtx;
    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

static MPP_RET jpegd_reset(void *ctx)
{
    jpegd_dbg_func("enter\n");
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;

    (void)JpegCtx;

    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

static MPP_RET jpegd_control(void *ctx, RK_S32 cmd, void *param)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdCtx *JpegCtx = (JpegdCtx *)ctx;
    if (NULL == JpegCtx) {
        mpp_err_f("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd) {
    case MPP_DEC_SET_OUTPUT_FORMAT: {
        JpegCtx->output_fmt = *((RK_U32 *)param);
        jpegd_dbg_parser("output_format:%d\n", JpegCtx->output_fmt);
    } break;
    default :
        ret = MPP_NOK;
    }
    jpegd_dbg_func("exit\n");
    return ret;
}

static MPP_RET jpegd_callback(void *ctx, void *err_info)
{
    jpegd_dbg_func("enter\n");
    (void) ctx;
    (void) err_info;
    jpegd_dbg_func("exit\n");
    return MPP_OK;
}

const ParserApi api_jpegd_parser = {
    .name = "jpegd_parse",
    .coding = MPP_VIDEO_CodingMJPEG,
    .ctx_size = sizeof(JpegdCtx),
    .flag = 0,
    .init = jpegd_init,
    .deinit = jpegd_deinit,
    .prepare = jpegd_prepare,
    .parse = jpegd_parse,
    .reset = jpegd_reset,
    .flush = jpegd_flush,
    .control = jpegd_control,
    .callback = jpegd_callback,
};


