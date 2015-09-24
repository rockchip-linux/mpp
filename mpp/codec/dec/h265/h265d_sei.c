/*
 *
 * Copyright 2010 Rockchip Electronics Co. LTD
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

#include "h265d_parser.h"

#define MODULE_TAG "h265d_sei"


static RK_S32 decode_nal_sei_decoded_picture_hash(HEVCContext *s)
{
    RK_S32 cIdx, i;
    RK_U8 hash_type;
    //uint16_t picture_crc;
    //RK_U32 picture_checksum;
    GetBitCxt_t*gb = &s->HEVClc->gb;
    READ_BITS(gb, 8, &hash_type);

    for (cIdx = 0; cIdx < 3/*((s->sps->chroma_format_idc == 0) ? 1 : 3)*/; cIdx++) {
        if (hash_type == 0) {
            //s->is_md5 = 1;
            for (i = 0; i < 16; i++)
                READ_SKIPBITS(gb, 8);
        } else if (hash_type == 1) {
            READ_SKIPBITS(gb, 16);
        } else if (hash_type == 2) {
            READ_SKIPBITS(gb, 32);
        }
    }
    return 0;
}

static RK_S32 decode_nal_sei_frame_packing_arrangement(HEVCContext *s)
{
    GetBitCxt_t *gb = &s->HEVClc->gb;
    RK_S32 value = 0;

    READ_UE(gb, &value);                  // frame_packing_arrangement_id
    READ_BIT1(gb, &value);
    s->sei_frame_packing_present = !value;

    if (s->sei_frame_packing_present) {
        READ_BITS(gb, 7, &s->frame_packing_arrangement_type);
        READ_BIT1(gb, &s->quincunx_subsampling);
        READ_BITS(gb, 6, &s->content_interpretation_type);

        // the following skips spatial_flipping_flag frame0_flipped_flag
        // field_views_flag current_frame_is_frame0_flag
        // frame0_self_contained_flag frame1_self_contained_flag
        READ_SKIPBITS(gb, 6);

        if (!s->quincunx_subsampling && s->frame_packing_arrangement_type != 5)
            READ_SKIPBITS(gb, 16);  // frame[01]_grid_position_[xy]
        READ_SKIPBITS(gb, 8);       // frame_packing_arrangement_reserved_byte
        READ_SKIPBITS(gb, 1);        // frame_packing_arrangement_persistance_flag
    }
    READ_SKIPBITS(gb, 1);            // upsampled_aspect_ratio_flag
    return 0;
}

static RK_S32 decode_pic_timing(HEVCContext *s)
{
    GetBitCxt_t *gb = &s->HEVClc->gb;
    HEVCSPS *sps;

    if (!s->sps_list[s->active_seq_parameter_set_id])
        return MPP_ERR_NOMEM;
    sps = (HEVCSPS*)s->sps_list[s->active_seq_parameter_set_id];
    s->picture_struct = MPP_PICTURE_STRUCTURE_UNKNOWN;
    if (sps->vui.frame_field_info_present_flag) {
        READ_BITS(gb, 4, &s->picture_struct);
        switch (s->picture_struct) {
        case  0 : s->picture_struct = MPP_PICTURE_STRUCTURE_FRAME;        h265d_dbg(H265D_DBG_SEI, "(progressive) frame \n"); break;
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
        READ_SKIPBITS(gb, 2);                   // source_scan_type
        READ_SKIPBITS(gb, 1);                   // duplicate_flag
    }
    return 1;
}

static RK_S32 active_parameter_sets(HEVCContext *s)
{
    GetBitCxt_t *gb = &s->HEVClc->gb;
    RK_S32 num_sps_ids_minus1;
    RK_S32 i, value;
    RK_U32 active_seq_parameter_set_id;

    READ_SKIPBITS(gb, 4); // active_video_parameter_set_id
    READ_SKIPBITS(gb, 1); // self_contained_cvs_flag
    READ_SKIPBITS(gb, 1); // num_sps_ids_minus1
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
}

static RK_S32 decode_nal_sei_message(HEVCContext *s)
{
    GetBitCxt_t *gb = &s->HEVClc->gb;

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
            READ_SKIPBITS(gb, 8 * payload_size);
            return ret;
        } else if (payload_type == 129) {
            active_parameter_sets(s);
            h265d_dbg(H265D_DBG_SEI, "Skipped PREFIX SEI %d\n", payload_type);
            return 1;
        } else {
            h265d_dbg(H265D_DBG_SEI, "Skipped PREFIX SEI %d\n", payload_type);
            READ_SKIPBITS(gb, 8 * payload_size);
            return 1;
        }
    } else { /* nal_unit_type == NAL_SEI_SUFFIX */
        if (payload_type == 132 /* && s->decode_checksum_sei */)
            decode_nal_sei_decoded_picture_hash(s);
        else {
            h265d_dbg(H265D_DBG_SEI, "Skipped SUFFIX SEI %d\n", payload_type);
            READ_SKIPBITS(gb, 8 * payload_size);
        }
        return 1;
    }
}

static RK_S32 more_rbsp_data(GetBitCxt_t *gb)
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
