/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "av1d_parser"

#include <stdlib.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_compat_impl.h"
#include "rk_hdr_meta_com.h"

#include "mpp_bitread.h"
#include "mpp_packet_impl.h"

#include "av1d_parser.h"
#include "mpp_dec_cb_param.h"
#include "mpp_frame_impl.h"

static const RK_S16 div_lut[AV1_DIV_LUT_NUM] = {
    16384, 16320, 16257, 16194, 16132, 16070, 16009, 15948, 15888, 15828, 15768,
    15709, 15650, 15592, 15534, 15477, 15420, 15364, 15308, 15252, 15197, 15142,
    15087, 15033, 14980, 14926, 14873, 14821, 14769, 14717, 14665, 14614, 14564,
    14513, 14463, 14413, 14364, 14315, 14266, 14218, 14170, 14122, 14075, 14028,
    13981, 13935, 13888, 13843, 13797, 13752, 13707, 13662, 13618, 13574, 13530,
    13487, 13443, 13400, 13358, 13315, 13273, 13231, 13190, 13148, 13107, 13066,
    13026, 12985, 12945, 12906, 12866, 12827, 12788, 12749, 12710, 12672, 12633,
    12596, 12558, 12520, 12483, 12446, 12409, 12373, 12336, 12300, 12264, 12228,
    12193, 12157, 12122, 12087, 12053, 12018, 11984, 11950, 11916, 11882, 11848,
    11815, 11782, 11749, 11716, 11683, 11651, 11619, 11586, 11555, 11523, 11491,
    11460, 11429, 11398, 11367, 11336, 11305, 11275, 11245, 11215, 11185, 11155,
    11125, 11096, 11067, 11038, 11009, 10980, 10951, 10923, 10894, 10866, 10838,
    10810, 10782, 10755, 10727, 10700, 10673, 10645, 10618, 10592, 10565, 10538,
    10512, 10486, 10460, 10434, 10408, 10382, 10356, 10331, 10305, 10280, 10255,
    10230, 10205, 10180, 10156, 10131, 10107, 10082, 10058, 10034, 10010, 9986,
    9963,  9939,  9916,  9892,  9869,  9846,  9823,  9800,  9777,  9754,  9732,
    9709,  9687,  9664,  9642,  9620,  9598,  9576,  9554,  9533,  9511,  9489,
    9468,  9447,  9425,  9404,  9383,  9362,  9341,  9321,  9300,  9279,  9259,
    9239,  9218,  9198,  9178,  9158,  9138,  9118,  9098,  9079,  9059,  9039,
    9020,  9001,  8981,  8962,  8943,  8924,  8905,  8886,  8867,  8849,  8830,
    8812,  8793,  8775,  8756,  8738,  8720,  8702,  8684,  8666,  8648,  8630,
    8613,  8595,  8577,  8560,  8542,  8525,  8508,  8490,  8473,  8456,  8439,
    8422,  8405,  8389,  8372,  8355,  8339,  8322,  8306,  8289,  8273,  8257,
    8240,  8224,  8208,  8192
};

