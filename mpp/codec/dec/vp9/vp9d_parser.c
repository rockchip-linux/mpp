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
#include <stdlib.h>

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_bitread.h"
#include "mpp_packet_impl.h"

#include "vp9data.h"
#include "vp9d_codec.h"
#include "vp9d_parser.h"

/**
 * Clip a signed integer into the -(2^p),(2^p-1) range.
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
static   RK_U32 av_clip_uintp2(RK_S32 a, RK_S32 p)
{
    if (a & ~((1 << p) - 1)) return -a >> 31 & ((1 << p) - 1);
    else                   return  a;
}


RK_U32 vp9d_debug = 0;

#define VP9_SYNCCODE 0x498342
//#define dump
#ifdef dump
static FILE *vp9_p_fp = NULL;
static FILE *vp9_p_fp1 = NULL;
static FILE *vp9_p_fp2 = NULL;
static RK_S32 dec_num = 0;
static RK_S32 count = 0;
#endif

#ifndef FASTDIV
#   define FASTDIV(a,b) ((RK_U32)((((RK_U64)a) * vpx_inverse[b]) >> 32))
#endif /* FASTDIV */

/* a*inverse[b]>>32 == a/b for all 0<=a<=16909558 && 2<=b<=256
 * for a>16909558, is an overestimate by less than 1 part in 1<<24 */
const RK_U32 vpx_inverse[257] = {
    0, 4294967295U, 2147483648U, 1431655766, 1073741824,  858993460,  715827883,  613566757,
    536870912,  477218589,  429496730,  390451573,  357913942,  330382100,  306783379,  286331154,
    268435456,  252645136,  238609295,  226050911,  214748365,  204522253,  195225787,  186737709,
    178956971,  171798692,  165191050,  159072863,  153391690,  148102321,  143165577,  138547333,
    134217728,  130150525,  126322568,  122713352,  119304648,  116080198,  113025456,  110127367,
    107374183,  104755300,  102261127,   99882961,   97612894,   95443718,   93368855,   91382283,
    89478486,   87652394,   85899346,   84215046,   82595525,   81037119,   79536432,   78090315,
    76695845,   75350304,   74051161,   72796056,   71582789,   70409300,   69273667,   68174085,
    67108864,   66076420,   65075263,   64103990,   63161284,   62245903,   61356676,   60492498,
    59652324,   58835169,   58040099,   57266231,   56512728,   55778797,   55063684,   54366675,
    53687092,   53024288,   52377650,   51746594,   51130564,   50529028,   49941481,   49367441,
    48806447,   48258060,   47721859,   47197443,   46684428,   46182445,   45691142,   45210183,
    44739243,   44278014,   43826197,   43383509,   42949673,   42524429,   42107523,   41698712,
    41297763,   40904451,   40518560,   40139882,   39768216,   39403370,   39045158,   38693400,
    38347923,   38008561,   37675152,   37347542,   37025581,   36709123,   36398028,   36092163,
    35791395,   35495598,   35204650,   34918434,   34636834,   34359739,   34087043,   33818641,
    33554432,   33294321,   33038210,   32786010,   32537632,   32292988,   32051995,   31814573,
    31580642,   31350127,   31122952,   30899046,   30678338,   30460761,   30246249,   30034737,
    29826162,   29620465,   29417585,   29217465,   29020050,   28825284,   28633116,   28443493,
    28256364,   28071682,   27889399,   27709467,   27531842,   27356480,   27183338,   27012373,
    26843546,   26676816,   26512144,   26349493,   26188825,   26030105,   25873297,   25718368,
    25565282,   25414008,   25264514,   25116768,   24970741,   24826401,   24683721,   24542671,
    24403224,   24265352,   24129030,   23994231,   23860930,   23729102,   23598722,   23469767,
    23342214,   23216040,   23091223,   22967740,   22845571,   22724695,   22605092,   22486740,
    22369622,   22253717,   22139007,   22025474,   21913099,   21801865,   21691755,   21582751,
    21474837,   21367997,   21262215,   21157475,   21053762,   20951060,   20849356,   20748635,
    20648882,   20550083,   20452226,   20355296,   20259280,   20164166,   20069941,   19976593,
    19884108,   19792477,   19701685,   19611723,   19522579,   19434242,   19346700,   19259944,
    19173962,   19088744,   19004281,   18920561,   18837576,   18755316,   18673771,   18592933,
    18512791,   18433337,   18354562,   18276457,   18199014,   18122225,   18046082,   17970575,
    17895698,   17821442,   17747799,   17674763,   17602325,   17530479,   17459217,   17388532,
    17318417,   17248865,   17179870,   17111424,   17043522,   16976156,   16909321,   16843010,
    16777216
};

static const RK_U8 bwh_tab[2][N_BS_SIZES][2] = {
    {
        { 16, 16 }, { 16, 8 }, { 8, 16 }, { 8, 8 }, { 8, 4 }, { 4, 8 },
        { 4, 4 }, { 4, 2 }, { 2, 4 }, { 2, 2 }, { 2, 1 }, { 1, 2 }, { 1, 1 },
    }, {
        { 8, 8 }, { 8, 4 }, { 4, 8 }, { 4, 4 }, { 4, 2 }, { 2, 4 },
        { 2, 2 }, { 2, 1 }, { 1, 2 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 },
    }
};

static void split_parse_frame(SplitContext_t *ctx, RK_U8 *buf, RK_S32 size)
{
    VP9ParseContext *s = (VP9ParseContext *)ctx->priv_data;

    if (buf[0] & 0x4) {
        ctx->key_frame = 0;
    } else {
        ctx->key_frame = 1;
    }

    if (buf[0] & 0x2) {
        if (ctx->pts == -1)
            ctx->pts = s->pts;
        s->pts = -1;
    } else {
        s->pts = ctx->pts;
        ctx->pts = -1;
    }

    (void)size;
}

RK_S32 vp9d_split_frame(SplitContext_t *ctx,
                        RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 size)
{
    VP9ParseContext *s = (VP9ParseContext *)ctx->priv_data;
    RK_S32 full_size = size;
    RK_S32 marker;

    if (size <= 0) {
        *out_size = 0;
        *out_data = data;

        return 0;
    }

    if (s->n_frames > 0) {
        *out_data = data;
        *out_size = s->size[--s->n_frames];
        split_parse_frame(ctx, *out_data, *out_size);

        return s->n_frames > 0 ? *out_size : size /* i.e. include idx tail */;
    }

    marker = data[size - 1];
    if ((marker & 0xe0) == 0xc0) {
        RK_S32 nbytes = 1 + ((marker >> 3) & 0x3);
        RK_S32 n_frames = 1 + (marker & 0x7), idx_sz = 2 + n_frames * nbytes;

        if (size >= idx_sz && data[size - idx_sz] == marker) {
            RK_U8 *idx = data + size + 1 - idx_sz;
            RK_S32 first = 1;

            switch (nbytes) {
#define case_n(a, rd) \
            case a: \
                while (n_frames--) { \
                    RK_U32 sz = rd; \
                    idx += a; \
                    if (sz == 0 || sz > (RK_U32)size) { \
                        s->n_frames = 0; \
                        *out_size = size > full_size ? full_size : size; \
                        *out_data = data; \
                        mpp_err("Superframe packet size too big: %u > %d\n", \
                               sz, size); \
                        return full_size; \
                    } \
                    if (first) { \
                        first = 0; \
                        *out_data = data; \
                        *out_size = sz; \
                        s->n_frames = n_frames; \
                    } else { \
                        s->size[n_frames] = sz; \
                    } \
                    data += sz; \
                    size -= sz; \
                } \
                split_parse_frame(ctx, *out_data, *out_size); \
                return *out_size

                case_n(1, *idx);
                case_n(2, MPP_RL16(idx));
                case_n(3, MPP_RL24(idx));
                case_n(4, MPP_RL32(idx));
            }
        }
    }

    *out_data = data;
    *out_size = size;
    split_parse_frame(ctx, data, size);
    return size;
}

MPP_RET vp9d_get_frame_stream(Vp9CodecContext *ctx, RK_U8 *buf, RK_S32 length)
{
    RK_S32 buff_size = 0;
    RK_U8 *data = NULL;
    RK_S32 size = 0;

    data = (RK_U8 *)mpp_packet_get_data(ctx->pkt);
    size = (RK_S32)mpp_packet_get_size(ctx->pkt);

    if (length > size) {
        mpp_free(data);
        mpp_packet_deinit(&ctx->pkt);
        buff_size = length + 10 * 1024;
        data = mpp_malloc(RK_U8, buff_size);
        mpp_packet_init(&ctx->pkt, (void *)data, length);
        mpp_packet_set_size(ctx->pkt, buff_size);
    }

    memcpy(data, buf, length);
    mpp_packet_set_length(ctx->pkt, length);

    return MPP_OK;
}

MPP_RET vp9d_split_init(Vp9CodecContext *vp9_ctx)
{
    SplitContext_t *ps;
    VP9ParseContext *sc;

    ps = (SplitContext_t *)mpp_calloc(SplitContext_t, 1);
    if (!ps) {
        mpp_err("vp9 parser malloc fail");
        return MPP_ERR_NOMEM;
    }

    sc = (VP9ParseContext *)mpp_calloc(VP9ParseContext, 1);
    if (!sc) {
        mpp_err("vp9 parser context malloc fail");
        mpp_free(ps);
        return MPP_ERR_NOMEM;
    }

    ps->priv_data = (void*)sc;
    vp9_ctx->priv_data2 = (void*)ps;

    return MPP_OK;
}

MPP_RET vp9d_split_deinit(Vp9CodecContext *vp9_ctx)
{
    SplitContext_t *ps = (SplitContext_t *)vp9_ctx->priv_data2;

    if (ps)
        MPP_FREE(ps->priv_data);
    MPP_FREE(vp9_ctx->priv_data2);

    return MPP_OK;
}

