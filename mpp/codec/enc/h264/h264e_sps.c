/*
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

#define MODULE_TAG "h264e_sps"

#include <string.h>

#include "mpp_common.h"

#include "mpp_enc_ref.h"
#include "mpp_bitwrite.h"
#include "h264e_debug.h"
#include "h264e_sps.h"

typedef struct H264eLevelInfo_t {
    H264Level       level;
    RK_S32          max_MBPS;       /* Max macroblock rate per second */
    RK_S32          max_MBs;        /* Max frame size */
    RK_S32          max_DpbMBs;     /* Max decoded picture buffer size */
    RK_S32          max_BR;         /* Max video bitrate */
    const char      *name;
} H264eLevelInfo;

H264eLevelInfo level_infos[] = {
    /*  level             max_MBs  max_MBPS  max_DpbMBs  max_BR  name  */
    {   H264_LEVEL_1_0,     1485,       99,        396,      64, "1"   },
    {   H264_LEVEL_1_b,     1485,       99,        396,     128, "1.b" },
    {   H264_LEVEL_1_1,     3000,      396,        900,     192, "1.1" },
    {   H264_LEVEL_1_2,     6000,      396,       2376,     384, "1.2" },
    {   H264_LEVEL_1_3,    11880,      396,       2376,     768, "1.3" },
    {   H264_LEVEL_2_0,    11880,      396,       2376,    2000, "2"   },
    {   H264_LEVEL_2_1,    19800,      792,       4752,    4000, "2.1" },
    {   H264_LEVEL_2_2,    20250,     1620,       8100,    4000, "2.2" },
    {   H264_LEVEL_3_0,    40500,     1620,       8100,   10000, "3"   },
    {   H264_LEVEL_3_1,   108000,     3600,      18000,   14000, "3.1" },
    {   H264_LEVEL_3_2,   216000,     5120,      20480,   20000, "3.2" },
    {   H264_LEVEL_4_0,   245760,     8192,      32768,   20000, "4"   },
    {   H264_LEVEL_4_1,   245760,     8192,      32768,   50000, "4.1" },
    {   H264_LEVEL_4_2,   522240,     8704,      34816,   50000, "4.2" },
    {   H264_LEVEL_5_0,   589824,    22080,     110400,  135000, "5"   },
    {   H264_LEVEL_5_1,   983040,    36864,     184320,  240000, "5.1" },
    {   H264_LEVEL_5_2,  2073600,    36864,     184320,  240000, "5.2" },
    {   H264_LEVEL_6_0,  4177920,   139264,     696320,  240000, "6"   },
    {   H264_LEVEL_6_1,  8355840,   139264,     696320,  480000, "6.1" },
    {   H264_LEVEL_6_2, 16711680,   139264,     696320,  800000, "6.2" },
};

