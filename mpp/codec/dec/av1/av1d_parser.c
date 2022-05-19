/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "av1d_parser"

#include <stdlib.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_compat_impl.h"

#include "mpp_bitread.h"
#include "mpp_packet_impl.h"

#include "av1d_parser.h"
#include "mpp_dec_cb_param.h"
#include "mpp_frame_impl.h"

RK_U32 av1d_debug = 0;
/**
 * Clip a signed integer into the -(2^p),(2^p-1) range.
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
static   RK_U32 mpp_clip_uintp2(RK_S32 a, RK_S32 p)
{
    if (a & ~((1 << p) - 1)) return -a >> 31 & ((1 << p) - 1);
    else                   return  a;
}

static const Av1UnitType unit_types[] = {
    AV1_OBU_TEMPORAL_DELIMITER,
    AV1_OBU_SEQUENCE_HEADER,
    AV1_OBU_FRAME_HEADER,
    AV1_OBU_TILE_GROUP,
    AV1_OBU_FRAME,
};

static MPP_RET get_pixel_format(Av1CodecContext *ctx)
{
    AV1Context *s = ctx->priv_data;
    const AV1RawSequenceHeader *seq = s->sequence_header;
    uint8_t bit_depth = 8;;
    MPP_RET ret = MPP_OK;
    MppFrameFormat pix_fmt = MPP_FMT_BUTT;

    if (seq->seq_profile == 2 && seq->color_config.high_bitdepth)
        bit_depth = seq->color_config.twelve_bit ? 12 : 10;
    else if (seq->seq_profile <= 2)
        bit_depth = seq->color_config.high_bitdepth ? 10 : 8;
    else {
        mpp_err_f("Unknown AV1 profile %d.\n", seq->seq_profile);
        return -1;
    }

    if (!seq->color_config.mono_chrome) {
        // 4:4:4 x:0 y:0, 4:2:2 x:1 y:0, 4:2:0 x:1 y:1
        if (seq->color_config.subsampling_x == 0 &&
            seq->color_config.subsampling_y == 0) {
            mpp_err_f("no support yuv444 AV1 pixel format.\n");
            return -1;
        } else if (seq->color_config.subsampling_x == 1 &&
                   seq->color_config.subsampling_y == 0) {
            if (bit_depth == 8)
                pix_fmt = MPP_FMT_YUV422P;
            else {
                mpp_err_f("no support yuv422 bit depth > 8\n");
                return -1;
            }
        } else if (seq->color_config.subsampling_x == 1 &&
                   seq->color_config.subsampling_y == 1) {
            if (bit_depth == 8)
                pix_fmt = MPP_FMT_YUV420SP;
            else if (bit_depth == 10) {
                pix_fmt = MPP_FMT_YUV420SP; //pp will covert to 8 bit
            } else {
                mpp_err_f("no support MPP_FMT_YUV420SP bit depth > 8\n");
                return -1;
            }
        }
    } else {
        mpp_err_f("no supprot PIX_FMT_GRAY pixel format.\n");
        return -1;
    }


    if (pix_fmt == MPP_FMT_BUTT)
        return -1;

    ctx->pix_fmt = pix_fmt;
    s->bit_depth = bit_depth;
    return ret;
}

static RK_U32 inverse_recenter(RK_S32 r, RK_U32 v)
{
    if ((RK_S32)v > 2 * r)
        return v;
    else if (v & 1)
        return r - ((v + 1) >> 1);
    else
        return r + (v >> 1);
}

static RK_U32 decode_unsigned_subexp_with_ref(RK_U32 sub_exp,
                                              RK_S32 mx, RK_S32 r)
{
    if ((r << 1) <= mx) {
        return inverse_recenter(r, sub_exp);
    } else {
        return mx - 1 - inverse_recenter(mx - 1 - r, sub_exp);
    }
}

static RK_S32 decode_signed_subexp_with_ref(RK_U32 sub_exp, RK_S32 low,
                                            RK_S32 high, RK_S32 r)
{
    RK_S32 x = decode_unsigned_subexp_with_ref(sub_exp, high - low, r - low);
    return x + low;
}

static void read_global_param(AV1Context *s, RK_S32 type, RK_S32 ref, RK_S32 idx)
{
    uint8_t primary_frame, prev_frame;
    RK_U32 abs_bits, prec_bits, round, prec_diff, sub, mx;
    RK_S32 r, prev_gm_param;

    primary_frame = s->raw_frame_header->primary_ref_frame;
    prev_frame = s->raw_frame_header->ref_frame_idx[primary_frame];
    abs_bits = AV1_GM_ABS_ALPHA_BITS;
    prec_bits = AV1_GM_ALPHA_PREC_BITS;

    /* setup_past_independence() sets PrevGmParams to default values. We can
     * simply point to the current's frame gm_params as they will be initialized
     * with defaults at this point.
     */
    if (s->raw_frame_header->primary_ref_frame == AV1_PRIMARY_REF_NONE)
        prev_gm_param = s->cur_frame.gm_params[ref][idx];
    else
        prev_gm_param = s->ref[prev_frame].gm_params[ref][idx];

    if (idx < 2) {
        if (type == AV1_WARP_MODEL_TRANSLATION) {
            abs_bits = AV1_GM_ABS_TRANS_ONLY_BITS -
                       !s->raw_frame_header->allow_high_precision_mv;
            prec_bits = AV1_GM_TRANS_ONLY_PREC_BITS -
                        !s->raw_frame_header->allow_high_precision_mv;
        } else {
            abs_bits = AV1_GM_ABS_TRANS_BITS;
            prec_bits = AV1_GM_TRANS_PREC_BITS;
        }
    }
    round = (idx % 3) == 2 ? (1 << AV1_WARPEDMODEL_PREC_BITS) : 0;
    prec_diff = AV1_WARPEDMODEL_PREC_BITS - prec_bits;
    sub = (idx % 3) == 2 ? (1 << prec_bits) : 0;
    mx = 1 << abs_bits;
    r = (prev_gm_param >> prec_diff) - sub;

    s->cur_frame.gm_params[ref][idx] =
        (decode_signed_subexp_with_ref(s->raw_frame_header->gm_params[ref][idx],
                                       -mx, mx + 1, r) << prec_diff) + round;
}