static MPP_RET get_pixel_format(Av1DecCtx *ctx)
{
    Av1Codec *s = ctx->priv_data;
    const AV1SeqHeader *seq = s->seq_header;
    uint8_t bit_depth = 8;
    MppFrameFormat pix_fmt = MPP_FMT_BUTT;

    if (seq->seq_profile == 2 && seq->color_config.high_bitdepth)
        bit_depth = (seq->color_config.twelve_bit != 0) ? 12 : 10;
    else if (seq->seq_profile <= 2)
        bit_depth = (seq->color_config.high_bitdepth != 0) ? 10 : 8;
    else {
        mpp_err_f("Unknown AV1 profile %d.\n", seq->seq_profile);
        return MPP_ERR_VALUE;
    }

    if (!seq->color_config.mono_chrome) {
        // 4:4:4 x:0 y:0, 4:2:2 x:1 y:0, 4:2:0 x:1 y:1
        if (seq->color_config.subsampling_x == 0 &&
            seq->color_config.subsampling_y == 0) {
            mpp_err_f("no support yuv444 AV1 pixel format.\n");
            return MPP_ERR_UNKNOW;
        } else if (seq->color_config.subsampling_x == 1 &&
                   seq->color_config.subsampling_y == 0) {
            if (bit_depth == 8)
                pix_fmt = MPP_FMT_YUV422P;
            else {
                mpp_err_f("no support yuv422 bit depth > 8\n");
                return MPP_ERR_UNKNOW;
            }
        } else if (seq->color_config.subsampling_x == 1 &&
                   seq->color_config.subsampling_y == 1) {
            if (bit_depth == 8)
                pix_fmt = MPP_FMT_YUV420SP;
            else if (bit_depth == 10) {
                pix_fmt = MPP_FMT_YUV420SP_10BIT;
                /* rk3588, allow user config 8bit for 10bit source. */
                if ((mpp_get_soc_type() == ROCKCHIP_SOC_RK3588) &&
                    (ctx->usr_set_fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV420SP &&
                    (ctx->cfg->base.out_fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV420SP)
                    pix_fmt = MPP_FMT_YUV420SP;
            } else {
                mpp_err_f("no support MPP_FMT_YUV420SP bit depth > 8\n");
                return MPP_ERR_UNKNOW;
            }
        }
    } else {
        mpp_err_f("no supprot PIX_FMT_GRAY pixel format.\n");
        return MPP_ERR_UNKNOW;
    }


    if (pix_fmt == MPP_FMT_BUTT)
        return MPP_ERR_UNKNOW;
    ctx->pix_fmt = pix_fmt;
    s->bit_depth = bit_depth;
    return MPP_OK;
}

// Inverse recenters a non-negative literal v around a reference r
static RK_U32 inv_recenter_nonneg(RK_S32 r, RK_U32 v)
{
    if (v > (RK_U32)(r << 1))
        return v;
    else if (v & 1) // v is odd
        return r - ((v + 1) >> 1);
    else // v is even
        return r + (v >> 1);
}

static RK_S32 decode_signed_subexp(RK_U32 sub_exp, RK_S32 low, RK_S32 high, RK_S32 r)
{
    RK_S32 x = 0;

    // Map to unsigned interval [0, high - low)
    high -= low;
    r -= low;
    // Inverse recenters a non-negative literal v in [0, n-1] around a
    // reference r also in [0, n-1]
    if (high < (r << 1)) {
        x = high - 1 - inv_recenter_nonneg(high - 1 - r, sub_exp);
    } else {
        x = inv_recenter_nonneg(r, sub_exp);
    }
    // Map back to signed interval [low, high)
    x += low;
    return x;
}

static MPP_RET set_current_global_param(Av1Codec *s, RK_S32 type, RK_S32 ref, RK_S32 idx)
{
    uint8_t primary_frame, prev_frame;
    RK_U32 abs_bits, prec_bits, round, prec_diff, sub, mx;
    RK_S32 r, prev_gm_param;
    const AV1FrameHeader *f = s->frame_header;

    primary_frame = f->primary_ref_frame;
    prev_frame = f->ref_frame_idx[primary_frame];
    abs_bits = AV1_GM_ABS_ALPHA_BITS;
    prec_bits = AV1_GM_ALPHA_PREC_BITS;

    /* setup_past_independence() sets PrevGmParams to default values. We can
     * simply point to the current's frame gm_params as they will be initialized
     * with defaults at this point.
     */
    if (primary_frame == AV1_PRIMARY_REF_NONE)
        prev_gm_param = s->cur_frame.gm_params[ref].wmmat[idx];
    else
        prev_gm_param = s->ref[prev_frame].gm_params[ref].wmmat[idx];

    if (idx < 2) {
        if (type == AV1_WARP_MODEL_TRANSLATION) {
            abs_bits = AV1_GM_ABS_TRANS_ONLY_BITS - !f->allow_high_precision_mv;
            prec_bits = AV1_GM_TRANS_ONLY_PREC_BITS - !f->allow_high_precision_mv;
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

    s->cur_frame.gm_params[ref].wmmat_val[idx] =
        decode_signed_subexp(f->gm_params[ref][idx], -mx, mx + 1, r);
    s->cur_frame.gm_params[ref].wmmat[idx] =
        (s->cur_frame.gm_params[ref].wmmat_val[idx] << prec_diff) + round;

    return MPP_OK;
}

/*
 * Decomposes a divisor D such that 1/D = y/2^shift, where y is returned
 * at precision of DIV_LUT_PREC_BITS along with the shift.
*/
static RK_S16 resolve_divisor(RK_U32 d, RK_S16 *shift)
{
    RK_S32 e, f;

    *shift = mpp_log2(d);
    // e is obtained from D after resetting the most significant 1 bit.
    e = d - (1 << (*shift));
    // Get the most significant DIV_LUT_BITS (8) bits of e into f
    if (*shift > AV1_DIV_LUT_BITS)
        f = mpp_round_pow2(e, *shift - AV1_DIV_LUT_BITS);
    else
        f = e << (AV1_DIV_LUT_BITS - (*shift));

    *shift += AV1_DIV_LUT_PREC_BITS;
    // Use f as lookup into the precomputed table of multipliers
    return div_lut[f];
}

// Returns 1 on success or 0 on an invalid affine set
static RK_U8 is_shear_params_valid(Av1Codec *s, RK_S32 idx)
{
    RK_S16 alpha, beta, gamma, delta, divf, divs;
    RK_S64 v, w;
    RK_S32 *mat = &s->cur_frame.gm_params[idx].wmmat[0];

    // check affine shear params validity
    if (mat[2] < 0)
        return 0;

    alpha = MPP_CLIP3(INT16_MIN, INT16_MAX, mat[2] - (1 << AV1_WARPEDMODEL_PREC_BITS));
    beta  = MPP_CLIP3(INT16_MIN, INT16_MAX, mat[3]);
    divf  = resolve_divisor(MPP_ABS(mat[2]), &divs) * (mat[2] < 0 ? -1 : 1);
    v     = (RK_S64)mat[4] * (1 << AV1_WARPEDMODEL_PREC_BITS);
    w     = (RK_S64)mat[3] * mat[4];
    gamma = MPP_CLIP3(INT16_MIN, INT16_MAX, (RK_S32)mpp_round_pow2_signed((v * divf), divs));
    delta = MPP_CLIP3(INT16_MIN, INT16_MAX,
                      mat[5] - (RK_S32)mpp_round_pow2_signed((w * divf),
                                                             divs) - (1 << AV1_WARPEDMODEL_PREC_BITS));

    alpha = mpp_round_pow2_signed(alpha, AV1_WARP_PARAM_REDUCE_BITS) << AV1_WARP_PARAM_REDUCE_BITS;
    beta  = mpp_round_pow2_signed(beta,  AV1_WARP_PARAM_REDUCE_BITS) << AV1_WARP_PARAM_REDUCE_BITS;
    gamma = mpp_round_pow2_signed(gamma, AV1_WARP_PARAM_REDUCE_BITS) << AV1_WARP_PARAM_REDUCE_BITS;
    delta = mpp_round_pow2_signed(delta, AV1_WARP_PARAM_REDUCE_BITS) << AV1_WARP_PARAM_REDUCE_BITS;

    s->cur_frame.gm_params[idx].alpha = alpha;
    s->cur_frame.gm_params[idx].beta = beta;
    s->cur_frame.gm_params[idx].gamma = gamma;
    s->cur_frame.gm_params[idx].delta = delta;

    // check shear params range
    RK_U8 out_range_flag = ((4 * MPP_ABS(alpha) + 7 * MPP_ABS(beta)) >= (1 << AV1_WARPEDMODEL_PREC_BITS) ||
                            (4 * MPP_ABS(gamma) + 4 * MPP_ABS(delta)) >= (1 << AV1_WARPEDMODEL_PREC_BITS));

    return out_range_flag ? 0 : 1;
}

static const AV1WarpedMotion default_warp_params = {
    AV1_WARP_MODEL_IDENTITY,
    { 0, 0, (1 << 16), 0, 0, (1 << 16) },
    { 0, 0, 0, 0, 0, 0 },
    (1 << 16), (1 << 16), (1 << 16), (1 << 16),
};

static MPP_RET set_current_global_motion_params(Av1Codec *s)
{
    const AV1FrameHeader *f = s->frame_header;
    RK_S32 type, ref;
    RK_S32 i = 0;

    for (ref = AV1_REF_FRAME_LAST; ref <= AV1_REF_FRAME_ALTREF; ref++) {
        s->cur_frame.gm_params[ref].wmtype = AV1_WARP_MODEL_IDENTITY;
        for (i = 0; i < 6; i++)
            s->cur_frame.gm_params[ref].wmmat[i] = (i % 3 == 2) ?
                                                   1 << AV1_WARPEDMODEL_PREC_BITS : 0;
    }
    if (f->frame_type == AV1_FRAME_KEY ||
        f->frame_type == AV1_FRAME_INTRA_ONLY)
        return MPP_OK;

    for (ref = AV1_REF_FRAME_LAST; ref <= AV1_REF_FRAME_ALTREF; ref++) {
        if (f->is_global[ref]) {
            if (f->is_rot_zoom[ref]) {
                type = AV1_WARP_MODEL_ROTZOOM;
            } else {
                type = f->is_translation[ref] ? AV1_WARP_MODEL_TRANSLATION
                       : AV1_WARP_MODEL_AFFINE;
            }
        } else {
            type = AV1_WARP_MODEL_IDENTITY;
        }
        s->cur_frame.gm_params[ref].wmtype = type;

        if (type >= AV1_WARP_MODEL_ROTZOOM) {
            set_current_global_param(s, type, ref, 2);
            set_current_global_param(s, type, ref, 3);
            if (type == AV1_WARP_MODEL_AFFINE) {
                set_current_global_param(s, type, ref, 4);
                set_current_global_param(s, type, ref, 5);
            } else {
                s->cur_frame.gm_params[ref].wmmat[4] = -s->cur_frame.gm_params[ref].wmmat[3];
                s->cur_frame.gm_params[ref].wmmat[5] = s->cur_frame.gm_params[ref].wmmat[2];
            }
        }
        if (type >= AV1_WARP_MODEL_TRANSLATION) {
            set_current_global_param(s, type, ref, 0);
            set_current_global_param(s, type, ref, 1);
        }
        // Validate shear params for affine and rotzoom models
        {
            RK_U32 shear_params_valid = 0;
            RK_U8 ref_uses_scaling = s->frame_width != s->ref_s[ref].frame_width ||
                                     s->frame_height != s->ref_s[ref].frame_height;

            if (type <= AV1_WARP_MODEL_AFFINE)
                shear_params_valid = is_shear_params_valid(s, ref);

            if (!shear_params_valid || ref_uses_scaling)
                s->cur_frame.gm_params[ref] = default_warp_params;
        }

        av1d_dbg(AV1D_DBG_HEADER, "ref %d alpa %d beta %d gamma %d delta %d\n",
                 ref,
                 s->cur_frame.gm_params[ref].alpha,
                 s->cur_frame.gm_params[ref].beta,
                 s->cur_frame.gm_params[ref].gamma,
                 s->cur_frame.gm_params[ref].delta);
    }
    return MPP_OK;
}

static MPP_RET set_current_skip_mode_params(Av1Codec *s)
{
    const AV1SeqHeader *seq = s->seq_header;
    const AV1FrameHeader *f = s->frame_header;

    RK_S32 forward_idx,  backward_idx;
    RK_S32 forward_hint, backward_hint;
    RK_S32 second_forward_idx, second_forward_hint;
    RK_S32 ref_hint, dist, i;

    if (!f->skip_mode_present)
        return MPP_OK;

    forward_idx  = -1;
    backward_idx = -1;
    forward_hint = -1;
    backward_hint = -1;
    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        ref_hint = s->ref[f->ref_frame_idx[i]].order_hint;
        dist = av1d_get_relative_dist(seq, ref_hint, f->order_hint);
        if (dist < 0) {
            if (forward_idx < 0 ||
                av1d_get_relative_dist(seq, ref_hint, forward_hint) > 0) {
                forward_idx  = i;
                forward_hint = ref_hint;
            }
        } else if (dist > 0) {
            if (backward_idx < 0 ||
                av1d_get_relative_dist(seq, ref_hint, backward_hint) < 0) {
                backward_idx  = i;
                backward_hint = ref_hint;
            }
        }
    }

    if (forward_idx < 0) {
        return MPP_OK;
    } else if (backward_idx >= 0) {
        s->cur_frame.skip_mode_frame_idx[0] =
            AV1_REF_FRAME_LAST + MPP_MIN(forward_idx, backward_idx);
        s->cur_frame.skip_mode_frame_idx[1] =
            AV1_REF_FRAME_LAST + MPP_MAX(forward_idx, backward_idx);
        return MPP_OK;
    }

    second_forward_idx = -1;
    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        ref_hint = s->ref[f->ref_frame_idx[i]].order_hint;
        if (av1d_get_relative_dist(seq, ref_hint, forward_hint) < 0) {
            if (second_forward_idx < 0 ||
                av1d_get_relative_dist(seq, ref_hint, second_forward_hint) > 0) {
                second_forward_idx  = i;
                second_forward_hint = ref_hint;
            }
        }
    }

    if (second_forward_idx < 0)
        return MPP_OK;

    s->cur_frame.skip_mode_frame_idx[0] =
        AV1_REF_FRAME_LAST + MPP_MIN(forward_idx, second_forward_idx);
    s->cur_frame.skip_mode_frame_idx[1] =
        AV1_REF_FRAME_LAST + MPP_MAX(forward_idx, second_forward_idx);

    return MPP_OK;
}

static MPP_RET set_current_coded_lossless_param(Av1Codec *s)
{
    const AV1FrameHeader *f = s->frame_header;

    if (f->delta_q_y_dc || f->delta_q_u_ac ||
        f->delta_q_u_dc || f->delta_q_v_ac || f->delta_q_v_dc) {
        s->cur_frame.coded_lossless = 0;

        return MPP_OK;
    }

    s->cur_frame.coded_lossless = 1;
    for (RK_S32 i = 0; i < AV1_MAX_SEGMENTS; i++) {
        RK_S32 qindex;
        if (f->feature_enabled[i][AV1_SEG_LVL_ALT_Q]) {
            qindex = (f->base_q_idx +
                      f->feature_value[i][AV1_SEG_LVL_ALT_Q]);
        } else {
            qindex = f->base_q_idx;
        }
        qindex = mpp_clip_uint_pow2(qindex, 8);

        if (qindex) {
            s->cur_frame.coded_lossless = 0;
            return MPP_OK;
        }
    }
    return MPP_OK;
}

static MPP_RET set_current_read_grain_params(Av1Codec *s)
{

    AV1FilmGrainParams *src, *dst;
    const AV1FilmGrainParams *film_grain = &s->frame_header->film_grain;

    src = &s->ref[film_grain->film_grain_params_ref_idx].film_grain;
    dst = &s->cur_frame.film_grain;

    if (!film_grain->apply_grain)
        return MPP_OK;

    if (film_grain->update_grain) {
        memcpy(dst, film_grain, sizeof(*dst));
        return MPP_OK;
    }

    memcpy(dst, src, sizeof(*dst));
    dst->grain_seed = film_grain->grain_seed;

    return MPP_OK;
}

static MPP_RET get_tiles_info(Av1DecCtx *ctx, const AV1TileGroup *tile_group)
{
    Av1Codec *s = ctx->priv_data;
    BitReadCtx_t m_gb, *gb = &m_gb;
    const AV1FrameHeader *f = s->frame_header;

    // Initialize BitReadCtx_t
    mpp_set_bitread_ctx(gb, tile_group->tile_data.data, tile_group->tile_data.data_size);

    if (s->tile_offset)
        s->tile_offset += tile_group->tile_data.offset;

    for (RK_S32 tile_num = tile_group->tg_start; tile_num <= tile_group->tg_end; tile_num++) {
        RK_S32 used_bytes, left_bytes, size_bytes, size, i;

        if (tile_num == tile_group->tg_end) {
            used_bytes = mpp_get_bits_count(gb) / 8;
            s->tile_offset_start[tile_num] = used_bytes + s->tile_offset;
            s->tile_offset_end[tile_num] = tile_group->tile_data.data_size + s->tile_offset;
            s->tile_offset = s->tile_offset_end[tile_num];
            return MPP_OK;
        }
        size_bytes = f->tile_size_bytes_minus1 + 1;
        // Check if there are enough bytes left
        left_bytes = mpp_get_bits_left(gb) / 8;
        if (left_bytes < size_bytes)
            return MPP_ERR_VALUE;

        // Read the size_bytes bytes
        READ_BITS_LONG(gb, size_bytes * 8, &size);
        // Check if there are enough bytes left
        left_bytes = mpp_get_bits_left(gb) / 8;
        if (left_bytes <= size)
            return MPP_ERR_VALUE;

        size++;
        used_bytes = mpp_get_bits_count(gb) / 8;
        s->tile_offset_start[tile_num] = used_bytes + s->tile_offset;
        s->tile_offset_end[tile_num] = used_bytes + size + s->tile_offset;

        for (i = 0; i < size; i++)
            SKIP_BITS(gb, 8);
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET av1d_frame_unref(Av1DecCtx *ctx, AV1FrameIInfo *info)
{
    info->spatial_id = info->temporal_id = 0;
    memset(info->skip_mode_frame_idx, 0,
           2 * sizeof(uint8_t));
    memset(&info->film_grain, 0, sizeof(info->film_grain));

    info->coded_lossless = 0;

    if (!info->ref || info->ref->ref_count <= 0 || info->slot_index >= 0x7f) {
        mpp_err("ref count alreay is zero");
        return MPP_ERR_VALUE;
    }
    info->ref->ref_count--;
    av1d_dbg(AV1D_DBG_REF, "ref %p, info->ref->ref_count %d, info->ref->invisible= %d",
             info->ref, info->ref->ref_count, info->ref->invisible);
    if (!info->ref->ref_count) {
        if (info->slot_index < 0x7f) {
            av1d_dbg(AV1D_DBG_REF, "clr info->slot_index = %d",  info->slot_index);
            if (!info->ref->is_output) {
                MppBuffer framebuf = NULL;
                mpp_buf_slot_get_prop(ctx->slots, info->slot_index, SLOT_BUFFER, &framebuf);
                av1d_dbg(AV1D_DBG_REF, "free framebuf prt %p", framebuf);
                if (framebuf)
                    mpp_buffer_put(framebuf);
                info->ref->invisible = 0;
            }
            mpp_buf_slot_clr_flag(ctx->slots, info->slot_index, SLOT_CODEC_USE);
        }
        info->slot_index = 0xff;
        mpp_free(info->ref);
        info->ref = NULL;
    }
    info->ref = NULL;

    return MPP_OK;
}

static MPP_RET set_output_frame(Av1DecCtx *ctx)
{
    Av1Codec *s = ctx->priv_data;
    MppFrame frame = NULL;
    MPP_RET ret = MPP_OK;

    // TODO: all layers
    if (s->operating_point_idc &&
        mpp_log2(s->operating_point_idc >> 8) > s->cur_frame.spatial_id)
        return MPP_OK;
    mpp_buf_slot_get_prop(ctx->slots, s->cur_frame.slot_index, SLOT_FRAME_PTR, &frame);
    if (s->hdr_dynamic_meta && s->hdr_dynamic) {
        mpp_frame_set_hdr_dynamic_meta(frame, s->hdr_dynamic_meta);
        s->hdr_dynamic = 0;
        if (s->frame_header->show_existing_frame)
            fill_hdr_meta_to_frame(frame, MPP_VIDEO_CodingAV1);
    }
    mpp_frame_set_pts(frame, s->pts);
    mpp_frame_set_dts(frame, s->dts);
    mpp_buf_slot_set_flag(ctx->slots, s->cur_frame.slot_index, SLOT_QUEUE_USE);
    mpp_buf_slot_enqueue(ctx->slots, s->cur_frame.slot_index, QUEUE_DISPLAY);
    s->cur_frame.ref->is_output = 1;

    return ret;
}

static MPP_RET av1d_frame_ref(Av1DecCtx *ctx, AV1FrameIInfo *dst, const AV1FrameIInfo *src)
{
    MppFrameImpl *impl_frm = (MppFrameImpl *)dst->f;
    dst->spatial_id = src->spatial_id;
    dst->temporal_id = src->temporal_id;
    dst->order_hint  = src->order_hint;

    memcpy(dst->gm_params,
           src->gm_params,
           sizeof(src->gm_params));
    memcpy(dst->skip_mode_frame_idx,
           src->skip_mode_frame_idx,
           2 * sizeof(uint8_t));
    memcpy(&dst->film_grain,
           &src->film_grain,
           sizeof(dst->film_grain));
    dst->coded_lossless = src->coded_lossless;

    if (src->slot_index >= 0x7f) {
        mpp_err("av1d_ref_frame is vaild");
        return MPP_ERR_VALUE;
    }

    dst->slot_index = src->slot_index;
    dst->ref = src->ref;
    dst->ref->ref_count++;

    mpp_buf_slot_get_prop(ctx->slots, src->slot_index, SLOT_FRAME, &dst->f);
    impl_frm->buffer = NULL; //parser no need process hal buf

    return MPP_OK;
}

static MPP_RET update_reference_list(Av1DecCtx *ctx)
{
    Av1Codec *s = ctx->priv_data;
    const AV1FrameHeader *f = s->frame_header;
    MPP_RET ret = MPP_OK;

    for (RK_S32 i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        if (f->refresh_frame_flags & (1 << i)) {
            s->ref_s[i].valid = 1;
            s->ref_s[i].frame_id = f->current_frame_id;
            s->ref_s[i].upscaled_width = s->upscaled_width;
            s->ref_s[i].frame_width = s->frame_width;
            s->ref_s[i].frame_height = s->frame_height;
            s->ref_s[i].render_width = s->render_width;
            s->ref_s[i].render_height = s->render_height;
            s->ref_s[i].frame_type = f->frame_type;
            s->ref_s[i].subsampling_x = s->seq_header->color_config.subsampling_x;
            s->ref_s[i].subsampling_y = s->seq_header->color_config.subsampling_y;
            s->ref_s[i].bit_depth = s->bit_depth;
            s->ref_s[i].order_hint = s->order_hint;

            memcpy(s->ref_s[i].loop_filter_ref_deltas, f->loop_filter_ref_deltas,
                   sizeof(f->loop_filter_ref_deltas));
            memcpy(s->ref_s[i].loop_filter_mode_deltas, f->loop_filter_mode_deltas,
                   sizeof(f->loop_filter_mode_deltas));
            memcpy(s->ref_s[i].feature_enabled, f->feature_enabled,
                   sizeof(f->feature_enabled));
            memcpy(s->ref_s[i].feature_value, f->feature_value,
                   sizeof(f->feature_value));
        }
    }

    if (!f->show_existing_frame) {
        RK_S32 lst2_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_LAST2 - AV1_REF_FRAME_LAST];
        RK_S32 lst3_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_LAST3 - AV1_REF_FRAME_LAST];
        RK_S32 gld_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_GOLDEN - AV1_REF_FRAME_LAST];
        RK_S32 alt_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_ALTREF - AV1_REF_FRAME_LAST];
        RK_S32 lst_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_LAST - AV1_REF_FRAME_LAST];
        RK_S32 bwd_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_BWDREF - AV1_REF_FRAME_LAST];
        RK_S32 alt2_buf_idx = f->ref_frame_idx[AV1_REF_FRAME_ALTREF2 - AV1_REF_FRAME_LAST];

        s->cur_frame.ref->lst2_frame_offset = s->ref[lst2_buf_idx].order_hint;
        s->cur_frame.ref->lst3_frame_offset = s->ref[lst3_buf_idx].order_hint;
        s->cur_frame.ref->gld_frame_offset  = s->ref[gld_buf_idx].order_hint;
        s->cur_frame.ref->alt_frame_offset  = s->ref[alt_buf_idx].order_hint;
        s->cur_frame.ref->lst_frame_offset  = s->ref[lst_buf_idx].order_hint;
        s->cur_frame.ref->bwd_frame_offset  = s->ref[bwd_buf_idx].order_hint;
        s->cur_frame.ref->alt2_frame_offset = s->ref[alt2_buf_idx].order_hint;
    }

    for (RK_S32 i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        av1d_dbg(AV1D_DBG_REF, "header->refresh_frame_flags = %d",
                 f->refresh_frame_flags);
        if (f->refresh_frame_flags & (1 << i)) {
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
    return MPP_OK;
}

static MPP_RET set_current_frame(Av1DecCtx *ctx)
{
    Av1Codec *s = ctx->priv_data;
    AV1FrameIInfo *frame = &s->cur_frame;
    const AV1FrameHeader *f = s->frame_header;
    RK_U32 value;

    if (frame->ref)
        av1d_frame_unref(ctx, frame);

    mpp_frame_set_meta(frame->f, NULL);
    mpp_frame_set_width(frame->f, s->frame_width);
    mpp_frame_set_height(frame->f, s->frame_height);

    /*
     * rk3588, user can set 8bit for 10bit source.
     * If user config 8bit output when input video is 10bti,
     * here should set hor_stride according to 8bit.
     */
    if (MPP_FRAME_FMT_IS_YUV_10BIT(ctx->pix_fmt))
        mpp_frame_set_hor_stride(frame->f, MPP_ALIGN(s->frame_width * s->bit_depth / 8, 8));
    else
        mpp_frame_set_hor_stride(frame->f, MPP_ALIGN(s->frame_width, 8));

    mpp_frame_set_ver_stride(frame->f, MPP_ALIGN(s->frame_height, 8));
    mpp_frame_set_errinfo(frame->f, 0);
    mpp_frame_set_discard(frame->f, 0);
    mpp_frame_set_pts(frame->f, s->pts);
    mpp_frame_set_dts(frame->f, s->dts);

    mpp_frame_set_color_trc(frame->f, ctx->color_trc);
    mpp_frame_set_color_primaries(frame->f, ctx->color_primaries);
    mpp_frame_set_colorspace(frame->f, ctx->colorspace);
    mpp_frame_set_color_range(frame->f, ctx->color_range);

    mpp_frame_set_mastering_display(frame->f, s->mastering_display);
    mpp_frame_set_content_light(frame->f, s->content_light);

    if (MPP_FRAME_FMT_IS_FBC(ctx->cfg->base.out_fmt)) {
        RK_U32 fbc_hdr_stride = MPP_ALIGN(ctx->width, 64);

        mpp_slots_set_prop(ctx->slots, SLOTS_HOR_ALIGN, mpp_align_16);
        if (s->bit_depth == 10) {
            if ((ctx->pix_fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV420SP ||
                (ctx->pix_fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV420SP_10BIT)
                ctx->pix_fmt = MPP_FMT_YUV420SP_10BIT;
            else
                mpp_err("422p 10bit no support");
        }

        ctx->pix_fmt |= ctx->cfg->base.out_fmt & (MPP_FRAME_FBC_MASK);
        mpp_frame_set_offset_x(frame->f, 0);
        mpp_frame_set_offset_y(frame->f, 0);
        if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3588)
            mpp_frame_set_ver_stride(frame->f, MPP_ALIGN(ctx->height, 8) + 28);

        if (*compat_ext_fbc_hdr_256_odd)
            fbc_hdr_stride = MPP_ALIGN(ctx->width, 256) | 256;

        mpp_frame_set_fbc_hdr_stride(frame->f, fbc_hdr_stride);
    } else if (MPP_FRAME_FMT_IS_TILE(ctx->cfg->base.out_fmt)) {
        ctx->pix_fmt |= ctx->cfg->base.out_fmt & (MPP_FRAME_TILE_FLAG);
    }

    if (s->is_hdr)
        ctx->pix_fmt |= MPP_FRAME_HDR;

    mpp_frame_set_fmt(frame->f, ctx->pix_fmt);

    if (ctx->cfg->base.enable_thumbnail && ctx->hw_info && ctx->hw_info->cap_down_scale)
        mpp_frame_set_thumbnail_en(frame->f, ctx->cfg->base.enable_thumbnail);
    else
        mpp_frame_set_thumbnail_en(frame->f, 0);

    value = 4;
    mpp_slots_set_prop(ctx->slots, SLOTS_NUMERATOR, &value);
    value = 1;
    mpp_slots_set_prop(ctx->slots, SLOTS_DENOMINATOR, &value);
    mpp_buf_slot_get_unused(ctx->slots, &frame->slot_index);
    av1d_dbg(AV1D_DBG_REF, "get frame->slot_index %d", frame->slot_index);
    mpp_buf_slot_set_prop(ctx->slots, frame->slot_index, SLOT_FRAME, frame->f);
    mpp_buf_slot_set_flag(ctx->slots, frame->slot_index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(ctx->slots, frame->slot_index, SLOT_HAL_OUTPUT);

    frame->ref = mpp_calloc(Av1RefInfo, 1);
    if (!frame->ref) {
        mpp_err("av1d ref info malloc fail");
        return MPP_ERR_NOMEM;
    }
    frame->ref->ref_count++;
    frame->ref->is_intra_frame = !f->frame_type;
    frame->ref->intra_only = (f->frame_type == 2);
    frame->ref->is_output = 0;
    frame->ref->invisible = (!f->show_frame && !f->showable_frame);
    set_current_global_motion_params(s);
    set_current_skip_mode_params(s);
    set_current_coded_lossless_param(s);
    set_current_read_grain_params(s);

    return MPP_OK;
}

static const RK_U32 av1_unit_types[] = {
    AV1_OBU_TEMPORAL_DELIMITER,
    AV1_OBU_SEQUENCE_HEADER,
    AV1_OBU_FRAME_HEADER,
    AV1_OBU_TILE_GROUP,
    AV1_OBU_METADATA,
    AV1_OBU_FRAME,
};

MPP_RET av1d_parser_init(Av1DecCtx *ctx, ParserCfg *init)
{
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;
    av1d_dbg_func("enter ctx %p\n", ctx);
    Av1Codec *s = mpp_calloc(Av1Codec, 1);
    ctx->priv_data = (void*)s;
    if (!ctx->priv_data) {
        mpp_err("av1d codec context malloc fail");
        return MPP_ERR_NOMEM;
    }

    s->seq_ref = mpp_calloc(AV1SeqHeader, 1);

    s->unit_types = av1_unit_types;
    s->nb_unit_types = MPP_ARRAY_ELEMS(av1_unit_types);
    ctx->packet_slots = init->packet_slots;
    ctx->slots = init->frame_slots;
    ctx->cfg = init->cfg;
    ctx->hw_info = init->hw_info;
    mpp_buf_slot_setup(ctx->slots, 25);

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
    av1d_set_default_cdfs(s->cdfs, s->cdfs_ndvc);

    return MPP_OK;

    av1d_dbg_func("leave ctx %p\n", ctx);

    return ret;
}

MPP_RET av1d_parser_deinit(Av1DecCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;

    Av1Codec *s = ctx->priv_data;
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

    av1d_fragment_reset(&s->current_obu);
    MPP_FREE(s->current_obu.units);
    MPP_FREE(s->frame_header_data);
    MPP_FREE(s->seq_ref);
    MPP_FREE((s->hdr_dynamic_meta));
    MPP_FREE(ctx->priv_data);
    return MPP_OK;

    av1d_dbg_func("leave ctx %p\n", ctx);

    return ret;
}

static const unsigned char av1_remap_lr_type[4] = {
    AV1_RESTORE_NONE,
    AV1_RESTORE_SWITCHABLE,
    AV1_RESTORE_WIENER,
    AV1_RESTORE_SGRPROJ
};
static MPP_RET av1d_parser2_syntax(Av1DecCtx *ctx)
{
    RK_S32 i, j, loop_cnt, uses_lr;
    RK_U8 is_rk3588;
    Av1Codec *h = ctx->priv_data;
    const AV1SeqHeader *seq = h->seq_header;
    const AV1FrameHeader *frame_header = h->frame_header;
    const AV1FilmGrainParams *film_grain = &h->cur_frame.film_grain;
    RK_S32 apply_grain = film_grain->apply_grain;

    DXVA_PicParams_AV1 *pp = &ctx->pic_params;
    memset(pp, 0, sizeof(*pp));

    pp->width  = h->frame_width;
    pp->height = h->frame_height;

    pp->max_width  = seq->max_frame_width_minus_1 + 1;
    pp->max_height = seq->max_frame_height_minus_1 + 1;

    pp->CurrPic.Index7Bits  = h->cur_frame.slot_index;
    pp->CurrPicTextureIndex = h->cur_frame.slot_index;
    pp->superres_denom      = frame_header->use_superres ? frame_header->coded_denom : AV1_SUPERRES_NUM;
    pp->use_superres        = frame_header->use_superres;
    pp->bitdepth            = h->bit_depth;
    pp->seq_profile         = seq->seq_profile;
    pp->frame_header_size   = h->frame_header_size;

    /* Tiling info */
    pp->tiles.cols = frame_header->tile_cols;
    pp->tiles.rows = frame_header->tile_rows;
    pp->tiles.context_update_id = frame_header->context_update_tile_id;

    for (i = 0; i < pp->tiles.cols; i++)
        pp->tiles.widths[i] = frame_header->width_in_sbs_minus_1[i] + 1;

    for (i = 0; i < pp->tiles.rows; i++)
        pp->tiles.heights[i] = frame_header->height_in_sbs_minus_1[i] + 1;

    for (i = 0; i < AV1_MAX_TILES; i++) {
        pp->tiles.tile_offset_start[i] = h->tile_offset_start[i];
        pp->tiles.tile_offset_end[i] = h->tile_offset_end[i];
    }

    pp->tiles.tile_sz_mag = h->frame_header->tile_size_bytes_minus1;
    /* Coding tools */
    pp->coding.current_operating_point      = seq->operating_point_idc[h->operating_point_idc];
    pp->coding.use_128x128_superblock       = seq->use_128x128_superblock;
    pp->coding.intra_edge_filter            = seq->enable_intra_edge_filter;
    pp->coding.interintra_compound          = seq->enable_interintra_compound;
    pp->coding.masked_compound              = seq->enable_masked_compound;
    pp->coding.warped_motion                = frame_header->allow_warped_motion;
    pp->coding.dual_filter                  = seq->enable_dual_filter;
    pp->coding.jnt_comp                     = seq->enable_jnt_comp;
    pp->coding.screen_content_tools         = frame_header->allow_screen_content_tools;
    pp->coding.integer_mv                   = frame_header->force_integer_mv;

    pp->coding.cdef_en                      = seq->enable_cdef;
    pp->coding.restoration                  = seq->enable_restoration;
    pp->coding.film_grain_en                = seq->film_grain_params_present  ;//&& !(avctx->export_side_data & AV_CODEC_EXPORT_DATA_FILM_GRAIN);
    pp->coding.intrabc                      = frame_header->allow_intrabc;
    pp->coding.high_precision_mv            = frame_header->allow_high_precision_mv;
    pp->coding.switchable_motion_mode       = frame_header->is_motion_mode_switchable;
    pp->coding.filter_intra                 = seq->enable_filter_intra;
    pp->coding.disable_frame_end_update_cdf = frame_header->disable_frame_end_update_cdf;
    pp->coding.disable_cdf_update           = frame_header->disable_cdf_update;
    pp->coding.reference_mode               = frame_header->reference_select;
    pp->coding.skip_mode                    = frame_header->skip_mode_present;
    pp->coding.reduced_tx_set               = frame_header->reduced_tx_set;
    pp->coding.superres                     = frame_header->use_superres;
    pp->coding.tx_mode                      = frame_header->tx_mode;
    pp->coding.use_ref_frame_mvs            = frame_header->use_ref_frame_mvs;
    pp->coding.enable_ref_frame_mvs         = seq->enable_ref_frame_mvs;
    pp->coding.reference_frame_update       = 1; // 0 for show_existing_frame with key frames, but those are not passed to the hwaccel
    pp->coding.error_resilient_mode         = frame_header->error_resilient_mode;

    /* Format & Picture Info flags */
    pp->format.frame_type     = frame_header->frame_type;
    pp->format.show_frame     = frame_header->show_frame;
    pp->format.showable_frame = frame_header->showable_frame;
    pp->format.subsampling_x  = seq->color_config.subsampling_x;
    pp->format.subsampling_y  = seq->color_config.subsampling_y;
    pp->format.mono_chrome    = seq->color_config.mono_chrome;
    pp->coded_lossless        = h->cur_frame.coded_lossless;
    pp->all_lossless          = h->all_lossless;
    /* References */
    pp->primary_ref_frame = frame_header->primary_ref_frame;
    pp->enable_order_hint = seq->enable_order_hint;
    pp->order_hint        = frame_header->order_hint;
    pp->order_hint_bits   = seq->enable_order_hint ? seq->order_hint_bits_minus_1 + 1 : 0;

    pp->ref_frame_valued = frame_header->ref_frame_valued;
    for (i = 0; i < AV1_REFS_PER_FRAME; i++)
        pp->ref_frame_idx[i] = frame_header->ref_frame_idx[i];

    memset(pp->RefFrameMapTextureIndex, 0xFF, sizeof(pp->RefFrameMapTextureIndex));
    is_rk3588 = mpp_get_soc_type() == ROCKCHIP_SOC_RK3588;
    loop_cnt = is_rk3588 ? AV1_REFS_PER_FRAME : AV1_NUM_REF_FRAMES;
    for (i = 0; i < loop_cnt; i++) {
        int8_t ref_idx = frame_header->ref_frame_idx[i];
        AV1FrameIInfo *ref_frame;
        Av1RefInfo *ref_i;

        if (is_rk3588)
            ref_frame = &h->ref[ref_idx];
        else
            ref_frame = &h->ref[i];
        ref_i = ref_frame->ref;

        if (ref_frame->f) {
            pp->frame_refs[i].width  = mpp_frame_get_width(ref_frame->f);
            pp->frame_refs[i].height = mpp_frame_get_height(ref_frame->f);;
        }
        pp->frame_refs[i].Index  = ref_frame->slot_index;
        pp->frame_refs[i].order_hint = ref_frame->order_hint;
        if (ref_i) {
            pp->frame_refs[i].lst_frame_offset = ref_i->lst_frame_offset;
            pp->frame_refs[i].lst2_frame_offset = ref_i->lst2_frame_offset;
            pp->frame_refs[i].lst3_frame_offset = ref_i->lst3_frame_offset;
            pp->frame_refs[i].gld_frame_offset = ref_i->gld_frame_offset;
            pp->frame_refs[i].bwd_frame_offset = ref_i->bwd_frame_offset;
            pp->frame_refs[i].alt2_frame_offset = ref_i->alt2_frame_offset;
            pp->frame_refs[i].alt_frame_offset = ref_i->alt_frame_offset ;
            pp->frame_refs[i].is_intra_frame = ref_i->is_intra_frame;
            pp->frame_refs[i].intra_only = ref_i->intra_only;
        }
        /* Global Motion */
        pp->frame_refs[i].wminvalid = (h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].wmtype == AV1_WARP_MODEL_IDENTITY);
        pp->frame_refs[i].wmtype    = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].wmtype;
        for (j = 0; j < 6; ++j) {
            pp->frame_refs[i].wmmat[j] = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].wmmat[j];
            pp->frame_refs[i].wmmat_val[j] = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].wmmat_val[j];
        }
        pp->frame_refs[i].alpha = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].alpha;
        pp->frame_refs[i].beta = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].beta;
        pp->frame_refs[i].gamma = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].gamma;
        pp->frame_refs[i].delta = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i].delta;
    }
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        AV1FrameIInfo *ref_frame = &h->ref[i];

        pp->frame_ref_state[i].valid           = h->ref_s[i].valid         ;
        pp->frame_ref_state[i].frame_id        = h->ref_s[i].frame_id      ;
        pp->frame_ref_state[i].upscaled_width  = h->ref_s[i].upscaled_width;
        pp->frame_ref_state[i].frame_width     = h->ref_s[i].frame_width   ;
        pp->frame_ref_state[i].frame_height    = h->ref_s[i].frame_height  ;
        pp->frame_ref_state[i].render_width    = h->ref_s[i].render_width  ;
        pp->frame_ref_state[i].render_height   = h->ref_s[i].render_height ;
        pp->frame_ref_state[i].frame_type      = h->ref_s[i].frame_type    ;
        pp->frame_ref_state[i].subsampling_x   = h->ref_s[i].subsampling_x ;
        pp->frame_ref_state[i].subsampling_y   = h->ref_s[i].subsampling_y ;
        pp->frame_ref_state[i].bit_depth       = h->ref_s[i].bit_depth     ;
        pp->frame_ref_state[i].order_hint      = h->ref_s[i].order_hint    ;

        pp->ref_order_hint[i] = frame_header->ref_order_hint[i];

        if (ref_frame->slot_index < 0x7f)
            pp->RefFrameMapTextureIndex[i] = ref_frame->slot_index;
        else
            pp->RefFrameMapTextureIndex[i] = 0xff;
    }

    /* Loop filter parameters */
    pp->loop_filter.filter_level[0]        = frame_header->loop_filter_level[0];
    pp->loop_filter.filter_level[1]        = frame_header->loop_filter_level[1];
    pp->loop_filter.filter_level_u         = frame_header->loop_filter_level[2];
    pp->loop_filter.filter_level_v         = frame_header->loop_filter_level[3];
    pp->loop_filter.sharpness_level        = frame_header->loop_filter_sharpness;
    pp->loop_filter.mode_ref_delta_enabled = frame_header->loop_filter_delta_enabled;
    pp->loop_filter.mode_ref_delta_update  = frame_header->loop_filter_delta_update;
    pp->loop_filter.delta_lf_multi         = frame_header->delta_lf_multi;
    pp->loop_filter.delta_lf_present       = frame_header->delta_lf_present;
    pp->loop_filter.delta_lf_res           = frame_header->delta_lf_res;

    for (i = 0; i < AV1_TOTAL_REFS_PER_FRAME; i++) {
        pp->loop_filter.ref_deltas[i] = frame_header->loop_filter_ref_deltas[i];
    }

    pp->loop_filter.mode_deltas[0]                = frame_header->loop_filter_mode_deltas[0];
    pp->loop_filter.mode_deltas[1]                = frame_header->loop_filter_mode_deltas[1];
    pp->loop_filter.frame_restoration_type[0]     = av1_remap_lr_type[frame_header->lr_type[0]];
    pp->loop_filter.frame_restoration_type[1]     = av1_remap_lr_type[frame_header->lr_type[1]];
    pp->loop_filter.frame_restoration_type[2]     = av1_remap_lr_type[frame_header->lr_type[2]];
    uses_lr = frame_header->lr_type[0] || frame_header->lr_type[1] || frame_header->lr_type[2];
    pp->loop_filter.log2_restoration_unit_size[0] = uses_lr ? (1 + frame_header->lr_unit_shift) : 3;
    pp->loop_filter.log2_restoration_unit_size[1] = uses_lr ? (1 + frame_header->lr_unit_shift - frame_header->lr_uv_shift) : 3;
    pp->loop_filter.log2_restoration_unit_size[2] = uses_lr ? (1 + frame_header->lr_unit_shift - frame_header->lr_uv_shift) : 3;

    /* Quantization */
    pp->quantization.delta_q_present = frame_header->delta_q_present;
    pp->quantization.delta_q_res     = frame_header->delta_q_res;
    pp->quantization.base_qindex     = frame_header->base_q_idx;
    pp->quantization.y_dc_delta_q    = frame_header->delta_q_y_dc;
    pp->quantization.u_dc_delta_q    = frame_header->delta_q_u_dc;
    pp->quantization.v_dc_delta_q    = frame_header->delta_q_v_dc;
    pp->quantization.u_ac_delta_q    = frame_header->delta_q_u_ac;
    pp->quantization.v_ac_delta_q    = frame_header->delta_q_v_ac;
    pp->quantization.using_qmatrix   = frame_header->using_qmatrix;
    pp->quantization.qm_y            = frame_header->using_qmatrix ? frame_header->qm_y : 0xFF;
    pp->quantization.qm_u            = frame_header->using_qmatrix ? frame_header->qm_u : 0xFF;
    pp->quantization.qm_v            = frame_header->using_qmatrix ? frame_header->qm_v : 0xFF;

    /* Cdef parameters */
    pp->cdef.damping = frame_header->cdef_damping_minus_3;
    pp->cdef.bits    = frame_header->cdef_bits;
    for (i = 0; i < 8; i++) {
        pp->cdef.y_strengths[i].primary    = frame_header->cdef_y_pri_strength[i];
        pp->cdef.y_strengths[i].secondary  = frame_header->cdef_y_sec_strength[i];
        pp->cdef.uv_strengths[i].primary   = frame_header->cdef_uv_pri_strength[i];
        pp->cdef.uv_strengths[i].secondary = frame_header->cdef_uv_sec_strength[i];
    }

    /* Misc flags */
    pp->interp_filter = frame_header->interpolation_filter;

    /* Segmentation */
    pp->segmentation.enabled         = frame_header->segmentation_enabled;
    pp->segmentation.update_map      = frame_header->segmentation_update_map;
    pp->segmentation.update_data     = frame_header->segmentation_update_data;
    pp->segmentation.temporal_update = frame_header->segmentation_temporal_update;
    for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
        for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
            pp->segmentation.feature_mask[i]      |= frame_header->feature_enabled[i][j] << j;
            pp->segmentation.feature_data[i][j]    = frame_header->feature_value[i][j];
        }
    }
    pp->segmentation.last_active     = frame_header->segmentation_id_last_active;
    pp->segmentation.preskip         = frame_header->segmentation_id_preskip;

    /* Film grain */
    pp->film_grain.matrix_coefficients      = seq->color_config.matrix_coefficients;
    if (apply_grain) {
        pp->film_grain.apply_grain              = 1;
        pp->film_grain.scaling_shift_minus8     = film_grain->grain_scaling_minus_8;
        pp->film_grain.chroma_scaling_from_luma = film_grain->chroma_scaling_from_luma;
        pp->film_grain.ar_coeff_lag             = film_grain->ar_coeff_lag;
        pp->film_grain.ar_coeff_shift_minus6    = film_grain->ar_coeff_shift_minus_6;
        pp->film_grain.grain_scale_shift        = film_grain->grain_scale_shift;
        pp->film_grain.overlap_flag             = film_grain->overlap_flag;
        pp->film_grain.clip_to_restricted_range = film_grain->clip_to_restricted_range;
        pp->film_grain.matrix_coeff_is_identity = (seq->color_config.matrix_coefficients == MPP_FRAME_SPC_RGB);

        pp->film_grain.grain_seed               = film_grain->grain_seed;
        pp->film_grain.update_grain             = film_grain->update_grain;
        pp->film_grain.num_y_points             = film_grain->num_y_points;
        for (i = 0; i < film_grain->num_y_points; i++) {
            pp->film_grain.scaling_points_y[i][0] = film_grain->point_y_value[i];
            pp->film_grain.scaling_points_y[i][1] = film_grain->point_y_scaling[i];
        }
        pp->film_grain.num_cb_points            = film_grain->num_cb_points;
        for (i = 0; i < film_grain->num_cb_points; i++) {
            pp->film_grain.scaling_points_cb[i][0] = film_grain->point_cb_value[i];
            pp->film_grain.scaling_points_cb[i][1] = film_grain->point_cb_scaling[i];
        }
        pp->film_grain.num_cr_points            = film_grain->num_cr_points;
        for (i = 0; i < film_grain->num_cr_points; i++) {
            pp->film_grain.scaling_points_cr[i][0] = film_grain->point_cr_value[i];
            pp->film_grain.scaling_points_cr[i][1] = film_grain->point_cr_scaling[i];
        }
        for (i = 0; i < 24; i++) {
            pp->film_grain.ar_coeffs_y[i] = film_grain->ar_coeffs_y_plus_128[i];
        }
        for (i = 0; i < 25; i++) {
            pp->film_grain.ar_coeffs_cb[i] = film_grain->ar_coeffs_cb_plus_128[i];
            pp->film_grain.ar_coeffs_cr[i] = film_grain->ar_coeffs_cr_plus_128[i];
        }
        pp->film_grain.cb_mult      = film_grain->cb_mult;
        pp->film_grain.cb_luma_mult = film_grain->cb_luma_mult;
        pp->film_grain.cr_mult      = film_grain->cr_mult;
        pp->film_grain.cr_luma_mult = film_grain->cr_luma_mult;
        pp->film_grain.cb_offset    = film_grain->cb_offset;
        pp->film_grain.cr_offset    = film_grain->cr_offset;
        pp->film_grain.cr_offset    = film_grain->cr_offset;
    }
    pp->upscaled_width = h->upscaled_width;
    pp->frame_to_show_map_idx = frame_header->frame_to_show_map_idx;
    pp->show_existing_frame = frame_header->show_existing_frame;
    pp->frame_tag_size = h->frame_tag_size;
    pp->skip_ref0      = h->skip_ref0;
    pp->skip_ref1      = h->skip_ref1;
    pp->refresh_frame_flags = frame_header->refresh_frame_flags;

    pp->cdfs = h->cdfs;
    pp->cdfs_ndvc = h->cdfs_ndvc;
    pp->tile_cols_log2 = frame_header->tile_cols_log2;
    pp->tile_rows_log2 = frame_header->tile_rows_log2;

    return MPP_OK;
}

