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
#define MODULE_TAG "av1d_cbs"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_bitread.h"

#include "av1d_parser.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

#ifndef INT_MAX
#define INT_MAX       2147483647      /* maximum (signed) int value */
#endif

#define BUFFER_PADDING_SIZE 64
#define MAX_UINT_BITS(length) ((UINT64_C(1) << (length)) - 1)
#define MAX_INT_BITS(length) ((INT64_C(1) << ((length) - 1)) - 1)
#define MIN_INT_BITS(length) (-(INT64_C(1) << ((length) - 1)))
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

static RK_S32 mpp_av1_read_uvlc(BitReadCtx_t *gbc, const char *name, RK_U32 *write_to,
                                RK_U32 range_min, RK_U32 range_max)
{
    RK_U32 value;

    mpp_read_ue(gbc, &value);

    if (value < range_min || value > range_max) {
        mpp_err_f("%s out of range: "
                  "%d, but must be in [%d,%d].\n",
                  name, value, range_min, range_max);
        return MPP_NOK;
    }
    *write_to = value;
    return MPP_OK;
}


static RK_S32 mpp_av1_read_leb128(BitReadCtx_t *gbc, RK_U64 *write_to)
{
    RK_U64 value;
    RK_S32 err = 0, i;

    value = 0;
    for (i = 0; i < 8; i++) {
        RK_U32 byte;

        READ_BITS(gbc, 8, &byte);

        if (err < 0)
            return err;

        value |= (RK_U64)(byte & 0x7f) << (i * 7);
        if (!(byte & 0x80))
            break;
    }

    if (value > UINT32_MAX)
        return MPP_NOK;


    *write_to = value;
    return MPP_OK;

__bitread_error:
    return MPP_NOK;

}

static RK_S32 mpp_av1_read_ns(BitReadCtx_t *gbc, const char *name,
                              RK_U32 n, RK_U32 *write_to)
{
    RK_U32 m, v, extra_bit, value;
    RK_S32 w;

    w = mpp_log2(n) + 1;
    m = (1 << w) - n;

    if (mpp_get_bits_left(gbc) < w) {
        mpp_err_f("Invalid non-symmetric value at "
                  "%s: bitstream ended.\n", name);
        return MPP_NOK;
    }
    if (w - 1 > 0)
        READ_BITS(gbc, w - 1, &v);
    else
        v = 0;

    if (v < m) {
        value = v;
    } else {
        READ_ONEBIT(gbc, &extra_bit);
        value = (v << 1) - m + extra_bit;
    }

    *write_to = value;
    return MPP_OK;

__bitread_error:
    return MPP_NOK;

}

static RK_S32 mpp_av1_read_increment(BitReadCtx_t *gbc, RK_U32 range_min,
                                     RK_U32 range_max, const char *name,
                                     RK_U32 *write_to)
{
    RK_U32 value;
    RK_S32 i;
    RK_S8 bits[33];

    mpp_assert(range_min <= range_max && range_max - range_min < sizeof(bits) - 1);

    for (i = 0, value = range_min; value < range_max;) {
        RK_U8 tmp = 0;
        if (mpp_get_bits_left(gbc) < 1) {
            mpp_err_f("Invalid increment value at "
                      "%s: bitstream ended.\n", name);
            return MPP_NOK;
        }
        READ_ONEBIT(gbc, &tmp);
        if (tmp) {
            bits[i++] = '1';
            ++value;
        } else {
            bits[i++] = '0';
            break;
        }
    }
    *write_to = value;
    return MPP_OK;

__bitread_error:
    return MPP_NOK;
}

RK_S32 mpp_av1_read_unsigned(BitReadCtx_t *gbc,
                             RK_S32 width, const char *name,
                             RK_U32 *write_to, RK_U32 range_min,
                             RK_U32 range_max)
{
    RK_U32 value;

    mpp_assert(width > 0 && width <= 32);

    if (mpp_get_bits_left(gbc) < width) {
        mpp_err_f("Invalid value at "
                  "%s: bitstream ended.\n", name);
        return MPP_NOK;
    }

    READ_BITS(gbc, width, &value);

    if (value < range_min || value > range_max) {
        mpp_err_f("%s out of range: "
                  "%d, but must be in [%d,%d].\n",
                  name, value, range_min, range_max);
        return MPP_NOK;
    }

    *write_to = value;
    return 0;

__bitread_error:
    return MPP_NOK;

}

static RK_S32 sign_extend(RK_S32 val, RK_U8 bits)
{
    RK_U8 shift = 8 * sizeof(RK_S32) - bits;
    union { RK_U8 u; RK_S32 s; } v = { (RK_U8) val << shift };
    return v.s >> shift;
}

RK_S32 mpp_av1_read_signed(BitReadCtx_t *gbc,
                           RK_S32 width, const char *name,
                           RK_S32 *write_to, RK_S32 range_min,
                           RK_S32 range_max)
{
    RK_S32 value;

    mpp_assert(width > 0 && width <= 32);

    if (mpp_get_bits_left(gbc) < width) {
        mpp_err_f("Invalid value at "
                  "%s: bitstream ended.\n", name);
        return MPP_NOK;
    }

    READ_BITS(gbc, width, &value);
    value = sign_extend(value, width);
    if (value < range_min || value > range_max) {
        mpp_err_f("%s out of range: "
                  "%d, but must be in [%d,%d].\n",
                  name, value, range_min, range_max);
        return MPP_NOK;
    }

    *write_to = value;
    return 0;

__bitread_error:
    return MPP_NOK;

}

static RK_S32 mpp_av1_read_subexp(BitReadCtx_t *gbc,
                                  RK_U32 range_max, RK_U32 *write_to)
{
    RK_U32 value;
    RK_S32 err;
    RK_U32 max_len, len, range_offset, range_bits;

    max_len = mpp_log2(range_max - 1) - 3;

    err = mpp_av1_read_increment(gbc, 0, max_len, "subexp_more_bits", &len);
    if (err < 0)
        return err;

    if (len) {
        range_bits   = 2 + len;
        range_offset = 1 << range_bits;
    } else {
        range_bits   = 3;
        range_offset = 0;
    }

    if (len < max_len) {
        err = mpp_av1_read_unsigned(gbc, range_bits,
                                    "subexp_bits", &value,
                                    0, MAX_UINT_BITS(range_bits));
        if (err < 0)
            return err;

    } else {
        err = mpp_av1_read_ns(gbc, "subexp_final_bits", range_max - range_offset,
                              &value);
        if (err < 0)
            return err;
    }
    value += range_offset;

    *write_to = value;
    return err;
}


static RK_S32 mpp_av1_tile_log2(RK_S32 blksize, RK_S32 target)
{
    RK_S32 k;
    for (k = 0; (blksize << k) < target; k++);
    return k;
}

static RK_S32 mpp_av1_get_relative_dist(const AV1RawSequenceHeader *seq,
                                        RK_U32 a, RK_U32 b)
{
    RK_U32 diff, m;
    if (!seq->enable_order_hint)
        return 0;
    diff = a - b;
    m = 1 << seq->order_hint_bits_minus_1;
    diff = (diff & (m - 1)) - (diff & m);
    return diff;
}

static size_t mpp_av1_get_payload_bytes_left(BitReadCtx_t *gbc)
{
    size_t size = 0;
    RK_U8 value = 0;
    RK_S32 i = 0;

    for (i = 0; mpp_get_bits_left(gbc) >= 8; i++) {
        READ_BITS(gbc, 8, &value);
        if (value)
            size = i;
    }
    return size;

__bitread_error:
    return MPP_NOK;

}

#define CHECK(call) do { \
        err = (call); \
        if (err < 0) \
            return err; \
    } while (0)


#define SUBSCRIPTS(subs, ...) (subs > 0 ? ((RK_S32[subs + 1]){ subs, __VA_ARGS__ }) : NULL)
#define fb(width, name) \
        xf(width, name, current->name, 0, MAX_UINT_BITS(width), 0, )
#define fc(width, name, range_min, range_max) \
        xf(width, name, current->name, range_min, range_max, 0, )
#define flag(name) fb(1, name)
#define su(width, name) \
        xsu(width, name, current->name, 0, )

#define fbs(width, name, subs, ...) \
        xf(width, name, current->name, 0, MAX_UINT_BITS(width), subs, __VA_ARGS__)
#define fcs(width, name, range_min, range_max, subs, ...) \
        xf(width, name, current->name, range_min, range_max, subs, __VA_ARGS__)
#define flags(name, subs, ...) \
        xf(1, name, current->name, 0, 1, subs, __VA_ARGS__)
#define sus(width, name, subs, ...) \
        xsu(width, name, current->name, subs, __VA_ARGS__)

#define xf(width, name, var, range_min, range_max, subs, ...) do { \
        RK_U32 value; \
        CHECK(mpp_av1_read_unsigned(gb, width, #name, \
                                   &value, range_min, range_max)); \
        var = value; \
    } while (0)

#define xsu(width, name, var, subs, ...) do { \
        RK_S32 value; \
        CHECK(mpp_av1_read_signed(gb, width, #name, \
                                 &value, \
                                 MIN_INT_BITS(width), \
                                 MAX_INT_BITS(width))); \
        var = value; \
    } while (0)

#define uvlc(name, range_min, range_max) do { \
        RK_U32 value; \
        CHECK(mpp_av1_read_uvlc(gb, #name, \
                                &value, range_min, range_max)); \
        current->name = value; \
    } while (0)

#define ns(max_value, name) do { \
        RK_U32 value; \
        CHECK(mpp_av1_read_ns(gb, #name, max_value, \
                               &value)); \
        current->name = value; \
    } while (0)

#define increment(name, min, max) do { \
        RK_U32 value; \
        CHECK(mpp_av1_read_increment(gb, min, max, #name, &value)); \
        current->name = value; \
    } while (0)

#define subexp(name, max) do { \
        RK_U32 value = 0; \
        CHECK(mpp_av1_read_subexp(gb, max, \
                                  &value)); \
        current->name = value; \
    } while (0)

#define delta_q(name) do { \
        RK_U8 delta_coded; \
        RK_S8 delta_q; \
        xf(1, name.delta_coded, delta_coded, 0, 1, 0, ); \
        if (delta_coded) \
            xsu(1 + 6, name.delta_q, delta_q, 0, ); \
        else \
            delta_q = 0; \
        current->name = delta_q; \
    } while (0)

#define leb128(name) do { \
        RK_U64 value; \
        CHECK(mpp_av1_read_leb128(gb, &value)); \
        current->name = value; \
    } while (0)

#define infer(name, value) do { \
        current->name = value; \
    } while (0)

#define byte_alignment(gb) (mpp_get_bits_count(gb) % 8)

static RK_S32 mpp_av1_read_obu_header(AV1Context *ctx, BitReadCtx_t *gb,
                                      AV1RawOBUHeader *current)
{
    RK_S32 err;

    fc(1, obu_forbidden_bit, 0, 0);

    fc(4, obu_type, 0, AV1_OBU_PADDING);
    flag(obu_extension_flag);
    flag(obu_has_size_field);

    fc(1, obu_reserved_1bit, 0, 0);

    if (current->obu_extension_flag) {
        fb(3, temporal_id);
        fb(2, spatial_id);
        fc(3, extension_header_reserved_3bits, 0, 0);
    } else {
        infer(temporal_id, 0);
        infer(spatial_id, 0);
    }

    ctx->temporal_id = current->temporal_id;
    ctx->spatial_id  = current->spatial_id;

    return 0;
}

static RK_S32 mpp_av1_trailing_bits(AV1Context *ctx, BitReadCtx_t *gb, RK_S32 nb_bits)
{
    (void)ctx;
    mpp_assert(nb_bits > 0);

    // fixed(1, trailing_one_bit, 1);
    mpp_skip_bits(gb, 1);

    --nb_bits;

    while (nb_bits > 0) {
        // fixed(1, trailing_zero_bit, 0);
        mpp_skip_bits(gb, 1);
        --nb_bits;
    }

    return 0;
}

