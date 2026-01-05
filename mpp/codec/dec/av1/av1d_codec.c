/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "av1d_codec"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_bitread.h"
#include "mpp_bitwrite.h"
#include "rk_hdr_meta_com.h"

#include "av1d_parser.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647      /* maximum (signed) RK_S32 value */
#endif

#define BUFFER_PADDING_SIZE 64

static MPP_RET read_ns(BitReadCtx_t *gb, RK_U32 n, RK_U32 *out_val)
{
    RK_U32 w = mpp_log2(n) + 1;
    RK_U32 m = (1 << w) - n;
    RK_U32 value = 0;
    RK_S32 v;

    if (w - 1 > 0)
        READ_BITS(gb, w - 1, &v);
    else
        v = 0;

    if (v < m) {
        value = v;
    } else {
        RK_S32 extra_bit;
        READ_ONEBIT(gb, &extra_bit);
        value = (v << 1) - m + extra_bit;
    }

    *out_val = value;
    return MPP_OK;

__BITREAD_ERR:
    return MPP_NOK;

}

/*
 * Reads an incrementally encoded value from the bitstream using a unary coding variant.
 * The function reads consecutive 1 bits from the bitstream, incrementing a value starting
 * from range_min, until a 0 bit is encountered or the value reaches range_max.
 */
static MPP_RET read_increment(BitReadCtx_t *gb, RK_U32 range_min,
                              RK_U32 range_max, RK_U32 *out_val)
{
    RK_U32 value = range_min;
    RK_S32 tmp;

    while (value < range_max) {
        mpp_read_bits(gb, 1, &tmp);
        if (!tmp) break;
        value++;
    }

    *out_val = value;
    return MPP_OK;
}

static MPP_RET read_signed_subexp(BitReadCtx_t *gb, RK_U32 range_max, RK_U32 *out_val)
{
    RK_U32 max_len = mpp_log2(range_max - 1) - 3;
    RK_U32 value = 0;
    RK_U32 len, range_offset, range_bits;

    read_increment(gb, 0, max_len, &len);
    if (len) {
        range_bits   = 2 + len;
        range_offset = 1 << range_bits;
    } else {
        range_bits   = 3;
        range_offset = 0;
    }

    if (len < max_len) {
        READ_BITS_LONG(gb, range_bits, &value);
    } else {
        MPP_RET ret = read_ns(gb, range_max - range_offset, &value);

        if (ret < 0)
            return ret;
    }
    value += range_offset;

    *out_val = value;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_NOK;
}

RK_S32 av1d_get_relative_dist(const AV1SeqHeader *seq, RK_U32 a, RK_U32 b)
{
    RK_U32 diff = 0;

    if (seq->enable_order_hint) {
        RK_U32 m = 1 << seq->order_hint_bits_minus_1;

        diff = a - b;
        diff = (diff & (m - 1)) - (diff & m);
    }
    return diff;
}

static MPP_RET read_leb128(BitReadCtx_t *gb, RK_U64 *out_value)
{
    RK_U64 value = 0;
    RK_S32 byte = 0;
    RK_S32 i;

    for (i = 0; i < 8; i++) { // kMaximumLeb128Size = 8
        READ_BITS(gb, 8, &byte);
        value |= (RK_U64)(byte & 0x7f) << (i * 7);
        if (!(byte & 0x80))
            break;
    }
    // Fail on values larger than 32-bits to ensure consistent behavior on
    // 32 and 64 bit targets: value is typically used to determine buffer
    // allocation size.
    if (value > UINT32_MAX)
        return MPP_NOK;

    *out_value = value;
    return MPP_OK;

__BITREAD_ERR:
    return MPP_NOK;
}

MPP_RET av1d_read_obu_header(BitReadCtx_t *gb, AV1OBUHeader *hdr)
{
    READ_ONEBIT(gb, &hdr->obu_forbidden_bit);
    READ_BITS(gb, 4, &hdr->obu_type);
    READ_ONEBIT(gb, &hdr->obu_extension_flag);
    READ_ONEBIT(gb, &hdr->obu_has_size_field);
    READ_ONEBIT(gb, &hdr->obu_reserved_1bit);

    if (hdr->obu_extension_flag) {
        READ_BITS(gb, 3, &hdr->temporal_id);
        READ_BITS(gb, 2, &hdr->spatial_id);
        READ_BITS(gb, 3, &hdr->extension_header_reserved_3bits);
    } else {
        hdr->temporal_id = 0;
        hdr->spatial_id = 0;
    }

    if (hdr->obu_has_size_field)
        read_leb128(gb, &hdr->obu_filed_size);

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_color_config(Av1Codec *ctx, BitReadCtx_t *gb,
                                 AV1ColorConfig *cfg, RK_S32 seq_profile)
{
    READ_ONEBIT(gb, &cfg->high_bitdepth);

    if (seq_profile == AV1_PROFILE_PROFESSIONAL &&
        cfg->high_bitdepth) {
        READ_ONEBIT(gb, &cfg->twelve_bit);
        ctx->bit_depth = cfg->twelve_bit ? 12 : 10;
    } else {
        ctx->bit_depth = cfg->high_bitdepth ? 10 : 8;
    }

    if (seq_profile == AV1_PROFILE_HIGH)
        cfg->mono_chrome = 0;
    else
        READ_ONEBIT(gb, &cfg->mono_chrome);
    ctx->num_planes = cfg->mono_chrome ? 1 : 3;

    READ_ONEBIT(gb, &cfg->color_description_present_flag);
    if (cfg->color_description_present_flag) {
        READ_BITS(gb, 8, &cfg->color_primaries);
        READ_BITS(gb, 8, &cfg->transfer_characteristics);
        READ_BITS(gb, 8, &cfg->matrix_coefficients);
        if (cfg->transfer_characteristics == MPP_FRAME_TRC_BT2020_10 ||
            cfg->transfer_characteristics == MPP_FRAME_TRC_SMPTEST2084)
            ctx->is_hdr = 1;
    } else {
        cfg->color_primaries          = MPP_FRAME_PRI_UNSPECIFIED;
        cfg->transfer_characteristics = MPP_FRAME_TRC_UNSPECIFIED;
        cfg->matrix_coefficients      = MPP_FRAME_SPC_UNSPECIFIED;
    }

    if (cfg->mono_chrome) {
        READ_ONEBIT(gb, &cfg->color_range);

        cfg->subsampling_x = 1;
        cfg->subsampling_y = 1;
        cfg->chroma_sample_position = AV1_CSP_UNKNOWN;
        cfg->separate_uv_delta_q = 0;

    } else if (cfg->color_primaries          == MPP_FRAME_PRI_BT709 &&
               cfg->transfer_characteristics == MPP_FRAME_TRC_IEC61966_2_1 &&
               cfg->matrix_coefficients      == MPP_FRAME_SPC_RGB) {
        cfg->color_range = 1;
        cfg->subsampling_x = 0;
        cfg->subsampling_y = 0;
        READ_ONEBIT(gb, &cfg->separate_uv_delta_q);

    } else {
        READ_ONEBIT(gb, &cfg->color_range);

        if (seq_profile == AV1_PROFILE_MAIN) {
            cfg->subsampling_x = 1;
            cfg->subsampling_y = 1;
        } else if (seq_profile == AV1_PROFILE_HIGH) {
            cfg->subsampling_x = 0;
            cfg->subsampling_y = 0;
        } else {
            if (ctx->bit_depth == 12) {
                READ_ONEBIT(gb, &cfg->subsampling_x);
                if (cfg->subsampling_x)
                    READ_ONEBIT(gb, &cfg->subsampling_y);
                else
                    cfg->subsampling_y = 0;
            } else {
                cfg->subsampling_x = 1;
                cfg->subsampling_y = 0;
            }
        }
        if (cfg->subsampling_x && cfg->subsampling_y) {
            READ_BITS(gb, 2, &cfg->chroma_sample_position);
        }

        READ_ONEBIT(gb, &cfg->separate_uv_delta_q);
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_timing_info(Av1Codec *ctx, BitReadCtx_t *gb,
                                AV1TimingInfo *info)
{
    READ_BITS_LONG(gb, 32, &info->num_units_in_display_tick);
    READ_BITS_LONG(gb, 32, &info->time_scale);

    READ_ONEBIT(gb, &info->equal_picture_interval);
    if (info->equal_picture_interval)
        READ_UE(gb, &info->num_ticks_per_picture_minus_1);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_model_info(Av1Codec *ctx, BitReadCtx_t *gb,
                               AV1DecoderModelInfo *info)
{
    READ_BITS(gb, 5, &info->buffer_delay_length_minus_1);
    READ_BITS_LONG(gb, 32, &info->num_units_in_decoding_tick);
    READ_BITS(gb, 5, &info->buffer_removal_time_length_minus_1);
    READ_BITS(gb, 5, &info->frame_presentation_time_length_minus_1);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_sequence_header_obu(Av1Codec *ctx, BitReadCtx_t *gb,
                                        AV1SeqHeader *seq)
{
    RK_S32 i;

    READ_BITS(gb, 3, &seq->seq_profile);
    READ_ONEBIT(gb, &seq->still_picture);
    READ_ONEBIT(gb, &seq->reduced_still_picture_header);

    if (seq->reduced_still_picture_header) {
        seq->timing_info_present_flag           = 0;
        seq->decoder_model_info_present_flag    = 0;
        seq->initial_display_delay_present_flag = 0;
        seq->operating_points_cnt_minus_1       = 0;
        seq->operating_point_idc[0]             = 0;

        READ_BITS(gb, 5, &seq->seq_level_idx[0]);

        seq->seq_tier[0] = 0;
        seq->decoder_model_present_for_this_op[0] = 0;
        seq->initial_display_delay_present_for_this_op[0] = 0;

    } else {
        READ_ONEBIT(gb, &seq->timing_info_present_flag);
        if (seq->timing_info_present_flag) {
            FUN_CHECK(read_timing_info(ctx, gb, &seq->timing_info));

            READ_ONEBIT(gb, &seq->decoder_model_info_present_flag);
            if (seq->decoder_model_info_present_flag) {
                FUN_CHECK(read_model_info(ctx, gb, &seq->decoder_model_info));
            }
        } else {
            seq->decoder_model_info_present_flag = 0;
        }

        READ_ONEBIT(gb, &seq->initial_display_delay_present_flag);

        READ_BITS(gb, 5, &seq->operating_points_cnt_minus_1);
        for (i = 0; i <= seq->operating_points_cnt_minus_1; i++) {
            READ_BITS(gb, 12, &seq->operating_point_idc[i]);
            READ_BITS(gb, 5, &seq->seq_level_idx[i]);

            if (seq->seq_level_idx[i] > 7)
                READ_ONEBIT(gb, &seq->seq_tier[i]);
            else
                seq->seq_tier[i] = 0;

            if (seq->decoder_model_info_present_flag) {
                READ_ONEBIT(gb, &seq->decoder_model_present_for_this_op[i]);
                if (seq->decoder_model_present_for_this_op[i]) {
                    RK_S32 n = seq->decoder_model_info.buffer_delay_length_minus_1 + 1;
                    READ_BITS_LONG(gb, n, &seq->decoder_buffer_delay[i]);
                    READ_BITS_LONG(gb, n, &seq->encoder_buffer_delay[i]);
                    READ_ONEBIT(gb, &seq->low_delay_mode_flag[i]);
                }
            } else {
                seq->decoder_model_present_for_this_op[i] = 0;
            }

            if (seq->initial_display_delay_present_flag) {
                READ_ONEBIT(gb, &seq->initial_display_delay_present_for_this_op[i]);
                if (seq->initial_display_delay_present_for_this_op[i])
                    READ_BITS(gb, 4, &seq->initial_display_delay_minus_1[i]);
            }
        }
    }

    READ_BITS(gb, 4, &seq->frame_width_bits_minus_1);
    READ_BITS(gb, 4, &seq->frame_height_bits_minus_1);

    READ_BITS_LONG(gb, seq->frame_width_bits_minus_1  + 1, &seq->max_frame_width_minus_1);
    READ_BITS_LONG(gb, seq->frame_height_bits_minus_1 + 1, &seq->max_frame_height_minus_1);

    if (seq->reduced_still_picture_header)
        seq->frame_id_numbers_present_flag = 0;
    else
        READ_ONEBIT(gb, &seq->frame_id_numbers_present_flag);
    if (seq->frame_id_numbers_present_flag) {
        READ_BITS(gb, 4, &seq->delta_frame_id_length_minus_2);
        READ_BITS(gb, 3, &seq->additional_frame_id_length_minus_1);
    }

    READ_ONEBIT(gb, &seq->use_128x128_superblock);
    READ_ONEBIT(gb, &seq->enable_filter_intra);
    READ_ONEBIT(gb, &seq->enable_intra_edge_filter);

    if (seq->reduced_still_picture_header) {
        seq->enable_interintra_compound = 0;
        seq->enable_masked_compound     = 0;
        seq->enable_warped_motion       = 0;
        seq->enable_dual_filter         = 0;
        seq->enable_order_hint          = 0;
        seq->enable_jnt_comp            = 0;
        seq->enable_ref_frame_mvs       = 0;
        seq->seq_force_screen_content_tools = AV1_SELECT_SCREEN_CONTENT_TOOLS;
        seq->seq_force_integer_mv = AV1_SELECT_INTEGER_MV;
    } else {
        READ_ONEBIT(gb, &seq->enable_interintra_compound);
        READ_ONEBIT(gb, &seq->enable_masked_compound);
        READ_ONEBIT(gb, &seq->enable_warped_motion);
        READ_ONEBIT(gb, &seq->enable_dual_filter);

        READ_ONEBIT(gb, &seq->enable_order_hint);
        if (seq->enable_order_hint) {
            READ_ONEBIT(gb, &seq->enable_jnt_comp);
            READ_ONEBIT(gb, &seq->enable_ref_frame_mvs);
        } else {
            seq->enable_jnt_comp            = 0;
            seq->enable_ref_frame_mvs       = 0;
        }

        READ_ONEBIT(gb, &seq->seq_choose_screen_content_tools);
        if (seq->seq_choose_screen_content_tools)
            seq->seq_force_screen_content_tools = AV1_SELECT_SCREEN_CONTENT_TOOLS;
        else
            READ_ONEBIT(gb, &seq->seq_force_screen_content_tools);
        if (seq->seq_force_screen_content_tools > 0) {
            READ_ONEBIT(gb, &seq->seq_choose_integer_mv);
            if (seq->seq_choose_integer_mv)
                seq->seq_force_integer_mv = AV1_SELECT_INTEGER_MV;
            else
                READ_ONEBIT(gb, &seq->seq_force_integer_mv);
        } else {
            seq->seq_force_integer_mv = AV1_SELECT_INTEGER_MV;
        }

        if (seq->enable_order_hint)
            READ_BITS(gb, 3, &seq->order_hint_bits_minus_1);
    }

    READ_ONEBIT(gb, &seq->enable_superres);
    READ_ONEBIT(gb, &seq->enable_cdef);
    READ_ONEBIT(gb, &seq->enable_restoration);

    FUN_CHECK(read_color_config(ctx, gb, &seq->color_config, seq->seq_profile));

    READ_ONEBIT(gb, &seq->film_grain_params_present);

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_temporal_delimiter_obu(Av1Codec *ctx, BitReadCtx_t *gb)
{
    ctx->seen_frame_header = 0;
    // TODO

    (void)gb;
    return MPP_OK;
}

static const RK_U8 av1_ref_frame_list[AV1_NUM_REF_FRAMES - 2] = {
    AV1_REF_FRAME_LAST2, AV1_REF_FRAME_LAST3, AV1_REF_FRAME_BWDREF,
    AV1_REF_FRAME_ALTREF2, AV1_REF_FRAME_ALTREF
};
static MPP_RET set_frame_refs(Av1Codec *ctx, AV1FrameHeader *f)
{
    RK_S32 i, j;
    RK_S8 ref_frame_idx[AV1_REFS_PER_FRAME];
    RK_S8 used_frame[AV1_NUM_REF_FRAMES];
    RK_S8 shifted_order_hints[AV1_NUM_REF_FRAMES];
    RK_S32 cur_frame_hint, latest_order_hint, earliest_order_hint, ref;
    const AV1SeqHeader *seq = ctx->seq_header;

    for (i = 0; i < AV1_REFS_PER_FRAME; i++)
        ref_frame_idx[i] = -1;
    ref_frame_idx[AV1_REF_FRAME_LAST - AV1_REF_FRAME_LAST] = f->last_frame_idx;
    ref_frame_idx[AV1_REF_FRAME_GOLDEN - AV1_REF_FRAME_LAST] = f->golden_frame_idx;

    /*
     * An array usedFrame marking which reference frames
     * have been used is prepared as follows:
     */
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++)
        used_frame[i] = 0;
    used_frame[f->last_frame_idx] = 1;
    used_frame[f->golden_frame_idx] = 1;

    /*
     * An array shiftedOrderHints (containing the expected output order shifted
     * such that the current frame has hint equal to curFrameHint) is prepared as follows:
     */
    cur_frame_hint = 1 << (seq->order_hint_bits_minus_1);
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++)
        shifted_order_hints[i] = cur_frame_hint +
                                 av1d_get_relative_dist(seq, ctx->ref_s[i].order_hint, ctx->order_hint);

    latest_order_hint = shifted_order_hints[f->last_frame_idx];
    earliest_order_hint = shifted_order_hints[f->golden_frame_idx];

    /* find_latest_backward */
    ref = -1;
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        RK_S32 hint = shifted_order_hints[i];
        if (!used_frame[i] && hint >= cur_frame_hint &&
            (ref < 0 || hint >= latest_order_hint)) {
            ref = i;
            latest_order_hint = hint;
        }
    }
    /*
     * The ALTREF_FRAME reference is set to be a backward reference to the frame
     * with highest output order as follows:
     */
    if (ref >= 0) {
        ref_frame_idx[AV1_REF_FRAME_ALTREF - AV1_REF_FRAME_LAST] = ref;
        used_frame[ref] = 1;
    }

    /* find_earliest_backward */
    ref = -1;
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        RK_S32 hint = shifted_order_hints[i];
        if (!used_frame[i] && hint >= cur_frame_hint &&
            (ref < 0 || hint < earliest_order_hint)) {
            ref = i;
            earliest_order_hint = hint;
        }
    }
    /*
     * The BWDREF_FRAME reference is set to be a backward reference to
     * the closest frame as follows:
     */
    if (ref >= 0) {
        ref_frame_idx[AV1_REF_FRAME_BWDREF - AV1_REF_FRAME_LAST] = ref;
        used_frame[ref] = 1;
    }

    ref = -1;
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        RK_S32 hint = shifted_order_hints[i];
        if (!used_frame[i] && hint >= cur_frame_hint &&
            (ref < 0 || hint < earliest_order_hint)) {
            ref = i;
            earliest_order_hint = hint;
        }
    }

    /*
     * The ALTREF2_FRAME reference is set to the next closest
     * backward reference as follows:
     */
    if (ref >= 0) {
        ref_frame_idx[AV1_REF_FRAME_ALTREF2 - AV1_REF_FRAME_LAST] = ref;
        used_frame[ref] = 1;
    }

    /*
     * The remaining references are set to be forward references
     * in anti-chronological order as follows:
     */
    for (i = 0; i < AV1_REFS_PER_FRAME - 2; i++) {
        RK_S32 ref_frame = av1_ref_frame_list[i];
        if (ref_frame_idx[ref_frame - AV1_REF_FRAME_LAST] < 0 ) {
            /* find_latest_forward */
            ref = -1;
            for (j = 0; j < AV1_NUM_REF_FRAMES; j++) {
                RK_S32 hint = shifted_order_hints[j];
                if (!used_frame[j] && hint < cur_frame_hint &&
                    (ref < 0 || hint >= latest_order_hint)) {
                    ref = j;
                    latest_order_hint = hint;
                }
            }
            if (ref >= 0) {
                ref_frame_idx[ref_frame - AV1_REF_FRAME_LAST] = ref;
                used_frame[ref] = 1;
            }
        }
    }

    /*
     * Finally, any remaining references are set to the reference
     * frame with smallest output order as follows:
     */
    ref = -1;
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        RK_S32 hint = shifted_order_hints[i];
        if (ref < 0 || hint < earliest_order_hint) {
            ref = i;
            earliest_order_hint = hint;
        }
    }
    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        if (ref_frame_idx[i] < 0)
            ref_frame_idx[i] = ref;
        f->ref_frame_idx[i] = ref_frame_idx[i];
    }

    return MPP_OK;
}