/**
* update gm type/params, since cbs already implemented part of this funcation,
* so we don't need to full implement spec.
*/
static void global_motion_params(AV1Context *s)
{
    const AV1RawFrameHeader *header = s->raw_frame_header;
    RK_S32 type, ref;
    RK_S32 i = 0;

    for (ref = AV1_REF_FRAME_LAST; ref <= AV1_REF_FRAME_ALTREF; ref++) {
        s->cur_frame.gm_type[ref] = AV1_WARP_MODEL_IDENTITY;
        for (i = 0; i < 6; i++)
            s->cur_frame.gm_params[ref][i] = (i % 3 == 2) ?
                                             1 << AV1_WARPEDMODEL_PREC_BITS : 0;
    }
    if (header->frame_type == AV1_FRAME_KEY ||
        header->frame_type == AV1_FRAME_INTRA_ONLY)
        return;

    for (ref = AV1_REF_FRAME_LAST; ref <= AV1_REF_FRAME_ALTREF; ref++) {
        if (header->is_global[ref]) {
            if (header->is_rot_zoom[ref]) {
                type = AV1_WARP_MODEL_ROTZOOM;
            } else {
                type = header->is_translation[ref] ? AV1_WARP_MODEL_TRANSLATION
                       : AV1_WARP_MODEL_AFFINE;
            }
        } else {
            type = AV1_WARP_MODEL_IDENTITY;
        }
        s->cur_frame.gm_type[ref] = type;

        if (type >= AV1_WARP_MODEL_ROTZOOM) {
            read_global_param(s, type, ref, 2);
            read_global_param(s, type, ref, 3);
            if (type == AV1_WARP_MODEL_AFFINE) {
                read_global_param(s, type, ref, 4);
                read_global_param(s, type, ref, 5);
            } else {
                s->cur_frame.gm_params[ref][4] = -s->cur_frame.gm_params[ref][3];
                s->cur_frame.gm_params[ref][5] = s->cur_frame.gm_params[ref][2];
            }
        }
        if (type >= AV1_WARP_MODEL_TRANSLATION) {
            read_global_param(s, type, ref, 0);
            read_global_param(s, type, ref, 1);
        }
    }
}

static RK_S32 get_relative_dist(const AV1RawSequenceHeader *seq,
                                RK_U32 a, RK_U32 b)
{
    RK_U32 diff = a - b;
    RK_U32 m = 1 << seq->order_hint_bits_minus_1;
    return (diff & (m - 1)) - (diff & m);
}

static void skip_mode_params(AV1Context *s)
{
    const AV1RawFrameHeader *header = s->raw_frame_header;
    const AV1RawSequenceHeader *seq = s->sequence_header;

    RK_S32 forward_idx,  backward_idx;
    RK_S32 forward_hint, backward_hint;
    RK_S32 second_forward_idx, second_forward_hint;
    RK_S32 ref_hint, dist, i;

    if (!header->skip_mode_present)
        return;

    forward_idx  = -1;
    backward_idx = -1;
    forward_hint = -1;
    backward_hint = -1;
    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        ref_hint = s->ref[header->ref_frame_idx[i]].order_hint;
        dist = get_relative_dist(seq, ref_hint, header->order_hint);
        if (dist < 0) {
            if (forward_idx < 0 ||
                get_relative_dist(seq, ref_hint, forward_hint) > 0) {
                forward_idx  = i;
                forward_hint = ref_hint;
            }
        } else if (dist > 0) {
            if (backward_idx < 0 ||
                get_relative_dist(seq, ref_hint, backward_hint) < 0) {
                backward_idx  = i;
                backward_hint = ref_hint;
            }
        }
    }

    if (forward_idx < 0) {
        return;
    } else if (backward_idx >= 0) {
        s->cur_frame.skip_mode_frame_idx[0] =
            AV1_REF_FRAME_LAST + MPP_MIN(forward_idx, backward_idx);
        s->cur_frame.skip_mode_frame_idx[1] =
            AV1_REF_FRAME_LAST + MPP_MAX(forward_idx, backward_idx);
        return;
    }

    second_forward_idx = -1;
    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        ref_hint = s->ref[header->ref_frame_idx[i]].order_hint;
        if (get_relative_dist(seq, ref_hint, forward_hint) < 0) {
            if (second_forward_idx < 0 ||
                get_relative_dist(seq, ref_hint, second_forward_hint) > 0) {
                second_forward_idx  = i;
                second_forward_hint = ref_hint;
            }
        }
    }

    if (second_forward_idx < 0)
        return;

    s->cur_frame.skip_mode_frame_idx[0] =
        AV1_REF_FRAME_LAST + MPP_MIN(forward_idx, second_forward_idx);
    s->cur_frame.skip_mode_frame_idx[1] =
        AV1_REF_FRAME_LAST + MPP_MAX(forward_idx, second_forward_idx);
}