MPP_RET h264e_sps_update(H264eSps *sps, MppEncCfgSet *cfg)
{
    H264eVui *vui = &sps->vui;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncH264Cfg *h264 = &cfg->codec.h264;
    MppEncRefCfg ref = cfg->ref_cfg;
    MppEncCpbInfo *info = mpp_enc_ref_cfg_get_cpb_info(ref);
    RK_S32 gop = rc->gop;
    RK_S32 width = prep->width;
    RK_S32 height = prep->height;
    RK_S32 aligned_w = MPP_ALIGN(width, 16);
    RK_S32 aligned_h = MPP_ALIGN(height, 16);
    RK_S32 crop_right = MPP_ALIGN(width, 16) - width;
    RK_S32 crop_bottom = MPP_ALIGN(height, 16) - height;
    /* default 720p */
    H264Level level_idc = h264->level;

    // default sps
    // profile baseline
    sps->profile_idc = h264->profile;
    sps->constraint_set0 = 1;
    sps->constraint_set1 = 1;
    sps->constraint_set2 = 0;
    sps->constraint_set3 = 0;
    sps->constraint_set4 = 0;
    sps->constraint_set5 = 0;

    // level_idc is connected with frame size
    {
        RK_S32 mbs = (aligned_w * aligned_h) >> 8;
        RK_S32 i;
        RK_S32 min_level = 10;

        for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(level_infos); i++) {
            if (level_infos[i].max_MBs >= mbs) {
                min_level = level_infos[i].level;

                if (min_level > (RK_S32)level_idc &&
                    min_level != H264_LEVEL_1_b) {
                    level_idc = min_level;
                    mpp_log("set level to %s\n", level_infos[i].name);
                }

                break;
            }
        }
    }
    sps->level_idc = level_idc;

    sps->sps_id = 0;
    sps->chroma_format_idc = H264_CHROMA_420;

    // set max frame number and poc lsb according to gop size
    sps->pic_order_cnt_type = h264->poc_type;
    sps->log2_max_poc_lsb_minus4 = h264->log2_max_poc_lsb;
    sps->log2_max_frame_num_minus4 = h264->log2_max_frame_num;

    mpp_assert(gop >= 0);
    if (gop == 0) {
        // only one I then all P frame
        sps->log2_max_frame_num_minus4 = 12;
        sps->log2_max_poc_lsb_minus4 = 12;
    } else if (gop == 1) {
        // all I frame
        sps->log2_max_frame_num_minus4 = 12;
        sps->log2_max_poc_lsb_minus4 = 12;
    } else {
        // normal case
        RK_S32 log2_gop = MPP_MIN(mpp_log2(gop), 16);
        RK_S32 log2_frm_num = (log2_gop <= 4) ? (0) : (log2_gop - 4);
        RK_S32 log2_poc_lsb = (log2_gop <= 3) ? (0) : (log2_gop - 3);

        if (sps->log2_max_frame_num_minus4 < log2_frm_num)
            sps->log2_max_frame_num_minus4 = log2_frm_num;

        if (sps->log2_max_poc_lsb_minus4 < log2_poc_lsb)
            sps->log2_max_poc_lsb_minus4 = log2_poc_lsb;
    }

    // max one reference frame
    sps->num_ref_frames = info->dpb_size;

    sps->gaps_in_frame_num_value_allowed = !h264->gaps_not_allowed;

    // default 720p without cropping
    sps->pic_width_in_mbs = aligned_w >> 4;
    sps->pic_height_in_mbs = aligned_h >> 4;
    sps->frame_mbs_only = 1;

    // baseline disable 8x8
    sps->direct8x8_inference = h264->transform8x8_mode;
    if (crop_right || crop_bottom) {
        sps->cropping = 1;
        sps->crop.left = 0;
        sps->crop.right = crop_right;
        sps->crop.top = 0;
        sps->crop.bottom = crop_bottom;
    } else {
        sps->cropping = 0;
        memset(&sps->crop, 0, sizeof(sps->crop));
    }

    memset(vui, 0, sizeof(*vui));
    vui->vui_present = 1;
    vui->timing_info_present = 1;
    vui->time_scale = rc->fps_out_num * 2;
    vui->num_units_in_tick = rc->fps_out_denorm;
    vui->fixed_frame_rate = !rc->fps_out_flex;
    vui->vidformat = MPP_FRAME_VIDEO_FMT_UNSPECIFIED;

    if (prep->range == MPP_FRAME_RANGE_JPEG) {
        vui->signal_type_present = 1;
        vui->fullrange = 1;
    }

    if ((prep->colorprim <= MPP_FRAME_PRI_JEDEC_P22 &&
         prep->colorprim != MPP_FRAME_PRI_UNSPECIFIED) ||
        (prep->colortrc <= MPP_FRAME_TRC_ARIB_STD_B67 &&
         prep->colortrc != MPP_FRAME_TRC_UNSPECIFIED) ||
        (prep->color <= MPP_FRAME_SPC_ICTCP &&
         prep->color != MPP_FRAME_SPC_UNSPECIFIED)) {
        vui->signal_type_present = 1;
        vui->color_description_present = 1;
        vui->colorprim = prep->colorprim;
        vui->colortrc = prep->colortrc;
        vui->colmatrix = prep->color;
    }

    vui->bitstream_restriction = 1;
    vui->motion_vectors_over_pic_boundaries = 1;
    vui->log2_max_mv_length_horizontal = 16;
    vui->log2_max_mv_length_vertical = 16;
    vui->max_dec_frame_buffering = info->dpb_size;

    return MPP_OK;
}