static MPP_RET read_superres_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                    AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 denom;

    if (seq->enable_superres)
        READ_ONEBIT(gb, &f->use_superres);
    else
        f->use_superres = 0;

    if (f->use_superres) {
        READ_BITS(gb, 3, &f->coded_denom);
        denom = f->coded_denom + AV1_SUPERRES_DENOM_MIN;
    } else {
        denom = AV1_SUPERRES_NUM;
    }

    ctx->upscaled_width = ctx->frame_width;
    ctx->frame_width = (ctx->upscaled_width * AV1_SUPERRES_NUM + denom / 2) / denom;

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_frame_size(Av1Codec *ctx, BitReadCtx_t *gb,
                               AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;

    if (f->frame_size_override_flag) {
        READ_BITS_LONG(gb, seq->frame_width_bits_minus_1 + 1,  &f->frame_width_minus_1);
        READ_BITS_LONG(gb, seq->frame_height_bits_minus_1 + 1, &f->frame_height_minus_1);
    } else {
        f->frame_width_minus_1  = seq->max_frame_width_minus_1;
        f->frame_height_minus_1 = seq->max_frame_height_minus_1;
    }

    ctx->frame_width  = f->frame_width_minus_1  + 1;
    ctx->frame_height = f->frame_height_minus_1 + 1;

    FUN_CHECK(read_superres_params(ctx, gb, f));

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_render_size(Av1Codec *ctx, BitReadCtx_t *gb,
                                AV1FrameHeader *f)
{
    READ_ONEBIT(gb, &f->render_and_frame_size_different);
    if (f->render_and_frame_size_different) {
        READ_BITS(gb, 16, &f->render_width_minus_1);
        READ_BITS(gb, 16, &f->render_height_minus_1);
    } else {
        f->render_width_minus_1  = f->frame_width_minus_1;
        f->render_height_minus_1 = f->frame_height_minus_1;
    }

    ctx->render_width  = f->render_width_minus_1  + 1;
    ctx->render_height = f->render_height_minus_1 + 1;

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_frame_size_with_refs(Av1Codec *ctx, BitReadCtx_t *gb,
                                         AV1FrameHeader *f)
{
    RK_S32 i;

    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        READ_ONEBIT(gb, &f->found_ref[i]);
        if (f->found_ref[i]) {
            AV1RefFrameState *ref = &ctx->ref_s[f->ref_frame_idx[i]];

            if (!ref->valid) {
                mpp_err_f("Missing reference frame, ref = %d, ref_frame_idx = %d.\n",
                          i, f->ref_frame_idx[i]);
                return MPP_ERR_PROTOL;
            }

            f->frame_width_minus_1  = ref->upscaled_width - 1;
            f->frame_height_minus_1 = ref->frame_height - 1;
            f->render_width_minus_1  = ref->render_width - 1;
            f->render_height_minus_1 = ref->render_height - 1;

            ctx->upscaled_width = ref->upscaled_width;
            ctx->frame_width    = ctx->upscaled_width;
            ctx->frame_height   = ref->frame_height;
            ctx->render_width   = ref->render_width;
            ctx->render_height  = ref->render_height;
            break;
        }
    }

    if (i >= AV1_REFS_PER_FRAME) {
        FUN_CHECK(read_frame_size(ctx, gb, f));
        FUN_CHECK(read_render_size(ctx, gb, f));
    } else {
        FUN_CHECK(read_superres_params(ctx, gb, f));
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_interpolation_filter(Av1Codec *ctx, BitReadCtx_t *gb,
                                         AV1FrameHeader *f)
{
    READ_ONEBIT(gb, &f->is_filter_switchable);
    if (f->is_filter_switchable)
        f->interpolation_filter = AV1_INTERP_SWITCHABLE;
    else
        READ_BITS(gb, 2, &f->interpolation_filter);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static RK_S32 tile_log2(RK_S32 blksize, RK_S32 target)
{
    RK_S32 k;

    for (k = 0; (blksize << k) < target; k++);
    return k;
}

static MPP_RET read_tile_info_max_tile(Av1Codec *ctx, BitReadCtx_t *gb,
                                       AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 max_tile_width_sb, max_tile_height_sb, max_tile_area_sb;
    RK_S32 min_log2_tile_cols, max_log2_tile_cols, max_log2_tile_rows;
    RK_S32 min_log2_tiles, min_log2_tile_rows;
    RK_S32 sb_rows, sb_cols;
    RK_S32 i;

    // get tile limits
    {
        RK_S32 mi_cols = 2 * ((ctx->frame_width  + 7) >> 3);
        RK_S32 mi_rows = 2 * ((ctx->frame_height + 7) >> 3);
        RK_S32 sb_size = seq->use_128x128_superblock ? 7 : 6;

        sb_cols = seq->use_128x128_superblock ? ((mi_cols + 31) >> 5) : ((mi_cols + 15) >> 4);
        sb_rows = seq->use_128x128_superblock ? ((mi_rows + 31) >> 5) : ((mi_rows + 15) >> 4);
        max_tile_width_sb = AV1_MAX_TILE_WIDTH >> sb_size;
        max_tile_area_sb  = AV1_MAX_TILE_AREA  >> (2 * sb_size);

        min_log2_tile_cols = tile_log2(max_tile_width_sb, sb_cols);
        max_log2_tile_cols = tile_log2(1, MPP_MIN(sb_cols, AV1_MAX_TILE_COLS));
        max_log2_tile_rows = tile_log2(1, MPP_MIN(sb_rows, AV1_MAX_TILE_ROWS));
        min_log2_tiles = MPP_MAX(min_log2_tile_cols, tile_log2(max_tile_area_sb, sb_rows * sb_cols));
    }

    READ_ONEBIT(gb, &f->uniform_tile_spacing_flag);
    av1d_dbg(AV1D_DBG_HEADER, "uniform_tile_spacing_flag=%d\n", f->uniform_tile_spacing_flag);
    if (f->uniform_tile_spacing_flag) {
        RK_S32 tile_width_sb, tile_height_sb;
        RK_U32 val;

        read_increment(gb, min_log2_tile_cols, max_log2_tile_cols, &val);
        f->tile_cols_log2 = (RK_U8)val;

        tile_width_sb = (sb_cols + (1 << f->tile_cols_log2) - 1) >> f->tile_cols_log2;
        f->tile_cols = (sb_cols + tile_width_sb - 1) / tile_width_sb;

        min_log2_tile_rows = MPP_MAX(min_log2_tiles - f->tile_cols_log2, 0);
        read_increment(gb, min_log2_tile_rows, max_log2_tile_rows, &val);
        f->tile_rows_log2 = (RK_U8)val;

        tile_height_sb = (sb_rows + (1 << f->tile_rows_log2) - 1) >>
                         f->tile_rows_log2;
        f->tile_rows = (sb_rows + tile_height_sb - 1) / tile_height_sb;

        for (i = 0; i < f->tile_cols - 1; i++)
            f->width_in_sbs_minus_1[i] = tile_width_sb - 1;
        f->width_in_sbs_minus_1[i] = sb_cols - (f->tile_cols - 1) * tile_width_sb - 1;
        for (i = 0; i < f->tile_rows - 1; i++)
            f->height_in_sbs_minus_1[i] = tile_height_sb - 1;
        f->height_in_sbs_minus_1[i] = sb_rows - (f->tile_rows - 1) * tile_height_sb - 1;
    } else {
        RK_S32 widest_tile_sb = 0;
        RK_S32 start_sb = 0;
        RK_S32 size_sb;

        for (i = 0; start_sb < sb_cols && i < AV1_MAX_TILE_COLS; i++) {
            RK_S32 max_width = MPP_MIN(sb_cols - start_sb, max_tile_width_sb);
            RK_U32 val = 0;

            read_ns(gb, max_width, &val);
            f->width_in_sbs_minus_1[i] = (RK_U8)val;
            size_sb = f->width_in_sbs_minus_1[i] + 1;
            widest_tile_sb = MPP_MAX(size_sb, widest_tile_sb);
            start_sb += size_sb;
        }
        f->tile_cols_log2 = tile_log2(1, i);
        f->tile_cols = i;

        if (min_log2_tiles > 0)
            max_tile_area_sb = (sb_rows * sb_cols) >> (min_log2_tiles + 1);
        else
            max_tile_area_sb = sb_rows * sb_cols;
        max_tile_height_sb = MPP_MAX(max_tile_area_sb / widest_tile_sb, 1);

        start_sb = 0;
        for (i = 0; start_sb < sb_rows && i < AV1_MAX_TILE_ROWS; i++) {
            RK_S32 max_height = MPP_MIN(sb_rows - start_sb, max_tile_height_sb);
            RK_U32 val = 0;

            read_ns(gb, max_height, &val);
            f->height_in_sbs_minus_1[i] = (RK_U8)val;
            size_sb = f->height_in_sbs_minus_1[i] + 1;
            start_sb += size_sb;
        }
        f->tile_rows_log2 = tile_log2(1, i);
        f->tile_rows = i;
    }

    if (f->tile_cols_log2 > 0 || f->tile_rows_log2 > 0) {
        READ_BITS(gb, f->tile_cols_log2 + f->tile_rows_log2,
                  &f->context_update_tile_id);
        READ_BITS(gb, 2, &f->tile_size_bytes_minus1);
    } else {
        f->context_update_tile_id = 0;
        f->tile_size_bytes_minus1 = 3;
    }

    ctx->tile_cols = f->tile_cols;
    ctx->tile_rows = f->tile_rows;

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_quantization_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                        AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 flag;

    READ_BITS(gb, 8, &f->base_q_idx);

    f->delta_q_y_dc = 0;
    READ_ONEBIT(gb, &flag);
    if (flag)
        READ_SIGNBITS(gb, 7, &f->delta_q_y_dc);

    if (ctx->num_planes > 1) {
        if (seq->color_config.separate_uv_delta_q)
            READ_ONEBIT(gb, &f->diff_uv_delta);
        else
            f->diff_uv_delta = 0;

        f->delta_q_u_dc = 0;
        READ_ONEBIT(gb, &flag);
        if (flag)
            READ_SIGNBITS(gb, 7, &f->delta_q_u_dc);
        f->delta_q_u_ac = 0;
        READ_ONEBIT(gb, &flag);
        if (flag)
            READ_SIGNBITS(gb, 7, &f->delta_q_u_ac);
        if (f->diff_uv_delta) {
            f->delta_q_v_dc = 0;
            READ_ONEBIT(gb, &flag);
            if (flag)
                READ_SIGNBITS(gb, 7, &f->delta_q_v_dc);
            f->delta_q_v_ac = 0;
            READ_ONEBIT(gb, &flag);
            if (flag)
                READ_SIGNBITS(gb, 7, &f->delta_q_v_ac);
        } else {
            f->delta_q_v_dc = f->delta_q_u_dc;
            f->delta_q_v_ac = f->delta_q_u_ac;
        }
    } else {
        f->delta_q_u_dc = 0;
        f->delta_q_u_ac = 0;
        f->delta_q_v_dc = 0;
        f->delta_q_v_ac = 0;
    }

    READ_ONEBIT(gb, &f->using_qmatrix);
    if (f->using_qmatrix) {
        READ_BITS(gb, 4, &f->qm_y);
        READ_BITS(gb, 4, &f->qm_u);
        if (seq->color_config.separate_uv_delta_q)
            READ_BITS(gb, 4, &f->qm_v);
        else
            f->qm_v = f->qm_u;
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_segmentation_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                        AV1FrameHeader *f)
{
    static const RK_U8 segment_bits[AV1_SEG_LVL_MAX] = { 8, 6, 6, 6, 6, 3, 0, 0 };
    static const RK_U8 segment_sign[AV1_SEG_LVL_MAX] = { 1, 1, 1, 1, 1, 0, 0, 0 };
    static const RK_U8 default_feature_enabled[AV1_SEG_LVL_MAX] = { 0 };
    static const RK_S16 default_feature_value[AV1_SEG_LVL_MAX] = { 0 };
    RK_S32 i, j;

    READ_ONEBIT(gb, &f->segmentation_enabled);

    if (f->segmentation_enabled) {
        if (f->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
            f->segmentation_update_map = 1;
            f->segmentation_temporal_update = 0;
            f->segmentation_update_data = 1;
        } else {
            READ_ONEBIT(gb, &f->segmentation_update_map);
            if (f->segmentation_update_map)
                READ_ONEBIT(gb, &f->segmentation_temporal_update);
            else
                f->segmentation_temporal_update = 0;
            READ_ONEBIT(gb, &f->segmentation_update_data);
        }

        for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
            const RK_U8 *ref_feature_enabled;
            const RK_S16 *ref_feature_value;

            if (f->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
                ref_feature_enabled = default_feature_enabled;
                ref_feature_value = default_feature_value;
            } else {
                ref_feature_enabled =
                    ctx->ref_s[f->ref_frame_idx[f->primary_ref_frame]].feature_enabled[i];
                ref_feature_value =
                    ctx->ref_s[f->ref_frame_idx[f->primary_ref_frame]].feature_value[i];
            }

            for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
                if (f->segmentation_update_data) {
                    READ_ONEBIT(gb, &f->feature_enabled[i][j]);
                    if (f->feature_enabled[i][j] && segment_bits[j] > 0) {
                        if (segment_sign[j]) {
                            RK_S32 sign_, data;

                            READ_ONEBIT(gb, &sign_);
                            READ_BITS(gb, segment_bits[j], &data);
                            if (sign_) data -= (1 << segment_bits[j]);
                            f->feature_value[i][j] = data;
                        } else
                            READ_BITS(gb, segment_bits[j], &f->feature_value[i][j]);
                    } else {
                        f->feature_value[i][j] = 0;
                    }
                } else {
                    f->feature_enabled[i][j] = ref_feature_enabled[j];
                    f->feature_value[i][j] = ref_feature_value[j];
                }
            }
        }
    } else {
        for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
            for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
                f->feature_enabled[i][j] = 0;
                f->feature_value[i][j] = 0;
            }
        }
    }

    f->segmentation_id_last_active = 0;
    f->segmentation_id_preskip = 0;
    for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
        for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
            if (f->feature_enabled[i][j]) {
                f->segmentation_id_last_active = i;
                if ( j > AV1_SEG_LVL_REF_FRAME)
                    f->segmentation_id_preskip = 1;
            }
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET read_delta_q_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                   AV1FrameHeader *f)
{
    if (f->base_q_idx > 0)
        READ_ONEBIT(gb, &f->delta_q_present);
    else
        f->delta_q_present = 0;

    if (f->delta_q_present)
        READ_BITS(gb, 2, &f->delta_q_res);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_delta_lf_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                    AV1FrameHeader *f)
{
    if (f->delta_q_present) {
        if (!f->allow_intrabc)
            READ_ONEBIT(gb, &f->delta_lf_present);
        else
            f->delta_lf_present = 0;
        if (f->delta_lf_present) {
            READ_BITS(gb, 2, &f->delta_lf_res);
            READ_ONEBIT(gb, &f->delta_lf_multi);
        } else {
            f->delta_lf_res = 0;
            f->delta_lf_multi = 0;
        }
    } else {
        f->delta_lf_present = 0;
        f->delta_lf_res = 0;
        f->delta_lf_multi = 0;
    }

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static const RK_S8 default_loop_filter_ref_deltas[AV1_TOTAL_REFS_PER_FRAME] =
{ 1, 0, 0, 0, -1, 0, -1, -1 };
static const RK_S8 default_loop_filter_mode_deltas[2] = { 0, 0 };

static MPP_RET read_loop_filter_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                       AV1FrameHeader *f)
{
    RK_S32 i;

    if (ctx->coded_lossless || f->allow_intrabc) {
        f->loop_filter_level[0] = 0;
        f->loop_filter_level[1] = 0;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_INTRA] = 1;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_LAST] = 0;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_LAST2] = 0;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_LAST3] = 0;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_BWDREF] = 0;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_GOLDEN] = -1;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_ALTREF] = -1;
        f->loop_filter_ref_deltas[AV1_REF_FRAME_ALTREF2] = -1;
        for (i = 0; i < 2; i++)
            f->loop_filter_mode_deltas[i] = 0;
        return MPP_OK;
    }

    READ_BITS(gb, 6, &f->loop_filter_level[0]);
    READ_BITS(gb, 6, &f->loop_filter_level[1]);

    if (ctx->num_planes > 1) {
        if (f->loop_filter_level[0] ||
            f->loop_filter_level[1]) {
            READ_BITS(gb, 6, &f->loop_filter_level[2]);
            READ_BITS(gb, 6, &f->loop_filter_level[3]);
        }
    }

    av1d_dbg(AV1D_DBG_HEADER, "orderhint %d loop_filter_level %d %d %d %d\n",
             f->order_hint,
             f->loop_filter_level[0], f->loop_filter_level[1],
             f->loop_filter_level[2], f->loop_filter_level[3]);
    READ_BITS(gb, 3, &f->loop_filter_sharpness);

    READ_ONEBIT(gb, &f->loop_filter_delta_enabled);
    if (f->loop_filter_delta_enabled) {
        const RK_S8 *ref_loop_filter_ref_deltas, *ref_loop_filter_mode_deltas;

        if (f->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
            ref_loop_filter_ref_deltas = default_loop_filter_ref_deltas;
            ref_loop_filter_mode_deltas = default_loop_filter_mode_deltas;
        } else {
            ref_loop_filter_ref_deltas =
                ctx->ref_s[f->ref_frame_idx[f->primary_ref_frame]].loop_filter_ref_deltas;
            ref_loop_filter_mode_deltas =
                ctx->ref_s[f->ref_frame_idx[f->primary_ref_frame]].loop_filter_mode_deltas;
        }

        READ_ONEBIT(gb, &f->loop_filter_delta_update);
        for (i = 0; i < AV1_TOTAL_REFS_PER_FRAME; i++) {
            if (f->loop_filter_delta_update)
                READ_ONEBIT(gb, &f->update_ref_delta[i]);
            else
                f->update_ref_delta[i] = 0;
            if (f->update_ref_delta[i])
                READ_SIGNBITS(gb, 7, &f->loop_filter_ref_deltas[i]);
            else
                f->loop_filter_ref_deltas[i] = ref_loop_filter_ref_deltas[i];
        }
        for (i = 0; i < 2; i++) {
            if (f->loop_filter_delta_update)
                READ_ONEBIT(gb, &f->update_mode_delta[i]);
            else
                f->update_mode_delta[i] = 0;
            if (f->update_mode_delta[i])
                READ_SIGNBITS(gb, 7, &f->loop_filter_mode_deltas[i]);
            else
                f->loop_filter_mode_deltas[i] = ref_loop_filter_mode_deltas[i];
        }
    } else {
        for (i = 0; i < AV1_TOTAL_REFS_PER_FRAME; i++)
            f->loop_filter_ref_deltas[i] = default_loop_filter_ref_deltas[i];
        for (i = 0; i < 2; i++)
            f->loop_filter_mode_deltas[i] = default_loop_filter_mode_deltas[i];
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_cdef_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 i;

    if (ctx->coded_lossless || f->allow_intrabc ||
        !seq->enable_cdef) {
        f->cdef_damping_minus_3 = 0;
        f->cdef_bits = 0;
        f->cdef_y_pri_strength[0] =  0;
        f->cdef_y_sec_strength[0] =  0;
        f->cdef_uv_pri_strength[0] = 0;
        f->cdef_uv_sec_strength[0] = 0;

        return MPP_OK;
    }

    READ_BITS(gb, 2, &f->cdef_damping_minus_3);
    READ_BITS(gb, 2, &f->cdef_bits);

    for (i = 0; i < (1 << f->cdef_bits); i++) {
        READ_BITS(gb, 4, &f->cdef_y_pri_strength[i]);
        READ_BITS(gb, 2, &f->cdef_y_sec_strength[i]);

        if (ctx->num_planes > 1) {
            READ_BITS(gb, 4, &f->cdef_uv_pri_strength[i]);
            READ_BITS(gb, 2, &f->cdef_uv_sec_strength[i]);
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_lr_params(Av1Codec *ctx, BitReadCtx_t *gb,
                              AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 uses_lr,  uses_chroma_lr;
    RK_S32 i;

    if (ctx->all_lossless || f->allow_intrabc ||
        !seq->enable_restoration) {
        return MPP_OK;
    }

    uses_lr = uses_chroma_lr = 0;
    for (i = 0; i < ctx->num_planes; i++) {
        READ_BITS(gb, 2, &f->lr_type[i]);

        if (f->lr_type[i] != AV1_RESTORE_NONE) {
            uses_lr = 1;
            if (i > 0)
                uses_chroma_lr = 1;
        }
    }

    if (uses_lr) {
        RK_U32 val = 0;

        if (seq->use_128x128_superblock)
            read_increment(gb, 1, 2, &val);
        else
            read_increment(gb, 0, 2, &val);
        f->lr_unit_shift = (RK_U8)val;

        if (seq->color_config.subsampling_x &&
            seq->color_config.subsampling_y && uses_chroma_lr) {
            READ_ONEBIT(gb, &f->lr_uv_shift);
        } else {
            f->lr_uv_shift = 0;
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_tx_mode(Av1Codec *ctx, BitReadCtx_t *gb,
                            AV1FrameHeader *f)
{
    if (ctx->coded_lossless)
        f->tx_mode = 0;
    else {
        READ_ONEBIT(gb, &f->tx_mode);
        f->tx_mode = f->tx_mode ? AV1_TX_MODE_SELECT : AV1_TX_MODE_LARGEST;
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_frame_reference_mode(Av1Codec *ctx, BitReadCtx_t *gb,
                                         AV1FrameHeader *f)
{
    if (f->frame_type == AV1_FRAME_INTRA_ONLY ||
        f->frame_type == AV1_FRAME_KEY)
        f->reference_select = 0;
    else
        READ_ONEBIT(gb, &f->reference_select);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_skip_mode_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                     AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_U8 skip_mode_allowed = 0;

    ctx->skip_ref0 = 0;
    ctx->skip_ref1 = 0;

    if (f->frame_type == AV1_FRAME_KEY ||
        f->frame_type == AV1_FRAME_INTRA_ONLY ||
        !f->reference_select || !seq->enable_order_hint) {
        f->skip_mode_present = 0;

        return MPP_OK;
    }

    /*
     * Determine whether skip mode is allowed.
     * If allowed, set skip_ref0 and skip_ref1.
     * skip_ref0 and skip_ref1 are the indices (plus 1) of the two
     * reference frames used in skip mode.
    */
    {
        RK_S32 forward_idx  = -1;
        RK_S32 backward_idx = -1;
        RK_S32 forward_hint = -1;
        RK_S32 backward_hint = -1;
        RK_S32 ref_hint, dist, i;

        for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
            ref_hint = ctx->ref_s[f->ref_frame_idx[i]].order_hint;
            dist = av1d_get_relative_dist(seq, ref_hint, ctx->order_hint);
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

        skip_mode_allowed = 0;
        if (forward_idx < 0) {
            skip_mode_allowed = 0;
        } else if (backward_idx >= 0) {
            skip_mode_allowed = 1;
            ctx->skip_ref0 = MPP_MIN(forward_idx, backward_idx) + 1;
            ctx->skip_ref1 = MPP_MAX(forward_idx, backward_idx) + 1;
        } else {
            RK_S32 second_forward_hint;
            RK_S32 second_forward_idx = -1;

            for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
                ref_hint = ctx->ref_s[f->ref_frame_idx[i]].order_hint;
                if (av1d_get_relative_dist(seq, ref_hint, forward_hint) < 0) {
                    if (second_forward_idx < 0 ||
                        av1d_get_relative_dist(seq, ref_hint, second_forward_hint) > 0) {
                        second_forward_idx  = i;
                        second_forward_hint = ref_hint;
                    }
                }
            }

            if (second_forward_idx < 0) {
                skip_mode_allowed = 0;
            } else {
                ctx->skip_ref0 = MPP_MIN(forward_idx, second_forward_idx) + 1;
                ctx->skip_ref1 = MPP_MAX(forward_idx, second_forward_idx) + 1;
                skip_mode_allowed = 1;
            }
        }
    }

    if (skip_mode_allowed)
        READ_ONEBIT(gb, &f->skip_mode_present);
    else
        f->skip_mode_present = 0;

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_gm_param(Av1Codec *ctx, BitReadCtx_t *gb,
                             AV1FrameHeader *f, RK_S32 type, RK_S32 ref, RK_S32 idx)
{
    RK_U32 abs_bits, prec_bits, num_syms;

    if (idx < 2) {
        if (type == AV1_WARP_MODEL_TRANSLATION) {
            abs_bits  = AV1_GM_ABS_TRANS_ONLY_BITS  - !f->allow_high_precision_mv;
            prec_bits = AV1_GM_TRANS_ONLY_PREC_BITS - !f->allow_high_precision_mv;
        } else {
            abs_bits  = AV1_GM_ABS_TRANS_BITS;
            prec_bits = AV1_GM_TRANS_PREC_BITS;
        }
    } else {
        abs_bits  = AV1_GM_ABS_ALPHA_BITS;
        prec_bits = AV1_GM_ALPHA_PREC_BITS;
    }

    num_syms = 2 * (1 << abs_bits) + 1;
    read_signed_subexp(gb, num_syms, &f->gm_params[ref][idx]);

    (void)prec_bits;
    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_global_motion_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                         AV1FrameHeader *f)
{
    RK_S32 ref, type;

    if (f->frame_type == AV1_FRAME_KEY ||
        f->frame_type == AV1_FRAME_INTRA_ONLY)
        return MPP_OK;

    for (ref = AV1_REF_FRAME_LAST; ref <= AV1_REF_FRAME_ALTREF; ref++) {
        READ_ONEBIT(gb, &f->is_global[ref]);
        if (f->is_global[ref]) {
            READ_ONEBIT(gb, &f->is_rot_zoom[ref]);
            if (f->is_rot_zoom[ref]) {
                type = AV1_WARP_MODEL_ROTZOOM;
            } else {
                READ_ONEBIT(gb, &f->is_translation[ref]);
                type = f->is_translation[ref] ? AV1_WARP_MODEL_TRANSLATION
                       : AV1_WARP_MODEL_AFFINE;
            }
        } else {
            type = AV1_WARP_MODEL_IDENTITY;
        }

        if (type >= AV1_WARP_MODEL_ROTZOOM) {
            FUN_CHECK(read_gm_param(ctx, gb, f, type, ref, 2));
            FUN_CHECK(read_gm_param(ctx, gb, f, type, ref, 3));
            if (type == AV1_WARP_MODEL_AFFINE) {
                FUN_CHECK(read_gm_param(ctx, gb, f, type, ref, 4));
                FUN_CHECK(read_gm_param(ctx, gb, f, type, ref, 5));
            } else {
                f->gm_params[ref][4] =  -f->gm_params[ref][3];
                f->gm_params[ref][5] =   f->gm_params[ref][2];
            }
        }
        if (type >= AV1_WARP_MODEL_TRANSLATION) {
            FUN_CHECK(read_gm_param(ctx, gb, f, type, ref, 0));
            FUN_CHECK(read_gm_param(ctx, gb, f, type, ref, 1));
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_film_grain_params(Av1Codec *ctx, BitReadCtx_t *gb,
                                      AV1FilmGrainParams *gm, AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 num_pos_luma, num_pos_chroma;
    RK_S32 i;

    if (!seq->film_grain_params_present ||
        (!f->show_frame && !f->showable_frame))
        return MPP_OK;

    READ_ONEBIT(gb, &gm->apply_grain);

    if (!gm->apply_grain)
        return MPP_OK;

    READ_BITS(gb, 16, &gm->grain_seed);

    if (f->frame_type == AV1_FRAME_INTER)
        READ_ONEBIT(gb, &gm->update_grain);
    else
        gm->update_grain = 1;

    if (!gm->update_grain) {
        READ_BITS(gb, 3, &gm->film_grain_params_ref_idx);
        return MPP_OK;
    }

    READ_BITS(gb, 4, &gm->num_y_points);
    for (i = 0; i < gm->num_y_points; i++) {
        READ_BITS(gb, 8, &gm->point_y_value[i]);
        READ_BITS(gb, 8, &gm->point_y_scaling[i]);
    }

    if (seq->color_config.mono_chrome)
        gm->chroma_scaling_from_luma = 0;
    else
        READ_ONEBIT(gb, &gm->chroma_scaling_from_luma);

    if (seq->color_config.mono_chrome ||
        gm->chroma_scaling_from_luma ||
        (seq->color_config.subsampling_x == 1 &&
         seq->color_config.subsampling_y == 1 &&
         gm->num_y_points == 0)) {
        gm->num_cb_points = 0;
        gm->num_cr_points = 0;
    } else {
        READ_BITS(gb, 4, &gm->num_cb_points);
        for (i = 0; i < gm->num_cb_points; i++) {
            READ_BITS(gb, 8, &gm->point_cb_value[i]);
            READ_BITS(gb, 8, &gm->point_cb_scaling[i]);
        }
        READ_BITS(gb, 4, &gm->num_cr_points);
        for (i = 0; i < gm->num_cr_points; i++) {
            READ_BITS(gb, 8, &gm->point_cr_value[i]);
            READ_BITS(gb, 8, &gm->point_cr_scaling[i]);
        }
    }

    READ_BITS(gb, 2, &gm->grain_scaling_minus_8);
    READ_BITS(gb, 2, &gm->ar_coeff_lag);
    num_pos_luma = 2 * gm->ar_coeff_lag * (gm->ar_coeff_lag + 1);
    if (gm->num_y_points) {
        num_pos_chroma = num_pos_luma + 1;
        for (i = 0; i < num_pos_luma; i++)
            READ_BITS(gb, 8, &gm->ar_coeffs_y_plus_128[i]);
    } else {
        num_pos_chroma = num_pos_luma;
    }
    if (gm->chroma_scaling_from_luma || gm->num_cb_points) {
        for (i = 0; i < num_pos_chroma; i++)
            READ_BITS(gb, 8, &gm->ar_coeffs_cb_plus_128[i]);
    }
    if (gm->chroma_scaling_from_luma || gm->num_cr_points) {
        for (i = 0; i < num_pos_chroma; i++)
            READ_BITS(gb, 8, &gm->ar_coeffs_cr_plus_128[i]);
    }
    READ_BITS(gb, 2, &gm->ar_coeff_shift_minus_6);
    READ_BITS(gb, 2, &gm->grain_scale_shift);
    if (gm->num_cb_points) {
        READ_BITS(gb, 8, &gm->cb_mult);
        READ_BITS(gb, 8, &gm->cb_luma_mult);
        READ_BITS(gb, 9, &gm->cb_offset);
    }
    if (gm->num_cr_points) {
        READ_BITS(gb, 8, &gm->cr_mult);
        READ_BITS(gb, 8, &gm->cr_luma_mult);
        READ_BITS(gb, 9, &gm->cr_offset);
    }

    READ_ONEBIT(gb, &gm->overlap_flag);
    READ_ONEBIT(gb, &gm->clip_to_restricted_range);

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_uncompressed_header(Av1Codec *ctx, BitReadCtx_t *gb,
                                        AV1FrameHeader *f)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 id_len, diff_len, all_frames, frame_is_intra, order_hint_bits;
    RK_S32 i;

    if (!seq) {
        mpp_err_f("No sequence header available.\n");
        return MPP_ERR_UNKNOW;
    }

    id_len = seq->additional_frame_id_length_minus_1 +
             seq->delta_frame_id_length_minus_2 + 3;
    all_frames = (1 << AV1_NUM_REF_FRAMES) - 1;

    if (seq->reduced_still_picture_header) {
        f->show_existing_frame = 0;
        f->frame_type = AV1_FRAME_KEY;
        f->show_frame = 1;
        f->showable_frame = 0;
        frame_is_intra = 1;
    } else {
        READ_ONEBIT(gb, &f->show_existing_frame);

        if (f->show_existing_frame) {
            AV1RefFrameState *ref;

            READ_BITS(gb, 3, &f->frame_to_show_map_idx);
            ref = &ctx->ref_s[f->frame_to_show_map_idx];

            if (!ref->valid) {
                mpp_err_f("Missing reference frame needed for "
                          "show_existing_frame (frame_to_show_map_idx = %d).\n",
                          f->frame_to_show_map_idx);
                return MPP_ERR_UNKNOW;
            }

            if (seq->decoder_model_info_present_flag &&
                !seq->timing_info.equal_picture_interval) {
                READ_BITS_LONG(gb, seq->decoder_model_info.frame_presentation_time_length_minus_1 + 1,
                               &f->frame_presentation_time);
            }

            if (seq->frame_id_numbers_present_flag)
                READ_BITS_LONG(gb, id_len, &f->display_frame_id);

            f->frame_type = ref->frame_type;
            if (f->frame_type == AV1_FRAME_KEY) {
                f->refresh_frame_flags = all_frames;

                // Section 7.21
                f->current_frame_id = ref->frame_id;
                ctx->upscaled_width  = ref->upscaled_width;
                ctx->frame_width     = ref->frame_width;
                ctx->frame_height    = ref->frame_height;
                ctx->render_width    = ref->render_width;
                ctx->render_height   = ref->render_height;
                ctx->bit_depth       = ref->bit_depth;
                ctx->order_hint      = ref->order_hint;
            } else
                f->refresh_frame_flags = 0;

            f->frame_width_minus_1 = ref->upscaled_width - 1;
            f->frame_height_minus_1 = ref->frame_height - 1;
            f->render_width_minus_1 = ref->render_width - 1;
            f->render_height_minus_1 = ref->render_height - 1;

            return MPP_OK;
        }

        READ_BITS(gb, 2, &f->frame_type);
        frame_is_intra = (f->frame_type == AV1_FRAME_INTRA_ONLY ||
                          f->frame_type == AV1_FRAME_KEY);

        ctx->frame_is_intra = frame_is_intra;
        if (f->frame_type == AV1_FRAME_KEY) {
            RK_U32 refresh_frame_flags = (1 << AV1_NUM_REF_FRAMES) - 1;

            av1d_get_cdfs(ctx, f->frame_to_show_map_idx);
            av1d_store_cdfs(ctx, refresh_frame_flags);
        }

        READ_ONEBIT(gb, &f->show_frame);
        if (f->show_frame &&
            seq->decoder_model_info_present_flag &&
            !seq->timing_info.equal_picture_interval) {
            READ_BITS_LONG(gb, seq->decoder_model_info.frame_presentation_time_length_minus_1 + 1,
                           &f->frame_presentation_time);
        }
        if (f->show_frame)
            f->showable_frame = (f->frame_type != AV1_FRAME_KEY) ? 1 : 0;
        else
            READ_ONEBIT(gb, &f->showable_frame);

        if (f->frame_type == AV1_FRAME_SWITCH ||
            (f->frame_type == AV1_FRAME_KEY && f->show_frame))
            f->error_resilient_mode = 1;
        else
            READ_ONEBIT(gb, &f->error_resilient_mode);
    }

    if (f->frame_type == AV1_FRAME_KEY && f->show_frame) {
        for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
            ctx->ref_s[i].valid = 0;
            ctx->ref_s[i].order_hint = 0;
        }
    }

    READ_ONEBIT(gb, &f->disable_cdf_update);

    if (seq->seq_force_screen_content_tools == AV1_SELECT_SCREEN_CONTENT_TOOLS) {
        READ_ONEBIT(gb, &f->allow_screen_content_tools);
    } else {
        f->allow_screen_content_tools = seq->seq_force_screen_content_tools;
    }
    if (f->allow_screen_content_tools) {
        if (seq->seq_force_integer_mv == AV1_SELECT_INTEGER_MV)
            READ_ONEBIT(gb, &f->force_integer_mv);
        else
            f->force_integer_mv = seq->seq_force_integer_mv;
    } else {
        f->force_integer_mv = 0;
    }

    if (seq->frame_id_numbers_present_flag) {
        READ_BITS_LONG(gb, id_len, &f->current_frame_id);

        diff_len = seq->delta_frame_id_length_minus_2 + 2;
        for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
            if (f->current_frame_id > (RK_S32)(1 << diff_len)) {
                if (ctx->ref_s[i].frame_id > f->current_frame_id ||
                    ctx->ref_s[i].frame_id < (f->current_frame_id - (RK_S32)(1 << diff_len)))
                    ctx->ref_s[i].valid = 0;
            } else {
                if (ctx->ref_s[i].frame_id > f->current_frame_id &&
                    ctx->ref_s[i].frame_id < (RK_S32)((1 << id_len) + f->current_frame_id - (1 << diff_len)))
                    ctx->ref_s[i].valid = 0;
            }
        }
    } else {
        f->current_frame_id = 0;
    }

    if (f->frame_type == AV1_FRAME_SWITCH)
        f->frame_size_override_flag = 1;
    else if (seq->reduced_still_picture_header)
        f->frame_size_override_flag = 0;
    else
        READ_ONEBIT(gb, &f->frame_size_override_flag);

    order_hint_bits = seq->enable_order_hint ? seq->order_hint_bits_minus_1 + 1 : 0;
    if (order_hint_bits > 0)
        READ_BITS_LONG(gb, order_hint_bits, &f->order_hint);
    else
        f->order_hint = 0;
    ctx->order_hint = f->order_hint;

    if (frame_is_intra || f->error_resilient_mode)
        f->primary_ref_frame = AV1_PRIMARY_REF_NONE;
    else
        READ_BITS(gb, 3, &f->primary_ref_frame);

    if (seq->decoder_model_info_present_flag) {
        READ_ONEBIT(gb, &f->buffer_removal_time_present_flag);
        if (f->buffer_removal_time_present_flag) {
            for (i = 0; i <= seq->operating_points_cnt_minus_1; i++) {
                if (seq->decoder_model_present_for_this_op[i]) {
                    RK_S32 op_pt_idc = seq->operating_point_idc[i];
                    RK_S32 in_temporal_layer = (op_pt_idc >>  ctx->temporal_id    ) & 1;
                    RK_S32 in_spatial_layer  = (op_pt_idc >> (ctx->spatial_id + 8)) & 1;

                    if (seq->operating_point_idc[i] == 0 ||
                        (in_temporal_layer && in_spatial_layer)) {
                        READ_BITS_LONG(gb, seq->decoder_model_info.buffer_removal_time_length_minus_1 + 1,
                                       &f->buffer_removal_time[i]);
                    }
                }
            }
        }
    }

    if (f->frame_type == AV1_FRAME_SWITCH ||
        (f->frame_type == AV1_FRAME_KEY && f->show_frame))
        f->refresh_frame_flags = all_frames;
    else
        READ_BITS(gb, 8, &f->refresh_frame_flags);

    ctx->refresh_frame_flags = f->refresh_frame_flags;
    if (!frame_is_intra || f->refresh_frame_flags != all_frames) {
        if (seq->enable_order_hint) {
            for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
                if (f->error_resilient_mode)
                    READ_BITS(gb, order_hint_bits, &f->ref_order_hint[i]);
                else
                    f->ref_order_hint[i] = ctx->ref_s[i].order_hint;
                if (f->ref_order_hint[i] != ctx->ref_s[i].order_hint)
                    ctx->ref_s[i].valid = 0;
            }
        }
    }

    f->ref_frame_valued = 1;
    if (f->frame_type == AV1_FRAME_KEY ||
        f->frame_type == AV1_FRAME_INTRA_ONLY) {
        FUN_CHECK(read_frame_size(ctx, gb, f));
        FUN_CHECK(read_render_size(ctx, gb, f));

        if (f->allow_screen_content_tools &&
            ctx->upscaled_width == ctx->frame_width)
            READ_ONEBIT(gb, &f->allow_intrabc);
        else
            f->allow_intrabc = 0;

        f->ref_frame_valued = 0;
    } else {
        if (!seq->enable_order_hint) {
            f->frame_refs_short_signaling = 0;
        } else {
            READ_ONEBIT(gb, &f->frame_refs_short_signaling);
            if (f->frame_refs_short_signaling) {
                READ_BITS(gb, 3, &f->last_frame_idx);
                READ_BITS(gb, 3, &f->golden_frame_idx);
                FUN_CHECK(set_frame_refs(ctx, f));
            }
        }

        for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
            if (!f->frame_refs_short_signaling)
                READ_BITS(gb, 3, &f->ref_frame_idx[i]);
            if (seq->frame_id_numbers_present_flag) {
                READ_BITS_LONG(gb, seq->delta_frame_id_length_minus_2 + 2,
                               &f->delta_frame_id_minus1[i]);
            }
        }

        if (f->frame_size_override_flag &&
            !f->error_resilient_mode) {
            FUN_CHECK(read_frame_size_with_refs(ctx, gb, f));
        } else {
            FUN_CHECK(read_frame_size(ctx, gb, f));
            FUN_CHECK(read_render_size(ctx, gb, f));
        }

        if (f->force_integer_mv)
            f->allow_high_precision_mv = 0;
        else
            READ_ONEBIT(gb, &f->allow_high_precision_mv);

        FUN_CHECK(read_interpolation_filter(ctx, gb, f));

        READ_ONEBIT(gb, &f->is_motion_mode_switchable);

        if (f->error_resilient_mode ||
            !seq->enable_ref_frame_mvs)
            f->use_ref_frame_mvs = 0;
        else
            READ_ONEBIT(gb, &f->use_ref_frame_mvs);

        f->allow_intrabc = 0;
    }

    if (seq->reduced_still_picture_header || f->disable_cdf_update)
        f->disable_frame_end_update_cdf = 1;
    else
        READ_ONEBIT(gb, &f->disable_frame_end_update_cdf);

    ctx->disable_frame_end_update_cdf = f->disable_frame_end_update_cdf;

    av1d_dbg(AV1D_DBG_HEADER, "ptile_info in %d", mpp_get_bits_count(gb));
    FUN_CHECK(read_tile_info_max_tile(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "ptile_info out %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_quantization_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "quantization out %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_segmentation_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "segmentation out %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_delta_q_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "delta_q out %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_delta_lf_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "lf out %d", mpp_get_bits_count(gb));

    // Load CDF tables for non-coeffs.
    if (f->error_resilient_mode || frame_is_intra ||
        f->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
        // Init non-coeff CDFs.
        // Setup past independence.
        ctx->cdfs = &ctx->default_cdfs;
        ctx->cdfs_ndvc = &ctx->default_cdfs_ndvc;
        av1d_set_default_coeff_probs(f->base_q_idx, ctx->cdfs);
    } else {
        // Load CDF tables from previous frame.
        // Load params from previous frame.
        RK_U32 idx = f->ref_frame_idx[f->primary_ref_frame];

        av1d_get_cdfs(ctx, idx);
    }
    av1d_dbg(AV1D_DBG_HEADER,
             "show_existing_frame_index %d primary_ref_frame %d %d (%d) "
             "refresh_frame_flags %d base_q_idx %d\n",
             f->frame_to_show_map_idx,
             f->ref_frame_idx[f->primary_ref_frame],
             ctx->ref[f->ref_frame_idx[f->primary_ref_frame]].slot_index,
             f->primary_ref_frame,
             f->refresh_frame_flags,
             f->base_q_idx);
    av1d_store_cdfs(ctx, f->refresh_frame_flags);

    ctx->coded_lossless = 1;
    for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
        RK_S32 qindex;

        if (f->feature_enabled[i][AV1_SEG_LVL_ALT_Q]) {
            qindex = (f->base_q_idx + f->feature_value[i][AV1_SEG_LVL_ALT_Q]);
        } else {
            qindex = f->base_q_idx;
        }
        qindex = mpp_clip_uint_pow2(qindex, 8);

        if (qindex || f->delta_q_y_dc ||
            f->delta_q_u_ac || f->delta_q_u_dc ||
            f->delta_q_v_ac || f->delta_q_v_dc) {
            ctx->coded_lossless = 0;
        }
    }
    ctx->all_lossless = ctx->coded_lossless && ctx->frame_width == ctx->upscaled_width;
    av1d_dbg(AV1D_DBG_HEADER, "filter in %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_loop_filter_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "cdef in %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_cdef_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "lr in %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_lr_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "read_tx in %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_tx_mode(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "reference in%d", mpp_get_bits_count(gb));

    FUN_CHECK(read_frame_reference_mode(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "kip_mode in %d", mpp_get_bits_count(gb));

    FUN_CHECK(read_skip_mode_params(ctx, gb, f));

    if (frame_is_intra || f->error_resilient_mode ||
        !seq->enable_warped_motion)
        f->allow_warped_motion = 0;
    else
        READ_ONEBIT(gb, &f->allow_warped_motion);

    READ_ONEBIT(gb, &f->reduced_tx_set);
    av1d_dbg(AV1D_DBG_HEADER, "motion in%d", mpp_get_bits_count(gb));

    FUN_CHECK(read_global_motion_params(ctx, gb, f));
    av1d_dbg(AV1D_DBG_HEADER, "grain in %d", mpp_get_bits_count(gb));
    FUN_CHECK(read_film_grain_params(ctx, gb, &f->film_grain, f));
    av1d_dbg(AV1D_DBG_HEADER, "film_grain out %d", mpp_get_bits_count(gb));

    av1d_dbg(AV1D_DBG_REF, "Frame %d:  size %dx%d  upscaled %d  render %dx%d  subsample %dx%d  "
             "bitdepth %d  tiles %dx%d.\n", ctx->order_hint,
             ctx->frame_width, ctx->frame_height, ctx->upscaled_width,
             ctx->render_width, ctx->render_height,
             seq->color_config.subsampling_x + 1,
             seq->color_config.subsampling_y + 1, ctx->bit_depth,
             ctx->tile_rows, ctx->tile_cols);

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_frame_header_obu(Av1Codec *ctx, BitReadCtx_t *gb,
                                     AV1FrameHeader *f, RK_S32 redundant,
                                     void *rw_buffer_ref)
{
    if (ctx->seen_frame_header) {
        if (!redundant) {
            mpp_err_f("Invalid repeated frame header OBU.\n");
            return MPP_ERR_UNKNOW;
        } else {
            BitReadCtx_t fh;

            mpp_set_bitread_ctx(&fh, ctx->frame_header_data, ctx->frame_header_size);
            for (size_t i = 0; i < ctx->frame_header_size; i += 8) {
                RK_S32 val;
                RK_S32 b = MPP_MIN(ctx->frame_header_size - i, 8);
                mpp_assert(b < 32);
                mpp_read_bits(&fh, b, &val);
                READ_BITS_LONG(gb, b, &val);
            }
        }
    } else {
        RK_S32 start_pos = mpp_get_bits_count(gb);

        FUN_CHECK(read_uncompressed_header(ctx, gb, f));

        ctx->tile_num = 0;
        ctx->seen_frame_header = f->show_existing_frame ? 0 : 1;
        if (!f->show_existing_frame) {
            RK_S32 fh_bits  = mpp_get_bits_count(gb) - start_pos;
            RK_U8 *fh_start = (RK_U8*)gb->buf + start_pos / 8;
            RK_S32 fh_bytes = (fh_bits + 7) / 8;

            ctx->frame_header_size = fh_bits;

            // Store frame header for possible future use.
            MPP_FREE(ctx->frame_header_data);
            ctx->frame_header_data = mpp_malloc(RK_U8, fh_bytes + BUFFER_PADDING_SIZE);
            if (!ctx->frame_header_data) {
                mpp_err_f("frame header data malloc failed\n");
                return MPP_ERR_NOMEM;
            }
            memcpy(ctx->frame_header_data, fh_start, fh_bytes);
        }
    }
    (void)rw_buffer_ref;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_NOK;
}

static MPP_RET read_tile_group_obu(Av1Codec *ctx, BitReadCtx_t *gb,
                                   AV1TileGroup *tile)
{
    RK_S32 num_tiles = ctx->tile_cols * ctx->tile_rows;

    if (num_tiles > 1)
        READ_ONEBIT(gb, &tile->tile_start_and_end_present_flag);
    else
        tile->tile_start_and_end_present_flag = 0;

    if (num_tiles == 1 || !tile->tile_start_and_end_present_flag) {
        tile->tg_start = 0;
        tile->tg_end = num_tiles - 1;
    } else {
        RK_S32 tile_bits = tile_log2(1, ctx->tile_cols) + tile_log2(1, ctx->tile_rows);

        READ_BITS_LONG(gb, tile_bits, &tile->tg_start);
        READ_BITS_LONG(gb, tile_bits, &tile->tg_end);
    }

    ctx->tile_num = tile->tg_end + 1;

    mpp_align_get_bits(gb);

    // reset for next frame
    if (tile->tg_end == num_tiles - 1)
        ctx->seen_frame_header = 0;

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_frame_obu(Av1Codec *ctx, BitReadCtx_t *gb,
                              AV1Frame *f, void *rw_buffer_ref)
{
    RK_U32 start_pos = mpp_get_bits_count(gb);

    FUN_CHECK(read_frame_header_obu(ctx, gb, &f->header, 0, rw_buffer_ref));

    mpp_align_get_bits(gb);

    FUN_CHECK(read_tile_group_obu(ctx, gb, &f->tile_group));
    ctx->frame_tag_size += (mpp_get_bits_count(gb) - start_pos + 7) >> 3;

    return MPP_OK;
}

static MPP_RET read_tile_list_obu(Av1Codec *ctx, BitReadCtx_t *gb,
                                  AV1TileList *list)
{
    READ_BITS(gb, 8, &list->output_frame_width_in_tiles_minus_1);
    READ_BITS(gb, 8, &list->output_frame_height_in_tiles_minus_1);
    READ_BITS(gb, 16, &list->tile_count_minus_1);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_metadata_hdr_cll(Av1Codec *ctx, BitReadCtx_t *gb,
                                     AV1MetadataHDRCLL *cll)
{
    READ_BITS(gb, 16, &cll->max_cll);
    READ_BITS(gb, 16, &cll->max_fall);

    ctx->content_light.MaxCLL = cll->max_cll;
    ctx->content_light.MaxFALL = cll->max_fall;

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static RK_S32 read_metadata_hdr_mdcv(Av1Codec *ctx, BitReadCtx_t *gb,
                                     AV1MetadataHDRMDCV *mdcv)
{
    RK_S32 i;

    for (i = 0; i < 3; i++) {
        READ_BITS(gb, 16, &mdcv->primary_chromaticity_x[i]);
        READ_BITS(gb, 16, &mdcv->primary_chromaticity_y[i]);
    }

    READ_BITS(gb, 16, &mdcv->white_point_chromaticity_x);
    READ_BITS(gb, 16, &mdcv->white_point_chromaticity_y);

    READ_BITS_LONG(gb, 32, &mdcv->luminance_max);
    READ_BITS_LONG(gb, 32, &mdcv->luminance_min);

    for (i = 0; i < 3; i++) {
        ctx->mastering_display.display_primaries[i][0] = mdcv->primary_chromaticity_x[i];
        ctx->mastering_display.display_primaries[i][1] = mdcv->primary_chromaticity_y[i];
    }
    ctx->mastering_display.white_point[0] = mdcv->white_point_chromaticity_x;
    ctx->mastering_display.white_point[1] = mdcv->white_point_chromaticity_y;
    ctx->mastering_display.max_luminance = mdcv->luminance_max;
    ctx->mastering_display.min_luminance = mdcv->luminance_min;

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_scalability_structure(Av1Codec *ctx, BitReadCtx_t *gb,
                                          AV1MetadataScalability *scale)
{
    const AV1SeqHeader *seq = ctx->seq_header;
    RK_S32 i, j;

    if (!seq) {
        mpp_err_f("No sequence header available.\n");
        return MPP_ERR_UNKNOW;
    }

    READ_BITS(gb, 2, &scale->spatial_layers_cnt_minus_1);
    READ_ONEBIT(gb, &scale->spatial_layer_dimensions_present_flag);
    READ_ONEBIT(gb, &scale->spatial_layer_description_present_flag);
    READ_ONEBIT(gb, &scale->temporal_group_description_present_flag);
    READ_BITS(gb, 3, &scale->scalability_structure_reserved_3bits);
    if (scale->spatial_layer_dimensions_present_flag) {
        for (i = 0; i <= scale->spatial_layers_cnt_minus_1; i++) {
            READ_BITS(gb, 16, &scale->spatial_layer_max_width[i]);
            READ_BITS(gb, 16, &scale->spatial_layer_max_height[i]);
        }
    }
    if (scale->spatial_layer_description_present_flag) {
        for (i = 0; i <= scale->spatial_layers_cnt_minus_1; i++)
            READ_BITS(gb, 8, &scale->spatial_layer_ref_id[i]);
    }
    if (scale->temporal_group_description_present_flag) {
        READ_BITS(gb, 8, &scale->temporal_group_size);
        for (i = 0; i < scale->temporal_group_size; i++) {
            READ_BITS(gb, 3, &scale->temporal_group_temporal_id[i]);
            READ_ONEBIT(gb, &scale->temporal_group_temporal_switching_up_point_flag[i]);
            READ_ONEBIT(gb, &scale->temporal_group_spatial_switching_up_point_flag[i]);
            READ_BITS(gb, 3, &scale->temporal_group_ref_cnt[i]);
            for (j = 0; j < scale->temporal_group_ref_cnt[i]; j++) {
                READ_BITS(gb, 8, &scale->temporal_group_ref_pic_diff[i][j]);
            }
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_metadata_scalability(Av1Codec *ctx, BitReadCtx_t *gb,
                                         AV1MetadataScalability *scale)
{
    READ_BITS(gb, 8, &scale->scalability_mode_idc);

    if (scale->scalability_mode_idc == AV1_SCALABILITY_SS)
        FUN_CHECK(read_scalability_structure(ctx, gb, scale));

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET get_dlby_rpu(Av1Codec *ctx, BitReadCtx_t *gb)
{
    MppFrameHdrDynamicMeta *dynamic = ctx->hdr_dynamic_meta;
    MppWriteCtx m_bc, *bc = &m_bc;
    RK_U32 emdf_payload_size = 0;
    RK_S32 flag = 0;
    RK_U32 i;

    // skip emdf_container
    SKIP_BITS(gb, 3);
    SKIP_BITS(gb, 2);
    SKIP_BITS(gb, 5);
    SKIP_BITS(gb, 5);
    SKIP_BITS(gb, 1);
    SKIP_BITS(gb, 5);
    SKIP_BITS(gb, 1);
    // skip emdf_payload_config
    SKIP_BITS(gb, 5);

    // get payload size
    do {
        RK_U32 tmp;

        READ_BITS(gb, 8, &tmp);
        emdf_payload_size += tmp;
        READ_ONEBIT(gb, &flag);
        if (!flag) break;
        emdf_payload_size <<= 8;
        emdf_payload_size += (1 << 8);
    } while (flag);

    if (!dynamic) {
        dynamic = mpp_calloc_size(MppFrameHdrDynamicMeta, sizeof(*dynamic) + SZ_1K);
        if (!dynamic) {
            mpp_err_f("malloc hdr dynamic data failed!\n");
            return MPP_ERR_NOMEM;
        }
    }

    mpp_writer_init(bc, dynamic->data, SZ_1K);

    mpp_writer_put_raw_bits(bc, 0, 24);
    mpp_writer_put_raw_bits(bc, 1, 8);
    mpp_writer_put_raw_bits(bc, 0x19, 8);
    for (i = 0; i < emdf_payload_size; i++) {
        RK_U8 data;

        READ_BITS(gb, 8, &data);
        mpp_writer_put_bits(bc, data, 8);
    }

    dynamic->size = mpp_writer_bytes(bc);
    dynamic->hdr_fmt = DLBY;
    av1d_dbg(AV1D_DBG_STRMIN, "dlby rpu size %d -> %d\n",
             emdf_payload_size, dynamic->size);

    ctx->hdr_dynamic_meta = dynamic;
    ctx->hdr_dynamic = 1;
    ctx->is_hdr = 1;

    if (av1d_debug & AV1D_DBG_DUMP_RPU) {
        RK_U8 *p = dynamic->data;
        char fname[128];
        FILE *fp_in = NULL;
        static RK_U32 g_frame_no = 0;

        sprintf(fname, "/data/video/meta_%d.txt", g_frame_no++);
        fp_in = fopen(fname, "wb");
        mpp_err("open %s %p\n", fname, fp_in);
        if (fp_in)
            fwrite(p, 1, dynamic->size, fp_in);
        fflush(fp_in);
        fclose(fp_in);
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET fill_dynamic_meta(Av1Codec *ctx, const RK_U8 *data, RK_U32 size, RK_U32 hdr_fmt)
{
    MppFrameHdrDynamicMeta *dynamic = ctx->hdr_dynamic_meta;

    if (dynamic && (dynamic->size < size)) {
        mpp_free(dynamic);
        dynamic = NULL;
    }

    if (!dynamic) {
        dynamic = mpp_calloc_size(MppFrameHdrDynamicMeta, sizeof(*dynamic) + size);
        if (!dynamic) {
            mpp_err_f("malloc hdr dynamic data failed!\n");
            return MPP_ERR_NOMEM;
        }
    }
    if (size && data) {
        switch (hdr_fmt) {
        case HDR10PLUS: {
            memcpy((RK_U8*)dynamic->data, (RK_U8*)data, size);
        } break;
        default: break;
        }
        dynamic->size = size;
        dynamic->hdr_fmt = hdr_fmt;

        ctx->hdr_dynamic_meta = dynamic;
        ctx->hdr_dynamic = 1;
        ctx->is_hdr = 1;
    }
    return MPP_OK;
}

static MPP_RET read_metadata_itut_t35(Av1Codec *ctx, BitReadCtx_t *gb,
                                      AV1MetadataITUTT35 *t35)
{
    READ_BITS(gb, 8, &t35->itu_t_t35_country_code);
    if (t35->itu_t_t35_country_code == 0xff)
        READ_BITS(gb, 8, &t35->itu_t_t35_country_code_extension_byte);

    t35->payload_size = mpp_get_bits_left(gb) / 8 - 1;

    av1d_dbg(AV1D_DBG_STRMIN, "%s itu_t_t35_country_code %d payload_size %d\n",
             __func__, t35->itu_t_t35_country_code, t35->payload_size);

    READ_BITS(gb, 16, &t35->itu_t_t35_terminal_provider_code);

    av1d_dbg(AV1D_DBG_STRMIN, "itu_t_t35_country_code 0x%x\n",
             t35->itu_t_t35_country_code);
    av1d_dbg(AV1D_DBG_STRMIN, "itu_t_t35_terminal_provider_code 0x%x\n",
             t35->itu_t_t35_terminal_provider_code);

    switch (t35->itu_t_t35_terminal_provider_code) {
    case 0x3B: {/* dlby provider_code is 0x3b*/
        READ_BITS_LONG(gb, 32, &t35->itu_t_t35_terminal_provider_oriented_code);
        av1d_dbg(AV1D_DBG_STRMIN, "itu_t_t35_terminal_provider_oriented_code 0x%x\n",
                 t35->itu_t_t35_terminal_provider_oriented_code);
        if (t35->itu_t_t35_terminal_provider_oriented_code == 0x800)
            get_dlby_rpu(ctx, gb);
    } break;
    case 0x3C: {/* smpte2094_40 provider_code is 0x3c*/
        const RK_U16 smpte2094_40_provider_oriented_code = 0x0001;
        const RK_U8 smpte2094_40_application_identifier = 0x04;
        RK_U8 application_identifier;

        READ_BITS(gb, 16, &t35->itu_t_t35_terminal_provider_oriented_code);
        av1d_dbg(AV1D_DBG_STRMIN, "itu_t_t35_terminal_provider_oriented_code 0x%x\n",
                 t35->itu_t_t35_terminal_provider_oriented_code);
        READ_BITS(gb, 8, &application_identifier);
        /* hdr10plus priverder_oriented_code is 0x0001, application_identifier is 0x04 */
        if (t35->itu_t_t35_terminal_provider_oriented_code == smpte2094_40_provider_oriented_code &&
            application_identifier == smpte2094_40_application_identifier)
            fill_dynamic_meta(ctx, gb->data_, mpp_get_bits_left(gb) >> 3, HDR10PLUS);
    } break;
    default:
        break;
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_metadata_timecode(Av1Codec *ctx, BitReadCtx_t *gb,
                                      AV1MetadataTimecode *timecode)
{
    READ_BITS(gb, 5, &timecode->counting_type);
    READ_ONEBIT(gb, &timecode->full_timestamp_flag);
    READ_ONEBIT(gb, &timecode->discontinuity_flag);
    READ_ONEBIT(gb, &timecode->cnt_dropped_flag);
    READ_BITS(gb, 9, &timecode->n_frames);

    if (timecode->full_timestamp_flag) {
        READ_BITS(gb, 6, &timecode->seconds_value);
        READ_BITS(gb, 6, &timecode->minutes_value);
        READ_BITS(gb, 5, &timecode->hours_value);
    } else {
        READ_ONEBIT(gb, &timecode->seconds_flag);
        if (timecode->seconds_flag) {
            READ_BITS(gb, 6, &timecode->seconds_value);
            READ_ONEBIT(gb, &timecode->minutes_flag);
            if (timecode->minutes_flag) {
                READ_BITS(gb, 6, &timecode->minutes_value);
                READ_ONEBIT(gb, &timecode->hours_flag);
                if (timecode->hours_flag)
                    READ_BITS(gb, 5, &timecode->hours_value);
            }
        }
    }

    READ_BITS(gb, 5, &timecode->time_offset_length);
    if (timecode->time_offset_length > 0)
        READ_BITS_LONG(gb, timecode->time_offset_length, &timecode->time_offset_value);
    else
        timecode->time_offset_length = 0;
    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET read_metadata_obu(Av1Codec *ctx, BitReadCtx_t *gb,
                                 AV1Metadata *meta)
{
    FUN_CHECK(read_leb128(gb, &meta->metadata_type));

    av1d_dbg(AV1D_DBG_STRMIN, "%s meta type %lld\n", __func__, meta->metadata_type);
    switch (meta->metadata_type) {
    case AV1_METADATA_TYPE_HDR_CLL:
        FUN_CHECK(read_metadata_hdr_cll(ctx, gb, &meta->metadata.hdr_cll));
        break;
    case AV1_METADATA_TYPE_HDR_MDCV:
        FUN_CHECK(read_metadata_hdr_mdcv(ctx, gb, &meta->metadata.hdr_mdcv));
        break;
    case AV1_METADATA_TYPE_SCALABILITY:
        FUN_CHECK(read_metadata_scalability(ctx, gb, &meta->metadata.scalability));
        break;
    case AV1_METADATA_TYPE_ITUT_T35:
        FUN_CHECK(read_metadata_itut_t35(ctx, gb, &meta->metadata.itut_t35));
        break;
    case AV1_METADATA_TYPE_TIMECODE:
        FUN_CHECK(read_metadata_timecode(ctx, gb, &meta->metadata.timecode));
        break;
    default:
        mpp_err_f("unknown metadata type %lld\n", meta->metadata_type);
        break;
    }

    return MPP_OK;
}

static MPP_RET read_padding_obu(Av1Codec *ctx, BitReadCtx_t *gb, AV1Padding *pad)
{
    BitReadCtx_t tmp_gb;
    RK_S32 flag, last_nonzero_byte = 0;
    RK_U32 i;

    // make a copy of gb for read twice
    memcpy(&tmp_gb, gb, sizeof(*gb));

    // read first, find last non-zero byte
    for (i = 0; mpp_get_bits_left(gb) >= 8; i++) {
        READ_BITS(gb, 8, &flag);
        if (flag)
            last_nonzero_byte = i;
    }
    pad->payload_size = last_nonzero_byte;
    pad->payload = mpp_malloc(RK_U8, pad->payload_size);
    if (!pad->payload )
        return MPP_ERR_NOMEM;

    // read twice for padding payload
    memcpy(gb, &tmp_gb, sizeof(*gb));
    for (i = 0; i < pad->payload_size; i++)
        READ_BITS(gb, 8, &pad->payload[i]);

    (void)ctx;
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_READ_BIT;
}

static MPP_RET insert_unit_data(Av1UnitFragment *frag, RK_S32 position,
                                RK_U32 type, RK_U8 *data, size_t data_size)
{
    Av1ObuUnit *units;

    if (position == -1)
        position = frag->nb_units;
    mpp_assert(position >= 0 && position <= frag->nb_units);

    // Make space for new unit.
    if (frag->nb_units < frag->nb_units_allocated) {
        units = frag->units;

        if (position < frag->nb_units)
            memmove(units + position + 1, units + position,
                    (frag->nb_units - position) * sizeof(*units));
    } else {
        units = mpp_malloc(Av1ObuUnit, frag->nb_units * 2 + 1);
        if (!units)
            return MPP_ERR_NOMEM;

        frag->nb_units_allocated = 2 * frag->nb_units_allocated + 1;

        if (position > 0)
            memcpy(units, frag->units, position * sizeof(*units));

        if (position < frag->nb_units)
            memcpy(units + position + 1, frag->units + position,
                   (frag->nb_units - position) * sizeof(*units));
    }

    memset(units + position, 0, sizeof(*units));

    // Update fragment unit list.
    if (units != frag->units) {
        mpp_free(frag->units);
        frag->units = units;
    }
    ++frag->nb_units;

    // Fill in unit data.
    frag->units[position].type      = type;
    frag->units[position].data      = data;
    frag->units[position].data_size = data_size;

    return MPP_OK;
}

static MPP_RET read_ref_tile_data(Av1ObuUnit *unit, BitReadCtx_t *gb, AV1TileData *td)
{
    RK_S32 pos = mpp_get_bits_count(gb);

    if (pos >= (RK_S32)(8 * unit->data_size)) {
        mpp_err( "Bitstream ended before %d bits read.\n", pos);
        return MPP_ERR_STREAM;
    }
    // Must be byte-aligned at this point.
    mpp_assert(pos % 8 == 0);

    td->offset    = pos / 8;
    td->data      = unit->data + pos / 8;
    td->data_size = unit->data_size - pos / 8;

    return MPP_OK;
}

static MPP_RET read_one_obu_unit(Av1Codec *ctx, Av1ObuUnit *unit)
{
    AV1OBU *obu = &unit->obu;
    AV1OBUHeader *h = &obu->header;
    BitReadCtx_t m_gb, *gb = &m_gb;
    RK_S32 start_pos, end_pos, hdr_start_pos;
    MPP_RET ret = MPP_OK;

    memset(obu, 0, sizeof(*obu));
    mpp_set_bitread_ctx(gb, unit->data, unit->data_size);
    hdr_start_pos = mpp_get_bits_count(gb);

    ret = av1d_read_obu_header(gb, h);
    if (ret < 0)
        return ret;
    mpp_assert(h->obu_type == unit->type);
    ctx->temporal_id = h->temporal_id;
    ctx->spatial_id  = h->spatial_id;

    if (h->obu_has_size_field) {
        obu->obu_size = h->obu_filed_size;
    } else {
        if (unit->data_size < (RK_U32)(1 + h->obu_extension_flag)) {
            mpp_err( "Invalid OBU length %d, too short.\n", unit->data_size);
            return MPP_NOK;
        }
        obu->obu_size = unit->data_size - 1 - h->obu_extension_flag;
    }

    start_pos = mpp_get_bits_count(gb);
    if (!ctx->fist_tile_group)
        ctx->frame_tag_size += ((start_pos - hdr_start_pos + 7) >> 3);
    if (h->obu_extension_flag) {
        if (h->obu_type != AV1_OBU_SEQUENCE_HEADER &&
            h->obu_type != AV1_OBU_TEMPORAL_DELIMITER &&
            ctx->operating_point_idc) {
            RK_S32 in_temporal_layer = (ctx->operating_point_idc >> ctx->temporal_id) & 1;
            RK_S32 in_spatial_layer  = (ctx->operating_point_idc >> (ctx->spatial_id + 8)) & 1;
            if (!in_temporal_layer || !in_spatial_layer) {
                return MPP_ERR_PROTOL; // drop_obu()
            }
        }
    }
    av1d_dbg(AV1D_DBG_HEADER, "obu type %d size %d\n", h->obu_type, obu->obu_size);
    switch (h->obu_type) {
    case AV1_OBU_SEQUENCE_HEADER: {
        ret = read_sequence_header_obu(ctx, gb, &obu->payload.seq_header);
        if (ret < 0)
            return ret;
        ctx->frame_tag_size += obu->obu_size;
        if (ctx->operating_point >= 0) {
            AV1SeqHeader *seq = &obu->payload.seq_header;

            if (ctx->operating_point > seq->operating_points_cnt_minus_1) {
                mpp_err("Invalid Operating Point %d requested, Must not be higher than %u.\n",
                        ctx->operating_point, seq->operating_points_cnt_minus_1);
                return MPP_ERR_PROTOL;
            }
            ctx->operating_point_idc = seq->operating_point_idc[ctx->operating_point];
        }

        ctx->seq_header = NULL;
        ctx->seq_header = &obu->payload.seq_header;
    } break;
    case AV1_OBU_TEMPORAL_DELIMITER: {
        ret = read_temporal_delimiter_obu(ctx, gb);
        if (ret < 0)
            return ret;
    } break;
    case AV1_OBU_FRAME_HEADER:
    case AV1_OBU_REDUNDANT_FRAME_HEADER: {
        ret = read_frame_header_obu(ctx, gb, &obu->payload.frame_header,
                                    h->obu_type == AV1_OBU_REDUNDANT_FRAME_HEADER,
                                    NULL);
        if (ret < 0)
            return ret;
        ctx->frame_tag_size += obu->obu_size;
    } break;
    case AV1_OBU_TILE_GROUP: {
        RK_U32 cur_pos = mpp_get_bits_count(gb);

        ret = read_tile_group_obu(ctx, gb, &obu->payload.tile_group);
        if (ret < 0)
            return ret;
        if (!ctx->fist_tile_group)
            ctx->frame_tag_size += MPP_ALIGN(mpp_get_bits_count(gb) - cur_pos, 8) / 8;
        ctx->fist_tile_group = 1;
        ret = read_ref_tile_data(unit, gb, &obu->payload.tile_group.tile_data);
        if (ret < 0)
            return ret;
    } break;
    case AV1_OBU_FRAME: {
        ret = read_frame_obu(ctx, gb, &obu->payload.frame, NULL);
        if (ret < 0)
            return ret;

        ret = read_ref_tile_data(unit, gb, &obu->payload.frame.tile_group.tile_data);
        if (ret < 0)
            return ret;
    } break;
    case AV1_OBU_TILE_LIST: {
        ret = read_tile_list_obu(ctx, gb, &obu->payload.tile_list);
        if (ret < 0)
            return ret;

        ret = read_ref_tile_data(unit, gb, &obu->payload.tile_list.tile_data);
        if (ret < 0)
            return ret;
    } break;
    case AV1_OBU_METADATA: {
        ctx->frame_tag_size += obu->obu_size;
        ret = read_metadata_obu(ctx, gb, &obu->payload.metadata);
        if (ret < 0)
            return ret;
    } break;
    case AV1_OBU_PADDING: {
        ret = read_padding_obu(ctx, gb, &obu->payload.padding);
        if (ret < 0)
            return ret;
    } break;
    default:
        return MPP_ERR_VALUE;
    }

    end_pos = mpp_get_bits_count(gb);
    mpp_assert(end_pos <= (RK_S32)(unit->data_size * 8));

    if (obu->obu_size > 0 &&
        h->obu_type != AV1_OBU_TILE_GROUP &&
        h->obu_type != AV1_OBU_TILE_LIST &&
        h->obu_type != AV1_OBU_FRAME) {
        RK_S32 nb_bits = obu->obu_size * 8 + start_pos - end_pos;

        if (nb_bits <= 0)
            return MPP_NOK;
        // skip the remaining bits in the OBU
        while (nb_bits >= 32) {
            SKIP_BITS(gb, 16);
            nb_bits -= 16;
        }
        SKIP_BITS(gb, nb_bits);
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

MPP_RET av1d_parser_fragment_header(Av1UnitFragment *frag)
{
    RK_U8 *data = frag->data;
    size_t size = frag->data_size;

    if (size && data[0] & 0x80) {
        RK_S32 version = data[0] & 0x7f;

        if (version != 1) {
            mpp_err( "Unknown version %d!\n", version);
            goto fail;
        }
        if (size < 4) {
            mpp_err("Size %d invalid\n", size);
            goto fail;
        }
        frag->data += 4;
        frag->data_size -= 4;
    }

success:
    return MPP_OK;
fail:
    frag->data_size = 0;
    return MPP_NOK;
}

MPP_RET av1d_get_fragment_units(Av1Codec *ctx, Av1UnitFragment *frag)
{
    RK_U8 *data = frag->data;
    size_t size = frag->data_size;
    MPP_RET ret = MPP_NOK;

    if (INT_MAX / 8 < size) {
        mpp_err( "Invalid size (%d bytes).\n", size);
        ret = MPP_NOK;
        goto fail;
    }

    while (size > 0) {
        AV1OBUHeader m_hdr;
        BitReadCtx_t m_gb, *gb = &m_gb;
        RK_S32 pos = 0;
        RK_U64 obu_size = 0;
        RK_U64 obu_length = 0;

        memset(&m_hdr, 0, sizeof(m_hdr));
        mpp_set_bitread_ctx(gb, data, size);
        ret = av1d_read_obu_header(gb, &m_hdr);
        if (ret < 0)
            goto fail;

        if (m_hdr.obu_has_size_field)
            obu_size = m_hdr.obu_filed_size;
        else
            obu_size = size - 1 - m_hdr.obu_extension_flag;

        pos = mpp_get_bits_count(gb);
        mpp_assert(pos % 8 == 0);
        mpp_assert(pos / 8 <= (RK_S32)size);
        obu_length = pos / 8 + obu_size;

        if (size < obu_length) {
            mpp_err( "Invalid OBU length: %lld > %d remaining bytes.\n",
                     obu_length, size);
            ret = MPP_NOK;
            goto fail;
        }
        ret = insert_unit_data(frag, -1, m_hdr.obu_type, data, obu_length);
        if (ret < 0)
            goto fail;

        data += obu_length;
        size -= obu_length;
    }

success:
    ret = MPP_OK;
fail:
    return ret;
}

MPP_RET av1d_read_fragment_uints(Av1Codec *ctx, Av1UnitFragment *frag)
{
    MPP_RET ret = MPP_NOK;
    RK_S32 i, j;

    ctx->frame_tag_size = 0;
    ctx->fist_tile_group = 0;
    for (i = 0; i < frag->nb_units; i++) {
        Av1ObuUnit *unit = &frag->units[i];

        if (ctx->unit_types) {
            for (j = 0; j < ctx->nb_unit_types; j++) {
                if (ctx->unit_types[j] == unit->type)
                    break;
            }
            if (j >= ctx->nb_unit_types)
                continue;
        }
        mpp_assert(unit->data);
        ret = read_one_obu_unit(ctx, unit);

        if (ret < 0) {
            mpp_err_f("Failed to read unit %d (type %d).\n", i, unit->type);
            return ret;
        }
        av1d_dbg(AV1D_DBG_HEADER, "obu_type %d, obu_size %d, frame_tag_size %d",
                 unit->obu.header.obu_type, unit->obu.obu_size, ctx->frame_tag_size);
    }
    return MPP_OK;
}

MPP_RET av1d_fragment_reset(Av1UnitFragment *frag)
{
    RK_S32 i;

    for (i = 0; i < frag->nb_units; i++) {
        Av1ObuUnit *unit = &frag->units[i];

        memset(&unit->obu, 0, sizeof(unit->obu));
        unit->data = NULL;
        unit->data_size = 0;
    }
    frag->nb_units = 0;
    frag->data = NULL;
    frag->data_size = 0;

    return MPP_OK;
}
