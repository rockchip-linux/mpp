/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#include <stdlib.h>

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_bitread.h"
#include "mpp_packet_impl.h"
#include "mpp_compat_impl.h"

#include "vp9d_ctx.h"
#include "vp9d.h"
#include "mpp_frame_impl.h"

//#define dump
#ifdef dump
static FILE *vp9_p_fp = NULL;
static FILE *vp9_p_fp1 = NULL;
static FILE *vp9_p_fp2 = NULL;
static RK_S32 dec_num = 0;
static RK_S32 count = 0;
#endif

static void split_parse_frame(VP9SplitCtx *ctx, RK_U8 *buf, RK_S32 size)
{
    VP9ParseCtx *s = (VP9ParseCtx *)ctx->priv_data;

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

RK_S32 vp9d_split_frame(VP9SplitCtx *ctx, RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 size)
{
    VP9ParseCtx *s = (VP9ParseCtx *)ctx->priv_data;
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

        return s->n_frames > 0 ? *out_size : size;
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

MPP_RET vp9d_get_frame_stream(Vp9DecCtx *ctx, RK_U8 *buf, RK_S32 length)
{
    RK_U8 *data = (RK_U8 *)mpp_packet_get_data(ctx->pkt);
    RK_S32 size = (RK_S32)mpp_packet_get_size(ctx->pkt);
    RK_S32 buff_size = 0;

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

MPP_RET vp9d_split_init(Vp9DecCtx *vp9_ctx)
{
    VP9SplitCtx *ps;
    VP9ParseCtx *sc;

    ps = (VP9SplitCtx *)mpp_calloc(VP9SplitCtx, 1);
    if (!ps) {
        mpp_err("vp9 parser malloc fail");
        return MPP_ERR_NOMEM;
    }

    sc = (VP9ParseCtx *)mpp_calloc(VP9ParseCtx, 1);
    if (!sc) {
        mpp_err("vp9 parser context malloc fail");
        mpp_free(ps);
        return MPP_ERR_NOMEM;
    }

    ps->priv_data = (void*)sc;
    vp9_ctx->priv_data2 = (void*)ps;

    return MPP_OK;
}

MPP_RET vp9d_split_deinit(Vp9DecCtx *vp9_ctx)
{
    VP9SplitCtx *ps = (VP9SplitCtx *)vp9_ctx->priv_data2;

    if (ps)
        MPP_FREE(ps->priv_data);
    MPP_FREE(vp9_ctx->priv_data2);

    return MPP_OK;
}

static RK_S32 vp9_ref_frame(Vp9DecCtx *ctx, VP9Frame *dst, VP9Frame *src)
{
    VP9Context *s = ctx->priv_data;
    MppFrameImpl *impl_frm = (MppFrameImpl *)dst->f;

    if (src->ref == NULL || src->slot_index >= 0x7f) {
        mpp_err("vp9_ref_frame is vaild");
        return -1;
    }
    dst->slot_index = src->slot_index;
    dst->ref = src->ref;
    dst->ref->show_frame_flag = src->ref->show_frame_flag;
    dst->ref->ref_count++;
    vp9d_dbg(VP9D_DBG_REF, "get prop slot frame %p  count %d", dst->f, dst->ref->ref_count);
    mpp_buf_slot_get_prop(s->slots, src->slot_index, SLOT_FRAME, &dst->f);
    impl_frm->buffer = NULL; //parser no need process hal buf
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
            if (f->ref->show_frame_flag && !f->ref->is_output) {
                MppBuffer framebuf = NULL;

                mpp_buf_slot_get_prop(s->slots, f->slot_index, SLOT_BUFFER, &framebuf);
                mpp_buffer_put(framebuf);
                f->ref->show_frame_flag = 0;
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

static RK_S32 vp9_frame_free(VP9Context *s)
{
    RK_S32 i;

    for (i = 0; i < 3; i++) {
        if (s->frames[i].ref) {
            vp9_unref_frame(s, &s->frames[i]);
        }
        mpp_frame_deinit(&s->frames[i].f);
    }
    for (i = 0; i < 8; i++) {
        if (s->refs[i].ref) {
            vp9_unref_frame(s, &s->refs[i]);
        }
        mpp_frame_deinit(&s->refs[i].f);
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

MPP_RET vp9d_parser_init(Vp9DecCtx *vp9_ctx, ParserCfg *init)
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
    s->cfg = init->cfg;
    s->hw_info = init->hw_info;
    mpp_buf_slot_setup(s->slots, 25);

    mpp_env_get_u32("vp9d_debug", &vp9d_debug, 0);

    return MPP_OK;
}

MPP_RET vp9d_parser_deinit(Vp9DecCtx *vp9_ctx)
{
    VP9Context *s = vp9_ctx->priv_data;

    vp9_frame_free(s);
    mpp_free(s->c_b);
    s->c_b_size = 0;
    MPP_FREE(vp9_ctx->priv_data);
    return MPP_OK;
}

static RK_S32 vp9_alloc_frame(Vp9DecCtx *ctx, VP9Frame *frame)
{
    VP9Context *s = ctx->priv_data;

    mpp_frame_set_width(frame->f, ctx->width);
    mpp_frame_set_height(frame->f, ctx->height);

    mpp_frame_set_hor_stride(frame->f, 0);
    mpp_frame_set_ver_stride(frame->f, 0);
    mpp_frame_set_errinfo(frame->f, 0);
    mpp_frame_set_discard(frame->f, 0);
    mpp_frame_set_pts(frame->f, s->pts);
    mpp_frame_set_dts(frame->f, s->dts);
    // set current poc
    s->cur_poc++;
    mpp_frame_set_poc(frame->f, s->cur_poc);

    if (MPP_FRAME_FMT_IS_FBC(s->cfg->base.out_fmt)) {
        RK_U32 fbc_hdr_stride = mpp_align_64(ctx->width);

        mpp_slots_set_prop(s->slots, SLOTS_HOR_ALIGN, mpp_align_64);
        mpp_frame_set_fmt(frame->f, ctx->pix_fmt | ((s->cfg->base.out_fmt & (MPP_FRAME_FBC_MASK))));

        if (*compat_ext_fbc_hdr_256_odd)
            fbc_hdr_stride = mpp_align_256_odd(ctx->width);

        mpp_frame_set_fbc_hdr_stride(frame->f, fbc_hdr_stride);
    } else {
        if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3576)
            mpp_slots_set_prop(s->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
        else
            mpp_slots_set_prop(s->slots, SLOTS_HOR_ALIGN, mpp_align_256_odd);
        mpp_slots_set_prop(s->slots, SLOTS_VER_ALIGN, mpp_align_64);
        if (MPP_FRAME_FMT_IS_TILE(s->cfg->base.out_fmt))
            mpp_frame_set_fmt(frame->f, ctx->pix_fmt | ((s->cfg->base.out_fmt & (MPP_FRAME_TILE_FLAG))));
        else
            mpp_frame_set_fmt(frame->f, ctx->pix_fmt);
    }

    if (s->cfg->base.enable_thumbnail && s->hw_info->cap_down_scale)
        mpp_frame_set_thumbnail_en(frame->f, s->cfg->base.enable_thumbnail);
    else
        mpp_frame_set_thumbnail_en(frame->f, 0);

    mpp_buf_slot_get_unused(s->slots, &frame->slot_index);
    mpp_buf_slot_set_prop(s->slots, frame->slot_index, SLOT_FRAME, frame->f);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_HAL_OUTPUT);
    frame->ref = mpp_calloc(VP9RefInfo, 1);
    frame->ref->ref_count++;
    frame->ref->show_frame_flag = s->show_frame_flag;
    frame->ref->is_output = 0;

    return 0;
}

static MPP_RET update_size(Vp9DecCtx *ctx, RK_S32 w, RK_S32 h, RK_S32 fmt)
{
    VP9Context *s = ctx->priv_data;

    if (w == ctx->width && h == ctx->height && ctx->pix_fmt == fmt)
        return MPP_OK;

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

    return MPP_OK;
}

static inline RK_S32 read_synccode(BitReadCtx_t *gb)
{
    RK_S32 code0, code1, code2;

    READ_BITS(gb, 8, &code0); // sync code0
    READ_BITS(gb, 8, &code1); // sync code1
    READ_BITS(gb, 8, &code2); // sync code2
    return ((code0 == 0x49) && (code1 == 0x83) && (code2 == 0x42));
__BITREAD_ERR:
    return 0;
}

static const MppFrameColorSpace colorspaces_table[8] = {
    MPP_FRAME_SPC_UNSPECIFIED, MPP_FRAME_SPC_BT470BG, MPP_FRAME_SPC_BT709, MPP_FRAME_SPC_SMPTE170M,
    MPP_FRAME_SPC_SMPTE240M, MPP_FRAME_SPC_BT2020_NCL, MPP_FRAME_SPC_RESERVED, MPP_FRAME_SPC_RGB,
};
static MPP_RET read_uncompressed_header(Vp9DecCtx *ctx, const RK_U8 *data, RK_S32 size,
                                        RK_S32 *refo)
{
    VP9Context *s = ctx->priv_data;
    RK_S32 fmt = ctx->pix_fmt;
    BitReadCtx_t *gb = &s->gb;
    RK_S32 i;
    RK_U32 w, h;
    RK_S32 val;

    /* general header */
    mpp_set_bitread_ctx(gb, (RK_U8*)data, size);

#define VP9_FRAME_MARKER 0x2
    READ_BITS(gb, 2, &val); // frame marker
    if (val != VP9_FRAME_MARKER) {
        mpp_err("Invalid frame marker\n");
        return MPP_ERR_STREAM;
    }
    // read profile
    READ_ONEBIT(gb, &val);
    ctx->profile = val;
    READ_ONEBIT(gb, &val);
    ctx->profile |= val << 1;
    if (ctx->profile > 2) {
        READ_ONEBIT(gb, &val);
        ctx->profile += val;
    }
    if (ctx->profile != VP9_PROFILE_0 && ctx->profile != VP9_PROFILE_2) {
        mpp_err("Profile %d is not yet supported\n", ctx->profile);
        return MPP_ERR_STREAM;
    }
    vp9d_dbg(VP9D_DBG_HEADER, "profile %d", ctx->profile);

    READ_ONEBIT(gb, &s->show_existing_frame);
    vp9d_dbg(VP9D_DBG_HEADER, "show_existing_frame %d", s->show_existing_frame);
    if (s->show_existing_frame) {
        READ_BITS(gb, 3, refo);
        vp9d_dbg(VP9D_DBG_HEADER, "frame_to_show %d", *refo);
        return 0;
    }

    s->last_keyframe = s->keyframe;
    READ_ONEBIT(gb, &val);
    s->keyframe = !val;
    vp9d_dbg(VP9D_DBG_HEADER, "keyframe %d", s->keyframe);
    RK_S32 last_invisible = s->show_frame_flag;
    READ_ONEBIT(gb, &val);
    s->show_frame_flag = !val;
    vp9d_dbg(VP9D_DBG_HEADER, "show_frame_flag %d", s->show_frame_flag);
    READ_ONEBIT(gb, &s->error_res_mode);
    vp9d_dbg(VP9D_DBG_HEADER, "error_resilient_mode %d", s->error_res_mode);
    s->use_last_frame_mvs = !s->error_res_mode && !last_invisible;
    s->got_keyframes += s->keyframe ? 1 : 0;
    vp9d_dbg(VP9D_DBG_HEADER, "keyframe=%d, intraonly=%d, got_keyframes=%d\n",
             s->keyframe, s->intraonly, s->got_keyframes);

    if (!s->got_keyframes) {
        mpp_err_f("have not got keyframe.\n");
        return MPP_ERR_STREAM;
    }

    /* set mvscale default */
    for (i = 0; i < 3; i++) {
        s->mvscale[i][0] = 16;
        s->mvscale[i][1] = 16;
    }

    if (s->keyframe) {
        if (!read_synccode(gb)) {
            mpp_err("Invalid sync code\n");
            return MPP_ERR_STREAM;
        }
        // read bitdepth colorspace sampling
        {
            RK_S32 bits = 0; // 0:8, 1:10, 2:12
            if (ctx->profile == VP9_PROFILE_2) {
                READ_ONEBIT(gb, &bits);
                bits += 1;
            }
            fmt = bits ? MPP_FMT_YUV420SP_10BIT : MPP_FMT_YUV420SP;
            vp9d_dbg(VP9D_DBG_HEADER, "bit_depth %d", 8 + bits * 2);
            s->bpp_index = bits;
            s->bpp = 8 + bits * 2;
            s->bytesperpixel = (7 + s->bpp) >> 3;
            READ_BITS(gb, 3, &val); // color_space
            ctx->colorspace = colorspaces_table[val];
            vp9d_dbg(VP9D_DBG_HEADER, "color_space %d", ctx->colorspace);
            if (ctx->colorspace == MPP_FRAME_SPC_RGB) { // RGB = profile 1 VP9_CS_SRGB
                mpp_err("RGB not supported in profile %d\n", ctx->profile);
                return MPP_ERR_STREAM;
            } else { // profile 0/2
                READ_ONEBIT(gb, &val);
                ctx->color_range = val ? MPP_FRAME_RANGE_JPEG : MPP_FRAME_RANGE_MPEG;
                vp9d_dbg(VP9D_DBG_HEADER, "color_range %d", ctx->color_range);
                s->extra_plane = 0;
                s->subsampling_x = s->subsampling_y = 1;
            }
        }

        s->refresh_frame_flags = 0xff;
        READ_BITS(gb, 16, &w);
        w += 1;
        vp9d_dbg(VP9D_DBG_HEADER, "frame_size_width %d", w);
        READ_BITS(gb, 16, &h);
        h += 1;
        vp9d_dbg(VP9D_DBG_HEADER, "frame_size_height %d", h);
        // display size
        READ_ONEBIT(gb, &val);
        if (val) {
            RK_S32 dw, dh;
            vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 1);
            READ_BITS(gb, 16, &dw);
            dw += 1;
            vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dw);
            READ_BITS(gb, 16, &dh);
            dh += 1;
            vp9d_dbg(VP9D_DBG_HEADER, "display_size_height %d", dh);
        } else
            vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", 0);
    } else {
        s->intraonly = 0;
        if (s->show_frame_flag)
            READ_ONEBIT(gb, &s->intraonly);
        vp9d_dbg(VP9D_DBG_HEADER, "intra_only %d", s->intraonly);
        s->resetctx = 0;
        if (!s->error_res_mode) {
            READ_BITS(gb, 2, &s->resetctx);
        }
        vp9d_dbg(VP9D_DBG_HEADER, "reset_frame_context_value %d", s->resetctx);
        if (s->intraonly) {
            if (!read_synccode(gb)) { // synccode
                mpp_err("Invalid sync code\n");
                return MPP_ERR_STREAM;
            }
            s->subsampling_x = 1;
            s->subsampling_y = 1;
            s->bpp = 8;
            s->bpp_index = 0;
            s->bytesperpixel = 1;
            fmt = MPP_FMT_YUV420SP;
            ctx->colorspace = MPP_FRAME_SPC_BT470BG;
            ctx->color_range = MPP_FRAME_RANGE_JPEG;

            READ_BITS(gb, 8, &s->refresh_frame_flags);
            vp9d_dbg(VP9D_DBG_HEADER, "refresh_frame_flags %d", s->refresh_frame_flags);
            READ_BITS(gb, 16, &w);
            w += 1;
            vp9d_dbg(VP9D_DBG_HEADER, "frame_size_width %d", w);
            READ_BITS(gb, 16, &h);
            h += 1;
            vp9d_dbg(VP9D_DBG_HEADER, "frame_size_height %d", h);
            // display size
            READ_ONEBIT(gb, &val);
            vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", val);
            if (val) {
                RK_S32 dw, dh;
                READ_BITS(gb, 16, &dw);
                dw += 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dw);
                READ_BITS(gb, 16, &dh);
                dh += 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_height %d", dh);
            }
        } else {
            READ_BITS(gb, 8, &s->refresh_frame_flags);
            vp9d_dbg(VP9D_DBG_HEADER, "refresh_frame_flags %d", s->refresh_frame_flags);
            for (i = 0; i < 3; i++) {
                READ_BITS(gb, 3, &s->refidx[i]);
                READ_ONEBIT(gb, &s->signbias[i]);
                s->signbias[i] = s->signbias[i] && !s->error_res_mode;
                vp9d_dbg(VP9D_DBG_HEADER, "[%d] refidx %d, signbias %d", i, s->refidx[i], s->signbias[i]);
                if (!s->refs[s->refidx[i]].ref)
                    mpp_err("refidx %d not available\n", s->refidx[i]);
            }
            // setup frame size with refs
            RK_S32 found = 0;
            for (i = 0; i < 3; i++) {
                READ_ONEBIT(gb, &val);
                vp9d_dbg(VP9D_DBG_HEADER, "ref_flag %d", i);
                if (val) {
                    w = mpp_frame_get_width(s->refs[s->refidx[i]].f);
                    h = mpp_frame_get_height(s->refs[s->refidx[i]].f);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                READ_BITS(gb, 16, &w);
                w += 1;
                vp9d_dbg(VP9D_DBG_HEADER, "frame_size_width %d", w);
                READ_BITS(gb, 16, &h);
                h += 1;
                vp9d_dbg(VP9D_DBG_HEADER, "frame_size_height %d", h);
            }

            if (w == 0 || h == 0) {
                mpp_err("ref frame w:%d h:%d\n", w, h);
                return MPP_ERR_STREAM;
            }
            //  set use_last_frame_mvs
            s->use_last_frame_mvs &= mpp_frame_get_width(s->frames[VP9_CUR_FRAME].f) == w &&
                                     mpp_frame_get_height(s->frames[VP9_CUR_FRAME].f) == h;
            // display size
            READ_ONEBIT(gb, &val);
            vp9d_dbg(VP9D_DBG_HEADER, "display_info_flag %d", val);
            if (val) {
                RK_S32 dw, dh;
                READ_BITS(gb, 16, &dw);
                dw += 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_width %d", dw);
                READ_BITS(gb, 16, &dh);
                dh += 1;
                vp9d_dbg(VP9D_DBG_HEADER, "display_size_height %d", dh);
            }
            READ_ONEBIT(gb, &s->allow_high_precision_mv);
            vp9d_dbg(VP9D_DBG_HEADER, "allow_high_precision_mv %d", s->allow_high_precision_mv);
            // read interp filter
            READ_ONEBIT(gb, &val);
            if (val) {
                s->interp_filter = VP9_FILTER_SWITCHABLE;
            } else {
                READ_BITS(gb, 2, &s->interp_filter);
            }
            vp9d_dbg(VP9D_DBG_HEADER, "interp_filter %d", s->interp_filter);
            for (i = 0; i < 3; i++) {
                RK_U32 refw = mpp_frame_get_width(s->refs[s->refidx[i]].f);
                RK_U32 refh = mpp_frame_get_height(s->refs[s->refidx[i]].f);
                RK_S32 reffmt = mpp_frame_get_fmt(s->refs[s->refidx[i]].f) & MPP_FRAME_FMT_MASK;

                vp9d_dbg(VP9D_DBG_REF, "ref get width frame slot %p", s->refs[s->refidx[i]].f);
                if (reffmt != fmt) {
                    /* TODO */

                } else if (refw == w && refh == h) {
                    s->mvscale[i][0] = (refw << 14) / w;
                    s->mvscale[i][1] = (refh << 14) / h;
                } else {
                    if (w * 2 < refw || h * 2 < refh ||
                        w > 16 * refw || h > 16 * refh) {
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

    if (s->error_res_mode) {
        s->refresh_frame_context = 0;
        s->frame_parallel_mode = 1;
    } else {
        READ_ONEBIT(gb, &s->refresh_frame_context);
        READ_ONEBIT(gb, &s->frame_parallel_mode);
    }
    vp9d_dbg(VP9D_DBG_HEADER, "refresh_frame_context_flag %d", s->refresh_frame_context);
    vp9d_dbg(VP9D_DBG_HEADER, "frame_parallel_decoding_mode %d", s->frame_parallel_mode);

    READ_BITS(gb, 2, &s->frame_context_idx);
    vp9d_dbg(VP9D_DBG_HEADER, "frame_context_idx %d", s->frame_context_idx);

    // set default loopfilter deltas
    if (s->keyframe || s->intraonly || s->error_res_mode) {
        s->lf_delta.enabled = 1;
        s->lf_delta.update = 1;
        s->lf_delta.ref[0] = 1;
        s->lf_delta.ref[1] = 0;
        s->lf_delta.ref[2] = -1;
        s->lf_delta.ref[3] = -1;
        s->lf_delta.mode[0] = 0;
        s->lf_delta.mode[1] = 0;
    }
    // set loopfilter deltas
    READ_BITS(gb, 6, &s->filter.level);
    vp9d_dbg(VP9D_DBG_HEADER, "filter_level %d", s->filter.level);
    {
        RK_S32 sharpness_level;
        READ_BITS(gb, 3, &sharpness_level);
        vp9d_dbg(VP9D_DBG_HEADER, "sharpness_level %d", sharpness_level);
        // update sharpness
        if (s->filter.sharpness != sharpness_level)
            memset(s->filter.lim_lut, 0, sizeof(s->filter.lim_lut));
        s->filter.sharpness = sharpness_level;
    }
    READ_ONEBIT(gb, &s->lf_delta.enabled);
    if (s->lf_delta.enabled) {
        vp9d_dbg(VP9D_DBG_HEADER, "mode_ref_delta_enabled=1");
        READ_ONEBIT(gb, &s->lf_delta.update);
        if (s->lf_delta.update) {
            vp9d_dbg(VP9D_DBG_HEADER, "mode_ref_delta_update=1");
            for (i = 0; i < 4; i++) {
                READ_ONEBIT(gb, &val);
                if (val)
                    READ_SIGNBITS(gb, 6, &s->lf_delta.ref[i]);
                vp9d_dbg(VP9D_DBG_HEADER, "ref_deltas %d", s->lf_delta.ref[i]);
            }
            for (i = 0; i < 2; i++) {
                READ_ONEBIT(gb, &val);
                if (val)
                    READ_SIGNBITS(gb, 6, &s->lf_delta.mode[i]);
                vp9d_dbg(VP9D_DBG_HEADER, "mode_deltas %d", s->lf_delta.mode[i]);
            }
        }
    }

    // read quantization header data
    READ_BITS(gb, 8, &s->base_qindex);
    vp9d_dbg(VP9D_DBG_HEADER, "base_qindex %d", s->base_qindex);
    s->y_dc_delta_q = 0;
    READ_ONEBIT(gb, &val);
    if (val)
        READ_SIGNBITS(gb, 4, &s->y_dc_delta_q);
    vp9d_dbg(VP9D_DBG_HEADER, "y_dc_delta_q %d", s->y_dc_delta_q);
    s->uv_dc_delta_q = 0;
    READ_ONEBIT(gb, &val);
    if (val)
        READ_SIGNBITS(gb, 4, &s->uv_dc_delta_q);
    vp9d_dbg(VP9D_DBG_HEADER, "uv_dc_delta_q %d", s->uv_dc_delta_q);
    s->uv_ac_delta_q = 0;
    READ_ONEBIT(gb, &val);
    if (val)
        READ_SIGNBITS(gb, 4, &s->uv_ac_delta_q);
    vp9d_dbg(VP9D_DBG_HEADER, "uv_ac_delta_q %d", s->uv_ac_delta_q);
    s->lossless = s->base_qindex == 0 && s->y_dc_delta_q == 0 &&
                  s->uv_dc_delta_q == 0 && s->uv_ac_delta_q == 0;

    // Segmentation map update
    s->segmentation.update_map = 0;
    s->segmentation.ignore_refmap = 0;
    READ_ONEBIT(gb, &s->segmentation.enabled);
    vp9d_dbg(VP9D_DBG_HEADER, "segmentation_enabled %d", s->segmentation.enabled);
    if (s->segmentation.enabled) {
        READ_ONEBIT(gb, &s->segmentation.update_map);
        vp9d_dbg(VP9D_DBG_HEADER, "update_map %d", s->segmentation.update_map);
        if (s->segmentation.update_map) {
            for (i = 0; i < 7; i++) {
                s->prob.seg[i] = 255;
                READ_ONEBIT(gb, &val);
                if (val)
                    READ_BITS(gb, 8, &s->prob.seg[i]);
                vp9d_dbg(VP9D_DBG_HEADER, "tree_probs %d value 0x%x", i, s->prob.seg[i]);
            }
            READ_ONEBIT(gb, &s->segmentation.temporal);
            vp9d_dbg(VP9D_DBG_HEADER, "tempora_update %d", s->segmentation.temporal);
            if (s->segmentation.temporal) {
                for (i = 0; i < 3; i++) {
                    s->prob.segpred[i] = 255;
                    READ_ONEBIT(gb, &val);
                    if (val)
                        READ_BITS(gb, 8, &s->prob.segpred[i]);
                    vp9d_dbg(VP9D_DBG_HEADER, "pred_probs %d value 0x%x", i, s->prob.segpred[i]);
                }
            } else {
                for (i = 0; i < 3; i++)
                    s->prob.segpred[i] = 0xff;
            }
        }
        if ((!s->segmentation.update_map || s->segmentation.temporal) &&
            (w != mpp_frame_get_width(s->frames[VP9_CUR_FRAME].f) ||
             h != mpp_frame_get_height(s->frames[VP9_CUR_FRAME].f))) {

            /* TODO */
            //return -1;
        }
        // segmentation data update
        READ_ONEBIT(gb, &val);
        vp9d_dbg(VP9D_DBG_HEADER, "update_data %d", val);
        if (val) {
            READ_ONEBIT(gb, &s->segmentation.absolute_vals);
            vp9d_dbg(VP9D_DBG_HEADER, "abs_delta %d", s->segmentation.absolute_vals);
            for (i = 0; i < 8; i++) {
                READ_ONEBIT(gb, &s->segmentation.feat[i].q_enabled);
                if (s->segmentation.feat[i].q_enabled)
                    READ_SIGNBITS(gb, 8, &s->segmentation.feat[i].q_val);
                vp9d_dbg(VP9D_DBG_HEADER, "q_enabled %d frame_qp_delta %d",
                         s->segmentation.feat[i].q_enabled, s->segmentation.feat[i].q_val);
                READ_ONEBIT(gb, &s->segmentation.feat[i].lf_enabled);
                if (s->segmentation.feat[i].lf_enabled)
                    READ_SIGNBITS(gb, 6, &s->segmentation.feat[i].lf_val);
                vp9d_dbg(VP9D_DBG_HEADER, "lf_enabled %d frame_loopfilter_value %d",
                         s->segmentation.feat[i].lf_enabled, s->segmentation.feat[i].lf_val);
                READ_ONEBIT(gb, &s->segmentation.feat[i].ref_enabled);
                if (s->segmentation.feat[i].ref_enabled)
                    READ_BITS(gb, 2, &s->segmentation.feat[i].ref_val);
                vp9d_dbg(VP9D_DBG_HEADER, "ref_enabled %d frame_reference_info %d",
                         s->segmentation.feat[i].ref_enabled, s->segmentation.feat[i].ref_val);
                READ_ONEBIT(gb, &s->segmentation.feat[i].skip_enabled);
                vp9d_dbg(VP9D_DBG_HEADER, "skip_enabled %d", s->segmentation.feat[i].skip_enabled);
            }
        }
    } else {
        s->segmentation.feat[0].q_enabled    = 0;
        s->segmentation.feat[0].lf_enabled   = 0;
        s->segmentation.feat[0].ref_enabled  = 0;
        s->segmentation.feat[0].skip_enabled = 0;
    }

    // Build y/uv dequant values based on segmentation
    for (i = 0; i < (s->segmentation.enabled ? 8 : 1); i++) {
        RK_S32 y_ac_q, y_dc_q, uv_ac_q, uv_dc_q;

        if (s->segmentation.feat[i].q_enabled) {
            if (s->segmentation.absolute_vals)
                y_ac_q = s->segmentation.feat[i].q_val;
            else
                y_ac_q = s->base_qindex + s->segmentation.feat[i].q_val;
        } else {
            y_ac_q  = s->base_qindex;
        }
        y_dc_q  = mpp_clip_uint_pow2(y_ac_q + s->y_dc_delta_q, 8);
        uv_dc_q = mpp_clip_uint_pow2(y_ac_q + s->uv_dc_delta_q, 8);
        uv_ac_q = mpp_clip_uint_pow2(y_ac_q + s->uv_ac_delta_q, 8);
        y_ac_q  = mpp_clip_uint_pow2(y_ac_q, 8);

        s->segmentation.feat[i].qmul[0][0] = vp9_dc_qlookup[s->bpp_index][y_dc_q];
        s->segmentation.feat[i].qmul[0][1] = vp9_ac_qlookup[s->bpp_index][y_ac_q];
        s->segmentation.feat[i].qmul[1][0] = vp9_dc_qlookup[s->bpp_index][uv_dc_q];
        s->segmentation.feat[i].qmul[1][1] = vp9_ac_qlookup[s->bpp_index][uv_ac_q];
    }

    // update size
    update_size(ctx, w, h, fmt);

    // get min log2 tile cols
    for (s->tiling.log2_tile_cols = 0;
         (s->sb_cols >> s->tiling.log2_tile_cols) > 64;
         s->tiling.log2_tile_cols++) ;
    // get max log2 tile cols
    RK_S32 max_log2 = 0;
    for (max_log2 = 0; (s->sb_cols >> max_log2) >= 4; max_log2++) ;
    max_log2 = MPP_MAX(0, max_log2 - 1);
    while ((RK_U32)max_log2 > s->tiling.log2_tile_cols) {
        RK_S32 log2_tile_col_end_flag;
        READ_ONEBIT(gb, &log2_tile_col_end_flag);
        vp9d_dbg(VP9D_DBG_HEADER, "log2_tile_col_end_flag %d", log2_tile_col_end_flag);
        if (log2_tile_col_end_flag) {
            s->tiling.log2_tile_cols++;
        } else {
            break;
        }
    }
    // read tile_rows
    READ_ONEBIT(gb, &s->tiling.log2_tile_rows);
    if (s->tiling.log2_tile_rows) {
        READ_ONEBIT(gb, &val);
        s->tiling.log2_tile_rows += val;
    }
    vp9d_dbg(VP9D_DBG_HEADER, "log2_tile_rows %d", s->tiling.log2_tile_rows);

    s->tiling.tile_rows = 1 << s->tiling.log2_tile_rows;
    if (s->tiling.tile_cols != (1U << s->tiling.log2_tile_cols)) {
        s->tiling.tile_cols = 1 << s->tiling.log2_tile_cols;
        {
            RK_U32 min_size = sizeof(Vp9dReader) * s->tiling.tile_cols;
            if (min_size > s->c_b_size) {
                s->c_b = (Vp9dReader *)mpp_malloc(RK_U8, min_size);
                s->c_b_size = min_size;
            }
        }
        if (!s->c_b) {
            mpp_err("Ran out of memory during range coder init\n");
            return MPP_ERR_NOMEM;
        }
    }
    // set frame context
    RK_S32 cidx = s->frame_context_idx;
    if (s->keyframe || s->error_res_mode || (s->intraonly && s->resetctx == 3)) {
        for (i = 0; i < 4; i++) {
            s->prob_ctx[i].p = vp9_default_probs;
            memcpy(s->prob_ctx[i].coef, vp9_default_coef_probs, sizeof(vp9_default_coef_probs));
        }
    } else if (s->intraonly && s->resetctx == 2) {
        s->prob_ctx[cidx].p = vp9_default_probs;
        memcpy(s->prob_ctx[cidx].coef, vp9_default_coef_probs, sizeof(vp9_default_coef_probs));
    }
    if (s->keyframe || s->error_res_mode || s->intraonly) // reset context
        s->frame_context_idx = 0;

    // next 16 bits is size of the rest of the header (arith-coded)
    READ_BITS(gb, 16, &s->first_partition_size);
    vp9d_dbg(VP9D_DBG_HEADER, "first_partition_size %d", s->first_partition_size);

    const RK_U8 *aligned_data = mpp_align_get_bits(gb);
    vp9d_dbg(VP9D_DBG_HEADER, "offset %d", aligned_data - data);
    s->uncompress_head_size_in_byte = aligned_data - data;
    vp9d_dbg(VP9D_DBG_HEADER, "uncompress_head_size_in_byte %d", s->uncompress_head_size_in_byte);
    if (s->first_partition_size > size - s->uncompress_head_size_in_byte) {
        mpp_err("invalid: compressed_header_size %d, size %d, uncompress_head_size_in_byte %d \n",
                s->first_partition_size, size, s->uncompress_head_size_in_byte);
        return MPP_ERR_STREAM;
    }
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET read_compressed_header(VP9Context *s, const RK_U8 *aligned_data,
                                      RK_S32 first_partition_size)
{
    RK_S32 c, i, j, k, l, m, n;

    vp9d_reader_init(&s->c, aligned_data, first_partition_size);

    if (vp9d_read(&s->c, 128)) { // marker bit
        mpp_err("Marker bit was set\n");
        return MPP_ERR_STREAM;
    }

    if (s->keyframe || s->intraonly) {
        memset(s->counts.coef, 0, sizeof(s->counts.coef));
        memset(s->counts.eob,  0, sizeof(s->counts.eob));
    } else {
        memset(&s->counts, 0, sizeof(s->counts));
    }

    // set prob context
    c = s->frame_context_idx;
    s->prob.p = s->prob_ctx[c].p;
    memset(&s->prob_flag_delta, 0, sizeof(s->prob_flag_delta));
    // txfm updates
    if (s->lossless) {
        s->txfmmode = VP9_TX_4X4;
    } else {
        s->txfmmode = vp9d_read_bits(&s->c, 2);
        if (s->txfmmode == VP9_TX_32X32)
            s->txfmmode += vp9d_read_bit(&s->c);

        if (s->txfmmode == VP9_TX_SWITCHABLE) {
            for (i = 0; i < 2; i++) {

                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.tx8p[i] = 1;
                    s->prob.p.tx8p[i] = vp9d_diff_update_prob(&s->c, s->prob.p.tx8p[i],
                                                              &s->prob_flag_delta.p_delta.tx8p[i]);
                }

            }
            for (i = 0; i < 2; i++)
                for (j = 0; j < 2; j++) {

                    if (vp9d_read(&s->c, 252)) {
                        s->prob_flag_delta.p_flag.tx16p[i][j] = 1;
                        s->prob.p.tx16p[i][j] =
                            vp9d_diff_update_prob(&s->c, s->prob.p.tx16p[i][j],
                                                  &s->prob_flag_delta.p_delta.tx16p[i][j]);
                    }
                }
            for (i = 0; i < 2; i++)
                for (j = 0; j < 3; j++) {

                    if (vp9d_read(&s->c, 252)) {
                        s->prob_flag_delta.p_flag.tx32p[i][j] = 1;
                        s->prob.p.tx32p[i][j] =
                            vp9d_diff_update_prob(&s->c, s->prob.p.tx32p[i][j],
                                                  &s->prob_flag_delta.p_delta.tx32p[i][j]);
                    }
                }
        }
    }

    // coef updates
    for (i = 0; i < 4; i++) {
        RK_U8 (*ref)[2][6][6][3] = s->prob_ctx[c].coef[i];

        if (vp9d_read_bit(&s->c)) {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 2; k++)
                    for (l = 0; l < 6; l++)
                        for (m = 0; m < 6; m++) {
                            RK_U8 *p = s->prob.coef[i][j][k][l][m];
                            RK_U8 *p_flag = s->prob_flag_delta.coef_flag[i][j][k][l][m];
                            RK_U8 *p_delta = s->prob_flag_delta.coef_delta[i][j][k][l][m];
                            RK_U8 *r = ref[j][k][l][m];
                            if (l == 0 && m >= 3) // dc only has 3 pt
                                break;
                            for (n = 0; n < 3; n++) {
                                if (vp9d_read(&s->c, 252)) {
                                    p_flag[n] = 1;
                                    p[n] = vp9d_diff_update_prob(&s->c, r[n], &p_delta[n]);
                                } else {
                                    p_flag[n] = 0;
                                    p[n] = r[n];
                                }
                            }
                        }
        } else {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 2; k++)
                    for (l = 0; l < 6; l++)
                        for (m = 0; m < 6; m++) {
                            RK_U8 *p = s->prob.coef[i][j][k][l][m];
                            RK_U8 *r = ref[j][k][l][m];
                            if (m >= 3 && l == 0) // dc only has 3 pt
                                break;
                            memcpy(p, r, 3);
                        }
        }
        if (s->txfmmode == (RK_U32)i)
            break;
    }

    // mode updates
    for (i = 0; i < 3; i++) {
        if (vp9d_read(&s->c, 252)) {
            s->prob_flag_delta.p_flag.skip[i] = 1;
            s->prob.p.skip[i] = vp9d_diff_update_prob(&s->c, s->prob.p.skip[i],
                                                      &s->prob_flag_delta.p_delta.skip[i]);
        }
    }

    if (!s->keyframe && !s->intraonly) {
        for (i = 0; i < 7; i++)
            for (j = 0; j < 3; j++) {
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.mv_mode[i][j] = 1;
                    s->prob.p.mv_mode[i][j] =
                        vp9d_diff_update_prob(&s->c, s->prob.p.mv_mode[i][j],
                                              &s->prob_flag_delta.p_delta.mv_mode[i][j]);
                }
            }

        if (s->interp_filter == VP9_FILTER_SWITCHABLE)
            for (i = 0; i < 4; i++)
                for (j = 0; j < 2; j++) {
                    if (vp9d_read(&s->c, 252)) {
                        s->prob_flag_delta.p_flag.filter[i][j] = 1;
                        s->prob.p.filter[i][j] =
                            vp9d_diff_update_prob(&s->c, s->prob.p.filter[i][j],
                                                  &s->prob_flag_delta.p_delta.filter[i][j]);
                    }
                }

        for (i = 0; i < 4; i++) {
            if (vp9d_read(&s->c, 252)) {
                s->prob_flag_delta.p_flag.intra[i] = 1;
                s->prob.p.intra[i] = vp9d_diff_update_prob(&s->c, s->prob.p.intra[i],
                                                           &s->prob_flag_delta.p_delta.intra[i]);
            }
        }
        s->allowcompinter = (s->signbias[0] != s->signbias[1] ||
                             s->signbias[0] != s->signbias[2]);
        if (s->allowcompinter) {
            s->comppredmode = vp9d_read_bit(&s->c);
            if (s->comppredmode)
                s->comppredmode += vp9d_read_bit(&s->c);
            if (s->comppredmode == VP9_PRED_SWITCHABLE)
                for (i = 0; i < 5; i++) {
                    if (vp9d_read(&s->c, 252)) {
                        s->prob_flag_delta.p_flag.comp[i] = 1;
                        s->prob.p.comp[i] =
                            vp9d_diff_update_prob(&s->c, s->prob.p.comp[i],
                                                  &s->prob_flag_delta.p_delta.comp[i]);
                    }
                }
        } else {
            s->comppredmode = VP9_PRED_SINGLEREF;
        }

        if (s->comppredmode != VP9_PRED_COMPREF) {
            for (i = 0; i < 5; i++) {
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.single_ref[i][0] = 1;
                    s->prob.p.single_ref[i][0] =
                        vp9d_diff_update_prob(&s->c, s->prob.p.single_ref[i][0],
                                              &s->prob_flag_delta.p_delta.single_ref[i][0]);
                }
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.single_ref[i][1] = 1;
                    s->prob.p.single_ref[i][1] =
                        vp9d_diff_update_prob(&s->c, s->prob.p.single_ref[i][1],
                                              &s->prob_flag_delta.p_delta.single_ref[i][1]);
                }
            }
        }

        if (s->comppredmode != VP9_PRED_SINGLEREF) {
            for (i = 0; i < 5; i++) {
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.comp_ref[i] = 1;
                    s->prob.p.comp_ref[i] =
                        vp9d_diff_update_prob(&s->c, s->prob.p.comp_ref[i],
                                              &s->prob_flag_delta.p_delta.comp_ref[i]);
                }
            }
        }

        for (i = 0; i < 4; i++)
            for (j = 0; j < 9; j++) {
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.y_mode[i][j] = 1;
                    s->prob.p.y_mode[i][j] =
                        vp9d_diff_update_prob(&s->c, s->prob.p.y_mode[i][j],
                                              &s->prob_flag_delta.p_delta.y_mode[i][j]);
                }
            }

        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                for (k = 0; k < 3; k++) {
                    if (vp9d_read(&s->c, 252)) {
                        s->prob_flag_delta.p_flag.partition[3 - i][j][k] = 1;
                        s->prob.p.partition[3 - i][j][k] =
                            vp9d_diff_update_prob(&s->c, s->prob.p.partition[3 - i][j][k],
                                                  &s->prob_flag_delta.p_delta.partition[3 - i][j][k]);
                    }
                }

        for (i = 0; i < 3; i++) {
            if (vp9d_read(&s->c, 252)) {
                s->prob_flag_delta.p_flag.mv_joint[i]   = 1;
                s->prob_flag_delta.p_delta.mv_joint[i]  =
                    s->prob.p.mv_joint[i]   = (vp9d_read_bits(&s->c, 7) << 1) | 1;
            }
        }

        for (i = 0; i < 2; i++) {
            if (vp9d_read(&s->c, 252)) {
                s->prob_flag_delta.p_flag.mv_comp[i].sign   = 1;
                s->prob_flag_delta.p_delta.mv_comp[i].sign  =
                    s->prob.p.mv_comp[i].sign   = (vp9d_read_bits(&s->c, 7) << 1) | 1;
            }

            for (j = 0; j < 10; j++)
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.mv_comp[i].classes[j]  = 1;
                    s->prob_flag_delta.p_delta.mv_comp[i].classes[j] =
                        s->prob.p.mv_comp[i].classes[j]  = (vp9d_read_bits(&s->c, 7) << 1) | 1;
                }

            if (vp9d_read(&s->c, 252)) {
                s->prob_flag_delta.p_flag.mv_comp[i].class0  = 1;
                s->prob_flag_delta.p_delta.mv_comp[i].class0 =
                    s->prob.p.mv_comp[i].class0  = (vp9d_read_bits(&s->c, 7) << 1) | 1;
            }

            for (j = 0; j < 10; j++)
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.mv_comp[i].bits[j]  = 1;
                    s->prob_flag_delta.p_delta.mv_comp[i].bits[j] =
                        s->prob.p.mv_comp[i].bits[j]  = (vp9d_read_bits(&s->c, 7) << 1) | 1;
                }
        }

        for (i = 0; i < 2; i++) {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 3; k++)
                    if (vp9d_read(&s->c, 252)) {
                        s->prob_flag_delta.p_flag.mv_comp[i].class0_fp[j][k]  = 1;
                        s->prob_flag_delta.p_delta.mv_comp[i].class0_fp[j][k] =
                            s->prob.p.mv_comp[i].class0_fp[j][k]  = (vp9d_read_bits(&s->c, 7) << 1) | 1;
                    }

            for (j = 0; j < 3; j++)
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.mv_comp[i].fp[j]  = 1;
                    s->prob_flag_delta.p_delta.mv_comp[i].fp[j] =
                        s->prob.p.mv_comp[i].fp[j]  =
                            (vp9d_read_bits(&s->c, 7) << 1) | 1;
                }
        }

        if (s->allow_high_precision_mv) {
            for (i = 0; i < 2; i++) {
                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.mv_comp[i].class0_hp  = 1;
                    s->prob_flag_delta.p_delta.mv_comp[i].class0_hp =
                        s->prob.p.mv_comp[i].class0_hp  = (vp9d_read_bits(&s->c, 7) << 1) | 1;
                }

                if (vp9d_read(&s->c, 252)) {
                    s->prob_flag_delta.p_flag.mv_comp[i].hp  = 1;
                    s->prob_flag_delta.p_delta.mv_comp[i].hp =
                        s->prob.p.mv_comp[i].hp  = (vp9d_read_bits(&s->c, 7) << 1) | 1;
                }
            }
        }
    }
    return MPP_OK;
}