MPP_RET h264e_sps_to_packet(H264eSps *sps, MppPacket packet, RK_S32 *len)
{
    void *pos = mpp_packet_get_pos(packet);
    void *data = mpp_packet_get_data(packet);
    size_t size = mpp_packet_get_size(packet);
    size_t length = mpp_packet_get_length(packet);
    void *p = pos + length;
    RK_S32 buf_size = (data + size) - (pos + length);
    MppWriteCtx bit_ctx;
    MppWriteCtx *bit = &bit_ctx;
    RK_S32 sps_size = 0;

    mpp_writer_init(bit, p, buf_size);

    /* start_code_prefix 00 00 00 01 */
    mpp_writer_put_raw_bits(bit, 0, 24);
    mpp_writer_put_raw_bits(bit, 1, 8);
    /* forbidden_zero_bit */
    mpp_writer_put_raw_bits(bit, 0, 1);
    /* nal_ref_idc */
    mpp_writer_put_raw_bits(bit, H264_NALU_PRIORITY_HIGHEST, 2);
    /* nal_unit_type */
    mpp_writer_put_raw_bits(bit, H264_NALU_TYPE_SPS, 5);

    /* profile_idc */
    mpp_writer_put_bits(bit, sps->profile_idc, 8);
    /* constraint_set0_flag */
    mpp_writer_put_bits(bit, sps->constraint_set0, 1);
    /* constraint_set1_flag */
    mpp_writer_put_bits(bit, sps->constraint_set1, 1);
    /* constraint_set2_flag */
    mpp_writer_put_bits(bit, sps->constraint_set2, 1);
    /* constraint_set3_flag */
    mpp_writer_put_bits(bit, sps->constraint_set3, 1);
    /* constraint_set4_flag */
    mpp_writer_put_bits(bit, sps->constraint_set4, 1);
    /* constraint_set5_flag */
    mpp_writer_put_bits(bit, sps->constraint_set5, 1);
    /* reserved_zero_2bits */
    mpp_writer_put_bits(bit, 0, 2);

    /* level_idc */
    mpp_writer_put_bits(bit, sps->level_idc, 8);
    /* seq_parameter_set_id */
    mpp_writer_put_ue(bit, sps->sps_id);

    if (sps->profile_idc >= H264_PROFILE_HIGH) {
        /* chroma_format_idc */
        mpp_writer_put_ue(bit, sps->chroma_format_idc);
        /* bit_depth_luma_minus8 */
        mpp_writer_put_ue(bit, 0);
        /* bit_depth_chroma_minus8 */
        mpp_writer_put_ue(bit, 0);
        /* qpprime_y_zero_transform_bypass_flag */
        mpp_writer_put_bits(bit, 0, 1);
        /* seq_scaling_matrix_present_flag */
        mpp_writer_put_bits(bit, 0, 1);
    }

    /* log2_max_frame_num_minus4 */
    mpp_writer_put_ue(bit, sps->log2_max_frame_num_minus4);
    /* pic_order_cnt_type */
    mpp_writer_put_ue(bit, sps->pic_order_cnt_type);

    if (sps->pic_order_cnt_type == 0) {
        /* log2_max_pic_order_cnt_lsb_minus4 */
        mpp_writer_put_ue(bit, sps->log2_max_poc_lsb_minus4);
    }

    /* max_num_ref_frames */
    mpp_writer_put_ue(bit, sps->num_ref_frames);
    /* gaps_in_frame_num_value_allowed_flag */
    mpp_writer_put_bits(bit, sps->gaps_in_frame_num_value_allowed, 1);

    /* pic_width_in_mbs_minus1 */
    mpp_writer_put_ue(bit, sps->pic_width_in_mbs - 1);
    /* pic_height_in_map_units_minus1 */
    mpp_writer_put_ue(bit, sps->pic_height_in_mbs - 1);

    /* frame_mbs_only_flag */
    mpp_writer_put_bits(bit, sps->frame_mbs_only, 1);
    /* direct_8x8_inference_flag */
    mpp_writer_put_bits(bit, sps->direct8x8_inference, 1);
    /* frame_cropping_flag */
    mpp_writer_put_bits(bit, sps->cropping, 1);
    if (sps->cropping) {
        /* frame_crop_left_offset */
        mpp_writer_put_ue(bit, sps->crop.left / 2);
        /* frame_crop_right_offset */
        mpp_writer_put_ue(bit, sps->crop.right / 2);
        /* frame_crop_top_offset */
        mpp_writer_put_ue(bit, sps->crop.top / 2);
        /* frame_crop_bottom_offset */
        mpp_writer_put_ue(bit, sps->crop.bottom / 2);
    }

    /* vui_parameters_present_flag */
    mpp_writer_put_bits(bit, sps->vui.vui_present, 1);
    if (sps->vui.vui_present) {
        H264eVui *vui = &sps->vui;

        /* aspect_ratio_info_present_flag */
        mpp_writer_put_bits(bit, vui->aspect_ratio_info_present, 1);
        if (vui->aspect_ratio_info_present) {
            /* aspect_ratio_idc */
            mpp_writer_put_bits(bit, vui->aspect_ratio_idc, 8);
            if (H264_EXTENDED_SAR == vui->aspect_ratio_idc) {
                /* sar_width */
                mpp_writer_put_bits(bit, vui->sar_width, 16);
                /* sar_height */
                mpp_writer_put_bits(bit, vui->sar_height, 16);
            }
        }

        /* overscan_info_present */
        mpp_writer_put_bits(bit, vui->overscan_info_present, 1);
        if (vui->overscan_info_present) {
            /* overscan_appropriate_flag */
            mpp_writer_put_bits(bit, vui->overscan_appropriate_flag, 1);
        }

        /* video_signal_type_present_flag */
        mpp_writer_put_bits(bit, vui->signal_type_present, 1);
        if (vui->signal_type_present) {
            /* video_format */
            mpp_writer_put_bits(bit, vui->vidformat, 3);
            /* video_full_range_flag */
            mpp_writer_put_bits(bit, vui->fullrange, 1);
            /* colour_description_present_flag */
            mpp_writer_put_bits(bit, vui->color_description_present, 1);
            if (vui->color_description_present) {
                /* colour_primaries */
                mpp_writer_put_bits(bit, vui->colorprim, 8);
                /* transfer_characteristics */
                mpp_writer_put_bits(bit, vui->colortrc, 8);
                /* matrix_coefficients */
                mpp_writer_put_bits(bit, vui->colmatrix, 8);
            }
        }

        /* chroma_loc_info_present_flag */
        mpp_writer_put_bits(bit, vui->chroma_loc_info_present, 1);
        if (vui->chroma_loc_info_present) {
            /* chroma_sample_loc_type_top_field */
            mpp_writer_put_ue(bit, vui->chroma_loc_top);
            /* chroma_sample_loc_type_bottom_field */
            mpp_writer_put_ue(bit, vui->chroma_loc_bottom);
        }

        /* timing_info_present_flag */
        mpp_writer_put_bits(bit, vui->timing_info_present, 1);
        if (vui->timing_info_present) {
            /* num_units_in_tick msb */
            mpp_writer_put_bits(bit, vui->num_units_in_tick >> 16, 16);
            /* num_units_in_tick lsb */
            mpp_writer_put_bits(bit, vui->num_units_in_tick & 0xffff, 16);
            /* time_scale msb */
            mpp_writer_put_bits(bit, vui->time_scale >> 16, 16);
            /* time_scale lsb */
            mpp_writer_put_bits(bit, vui->time_scale & 0xffff, 16);
            /* fixed_frame_rate_flag */
            mpp_writer_put_bits(bit, vui->fixed_frame_rate, 1);
        }

        /* nal_hrd_parameters_present_flag */
        mpp_writer_put_bits(bit, vui->nal_hrd_parameters_present, 1);
        /* vcl_hrd_parameters_present_flag */
        mpp_writer_put_bits(bit, vui->vcl_hrd_parameters_present, 1);
        /* pic_struct_present_flag */
        mpp_writer_put_bits(bit, vui->pic_struct_present, 1);

        /* bit_stream_restriction_flag */
        mpp_writer_put_bits(bit, vui->bitstream_restriction, 1);
        if (vui->bitstream_restriction) {
            /* motion_vectors_over_pic_boundaries */
            mpp_writer_put_bits(bit, vui->motion_vectors_over_pic_boundaries, 1);
            /* max_bytes_per_pic_denom */
            mpp_writer_put_ue(bit, vui->max_bytes_per_pic_denom);
            /* max_bits_per_mb_denom */
            mpp_writer_put_ue(bit, vui->max_bits_per_mb_denom);
            /* log2_mv_length_horizontal */
            mpp_writer_put_ue(bit, vui->log2_max_mv_length_horizontal);
            /* log2_mv_length_vertical */
            mpp_writer_put_ue(bit, vui->log2_max_mv_length_vertical);
            /* num_reorder_frames */
            mpp_writer_put_ue(bit, vui->num_reorder_frames);
            /* max_dec_frame_buffering */
            mpp_writer_put_ue(bit, vui->max_dec_frame_buffering);
        }
    }

    mpp_writer_trailing(bit);

    sps_size = mpp_writer_bytes(bit);
    if (len)
        *len = sps_size;

    mpp_packet_set_length(packet, length + sps_size);

    return MPP_OK;
}

MPP_RET h264e_sps_dump(H264eSps *sps)
{
    (void) sps;
    return MPP_OK;
}