static RK_S32 mpp_av1_byte_alignment(AV1Context *ctx, BitReadCtx_t *gb)
{

    (void)ctx;

    while (byte_alignment(gb) != 0)
        mpp_skip_bits(gb, 1);
    //fixed(1, zero_bit, 0);

    return 0;
}

static RK_S32 mpp_av1_color_config(AV1Context *ctx, BitReadCtx_t *gb,
                                   AV1RawColorConfig *current, RK_S32 seq_profile)
{
    RK_S32 err;

    flag(high_bitdepth);

    if (seq_profile == PROFILE_AV1_PROFESSIONAL &&
        current->high_bitdepth) {
        flag(twelve_bit);
        ctx->bit_depth = current->twelve_bit ? 12 : 10;
    } else {
        ctx->bit_depth = current->high_bitdepth ? 10 : 8;
    }

    if (seq_profile == PROFILE_AV1_HIGH)
        infer(mono_chrome, 0);
    else
        flag(mono_chrome);
    ctx->num_planes = current->mono_chrome ? 1 : 3;

    flag(color_description_present_flag);
    if (current->color_description_present_flag) {
        fb(8, color_primaries);
        fb(8, transfer_characteristics);
        fb(8, matrix_coefficients);
    } else {
        infer(color_primaries,          MPP_FRAME_PRI_UNSPECIFIED);
        infer(transfer_characteristics, MPP_FRAME_TRC_UNSPECIFIED);
        infer(matrix_coefficients,      MPP_FRAME_SPC_UNSPECIFIED);
    }

    if (current->mono_chrome) {
        flag(color_range);

        infer(subsampling_x, 1);
        infer(subsampling_y, 1);
        infer(chroma_sample_position, AV1_CSP_UNKNOWN);
        infer(separate_uv_delta_q, 0);

    } else if (current->color_primaries          == MPP_FRAME_PRI_BT709 &&
               current->transfer_characteristics == MPP_FRAME_TRC_IEC61966_2_1 &&
               current->matrix_coefficients      == MPP_FRAME_SPC_RGB) {
        infer(color_range,   1);
        infer(subsampling_x, 0);
        infer(subsampling_y, 0);
        flag(separate_uv_delta_q);

    } else {
        flag(color_range);

        if (seq_profile == PROFILE_AV1_MAIN) {
            infer(subsampling_x, 1);
            infer(subsampling_y, 1);
        } else if (seq_profile == PROFILE_AV1_HIGH) {
            infer(subsampling_x, 0);
            infer(subsampling_y, 0);
        } else {
            if (ctx->bit_depth == 12) {
                fb(1, subsampling_x);
                if (current->subsampling_x)
                    fb(1, subsampling_y);
                else
                    infer(subsampling_y, 0);
            } else {
                infer(subsampling_x, 1);
                infer(subsampling_y, 0);
            }
        }
        if (current->subsampling_x && current->subsampling_y) {
            fc(2, chroma_sample_position, AV1_CSP_UNKNOWN,
               AV1_CSP_COLOCATED);
        }

        flag(separate_uv_delta_q);
    }

    return 0;
}

static RK_S32 mpp_av1_timing_info(AV1Context *ctx, BitReadCtx_t *gb,
                                  AV1RawTimingInfo *current)
{
    (void)ctx;
    RK_S32 err;

    fc(32, num_units_in_display_tick, 1, MAX_UINT_BITS(32));
    fc(32, time_scale,                1, MAX_UINT_BITS(32));

    flag(equal_picture_interval);
    if (current->equal_picture_interval)
        uvlc(num_ticks_per_picture_minus_1, 0, MAX_UINT_BITS(32) - 1);

    return 0;
}

static RK_S32 mpp_av1_decoder_model_info(AV1Context *ctx, BitReadCtx_t *gb,
                                         AV1RawDecoderModelInfo *current)
{
    RK_S32 err;
    (void)ctx;
    fb(5, buffer_delay_length_minus_1);
    fb(32, num_units_in_decoding_tick);
    fb(5,  buffer_removal_time_length_minus_1);
    fb(5,  frame_presentation_time_length_minus_1);

    return 0;
}

static RK_S32 mpp_av1_sequence_header_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                          AV1RawSequenceHeader *current)
{
    RK_S32 i, err;

    fc(3, seq_profile, PROFILE_AV1_MAIN,
       PROFILE_AV1_PROFESSIONAL);
    flag(still_picture);
    flag(reduced_still_picture_header);

    if (current->reduced_still_picture_header) {
        infer(timing_info_present_flag,           0);
        infer(decoder_model_info_present_flag,    0);
        infer(initial_display_delay_present_flag, 0);
        infer(operating_points_cnt_minus_1,       0);
        infer(operating_point_idc[0],             0);

        fb(5, seq_level_idx[0]);

        infer(seq_tier[0], 0);
        infer(decoder_model_present_for_this_op[0],         0);
        infer(initial_display_delay_present_for_this_op[0], 0);

    } else {
        flag(timing_info_present_flag);
        if (current->timing_info_present_flag) {
            CHECK(mpp_av1_timing_info(ctx, gb, &current->timing_info));

            flag(decoder_model_info_present_flag);
            if (current->decoder_model_info_present_flag) {
                CHECK(mpp_av1_decoder_model_info
                      (ctx, gb, &current->decoder_model_info));
            }
        } else {
            infer(decoder_model_info_present_flag, 0);
        }

        flag(initial_display_delay_present_flag);

        fb(5, operating_points_cnt_minus_1);
        for (i = 0; i <= current->operating_points_cnt_minus_1; i++) {
            fbs(12, operating_point_idc[i], 1, i);
            fbs(5,  seq_level_idx[i], 1, i);

            if (current->seq_level_idx[i] > 7)
                flags(seq_tier[i], 1, i);
            else
                infer(seq_tier[i], 0);

            if (current->decoder_model_info_present_flag) {
                flags(decoder_model_present_for_this_op[i], 1, i);
                if (current->decoder_model_present_for_this_op[i]) {
                    RK_S32 n = current->decoder_model_info.buffer_delay_length_minus_1 + 1;
                    fbs(n, decoder_buffer_delay[i], 1, i);
                    fbs(n, encoder_buffer_delay[i], 1, i);
                    flags(low_delay_mode_flag[i], 1, i);
                }
            } else {
                infer(decoder_model_present_for_this_op[i], 0);
            }

            if (current->initial_display_delay_present_flag) {
                flags(initial_display_delay_present_for_this_op[i], 1, i);
                if (current->initial_display_delay_present_for_this_op[i])
                    fbs(4, initial_display_delay_minus_1[i], 1, i);
            }
        }
    }

    fb(4, frame_width_bits_minus_1);
    fb(4, frame_height_bits_minus_1);

    fb(current->frame_width_bits_minus_1  + 1, max_frame_width_minus_1);
    fb(current->frame_height_bits_minus_1 + 1, max_frame_height_minus_1);

    if (current->reduced_still_picture_header)
        infer(frame_id_numbers_present_flag, 0);
    else
        flag(frame_id_numbers_present_flag);
    if (current->frame_id_numbers_present_flag) {
        fb(4, delta_frame_id_length_minus_2);
        fb(3, additional_frame_id_length_minus_1);
    }

    flag(use_128x128_superblock);
    flag(enable_filter_intra);
    flag(enable_intra_edge_filter);

    if (current->reduced_still_picture_header) {
        infer(enable_interintra_compound, 0);
        infer(enable_masked_compound,     0);
        infer(enable_warped_motion,       0);
        infer(enable_dual_filter,         0);
        infer(enable_order_hint,          0);
        infer(enable_jnt_comp,            0);
        infer(enable_ref_frame_mvs,       0);

        infer(seq_force_screen_content_tools,
              AV1_SELECT_SCREEN_CONTENT_TOOLS);
        infer(seq_force_integer_mv,
              AV1_SELECT_INTEGER_MV);
    } else {
        flag(enable_interintra_compound);
        flag(enable_masked_compound);
        flag(enable_warped_motion);
        flag(enable_dual_filter);

        flag(enable_order_hint);
        if (current->enable_order_hint) {
            flag(enable_jnt_comp);
            flag(enable_ref_frame_mvs);
        } else {
            infer(enable_jnt_comp,      0);
            infer(enable_ref_frame_mvs, 0);
        }

        flag(seq_choose_screen_content_tools);
        if (current->seq_choose_screen_content_tools)
            infer(seq_force_screen_content_tools,
                  AV1_SELECT_SCREEN_CONTENT_TOOLS);
        else
            fb(1, seq_force_screen_content_tools);
        if (current->seq_force_screen_content_tools > 0) {
            flag(seq_choose_integer_mv);
            if (current->seq_choose_integer_mv)
                infer(seq_force_integer_mv,
                      AV1_SELECT_INTEGER_MV);
            else
                fb(1, seq_force_integer_mv);
        } else {
            infer(seq_force_integer_mv, AV1_SELECT_INTEGER_MV);
        }

        if (current->enable_order_hint)
            fb(3, order_hint_bits_minus_1);
    }

    flag(enable_superres);
    flag(enable_cdef);
    flag(enable_restoration);

    CHECK(mpp_av1_color_config(ctx, gb, &current->color_config,
                               current->seq_profile));

    flag(film_grain_params_present);

    return 0;
}