static void coded_lossless_param(AV1Context *s)
{
    const AV1RawFrameHeader *header = s->raw_frame_header;
    RK_S32 i;

    if (header->delta_q_y_dc || header->delta_q_u_ac ||
        header->delta_q_u_dc || header->delta_q_v_ac ||
        header->delta_q_v_dc) {
        s->cur_frame.coded_lossless = 0;
        return;
    }

    s->cur_frame.coded_lossless = 1;
    for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
        RK_S32 qindex;
        if (header->feature_enabled[i][AV1_SEG_LVL_ALT_Q]) {
            qindex = (header->base_q_idx +
                      header->feature_value[i][AV1_SEG_LVL_ALT_Q]);
        } else {
            qindex = header->base_q_idx;
        }
        qindex = mpp_clip_uintp2(qindex, 8);

        if (qindex) {
            s->cur_frame.coded_lossless = 0;
            return;
        }
    }
}

static void load_grain_params(AV1Context *s)
{
    const AV1RawFrameHeader *header = s->raw_frame_header;
    const AV1RawFilmGrainParams *film_grain = &header->film_grain, *src;
    AV1RawFilmGrainParams *dst = &s->cur_frame.film_grain;

    if (!film_grain->apply_grain)
        return;

    if (film_grain->update_grain) {
        memcpy(dst, film_grain, sizeof(*dst));
        return;
    }

    src = &s->ref[film_grain->film_grain_params_ref_idx].film_grain;

    memcpy(dst, src, sizeof(*dst));
    dst->grain_seed = film_grain->grain_seed;
}

typedef struct GetByteCxt_t {
    const uint8_t *buffer, *buffer_end, *buffer_start;
} GetByteCxt;
void bytestream_init(GetByteCxt *g,
                     const uint8_t *buf,
                     int buf_size)
{
    mpp_assert(buf_size >= 0);
    g->buffer       = buf;
    g->buffer_start = buf;
    g->buffer_end   = buf + buf_size;
}

static int bytestream_get_bytes_left(GetByteCxt *g)
{
    return g->buffer_end - g->buffer;
}

static void bytestream_skipu(GetByteCxt *g, unsigned int size)
{
    g->buffer += size;
}

static int bytestream_tell(GetByteCxt *g)
{
    return (int)(g->buffer - g->buffer_start);
}

static int bytestream_get_byteu(GetByteCxt *g)
{
    int tmp;
    tmp = g->buffer[0];
    g->buffer++;
    return tmp;
}

static RK_S32 get_tiles_info(Av1CodecContext *ctx, const AV1RawTileGroup *tile_group)
{
    AV1Context *s = ctx->priv_data;
    GetByteCxt gb;
    RK_S16 tile_num;
    RK_S32 size = 0, size_bytes = 0;
    RK_S32 i;

    bytestream_init(&gb, tile_group->tile_data.data,
                    tile_group->tile_data.data_size);

    for (tile_num = tile_group->tg_start; tile_num <= tile_group->tg_end; tile_num++) {

        if (tile_num == tile_group->tg_end) {
            s->tile_offset_start[tile_num] = bytestream_tell(&gb);
            s->tile_offset_end[tile_num] = bytestream_tell(&gb) + bytestream_get_bytes_left(&gb);
            return 0;
        }
        size_bytes = s->raw_frame_header->tile_size_bytes_minus1 + 1;
        if (bytestream_get_bytes_left(&gb) < size_bytes)
            return MPP_ERR_VALUE;
        size = 0;
        for (i = 0; i < size_bytes; i++)
            size |= bytestream_get_byteu(&gb) << 8 * i;
        if (bytestream_get_bytes_left(&gb) <= size)
            return MPP_ERR_VALUE;
        size++;

        s->tile_offset_start[tile_num] = bytestream_tell(&gb);

        s->tile_offset_end[tile_num] = bytestream_tell(&gb) + size;

        bytestream_skipu(&gb, size);
    }

    return 0;

}