static RK_S32 vp9_ref_frame(Vp9CodecContext *ctx, VP9Frame *dst, VP9Frame *src)
{
    VP9Context *s = ctx->priv_data;
    if (src->ref == NULL || src->slot_index >= 0x7f) {
        mpp_err("vp9_ref_frame is vaild");
        return -1;
    }
    dst->slot_index = src->slot_index;
    dst->ref = src->ref;
    dst->ref->invisible = src->ref->invisible;
    dst->ref->ref_count++;
    vp9d_dbg(VP9D_DBG_REF, "get prop slot frame %p  count %d", dst->f, dst->ref->ref_count);
    mpp_buf_slot_get_prop(s->slots, src->slot_index, SLOT_FRAME, &dst->f);

    vp9d_dbg(VP9D_DBG_REF, "get prop slot frame after %p", dst->f);
    return 0;
}

static void vp9_unref_frame( VP9Context *s, VP9Frame *f)
{
    if (f->ref->ref_count <= 0 || f->slot_index >= 0x7f) {
        mpp_err("ref count alreay is zero");
        return;
    }
    f->ref->ref_count--;
    if (!f->ref->ref_count) {
        if (f->slot_index <= 0x7f) {
            MppBuffer framebuf = NULL;
            if (f->ref->invisible) {
                mpp_buf_slot_get_prop(s->slots, f->slot_index, SLOT_BUFFER, &framebuf);
                mpp_buffer_put(framebuf);
                f->ref->invisible = 0;
            }
            mpp_buf_slot_clr_flag(s->slots, f->slot_index, SLOT_CODEC_USE);

        }
        mpp_free(f->ref);
        f->slot_index = 0xff;
        f->ref = NULL;
    }
    f->ref = NULL;
    return;
}


static  RK_S32 vp9_frame_free(VP9Context *s)
{
    RK_S32 i;
    for (i = 0; i < 3; i++) {
        if (s->frames[i].ref) {
            vp9_unref_frame(s, &s->frames[i]);
        }
        mpp_free(s->frames[i].f);
    }
    for (i = 0; i < 8; i++) {
        if (s->refs[i].ref) {
            vp9_unref_frame(s, &s->refs[i]);
        }
        mpp_free(s->refs[i].f);
    }
    return 0;
}

static RK_S32 vp9_frame_init(VP9Context *s)
{
    RK_S32 i;
    for (i = 0; i < 3; i++) {
        mpp_frame_init(&s->frames[i].f);
        if (!s->frames[i].f) {
            vp9_frame_free(s);
            mpp_err("Failed to allocate frame buffer %d\n", i);
            return MPP_ERR_NOMEM;
        }
        s->frames[i].slot_index = 0x7f;
        s->frames[i].ref = NULL;
    }

    for (i = 0; i < 8; i++) {
        mpp_frame_init(&(s->refs[i].f));
        if (!s->refs[i].f) {
            vp9_frame_free(s);
            mpp_err("Failed to allocate frame buffer %d\n", i);
            return MPP_ERR_NOMEM;
        }
        s->refs[i].slot_index = 0x7f;
        s->refs[i].ref = NULL;
    }
    return MPP_OK;
}

MPP_RET vp9d_parser_init(Vp9CodecContext *vp9_ctx, ParserCfg *init)
{
    VP9Context *s = mpp_calloc(VP9Context, 1);
    vp9_ctx->priv_data = (void*)s;
    if (!vp9_ctx->priv_data) {
        mpp_err("vp9 codec context malloc fail");
        return MPP_ERR_NOMEM;
    }
    vp9_frame_init(s);
    s->last_bpp = 0;
    s->filter.sharpness = -1;

#ifdef dump
    count = 0;
#endif

    s->packet_slots = init->packet_slots;
    s->slots = init->frame_slots;
    mpp_buf_slot_setup(s->slots, 25);

    mpp_env_get_u32("vp9d_debug", &vp9d_debug, 0);

    return MPP_OK;
}

MPP_RET vp9d_parser_deinit(Vp9CodecContext *vp9_ctx)
{
    VP9Context *s = vp9_ctx->priv_data;
    vp9_frame_free(s);
    mpp_free(s->c_b);
    s->c_b_size = 0;
    MPP_FREE(vp9_ctx->priv_data);
    return MPP_OK;
}



static RK_S32 vp9_alloc_frame(Vp9CodecContext *ctx, VP9Frame *frame)
{
    VP9Context *s = ctx->priv_data;
    mpp_frame_set_width(frame->f, ctx->width);
    mpp_frame_set_height(frame->f, ctx->height);

    mpp_frame_set_hor_stride(frame->f, ctx->width);
    mpp_frame_set_ver_stride(frame->f, ctx->height);
    mpp_frame_set_errinfo(frame->f, 0);
    mpp_frame_set_discard(frame->f, 0);
    mpp_frame_set_pts(frame->f, s->pts);
#if 0
    mpp_frame_set_fmt(frame->frame, s->h265dctx->pix_fmt);
    mpp_frame_set_errinfo(frame->f, 0);

#endif
    mpp_buf_slot_get_unused(s->slots, &frame->slot_index);
    mpp_buf_slot_set_prop(s->slots, frame->slot_index, SLOT_FRAME, frame->f);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_HAL_OUTPUT);
    frame->ref = mpp_calloc(RefInfo, 1);
    frame->ref->ref_count++;
    frame->ref->invisible = s->invisible;

    return 0;
}