static RK_S32 mpp_av1_temporal_delimiter_obu(AV1Context *ctx, BitReadCtx_t *gb)
{
    (void)gb;
    ctx->seen_frame_header = 0;

    return 0;
}
static RK_S32 mpp_av1_set_frame_refs(AV1Context *ctx, BitReadCtx_t *gb,
                                     AV1RawFrameHeader *current)
{
    (void)gb;
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    static const RK_U8 ref_frame_list[AV1_NUM_REF_FRAMES - 2] = {
        AV1_REF_FRAME_LAST2, AV1_REF_FRAME_LAST3, AV1_REF_FRAME_BWDREF,
        AV1_REF_FRAME_ALTREF2, AV1_REF_FRAME_ALTREF
    };
    RK_S8 ref_frame_idx[AV1_REFS_PER_FRAME], used_frame[AV1_NUM_REF_FRAMES];
    RK_S8 shifted_order_hints[AV1_NUM_REF_FRAMES];
    RK_S32 cur_frame_hint, latest_order_hint, earliest_order_hint, ref;
    RK_S32 i, j;

    for (i = 0; i < AV1_REFS_PER_FRAME; i++)
        ref_frame_idx[i] = -1;
    ref_frame_idx[AV1_REF_FRAME_LAST - AV1_REF_FRAME_LAST] = current->last_frame_idx;
    ref_frame_idx[AV1_REF_FRAME_GOLDEN - AV1_REF_FRAME_LAST] = current->golden_frame_idx;

    for (i = 0; i < AV1_NUM_REF_FRAMES; i++)
        used_frame[i] = 0;
    used_frame[current->last_frame_idx] = 1;
    used_frame[current->golden_frame_idx] = 1;

    cur_frame_hint = 1 << (seq->order_hint_bits_minus_1);
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++)
        shifted_order_hints[i] = cur_frame_hint +
                                 mpp_av1_get_relative_dist(seq, ctx->ref_s[i].order_hint,
                                                           ctx->order_hint);

    latest_order_hint = shifted_order_hints[current->last_frame_idx];
    earliest_order_hint = shifted_order_hints[current->golden_frame_idx];

    ref = -1;
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        RK_S32 hint = shifted_order_hints[i];
        if (!used_frame[i] && hint >= cur_frame_hint &&
            (ref < 0 || hint >= latest_order_hint)) {
            ref = i;
            latest_order_hint = hint;
        }
    }
    if (ref >= 0) {
        ref_frame_idx[AV1_REF_FRAME_ALTREF - AV1_REF_FRAME_LAST] = ref;
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
    if (ref >= 0) {
        ref_frame_idx[AV1_REF_FRAME_ALTREF2 - AV1_REF_FRAME_LAST] = ref;
        used_frame[ref] = 1;
    }

    for (i = 0; i < AV1_REFS_PER_FRAME - 2; i++) {
        RK_S32 ref_frame = ref_frame_list[i];
        if (ref_frame_idx[ref_frame - AV1_REF_FRAME_LAST] < 0 ) {
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
        infer(ref_frame_idx[i], ref_frame_idx[i]);
    }

    return 0;
}

static RK_S32 mpp_av1_superres_params(AV1Context *ctx, BitReadCtx_t *gb,
                                      AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 denom, err;

    if (seq->enable_superres)
        flag(use_superres);
    else
        infer(use_superres, 0);

    if (current->use_superres) {
        fb(3, coded_denom);
        denom = current->coded_denom + AV1_SUPERRES_DENOM_MIN;
    } else {
        denom = AV1_SUPERRES_NUM;
    }

    ctx->upscaled_width = ctx->frame_width;
    ctx->frame_width = (ctx->upscaled_width * AV1_SUPERRES_NUM +
                        denom / 2) / denom;
    return 0;
}

static RK_S32 mpp_av1_frame_size(AV1Context *ctx, BitReadCtx_t *gb,
                                 AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 err;

    if (current->frame_size_override_flag) {
        fb(seq->frame_width_bits_minus_1 + 1,  frame_width_minus_1);
        fb(seq->frame_height_bits_minus_1 + 1, frame_height_minus_1);
    } else {
        infer(frame_width_minus_1,  seq->max_frame_width_minus_1);
        infer(frame_height_minus_1, seq->max_frame_height_minus_1);
    }

    ctx->frame_width  = current->frame_width_minus_1  + 1;
    ctx->frame_height = current->frame_height_minus_1 + 1;

    CHECK(mpp_av1_superres_params(ctx, gb, current));

    return 0;
}

static RK_S32 mpp_av1_render_size(AV1Context *ctx, BitReadCtx_t *gb,
                                  AV1RawFrameHeader *current)
{
    RK_S32 err;

    flag(render_and_frame_size_different);

    if (current->render_and_frame_size_different) {
        fb(16, render_width_minus_1);
        fb(16, render_height_minus_1);
    } else {
        infer(render_width_minus_1,  current->frame_width_minus_1);
        infer(render_height_minus_1, current->frame_height_minus_1);
    }

    ctx->render_width  = current->render_width_minus_1  + 1;
    ctx->render_height = current->render_height_minus_1 + 1;

    return 0;
}

static RK_S32 mpp_av1_frame_size_with_refs(AV1Context *ctx, BitReadCtx_t *gb,
                                           AV1RawFrameHeader *current)
{
    RK_S32 i, err;

    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        flags(found_ref[i], 1, i);
        if (current->found_ref[i]) {
            AV1ReferenceFrameState *ref =
                &ctx->ref_s[current->ref_frame_idx[i]];

            if (!ref->valid) {
                mpp_err_f("Missing reference frame needed for frame size "
                          "(ref = %d, ref_frame_idx = %d).\n",
                          i, current->ref_frame_idx[i]);
                return MPP_ERR_PROTOL;
            }

            infer(frame_width_minus_1,   ref->upscaled_width - 1);
            infer(frame_height_minus_1,  ref->frame_height - 1);
            infer(render_width_minus_1,  ref->render_width - 1);
            infer(render_height_minus_1, ref->render_height - 1);

            ctx->upscaled_width = ref->upscaled_width;
            ctx->frame_width    = ctx->upscaled_width;
            ctx->frame_height   = ref->frame_height;
            ctx->render_width   = ref->render_width;
            ctx->render_height  = ref->render_height;
            break;
        }
    }

    if (i >= AV1_REFS_PER_FRAME) {
        CHECK(mpp_av1_frame_size(ctx, gb, current));
        CHECK(mpp_av1_render_size(ctx, gb, current));
    } else {
        CHECK(mpp_av1_superres_params(ctx, gb, current));
    }

    return 0;
}

static RK_S32  mpp_av1_interpolation_filter(AV1Context *ctx, BitReadCtx_t *gb,
                                            AV1RawFrameHeader *current)
{
    RK_S32 err;
    (void)ctx;
    flag(is_filter_switchable);
    if (current->is_filter_switchable)
        infer(interpolation_filter,
              AV1_INTERPOLATION_FILTER_SWITCHABLE);
    else
        fb(2, interpolation_filter);

    return 0;
}

static RK_S32 mpp_av1_tile_info(AV1Context *ctx, BitReadCtx_t *gb,
                                AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 mi_cols, mi_rows, sb_cols, sb_rows, sb_shift, sb_size;
    RK_S32 max_tile_width_sb, max_tile_height_sb, max_tile_area_sb;
    RK_S32 min_log2_tile_cols, max_log2_tile_cols, max_log2_tile_rows;
    RK_S32 min_log2_tiles, min_log2_tile_rows;
    RK_S32 i, err;

    mi_cols = 2 * ((ctx->frame_width  + 7) >> 3);
    mi_rows = 2 * ((ctx->frame_height + 7) >> 3);

    sb_cols = seq->use_128x128_superblock ? ((mi_cols + 31) >> 5)
              : ((mi_cols + 15) >> 4);
    sb_rows = seq->use_128x128_superblock ? ((mi_rows + 31) >> 5)
              : ((mi_rows + 15) >> 4);

    sb_shift = seq->use_128x128_superblock ? 5 : 4;
    sb_size  = sb_shift + 2;

    max_tile_width_sb = AV1_MAX_TILE_WIDTH >> sb_size;
    max_tile_area_sb  = AV1_MAX_TILE_AREA  >> (2 * sb_size);

    min_log2_tile_cols = mpp_av1_tile_log2(max_tile_width_sb, sb_cols);
    max_log2_tile_cols = mpp_av1_tile_log2(1, MPP_MIN(sb_cols, AV1_MAX_TILE_COLS));
    max_log2_tile_rows = mpp_av1_tile_log2(1, MPP_MIN(sb_rows, AV1_MAX_TILE_ROWS));
    min_log2_tiles = MPP_MAX(min_log2_tile_cols,
                             mpp_av1_tile_log2(max_tile_area_sb, sb_rows * sb_cols));

    flag(uniform_tile_spacing_flag);

    if (current->uniform_tile_spacing_flag) {
        RK_S32 tile_width_sb, tile_height_sb;

        increment(tile_cols_log2, min_log2_tile_cols, max_log2_tile_cols);

        tile_width_sb = (sb_cols + (1 << current->tile_cols_log2) - 1) >>
                        current->tile_cols_log2;
        current->tile_cols = (sb_cols + tile_width_sb - 1) / tile_width_sb;

        min_log2_tile_rows = MPP_MAX(min_log2_tiles - current->tile_cols_log2, 0);

        increment(tile_rows_log2, min_log2_tile_rows, max_log2_tile_rows);

        tile_height_sb = (sb_rows + (1 << current->tile_rows_log2) - 1) >>
                         current->tile_rows_log2;
        current->tile_rows = (sb_rows + tile_height_sb - 1) / tile_height_sb;

        for (i = 0; i < current->tile_cols - 1; i++)
            infer(width_in_sbs_minus_1[i], tile_width_sb - 1);
        infer(width_in_sbs_minus_1[i],
              sb_cols - (current->tile_cols - 1) * tile_width_sb - 1);
        for (i = 0; i < current->tile_rows - 1; i++)
            infer(height_in_sbs_minus_1[i], tile_height_sb - 1);
        infer(height_in_sbs_minus_1[i],
              sb_rows - (current->tile_rows - 1) * tile_height_sb - 1);

    } else {
        RK_S32 widest_tile_sb, start_sb, size_sb, max_width, max_height;

        widest_tile_sb = 0;

        start_sb = 0;
        for (i = 0; start_sb < sb_cols && i < AV1_MAX_TILE_COLS; i++) {
            max_width = MPP_MIN(sb_cols - start_sb, max_tile_width_sb);
            ns(max_width, width_in_sbs_minus_1[i]);
            //ns(max_width, width_in_sbs_minus_1[i]);
            size_sb = current->width_in_sbs_minus_1[i] + 1;
            widest_tile_sb = MPP_MAX(size_sb, widest_tile_sb);
            start_sb += size_sb;
        }
        current->tile_cols_log2 = mpp_av1_tile_log2(1, i);
        current->tile_cols = i;

        if (min_log2_tiles > 0)
            max_tile_area_sb = (sb_rows * sb_cols) >> (min_log2_tiles + 1);
        else
            max_tile_area_sb = sb_rows * sb_cols;
        max_tile_height_sb = MPP_MAX(max_tile_area_sb / widest_tile_sb, 1);

        start_sb = 0;
        for (i = 0; start_sb < sb_rows && i < AV1_MAX_TILE_ROWS; i++) {
            max_height = MPP_MIN(sb_rows - start_sb, max_tile_height_sb);
            ns(max_height, height_in_sbs_minus_1[i]);
            size_sb = current->height_in_sbs_minus_1[i] + 1;
            start_sb += size_sb;
        }
        current->tile_rows_log2 = mpp_av1_tile_log2(1, i);
        current->tile_rows = i;
    }

    if (current->tile_cols_log2 > 0 ||
        current->tile_rows_log2 > 0) {
        fb(current->tile_cols_log2 + current->tile_rows_log2,
           context_update_tile_id);
        fb(2, tile_size_bytes_minus1);
    } else {
        infer(context_update_tile_id, 0);
        current->tile_size_bytes_minus1 = 3;
    }

    ctx->tile_cols = current->tile_cols;
    ctx->tile_rows = current->tile_rows;

    return 0;
}

static RK_S32 mpp_av1_quantization_params(AV1Context *ctx, BitReadCtx_t *gb,
                                          AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 err;

    fb(8, base_q_idx);

    delta_q(delta_q_y_dc);

    if (ctx->num_planes > 1) {
        if (seq->color_config.separate_uv_delta_q)
            flag(diff_uv_delta);
        else
            infer(diff_uv_delta, 0);

        delta_q(delta_q_u_dc);
        delta_q(delta_q_u_ac);

        if (current->diff_uv_delta) {
            delta_q(delta_q_v_dc);
            delta_q(delta_q_v_ac);
        } else {
            infer(delta_q_v_dc, current->delta_q_u_dc);
            infer(delta_q_v_ac, current->delta_q_u_ac);
        }
    } else {
        infer(delta_q_u_dc, 0);
        infer(delta_q_u_ac, 0);
        infer(delta_q_v_dc, 0);
        infer(delta_q_v_ac, 0);
    }

    flag(using_qmatrix);
    if (current->using_qmatrix) {
        fb(4, qm_y);
        fb(4, qm_u);
        if (seq->color_config.separate_uv_delta_q)
            fb(4, qm_v);
        else
            infer(qm_v, current->qm_u);
    }

    return 0;
}

static RK_S32 mpp_av1_segmentation_params(AV1Context *ctx, BitReadCtx_t *gb,
                                          AV1RawFrameHeader *current)
{
    static const RK_U8 bits[AV1_SEG_LVL_MAX] = { 8, 6, 6, 6, 6, 3, 0, 0 };
    static const RK_U8 sign[AV1_SEG_LVL_MAX] = { 1, 1, 1, 1, 1, 0, 0, 0 };
    static const RK_U8 default_feature_enabled[AV1_SEG_LVL_MAX] = { 0 };
    static const RK_S16 default_feature_value[AV1_SEG_LVL_MAX] = { 0 };
    RK_S32 i, j, err;

    flag(segmentation_enabled);

    if (current->segmentation_enabled) {
        if (current->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
            infer(segmentation_update_map,      1);
            infer(segmentation_temporal_update, 0);
            infer(segmentation_update_data,     1);
        } else {
            flag(segmentation_update_map);
            if (current->segmentation_update_map)
                flag(segmentation_temporal_update);
            else
                infer(segmentation_temporal_update, 0);
            flag(segmentation_update_data);
        }

        for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
            const RK_U8 *ref_feature_enabled;
            const RK_S16 *ref_feature_value;

            if (current->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
                ref_feature_enabled = default_feature_enabled;
                ref_feature_value = default_feature_value;
            } else {
                ref_feature_enabled =
                    ctx->ref_s[current->ref_frame_idx[current->primary_ref_frame]].feature_enabled[i];
                ref_feature_value =
                    ctx->ref_s[current->ref_frame_idx[current->primary_ref_frame]].feature_value[i];
            }

            for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
                if (current->segmentation_update_data) {
                    flags(feature_enabled[i][j], 2, i, j);

                    if (current->feature_enabled[i][j] && bits[j] > 0) {
                        if (sign[j])
                            sus(1 + bits[j], feature_value[i][j], 2, i, j);
                        else
                            fbs(bits[j], feature_value[i][j], 2, i, j);
                    } else {
                        infer(feature_value[i][j], 0);
                    }
                } else {
                    infer(feature_enabled[i][j], ref_feature_enabled[j]);
                    infer(feature_value[i][j], ref_feature_value[j]);
                }
            }
        }
    } else {
        for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
            for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
                infer(feature_enabled[i][j], 0);
                infer(feature_value[i][j],   0);
            }
        }
    }

    return 0;
}