MPP_RET av1d_parser_frame(Av1DecCtx *ctx, HalDecTask *task)
{
    RK_U8 *data = NULL;
    RK_S32 size = 0;
    Av1Codec *s = ctx->priv_data;
    AV1TileGroup *raw_tile_group = NULL;
    MPP_RET ret = MPP_OK;


    av1d_dbg_func("enter ctx %p\n", ctx);
    task->valid = 0;

    data = (RK_U8 *)mpp_packet_get_pos(ctx->pkt);
    size = (RK_S32)mpp_packet_get_length(ctx->pkt);

    s->pts = mpp_packet_get_pts(ctx->pkt);
    s->dts = mpp_packet_get_dts(ctx->pkt);

    s->current_obu.data = data;
    s->current_obu.data_size = size;
    s->tile_offset = 0;
    ret = av1d_get_fragment_units(&s->current_obu);
    if (ret < 0) {
        return ret;
    }
    ret = av1d_read_fragment_uints(s, &s->current_obu);
    if (ret < 0) {
        return ret;
    }

    for (RK_S32 i = 0; i < s->current_obu.nb_units; i++) {
        Av1ObuUnit *unit = &s->current_obu.units[i];
        AV1OBU *obu = &unit->obu;
        const AV1OBUHeader *header = &obu->header;
        if (!unit->data || !unit->data_size)
            continue;
        switch (unit->type) {
        case AV1_OBU_SEQUENCE_HEADER:
            memcpy(s->seq_ref, &obu->payload.seq_header, sizeof(AV1SeqHeader));
            s->seq_header = s->seq_ref;
            ret = av1d_set_context_with_seq(ctx, s->seq_header);
            if (ret < 0) {
                mpp_err_f("Failed to set context.\n");
                s->seq_header = NULL;
                goto end;
            }

            s->operating_point_idc = s->seq_header->operating_point_idc[s->operating_point];

            ret = get_pixel_format(ctx);
            if (ret < 0) {
                mpp_err_f("Failed to get pixel format.\n");
                s->seq_header = NULL;
                goto end;
            }
            break;
        case AV1_OBU_REDUNDANT_FRAME_HEADER:
            if (s->frame_header)
                break;
            // fall-through
        case AV1_OBU_FRAME:
        case AV1_OBU_FRAME_HEADER:
            if (!s->seq_header) {
                mpp_err_f("Missing Sequence Header.\n");
                ret = MPP_ERR_VALUE;
                goto end;
            }

            if (unit->type == AV1_OBU_FRAME)
                s->frame_header = &obu->payload.frame.header;
            else
                s->frame_header = &obu->payload.frame_header;

            if (s->frame_header->show_existing_frame) {
                if (s->cur_frame.ref) {
                    av1d_frame_unref(ctx, &s->cur_frame);
                }

                ret = av1d_frame_ref(ctx, &s->cur_frame,
                                     &s->ref[s->frame_header->frame_to_show_map_idx]);

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

                s->frame_header = NULL;

                goto end;
            }
            ret = set_current_frame(ctx);
            if (ret < 0) {
                mpp_err_f("Get current frame error\n");
                goto end;
            }
            s->cur_frame.spatial_id  = header->spatial_id;
            s->cur_frame.temporal_id = header->temporal_id;
            s->cur_frame.order_hint  = s->frame_header->order_hint;

            if (unit->type != AV1_OBU_FRAME)
                break;
            // fall-through
        case AV1_OBU_TILE_GROUP:
            if (!s->frame_header) {
                mpp_err_f("Missing Frame Header.\n");
                ret = MPP_ERR_VALUE;
                goto end;
            }

            if (unit->type == AV1_OBU_FRAME)
                raw_tile_group = &obu->payload.frame.tile_group;
            else
                raw_tile_group = &obu->payload.tile_group;

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
        // the last tile in the tile group
        if (raw_tile_group && (s->tile_num == raw_tile_group->tg_end + 1)) {
            RK_U32 j;

            av1d_parser2_syntax(ctx);
            task->syntax.data = (void*)&ctx->pic_params;
            task->syntax.number = 1;
            task->valid = 1;
            task->output = s->cur_frame.slot_index;
            task->input_packet = ctx->pkt;

            for (j = 0; j < AV1_REFS_PER_FRAME; j++) {
                int8_t ref_idx = s->frame_header->ref_frame_idx[j];
                if (s->ref[ref_idx].slot_index < 0x7f) {
                    mpp_buf_slot_set_flag(ctx->slots, s->ref[ref_idx].slot_index, SLOT_HAL_INPUT);
                    MppFrame mframe = NULL;
                    task->refer[j] = s->ref[ref_idx].slot_index;
                    mpp_buf_slot_get_prop(ctx->slots, task->refer[j], SLOT_FRAME_PTR, &mframe);
                    if (mframe)
                        task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
                } else {
                    task->refer[j] = -1;
                }
            }
            ret = update_reference_list(ctx);
            if (ret < 0) {
                mpp_err_f("Failed to update reference list.\n");
                goto end;
            }

            if (s->frame_header->show_frame) {
                ret = set_output_frame(ctx);
                if (ret < 0) {
                    mpp_err_f("Set output frame error\n");
                    goto end;
                }
            }
            raw_tile_group = NULL;
            s->frame_header = NULL;
        }
    }

    if (s->eos) {
        task->flags.eos = 1;
    }
end:
    av1d_fragment_reset(&s->current_obu);
    if (ret < 0)
        s->frame_header = NULL;

    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;
}

MPP_RET av1d_get_cdfs(Av1Codec *ctx, RK_U32 ref_idx)
{
    ctx->cdfs = &ctx->cdfs_last[ref_idx];
    ctx->cdfs_ndvc = &ctx->cdfs_last_ndvc[ref_idx];

    return MPP_OK;
}

MPP_RET av1d_store_cdfs(Av1Codec *ctx, RK_U32 refresh_frame_flags)
{
    RK_U32 i;

    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        if (refresh_frame_flags & (1 << i)) {
            if (&ctx->cdfs_last[i] != ctx->cdfs) {
                ctx->cdfs_last[i] = *ctx->cdfs;
                ctx->cdfs_last_ndvc[i] = *ctx->cdfs_ndvc;
            }
        }
    }

    return MPP_OK;
}