static MPP_RET decode_parser_header(Vp9DecCtx *ctx,
                                    const RK_U8 *data, RK_S32 size)
{
    RK_S32 ref = 0;
    MPP_RET ret = MPP_NOK;
    VP9Context *s = ctx->priv_data;

#ifdef dump
    char filename[20] = "data/acoef";
    if (vp9_p_fp2 != NULL) {
        fclose(vp9_p_fp2);

    }
    sprintf(&filename[10], "%d.bin", dec_num);
    vp9_p_fp2 = fopen(filename, "wb");
#endif

    ret = read_uncompressed_header(ctx, data, size, &ref);
    if (ret < 0) {
        mpp_err("read uncompressed header failed\n");
        return ret;
    }

    if (s->show_existing_frame) {
        MppFrame frame = NULL;

        if (!s->refs[ref].ref) {
            //mpp_err("Requested reference %d not available\n", ref);
            return MPP_ERR_NULL_PTR;
        }
        mpp_buf_slot_get_prop(s->slots, s->refs[ref].slot_index, SLOT_FRAME_PTR, &frame);
        mpp_frame_set_pts(frame, s->pts);
        mpp_frame_set_dts(frame, s->dts);
        mpp_buf_slot_set_flag(s->slots, s->refs[ref].slot_index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(s->slots, s->refs[ref].slot_index, QUEUE_DISPLAY);
        s->refs[ref].ref->is_output = 1;
        vp9d_dbg(VP9D_DBG_HEADER, "out repeat num %d", s->outframe_num++);

        return MPP_OK;
    } else {
        const RK_U8 *aligned_data = mpp_align_get_bits(&s->gb);
        ret = read_compressed_header(s, aligned_data, s->first_partition_size);
        if (ret < 0) {
            mpp_err("read compressed header failed\n");
            return ret;
        }
    }

    return MPP_OK;
}

#define TRANS_TO_HW_STYLE(uv_mode)                          \
do{                                                         \
    RK_U8 *uv_ptr = NULL;                                   \
    RK_U8 uv_mode_prob[10][9];                              \
    for (i = 0; i < 10; i++) {                              \
        if (i == 0) {                                       \
            uv_ptr = uv_mode[2];                            \
        } else if ( i == 1) {                               \
            uv_ptr = uv_mode[0];                            \
        }  else if ( i == 2) {                              \
            uv_ptr = uv_mode[1];                            \
        }  else if ( i == 7) {                              \
            uv_ptr = uv_mode[8];                            \
        } else if (i == 8) {                                \
            uv_ptr = uv_mode[7];                            \
        } else {                                            \
            uv_ptr = uv_mode[i];                            \
        }                                                   \
        memcpy(&uv_mode_prob[i], uv_ptr, 9);                \
    }                                                       \
    memcpy(uv_mode, uv_mode_prob, sizeof(uv_mode_prob));    \
}while(0)

static RK_S32 vp9d_fill_segmentation(VP9Context *s, DXVA_segmentation_VP9 *seg)
{
    RK_S32 i;

    seg->enabled = s->segmentation.enabled;
    seg->update_map = s->segmentation.update_map;
    seg->temporal_update = s->segmentation.temporal;
    seg->abs_delta = s->segmentation.absolute_vals;
    seg->ReservedSegmentFlags4Bits = 0;

    for (i = 0; i < 7; i++) {
        seg->tree_probs[i] = s->prob.seg[i];
    }

    seg->pred_probs[0] = s->prob.segpred[0];
    seg->pred_probs[1] = s->prob.segpred[1];
    seg->pred_probs[2] = s->prob.segpred[2];

    for (i = 0; i < 8; i++) {
        seg->feature_data[i][0] = s->segmentation.feat[i].q_val;
        seg->feature_data[i][1] = s->segmentation.feat[i].lf_val;
        seg->feature_data[i][2] = s->segmentation.feat[i].ref_val;
        seg->feature_data[i][3] = s->segmentation.feat[i].skip_enabled;
        seg->feature_mask[i] = s->segmentation.feat[i].q_enabled
                               | (s->segmentation.feat[i].lf_enabled << 1)
                               | (s->segmentation.feat[i].ref_enabled << 2)
                               | (s->segmentation.feat[i].skip_enabled << 3);
#if 0
        mpp_log("seg->feature_data[%d][0] = 0x%x", i, seg->feature_data[i][0]);

        mpp_log("seg->feature_data[%d][1] = 0x%x", i, seg->feature_data[i][0]);

        mpp_log("seg->feature_data[%d][2] = 0x%x", i, seg->feature_data[i][0]);

        mpp_log("seg->feature_data[%d][3] = 0x%x", i, seg->feature_data[i][0]);
        mpp_log("seg->feature_mask[%d] = 0x%x", i, seg->feature_mask[i]);
#endif
    }

    return 0;
}

static RK_S32 vp9d_fill_picparams(Vp9DecCtx *ctx, DXVA_PicParams_VP9 *pic)
{
    VP9Context *s = ctx->priv_data;
    RK_U8 partition_probs[16][3];
    RK_U8 partition_probs_flag[16][3];
    RK_U8 partition_probs_delata[16][3];
    DXVA_prob_vp9* prob_flag = &pic->prob_flag_delta.p_flag;
    DXVA_prob_vp9* prob_delta = &pic->prob_flag_delta.p_delta;
    RK_S32 i;

    pic->profile = ctx->profile;
    pic->show_existing_frame = s->show_existing_frame;
    pic->frame_type = !s->keyframe;
    pic->show_frame = !s->show_frame_flag;
    pic->error_resilient_mode =  s->error_res_mode;
    pic->subsampling_x = s->subsampling_x;
    pic->subsampling_y = s->subsampling_y;
    pic->extra_plane = s->extra_plane;
    pic->refresh_frame_context = s->refresh_frame_context;
    pic->intra_only = s->intraonly;
    pic->frame_context_idx = s->frame_context_idx;
    pic->reset_frame_context = s->resetctx;
    pic->allow_high_precision_mv = s->allow_high_precision_mv;
    pic->parallelmode = s->frame_parallel_mode;
    pic->width = ctx->width;
    pic->height = ctx->height;
    pic->BitDepthMinus8Luma = s->bpp - 8;
    pic->BitDepthMinus8Chroma = s->bpp - 8;
    pic->interp_filter = s->interp_filter;
    pic->CurrPic.Index7Bits = s->frames[VP9_CUR_FRAME].slot_index;

    for (i = 0; i < 8; i++) {
        pic->ref_frame_map[i].Index7Bits = s->refs[i].slot_index;
        pic->ref_frame_coded_width[i] = mpp_frame_get_width(s->refs[i].f);
        pic->ref_frame_coded_height[i] = mpp_frame_get_height(s->refs[i].f);
    }
    pic->frame_refs[0].Index7Bits =  s->refidx[0];
    pic->frame_refs[1].Index7Bits =  s->refidx[1];
    pic->frame_refs[2].Index7Bits =  s->refidx[2];
    pic->ref_frame_sign_bias[1] = s->signbias[0];
    pic->ref_frame_sign_bias[2] = s->signbias[1];
    pic->ref_frame_sign_bias[3] = s->signbias[2];
    pic->filter_level = s->filter.level;
    pic->sharpness_level = s->filter.sharpness;
    pic->mode_ref_delta_enabled = s->lf_delta.enabled;
    pic->mode_ref_delta_update = s->lf_delta.update;
    pic->use_prev_in_find_mv_refs = s->use_last_frame_mvs;
    pic->ref_deltas[0] = s->lf_delta.ref[0];
    pic->ref_deltas[1] = s->lf_delta.ref[1];
    pic->ref_deltas[2] = s->lf_delta.ref[2];
    pic->ref_deltas[3] = s->lf_delta.ref[3];
    pic->mode_deltas[0] = s->lf_delta.mode[0];
    pic->mode_deltas[1] = s->lf_delta.mode[1];
    pic->base_qindex = s->base_qindex;
    pic->y_dc_delta_q = s->y_dc_delta_q;
    pic->uv_dc_delta_q = s->uv_dc_delta_q;
    pic->uv_ac_delta_q = s->uv_ac_delta_q;
    pic->txmode = s->txfmmode;
    pic->refmode = s->comppredmode;
    vp9d_fill_segmentation(s, &pic->stVP9Segments);
    pic->log2_tile_cols = s->tiling.log2_tile_cols;
    pic->log2_tile_rows = s->tiling.log2_tile_rows;
    pic->first_partition_size = s->first_partition_size;
    pic->uncompressed_header_size_byte_aligned = s->uncompress_head_size_in_byte;
    memcpy(pic->mvscale, s->mvscale, sizeof(s->mvscale));
    memcpy(&pic->prob, &s->prob, sizeof(pic->prob));
    memcpy(&pic->prob_flag_delta, &s->prob_flag_delta, sizeof(pic->prob_flag_delta));
    {
        /*change partition to hardware need style*/
        /*
              hardware            syntax
          *+++++8x8+++++*     *++++64x64++++*
          *+++++16x16+++*     *++++32x32++++*
          *+++++32x32+++*     *++++16x16++++*
          *+++++64x64+++*     *++++8x8++++++*
        */
        RK_U32 m = 0;
        RK_U32 len = sizeof(pic->prob.partition[0]);
        RK_U32 step = len / sizeof(partition_probs[0]);

        for (i = MPP_ARRAY_ELEMS(pic->prob.partition) - 1; i >= 0; i--) {
            memcpy(&partition_probs[m][0], &pic->prob.partition[i][0][0], len);
            memcpy(&partition_probs_flag[m][0], &prob_flag->partition[i][0][0], len);
            memcpy(&partition_probs_delata[m][0], &prob_delta->partition[i][0][0], len);
            m += step;
        }
        memcpy(pic->prob.partition, partition_probs, sizeof(partition_probs));
        memcpy(prob_flag->partition, partition_probs_flag, sizeof(partition_probs_flag));
        memcpy(prob_delta->partition, partition_probs_delata, sizeof(partition_probs_delata));

        /*change uv_mode to hardware need style*/
        /*
            hardware              syntax
         *+++++ dc  ++++*     *++++ v   ++++*
         *+++++ v   ++++*     *++++ h   ++++*
         *+++++ h   ++++*     *++++ dc  ++++*
         *+++++ d45 ++++*     *++++ d45 ++++*
         *+++++ d135++++*     *++++ d135++++*
         *+++++ d117++++*     *++++ d117++++*
         *+++++ d153++++*     *++++ d153++++*
         *+++++ d207++++*     *++++ d63 ++++*
         *+++++ d63 ++++*     *++++ d207++++*
         *+++++ tm  ++++*     *++++ tm  ++++*
        */

        TRANS_TO_HW_STYLE(pic->prob.uv_mode);
        TRANS_TO_HW_STYLE(prob_flag->uv_mode);
        TRANS_TO_HW_STYLE(prob_delta->uv_mode);
    }
    return 0;
}

RK_S32 vp9_parser_frame(Vp9DecCtx *ctx, HalDecTask *task)
{
    const RK_U8 *data = NULL;
    RK_S32 size = 0;
    VP9Context *s = (VP9Context *)ctx->priv_data;
    RK_S32 ret;
    RK_S32 i;

    vp9d_dbg(VP9D_DBG_FUNCTION, "%s", __FUNCTION__);
    task->valid = -1;
#ifdef dump
    dec_num++;
#endif
    data = (const RK_U8 *)mpp_packet_get_pos(ctx->pkt);
    size = (RK_S32)mpp_packet_get_length(ctx->pkt);

    s->pts = mpp_packet_get_pts(ctx->pkt);
    s->dts = mpp_packet_get_dts(ctx->pkt);

    vp9d_dbg(VP9D_DBG_HEADER, "data size %d", size);
    if (size <= 0) {
        return MPP_OK;
    }
    ret = decode_parser_header(ctx, data, size);
    if (ret < 0)
        return ret;

    if (s->show_existing_frame) {
        return size;
    } else {
        RK_S32 head_size = s->first_partition_size + (RK_S32)s->uncompress_head_size_in_byte;
        vp9d_dbg(VP9D_DBG_HEADER, "%s end, head_size %d\n", __FUNCTION__, head_size);
        // move to frame data
        data += head_size;
        size -= head_size;
    }

    if (s->frames[VP9_REF_FRAME_MVPAIR].ref)
        vp9_unref_frame(s, &s->frames[VP9_REF_FRAME_MVPAIR]);

    if (!s->intraonly && !s->keyframe && !s->error_res_mode && s->frames[VP9_CUR_FRAME].ref) {
        ret = vp9_ref_frame(ctx, &s->frames[VP9_REF_FRAME_MVPAIR], &s->frames[VP9_CUR_FRAME]);
        if (ret < 0)
            return ret;
    }

    if (s->frames[VP9_CUR_FRAME].ref)
        vp9_unref_frame(s, &s->frames[VP9_CUR_FRAME]);

    ret = vp9_alloc_frame(ctx, &s->frames[VP9_CUR_FRAME]);
    if (ret < 0)
        return ret;

    // if context need refresh, copy prob_ctx
    if (s->refresh_frame_context && s->frame_parallel_mode) {
        RK_S32 j, k, l, m;

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 2; k++)
                    for (l = 0; l < 6; l++)
                        for (m = 0; m < 6; m++)
                            memcpy(s->prob_ctx[s->frame_context_idx].coef[i][j][k][l][m],
                                   s->prob.coef[i][j][k][l][m], 3);
            if ((RK_S32)s->txfmmode == i)
                break;
        }
        s->prob_ctx[s->frame_context_idx].p = s->prob.p;
    }

    vp9d_fill_picparams(ctx, &ctx->pic_params);
    // set task
    task->syntax.data = (void*)&ctx->pic_params;
    task->syntax.number = 1;
    task->valid = 1;
    task->output = s->frames[VP9_CUR_FRAME].slot_index;
    task->input_packet = ctx->pkt;

    for (i = 0; i < 3; i++) {
        if (s->refs[s->refidx[i]].slot_index < 0x7f) {
            MppFrame mframe = NULL;

            mpp_buf_slot_set_flag(s->slots, s->refs[s->refidx[i]].slot_index, SLOT_HAL_INPUT);
            task->refer[i] = s->refs[s->refidx[i]].slot_index;
            mpp_buf_slot_get_prop(s->slots, task->refer[i], SLOT_FRAME_PTR, &mframe);
            if (mframe && !s->keyframe && !s->intraonly)
                task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
        } else {
            task->refer[i] = -1;
        }
    }

    vp9d_dbg(VP9D_DBG_REF, "ref_errinfo=%d\n", task->flags.ref_err);
    if (s->eos) {
        task->flags.eos = 1;
    }

    if (!s->show_frame_flag) {
        mpp_buf_slot_set_flag(s->slots,  s->frames[VP9_CUR_FRAME].slot_index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(s->slots, s->frames[VP9_CUR_FRAME].slot_index, QUEUE_DISPLAY);
    }
    vp9d_dbg(VP9D_DBG_REF, "s->refresh_frame_flags = %d s->frames[VP9_CUR_FRAME] = %d",
             s->refresh_frame_flags, s->frames[VP9_CUR_FRAME].slot_index);
    for (i = 0; i < 3; i++) {
        if (s->refs[s->refidx[i]].ref != NULL) {
            vp9d_dbg(VP9D_DBG_REF, "ref buf select %d", s->refs[s->refidx[i]].slot_index);
        }
    }
    // ref frame setup
    for (i = 0; i < 8; i++) {
        vp9d_dbg(VP9D_DBG_REF, "s->refresh_frame_flags = 0x%x", s->refresh_frame_flags);
        ret = 0;
        if (s->refresh_frame_flags & (1 << i)) {
            if (s->refs[i].ref)
                vp9_unref_frame(s, &s->refs[i]);
            vp9d_dbg(VP9D_DBG_REF, "update ref index in %d", i);
            ret = vp9_ref_frame(ctx, &s->refs[i], &s->frames[VP9_CUR_FRAME]);
        }

        if (s->refs[i].ref)
            vp9d_dbg(VP9D_DBG_REF, "s->refs[%d] = %d", i, s->refs[i].slot_index);
        if (ret < 0)
            return 0/*ret*/;
    }
    return 0;
}