// for some reason the sign bit is at the end, not the start, of a bit sequence
static RK_S32 get_sbits_inv(BitReadCtx_t *gb, RK_S32 n)
{
    RK_S32 value;
    RK_S32 v;
    READ_BITS(gb, n, &v);
    READ_ONEBIT(gb, &value);
    return value ? -v : v;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 update_size(Vp9CodecContext *ctx, RK_S32 w, RK_S32 h, RK_S32 fmt)
{
    VP9Context *s = ctx->priv_data;

    if (w == ctx->width && h == ctx->height && ctx->pix_fmt == fmt)
        return 0;

    ctx->width   = w;
    ctx->height  = h;
    ctx->pix_fmt = fmt;
    s->sb_cols   = (w + 63) >> 6;
    s->sb_rows   = (h + 63) >> 6;
    s->cols      = (w + 7) >> 3;
    s->rows      = (h + 7) >> 3;

    // these will be re-allocated a little later
    if (s->bpp != s->last_bpp) {
        s->last_bpp = s->bpp;
    }

    return 0;
}

static RK_S32 inv_recenter_nonneg(RK_S32 v, RK_S32 m)
{
    return v > 2 * m ? v : v & 1 ? m - ((v + 1) >> 1) : m + (v >> 1);
}

// differential forward probability updates
static RK_S32 update_prob(VpxRangeCoder *c, RK_S32 p)
{
    static const RK_S32 inv_map_table[255] = {
        7,  20,  33,  46,  59,  72,  85,  98, 111, 124, 137, 150, 163, 176,
        189, 202, 215, 228, 241, 254,   1,   2,   3,   4,   5,   6,   8,   9,
        10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  21,  22,  23,  24,
        25,  26,  27,  28,  29,  30,  31,  32,  34,  35,  36,  37,  38,  39,
        40,  41,  42,  43,  44,  45,  47,  48,  49,  50,  51,  52,  53,  54,
        55,  56,  57,  58,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
        70,  71,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
        86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  99, 100,
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 112, 113, 114, 115,
        116, 117, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 129, 130,
        131, 132, 133, 134, 135, 136, 138, 139, 140, 141, 142, 143, 144, 145,
        146, 147, 148, 149, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
        161, 162, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 203, 204, 205, 206,
        207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220, 221,
        222, 223, 224, 225, 226, 227, 229, 230, 231, 232, 233, 234, 235, 236,
        237, 238, 239, 240, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251,
        252, 253, 253,
    };
    RK_S32 d;

    /* This code is trying to do a differential probability update. For a
     * current probability A in the range [1, 255], the difference to a new
     * probability of any value can be expressed differentially as 1-A,255-A
     * where some part of this (absolute range) exists both in positive as
     * well as the negative part, whereas another part only exists in one
     * half. We're trying to code this shared part differentially, i.e.
     * times two where the value of the lowest bit specifies the sign, and
     * the single part is then coded on top of this. This absolute difference
     * then again has a value of [0,254], but a bigger value in this range
     * indicates that we're further away from the original value A, so we
     * can code this as a VLC code, since higher values are increasingly
     * unlikely. The first 20 values in inv_map_table[] allow 'cheap, rough'
     * updates vs. the 'fine, exact' updates further down the range, which
     * adds one extra dimension to this differential update model. */

    if (!vpx_rac_get(c)) {
        d = vpx_rac_get_uint(c, 4) + 0;
    } else if (!vpx_rac_get(c)) {
        d = vpx_rac_get_uint(c, 4) + 16;
    } else if (!vpx_rac_get(c)) {
        d = vpx_rac_get_uint(c, 5) + 32;
    } else {
        d = vpx_rac_get_uint(c, 7);
        if (d >= 65)
            d = (d << 1) - 65 + vpx_rac_get(c);
        d += 64;
        //av_assert2(d < FF_ARRAY_ELEMS(inv_map_table));
    }

    return p <= 128 ? 1 + inv_recenter_nonneg(inv_map_table[d], p - 1) :
           255 - inv_recenter_nonneg(inv_map_table[d], 255 - p);
}

static RK_S32 mpp_get_bit1(BitReadCtx_t *gb)
{
    RK_S32 value;
    READ_ONEBIT(gb, &value);
    return value;
__BITREAD_ERR:
    return 0;
}

static RK_S32 mpp_get_bits(BitReadCtx_t *gb, RK_S32 num_bit)
{
    RK_S32 value;
    READ_BITS(gb, num_bit, &value);
    return value;
__BITREAD_ERR:
    return 0;
}

static RK_S32 read_colorspace_details(Vp9CodecContext *ctx)
{
    static const MppFrameColorSpace colorspaces[8] = {
        MPP_FRAME_SPC_UNSPECIFIED, MPP_FRAME_SPC_BT470BG, MPP_FRAME_SPC_BT709, MPP_FRAME_SPC_SMPTE170M,
        MPP_FRAME_SPC_SMPTE240M, MPP_FRAME_SPC_BT2020_NCL, MPP_FRAME_SPC_RESERVED, MPP_FRAME_SPC_RGB,
    };
    VP9Context *s = ctx->priv_data;
    RK_S32 res;
    RK_S32 bits = ctx->profile <= 1 ? 0 : 1 + mpp_get_bit1(&s->gb); // 0:8, 1:10, 2:12

    vp9d_dbg(VP9D_DBG_HEADER, "bit_depth %d", 8 + bits * 2);
    s->bpp_index = bits;
    s->bpp = 8 + bits * 2;
    s->bytesperpixel = (7 + s->bpp) >> 3;
    ctx->colorspace = colorspaces[mpp_get_bits(&s->gb, 3)];
    vp9d_dbg(VP9D_DBG_HEADER, "color_space %d", ctx->colorspace);
    if (ctx->colorspace == MPP_FRAME_SPC_RGB) { // RGB = profile 1

        {
            mpp_err("RGB not supported in profile %d\n", ctx->profile);
            return MPP_ERR_STREAM;
        }
    } else {
        static const RK_S32 pix_fmt_for_ss[3][2 /* v */][2 /* h */] = {
            {   { -1, MPP_FMT_YUV422SP },
                { -1, MPP_FMT_YUV420SP }
            },
            {   { -1, MPP_FMT_YUV422SP_10BIT},
                { -1, MPP_FMT_YUV420SP_10BIT}
            },
            {   { -1, -1 },
                { -1, -1 }
            }
        };
        ctx->color_range = mpp_get_bit1(&s->gb) ? MPP_FRAME_RANGE_JPEG : MPP_FRAME_RANGE_MPEG;
        vp9d_dbg(VP9D_DBG_HEADER, "color_range %d", ctx->color_range);
        if (ctx->profile & 1) {
            s->ss_h = mpp_get_bit1(&s->gb);
            vp9d_dbg(VP9D_DBG_HEADER, "subsampling_x %d", s->ss_h);
            s->ss_v = mpp_get_bit1(&s->gb);
            vp9d_dbg(VP9D_DBG_HEADER, "subsampling_y %d", s->ss_v);
            s->extra_plane = 0;
            if ((res = pix_fmt_for_ss[bits][s->ss_v][s->ss_h]) == MPP_FMT_YUV420SP) {
                mpp_err("YUV 4:2:0 not supported in profile %d\n", ctx->profile);
                return MPP_ERR_STREAM;
            } else if (mpp_get_bit1(&s->gb)) {
                s->extra_plane = 1;
                vp9d_dbg(VP9D_DBG_HEADER, "has_extra_plane 1");
                mpp_err("Profile %d color details reserved bit set\n", ctx->profile);
                return  MPP_ERR_STREAM;
            }
        } else {
            s->extra_plane = 0;
            s->ss_h = s->ss_v = 1;
            res = pix_fmt_for_ss[bits][1][1];
        }
    }

    return res;
}

static RK_S32 decode012(BitReadCtx_t *gb)
{
    RK_S32 n;
    n = mpp_get_bit1(gb);
    if (n == 0)
        return 0;
    else
        return mpp_get_bit1(gb) + 1;
}

static RK_S32 decode_parser_header(Vp9CodecContext *ctx,
                                   const RK_U8 *data, RK_S32 size, RK_S32 *refo)
{
    VP9Context *s = ctx->priv_data;
    RK_S32 c, i, j, k, l, m, n, max, size2, res, sharp;
    RK_U32 w, h;
    RK_S32 fmt = ctx->pix_fmt;
    RK_S32 last_invisible;
    const RK_U8 *data2;
#ifdef dump
    char filename[20] = "data/acoef";
    if (vp9_p_fp2 != NULL) {
        fclose(vp9_p_fp2);

    }
    sprintf(&filename[10], "%d.bin", dec_num);
    vp9_p_fp2 = fopen(filename, "wb");
#endif

    /* general header */
    mpp_set_bitread_ctx(&s->gb, (RK_U8*)data, size);
    if (mpp_get_bits(&s->gb, 2) != 0x2) { // frame marker
        mpp_err("Invalid frame marker\n");
        return MPP_ERR_STREAM;
    }
    ctx->profile  = mpp_get_bit1(&s->gb);
    ctx->profile |= mpp_get_bit1(&s->gb) << 1;
    if (ctx->profile == 3) ctx->profile += mpp_get_bit1(&s->gb);
    if (ctx->profile > 3) {
        mpp_err("Profile %d is not yet supported\n", ctx->profile);
        return MPP_ERR_STREAM;
    }
    vp9d_dbg(VP9D_DBG_HEADER, "profile %d", ctx->profile);
    if (mpp_get_bit1(&s->gb)) {
        vp9d_dbg(VP9D_DBG_HEADER, "show_existing_frame 1");
        *refo = mpp_get_bits(&s->gb, 3);
        vp9d_dbg(VP9D_DBG_HEADER, "frame_to_show %d", *refo);
        return 0;
    }
    s->last_keyframe  = s->keyframe;
    s->keyframe       = !mpp_get_bit1(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "frame_type %d", s->keyframe);
    last_invisible    = s->invisible;
    s->invisible      = !mpp_get_bit1(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "show_frame_flag %d", s->invisible);
    s->errorres       = mpp_get_bit1(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "error_resilient_mode %d", s->errorres);
    s->use_last_frame_mvs = !s->errorres && !last_invisible;
    s->got_keyframes += s->keyframe ? 1 : 0;
    vp9d_dbg(VP9D_DBG_HEADER, "keyframe=%d, intraonly=%d, got_keyframes=%d\n",
             s->keyframe, s->intraonly, s->got_keyframes);
    if (!s->got_keyframes) {
        mpp_err_f("have not got keyframe.\n");
        return MPP_ERR_STREAM;
    }
    if (s->keyframe) {
        if (mpp_get_bits(&s->gb, 24) != VP9_SYNCCODE) { // synccode
            mpp_err("Invalid sync code\n");
            return MPP_ERR_STREAM;
        }
        if ((fmt = read_colorspace_details(ctx)) < 0)
            return fmt;
        // for profile 1, here follows the subsampling bits
        s->refreshrefmask = 0xff;
        w = mpp_get_bits(&s->gb, 16) + 1;
        vp9d_dbg(VP9D_DBG_HEADER, "frame_size_width %d", w);
        h = mpp_get_bits(&s->gb, 16) + 1;
        vp9d_dbg(VP9D_DBG_HEADER, "frame_size_height %d", h);
        if (mpp_get_bit1(&s->gb)) {// display size
            RK_S32 dw, dh;
            vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 1);
            dw = mpp_get_bits(&s->gb, 16) + 1;
            vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dw);
            dh = mpp_get_bits(&s->gb, 16) + 1;
            vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dh);
        } else
            vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 0);
    } else {
        s->intraonly  = s->invisible ? mpp_get_bit1(&s->gb) : 0;
        vp9d_dbg(VP9D_DBG_HEADER, "intra_only %d", s->intraonly);
        s->resetctx   = s->errorres ? 0 : mpp_get_bits(&s->gb, 2);
        vp9d_dbg(VP9D_DBG_HEADER, "reset_frame_context_value %d", s->resetctx);
        if (s->intraonly) {
            if (mpp_get_bits(&s->gb, 24) != VP9_SYNCCODE) { // synccode
                mpp_err("Invalid sync code\n");
                return MPP_ERR_STREAM;
            }
            if (ctx->profile == 1) {
                if ((fmt = read_colorspace_details(ctx)) < 0)
                    return fmt;
            } else {
                s->ss_h = s->ss_v = 1;
                s->bpp = 8;
                s->bpp_index = 0;
                s->bytesperpixel = 1;
                fmt = MPP_FMT_YUV420SP;
                ctx->colorspace = MPP_FRAME_SPC_BT470BG;
                ctx->color_range = MPP_FRAME_RANGE_JPEG;
            }
            s->refreshrefmask = mpp_get_bits(&s->gb, 8);
            vp9d_dbg(VP9D_DBG_HEADER, "refresh_frame_flags %d", s->refreshrefmask);
            w = mpp_get_bits(&s->gb, 16) + 1;
            vp9d_dbg(VP9D_DBG_HEADER, "frame_size_width %d", w);
            h = mpp_get_bits(&s->gb, 16) + 1;
            vp9d_dbg(VP9D_DBG_HEADER, "frame_size_height %d", h);
            if (mpp_get_bit1(&s->gb)) {// display size
                RK_S32 dw, dh;
                vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 1);
                dw = mpp_get_bits(&s->gb, 16) + 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dw);
                dh = mpp_get_bits(&s->gb, 16) + 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dh);
            } else
                vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 0);
        } else {
            s->refreshrefmask = mpp_get_bits(&s->gb, 8);
            vp9d_dbg(VP9D_DBG_HEADER, "refresh_frame_flags %d", s->refreshrefmask);
            s->refidx[0]      = mpp_get_bits(&s->gb, 3);
            s->signbias[0]    = mpp_get_bit1(&s->gb) && !s->errorres;
            s->refidx[1]      = mpp_get_bits(&s->gb, 3);
            s->signbias[1]    = mpp_get_bit1(&s->gb) && !s->errorres;
            s->refidx[2]      = mpp_get_bits(&s->gb, 3);
            s->signbias[2]    = mpp_get_bit1(&s->gb) && !s->errorres;
            vp9d_dbg(VP9D_DBG_HEADER, "ref_idx %d %d %d",
                     s->refidx[0], s->refidx[1], s->refidx[2]);
            vp9d_dbg(VP9D_DBG_HEADER, "ref_idx_ref_frame_sign_bias %d %d %d",
                     s->signbias[0], s->signbias[1], s->signbias[2]);
            if (!s->refs[s->refidx[0]].ref ||
                !s->refs[s->refidx[1]].ref ||
                !s->refs[s->refidx[2]].ref ) {
                mpp_err("Not all references are available\n");
                //return -1;//AVERROR_INVALIDDATA;
            }
            if (mpp_get_bit1(&s->gb)) {

                vp9d_dbg(VP9D_DBG_HEADER, "ref_flag 0");
                w = mpp_frame_get_width(s->refs[s->refidx[0]].f);
                h = mpp_frame_get_height(s->refs[s->refidx[0]].f);
            } else if (mpp_get_bit1(&s->gb)) {
                vp9d_dbg(VP9D_DBG_HEADER, "ref_flag 2");
                w = mpp_frame_get_width(s->refs[s->refidx[1]].f);
                h = mpp_frame_get_height(s->refs[s->refidx[1]].f);
            } else if (mpp_get_bit1(&s->gb)) {
                vp9d_dbg(VP9D_DBG_HEADER, "ref_flag 1");
                w = mpp_frame_get_width(s->refs[s->refidx[2]].f);
                h = mpp_frame_get_height(s->refs[s->refidx[2]].f);
            } else {
                w = mpp_get_bits(&s->gb, 16) + 1;
                vp9d_dbg(VP9D_DBG_HEADER, "frame_size_width %d", w);
                h = mpp_get_bits(&s->gb, 16) + 1;
                vp9d_dbg(VP9D_DBG_HEADER, "frame_size_height %d", h);
            }
            // Note that in this code, "CUR_FRAME" is actually before we
            // have formally allocated a frame, and thus actually represents
            // the _last_ frame
            s->use_last_frame_mvs &= mpp_frame_get_width(s->frames[CUR_FRAME].f) == w &&
                                     mpp_frame_get_height(s->frames[CUR_FRAME].f) == h;
            if (mpp_get_bit1(&s->gb)) {// display size
                RK_S32 dw, dh;
                vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 1);
                dw = mpp_get_bits(&s->gb, 16) + 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dw);
                dh = mpp_get_bits(&s->gb, 16) + 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dh);
            } else
                vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 0);
            s->highprecisionmvs = mpp_get_bit1(&s->gb);
            vp9d_dbg(VP9D_DBG_HEADER, "allow_high_precision_mv %d", s->highprecisionmvs);
            s->filtermode = mpp_get_bit1(&s->gb) ? FILTER_SWITCHABLE :
                            mpp_get_bits(&s->gb, 2);
            vp9d_dbg(VP9D_DBG_HEADER, "filtermode %d", s->filtermode);
            s->allowcompinter = (s->signbias[0] != s->signbias[1] ||
                                 s->signbias[0] != s->signbias[2]);
            if (s->allowcompinter) {
                if (s->signbias[0] == s->signbias[1]) {
                    s->fixcompref    = 2;
                    s->varcompref[0] = 0;
                    s->varcompref[1] = 1;
                } else if (s->signbias[0] == s->signbias[2]) {
                    s->fixcompref    = 1;
                    s->varcompref[0] = 0;
                    s->varcompref[1] = 2;
                } else {
                    s->fixcompref    = 0;
                    s->varcompref[0] = 1;
                    s->varcompref[1] = 2;
                }
            }

            for (i = 0; i < 3; i++) {
                RK_U32 refw = mpp_frame_get_width(s->refs[s->refidx[i]].f);
                RK_U32 refh = mpp_frame_get_height(s->refs[s->refidx[i]].f);
                RK_S32 reffmt =  mpp_frame_get_fmt(s->refs[s->refidx[i]].f);

                vp9d_dbg(VP9D_DBG_REF, "ref get width frame slot %p", s->refs[s->refidx[i]].f);
                if (reffmt != fmt) {
                    /* mpp_err("Ref pixfmt (%s) did not match current frame (%s)",
                           av_get_pix_fmt_name(ref->format),
                           av_get_pix_fmt_name(fmt)); */
                    //return -1;//AVERROR_INVALIDDATA;
                } else if (refw == w && refh == h) {
                    s->mvscale[i][0] = (refw << 14) / w;
                    s->mvscale[i][1] = (refh << 14) / h;
                } else {
                    if (w * 2 < refw || h * 2 < refh || w > 16 * refw || h > 16 * refh) {
                        mpp_err("Invalid ref frame dimensions %dx%d for frame size %dx%d\n",
                                refw, refh, w, h);
                        return MPP_ERR_VALUE;
                    }
                    s->mvscale[i][0] = (refw << 14) / w;
                    s->mvscale[i][1] = (refh << 14) / h;
                    s->mvstep[i][0] = 16 * s->mvscale[i][0] >> 14;
                    s->mvstep[i][1] = 16 * s->mvscale[i][1] >> 14;
                }
            }
        }
    }

    s->refreshctx   = s->errorres ? 0 : mpp_get_bit1(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "refresh_frame_context_flag %d", s->refreshctx);
    s->parallelmode = s->errorres ? 1 : mpp_get_bit1(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "frame_parallel_decoding_mode %d", s->parallelmode);
    s->framectxid   = c = mpp_get_bits(&s->gb, 2);
    vp9d_dbg(VP9D_DBG_HEADER, "frame_context_idx %d", s->framectxid);

    /* loopfilter header data */
    if (s->keyframe || s->errorres || s->intraonly) {
        // reset loopfilter defaults
        s->lf_delta.ref[0] = 1;
        s->lf_delta.ref[1] = 0;
        s->lf_delta.ref[2] = -1;
        s->lf_delta.ref[3] = -1;
        s->lf_delta.mode[0] = 0;
        s->lf_delta.mode[1] = 0;
    }
    s->filter.level = mpp_get_bits(&s->gb, 6);
    vp9d_dbg(VP9D_DBG_HEADER, "filter_level %d", s->filter.level);
    sharp = mpp_get_bits(&s->gb, 3);
    vp9d_dbg(VP9D_DBG_HEADER, "sharpness_level %d", sharp);
    // if sharpness changed, reinit lim/mblim LUTs. if it didn't change, keep
    // the old cache values since they are still valid
    if (s->filter.sharpness != sharp)
        memset(s->filter.lim_lut, 0, sizeof(s->filter.lim_lut));
    s->filter.sharpness = sharp;

    if ((s->lf_delta.enabled = mpp_get_bit1(&s->gb))) {
        vp9d_dbg(VP9D_DBG_HEADER, "mode_ref_delta_enabled 1");
        if ((s->lf_delta.update = mpp_get_bit1(&s->gb))) {
            vp9d_dbg(VP9D_DBG_HEADER, "mode_ref_delta_update 1");
            for (i = 0; i < 4; i++) {
                if (mpp_get_bit1(&s->gb))
                    s->lf_delta.ref[i] = get_sbits_inv(&s->gb, 6);
                vp9d_dbg(VP9D_DBG_HEADER, "ref_deltas %d", s->lf_delta.ref[i]);
            }
            for (i = 0; i < 2; i++) {
                if (mpp_get_bit1(&s->gb))
                    s->lf_delta.mode[i] = get_sbits_inv(&s->gb, 6);
                vp9d_dbg(VP9D_DBG_HEADER, "mode_deltas %d", s->lf_delta.mode[i]);
            }
        }
    }

    /* quantization header data */
    s->yac_qi      = mpp_get_bits(&s->gb, 8);
    vp9d_dbg(VP9D_DBG_HEADER, "base_qindex %d", s->yac_qi);
    s->ydc_qdelta  = mpp_get_bit1(&s->gb) ? get_sbits_inv(&s->gb, 4) : 0;
    vp9d_dbg(VP9D_DBG_HEADER, "ydc_qdelta %d", s->ydc_qdelta);
    s->uvdc_qdelta = mpp_get_bit1(&s->gb) ? get_sbits_inv(&s->gb, 4) : 0;
    vp9d_dbg(VP9D_DBG_HEADER, "uvdc_qdelta %d", s->uvdc_qdelta);
    s->uvac_qdelta = mpp_get_bit1(&s->gb) ? get_sbits_inv(&s->gb, 4) : 0;
    vp9d_dbg(VP9D_DBG_HEADER, "uvac_qdelta %d", s->uvac_qdelta);
    s->lossless    = s->yac_qi == 0 && s->ydc_qdelta == 0 &&
                     s->uvdc_qdelta == 0 && s->uvac_qdelta == 0;

    /* segmentation header info */

    s->segmentation.ignore_refmap = 0;
    if ((s->segmentation.enabled = mpp_get_bit1(&s->gb))) {
        vp9d_dbg(VP9D_DBG_HEADER, "segmentation_enabled 1");
        if ((s->segmentation.update_map = mpp_get_bit1(&s->gb))) {
            vp9d_dbg(VP9D_DBG_HEADER, "update_map 1");
            for (i = 0; i < 7; i++) {
                s->prob.seg[i] = mpp_get_bit1(&s->gb) ?
                                 mpp_get_bits(&s->gb, 8) : 255;
                vp9d_dbg(VP9D_DBG_HEADER, "tree_probs %d value 0x%x", i, s->prob.seg[i]);
            }
            s->segmentation.temporal = mpp_get_bit1(&s->gb);
            if (s->segmentation.temporal) {
                vp9d_dbg(VP9D_DBG_HEADER, "tempora_update 1");
                for (i = 0; i < 3; i++) {
                    s->prob.segpred[i] = mpp_get_bit1(&s->gb) ?
                                         mpp_get_bits(&s->gb, 8) : 255;
                    vp9d_dbg(VP9D_DBG_HEADER, "pred_probs %d", i, s->prob.segpred[i]);
                }
            } else {
                for (i = 0; i < 3; i++)
                    s->prob.segpred[i] = 0xff;
            }
        }
        if ((!s->segmentation.update_map || s->segmentation.temporal) &&
            (w !=  mpp_frame_get_width(s->frames[CUR_FRAME].f) ||
             h !=  mpp_frame_get_height(s->frames[CUR_FRAME].f))) {
            /* av_log(ctx, AV_LOG_WARNING,
                   "Reference segmap (temp=%d,update=%d) enabled on size-change!\n",
                   s->segmentation.temporal, s->segmentation.update_map);
                s->segmentation.ignore_refmap = 1; */
            //return -1;//AVERROR_INVALIDDATA;
        }

        if (mpp_get_bit1(&s->gb)) {
            vp9d_dbg(VP9D_DBG_HEADER, "update_data 1");
            s->segmentation.absolute_vals = mpp_get_bit1(&s->gb);
            vp9d_dbg(VP9D_DBG_HEADER, "abs_delta %d", s->segmentation.absolute_vals);
            for (i = 0; i < 8; i++) {
                if ((s->segmentation.feat[i].q_enabled = mpp_get_bit1(&s->gb)))
                    s->segmentation.feat[i].q_val = get_sbits_inv(&s->gb, 8);
                vp9d_dbg(VP9D_DBG_HEADER, "frame_qp_delta %d", s->segmentation.feat[i].q_val);
                if ((s->segmentation.feat[i].lf_enabled = mpp_get_bit1(&s->gb)))
                    s->segmentation.feat[i].lf_val = get_sbits_inv(&s->gb, 6);
                vp9d_dbg(VP9D_DBG_HEADER, "frame_loopfilter_value %d", i, s->segmentation.feat[i].lf_val);
                if ((s->segmentation.feat[i].ref_enabled = mpp_get_bit1(&s->gb)))
                    s->segmentation.feat[i].ref_val = mpp_get_bits(&s->gb, 2);
                vp9d_dbg(VP9D_DBG_HEADER, "frame_reference_info %d", i, s->segmentation.feat[i].ref_val);
                s->segmentation.feat[i].skip_enabled = mpp_get_bit1(&s->gb);
                vp9d_dbg(VP9D_DBG_HEADER, "frame_skip %d", i, s->segmentation.feat[i].skip_enabled);
            }
        }
    } else {
        vp9d_dbg(VP9D_DBG_HEADER, "segmentation_enabled 0");
        s->segmentation.feat[0].q_enabled    = 0;
        s->segmentation.feat[0].lf_enabled   = 0;
        s->segmentation.feat[0].skip_enabled = 0;
        s->segmentation.feat[0].ref_enabled  = 0;
    }

    // set qmul[] based on Y/UV, AC/DC and segmentation Q idx deltas
    for (i = 0; i < (s->segmentation.enabled ? 8 : 1); i++) {
        RK_S32 qyac, qydc, quvac, quvdc, lflvl, sh;

        if (s->segmentation.feat[i].q_enabled) {
            if (s->segmentation.absolute_vals)
                qyac = s->segmentation.feat[i].q_val;
            else
                qyac = s->yac_qi + s->segmentation.feat[i].q_val;
        } else {
            qyac  = s->yac_qi;
        }
        qydc  = av_clip_uintp2(qyac + s->ydc_qdelta, 8);
        quvdc = av_clip_uintp2(qyac + s->uvdc_qdelta, 8);
        quvac = av_clip_uintp2(qyac + s->uvac_qdelta, 8);
        qyac  = av_clip_uintp2(qyac, 8);

        s->segmentation.feat[i].qmul[0][0] = vp9_dc_qlookup[s->bpp_index][qydc];
        s->segmentation.feat[i].qmul[0][1] = vp9_ac_qlookup[s->bpp_index][qyac];
        s->segmentation.feat[i].qmul[1][0] = vp9_dc_qlookup[s->bpp_index][quvdc];
        s->segmentation.feat[i].qmul[1][1] = vp9_ac_qlookup[s->bpp_index][quvac];

        sh = s->filter.level >= 32;
        if (s->segmentation.feat[i].lf_enabled) {
            if (s->segmentation.absolute_vals)
                lflvl = av_clip_uintp2(s->segmentation.feat[i].lf_val, 6);
            else
                lflvl = av_clip_uintp2(s->filter.level + s->segmentation.feat[i].lf_val, 6);
        } else {
            lflvl  = s->filter.level;
        }
        if (s->lf_delta.enabled) {
            s->segmentation.feat[i].lflvl[0][0] =
                s->segmentation.feat[i].lflvl[0][1] =
                    av_clip_uintp2(lflvl + (s->lf_delta.ref[0] << sh), 6);
            for (j = 1; j < 4; j++) {
                s->segmentation.feat[i].lflvl[j][0] =
                    av_clip_uintp2(lflvl + ((s->lf_delta.ref[j] +
                                             s->lf_delta.mode[0]) * (1 << sh)), 6);
                s->segmentation.feat[i].lflvl[j][1] =
                    av_clip_uintp2(lflvl + ((s->lf_delta.ref[j] +
                                             s->lf_delta.mode[1]) * (1 << sh)), 6);
            }
        } else {
            memset(s->segmentation.feat[i].lflvl, lflvl,
                   sizeof(s->segmentation.feat[i].lflvl));
        }
    }

    /* tiling info */
    if ((res = update_size(ctx, w, h, fmt)) < 0) {
        mpp_err("Failed to initialize decoder for %dx%d @ %d\n", w, h, fmt);
        return res;
    }

    for (s->tiling.log2_tile_cols = 0;
         (s->sb_cols >> s->tiling.log2_tile_cols) > 64;
         s->tiling.log2_tile_cols++) ;
    for (max = 0; (s->sb_cols >> max) >= 4; max++) ;
    max = MPP_MAX(0, max - 1);
    while ((RK_U32)max > s->tiling.log2_tile_cols) {
        if (mpp_get_bit1(&s->gb)) {
            s->tiling.log2_tile_cols++;
            vp9d_dbg(VP9D_DBG_HEADER, "log2_tile_col_end_flag 1");
        } else {
            vp9d_dbg(VP9D_DBG_HEADER, "log2_tile_col_end_flag 0");
            break;
        }
    }
    s->tiling.log2_tile_rows = decode012(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "log2_tile_rows %d", s->tiling.log2_tile_rows);
    s->tiling.tile_rows = 1 << s->tiling.log2_tile_rows;
    if (s->tiling.tile_cols != (1U << s->tiling.log2_tile_cols)) {
        s->tiling.tile_cols = 1 << s->tiling.log2_tile_cols;
        {
            RK_U32 min_size = sizeof(VpxRangeCoder) * s->tiling.tile_cols;
            if (min_size > s->c_b_size) {
                s->c_b = (VpxRangeCoder *)mpp_malloc(RK_U8, min_size);
                s->c_b_size = min_size;
            }
        }
        if (!s->c_b) {
            mpp_err("Ran out of memory during range coder init\n");
            return MPP_ERR_NOMEM;
        }
    }

    if (s->keyframe || s->errorres ||
        (s->intraonly && s->resetctx == 3)) {
        s->prob_ctx[0].p = s->prob_ctx[1].p = s->prob_ctx[2].p =
                                                  s->prob_ctx[3].p = vp9_default_probs;
        memcpy(s->prob_ctx[0].coef, vp9_default_coef_probs,
               sizeof(vp9_default_coef_probs));
        memcpy(s->prob_ctx[1].coef, vp9_default_coef_probs,
               sizeof(vp9_default_coef_probs));
        memcpy(s->prob_ctx[2].coef, vp9_default_coef_probs,
               sizeof(vp9_default_coef_probs));
        memcpy(s->prob_ctx[3].coef, vp9_default_coef_probs,
               sizeof(vp9_default_coef_probs));
    } else if (s->intraonly && s->resetctx == 2) {
        s->prob_ctx[c].p = vp9_default_probs;
        memcpy(s->prob_ctx[c].coef, vp9_default_coef_probs,
               sizeof(vp9_default_coef_probs));
    }
    if (s->keyframe || s->errorres || s->intraonly)
        s->framectxid = c = 0;

    // next 16 bits is size of the rest of the header (arith-coded)
    size2 = mpp_get_bits(&s->gb, 16);
    vp9d_dbg(VP9D_DBG_HEADER, "first_partition_size %d", size2);
    data2 = mpp_align_get_bits(&s->gb);
    vp9d_dbg(VP9D_DBG_HEADER, "offset %d", data2 - data);
    if (size2 > size - (data2 - data)) {
        mpp_err("Invalid compressed header size\n");
        return MPP_ERR_STREAM;
    }
    vpx_init_range_decoder(&s->c, data2, size2);
    if (vpx_rac_get_prob_branchy(&s->c, 128)) { // marker bit
        mpp_err("Marker bit was set\n");
        return MPP_ERR_STREAM;
    }

    if (s->keyframe || s->intraonly) {
        memset(s->counts.coef, 0, sizeof(s->counts.coef));
        memset(s->counts.eob,  0, sizeof(s->counts.eob));
    } else {
        memset(&s->counts, 0, sizeof(s->counts));
    }
    // FIXME is it faster to not copy here, but do it down in the fw updates
    // as explicit copies if the fw update is missing (and skip the copy upon
    // fw update)?
    s->prob.p = s->prob_ctx[c].p;
    // txfm updates
    if (s->lossless) {
        s->txfmmode = TX_4X4;
    } else {
        s->txfmmode = vpx_rac_get_uint(&s->c, 2);
        if (s->txfmmode == 3)
            s->txfmmode += vpx_rac_get(&s->c);

        if (s->txfmmode == TX_SWITCHABLE) {
            for (i = 0; i < 2; i++) {

                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.tx8p[i] = update_prob(&s->c, s->prob.p.tx8p[i]);
            }
            for (i = 0; i < 2; i++)
                for (j = 0; j < 2; j++) {

                    if (vpx_rac_get_prob_branchy(&s->c, 252))
                        s->prob.p.tx16p[i][j] =
                            update_prob(&s->c, s->prob.p.tx16p[i][j]);
                }
            for (i = 0; i < 2; i++)
                for (j = 0; j < 3; j++) {

                    if (vpx_rac_get_prob_branchy(&s->c, 252))
                        s->prob.p.tx32p[i][j] =
                            update_prob(&s->c, s->prob.p.tx32p[i][j]);

                }
        }
    }

    // coef updates
    for (i = 0; i < 4; i++) {
        RK_U8 (*ref)[2][6][6][3] = s->prob_ctx[c].coef[i];
        if (vpx_rac_get(&s->c)) {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 2; k++)
                    for (l = 0; l < 6; l++)
                        for (m = 0; m < 6; m++) {
                            RK_U8 *p = s->prob.coef[i][j][k][l][m];
                            RK_U8 *r = ref[j][k][l][m];
                            if (m >= 3 && l == 0) // dc only has 3 pt
                                break;
                            for (n = 0; n < 3; n++) {
                                if (vpx_rac_get_prob_branchy(&s->c, 252)) {
                                    p[n] = update_prob(&s->c, r[n]);
                                } else {
                                    p[n] = r[n];
                                }
                            }
                            p[3] = 0;
                        }
        } else {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 2; k++)
                    for (l = 0; l < 6; l++)
                        for (m = 0; m < 6; m++) {
                            RK_U8 *p = s->prob.coef[i][j][k][l][m];
                            RK_U8 *r = ref[j][k][l][m];
                            if (m > 3 && l == 0) // dc only has 3 pt
                                break;
                            memcpy(p, r, 3);
                            p[3] = 0;
                        }
        }
        if (s->txfmmode == (RK_U32)i)
            break;
    }

    // mode updates
    for (i = 0; i < 3; i++) {

        if (vpx_rac_get_prob_branchy(&s->c, 252))
            s->prob.p.skip[i] = update_prob(&s->c, s->prob.p.skip[i]);
    }

    if (!s->keyframe && !s->intraonly) {
        for (i = 0; i < 7; i++)
            for (j = 0; j < 3; j++)
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.mv_mode[i][j] =
                        update_prob(&s->c, s->prob.p.mv_mode[i][j]);

        if (s->filtermode == FILTER_SWITCHABLE)
            for (i = 0; i < 4; i++)
                for (j = 0; j < 2; j++)
                    if (vpx_rac_get_prob_branchy(&s->c, 252))
                        s->prob.p.filter[i][j] =
                            update_prob(&s->c, s->prob.p.filter[i][j]);

        for (i = 0; i < 4; i++) {

            if (vpx_rac_get_prob_branchy(&s->c, 252))
                s->prob.p.intra[i] = update_prob(&s->c, s->prob.p.intra[i]);

        }

        if (s->allowcompinter) {
            s->comppredmode = vpx_rac_get(&s->c);
            if (s->comppredmode)
                s->comppredmode += vpx_rac_get(&s->c);
            if (s->comppredmode == PRED_SWITCHABLE)
                for (i = 0; i < 5; i++)
                    if (vpx_rac_get_prob_branchy(&s->c, 252))
                        s->prob.p.comp[i] =
                            update_prob(&s->c, s->prob.p.comp[i]);
        } else {
            s->comppredmode = PRED_SINGLEREF;
        }

        if (s->comppredmode != PRED_COMPREF) {
            for (i = 0; i < 5; i++) {
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.single_ref[i][0] =
                        update_prob(&s->c, s->prob.p.single_ref[i][0]);
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.single_ref[i][1] =
                        update_prob(&s->c, s->prob.p.single_ref[i][1]);
            }
        }

        if (s->comppredmode != PRED_SINGLEREF) {
            for (i = 0; i < 5; i++)
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.comp_ref[i] =
                        update_prob(&s->c, s->prob.p.comp_ref[i]);
        }

        for (i = 0; i < 4; i++)
            for (j = 0; j < 9; j++)
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.y_mode[i][j] =
                        update_prob(&s->c, s->prob.p.y_mode[i][j]);


        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                for (k = 0; k < 3; k++)
                    if (vpx_rac_get_prob_branchy(&s->c, 252))
                        s->prob.p.partition[3 - i][j][k] =
                            update_prob(&s->c, s->prob.p.partition[3 - i][j][k]);
        // mv fields don't use the update_prob subexp model for some reason
        for (i = 0; i < 3; i++)
            if (vpx_rac_get_prob_branchy(&s->c, 252))
                s->prob.p.mv_joint[i] = (vpx_rac_get_uint(&s->c, 7) << 1) | 1;

        for (i = 0; i < 2; i++) {
            if (vpx_rac_get_prob_branchy(&s->c, 252))
                s->prob.p.mv_comp[i].sign = (vpx_rac_get_uint(&s->c, 7) << 1) | 1;

            for (j = 0; j < 10; j++)
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.mv_comp[i].classes[j] =
                        (vpx_rac_get_uint(&s->c, 7) << 1) | 1;

            if (vpx_rac_get_prob_branchy(&s->c, 252))
                s->prob.p.mv_comp[i].class0 = (vpx_rac_get_uint(&s->c, 7) << 1) | 1;

            for (j = 0; j < 10; j++)
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.mv_comp[i].bits[j] =
                        (vpx_rac_get_uint(&s->c, 7) << 1) | 1;
        }

        for (i = 0; i < 2; i++) {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 3; k++)
                    if (vpx_rac_get_prob_branchy(&s->c, 252))
                        s->prob.p.mv_comp[i].class0_fp[j][k] =
                            (vpx_rac_get_uint(&s->c, 7) << 1) | 1;

            for (j = 0; j < 3; j++)
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.mv_comp[i].fp[j] =
                        (vpx_rac_get_uint(&s->c, 7) << 1) | 1;
        }

        if (s->highprecisionmvs) {
            for (i = 0; i < 2; i++) {
                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.mv_comp[i].class0_hp =
                        (vpx_rac_get_uint(&s->c, 7) << 1) | 1;

                if (vpx_rac_get_prob_branchy(&s->c, 252))
                    s->prob.p.mv_comp[i].hp =
                        (vpx_rac_get_uint(&s->c, 7) << 1) | 1;
            }
        }
    }

    return (RK_S32)((data2 - data) + size2);
}

