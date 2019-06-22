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

/*
 * @file       h265d_sei.c
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#define MODULE_TAG "h265d_sei"

#include "h265d_parser.h"


static RK_S32 decode_nal_sei_decoded_picture_hash(HEVCContext *s)
{
    RK_S32 cIdx, i;
    RK_U8 hash_type;
    //uint16_t picture_crc;
    //RK_U32 picture_checksum;
    BitReadCtx_t*gb = &s->HEVClc->gb;
    READ_BITS(gb, 8, &hash_type);

    for (cIdx = 0; cIdx < 3/*((s->sps->chroma_format_idc == 0) ? 1 : 3)*/; cIdx++) {
        if (hash_type == 0) {
            //s->is_md5 = 1;
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
    return  MPP_ERR_STREAM;
}

static RK_S32 decode_nal_sei_frame_packing_arrangement(HEVCContext *s)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_S32 value = 0;

    READ_UE(gb, &value);                  // frame_packing_arrangement_id
    READ_ONEBIT(gb, &value);
    s->sei_frame_packing_present = !value;

    if (s->sei_frame_packing_present) {
        READ_BITS(gb, 7, &s->frame_packing_arrangement_type);
        READ_ONEBIT(gb, &s->quincunx_subsampling);
        READ_BITS(gb, 6, &s->content_interpretation_type);

        // the following skips spatial_flipping_flag frame0_flipped_flag
        // field_views_flag current_frame_is_frame0_flag
        // frame0_self_contained_flag frame1_self_contained_flag
        SKIP_BITS(gb, 6);

        if (!s->quincunx_subsampling && s->frame_packing_arrangement_type != 5)
            SKIP_BITS(gb, 16);  // frame[01]_grid_position_[xy]
        SKIP_BITS(gb, 8);       // frame_packing_arrangement_reserved_byte
        SKIP_BITS(gb, 1);        // frame_packing_arrangement_persistance_flag
    }
    SKIP_BITS(gb, 1);            // upsampled_aspect_ratio_flag
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 decode_pic_timing(HEVCContext *s)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;
    HEVCSPS *sps;

    if (!s->sps_list[s->active_seq_parameter_set_id])
        return MPP_ERR_NOMEM;
    sps = (HEVCSPS*)s->sps_list[s->active_seq_parameter_set_id];
    s->picture_struct = MPP_PICTURE_STRUCTURE_UNKNOWN;
    if (sps->vui.frame_field_info_present_flag) {
        READ_BITS(gb, 4, &s->picture_struct);
        switch (s->picture_struct) {
        case  0 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "(progressive) frame \n"); break;
        case  1 : s->picture_struct = MPP_PICTURE_STRUCTURE_TOP_FIELD;     h265d_dbg(H265D_DBG_SEI, "top field\n"); break;
        case  2 : s->picture_struct = MPP_PICTURE_STRUCTURE_BOTTOM_FIELD;  h265d_dbg(H265D_DBG_SEI, "bottom field\n"); break;
        case  3 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "top field, bottom field, in that order\n"); break;
        case  4 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "bottom field, top field, in that order\n"); break;
        case  5 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "top field, bottom field, top field repeated, in that order\n"); break;
        case  6 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "bottom field, top field, bottom field repeated, in that order\n"); break;
        case  7 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "frame doubling\n"); break;
        case  8 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;         h265d_dbg(H265D_DBG_SEI, "frame tripling\n"); break;
        case  9 : s->picture_struct = MPP_PICTURE_STRUCTURE_TOP_FIELD;     h265d_dbg(H265D_DBG_SEI, "top field paired with previous bottom field in output order\n"); break;
        case 10 : s->picture_struct = MPP_PICTURE_STRUCTURE_BOTTOM_FIELD;  h265d_dbg(H265D_DBG_SEI, "bottom field paired with previous top field in output order\n"); break;
        case 11 : s->picture_struct = MPP_PICTURE_STRUCTURE_TOP_FIELD;     h265d_dbg(H265D_DBG_SEI, "top field paired with next bottom field in output order\n"); break;
        case 12 : s->picture_struct = MPP_PICTURE_STRUCTURE_BOTTOM_FIELD;  h265d_dbg(H265D_DBG_SEI, "bottom field paired with next top field in output order\n"); break;
        }
        SKIP_BITS(gb, 2);                   // source_scan_type
        SKIP_BITS(gb, 1);                   // duplicate_flag
    }
    return 1;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 active_parameter_sets(HEVCContext *s)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_S32 num_sps_ids_minus1;
    RK_S32 i, value;
    RK_U32 active_seq_parameter_set_id;

    SKIP_BITS(gb, 4); // active_video_parameter_set_id
    SKIP_BITS(gb, 1); // self_contained_cvs_flag
    SKIP_BITS(gb, 1); // num_sps_ids_minus1
    READ_UE(gb, &num_sps_ids_minus1); // num_sps_ids_minus1

    READ_UE(gb, &active_seq_parameter_set_id);
    if (active_seq_parameter_set_id >= MAX_SPS_COUNT) {
        mpp_err( "active_parameter_set_id %d invalid\n", active_seq_parameter_set_id);
        return  MPP_ERR_STREAM;
    }
    s->active_seq_parameter_set_id = active_seq_parameter_set_id;

    for (i = 1; i <= num_sps_ids_minus1; i++)
        READ_UE(gb, &value); // active_seq_parameter_set_id[i]

    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 mastering_display_colour_volume(HEVCContext *s)
{
    RK_S32 i = 0;
    RK_U16 value = 0;
    RK_U32 lum = 0;
    BitReadCtx_t *gb = &s->HEVClc->gb;
    for (i = 0; i < 3; i++) {
        READ_BITS(gb, 16, &value);
        s->mastering_display.display_primaries[i][0] = value;
        READ_BITS(gb, 16, &value);
        s->mastering_display.display_primaries[i][1] = value;
    }
    READ_BITS(gb, 16, &value);
    s->mastering_display.white_point[0] = value;
    READ_BITS(gb, 16, &value);
    s->mastering_display.white_point[1] = value;
    mpp_read_longbits(gb, 32, &lum);
    s->mastering_display.max_luminance = lum;
    mpp_read_longbits(gb, 32, &lum);
    s->mastering_display.min_luminance = lum;

    return 0;

__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 content_light_info(HEVCContext *s)
{
    RK_U32 value = 0;
    BitReadCtx_t *gb = &s->HEVClc->gb;
    mpp_read_longbits(gb, 16, &value);
    s->content_light.MaxCLL = value;
    mpp_read_longbits(gb, 16, &value);
    s->content_light.MaxFALL = value;
    return 0;
}

static RK_S32 colour_remapping_info(HEVCContext *s)
{
    RK_U32 i = 0, j = 0;
    RK_U32 value = 0;
    RK_U32 in_bit_depth = 0;
    RK_U32 out_bit_depth = 0;

    BitReadCtx_t *gb = &s->HEVClc->gb;
    READ_UE(gb, &value); //colour_remap ID
    READ_ONEBIT(gb, &value); //colour_remap_cancel_flag
    if (!value) {
        READ_ONEBIT(gb, &value); //colour_remap_persistence_flag
        READ_ONEBIT(gb, &value); //colour_remap_video_signal_info_present_flag
        if (value) {
            READ_ONEBIT(gb, &value); //colour_remap_full_rang_flag
            READ_BITS(gb, 8, &value); //colour_remap_primaries
            READ_BITS(gb, 8, &value); //colour_remap_transfer_function
            READ_BITS(gb, 8, &value); //colour_remap_matries_coefficients
        }

        READ_BITS(gb, 8, &in_bit_depth); //colour_remap_input_bit_depth
        READ_BITS(gb, 8, &out_bit_depth); //colour_remap_bit_depth
        for (i = 0; i < 3; i++) {
            RK_U32 pre_lut_num_val_minus1 = 0;
            RK_U32 in_bit = ((in_bit_depth + 7) >> 3) << 3;
            RK_U32 out_bit = ((out_bit_depth + 7) >> 3) << 3;
            READ_BITS(gb, 8, &pre_lut_num_val_minus1); //pre_lut_num_val_minus1
            if (pre_lut_num_val_minus1 > 0) {
                for (j = 0; j <= pre_lut_num_val_minus1; j++) {
                    READ_BITS(gb, in_bit, &value); //pre_lut_coded_value
                    READ_BITS(gb, out_bit, &value); //pre_lut_target_value
                }
            }
        }
        READ_ONEBIT(gb, &value); //colour_remap_matrix_present_flag
        if (value) {
            READ_BITS(gb, 4, &value); //log2_matrix_denom
            for (i = 0; i < 3; i++) {
                for (j = 0; j < 3; j++)
                    READ_SE(gb, &value); //colour_remap_coeffs
            }
        }
        for (i = 0; i < 3; i++) {
            RK_U32 post_lut_num_val_minus1 = 0;
            RK_U32 in_bit = ((in_bit_depth + 7) >> 3) << 3;
            RK_U32 out_bit = ((out_bit_depth + 7) >> 3) << 3;
            READ_BITS(gb, 8, &post_lut_num_val_minus1); //post_lut_num_val_minus1
            if (post_lut_num_val_minus1 > 0) {
                for (j = 0; j <= post_lut_num_val_minus1; j++) {
                    READ_BITS(gb, in_bit, &value); //post_lut_coded_value
                    READ_BITS(gb, out_bit, &value); //post_lut_target_value
                }
            }
        }

    }

__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 tone_mapping_info(HEVCContext *s)
{
    RK_U32 i = 0;
    RK_U32 value = 0;
    RK_U32 codec_bit_depth = 0;
    RK_U32 target_bit_depth = 0;

    BitReadCtx_t *gb = &s->HEVClc->gb;
    READ_UE(gb, &value); //tone_map ID
    READ_ONEBIT(gb, &value); //tone_map_cancel_flag
    if (!value) {
        RK_U32 tone_map_model_id;
        READ_ONEBIT(gb, &value); //tone_map_persistence_flag
        READ_BITS(gb, 8, &codec_bit_depth); //coded_data_bit_depth
        READ_BITS(gb, 8, &target_bit_depth); //target_bit_depth
        READ_UE(gb, &tone_map_model_id); //tone_map_model_id
        switch (tone_map_model_id) {
        case 0: {
            mpp_read_longbits(gb, 32, &value); //min_value
            mpp_read_longbits(gb, 32, &value); //max_value
            break;
        }
        case 1: {
            mpp_read_longbits(gb, 32, &value); //sigmoid_midpoint
            mpp_read_longbits(gb, 32, &value); //sigmoid_width
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
            READ_BITS(gb, 16, &num_pivots); //num_pivots
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
                mpp_read_longbits(gb, 32, &value); //camera_iso_speed_value

            }
            READ_BITS(gb, 8, &exposure_index_idc);
            if (exposure_index_idc == 255) {
                mpp_read_longbits(gb, 32, &value); //exposure_index_value
            }
            READ_ONEBIT(gb, &value); //exposure_compensation_value_sign_flag
            READ_BITS(gb, 16, &value); //exposure_compensation_value_numerator
            READ_BITS(gb, 16, &value); //exposure_compensation_value_denom_idc
            READ_BITS(gb, 32, &value); //ref_screen_luminance_white
            READ_BITS(gb, 32, &value); //extended_range_white_level
            READ_BITS(gb, 16, &value); //nominal_black_level_code_value
            READ_BITS(gb, 16, &value); //nominal_white_level_code_value
            READ_BITS(gb, 16, &value); //extended_white_level_code_value
            break;
        }
        default:
            break;
        }
    }
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 decode_nal_sei_message(HEVCContext *s)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;

    int payload_type = 0;
    int payload_size = 0;
    int byte = 0xFF;
    h265d_dbg(H265D_DBG_SEI, "Decoding SEI\n");

    while (byte == 0xFF) {
        READ_BITS(gb, 8, &byte);
        payload_type += byte;
    }
    byte = 0xFF;
    while (byte == 0xFF) {
        READ_BITS(gb, 8, &byte);
        payload_size += byte;
    }

    h265d_dbg(H265D_DBG_SEI, "s->nal_unit_type %d payload_type %d payload_size %d\n", s->nal_unit_type, payload_type, payload_size);

    if (s->nal_unit_type == NAL_SEI_PREFIX) {
        if (payload_type == 256 /*&& s->decode_checksum_sei*/) {
            decode_nal_sei_decoded_picture_hash(s);
            return 1;
        } else if (payload_type == 45) {
            decode_nal_sei_frame_packing_arrangement(s);
            return 1;
        } else if (payload_type == 1) {
            int ret = decode_pic_timing(s);
            h265d_dbg(H265D_DBG_SEI, "Skipped PREFIX SEI %d\n", payload_type);
            SKIP_BITS(gb, 8 * payload_size);
            return ret;
        } else if (payload_type == 129) {
            active_parameter_sets(s);
            h265d_dbg(H265D_DBG_SEI, "Skipped PREFIX SEI %d\n", payload_type);
            return 1;
        } else if (payload_type == 137) {
            h265d_dbg(H265D_DBG_SEI, "mastering_display_colour_volume in\n");
            mastering_display_colour_volume(s);
            return 1;
        } else if (payload_type == 144) {
            h265d_dbg(H265D_DBG_SEI, "content_light_info in\n");
            content_light_info(s);
            return 1;
        } else if (payload_type == 143) {
            h265d_dbg(H265D_DBG_SEI, "colour_remapping_info in\n");
            colour_remapping_info(s);
            return 1;
        } else if (payload_type == 23) {
            h265d_dbg(H265D_DBG_SEI, "tone_mapping_info in\n");
            tone_mapping_info(s);
            return 1;
        } else {
            h265d_dbg(H265D_DBG_SEI, "Skipped PREFIX SEI %d\n", payload_type);
            SKIP_BITS(gb, 8 * payload_size);
            return 1;
        }
    } else { /* nal_unit_type == NAL_SEI_SUFFIX */
        if (payload_type == 132 /* && s->decode_checksum_sei */)
            decode_nal_sei_decoded_picture_hash(s);
        else {
            h265d_dbg(H265D_DBG_SEI, "Skipped SUFFIX SEI %d\n", payload_type);
            SKIP_BITS(gb, 8 * payload_size);
        }
        return 1;
    }
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 more_rbsp_data(BitReadCtx_t *gb)
{
    return gb->bytes_left_ > 1 &&  gb->data_[0] != 0x80;
}

RK_S32 mpp_hevc_decode_nal_sei(HEVCContext *s)
{
    RK_S32 ret;

    do {
        ret = decode_nal_sei_message(s);
        if (ret < 0)
            return MPP_ERR_NOMEM;
    } while (more_rbsp_data(&s->HEVClc->gb));
    return 1;
}