static RK_S32 mpp_av1_delta_q_params(AV1Context *ctx, BitReadCtx_t *gb,
                                     AV1RawFrameHeader *current)
{
    RK_S32 err;
    (void)ctx;
    if (current->base_q_idx > 0)
        flag(delta_q_present);
    else
        infer(delta_q_present, 0);

    if (current->delta_q_present)
        fb(2, delta_q_res);

    return 0;
}

static RK_S32 mpp_av1_delta_lf_params(AV1Context *ctx, BitReadCtx_t *gb,
                                      AV1RawFrameHeader *current)
{
    RK_S32 err;
    (void)ctx;
    if (current->delta_q_present) {
        if (!current->allow_intrabc)
            flag(delta_lf_present);
        else
            infer(delta_lf_present, 0);
        if (current->delta_lf_present) {
            fb(2, delta_lf_res);
            flag(delta_lf_multi);
        } else {
            infer(delta_lf_res,   0);
            infer(delta_lf_multi, 0);
        }
    } else {
        infer(delta_lf_present, 0);
        infer(delta_lf_res,     0);
        infer(delta_lf_multi,   0);
    }

    return 0;
}

static RK_S32 mpp_av1_loop_filter_params(AV1Context *ctx, BitReadCtx_t *gb,
                                         AV1RawFrameHeader *current)
{
    static const RK_S8 default_loop_filter_ref_deltas[AV1_TOTAL_REFS_PER_FRAME] =
    { 1, 0, 0, 0, -1, 0, -1, -1 };
    static const RK_S8 default_loop_filter_mode_deltas[2] = { 0, 0 };
    RK_S32 i, err;

    if (ctx->coded_lossless || current->allow_intrabc) {
        infer(loop_filter_level[0], 0);
        infer(loop_filter_level[1], 0);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_INTRA],    1);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_LAST],     0);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_LAST2],    0);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_LAST3],    0);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_BWDREF],   0);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_GOLDEN],  -1);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_ALTREF],  -1);
        infer(loop_filter_ref_deltas[AV1_REF_FRAME_ALTREF2], -1);
        for (i = 0; i < 2; i++)
            infer(loop_filter_mode_deltas[i], 0);
        return 0;
    }

    fb(6, loop_filter_level[0]);
    fb(6, loop_filter_level[1]);

    if (ctx->num_planes > 1) {
        if (current->loop_filter_level[0] ||
            current->loop_filter_level[1]) {
            fb(6, loop_filter_level[2]);
            fb(6, loop_filter_level[3]);
        }
    }

    fb(3, loop_filter_sharpness);

    flag(loop_filter_delta_enabled);
    if (current->loop_filter_delta_enabled) {
        const RK_S8 *ref_loop_filter_ref_deltas, *ref_loop_filter_mode_deltas;

        if (current->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
            ref_loop_filter_ref_deltas = default_loop_filter_ref_deltas;
            ref_loop_filter_mode_deltas = default_loop_filter_mode_deltas;
        } else {
            ref_loop_filter_ref_deltas =
                ctx->ref_s[current->ref_frame_idx[current->primary_ref_frame]].loop_filter_ref_deltas;
            ref_loop_filter_mode_deltas =
                ctx->ref_s[current->ref_frame_idx[current->primary_ref_frame]].loop_filter_mode_deltas;
        }

        flag(loop_filter_delta_update);
        for (i = 0; i < AV1_TOTAL_REFS_PER_FRAME; i++) {
            if (current->loop_filter_delta_update)
                flags(update_ref_delta[i], 1, i);
            else
                infer(update_ref_delta[i], 0);
            if (current->update_ref_delta[i])
                sus(1 + 6, loop_filter_ref_deltas[i], 1, i);
            else
                infer(loop_filter_ref_deltas[i], ref_loop_filter_ref_deltas[i]);
        }
        for (i = 0; i < 2; i++) {
            if (current->loop_filter_delta_update)
                flags(update_mode_delta[i], 1, i);
            else
                infer(update_mode_delta[i], 0);
            if (current->update_mode_delta[i])
                sus(1 + 6, loop_filter_mode_deltas[i], 1, i);
            else
                infer(loop_filter_mode_deltas[i], ref_loop_filter_mode_deltas[i]);
        }
    } else {
        for (i = 0; i < AV1_TOTAL_REFS_PER_FRAME; i++)
            infer(loop_filter_ref_deltas[i], default_loop_filter_ref_deltas[i]);
        for (i = 0; i < 2; i++)
            infer(loop_filter_mode_deltas[i], default_loop_filter_mode_deltas[i]);
    }

    return 0;
}

static RK_S32 mpp_av1_cdef_params(AV1Context *ctx, BitReadCtx_t *gb,
                                  AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 i, err;
    if (ctx->coded_lossless || current->allow_intrabc ||
        !seq->enable_cdef) {
        infer(cdef_damping_minus_3, 0);
        infer(cdef_bits, 0);
        infer(cdef_y_pri_strength[0],  0);
        infer(cdef_y_sec_strength[0],  0);
        infer(cdef_uv_pri_strength[0], 0);
        infer(cdef_uv_sec_strength[0], 0);

        return 0;
    }

    fb(2, cdef_damping_minus_3);
    fb(2, cdef_bits);

    for (i = 0; i < (1 << current->cdef_bits); i++) {
        fbs(4, cdef_y_pri_strength[i], 1, i);
        fbs(2, cdef_y_sec_strength[i], 1, i);

        if (ctx->num_planes > 1) {
            fbs(4, cdef_uv_pri_strength[i], 1, i);
            fbs(2, cdef_uv_sec_strength[i], 1, i);
        }
    }

    return 0;
}

static RK_S32 mpp_av1_lr_params(AV1Context *ctx, BitReadCtx_t *gb,
                                AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 uses_lr,  uses_chroma_lr;
    RK_S32 i, err;

    if (ctx->all_lossless || current->allow_intrabc ||
        !seq->enable_restoration) {
        return 0;
    }

    uses_lr = uses_chroma_lr = 0;
    for (i = 0; i < ctx->num_planes; i++) {
        fbs(2, lr_type[i], 1, i);

        if (current->lr_type[i] != AV1_RESTORE_NONE) {
            uses_lr = 1;
            if (i > 0)
                uses_chroma_lr = 1;
        }
    }

    if (uses_lr) {
        if (seq->use_128x128_superblock)
            increment(lr_unit_shift, 1, 2);
        else
            increment(lr_unit_shift, 0, 2);

        if (seq->color_config.subsampling_x &&
            seq->color_config.subsampling_y && uses_chroma_lr) {
            fb(1, lr_uv_shift);
        } else {
            infer(lr_uv_shift, 0);
        }
    }

    return 0;
}

static RK_S32 mpp_av1_read_tx_mode(AV1Context *ctx, BitReadCtx_t *gb,
                                   AV1RawFrameHeader *current)
{
    RK_S32 err;

    if (ctx->coded_lossless)
        infer(tx_mode, 0);
    else
        increment(tx_mode, 1, 2);
    if (current->tx_mode == 1) {
        current->tx_mode = 3;
    } else {
        current->tx_mode = 4;
    }
    return 0;
}

static RK_S32 mpp_av1_frame_reference_mode(AV1Context *ctx, BitReadCtx_t *gb,
                                           AV1RawFrameHeader *current)
{
    RK_S32 err;
    (void)ctx;
    if (current->frame_type == AV1_FRAME_INTRA_ONLY ||
        current->frame_type == AV1_FRAME_KEY)
        infer(reference_select, 0);
    else
        flag(reference_select);

    return 0;
}

static RK_S32 mpp_av1_skip_mode_params(AV1Context *ctx, BitReadCtx_t *gb,
                                       AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 skip_mode_allowed;
    RK_S32 err;

    if (current->frame_type == AV1_FRAME_KEY ||
        current->frame_type == AV1_FRAME_INTRA_ONLY ||
        !current->reference_select || !seq->enable_order_hint) {
        skip_mode_allowed = 0;
    } else {
        RK_S32 forward_idx,  backward_idx;
        RK_S32 forward_hint, backward_hint;
        RK_S32 ref_hint, dist, i;

        forward_idx  = -1;
        backward_idx = -1;
        forward_hint = -1;
        backward_hint = -1;
        for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
            ref_hint = ctx->ref_s[current->ref_frame_idx[i]].order_hint;
            dist = mpp_av1_get_relative_dist(seq, ref_hint,
                                             ctx->order_hint);
            if (dist < 0) {
                if (forward_idx < 0 ||
                    mpp_av1_get_relative_dist(seq, ref_hint,
                                              forward_hint) > 0) {
                    forward_idx  = i;
                    forward_hint = ref_hint;
                }
            } else if (dist > 0) {
                if (backward_idx < 0 ||
                    mpp_av1_get_relative_dist(seq, ref_hint,
                                              backward_hint) < 0) {
                    backward_idx  = i;
                    backward_hint = ref_hint;
                }
            }
        }

        if (forward_idx < 0) {
            skip_mode_allowed = 0;
        } else if (backward_idx >= 0) {
            skip_mode_allowed = 1;
            ctx->skip_ref0 = MPP_MIN(forward_idx, backward_idx) + 1;
            ctx->skip_ref1 = MPP_MAX(forward_idx, backward_idx) + 1;
            // Frames for skip mode are forward_idx and backward_idx.
        } else {
            RK_S32 second_forward_idx;
            RK_S32 second_forward_hint;
            second_forward_idx = -1;
            for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
                ref_hint = ctx->ref_s[current->ref_frame_idx[i]].order_hint;
                if (mpp_av1_get_relative_dist(seq, ref_hint,
                                              forward_hint) < 0) {
                    if (second_forward_idx < 0 ||
                        mpp_av1_get_relative_dist(seq, ref_hint,
                                                  second_forward_hint) > 0) {
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
                // Frames for skip mode are forward_idx and second_forward_idx.
            }
        }
    }

    if (skip_mode_allowed)
        flag(skip_mode_present);
    else
        infer(skip_mode_present, 0);

    return 0;
}

static RK_S32 mpp_av1_global_motion_param(AV1Context *ctx, BitReadCtx_t *gb,
                                          AV1RawFrameHeader *current,
                                          RK_S32 type, RK_S32 ref, RK_S32 idx)
{
    RK_U32 abs_bits, prec_bits, num_syms;
    RK_S32 err;
    (void)ctx;
    if (idx < 2) {
        if (type == AV1_WARP_MODEL_TRANSLATION) {
            abs_bits  = AV1_GM_ABS_TRANS_ONLY_BITS  - !current->allow_high_precision_mv;
            prec_bits = AV1_GM_TRANS_ONLY_PREC_BITS - !current->allow_high_precision_mv;
        } else {
            abs_bits  = AV1_GM_ABS_TRANS_BITS;
            prec_bits = AV1_GM_TRANS_PREC_BITS;
        }
    } else {
        abs_bits  = AV1_GM_ABS_ALPHA_BITS;
        prec_bits = AV1_GM_ALPHA_PREC_BITS;
    }

    num_syms = 2 * (1 << abs_bits) + 1;
    subexp(gm_params[ref][idx], num_syms);// 2, ref, idx);

    // Actual gm_params value is not reconstructed here.
    (void)prec_bits;

    return 0;
}