static void adapt_prob(RK_U8 *p, RK_U32 ct0, RK_U32 ct1,
                       RK_S32 max_count, RK_S32 update_factor)
{
    RK_U32 ct = ct0 + ct1, p2, p1;

    if (!ct)
        return;

    p1 = *p;
    p2 = ((ct0 << 8) + (ct >> 1)) / ct;
    p2 = mpp_clip(p2, 1, 255);
    ct = MPP_MIN(ct, (RK_U32)max_count);
    update_factor = FASTDIV(update_factor * ct, max_count);

    // (p1 * (256 - update_factor) + p2 * update_factor + 128) >> 8
    *p = p1 + (((p2 - p1) * update_factor + 128) >> 8);
}

static void adapt_probs(VP9Context *s)
{
    RK_S32 i, j, k, l, m;
    prob_context *p = &s->prob_ctx[s->framectxid].p;
    RK_S32 uf = (s->keyframe || s->intraonly || !s->last_keyframe) ? 112 : 128;

    // coefficients
    for (i = 0; i < 4; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 2; k++)
                for (l = 0; l < 6; l++)
                    for (m = 0; m < 6; m++) {
                        RK_U8 *pp = s->prob_ctx[s->framectxid].coef[i][j][k][l][m];
                        RK_U32 *e = s->counts.eob[i][j][k][l][m];
                        RK_U32 *c = s->counts.coef[i][j][k][l][m];

                        if (l == 0 && m >= 3) // dc only has 3 pt
                            break;
                        /*  if(i == 0 && j == 0 && k== 1 && l == 0){
                             mpp_log("e[0] = 0x%x e[1] = 0x%x c[0] = 0x%x c[1] = 0x%x c[2] = 0x%x \n",
                             e[0],e[1],c[0],c[1],c[2]);
                             mpp_log("pp[0] = 0x%x pp[1] = 0x%x pp[2] = 0x%x",pp[0],pp[1],pp[2]);
                          }*/
                        adapt_prob(&pp[0], e[0], e[1], 24, uf);
                        adapt_prob(&pp[1], c[0], c[1] + c[2], 24, uf);
                        adapt_prob(&pp[2], c[1], c[2], 24, uf);
                        /* if(i == 0 && j == 0 && k== 1 && l == 0){
                            mpp_log("after pp[0] = 0x%x pp[1] = 0x%x pp[2] = 0x%x",pp[0],pp[1],pp[2]);
                         }*/
                    }
#ifdef dump
    fwrite(&s->counts, 1, sizeof(s->counts), vp9_p_fp);
    fflush(vp9_p_fp);
#endif

    if (s->keyframe || s->intraonly) {
        memcpy(p->skip,  s->prob.p.skip,  sizeof(p->skip));
        memcpy(p->tx32p, s->prob.p.tx32p, sizeof(p->tx32p));
        memcpy(p->tx16p, s->prob.p.tx16p, sizeof(p->tx16p));
        memcpy(p->tx8p,  s->prob.p.tx8p,  sizeof(p->tx8p));
        return;
    }

    // skip flag
    for (i = 0; i < 3; i++)
        adapt_prob(&p->skip[i], s->counts.skip[i][0], s->counts.skip[i][1], 20, 128);

    // intra/inter flag
    for (i = 0; i < 4; i++)
        adapt_prob(&p->intra[i], s->counts.intra[i][0], s->counts.intra[i][1], 20, 128);

    // comppred flag
    if (s->comppredmode == PRED_SWITCHABLE) {
        for (i = 0; i < 5; i++)
            adapt_prob(&p->comp[i], s->counts.comp[i][0], s->counts.comp[i][1], 20, 128);
    }

    // reference frames
    if (s->comppredmode != PRED_SINGLEREF) {
        for (i = 0; i < 5; i++)
            adapt_prob(&p->comp_ref[i], s->counts.comp_ref[i][0],
                       s->counts.comp_ref[i][1], 20, 128);
    }

    if (s->comppredmode != PRED_COMPREF) {
        for (i = 0; i < 5; i++) {
            RK_U8 *pp = p->single_ref[i];
            RK_U32 (*c)[2] = s->counts.single_ref[i];

            adapt_prob(&pp[0], c[0][0], c[0][1], 20, 128);
            adapt_prob(&pp[1], c[1][0], c[1][1], 20, 128);
        }
    }

    // block partitioning
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            RK_U8 *pp = p->partition[i][j];
            RK_U32 *c = s->counts.partition[i][j];
            // mpp_log("befor pp[0] = 0x%x pp[1] = 0x%x pp[2] = 0x%x",pp[0],pp[1],pp[2]);
            // mpp_log("befor c[0] = 0x%x c[1] = 0x%x c[2] = 0x%x",c[0],c[1],c[2]);
            adapt_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
            adapt_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
            adapt_prob(&pp[2], c[2], c[3], 20, 128);
            // mpp_log(" after pp[0] = 0x%x pp[1] = 0x%x pp[2] = 0x%x",pp[0],pp[1],pp[2]);
        }

    // tx size
    if (s->txfmmode == TX_SWITCHABLE) {
        for (i = 0; i < 2; i++) {
            RK_U32 *c16 = s->counts.tx16p[i], *c32 = s->counts.tx32p[i];

            adapt_prob(&p->tx8p[i], s->counts.tx8p[i][0], s->counts.tx8p[i][1], 20, 128);
            adapt_prob(&p->tx16p[i][0], c16[0], c16[1] + c16[2], 20, 128);
            adapt_prob(&p->tx16p[i][1], c16[1], c16[2], 20, 128);
            adapt_prob(&p->tx32p[i][0], c32[0], c32[1] + c32[2] + c32[3], 20, 128);
            adapt_prob(&p->tx32p[i][1], c32[1], c32[2] + c32[3], 20, 128);
            adapt_prob(&p->tx32p[i][2], c32[2], c32[3], 20, 128);
        }
    }

    // interpolation filter
    if (s->filtermode == FILTER_SWITCHABLE) {
        for (i = 0; i < 4; i++) {
            RK_U8 *pp = p->filter[i];
            RK_U32 *c = s->counts.filter[i];

            adapt_prob(&pp[0], c[0], c[1] + c[2], 20, 128);
            adapt_prob(&pp[1], c[1], c[2], 20, 128);
        }
    }

    // inter modes
    for (i = 0; i < 7; i++) {
        RK_U8 *pp = p->mv_mode[i];
        RK_U32 *c = s->counts.mv_mode[i];

        adapt_prob(&pp[0], c[2], c[1] + c[0] + c[3], 20, 128);
        adapt_prob(&pp[1], c[0], c[1] + c[3], 20, 128);
        adapt_prob(&pp[2], c[1], c[3], 20, 128);
    }

    // mv joints
    {
        RK_U8 *pp = p->mv_joint;
        RK_U32 *c = s->counts.mv_joint;

        adapt_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
        adapt_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
        adapt_prob(&pp[2], c[2], c[3], 20, 128);
    }

    // mv components
    for (i = 0; i < 2; i++) {
        RK_U8 *pp;
        RK_U32 *c, (*c2)[2], sum;

        adapt_prob(&p->mv_comp[i].sign, s->counts.sign[i][0],
                   s->counts.sign[i][1], 20, 128);

        pp = p->mv_comp[i].classes;
        c = s->counts.classes[i];
        sum = c[1] + c[2] + c[3] + c[4] + c[5] + c[6] + c[7] + c[8] + c[9] + c[10];
        adapt_prob(&pp[0], c[0], sum, 20, 128);
        sum -= c[1];
        adapt_prob(&pp[1], c[1], sum, 20, 128);
        sum -= c[2] + c[3];
        adapt_prob(&pp[2], c[2] + c[3], sum, 20, 128);
        adapt_prob(&pp[3], c[2], c[3], 20, 128);
        sum -= c[4] + c[5];
        adapt_prob(&pp[4], c[4] + c[5], sum, 20, 128);
        adapt_prob(&pp[5], c[4], c[5], 20, 128);
        sum -= c[6];
        adapt_prob(&pp[6], c[6], sum, 20, 128);
        adapt_prob(&pp[7], c[7] + c[8], c[9] + c[10], 20, 128);
        adapt_prob(&pp[8], c[7], c[8], 20, 128);
        adapt_prob(&pp[9], c[9], c[10], 20, 128);

        adapt_prob(&p->mv_comp[i].class0, s->counts.class0[i][0],
                   s->counts.class0[i][1], 20, 128);
        pp = p->mv_comp[i].bits;
        c2 = s->counts.bits[i];
        for (j = 0; j < 10; j++)
            adapt_prob(&pp[j], c2[j][0], c2[j][1], 20, 128);

        for (j = 0; j < 2; j++) {
            pp = p->mv_comp[i].class0_fp[j];
            c = s->counts.class0_fp[i][j];
            adapt_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
            adapt_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
            adapt_prob(&pp[2], c[2], c[3], 20, 128);
        }
        pp = p->mv_comp[i].fp;
        c = s->counts.fp[i];
        adapt_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
        adapt_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
        adapt_prob(&pp[2], c[2], c[3], 20, 128);

        if (s->highprecisionmvs) {
            adapt_prob(&p->mv_comp[i].class0_hp, s->counts.class0_hp[i][0],
                       s->counts.class0_hp[i][1], 20, 128);
            adapt_prob(&p->mv_comp[i].hp, s->counts.hp[i][0],
                       s->counts.hp[i][1], 20, 128);
        }
    }

    // y intra modes
    for (i = 0; i < 4; i++) {
        RK_U8 *pp = p->y_mode[i];
        RK_U32 *c = s->counts.y_mode[i], sum, s2;

        sum = c[0] + c[1] + c[3] + c[4] + c[5] + c[6] + c[7] + c[8] + c[9];
        adapt_prob(&pp[0], c[DC_PRED], sum, 20, 128);
        sum -= c[TM_VP8_PRED];
        adapt_prob(&pp[1], c[TM_VP8_PRED], sum, 20, 128);
        sum -= c[VERT_PRED];
        adapt_prob(&pp[2], c[VERT_PRED], sum, 20, 128);
        s2 = c[HOR_PRED] + c[DIAG_DOWN_RIGHT_PRED] + c[VERT_RIGHT_PRED];
        sum -= s2;
        adapt_prob(&pp[3], s2, sum, 20, 128);
        s2 -= c[HOR_PRED];
        adapt_prob(&pp[4], c[HOR_PRED], s2, 20, 128);
        adapt_prob(&pp[5], c[DIAG_DOWN_RIGHT_PRED], c[VERT_RIGHT_PRED], 20, 128);
        sum -= c[DIAG_DOWN_LEFT_PRED];
        adapt_prob(&pp[6], c[DIAG_DOWN_LEFT_PRED], sum, 20, 128);
        sum -= c[VERT_LEFT_PRED];
        adapt_prob(&pp[7], c[VERT_LEFT_PRED], sum, 20, 128);
        adapt_prob(&pp[8], c[HOR_DOWN_PRED], c[HOR_UP_PRED], 20, 128);
    }

    // uv intra modes
    for (i = 0; i < 10; i++) {
        RK_U8 *pp = p->uv_mode[i];
        RK_U32 *c = s->counts.uv_mode[i], sum, s2;

        sum = c[0] + c[1] + c[3] + c[4] + c[5] + c[6] + c[7] + c[8] + c[9];
        adapt_prob(&pp[0], c[DC_PRED], sum, 20, 128);
        sum -= c[TM_VP8_PRED];
        adapt_prob(&pp[1], c[TM_VP8_PRED], sum, 20, 128);
        sum -= c[VERT_PRED];
        adapt_prob(&pp[2], c[VERT_PRED], sum, 20, 128);
        s2 = c[HOR_PRED] + c[DIAG_DOWN_RIGHT_PRED] + c[VERT_RIGHT_PRED];
        sum -= s2;
        adapt_prob(&pp[3], s2, sum, 20, 128);
        s2 -= c[HOR_PRED];
        adapt_prob(&pp[4], c[HOR_PRED], s2, 20, 128);
        adapt_prob(&pp[5], c[DIAG_DOWN_RIGHT_PRED], c[VERT_RIGHT_PRED], 20, 128);
        sum -= c[DIAG_DOWN_LEFT_PRED];
        adapt_prob(&pp[6], c[DIAG_DOWN_LEFT_PRED], sum, 20, 128);
        sum -= c[VERT_LEFT_PRED];
        adapt_prob(&pp[7], c[VERT_LEFT_PRED], sum, 20, 128);
        adapt_prob(&pp[8], c[HOR_DOWN_PRED], c[HOR_UP_PRED], 20, 128);
    }