static void av1d_frame_unref(Av1CodecContext *ctx, AV1Frame *f)
{
    AV1Context *s = ctx->priv_data;
    f->raw_frame_header = NULL;
    f->spatial_id = f->temporal_id = 0;
    memset(f->skip_mode_frame_idx, 0,
           2 * sizeof(uint8_t));
    memset(&f->film_grain, 0, sizeof(f->film_grain));

    f->coded_lossless = 0;

    if (!f->ref || f->ref->ref_count <= 0 || f->slot_index >= 0x7f) {
        mpp_err("ref count alreay is zero");
        return;
    }

    f->ref->ref_count--;
    av1d_dbg(AV1D_DBG_REF, "ref %p, f->ref->ref_count %d, ref->invisible= %d", f->ref, f->ref->ref_count, f->ref->invisible);
    if (!f->ref->ref_count) {
        if (f->slot_index < 0x7f) {
            av1d_dbg(AV1D_DBG_REF, "clr f->slot_index = %d",  f->slot_index);
            /* if pic no output for disaplay when ref_cnt
               clear we will free this buffer directly,
               maybe cause some frame can't display    */
            // if (f->ref->invisible && !f->ref->is_output) {
            if (!f->ref->is_output) {
                MppBuffer framebuf = NULL;
                mpp_buf_slot_get_prop(s->slots, f->slot_index, SLOT_BUFFER, &framebuf);
                av1d_dbg(AV1D_DBG_REF, "free framebuf prt %p", framebuf);
                if (framebuf)
                    mpp_buffer_put(framebuf);
                f->ref->invisible = 0;
            }
            mpp_buf_slot_clr_flag(s->slots, f->slot_index, SLOT_CODEC_USE);
        }
        f->slot_index = 0xff;
        mpp_free(f->ref);
        f->ref = NULL;
    }
    f->ref = NULL;
}


static MPP_RET set_output_frame(Av1CodecContext *ctx)
{
    AV1Context *s = ctx->priv_data;
    MppFrame frame = NULL;
    MPP_RET ret = MPP_OK;

    // TODO: all layers
    if (s->operating_point_idc &&
        mpp_log2(s->operating_point_idc >> 8) > s->cur_frame.spatial_id)
        return 0;
    mpp_buf_slot_get_prop(s->slots, s->cur_frame.slot_index, SLOT_FRAME_PTR, &frame);
    mpp_frame_set_pts(frame, s->pts);
    mpp_buf_slot_set_flag(s->slots, s->cur_frame.slot_index, SLOT_QUEUE_USE);
    mpp_buf_slot_enqueue(s->slots, s->cur_frame.slot_index, QUEUE_DISPLAY);
    s->cur_frame.ref->is_output = 1;

    return ret;
}

static RK_S32 av1d_frame_ref(Av1CodecContext *ctx, AV1Frame *dst, const AV1Frame *src)
{
    AV1Context *s = ctx->priv_data;
    MppFrameImpl *impl_frm = (MppFrameImpl *)dst->f;
    dst->spatial_id = src->spatial_id;
    dst->temporal_id = src->temporal_id;
    dst->order_hint  = src->order_hint;

    memcpy(dst->gm_type,
           src->gm_type,
           AV1_NUM_REF_FRAMES * sizeof(uint8_t));
    memcpy(dst->gm_params,
           src->gm_params,
           AV1_NUM_REF_FRAMES * 6 * sizeof(RK_S32));
    memcpy(dst->skip_mode_frame_idx,
           src->skip_mode_frame_idx,
           2 * sizeof(uint8_t));
    memcpy(&dst->film_grain,
           &src->film_grain,
           sizeof(dst->film_grain));
    dst->coded_lossless = src->coded_lossless;

    if (src->slot_index >= 0x7f) {
        mpp_err("av1d_ref_frame is vaild");
        return -1;
    }

    dst->slot_index = src->slot_index;
    dst->ref = src->ref;
    dst->ref->ref_count++;

    mpp_buf_slot_get_prop(s->slots, src->slot_index, SLOT_FRAME, &dst->f);
    impl_frm->buffer = NULL; //parser no need process hal buf


    return 0;
}

static MPP_RET update_reference_list(Av1CodecContext *ctx)
{
    AV1Context *s = ctx->priv_data;
    const AV1RawFrameHeader *header = s->raw_frame_header;
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;
    RK_S32 lst2_buf_idx;
    RK_S32 lst3_buf_idx;
    RK_S32 gld_buf_idx;
    RK_S32 alt_buf_idx;
    RK_S32 lst_buf_idx;
    RK_S32 bwd_buf_idx;
    RK_S32 alt2_buf_idx;

    if (!header->show_existing_frame) {
        lst2_buf_idx = s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_LAST2 - AV1_REF_FRAME_LAST];
        lst3_buf_idx = s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_LAST3 - AV1_REF_FRAME_LAST];
        gld_buf_idx =  s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_GOLDEN - AV1_REF_FRAME_LAST];
        alt_buf_idx =  s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_ALTREF - AV1_REF_FRAME_LAST];
        lst_buf_idx =  s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_LAST - AV1_REF_FRAME_LAST];
        bwd_buf_idx =  s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_BWDREF - AV1_REF_FRAME_LAST];
        alt2_buf_idx = s->raw_frame_header->ref_frame_idx[AV1_REF_FRAME_ALTREF2 - AV1_REF_FRAME_LAST];

        s->cur_frame.ref->lst2_frame_offset = s->ref[lst2_buf_idx].order_hint;
        s->cur_frame.ref->lst3_frame_offset = s->ref[lst3_buf_idx].order_hint;
        s->cur_frame.ref->gld_frame_offset  = s->ref[gld_buf_idx].order_hint;
        s->cur_frame.ref->alt_frame_offset  = s->ref[alt_buf_idx].order_hint;
        s->cur_frame.ref->lst_frame_offset  = s->ref[lst_buf_idx].order_hint;
        s->cur_frame.ref->bwd_frame_offset  = s->ref[bwd_buf_idx].order_hint;
        s->cur_frame.ref->alt2_frame_offset = s->ref[alt2_buf_idx].order_hint;
    }

    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        av1d_dbg(AV1D_DBG_REF, "header->refresh_frame_flags = %d",
                 header->refresh_frame_flags);
        if (header->refresh_frame_flags & (1 << i)) {
            av1d_dbg(AV1D_DBG_REF, "av1 ref idx %d s->ref[%d].slot_index %d",
                     i, i, s->ref[i].slot_index);
            if (s->ref[i].ref)
                av1d_frame_unref(ctx, &s->ref[i]);

            if ((ret = av1d_frame_ref(ctx, &s->ref[i], &s->cur_frame)) < 0) {
                mpp_err_f("Failed to update frame %d in reference list\n", i);
                return ret;
            }
        }
    }
    return ret;
}