static RK_S32 mpp_av1_global_motion_params(AV1Context *ctx, BitReadCtx_t *gb,
                                           AV1RawFrameHeader *current)
{
    RK_S32 ref, type;
    RK_S32 err;

    if (current->frame_type == AV1_FRAME_KEY ||
        current->frame_type == AV1_FRAME_INTRA_ONLY)
        return 0;

    for (ref = AV1_REF_FRAME_LAST; ref <= AV1_REF_FRAME_ALTREF; ref++) {
        flags(is_global[ref], 1, ref);
        if (current->is_global[ref]) {
            flags(is_rot_zoom[ref], 1, ref);
            if (current->is_rot_zoom[ref]) {
                type = AV1_WARP_MODEL_ROTZOOM;
            } else {
                flags(is_translation[ref], 1, ref);
                type = current->is_translation[ref] ? AV1_WARP_MODEL_TRANSLATION
                       : AV1_WARP_MODEL_AFFINE;
            }
        } else {
            type = AV1_WARP_MODEL_IDENTITY;
        }

        if (type >= AV1_WARP_MODEL_ROTZOOM) {
            CHECK(mpp_av1_global_motion_param(ctx, gb, current, type, ref, 2));
            CHECK(mpp_av1_global_motion_param(ctx, gb, current, type, ref, 3));
            if (type == AV1_WARP_MODEL_AFFINE) {
                CHECK(mpp_av1_global_motion_param(ctx, gb, current, type, ref, 4));
                CHECK(mpp_av1_global_motion_param(ctx, gb, current, type, ref, 5));
            } else {
                current->gm_params[ref][4] =  -current->gm_params[ref][3];
                current->gm_params[ref][5] =   current->gm_params[ref][2];
            }
        }
        if (type >= AV1_WARP_MODEL_TRANSLATION) {
            CHECK(mpp_av1_global_motion_param(ctx, gb, current, type, ref, 0));
            CHECK(mpp_av1_global_motion_param(ctx, gb, current, type, ref, 1));
        }
    }
    /* // update alpha..
    if (params->wmtype <= AFFINE) {
        int good_shear_params = get_shear_params(params);
        if (!good_shear_params) return 0;
    }
     */

    return 0;
}

static RK_S32 mpp_av1_film_grain_params(AV1Context *ctx, BitReadCtx_t *gb,
                                        AV1RawFilmGrainParams *current,
                                        AV1RawFrameHeader *frame_header)
{
    const AV1RawSequenceHeader *seq = ctx->sequence_header;
    RK_S32 num_pos_luma, num_pos_chroma;
    RK_S32 i, err;

    if (!seq->film_grain_params_present ||
        (!frame_header->show_frame && !frame_header->showable_frame))
        return 0;

    flag(apply_grain);

    if (!current->apply_grain)
        return 0;

    fb(16, grain_seed);

    if (frame_header->frame_type == AV1_FRAME_INTER)
        flag(update_grain);
    else
        infer(update_grain, 1);

    if (!current->update_grain) {
        fb(3, film_grain_params_ref_idx);
        return 0;
    }

    fc(4, num_y_points, 0, 14);
    for (i = 0; i < current->num_y_points; i++) {
        fcs(8, point_y_value[i],
            i ? current->point_y_value[i - 1] + 1 : 0,
            MAX_UINT_BITS(8) - (current->num_y_points - i - 1),
            1, i);
        fbs(8, point_y_scaling[i], 1, i);
    }

    if (seq->color_config.mono_chrome)
        infer(chroma_scaling_from_luma, 0);
    else
        flag(chroma_scaling_from_luma);

    if (seq->color_config.mono_chrome ||
        current->chroma_scaling_from_luma ||
        (seq->color_config.subsampling_x == 1 &&
         seq->color_config.subsampling_y == 1 &&
         current->num_y_points == 0)) {
        infer(num_cb_points, 0);
        infer(num_cr_points, 0);
    } else {
        fc(4, num_cb_points, 0, 10);
        for (i = 0; i < current->num_cb_points; i++) {
            fcs(8, point_cb_value[i],
                i ? current->point_cb_value[i - 1] + 1 : 0,
                MAX_UINT_BITS(8) - (current->num_cb_points - i - 1),
                1, i);
            fbs(8, point_cb_scaling[i], 1, i);
        }
        fc(4, num_cr_points, 0, 10);
        for (i = 0; i < current->num_cr_points; i++) {
            fcs(8, point_cr_value[i],
                i ? current->point_cr_value[i - 1] + 1 : 0,
                MAX_UINT_BITS(8) - (current->num_cr_points - i - 1),
                1, i);
            fbs(8, point_cr_scaling[i], 1, i);
        }
    }

    fb(2, grain_scaling_minus_8);
    fb(2, ar_coeff_lag);
    num_pos_luma = 2 * current->ar_coeff_lag * (current->ar_coeff_lag + 1);
    if (current->num_y_points) {
        num_pos_chroma = num_pos_luma + 1;
        for (i = 0; i < num_pos_luma; i++)
            fbs(8, ar_coeffs_y_plus_128[i], 1, i);
    } else {
        num_pos_chroma = num_pos_luma;
    }
    if (current->chroma_scaling_from_luma || current->num_cb_points) {
        for (i = 0; i < num_pos_chroma; i++)
            fbs(8, ar_coeffs_cb_plus_128[i], 1, i);
    }
    if (current->chroma_scaling_from_luma || current->num_cr_points) {
        for (i = 0; i < num_pos_chroma; i++)
            fbs(8, ar_coeffs_cr_plus_128[i], 1, i);
    }
    fb(2, ar_coeff_shift_minus_6);
    fb(2, grain_scale_shift);
    if (current->num_cb_points) {
        fb(8, cb_mult);
        fb(8, cb_luma_mult);
        fb(9, cb_offset);
    }
    if (current->num_cr_points) {
        fb(8, cr_mult);
        fb(8, cr_luma_mult);
        fb(9, cr_offset);
    }

    flag(overlap_flag);
    flag(clip_to_restricted_range);

    return 0;
}