#if 0 //def dump
    fwrite(s->counts.y_mode, 1, sizeof(s->counts.y_mode), vp9_p_fp1);
    fwrite(s->counts.uv_mode, 1, sizeof(s->counts.uv_mode), vp9_p_fp1);
    fflush(vp9_p_fp1);
#endif
}


RK_S32 vp9_parser_frame(Vp9CodecContext *ctx, HalDecTask *task)
{

    const RK_U8 *data = NULL;
    RK_S32 size = 0;
    VP9Context *s = (VP9Context *)ctx->priv_data;
    RK_S32 res, i, ref = 0;

    vp9d_dbg(VP9D_DBG_FUNCTION, "%s", __FUNCTION__);
    task->valid = -1;
#ifdef dump
    dec_num++;
#endif
    data = (const RK_U8 *)mpp_packet_get_pos(ctx->pkt);
    size = (RK_S32)mpp_packet_get_length(ctx->pkt);

    s->pts = mpp_packet_get_pts(ctx->pkt);

    vp9d_dbg(VP9D_DBG_HEADER, "data size %d", size);
    if (size <= 0) {
        return MPP_OK;
    }
    if ((res = decode_parser_header(ctx, data, size, &ref)) < 0) {
        return res;
    } else if (res == 0) {
        if (!s->refs[ref].ref) {
            //mpp_err("Requested reference %d not available\n", ref);
            return -1;//AVERROR_INVALIDDATA;
        }

        mpp_buf_slot_set_flag(s->slots, s->refs[ref].slot_index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(s->slots, s->refs[ref].slot_index, QUEUE_DISPLAY);
        mpp_log("repeat frame");
        //    if ((res = vp9_ref_frame(frame, s->refs[ref].f)) < 0)
        // return res;
#if 0
        ((AVFrame *)frame)->pkt_pts = 0;//pkt->pts;
        ((AVFrame *)frame)->pkt_dts = 0;//pkt->dts;
#endif
        mpp_log("out repeat num %d", s->outframe_num++);
        return size;//pkt->size;
    }
    data += res;
    size -= res;

    if (s->frames[REF_FRAME_MVPAIR].ref)
        vp9_unref_frame(s, &s->frames[REF_FRAME_MVPAIR]);

    if (!s->intraonly && !s->keyframe && !s->errorres && s->frames[CUR_FRAME].ref) {
        if ((res = vp9_ref_frame(ctx, &s->frames[REF_FRAME_MVPAIR], &s->frames[CUR_FRAME])) < 0)
            return res;
    }

    if (s->frames[CUR_FRAME].ref)
        vp9_unref_frame(s, &s->frames[CUR_FRAME]);

    if ((res = vp9_alloc_frame(ctx, &s->frames[CUR_FRAME])) < 0)
        return res;

    if (s->refreshctx && s->parallelmode) {
        RK_S32 j, k, l, m;

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 2; k++)
                    for (l = 0; l < 6; l++)
                        for (m = 0; m < 6; m++)
                            memcpy(s->prob_ctx[s->framectxid].coef[i][j][k][l][m],
                                   s->prob.coef[i][j][k][l][m], 3);
            if ((RK_S32)s->txfmmode == i)
                break;
        }
        s->prob_ctx[s->framectxid].p = s->prob.p;
    }

    vp9d_parser2_syntax(ctx);

    task->syntax.data = (void*)&ctx->pic_params;
    task->syntax.number = 1;
    task->valid = 1;
    task->output = s->frames[CUR_FRAME].slot_index;
    task->input_packet = ctx->pkt;

    for (i = 0; i < 3; i++) {
        if (s->refs[s->refidx[i]].slot_index < 0x7f) {
            MppFrame mframe = NULL;
            mpp_buf_slot_set_flag(s->slots, s->refs[s->refidx[i]].slot_index, SLOT_HAL_INPUT);
            task->refer[i] = s->refs[s->refidx[i]].slot_index;
            mpp_buf_slot_get_prop(s->slots, task->refer[i], SLOT_FRAME_PTR, &mframe);
            if (mframe)
                task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
        } else {
            task->refer[i] = -1;
        }
    }
    vp9d_dbg(VP9D_DBG_REF, "ref_errinfo=%d\n", task->flags.ref_err);
    if (s->eos) {
        task->flags.eos = 1;
    }

    if (!s->invisible) {
        mpp_buf_slot_set_flag(s->slots,  s->frames[CUR_FRAME].slot_index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(s->slots, s->frames[CUR_FRAME].slot_index, QUEUE_DISPLAY);
    }
    vp9d_dbg(VP9D_DBG_REF, "s->refreshrefmask = %d s->frames[CUR_FRAME] = %d",
             s->refreshrefmask, s->frames[CUR_FRAME].slot_index);
    for (i = 0; i < 3; i++) {
        if (s->refs[s->refidx[i]].ref != NULL) {
            vp9d_dbg(VP9D_DBG_REF, "ref buf select %d", s->refs[s->refidx[i]].slot_index);
        }
    }
    // ref frame setup
    for (i = 0; i < 8; i++) {
        vp9d_dbg(VP9D_DBG_REF, "s->refreshrefmask = 0x%x", s->refreshrefmask);
        res = 0;
        if (s->refreshrefmask & (1 << i)) {
            if (s->refs[i].ref)
                vp9_unref_frame(s, &s->refs[i]);
            vp9d_dbg(VP9D_DBG_REF, "update ref index in %d", i);
            res = vp9_ref_frame(ctx, &s->refs[i], &s->frames[CUR_FRAME]);
        }

        if (s->refs[i].ref)
            vp9d_dbg(VP9D_DBG_REF, "s->refs[%d] = %d", i, s->refs[i].slot_index);
        if (res < 0)
            return 0;
    }
    return 0;
}