static RK_U32 hor_align_16(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static MPP_RET get_current_frame(Av1CodecContext *ctx)
{
    AV1Context *s = ctx->priv_data;
    MPP_RET ret = MPP_OK;
    AV1Frame *frame = &s->cur_frame;
    RK_U32 value;

    if (frame->ref)
        av1d_frame_unref(ctx, frame);

    mpp_frame_set_width(frame->f, ctx->width);
    mpp_frame_set_height(frame->f, ctx->height);

    mpp_frame_set_hor_stride(frame->f, MPP_ALIGN(ctx->width, 16));
    mpp_frame_set_ver_stride(frame->f, MPP_ALIGN(ctx->height, 8));
    mpp_frame_set_errinfo(frame->f, 0);
    mpp_frame_set_discard(frame->f, 0);
    mpp_frame_set_pts(frame->f, s->pts);

    if (MPP_FRAME_FMT_IS_FBC(s->cfg->base.out_fmt)) {
        mpp_slots_set_prop(s->slots, SLOTS_HOR_ALIGN, hor_align_16);
        if (s->bit_depth == 10) {
            if (ctx->pix_fmt == MPP_FMT_YUV420SP)
                ctx->pix_fmt = MPP_FMT_YUV420SP_10BIT;
            else
                mpp_err("422p 10bit no support");
        }
        mpp_frame_set_fmt(frame->f, ctx->pix_fmt | ((s->cfg->base.out_fmt & (MPP_FRAME_FBC_MASK))));
        mpp_frame_set_offset_x(frame->f, 0);
        mpp_frame_set_offset_y(frame->f, 8);
        if (*compat_ext_fbc_buf_size)
            mpp_frame_set_ver_stride(frame->f, MPP_ALIGN(ctx->height, 8) + 16);
    } else
        mpp_frame_set_fmt(frame->f, ctx->pix_fmt);

    if (ctx->pix_fmt == MPP_FMT_YUV420SP_10BIT)
        mpp_frame_set_hor_stride(frame->f, MPP_ALIGN(ctx->width * s->bit_depth / 8, 8));

    value = 4;
    mpp_slots_set_prop(s->slots, SLOTS_NUMERATOR, &value);
    value = 1;
    mpp_slots_set_prop(s->slots, SLOTS_DENOMINATOR, &value);
    mpp_buf_slot_get_unused(s->slots, &frame->slot_index);
    av1d_dbg(AV1D_DBG_REF, "get frame->slot_index %d", frame->slot_index);
    mpp_buf_slot_set_prop(s->slots, frame->slot_index, SLOT_FRAME, frame->f);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_HAL_OUTPUT);

    frame->ref = mpp_calloc(RefInfo, 1);
    frame->ref->ref_count++;
    frame->ref->is_intra_frame = !s->raw_frame_header->frame_type;
    frame->ref->intra_only     = (s->raw_frame_header->frame_type == 2);
    frame->ref->is_output = 0;
    if (!s->raw_frame_header->show_frame && !s->raw_frame_header->showable_frame) {
        frame->ref->invisible = 1;
    }

    if (ret < 0) {
        mpp_err_f("Failed to allocate space for current frame.\n");
        return ret;
    }

    /*  ret = init_tile_data(s);
      if (ret < 0) {
          mpp_err_f( "Failed to init tile data.\n");
          return ret;
      }*/

    global_motion_params(s);
    skip_mode_params(s);
    coded_lossless_param(s);
    load_grain_params(s);

    return ret;
}