static RK_S32 mpp_av1_uncompressed_header(AV1Context *ctx, BitReadCtx_t *gb,
                                          AV1RawFrameHeader *current)
{
    const AV1RawSequenceHeader *seq;
    RK_S32 id_len, diff_len, all_frames, frame_is_intra, order_hint_bits;
    RK_S32 i, err;

    RK_S32 start_pos = mpp_get_bits_count(gb);

    if (!ctx->sequence_header) {
        mpp_err_f("No sequence header available: "
                  "unable to decode frame header.\n");
        return MPP_ERR_UNKNOW;
    }
    seq = ctx->sequence_header;

    id_len = seq->additional_frame_id_length_minus_1 +
             seq->delta_frame_id_length_minus_2 + 3;
    all_frames = (1 << AV1_NUM_REF_FRAMES) - 1;

    if (seq->reduced_still_picture_header) {
        infer(show_existing_frame, 0);
        infer(frame_type,     AV1_FRAME_KEY);
        infer(show_frame,     1);
        infer(showable_frame, 0);
        frame_is_intra = 1;

    } else {
        flag(show_existing_frame);

        if (current->show_existing_frame) {
            AV1ReferenceFrameState *ref;

            fb(3, frame_to_show_map_idx);
            ref = &ctx->ref_s[current->frame_to_show_map_idx];

            if (!ref->valid) {
                mpp_err_f("Missing reference frame needed for "
                          "show_existing_frame (frame_to_show_map_idx = %d).\n",
                          current->frame_to_show_map_idx);
                return MPP_ERR_UNKNOW;
            }

            if (seq->decoder_model_info_present_flag &&
                !seq->timing_info.equal_picture_interval) {
                fb(seq->decoder_model_info.frame_presentation_time_length_minus_1 + 1,
                   frame_presentation_time);
            }

            if (seq->frame_id_numbers_present_flag)
                fb(id_len, display_frame_id);

            infer(frame_type, ref->frame_type);
            if (current->frame_type == AV1_FRAME_KEY) {
                infer(refresh_frame_flags, all_frames);

                // Section 7.21
                infer(current_frame_id, ref->frame_id);
                ctx->upscaled_width  = ref->upscaled_width;
                ctx->frame_width     = ref->frame_width;
                ctx->frame_height    = ref->frame_height;
                ctx->render_width    = ref->render_width;
                ctx->render_height   = ref->render_height;
                ctx->bit_depth       = ref->bit_depth;
                ctx->order_hint      = ref->order_hint;
            } else
                infer(refresh_frame_flags, 0);

            infer(frame_width_minus_1,   ref->upscaled_width - 1);
            infer(frame_height_minus_1,  ref->frame_height - 1);
            infer(render_width_minus_1,  ref->render_width - 1);
            infer(render_height_minus_1, ref->render_height - 1);

            // Section 7.20
            goto update_refs;
        }

        fb(2, frame_type);
        frame_is_intra = (current->frame_type == AV1_FRAME_INTRA_ONLY ||
                          current->frame_type == AV1_FRAME_KEY);

        ctx->frame_is_intra = frame_is_intra;
        if (current->frame_type == AV1_FRAME_KEY) {
            RK_U32 refresh_frame_flags = (1 << NUM_REF_FRAMES) - 1;

            Av1GetCDFs(ctx, current->frame_to_show_map_idx);
            Av1StoreCDFs(ctx, refresh_frame_flags);
        }

        flag(show_frame);
        if (current->show_frame &&
            seq->decoder_model_info_present_flag &&
            !seq->timing_info.equal_picture_interval) {
            fb(seq->decoder_model_info.frame_presentation_time_length_minus_1 + 1,
               frame_presentation_time);
        }
        if (current->show_frame)
            infer(showable_frame, current->frame_type != AV1_FRAME_KEY);
        else
            flag(showable_frame);

        if (current->frame_type == AV1_FRAME_SWITCH ||
            (current->frame_type == AV1_FRAME_KEY && current->show_frame))
            infer(error_resilient_mode, 1);
        else
            flag(error_resilient_mode);
    }

    if (current->frame_type == AV1_FRAME_KEY && current->show_frame) {
        for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
            ctx->ref_s[i].valid = 0;
            ctx->ref_s[i].order_hint = 0;
        }
    }

    flag(disable_cdf_update);

    if (seq->seq_force_screen_content_tools ==
        AV1_SELECT_SCREEN_CONTENT_TOOLS) {
        flag(allow_screen_content_tools);
    } else {
        infer(allow_screen_content_tools,
              seq->seq_force_screen_content_tools);
    }
    if (current->allow_screen_content_tools) {
        if (seq->seq_force_integer_mv == AV1_SELECT_INTEGER_MV)
            flag(force_integer_mv);
        else
            infer(force_integer_mv, seq->seq_force_integer_mv);
    } else {
        infer(force_integer_mv, 0);
    }

    if (seq->frame_id_numbers_present_flag) {
        fb(id_len, current_frame_id);

        diff_len = seq->delta_frame_id_length_minus_2 + 2;
        for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
            if (current->current_frame_id > (RK_S32)(1 << diff_len)) {
                if (ctx->ref_s[i].frame_id > current->current_frame_id ||
                    ctx->ref_s[i].frame_id < (current->current_frame_id -
                                              (RK_S32)(1 << diff_len)))
                    ctx->ref_s[i].valid = 0;
            } else {
                if (ctx->ref_s[i].frame_id > current->current_frame_id &&
                    ctx->ref_s[i].frame_id < ((RK_S32)(1 << id_len) +
                                              current->current_frame_id -
                                              (RK_S32)(1 << diff_len)))
                    ctx->ref_s[i].valid = 0;
            }
        }
    } else {
        infer(current_frame_id, 0);
    }

    if (current->frame_type == AV1_FRAME_SWITCH)
        infer(frame_size_override_flag, 1);
    else if (seq->reduced_still_picture_header)
        infer(frame_size_override_flag, 0);
    else
        flag(frame_size_override_flag);

    order_hint_bits =
        seq->enable_order_hint ? seq->order_hint_bits_minus_1 + 1 : 0;
    if (order_hint_bits > 0)
        fb(order_hint_bits, order_hint);
    else
        infer(order_hint, 0);
    ctx->order_hint = current->order_hint;

    if (frame_is_intra || current->error_resilient_mode)
        infer(primary_ref_frame, AV1_PRIMARY_REF_NONE);
    else
        fb(3, primary_ref_frame);

    if (seq->decoder_model_info_present_flag) {
        flag(buffer_removal_time_present_flag);
        if (current->buffer_removal_time_present_flag) {
            for (i = 0; i <= seq->operating_points_cnt_minus_1; i++) {
                if (seq->decoder_model_present_for_this_op[i]) {
                    RK_S32 op_pt_idc = seq->operating_point_idc[i];
                    RK_S32 in_temporal_layer = (op_pt_idc >>  ctx->temporal_id    ) & 1;
                    RK_S32 in_spatial_layer  = (op_pt_idc >> (ctx->spatial_id + 8)) & 1;
                    if (seq->operating_point_idc[i] == 0 ||
                        (in_temporal_layer && in_spatial_layer)) {
                        fbs(seq->decoder_model_info.buffer_removal_time_length_minus_1 + 1,
                            buffer_removal_time[i], 1, i);
                    }
                }
            }
        }
    }

    if (current->frame_type == AV1_FRAME_SWITCH ||
        (current->frame_type == AV1_FRAME_KEY && current->show_frame))
        infer(refresh_frame_flags, all_frames);
    else
        fb(8, refresh_frame_flags);

    ctx->refresh_frame_flags = current->refresh_frame_flags;
    if (!frame_is_intra || current->refresh_frame_flags != all_frames) {
        if (seq->enable_order_hint) {
            for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
                if (current->error_resilient_mode)
                    fbs(order_hint_bits, ref_order_hint[i], 1, i);
                else
                    infer(ref_order_hint[i], ctx->ref_s[i].order_hint);
                if (current->ref_order_hint[i] != ctx->ref_s[i].order_hint)
                    ctx->ref_s[i].valid = 0;
            }
        }
    }

    if (current->frame_type == AV1_FRAME_KEY ||
        current->frame_type == AV1_FRAME_INTRA_ONLY) {
        CHECK(mpp_av1_frame_size(ctx, gb, current));
        CHECK(mpp_av1_render_size(ctx, gb, current));

        if (current->allow_screen_content_tools &&
            ctx->upscaled_width == ctx->frame_width)
            flag(allow_intrabc);
        else
            infer(allow_intrabc, 0);

    } else {
        if (!seq->enable_order_hint) {
            infer(frame_refs_short_signaling, 0);
        } else {
            flag(frame_refs_short_signaling);
            if (current->frame_refs_short_signaling) {
                fb(3, last_frame_idx);
                fb(3, golden_frame_idx);
                CHECK(mpp_av1_set_frame_refs(ctx, gb, current));
            }
        }

        for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
            if (!current->frame_refs_short_signaling)
                fbs(3, ref_frame_idx[i], 1, i);
            if (seq->frame_id_numbers_present_flag) {
                fbs(seq->delta_frame_id_length_minus_2 + 2,
                    delta_frame_id_minus1[i], 1, i);
            }
        }

        if (current->frame_size_override_flag &&
            !current->error_resilient_mode) {
            CHECK(mpp_av1_frame_size_with_refs(ctx, gb, current));
        } else {
            CHECK(mpp_av1_frame_size(ctx, gb, current));
            CHECK(mpp_av1_render_size(ctx, gb, current));
        }

        if (current->force_integer_mv)
            infer(allow_high_precision_mv, 0);
        else
            flag(allow_high_precision_mv);

        CHECK(mpp_av1_interpolation_filter(ctx, gb, current));

        flag(is_motion_mode_switchable);

        if (current->error_resilient_mode ||
            !seq->enable_ref_frame_mvs)
            infer(use_ref_frame_mvs, 0);
        else
            flag(use_ref_frame_mvs);

        infer(allow_intrabc, 0);
    }

    if (!frame_is_intra) {
        // Derive reference frame sign biases.
    }

    if (seq->reduced_still_picture_header || current->disable_cdf_update)
        infer(disable_frame_end_update_cdf, 1);
    else
        flag(disable_frame_end_update_cdf);

    ctx->disable_frame_end_update_cdf = current->disable_frame_end_update_cdf;

    if (current->use_ref_frame_mvs) {
        // Perform motion field estimation process.
    }
    av1d_dbg(AV1D_DBG_HEADER, "ptile_info in %d", mpp_get_bits_count(gb));
    CHECK(mpp_av1_tile_info(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "ptile_info out %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_quantization_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "quantization out %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_segmentation_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "segmentation out %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_delta_q_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "delta_q out %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_delta_lf_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "lf out %d", mpp_get_bits_count(gb));

    // Init coeff CDFs / load previous segments.
    if (current->error_resilient_mode || frame_is_intra || current->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
        // Init non-coeff CDFs.
        // Setup past independence.
        ctx->cdfs = &ctx->default_cdfs;
        ctx->cdfs_ndvc = &ctx->default_cdfs_ndvc;
        Av1DefaultCoeffProbs(current->base_q_idx, ctx->cdfs);
    } else {
        // Load CDF tables from previous frame.
        // Load params from previous frame.
        RK_U32 idx = current->ref_frame_idx[current->primary_ref_frame];

        Av1GetCDFs(ctx, idx);
    }
    av1d_dbg(AV1D_DBG_HEADER, "show_existing_frame_index %d primary_ref_frame %d %d (%d) refresh_frame_flags %d base_q_idx %d\n",
             current->frame_to_show_map_idx,
             current->ref_frame_idx[current->primary_ref_frame],
             ctx->ref[current->ref_frame_idx[current->primary_ref_frame]].slot_index,
             current->primary_ref_frame,
             current->refresh_frame_flags,
             current->base_q_idx);
    Av1StoreCDFs(ctx, current->refresh_frame_flags);

    ctx->coded_lossless = 1;
    for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
        RK_S32 qindex;
        if (current->feature_enabled[i][AV1_SEG_LVL_ALT_Q]) {
            qindex = (current->base_q_idx +
                      current->feature_value[i][AV1_SEG_LVL_ALT_Q]);
        } else {
            qindex = current->base_q_idx;
        }
        qindex = mpp_clip_uintp2(qindex, 8);

        if (qindex                || current->delta_q_y_dc ||
            current->delta_q_u_ac || current->delta_q_u_dc ||
            current->delta_q_v_ac || current->delta_q_v_dc) {
            ctx->coded_lossless = 0;
        }
    }
    ctx->all_lossless = ctx->coded_lossless &&
                        ctx->frame_width == ctx->upscaled_width;
    av1d_dbg(AV1D_DBG_HEADER, "filter in %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_loop_filter_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "cdef in %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_cdef_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "lr in %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_lr_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "read_tx in %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_read_tx_mode(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "reference in%d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_frame_reference_mode(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "kip_mode in %d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_skip_mode_params(ctx, gb, current));

    if (frame_is_intra || current->error_resilient_mode ||
        !seq->enable_warped_motion)
        infer(allow_warped_motion, 0);
    else
        flag(allow_warped_motion);

    flag(reduced_tx_set);
    av1d_dbg(AV1D_DBG_HEADER, "motion in%d", mpp_get_bits_count(gb));

    CHECK(mpp_av1_global_motion_params(ctx, gb, current));
    av1d_dbg(AV1D_DBG_HEADER, "grain in %d", mpp_get_bits_count(gb));
    CHECK(mpp_av1_film_grain_params(ctx, gb, &current->film_grain, current));
    av1d_dbg(AV1D_DBG_HEADER, "film_grain out %d", mpp_get_bits_count(gb));
    ctx->frame_tag_size = ((mpp_get_bits_count(gb) - start_pos) + 7) / 8;


    av1d_dbg(AV1D_DBG_REF, "Frame %d:  size %dx%d  "
             "upscaled %d  render %dx%d  subsample %dx%d  "
             "bitdepth %d  tiles %dx%d.\n", ctx->order_hint,
             ctx->frame_width, ctx->frame_height, ctx->upscaled_width,
             ctx->render_width, ctx->render_height,
             seq->color_config.subsampling_x + 1,
             seq->color_config.subsampling_y + 1, ctx->bit_depth,
             ctx->tile_rows, ctx->tile_cols);

update_refs:
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        if (current->refresh_frame_flags & (1 << i)) {
            ctx->ref_s[i] = (AV1ReferenceFrameState) {
                .valid          = 1,
                 .frame_id       = current->current_frame_id,
                  .upscaled_width = ctx->upscaled_width,
                   .frame_width    = ctx->frame_width,
                    .frame_height   = ctx->frame_height,
                     .render_width   = ctx->render_width,
                      .render_height  = ctx->render_height,
                       .frame_type     = current->frame_type,
                        .subsampling_x  = seq->color_config.subsampling_x,
                         .subsampling_y  = seq->color_config.subsampling_y,
                          .bit_depth      = ctx->bit_depth,
                           .order_hint     = ctx->order_hint,
            };
            memcpy(ctx->ref_s[i].loop_filter_ref_deltas, current->loop_filter_ref_deltas,
                   sizeof(current->loop_filter_ref_deltas));
            memcpy(ctx->ref_s[i].loop_filter_mode_deltas, current->loop_filter_mode_deltas,
                   sizeof(current->loop_filter_mode_deltas));
            memcpy(ctx->ref_s[i].feature_enabled, current->feature_enabled,
                   sizeof(current->feature_enabled));
            memcpy(ctx->ref_s[i].feature_value, current->feature_value,
                   sizeof(current->feature_value));
        }
    }

    return 0;
}

static RK_S32 mpp_av1_frame_header_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                       AV1RawFrameHeader *current, RK_S32 redundant,
                                       void *rw_buffer_ref)
{
    RK_S32 start_pos, fh_bits, fh_bytes, err;
    RK_U8 *fh_start;
    (void)rw_buffer_ref;
    if (ctx->seen_frame_header) {
        if (!redundant) {
            mpp_err_f("Invalid repeated "
                      "frame header OBU.\n");
            return MPP_ERR_UNKNOW;
        } else {
            BitReadCtx_t fh;
            size_t i, b;
            RK_U32 val;

//            mpp_assert(ctx->frame_header_ref && ctx->frame_header);

            mpp_set_bitread_ctx(&fh, ctx->frame_header,
                                ctx->frame_header_size);

            for (i = 0; i < ctx->frame_header_size; i += 8) {
                b = MPP_MIN(ctx->frame_header_size - i, 8);
                mpp_read_bits(&fh, b, (RK_S32*)&val);
                xf(b, frame_header_copy[i],
                   val, val, val, 1, i / 8);
            }
        }
    } else {

        start_pos = mpp_get_bits_count(gb);

        CHECK(mpp_av1_uncompressed_header(ctx, gb, current));

        ctx->tile_num = 0;

        if (current->show_existing_frame) {
            ctx->seen_frame_header = 0;
        } else {
            ctx->seen_frame_header = 1;

            fh_bits  = mpp_get_bits_count(gb) - start_pos;
            fh_start = (RK_U8*)gb->buf + start_pos / 8;

            fh_bytes = (fh_bits + 7) / 8;

            ctx->frame_header_size = fh_bits;
            MPP_FREE(ctx->frame_header);

            ctx->frame_header =
                mpp_malloc(RK_U8, fh_bytes + BUFFER_PADDING_SIZE);
            if (!ctx->frame_header)
                return MPP_ERR_NOMEM;
            memcpy(ctx->frame_header, fh_start, fh_bytes);
        }
    }

    return 0;
}