MPP_RET av1d_parser_update(Av1DecCtx *ctx, void *info)
{
    Av1Codec* c_ctx = (Av1Codec*)ctx->priv_data;
    DecCbHalDone *cb = (DecCbHalDone*)info;
    RK_U8 *data = (RK_U8*)cb->task;
    RK_U32 i;
    const RK_U32 mv_cdf_offset = offsetof(AV1CDFs, mv_cdf);
    const RK_U32 mv_cdf_size = sizeof(Av1MvCDFs);
    const RK_U32 mv_cdf_end_offset = mv_cdf_offset + mv_cdf_size;
    const RK_U32 cdf_size = sizeof(AV1CDFs);

    av1d_dbg_func("enter ctx %p\n", ctx);
    if (!c_ctx->disable_frame_end_update_cdf) {
        for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
            if (c_ctx->refresh_frame_flags & (1 << i)) {
                /* 1. get cdfs */
                av1d_get_cdfs(c_ctx, i);
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
                av1d_store_cdfs(c_ctx, c_ctx->refresh_frame_flags);
                break;
            }
        }
    }

    av1d_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

MPP_RET av1d_paser_reset(Av1DecCtx *ctx)
{
    (void)ctx;
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;
    Av1Codec *s = ctx->priv_data;

    av1d_dbg_func("enter ctx %p\n", ctx);
    for ( i = 0; i < MPP_ARRAY_ELEMS(s->ref); i++) {
        AV1FrameIInfo *f = &s->ref[i];

        if (f->ref)
            av1d_frame_unref(ctx, &s->ref[i]);
    }

    if (s->cur_frame.ref) {
        av1d_frame_unref(ctx, &s->cur_frame);
    }

    ctx->frame_header_cnt = 0;
    ctx->stream_offset = 0;
    ctx->eos = 0;

    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;
}

