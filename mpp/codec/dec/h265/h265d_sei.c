/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "h265d"

#include "mpp_bitread.h"
#include "rk_hdr_meta_com.h"

#include "h265d_debug.h"
#include "h2645d_sei.h"
#include "h265d_sei.h"

static RK_S32 decode_nal_sei_decoded_picture_hash(BitReadCtx_t *gb)
{
    RK_S32 cIdx, i;
    RK_U8 hash_type;
    READ_BITS(gb, 8, &hash_type);

    for (cIdx = 0; cIdx < 3; cIdx++) {
        if (hash_type == 0) {
            for (i = 0; i < 16; i++)
                SKIP_BITS(gb, 8);
        } else if (hash_type == 1) {
            SKIP_BITS(gb, 16);
        } else if (hash_type == 2) {
            SKIP_BITS(gb, 32);
        }
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 decode_nal_sei_frame_packing_arrangement(BitReadCtx_t *gb)
{
    RK_S32 value = 0;

    READ_UE(gb, &value);
    READ_ONEBIT(gb, &value);

    if (!value) {
        /* frame_packing_arrangement_type */
        READ_BITS(gb, 7, &value);
        /* quincunx_subsampling */
        READ_ONEBIT(gb, &value);
        /* content_interpretation_type */
        READ_BITS(gb, 6, &value);

        SKIP_BITS(gb, 6);

        /* spatiotemporal_idle_flag */
        READ_ONEBIT(gb, &value);
        if (value != 3)
            SKIP_BITS(gb, 16);
        SKIP_BITS(gb, 8);
        SKIP_BITS(gb, 1);
    }
    /* extension_flag */
    SKIP_BITS(gb, 1);
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 decode_pic_timing(H265dPrs *p, BitReadCtx_t *gb)
{
    H265dSps *sps;
    RK_S32 value;

    if (!p->sps_list[p->active_seq_parameter_set_id])
        return MPP_ERR_NOMEM;

    sps = p->sps_list[p->active_seq_parameter_set_id];
    if (sps->vui.frame_field_info_present_flag) {
        /* pic_struct */
        READ_BITS(gb, 4, &value);
        (void)value;
        SKIP_BITS(gb, 2);
        SKIP_BITS(gb, 1);
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 active_parameter_sets(H265dPrs *p, BitReadCtx_t *gb)
{
    RK_S32 num_sps_ids_minus1;
    RK_S32 i, value;
    RK_U32 active_seq_parameter_set_id;

    SKIP_BITS(gb, 4);
    SKIP_BITS(gb, 1);
    SKIP_BITS(gb, 1);
    READ_UE(gb, &num_sps_ids_minus1);

    READ_UE(gb, &active_seq_parameter_set_id);
    if (active_seq_parameter_set_id >= MAX_SPS_COUNT) {
        mpp_loge("sei: active param set id %d invalid\n", active_seq_parameter_set_id);
        return MPP_ERR_STREAM;
    }
    p->active_seq_parameter_set_id = active_seq_parameter_set_id;

    for (i = 1; i <= num_sps_ids_minus1; i++)
        READ_UE(gb, &value);

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 mastering_display_colour_volume(H265dPrs *p, BitReadCtx_t *gb)
{
    RK_U32 value = 0;
    RK_U32 lum = 0;
    RK_S32 i = 0;

    for (i = 0; i < 3; i++) {
        READ_BITS(gb, 16, &value);
        p->mastering_display.display_primaries[i][0] = value;
        READ_BITS(gb, 16, &value);
        p->mastering_display.display_primaries[i][1] = value;
    }
    READ_BITS(gb, 16, &value);
    p->mastering_display.white_point[0] = value;
    READ_BITS(gb, 16, &value);
    p->mastering_display.white_point[1] = value;
    mpp_read_longbits(gb, 32, &lum);
    p->mastering_display.max_luminance = lum;
    mpp_read_longbits(gb, 32, &lum);
    p->mastering_display.min_luminance = lum;

    h265d_dbg_sei("dis_prim [%d %d] [%d %d] [%d %d] white point %d %d luminance %d %d\n",
                  p->mastering_display.display_primaries[0][0],
                  p->mastering_display.display_primaries[0][1],
                  p->mastering_display.display_primaries[1][0],
                  p->mastering_display.display_primaries[1][1],
                  p->mastering_display.display_primaries[2][0],
                  p->mastering_display.display_primaries[2][1],
                  p->mastering_display.white_point[0],
                  p->mastering_display.white_point[1],
                  p->mastering_display.max_luminance,
                  p->mastering_display.min_luminance);

    return 0;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 content_light_info(H265dPrs *p, BitReadCtx_t *gb)
{
    RK_U32 value = 0;

    mpp_read_longbits(gb, 16, &value);
    p->content_light.MaxCLL = value;
    mpp_read_longbits(gb, 16, &value);
    p->content_light.MaxFALL = value;
    return 0;
}

static RK_S32 colour_remapping_info(BitReadCtx_t *gb)
{
    RK_U32 in_bit_depth = 0;
    RK_U32 out_bit_depth = 0;
    RK_U32 value = 0;
    RK_U32 i = 0, j = 0;

    READ_UE(gb, &value);
    READ_ONEBIT(gb, &value);
    if (!value) {
        READ_ONEBIT(gb, &value);
        READ_ONEBIT(gb, &value);
        if (value) {
            READ_ONEBIT(gb, &value);
            READ_BITS(gb, 8, &value);
            READ_BITS(gb, 8, &value);
            READ_BITS(gb, 8, &value);
        }

        READ_BITS(gb, 8, &in_bit_depth);
        READ_BITS(gb, 8, &out_bit_depth);
        for (i = 0; i < 3; i++) {
            RK_U32 pre_lut_num_val_minus1 = 0;
            RK_U32 in_bit = ((in_bit_depth + 7) >> 3) << 3;
            RK_U32 out_bit = ((out_bit_depth + 7) >> 3) << 3;
            READ_BITS(gb, 8, &pre_lut_num_val_minus1);
            if (pre_lut_num_val_minus1 > 0) {
                for (j = 0; j <= pre_lut_num_val_minus1; j++) {
                    READ_BITS(gb, in_bit, &value);
                    READ_BITS(gb, out_bit, &value);
                }
            }
        }
        READ_ONEBIT(gb, &value);
        if (value) {
            READ_BITS(gb, 4, &value);
            for (i = 0; i < 3; i++) {
                for (j = 0; j < 3; j++)
                    READ_SE(gb, &value);
            }
        }
        for (i = 0; i < 3; i++) {
            RK_U32 post_lut_num_val_minus1 = 0;
            RK_U32 in_bit = ((in_bit_depth + 7) >> 3) << 3;
            RK_U32 out_bit = ((out_bit_depth + 7) >> 3) << 3;
            READ_BITS(gb, 8, &post_lut_num_val_minus1);
            if (post_lut_num_val_minus1 > 0) {
                for (j = 0; j <= post_lut_num_val_minus1; j++) {
                    READ_BITS(gb, in_bit, &value);
                    READ_BITS(gb, out_bit, &value);
                }
            }
        }

    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 tone_mapping_info(BitReadCtx_t *gb)
{
    RK_U32 codec_bit_depth = 0;
    RK_U32 target_bit_depth = 0;
    RK_U32 value = 0;
    RK_U32 i = 0;

    READ_UE(gb, &value);
    READ_ONEBIT(gb, &value);
    if (!value) {
        RK_U32 tone_map_model_id;

        READ_ONEBIT(gb, &value);
        READ_BITS(gb, 8, &codec_bit_depth);
        READ_BITS(gb, 8, &target_bit_depth);
        READ_UE(gb, &tone_map_model_id);
        switch (tone_map_model_id) {
        case 0: {
            mpp_read_longbits(gb, 32, &value);
            mpp_read_longbits(gb, 32, &value);
            break;
        }
        case 1: {
            mpp_read_longbits(gb, 32, &value);
            mpp_read_longbits(gb, 32, &value);
            break;
        }
        case 2: {
            RK_U32 in_bit = ((codec_bit_depth + 7) >> 3) << 3;

            for (i = 0; i < (RK_U32)(1 << target_bit_depth); i++) {
                READ_BITS(gb, in_bit, &value);
            }
            break;
        }
        case 3: {
            RK_U32  num_pivots;
            RK_U32 in_bit = ((codec_bit_depth + 7) >> 3) << 3;
            RK_U32 out_bit = ((target_bit_depth + 7) >> 3) << 3;

            READ_BITS(gb, 16, &num_pivots);
            for (i = 0; i < num_pivots; i++) {
                READ_BITS(gb, in_bit, &value);
                READ_BITS(gb, out_bit, &value);
            }
            break;
        }
        case 4: {
            RK_U32 camera_iso_speed_idc;
            RK_U32 exposure_index_idc;

            READ_BITS(gb, 8, &camera_iso_speed_idc);
            if (camera_iso_speed_idc == 255) {
                mpp_read_longbits(gb, 32, &value);

            }
            READ_BITS(gb, 8, &exposure_index_idc);
            if (exposure_index_idc == 255) {
                mpp_read_longbits(gb, 32, &value);
            }
            READ_ONEBIT(gb, &value);
            READ_BITS(gb, 16, &value);
            READ_BITS(gb, 16, &value);
            READ_BITS_LONG(gb, 32, &value);
            READ_BITS_LONG(gb, 32, &value);
            READ_BITS(gb, 16, &value);
            READ_BITS(gb, 16, &value);
            READ_BITS(gb, 16, &value);
            break;
        }
        default:
            break;
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 vivid_display_info(H265dPrs *p, BitReadCtx_t *gb, RK_U32 size)
{
    if (gb)
        h265d_fill_dynamic_meta(p, gb->data_, size, HDRVIVID);
    return 0;
}

static RK_S32 hdr10plus_dynamic_data(H265dPrs *p, BitReadCtx_t *gb, RK_U32 size)
{
    if (gb)
        h265d_fill_dynamic_meta(p, gb->data_, size, HDR10PLUS);
    return 0;
}

static RK_S32 user_data_registered_itu_t_t35(H265dPrs *p, BitReadCtx_t *gb, RK_S32 size)
{
    RK_S32 country_code, provider_code;
    RK_U16 provider_oriented_code;

    if (size < 3)
        return 0;

    READ_BITS(gb, 8, &country_code);
    if (country_code == 0xFF) {
        if (size < 1)
            return 0;

        SKIP_BITS(gb, 8);
    }

    if (country_code != 0xB5 && country_code != 0x26) {
        mpp_log("Unsupported User Data Registered ITU-T T35 SEI message (country_code %d)", country_code);
        return MPP_ERR_STREAM;
    }

    READ_BITS(gb, 16, &provider_code);
    READ_BITS(gb, 16, &provider_oriented_code);
    h265d_dbg_sei("country_code=%d provider_code %d terminal_provider_code %d\n",
                  country_code, provider_code, provider_oriented_code);
    switch (provider_code) {
    case 0x4: {
        vivid_display_info(p, gb, mpp_get_bits_left(gb) >> 3);
    } break;
    case 0x3c: {
        const RK_U16 smpte2094_40_provider_oriented_code = 0x0001;
        const RK_U8 smpte2094_40_application_identifier = 0x04;
        RK_U8 application_identifier;

        READ_BITS(gb, 8, &application_identifier);
        if (provider_oriented_code == smpte2094_40_provider_oriented_code &&
            application_identifier == smpte2094_40_application_identifier)
            hdr10plus_dynamic_data(p, gb, mpp_get_bits_left(gb) >> 3);
    } break;
    default:
        break;
    }

    return 0;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 decode_nal_sei_alternative_transfer(H265dPrs *p, BitReadCtx_t *gb)
{
    HEVCSEIAlternativeTransfer *alternative_transfer = &p->alternative_transfer;
    RK_S32 val;

    READ_BITS(gb, 8, &val);
    alternative_transfer->present = 1;
    alternative_transfer->preferred_transfer_characteristics = val;
    p->is_hdr = 1;
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

MPP_RET decode_recovery_point(BitReadCtx_t *gb, H265dPrs *p)
{
    RK_S32 val = -1;

    READ_SE(gb, &val);
    if (val > 32767 || val < -32767) {
        h265d_dbg_sei("recovery_poc_cnt %d, is out of range");
        return MPP_ERR_STREAM;
    }

    memset(&p->recovery, 0, sizeof(RecoveryPoint));
    p->recovery.valid_flag = 1;
    p->recovery.recovery_frame_cnt = val;

    h265d_dbg_sei("Recovery point: poc_cnt %d", p->recovery.recovery_frame_cnt);
    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

MPP_RET h265d_nal_sei(H265dPrs *p)
{
    BitReadCtx_t payload_bitctx;
    BitReadCtx_t *gb = &p->bit;
    RK_S32 payload_type = 0;
    RK_S32 payload_size = 0;
    RK_S32 byte = 0xFF;
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;

    h265d_dbg_sei("Decoding SEI\n");

    do {
        payload_type = 0;
        payload_size = 0;
        byte = 0xFF;
        while (byte == 0xFF) {
            if (gb->bytes_left_ < 2 || payload_type > INT_MAX - 255) {
                mpp_err("parse payload_type error: byte_left %d payload_type %d\n",
                        gb->bytes_left_, payload_type);
                return MPP_ERR_STREAM;
            }

            READ_BITS(gb, 8, &byte);
            payload_type += byte;
        }
        byte = 0xFF;
        while (byte == 0xFF) {
            if ((RK_S32)gb->bytes_left_ < payload_size + 1) {
                mpp_err("parse payload_size error: byte_left %d payload_size %d\n",
                        gb->bytes_left_, payload_size + 1);
                return MPP_ERR_STREAM;
            }

            READ_BITS(gb, 8, &byte);
            payload_size += byte;
        }

        if ((RK_S32)gb->bytes_left_ < payload_size) {
            mpp_err("parse payload_size error: byte_left %d payload_size %d\n",
                    gb->bytes_left_, payload_size);
            return MPP_ERR_STREAM;
        }

        memset(&payload_bitctx, 0, sizeof(payload_bitctx));
        mpp_set_bitread_ctx(&payload_bitctx, p->bit.data_, payload_size);
        mpp_set_bitread_pseudo_code_type(&payload_bitctx, PSEUDO_CODE_H264_H265_SEI);

        h265d_dbg_sei("s->nal_unit_type %d payload_type %d payload_size %d\n", p->nal_unit_type, payload_type, payload_size);

        if (p->nal_unit_type == NAL_SEI_PREFIX) {
            if (payload_type == 256) {
                ret = decode_nal_sei_decoded_picture_hash(&payload_bitctx);
            } else if (payload_type == 45) {
                ret = decode_nal_sei_frame_packing_arrangement(&payload_bitctx);
            } else if (payload_type == 1) {
                ret = decode_pic_timing(p, &payload_bitctx);
                h265d_dbg_sei("Skipped PREFIX SEI %d\n", payload_type);
            } else if (payload_type == 4) {
                ret = user_data_registered_itu_t_t35(p, &payload_bitctx, payload_size);
            } else if (payload_type == 5) {
                ret = check_encoder_sei_info(&payload_bitctx, payload_size, &p->deny_flag);

                if (p->deny_flag)
                    h265d_dbg_sei("Bitstream is encoded by special encoder.");
            } else if (payload_type == 129) {
                ret = active_parameter_sets(p, &payload_bitctx);
                h265d_dbg_sei("Skipped PREFIX SEI %d\n", payload_type);
            } else if (payload_type == 137) {
                h265d_dbg_sei("mastering_display_colour_volume in\n");
                ret = mastering_display_colour_volume(p, &payload_bitctx);
                p->is_hdr = 1;
            } else if (payload_type == 144) {
                h265d_dbg_sei("content_light_info in\n");
                ret = content_light_info(p, &payload_bitctx);
            } else if (payload_type == 143) {
                h265d_dbg_sei("colour_remapping_info in\n");
                ret = colour_remapping_info(&payload_bitctx);
            } else if (payload_type == 23) {
                h265d_dbg_sei("tone_mapping_info in\n");
                ret = tone_mapping_info(&payload_bitctx);
            } else if (payload_type == 6) {
                h265d_dbg_sei("recovery point in\n");
                p->max_ra = INT_MIN;
                ret = decode_recovery_point(&payload_bitctx, p);
            }  else if (payload_type == 147) {
                h265d_dbg_sei("alternative_transfer in\n");
                ret = decode_nal_sei_alternative_transfer(p, &payload_bitctx);
            } else {
                h265d_dbg_sei("Skipped PREFIX SEI %d\n", payload_type);
            }
        } else {
            if (payload_type == 132)
                ret = decode_nal_sei_decoded_picture_hash(&payload_bitctx);
            else {
                h265d_dbg_sei("Skipped SUFFIX SEI %d\n", payload_type);
            }
        }

        for (i = 0; i < payload_size; i++)
            SKIP_BITS(gb, 8);

        if (ret)
            return ret;
    } while (gb->bytes_left_ > 1 &&  gb->data_[0] != 0x80);

    return ret;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}