static RK_S32 mpp_av1_tile_group_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                     AV1RawTileGroup *current)
{
    RK_S32 num_tiles, tile_bits;
    RK_S32 err;
    RK_S32 cur_pos = mpp_get_bits_count(gb);

    num_tiles = ctx->tile_cols * ctx->tile_rows;
    if (num_tiles > 1)
        flag(tile_start_and_end_present_flag);
    else
        infer(tile_start_and_end_present_flag, 0);

    if (num_tiles == 1 || !current->tile_start_and_end_present_flag) {
        infer(tg_start, 0);
        infer(tg_end, num_tiles - 1);
    } else {
        tile_bits = mpp_av1_tile_log2(1, ctx->tile_cols) +
                    mpp_av1_tile_log2(1, ctx->tile_rows);
        fc(tile_bits, tg_start, ctx->tile_num, num_tiles - 1);
        fc(tile_bits, tg_end, current->tg_start, num_tiles - 1);
    }

    ctx->tile_num = current->tg_end + 1;

    CHECK(mpp_av1_byte_alignment(ctx, gb));

    // Reset header for next frame.
    if (current->tg_end == num_tiles - 1)
        ctx->seen_frame_header = 0;

    ctx->frame_tag_size += MPP_ALIGN(mpp_get_bits_count(gb) - cur_pos, 8) / 8;

    // Tile data follows.

    return 0;
}

static RK_S32 mpp_av1_frame_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                AV1RawFrame *current,
                                void *rw_buffer_ref)
{
    RK_S32 err;

    CHECK(mpp_av1_frame_header_obu(ctx, gb, &current->header,
                                   0, rw_buffer_ref));

    CHECK(mpp_av1_byte_alignment(ctx, gb));

    CHECK(mpp_av1_tile_group_obu(ctx, gb, &current->tile_group));

    return 0;
}

static RK_S32 mpp_av1_tile_list_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                    AV1RawTileList *current)
{
    RK_S32 err;
    (void)ctx;
    fb(8, output_frame_width_in_tiles_minus_1);
    fb(8, output_frame_height_in_tiles_minus_1);

    fb(16, tile_count_minus_1);

    // Tile data follows.

    return 0;
}

static RK_S32 mpp_av1_metadata_hdr_cll(AV1Context *ctx, BitReadCtx_t *gb,
                                       AV1RawMetadataHDRCLL *current)
{
    RK_S32 err;
    (void)ctx;
    fb(16, max_cll);
    fb(16, max_fall);

    return 0;
}

static RK_S32 mpp_av1_metadata_hdr_mdcv(AV1Context *ctx, BitReadCtx_t *gb,
                                        AV1RawMetadataHDRMDCV *current)
{
    RK_S32 err, i;
    (void)ctx;
    for (i = 0; i < 3; i++) {
        fbs(16, primary_chromaticity_x[i], 1, i);
        fbs(16, primary_chromaticity_y[i], 1, i);
    }

    fb(16, white_point_chromaticity_x);
    fb(16, white_point_chromaticity_y);

    fc(32, luminance_max, 1, MAX_UINT_BITS(32));
    // luminance_min must be lower than luminance_max. Convert luminance_max from
    // 24.8 fixed point to 18.14 fixed point in order to compare them.
    fc(32, luminance_min, 0, MPP_MIN(((RK_U64)current->luminance_max << 6) - 1,
                                     MAX_UINT_BITS(32)));

    return 0;
}

static RK_S32 mpp_av1_scalability_structure(AV1Context *ctx, BitReadCtx_t *gb,
                                            AV1RawMetadataScalability *current)
{
    const AV1RawSequenceHeader *seq;
    RK_S32 err, i, j;

    if (!ctx->sequence_header) {
        mpp_err_f("No sequence header available: "
                  "unable to parse scalability metadata.\n");
        return MPP_ERR_UNKNOW;
    }
    seq = ctx->sequence_header;

    fb(2, spatial_layers_cnt_minus_1);
    flag(spatial_layer_dimensions_present_flag);
    flag(spatial_layer_description_present_flag);
    flag(temporal_group_description_present_flag);
    fc(3, scalability_structure_reserved_3bits, 0, 0);
    if (current->spatial_layer_dimensions_present_flag) {
        for (i = 0; i <= current->spatial_layers_cnt_minus_1; i++) {
            fcs(16, spatial_layer_max_width[i],
                0, seq->max_frame_width_minus_1 + 1, 1, i);
            fcs(16, spatial_layer_max_height[i],
                0, seq->max_frame_height_minus_1 + 1, 1, i);
        }
    }
    if (current->spatial_layer_description_present_flag) {
        for (i = 0; i <= current->spatial_layers_cnt_minus_1; i++)
            fbs(8, spatial_layer_ref_id[i], 1, i);
    }
    if (current->temporal_group_description_present_flag) {
        fb(8, temporal_group_size);
        for (i = 0; i < current->temporal_group_size; i++) {
            fbs(3, temporal_group_temporal_id[i], 1, i);
            flags(temporal_group_temporal_switching_up_point_flag[i], 1, i);
            flags(temporal_group_spatial_switching_up_point_flag[i], 1, i);
            fbs(3, temporal_group_ref_cnt[i], 1, i);
            for (j = 0; j < current->temporal_group_ref_cnt[i]; j++) {
                fbs(8, temporal_group_ref_pic_diff[i][j], 2, i, j);
            }
        }
    }

    return 0;
}

static RK_S32 mpp_av1_metadata_scalability(AV1Context *ctx, BitReadCtx_t *gb,
                                           AV1RawMetadataScalability *current)
{
    RK_S32 err;

    fb(8, scalability_mode_idc);

    if (current->scalability_mode_idc == AV1_SCALABILITY_SS)
        CHECK(mpp_av1_scalability_structure(ctx, gb, current));

    return 0;
}

static RK_S32 mpp_av1_metadata_itut_t35(AV1Context *ctx, BitReadCtx_t *gb,
                                        AV1RawMetadataITUTT35 *current)
{
    RK_S32 err;
    size_t i;
    (void)ctx;

    fb(8, itu_t_t35_country_code);
    if (current->itu_t_t35_country_code == 0xff)
        fb(8, itu_t_t35_country_code_extension_byte);

    current->payload_size = mpp_av1_get_payload_bytes_left(gb);

    current->payload = mpp_malloc(RK_U8, current->payload_size);
    if (!current->payload)
        return MPP_ERR_NOMEM;

    for (i = 0; i < current->payload_size; i++)
        xf(8, itu_t_t35_payload_bytes[i], current->payload[i],
           0x00, 0xff, 1, i);

    return 0;
}

static RK_S32 mpp_av1_metadata_timecode(AV1Context *ctx, BitReadCtx_t *gb,
                                        AV1RawMetadataTimecode *current)
{
    RK_S32 err;
    (void)ctx;

    fb(5, counting_type);
    flag(full_timestamp_flag);
    flag(discontinuity_flag);
    flag(cnt_dropped_flag);
    fb(9, n_frames);

    if (current->full_timestamp_flag) {
        fc(6, seconds_value, 0, 59);
        fc(6, minutes_value, 0, 59);
        fc(5, hours_value,   0, 23);
    } else {
        flag(seconds_flag);
        if (current->seconds_flag) {
            fc(6, seconds_value, 0, 59);
            flag(minutes_flag);
            if (current->minutes_flag) {
                fc(6, minutes_value, 0, 59);
                flag(hours_flag);
                if (current->hours_flag)
                    fc(5, hours_value, 0, 23);
            }
        }
    }

    fb(5, time_offset_length);
    if (current->time_offset_length > 0)
        fb(current->time_offset_length, time_offset_value);
    else
        infer(time_offset_length, 0);

    return 0;
}

static RK_S32 mpp_av1_metadata_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                   AV1RawMetadata *current)
{
    RK_S32 err;

    leb128(metadata_type);

    switch (current->metadata_type) {
    case AV1_METADATA_TYPE_HDR_CLL:
        CHECK(mpp_av1_metadata_hdr_cll(ctx, gb, &current->metadata.hdr_cll));
        break;
    case AV1_METADATA_TYPE_HDR_MDCV:
        CHECK(mpp_av1_metadata_hdr_mdcv(ctx, gb, &current->metadata.hdr_mdcv));
        break;
    case AV1_METADATA_TYPE_SCALABILITY:
        CHECK(mpp_av1_metadata_scalability(ctx, gb, &current->metadata.scalability));
        break;
    case AV1_METADATA_TYPE_ITUT_T35:
        CHECK(mpp_av1_metadata_itut_t35(ctx, gb, &current->metadata.itut_t35));
        break;
    case AV1_METADATA_TYPE_TIMECODE:
        CHECK(mpp_av1_metadata_timecode(ctx, gb, &current->metadata.timecode));
        break;
    default:
        // Unknown metadata type.
        return MPP_ERR_UNKNOW;
    }

    return 0;
}

static RK_S32 mpp_av1_padding_obu(AV1Context *ctx, BitReadCtx_t *gb,
                                  AV1RawPadding *current)
{
    RK_S32 err;
    RK_U32 i;
    (void)ctx;
    current->payload_size = mpp_av1_get_payload_bytes_left(gb);

    current->payload  = mpp_malloc(RK_U8, current->payload_size);
    if (!current->payload )
        return MPP_ERR_NOMEM;

    for (i = 0; i < current->payload_size; i++)
        xf(8, obu_padding_byte[i], current->payload[i], 0x00, 0xff, 1, i);

    return 0;
}



static MPP_RET mpp_insert_unit(Av1UnitFragment *frag, RK_S32 position)
{
    Av1ObuUnit *units;

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

    if (units != frag->units) {
        mpp_free(frag->units);
        frag->units = units;
    }

    ++frag->nb_units;

    return MPP_OK;
}

static MPP_RET mpp_insert_unit_data(Av1UnitFragment *frag,
                                    RK_S32 position,
                                    Av1UnitType type,
                                    RK_U8 *data, size_t data_size)
{
    Av1ObuUnit *unit;
    MPP_RET ret;

    if (position == -1)
        position = frag->nb_units;

    mpp_assert(position >= 0 && position <= frag->nb_units);
    ret = mpp_insert_unit(frag, position);
    if (ret < 0) {
        return ret;
    }

    unit = &frag->units[position];
    unit->type      = type;
    unit->data      = data;
    unit->data_size = data_size;

    return MPP_OK;
}

RK_S32 mpp_av1_split_fragment(AV1Context *ctx, Av1UnitFragment *frag, RK_S32 header_flag)
{
    BitReadCtx_t gbc;
    RK_U8 *data;
    size_t size;
    RK_U64 obu_length;
    RK_S32 pos, err;

    data = frag->data;
    size = frag->data_size;

    if (INT_MAX / 8 < size) {
        mpp_err( "Invalid fragment: "
                 "too large (%d bytes).\n", size);
        err = MPP_NOK;
        goto fail;
    }

    if (header_flag && size && data[0] & 0x80) {
        // first bit is nonzero, the extradata does not consist purely of
        // OBUs. Expect MP4/Matroska AV1CodecConfigurationRecord
        RK_S32 config_record_version = data[0] & 0x7f;

        if (config_record_version != 1) {
            mpp_err(
                "Unknown version %d of AV1CodecConfigurationRecord "
                "found!\n",
                config_record_version);
            err = MPP_NOK;
            goto fail;
        }

        if (size <= 4) {
            if (size < 4) {
                av1d_dbg(AV1D_DBG_STRMIN,
                         "Undersized AV1CodecConfigurationRecord v%d found!\n",
                         config_record_version);
                err = MPP_NOK;
                goto fail;
            }

            goto success;
        }

        // In AV1CodecConfigurationRecord v1, actual OBUs start after
        // four bytes. Thus set the offset as required for properly
        // parsing them.
        data += 4;
        size -= 4;
    }

    while (size > 0) {
        AV1RawOBUHeader header;
        RK_U64 obu_size = 0;

        mpp_set_bitread_ctx(&gbc, data, size);

        err = mpp_av1_read_obu_header(ctx, &gbc, &header);
        if (err < 0)
            goto fail;

        if (header.obu_has_size_field) {
            if (mpp_get_bits_left(&gbc) < 8) {
                mpp_err( "Invalid OBU: fragment "
                         "too short (%d bytes).\n", size);
                err = MPP_NOK;
                goto fail;
            }
            err = mpp_av1_read_leb128(&gbc, &obu_size);
            if (err < 0)
                goto fail;
        } else
            obu_size = size - 1 - header.obu_extension_flag;

        pos = mpp_get_bits_count(&gbc);

        mpp_assert(pos % 8 == 0 && pos / 8 <= (RK_S32)size);

        obu_length = pos / 8 + obu_size;

        if (size < obu_length) {
            mpp_err( "Invalid OBU length: "
                     "%lld, but only %d bytes remaining in fragment.\n",
                     obu_length, size);
            err = MPP_NOK;
            goto fail;
        }
        err = mpp_insert_unit_data(frag, -1, header.obu_type,
                                   data, obu_length);
        if (err < 0)
            goto fail;

        data += obu_length;
        size -= obu_length;
    }

success:
    err = 0;
fail:
    return err;
}