MPP_RET av1d_parser_init(Av1CodecContext *ctx, ParserCfg *init)
{
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;
    av1d_dbg_func("enter ctx %p\n", ctx);
    AV1Context *s = mpp_calloc(AV1Context, 1);
    ctx->priv_data = (void*)s;
    if (!ctx->priv_data) {
        mpp_err("av1d codec context malloc fail");
        return MPP_ERR_NOMEM;
    }

    s->seq_ref = mpp_calloc(AV1RawSequenceHeader, 1);

    s->unit_types = unit_types;
    s->nb_unit_types = MPP_ARRAY_ELEMS(unit_types);
    s->packet_slots = init->packet_slots;
    s->slots = init->frame_slots;
    s->cfg = init->cfg;
    mpp_buf_slot_setup(s->slots, 25);

    mpp_env_get_u32("av1d_debug", &av1d_debug, 0);
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        mpp_frame_init(&s->ref[i].f);
        if (!s->ref[i].f) {
            mpp_err("Failed to allocate frame buffer %d\n", i);
            return MPP_ERR_NOMEM;
        }
        s->ref[i].slot_index = 0x7f;
        s->ref[i].ref = NULL;
    }

    mpp_frame_init(&s->cur_frame.f);
    s->cur_frame.ref = NULL;
    s->cur_frame.slot_index = 0xff;
    if (!s->cur_frame.f) {
        mpp_err("Failed to allocate frame buffer %d\n", i);
        return MPP_ERR_NOMEM;
    }
    s->cdfs = &s->default_cdfs;
    s->cdfs_ndvc = &s->default_cdfs_ndvc;
    AV1SetDefaultCDFs(s->cdfs, s->cdfs_ndvc);

    return MPP_OK;

    av1d_dbg_func("leave ctx %p\n", ctx);

    return ret;
}

MPP_RET av1d_parser_deinit(Av1CodecContext *ctx)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;

    AV1Context *s = ctx->priv_data;
    for ( i = 0; i < MPP_ARRAY_ELEMS(s->ref); i++) {
        if (s->ref[i].ref) {
            av1d_frame_unref(ctx, &s->ref[i]);
        }
        mpp_frame_deinit(&s->ref[i].f);
        s->ref[i].f = NULL;
    }
    if (s->cur_frame.ref) {
        av1d_frame_unref(ctx, &s->cur_frame);
    }
    mpp_frame_deinit(&s->cur_frame.f);

    mpp_av1_fragment_reset(&s->current_obu);
    MPP_FREE(s->seq_ref);
    MPP_FREE(ctx->priv_data);
    return MPP_OK;

    av1d_dbg_func("leave ctx %p\n", ctx);

    return ret;
}