RK_S32 av1d_split_frame(Av1DecCtx *ctx,
                        RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 data_size)
{
    RK_S32 err = MPP_NOK;
    uint8_t *ptr = data, *end = data + data_size;
    av1d_dbg_func("enter ctx %p\n", ctx);

    *out_data = data;

    while (ptr < end) {
        AV1OBUHeader m_hdr;
        BitReadCtx_t m_gb, *gb = &m_gb;
        RK_U64 obu_size = 0, obu_length = 0;

        memset(&m_hdr, 0, sizeof(m_hdr));
        mpp_set_bitread_ctx(gb, ptr, data_size);

        err = av1d_read_obu_header(gb, &m_hdr);
        if (err < 0)
            break;
        if (m_hdr.obu_has_size_field)
            obu_size = m_hdr.obu_filed_size;
        else
            obu_size = data_size - 1 - m_hdr.obu_extension_flag;

        obu_length = mpp_get_bits_count(gb) / 8 + obu_size;
        if (obu_length > (RK_U64)data_size)
            break;

        av1d_dbg(AV1D_DBG_STRMIN, "obu_type: %d, temporal_id: %d, spatial_id: %d, payload size: %d\n",
                 m_hdr.obu_type, m_hdr.temporal_id, m_hdr.spatial_id, obu_size);

        if (m_hdr.obu_type == AV1_OBU_FRAME_HEADER ||
            m_hdr.obu_type == AV1_OBU_FRAME ||
            ((m_hdr.obu_type == AV1_OBU_TEMPORAL_DELIMITER ||
              m_hdr.obu_type == AV1_OBU_METADATA ||
              m_hdr.obu_type == AV1_OBU_SEQUENCE_HEADER)
             && ctx->frame_header_cnt))
            ctx->frame_header_cnt ++;
        if (ctx->frame_header_cnt == 2) {
            *out_size = (RK_S32)(ptr - data);
            ctx->is_new_frame = 1;
            ctx->frame_header_cnt = 0;
            return ptr - data;
        }
        if (m_hdr.obu_type == AV1_OBU_FRAME) {
            ptr += obu_length;
            data_size -= obu_length;
            *out_size = (RK_S32)(ptr - data);
            ctx->is_new_frame = 1;
            ctx->frame_header_cnt = 0;
            return ptr - data;
        }
        ptr += obu_length;
        data_size -= obu_length;
    }
    RK_S32 out_len = (RK_S32)(ptr - data);
    *out_size = out_len;

    av1d_dbg_func("leave ctx %p\n", ctx);
    return out_len;
}