MPP_RET vp9d_paser_reset(Vp9DecCtx *ctx)
{
    VP9Context *s = ctx->priv_data;
    VP9SplitCtx *ps = (VP9SplitCtx *)ctx->priv_data2;
    VP9ParseCtx *pc = (VP9ParseCtx *)ps->priv_data;
    RK_S32 i;

    s->got_keyframes = 0;
    s->cur_poc = 0;
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
    memset(pc, 0, sizeof(VP9ParseCtx));

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

static void adapt_coef_probs(VP9Context *s)
{
    Vp9ProbCtx *p = &s->prob_ctx[s->frame_context_idx].p;
    RK_S32 uf = (s->keyframe || s->intraonly || !s->last_keyframe) ? 112 : 128;
    RK_S32 i, j, k, l, m;

    // coefficients
    for (i = 0; i < 4; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 2; k++)
                for (l = 0; l < 6; l++)
                    for (m = 0; m < 6; m++) {
                        RK_U8 *pp = s->prob_ctx[s->frame_context_idx].coef[i][j][k][l][m];
                        RK_U32 *e = s->counts.eob[i][j][k][l][m];
                        RK_U32 *c = s->counts.coef[i][j][k][l][m];

                        if (l == 0 && m >= 3) // dc only has 3 pt
                            break;
                        vp9d_merge_prob(&pp[0], e[0], e[1], 24, uf);
                        vp9d_merge_prob(&pp[1], c[0], c[1] + c[2], 24, uf);
                        vp9d_merge_prob(&pp[2], c[1], c[2], 24, uf);
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
        vp9d_merge_prob(&p->skip[i], s->counts.skip[i][0], s->counts.skip[i][1], 20, 128);

    // intra/inter flag
    for (i = 0; i < 4; i++)
        vp9d_merge_prob(&p->intra[i], s->counts.intra[i][0], s->counts.intra[i][1], 20, 128);

    // comppred flag
    if (s->comppredmode == VP9_PRED_SWITCHABLE) {
        for (i = 0; i < 5; i++)
            vp9d_merge_prob(&p->comp[i], s->counts.comp[i][0], s->counts.comp[i][1], 20, 128);
    }

    // reference frames
    if (s->comppredmode != VP9_PRED_SINGLEREF) {
        for (i = 0; i < 5; i++)
            vp9d_merge_prob(&p->comp_ref[i], s->counts.comp_ref[i][0],
                            s->counts.comp_ref[i][1], 20, 128);
    }

    if (s->comppredmode != VP9_PRED_COMPREF) {
        for (i = 0; i < 5; i++) {
            RK_U8 *pp = p->single_ref[i];
            RK_U32 (*c)[2] = s->counts.single_ref[i];

            vp9d_merge_prob(&pp[0], c[0][0], c[0][1], 20, 128);
            vp9d_merge_prob(&pp[1], c[1][0], c[1][1], 20, 128);
        }
    }

    // block partitioning
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            RK_U8 *pp = p->partition[i][j];
            RK_U32 *c = s->counts.partition[i][j];

            vp9d_merge_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
            vp9d_merge_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
            vp9d_merge_prob(&pp[2], c[2], c[3], 20, 128);
        }

    // tx size
    if (s->txfmmode == VP9_TX_SWITCHABLE) {
        for (i = 0; i < 2; i++) {
            RK_U32 *c16 = s->counts.tx16p[i], *c32 = s->counts.tx32p[i];

            vp9d_merge_prob(&p->tx8p[i], s->counts.tx8p[i][0], s->counts.tx8p[i][1], 20, 128);
            vp9d_merge_prob(&p->tx16p[i][0], c16[0], c16[1] + c16[2], 20, 128);
            vp9d_merge_prob(&p->tx16p[i][1], c16[1], c16[2], 20, 128);
            vp9d_merge_prob(&p->tx32p[i][0], c32[0], c32[1] + c32[2] + c32[3], 20, 128);
            vp9d_merge_prob(&p->tx32p[i][1], c32[1], c32[2] + c32[3], 20, 128);
            vp9d_merge_prob(&p->tx32p[i][2], c32[2], c32[3], 20, 128);
        }
    }

    // interpolation filter
    if (s->interp_filter == VP9_FILTER_SWITCHABLE) {
        for (i = 0; i < 4; i++) {
            RK_U8 *pp = p->filter[i];
            RK_U32 *c = s->counts.filter[i];

            vp9d_merge_prob(&pp[0], c[0], c[1] + c[2], 20, 128);
            vp9d_merge_prob(&pp[1], c[1], c[2], 20, 128);
        }
    }

    // inter modes
    for (i = 0; i < 7; i++) {
        RK_U8 *pp = p->mv_mode[i];
        RK_U32 *c = s->counts.mv_mode[i];

        vp9d_merge_prob(&pp[0], c[2], c[1] + c[0] + c[3], 20, 128);
        vp9d_merge_prob(&pp[1], c[0], c[1] + c[3], 20, 128);
        vp9d_merge_prob(&pp[2], c[1], c[3], 20, 128);
    }

    // mv joints
    {
        RK_U8 *pp = p->mv_joint;
        RK_U32 *c = s->counts.mv_joint;

        vp9d_merge_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
        vp9d_merge_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
        vp9d_merge_prob(&pp[2], c[2], c[3], 20, 128);
    }

    // mv components
    for (i = 0; i < 2; i++) {
        RK_U8 *pp;
        RK_U32 *c, (*c2)[2], sum;

        vp9d_merge_prob(&p->mv_comp[i].sign, s->counts.sign[i][0],
                        s->counts.sign[i][1], 20, 128);

        pp = p->mv_comp[i].classes;
        c = s->counts.classes[i];
        sum = c[1] + c[2] + c[3] + c[4] + c[5] + c[6] + c[7] + c[8] + c[9] + c[10];
        vp9d_merge_prob(&pp[0], c[0], sum, 20, 128);
        sum -= c[1];
        vp9d_merge_prob(&pp[1], c[1], sum, 20, 128);
        sum -= c[2] + c[3];
        vp9d_merge_prob(&pp[2], c[2] + c[3], sum, 20, 128);
        vp9d_merge_prob(&pp[3], c[2], c[3], 20, 128);
        sum -= c[4] + c[5];
        vp9d_merge_prob(&pp[4], c[4] + c[5], sum, 20, 128);
        vp9d_merge_prob(&pp[5], c[4], c[5], 20, 128);
        sum -= c[6];
        vp9d_merge_prob(&pp[6], c[6], sum, 20, 128);
        vp9d_merge_prob(&pp[7], c[7] + c[8], c[9] + c[10], 20, 128);
        vp9d_merge_prob(&pp[8], c[7], c[8], 20, 128);
        vp9d_merge_prob(&pp[9], c[9], c[10], 20, 128);

        vp9d_merge_prob(&p->mv_comp[i].class0, s->counts.class0[i][0],
                        s->counts.class0[i][1], 20, 128);
        pp = p->mv_comp[i].bits;
        c2 = s->counts.bits[i];
        for (j = 0; j < 10; j++)
            vp9d_merge_prob(&pp[j], c2[j][0], c2[j][1], 20, 128);

        for (j = 0; j < 2; j++) {
            pp = p->mv_comp[i].class0_fp[j];
            c = s->counts.class0_fp[i][j];
            vp9d_merge_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
            vp9d_merge_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
            vp9d_merge_prob(&pp[2], c[2], c[3], 20, 128);
        }
        pp = p->mv_comp[i].fp;
        c = s->counts.fp[i];
        vp9d_merge_prob(&pp[0], c[0], c[1] + c[2] + c[3], 20, 128);
        vp9d_merge_prob(&pp[1], c[1], c[2] + c[3], 20, 128);
        vp9d_merge_prob(&pp[2], c[2], c[3], 20, 128);

        if (s->allow_high_precision_mv) {
            vp9d_merge_prob(&p->mv_comp[i].class0_hp, s->counts.class0_hp[i][0],
                            s->counts.class0_hp[i][1], 20, 128);
            vp9d_merge_prob(&p->mv_comp[i].hp, s->counts.hp[i][0],
                            s->counts.hp[i][1], 20, 128);
        }
    }

    // y intra modes
    for (i = 0; i < 4; i++) {
        RK_U8 *pp = p->y_mode[i];
        RK_U32 *c = s->counts.y_mode[i], sum, s2;

        sum = c[0] + c[1] + c[3] + c[4] + c[5] + c[6] + c[7] + c[8] + c[9];
        vp9d_merge_prob(&pp[0], c[VP9_DC_PRED], sum, 20, 128);
        sum -= c[VP9_TM_VP8_PRED];
        vp9d_merge_prob(&pp[1], c[VP9_TM_VP8_PRED], sum, 20, 128);
        sum -= c[VP9_VERT_PRED];
        vp9d_merge_prob(&pp[2], c[VP9_VERT_PRED], sum, 20, 128);
        s2 = c[VP9_HOR_PRED] + c[VP9_DIAG_DOWN_RIGHT_PRED] + c[VP9_VERT_RIGHT_PRED];
        sum -= s2;
        vp9d_merge_prob(&pp[3], s2, sum, 20, 128);
        s2 -= c[VP9_HOR_PRED];
        vp9d_merge_prob(&pp[4], c[VP9_HOR_PRED], s2, 20, 128);
        vp9d_merge_prob(&pp[5], c[VP9_DIAG_DOWN_RIGHT_PRED], c[VP9_VERT_RIGHT_PRED], 20, 128);
        sum -= c[VP9_DIAG_DOWN_LEFT_PRED];
        vp9d_merge_prob(&pp[6], c[VP9_DIAG_DOWN_LEFT_PRED], sum, 20, 128);
        sum -= c[VP9_VERT_LEFT_PRED];
        vp9d_merge_prob(&pp[7], c[VP9_VERT_LEFT_PRED], sum, 20, 128);
        vp9d_merge_prob(&pp[8], c[VP9_HOR_DOWN_PRED], c[VP9_HOR_UP_PRED], 20, 128);
    }

    // uv intra modes
    for (i = 0; i < 10; i++) {
        RK_U8 *pp = p->uv_mode[i];
        RK_U32 *c = s->counts.uv_mode[i], sum, s2;

        sum = c[0] + c[1] + c[3] + c[4] + c[5] + c[6] + c[7] + c[8] + c[9];
        vp9d_merge_prob(&pp[0], c[VP9_DC_PRED], sum, 20, 128);
        sum -= c[VP9_TM_VP8_PRED];
        vp9d_merge_prob(&pp[1], c[VP9_TM_VP8_PRED], sum, 20, 128);
        sum -= c[VP9_VERT_PRED];
        vp9d_merge_prob(&pp[2], c[VP9_VERT_PRED], sum, 20, 128);
        s2 = c[VP9_HOR_PRED] + c[VP9_DIAG_DOWN_RIGHT_PRED] + c[VP9_VERT_RIGHT_PRED];
        sum -= s2;
        vp9d_merge_prob(&pp[3], s2, sum, 20, 128);
        s2 -= c[VP9_HOR_PRED];
        vp9d_merge_prob(&pp[4], c[VP9_HOR_PRED], s2, 20, 128);
        vp9d_merge_prob(&pp[5], c[VP9_DIAG_DOWN_RIGHT_PRED], c[VP9_VERT_RIGHT_PRED], 20, 128);
        sum -= c[VP9_DIAG_DOWN_LEFT_PRED];
        vp9d_merge_prob(&pp[6], c[VP9_DIAG_DOWN_LEFT_PRED], sum, 20, 128);
        sum -= c[VP9_VERT_LEFT_PRED];
        vp9d_merge_prob(&pp[7], c[VP9_VERT_LEFT_PRED], sum, 20, 128);
        vp9d_merge_prob(&pp[8], c[VP9_HOR_DOWN_PRED], c[VP9_HOR_UP_PRED], 20, 128);
    }
#if 0 //def dump
    fwrite(s->counts.y_mode, 1, sizeof(s->counts.y_mode), vp9_p_fp1);
    fwrite(s->counts.uv_mode, 1, sizeof(s->counts.uv_mode), vp9_p_fp1);
    fflush(vp9_p_fp1);
#endif
}

MPP_RET vp9_parser_update(Vp9DecCtx *ctx, void *count_info)
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

        if (s->refresh_frame_context && !s->frame_parallel_mode) {
#ifdef dump
            count++;
#endif
            inv_count_data(s);
            adapt_coef_probs(s);

        }
    }

    return MPP_OK;
}