MPP_RET av1d_parser_frame(Av1CodecContext *ctx, HalDecTask *task)
{
    RK_U8 *data = NULL;
    RK_S32 size = 0;
    AV1Context *s = ctx->priv_data;
    AV1RawTileGroup *raw_tile_group = NULL;
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    av1d_dbg_func("enter ctx %p\n", ctx);
    task->valid = 0;

    data = (RK_U8 *)mpp_packet_get_pos(ctx->pkt);
    size = (RK_S32)mpp_packet_get_length(ctx->pkt);

    s->pts = mpp_packet_get_pts(ctx->pkt);

    s->current_obu.data = data;
    s->current_obu.data_size = size;
    s->obu_len = 0;
    ret = mpp_av1_split_fragment(s, &s->current_obu, 0);
    if (ret < 0) {
        return ret;
    }
    ret = mpp_av1_read_fragment_content(s, &s->current_obu);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < s->current_obu.nb_units; i++) {
        Av1ObuUnit *unit = &s->current_obu.units[i];
        AV1RawOBU  *obu = unit->content;
        const AV1RawOBUHeader *header;

        if (!obu)
            continue;

        header = &obu->header;

        switch (unit->type) {
        case AV1_OBU_SEQUENCE_HEADER:
            memcpy(s->seq_ref, &obu->obu.sequence_header, sizeof(AV1RawSequenceHeader));
            s->sequence_header = s->seq_ref;
            ret = mpp_av1_set_context_with_sequence(ctx, s->sequence_header);
            if (ret < 0) {
                mpp_err_f("Failed to set context.\n");
                s->sequence_header = NULL;
                goto end;
            }

            s->operating_point_idc = s->sequence_header->operating_point_idc[s->operating_point];

            if (ctx->pix_fmt == MPP_FMT_BUTT) {
                ret = get_pixel_format(ctx);
                if (ret < 0) {
                    mpp_err_f("Failed to get pixel format.\n");
                    s->sequence_header = NULL;
                    goto end;
                }
            }
            break;
        case AV1_OBU_REDUNDANT_FRAME_HEADER:
            if (s->raw_frame_header)
                break;
            // fall-through
        case AV1_OBU_FRAME:
        case AV1_OBU_FRAME_HEADER:
            if (!s->sequence_header) {
                mpp_err_f("Missing Sequence Header.\n");
                ret = MPP_ERR_VALUE;
                goto end;
            }

            if (unit->type == AV1_OBU_FRAME)
                s->raw_frame_header = &obu->obu.frame.header;
            else
                s->raw_frame_header = &obu->obu.frame_header;

            if (s->raw_frame_header->show_existing_frame) {
                if (s->cur_frame.ref) {
                    av1d_frame_unref(ctx, &s->cur_frame);
                }

                ret = av1d_frame_ref(ctx, &s->cur_frame,
                                     &s->ref[s->raw_frame_header->frame_to_show_map_idx]);

                if (ret < 0) {
                    mpp_err_f("Failed to get reference frame.\n");
                    goto end;
                }

                ret = update_reference_list(ctx);
                if (ret < 0) {
                    mpp_err_f("Failed to update reference list.\n");
                    goto end;
                }
                ret = set_output_frame(ctx);
                if (ret < 0)
                    mpp_err_f("Set output frame error.\n");

                s->raw_frame_header = NULL;

                goto end;
            }
            ret = get_current_frame(ctx);
            if (ret < 0) {
                mpp_err_f("Get current frame error\n");
                goto end;
            }
            s->cur_frame.spatial_id  = header->spatial_id;
            s->cur_frame.temporal_id = header->temporal_id;
            s->cur_frame.order_hint  = s->raw_frame_header->order_hint;

            if (unit->type != AV1_OBU_FRAME)
                break;
            // fall-through
        case AV1_OBU_TILE_GROUP:
            if (!s->raw_frame_header) {
                mpp_err_f("Missing Frame Header.\n");
                ret = MPP_ERR_VALUE;
                goto end;
            }

            if (unit->type == AV1_OBU_FRAME)
                raw_tile_group = &obu->obu.frame.tile_group;
            else
                raw_tile_group = &obu->obu.tile_group;

            ret = get_tiles_info(ctx, raw_tile_group);
            if (ret < 0)
                goto end;

            break;
        case AV1_OBU_TILE_LIST:
        case AV1_OBU_TEMPORAL_DELIMITER:
        case AV1_OBU_PADDING:
        case AV1_OBU_METADATA:
            break;
        default:
            av1d_dbg(AV1D_DBG_HEADER, "Unknown obu type: %d (%d bits).\n",
                     unit->type, unit->data_size);
        }

        if (raw_tile_group && (s->tile_num == raw_tile_group->tg_end + 1)) {
            av1d_parser2_syntax(ctx);
            task->syntax.data = (void*)&ctx->pic_params;
            task->syntax.number = 1;
            task->valid = 1;
            task->output = s->cur_frame.slot_index;
            task->input_packet = ctx->pkt;

            for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
                int8_t ref_idx = s->raw_frame_header->ref_frame_idx[i];
                if (s->ref[ref_idx].slot_index < 0x7f) {
                    mpp_buf_slot_set_flag(s->slots, s->ref[ref_idx].slot_index, SLOT_HAL_INPUT);
                    MppFrame mframe = NULL;
                    task->refer[i] = s->ref[ref_idx].slot_index;
                    mpp_buf_slot_get_prop(s->slots, task->refer[i], SLOT_FRAME_PTR, &mframe);
                    if (mframe)
                        task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
                } else {
                    task->refer[i] = -1;
                }
            }
            ret = update_reference_list(ctx);
            if (ret < 0) {
                mpp_err_f("Failed to update reference list.\n");
                goto end;
            }

            if (s->raw_frame_header->show_frame) {
                ret = set_output_frame(ctx);
                if (ret < 0) {
                    mpp_err_f("Set output frame error\n");
                    goto end;
                }
            }
            raw_tile_group = NULL;
            s->raw_frame_header = NULL;
        }
    }

    if (s->eos) {
        task->flags.eos = 1;
    }
end:
    mpp_av1_fragment_reset(&s->current_obu);
    if (ret < 0)
        s->raw_frame_header = NULL;

    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;
}

void av1d_parser_update(Av1CodecContext *ctx, void *info)
{
    AV1Context* c_ctx = (AV1Context*)ctx->priv_data;
    DecCbHalDone *cb = (DecCbHalDone*)info;
    RK_U8 *data = (RK_U8*)cb->task;
    RK_U32 i;
    const RK_U32 mv_cdf_offset = offsetof(AV1CDFs, mv_cdf);
    const RK_U32 mv_cdf_size = sizeof(MvCDFs);
    const RK_U32 mv_cdf_end_offset = mv_cdf_offset + mv_cdf_size;
    const RK_U32 cdf_size = sizeof(AV1CDFs);

    av1d_dbg_func("enter ctx %p\n", ctx);
    if (!c_ctx->disable_frame_end_update_cdf) {
        for (i = 0; i < NUM_REF_FRAMES; i++) {
            if (c_ctx->refresh_frame_flags & (1 << i)) {
                /* 1. get cdfs */
                Av1GetCDFs(c_ctx, i);
                {
                    RK_U8 *cdf_base = (RK_U8 *)c_ctx->cdfs;
                    RK_U8 *cdf_ndvc_base = (RK_U8 *)c_ctx->cdfs_ndvc;
                    /* 2. read cdfs from memory*/
                    if (c_ctx->frame_is_intra) {
                        memcpy(cdf_base, data, mv_cdf_offset);
                        // Read intrabc MV context
                        memcpy(cdf_ndvc_base, data + mv_cdf_offset, mv_cdf_size);
                        memcpy(cdf_base + mv_cdf_end_offset, data + mv_cdf_end_offset,
                               cdf_size - mv_cdf_end_offset);
                    } else {
                        memcpy(cdf_base, data, cdf_size);
                    }
                }
                /* 3. store cdfs*/
                Av1StoreCDFs(c_ctx, c_ctx->refresh_frame_flags);
                break;
            }
        }
    }

    av1d_dbg_func("leave ctx %p\n", ctx);
}