MPP_RET vp9d_paser_reset(Vp9CodecContext *ctx)
{
    RK_S32 i;
    VP9Context *s = ctx->priv_data;
    SplitContext_t *ps = (SplitContext_t *)ctx->priv_data2;
    VP9ParseContext *pc = (VP9ParseContext *)ps->priv_data;

    s->got_keyframes = 0;
    for (i = 0; i < 3; i++) {
        if (s->frames[i].ref) {
            vp9_unref_frame(s, &s->frames[i]);
        }
    }
    for (i = 0; i < 8; i++) {
        if (s->refs[i].ref) {
            vp9_unref_frame(s, &s->refs[i]);
        }
    }
    memset(pc, 0, sizeof(VP9ParseContext));

    s->eos = 0;
    if (ps) {
        ps->eos = 0;
    }
    return MPP_OK;
}
static void inv_count_data(VP9Context *s)
{
    RK_U32 partition_probs[4][4][4];
    RK_U32 count_uv[10][10];
    RK_U32 count_y_mode[4][10];
    RK_U32 *dst_uv = NULL;
    RK_S32 i, j;

    /*
                 syntax              hardware
             *+++++64x64+++++*   *++++8x8++++*
             *+++++32x32+++*     *++++16x16++++*
             *+++++16x16+++*     *++++32x32++++*
             *+++++8x8+++*       *++++64x64++++++*
     */

    memcpy(&partition_probs, s->counts.partition, sizeof(s->counts.partition));
    j = 0;
    for (i = 3; i >= 0; i--) {
        memcpy(&s->counts.partition[j], &partition_probs[i], 64);
        j++;
    }
    if (!(s->keyframe || s->intraonly)) {
        memcpy(count_y_mode, s->counts.y_mode, sizeof(s->counts.y_mode));
        for (i = 0; i < 4; i++) {
            RK_U32 value = 0;
            for (j = 0; j < 10; j++) {
                value = count_y_mode[i][j];
                if (j == 0)
                    s->counts.y_mode[i][2] = value;
                else if (j == 1)
                    s->counts.y_mode[i][0] = value;
                else if (j == 2)
                    s->counts.y_mode[i][1] = value;
                else if (j == 7)
                    s->counts.y_mode[i][8] = value;
                else if (j == 8)
                    s->counts.y_mode[i][7] = value;
                else
                    s->counts.y_mode[i][j] = value;

            }
        }


        memcpy(count_uv, s->counts.uv_mode, sizeof(s->counts.uv_mode));

        /*change uv_mode to hardware need style*/
        /*
              syntax              hardware
         *+++++ v   ++++*     *++++ dc   ++++*
         *+++++ h   ++++*     *++++ v   ++++*
         *+++++ dc  ++++*     *++++ h  ++++*
         *+++++ d45 ++++*     *++++ d45 ++++*
         *+++++ d135++++*     *++++ d135++++*
         *+++++ d117++++*     *++++ d117++++*
         *+++++ d153++++*     *++++ d153++++*
         *+++++ d63 ++++*     *++++ d207++++*
         *+++++ d207 ++++*    *++++ d63 ++++*
         *+++++ tm  ++++*     *++++ tm  ++++*
        */
        for (i = 0; i < 10; i++) {
            RK_U32 *src_uv = (RK_U32 *)(count_uv[i]);
            RK_U32 value = 0;
            if (i == 0) {
                dst_uv = s->counts.uv_mode[2]; //dc
            } else if ( i == 1) {
                dst_uv = s->counts.uv_mode[0]; //h
            }  else if ( i == 2) {
                dst_uv = s->counts.uv_mode[1]; //h
            }  else if ( i == 7) {
                dst_uv = s->counts.uv_mode[8]; //d207
            } else if (i == 8) {
                dst_uv = s->counts.uv_mode[7]; //d63
            } else {
                dst_uv = s->counts.uv_mode[i];
            }
            for (j = 0; j < 10; j++) {
                value = src_uv[j];
                if (j == 0)
                    dst_uv[2] = value;
                else if (j == 1)
                    dst_uv[0] = value;
                else if (j == 2)
                    dst_uv[1] = value;
                else if (j == 7)
                    dst_uv[8] = value;
                else if (j == 8)
                    dst_uv[7] = value;
                else
                    dst_uv[j] = value;
            }

        }
    }
}

void vp9_parser_update(Vp9CodecContext *ctx, void *count_info)
{
    VP9Context *s = ctx->priv_data;

#ifdef dump
    char filename[20] = "data/pcout";
    char filename1[20] = "data/uppor";
    if (vp9_p_fp != NULL) {
        fclose(vp9_p_fp);

    }
    if (vp9_p_fp1 != NULL) {
        fclose(vp9_p_fp1);

    }
    sprintf(&filename[10], "%d.bin", count);
    sprintf(&filename1[10], "%d.bin", count);
    mpp_log("filename %s", filename);
    vp9_p_fp = fopen(filename, "wb");
    vp9_p_fp1 = fopen(filename1, "wb");
#endif
    //update count from hardware
    if (count_info != NULL) {

        memcpy((void *)&s->counts, count_info, sizeof(s->counts));

        if (s->refreshctx && !s->parallelmode) {
#ifdef dump
            count++;
#endif
            inv_count_data(s);
            adapt_probs(s);

        }
    }

    return;
}