MPP_RET av1d_get_frame_stream(Av1DecCtx *ctx, RK_U8 *buf, RK_S32 length)
{
    MPP_RET ret = MPP_OK;
    av1d_dbg_func("enter ctx %p\n", ctx);
    RK_S32 buff_size = 0;
    RK_U8 *data = NULL;
    RK_S32 size = 0;
    RK_S32 offset = ctx->stream_offset;

    data = (RK_U8 *)mpp_packet_get_data(ctx->pkt);
    size = (RK_S32)mpp_packet_get_size(ctx->pkt);

    if ((length + offset) > size) {
        mpp_packet_deinit(&ctx->pkt);
        buff_size = length + offset + 10 * 1024;
        data = mpp_realloc(data, RK_U8, buff_size);
        mpp_packet_init(&ctx->pkt, (void *)data, buff_size);
        mpp_packet_set_size(ctx->pkt, buff_size);
        ctx->stream_size = buff_size;
    }

    memcpy(data + offset, buf, length);
    ctx->stream_offset += length;
    mpp_packet_set_length(ctx->pkt, ctx->stream_offset);

    av1d_dbg_func("leave ctx %p\n", ctx);
    return ret;
}

MPP_RET av1d_set_context_with_seq(Av1DecCtx *ctx, const AV1SeqHeader *seq)
{
    RK_S32 width = seq->max_frame_width_minus_1 + 1;
    RK_S32 height = seq->max_frame_height_minus_1 + 1;

    ctx->profile = seq->seq_profile;
    ctx->level = seq->seq_level_idx[0];

    ctx->color_range =
        seq->color_config.color_range ? MPP_FRAME_RANGE_JPEG : MPP_FRAME_RANGE_MPEG;
    ctx->color_primaries = seq->color_config.color_primaries;
    ctx->colorspace = seq->color_config.matrix_coefficients;
    ctx->color_trc = seq->color_config.transfer_characteristics;

    switch (seq->color_config.chroma_sample_position) {
    case AV1_CSP_VERTICAL:
        ctx->chroma_sample_location = MPP_CHROMA_LOC_LEFT;
        break;
    case AV1_CSP_COLOCATED:
        ctx->chroma_sample_location =  MPP_CHROMA_LOC_TOPLEFT;
        break;
    }
    // Update width and height if they have changed.
    if (ctx->width != width || ctx->height != height) {
        ctx->width = width;
        ctx->height = height;
    }
    return MPP_OK;
}