MPP_RET av1d_paser_reset(Av1CodecContext *ctx)
{
    (void)ctx;
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;
    AV1Context *s = ctx->priv_data;

    av1d_dbg_func("enter ctx %p\n", ctx);
    for ( i = 0; i < MPP_ARRAY_ELEMS(s->ref); i++) {
        AV1Frame *f = &s->ref[i];
        if (f->ref) {
            av1d_frame_unref(ctx, &s->ref[i]);
        }

    }
    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;

}


static inline int64_t leb128(BitReadCtx_t *gb)
{
    int64_t ret = 0;
    RK_S32 byte = 0;
    RK_S32 i;

    for (i = 0; i < 8; i++) {
        mpp_read_bits(gb, 8, &byte);
        ret |= (int64_t)(byte & 0x7f) << (i * 7);
        if (!(byte & 0x80))
            break;
    }
    return ret;
}

static inline RK_S32 parse_obu_header(uint8_t *buf, RK_S32 buf_size,
                                      int64_t *obu_size, RK_S32 *start_pos, RK_S32 *type,
                                      RK_S32 *temporal_id, RK_S32 *spatial_id)
{
    BitReadCtx_t gb;
    RK_S32 extension_flag, has_size_flag;
    int64_t size;
    RK_S32 value = 0;
    mpp_set_bitread_ctx(&gb, buf, MPP_MIN(buf_size, MAX_OBU_HEADER_SIZE));

    mpp_read_bits(&gb, 1, &value);
    if (value != 0) // obu_forbidden_bit
        return MPP_ERR_PROTOL;

    mpp_read_bits(&gb, 4, type);
    mpp_read_bits(&gb, 1, &extension_flag);
    mpp_read_bits(&gb, 1, &has_size_flag);
    mpp_skip_bits(&gb, 1); // obu_reserved_1bit

    if (extension_flag) {
        mpp_read_bits(&gb, 3, temporal_id);
        mpp_read_bits(&gb, 2, spatial_id);
        mpp_skip_bits(&gb, 3); // extension_header_reserved_3bits
    } else {
        *temporal_id = *spatial_id = 0;
    }

    *obu_size  = has_size_flag ? leb128(&gb)
                 : buf_size - 1 - extension_flag;

    if (mpp_get_bits_left(&gb) < 0)
        return MPP_ERR_PROTOL;

    *start_pos = mpp_get_bits_count(&gb) / 8;

    size = *obu_size + *start_pos;

    if (size > buf_size)
        return MPP_ERR_PROTOL;

    return size;

}

RK_S32 av1_extract_obu(AV1OBU *obu, uint8_t *buf, RK_S32 length)
{
    int64_t obu_size;
    RK_S32 start_pos, type, temporal_id, spatial_id;
    RK_S32 len;

    len = parse_obu_header(buf, length, &obu_size, &start_pos,
                           &type, &temporal_id, &spatial_id);
    if (len < 0)
        return len;

    obu->type        = type;
    obu->temporal_id = temporal_id;
    obu->spatial_id  = spatial_id;

    obu->data     = buf + start_pos;
    obu->size     = obu_size;
    obu->raw_data = buf;
    obu->raw_size = len;

    av1d_dbg(AV1D_DBG_STRMIN, "obu_type: %d, temporal_id: %d, spatial_id: %d, payload size: %d\n",
             obu->type, obu->temporal_id, obu->spatial_id, obu->size);

    return len;
}


RK_S32 av1d_split_frame(SplitContext_t *ctx,
                        RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 size)
{
    (void)ctx;
    (void)out_data;
    (void)out_size;

    AV1OBU obu;
    uint8_t *ptr = data, *end = data + size;
    av1d_dbg_func("enter ctx %p\n", ctx);

    *out_data = data;

    while (ptr < end) {
        RK_S32 len = av1_extract_obu(&obu, ptr, size);
        if (len < 0)
            break;
        if (obu.type == AV1_OBU_FRAME_HEADER ||
            obu.type == AV1_OBU_FRAME) {
            ptr      += len;
            size     -= len;
            *out_size = (RK_S32)(ptr - data);
            return ptr - data;
        }
        ptr      += len;
        size -= len;
    }

    return 0;

    av1d_dbg_func("leave ctx %p\n", ctx);
    return 0;

}

MPP_RET av1d_get_frame_stream(Av1CodecContext *ctx, RK_U8 *buf, RK_S32 length)
{
    MPP_RET ret = MPP_OK;
    av1d_dbg_func("enter ctx %p\n", ctx);
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

    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;

}

MPP_RET av1d_split_deinit(Av1CodecContext *ctx)
{
    MPP_RET ret = MPP_OK;
    (void)ctx;

    av1d_dbg_func("enter ctx %p\n", ctx);

    av1d_dbg_func("leave ctx %p\n", ctx);

    return ret;
}

MPP_RET av1d_split_init(Av1CodecContext *ctx)
{
    MPP_RET ret = MPP_OK;
    (void)ctx;

    av1d_dbg_func("enter ctx %p\n", ctx);

    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;
}