static RK_S32 mpp_av1_ref_tile_data(Av1ObuUnit *unit,
                                    BitReadCtx_t *gbc,
                                    AV1RawTileData *td)
{
    RK_S32 pos;

    pos = mpp_get_bits_count(gbc);
    if (pos >= (RK_S32)(8 * unit->data_size)) {
        mpp_err( "Bitstream ended before "
                 "any data in tile group (%d bits read).\n", pos);
        return MPP_NOK;
    }
    // Must be byte-aligned at this point.
    mpp_assert(pos % 8 == 0);



    td->data      = unit->data      + pos / 8;
    td->data_size = unit->data_size - pos / 8;

    return 0;
}

static MPP_RET mpp_av1_alloc_unit_content(Av1ObuUnit *unit)
{
    (void)unit;
    MPP_FREE(unit->content);
    unit->content = mpp_calloc(AV1RawOBU, 1);
    if (!unit->content) {
        return MPP_ERR_NOMEM; // drop_obu()
    }
    return MPP_OK;
}

MPP_RET mpp_av1_read_unit(AV1Context *ctx, Av1ObuUnit *unit)
{
    AV1RawOBU *obu;
    BitReadCtx_t gbc;
    RK_S32 err = 0, start_pos, end_pos, hdr_start_pos;

    err = mpp_av1_alloc_unit_content(unit);

    if (err < 0)
        return err;

    obu = unit->content;

    mpp_set_bitread_ctx(&gbc, unit->data, unit->data_size);

    hdr_start_pos = mpp_get_bits_count(&gbc);

    err = mpp_av1_read_obu_header(ctx, &gbc, &obu->header);
    if (err < 0)
        return err;
    mpp_assert(obu->header.obu_type == unit->type);

    if (obu->header.obu_has_size_field) {
        RK_U64 obu_size = 0;
        err = mpp_av1_read_leb128(&gbc, &obu_size);
        if (err < 0)
            return err;
        obu->obu_size = obu_size;
    } else {
        if (unit->data_size < (RK_U32)(1 + obu->header.obu_extension_flag)) {
            mpp_err( "Invalid OBU length: "
                     "unit too short (%d).\n", unit->data_size);
            return MPP_NOK;
        }
        obu->obu_size = unit->data_size - 1 - obu->header.obu_extension_flag;
    }

    start_pos = mpp_get_bits_count(&gbc);
    ctx->obu_len += ((start_pos - hdr_start_pos) >> 3);
    if (obu->header.obu_extension_flag) {
        if (obu->header.obu_type != AV1_OBU_SEQUENCE_HEADER &&
            obu->header.obu_type != AV1_OBU_TEMPORAL_DELIMITER &&
            ctx->operating_point_idc) {
            RK_S32 in_temporal_layer =
                (ctx->operating_point_idc >>  ctx->temporal_id    ) & 1;
            RK_S32 in_spatial_layer  =
                (ctx->operating_point_idc >> (ctx->spatial_id + 8)) & 1;
            if (!in_temporal_layer || !in_spatial_layer) {
                return MPP_ERR_PROTOL; // drop_obu()
            }
        }
    }

    switch (obu->header.obu_type) {
    case AV1_OBU_SEQUENCE_HEADER: {
        err = mpp_av1_sequence_header_obu(ctx, &gbc,
                                          &obu->obu.sequence_header);
        if (err < 0)
            return err;

        if (ctx->operating_point >= 0) {
            AV1RawSequenceHeader *sequence_header = &obu->obu.sequence_header;

            if (ctx->operating_point > sequence_header->operating_points_cnt_minus_1) {
                mpp_err("Invalid Operating Point %d requested. "
                        "Must not be higher than %u.\n",
                        ctx->operating_point, sequence_header->operating_points_cnt_minus_1);
                return MPP_ERR_PROTOL;
            }
            ctx->operating_point_idc = sequence_header->operating_point_idc[ctx->operating_point];
        }

        ctx->sequence_header = NULL;
        ctx->sequence_header = &obu->obu.sequence_header;
    } break;
    case AV1_OBU_TEMPORAL_DELIMITER: {
        err = mpp_av1_temporal_delimiter_obu(ctx, &gbc);
        if (err < 0)
            return err;
    } break;
    case AV1_OBU_FRAME_HEADER:
    case AV1_OBU_REDUNDANT_FRAME_HEADER: {
        err = mpp_av1_frame_header_obu(ctx, &gbc,
                                       &obu->obu.frame_header,
                                       obu->header.obu_type ==
                                       AV1_OBU_REDUNDANT_FRAME_HEADER,
                                       NULL);
        if (err < 0)
            return err;
    } break;
    case AV1_OBU_TILE_GROUP: {
        err = mpp_av1_tile_group_obu(ctx, &gbc, &obu->obu.tile_group);
        if (err < 0)
            return err;

        err = mpp_av1_ref_tile_data(unit, &gbc,
                                    &obu->obu.tile_group.tile_data);
        if (err < 0)
            return err;
    } break;
    case AV1_OBU_FRAME: {
        err = mpp_av1_frame_obu(ctx, &gbc, &obu->obu.frame,
                                NULL);
        if (err < 0)
            return err;

        err = mpp_av1_ref_tile_data(unit, &gbc,
                                    &obu->obu.frame.tile_group.tile_data);
        if (err < 0)
            return err;
    } break;
    case AV1_OBU_TILE_LIST: {
        err = mpp_av1_tile_list_obu(ctx, &gbc, &obu->obu.tile_list);
        if (err < 0)
            return err;

        err = mpp_av1_ref_tile_data(unit, &gbc,
                                    &obu->obu.tile_list.tile_data);
        if (err < 0)
            return err;
    } break;
    case AV1_OBU_METADATA: {
        err = mpp_av1_metadata_obu(ctx, &gbc, &obu->obu.metadata);
        if (err < 0)
            return err;
    } break;
    case AV1_OBU_PADDING: {
        err = mpp_av1_padding_obu(ctx, &gbc, &obu->obu.padding);
        if (err < 0)
            return err;
    } break;
    default:
        return MPP_ERR_VALUE;
    }

    end_pos = mpp_get_bits_count(&gbc);
    mpp_assert(end_pos <= (RK_S32)(unit->data_size * 8));

    if (obu->obu_size > 0 &&
        obu->header.obu_type != AV1_OBU_TILE_GROUP &&
        obu->header.obu_type != AV1_OBU_TILE_LIST &&
        obu->header.obu_type != AV1_OBU_FRAME) {
        RK_S32 nb_bits = obu->obu_size * 8 + start_pos - end_pos;

        if (nb_bits <= 0)
            return MPP_NOK;

        err = mpp_av1_trailing_bits(ctx, &gbc, nb_bits);
        if (err < 0)
            return err;
    }

    return 0;
}

RK_S32 mpp_av1_read_fragment_content(AV1Context *ctx, Av1UnitFragment *frag)
{
    int err, i, j;
    ctx->obu_len = 0;
    AV1RawOBU *obu;
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
        MPP_FREE(unit->content);
        mpp_assert(unit->data);
        err = mpp_av1_read_unit(ctx, unit);

        if (err == MPP_ERR_VALUE) {
            mpp_err_f("Decomposition unimplemented for unit %d "
                      "(type %d).\n", i, unit->type);
        } else if (err == MPP_ERR_PROTOL) {
            mpp_err_f("Skipping decomposition of"
                      "unit %d (type %d).\n", i, unit->type);
            MPP_FREE(unit->content);
            unit->content = NULL;
        } else if (err < 0) {
            mpp_err_f("Failed to read unit %d (type %d).\n", i, unit->type);
            return err;
        }
        obu = unit->content;
        av1d_dbg(AV1D_DBG_HEADER, "obu->header.obu_type %d, obu->obu_size = %d ctx->frame_tag_size %d",
                 obu->header.obu_type, obu->obu_size, ctx->frame_tag_size);
        if ((obu->header.obu_type != AV1_OBU_FRAME) &&
            (obu->header.obu_type != AV1_OBU_TILE_GROUP)) {
            ctx->obu_len +=  obu->obu_size;
        }
    }
    ctx->frame_tag_size += ctx->obu_len;
    return 0;
}

int mpp_av1_set_context_with_sequence(Av1CodecContext *ctx,
                                      const AV1RawSequenceHeader *seq)
{
    int width = seq->max_frame_width_minus_1 + 1;
    int height = seq->max_frame_height_minus_1 + 1;

    ctx->profile = seq->seq_profile;
    ctx->level = seq->seq_level_idx[0];

    ctx->color_range =
        seq->color_config.color_range ? MPP_FRAME_RANGE_JPEG : MPP_FRAME_RANGE_MPEG;
    ctx->color_primaries = seq->color_config.color_primaries;
    ctx->colorspace = seq->color_config.color_primaries;
    ctx->color_trc = seq->color_config.transfer_characteristics;

    switch (seq->color_config.chroma_sample_position) {
    case AV1_CSP_VERTICAL:
        ctx->chroma_sample_location = MPP_CHROMA_LOC_LEFT;
        break;
    case AV1_CSP_COLOCATED:
        ctx->chroma_sample_location =  MPP_CHROMA_LOC_TOPLEFT;
        break;
    }

    if (ctx->width != width || ctx->height != height) {
        ctx->width = width;
        ctx->height = height;
    }
    return 0;
}

void mpp_av1_fragment_reset(Av1UnitFragment *frag)
{
    int i;

    for (i = 0; i < frag->nb_units; i++) {
        Av1ObuUnit *unit = &frag->units[i];
        MPP_FREE(unit->content);
        unit->data             = NULL;
        unit->data_size        = 0;
    }
    frag->nb_units         = 0;
    frag->data             = NULL;
    frag->data_size        = 0;
}

RK_S32 mpp_av1_assemble_fragment(AV1Context *ctx, Av1UnitFragment *frag)
{
    size_t size, pos;
    RK_S32 i;
    (void)ctx;
    size = 0;
    for (i = 0; i < frag->nb_units; i++)
        size += frag->units[i].data_size;

    frag->data = mpp_malloc(RK_U8, size + BUFFER_PADDING_SIZE);
    if (!frag->data)
        return MPP_ERR_NOMEM;

    memset(frag->data + size, 0, BUFFER_PADDING_SIZE);

    pos = 0;
    for (i = 0; i < frag->nb_units; i++) {
        memcpy(frag->data + pos, frag->units[i].data,
               frag->units[i].data_size);
        pos += frag->units[i].data_size;
    }
    mpp_assert(pos == size);
    frag->data_size = size;

    return 0;
}

void mpp_av1_flush(AV1Context *ctx)
{
    //  ctx->sequencframe_headere_header = NULL;
    //  ctx-> = NULL;

    memset(ctx->ref_s, 0, sizeof(ctx->ref_s));
    ctx->operating_point_idc = 0;
    ctx->seen_frame_header = 0;
    ctx->tile_num = 0;
}

void mpp_av1_close(AV1Context *ctx)
{
    MPP_FREE(ctx->frame_header);
    MPP_FREE(ctx->sequence_header);
    MPP_FREE(ctx->raw_frame_header);
}

void mpp_av1_free_metadata(void *unit, RK_U8 *content)
{
    AV1RawOBU *obu = (AV1RawOBU*)content;
    (void)unit;
    mpp_assert(obu->header.obu_type == AV1_OBU_METADATA);
    MPP_FREE(content);
}
