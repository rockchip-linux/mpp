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

#ifndef __H264E_SPS_H__
#define __H264E_SPS_H__

#include "mpp_packet.h"
#include "mpp_enc_cfg.h"

#include "h264_syntax.h"

typedef struct H264eVui_t {
    RK_U32      vui_present;

    RK_S32      aspect_ratio_info_present;
    RK_S32      aspect_ratio_idc;
    RK_S32      sar_width;
    RK_S32      sar_height;

    RK_S32      overscan_info_present;
    RK_S32      overscan_appropriate_flag;

    RK_S32      signal_type_present;
    RK_S32      vidformat;
    RK_S32      fullrange;
    RK_S32      color_description_present;
    RK_S32      colorprim;
    RK_S32      colortrc;
    RK_S32      colmatrix;

    RK_S32      chroma_loc_info_present;
    RK_S32      chroma_loc_top;
    RK_S32      chroma_loc_bottom;

    RK_S32      timing_info_present;
    RK_U32      num_units_in_tick;
    RK_U32      time_scale;
    RK_S32      fixed_frame_rate;

    RK_S32      nal_hrd_parameters_present;
    RK_S32      vcl_hrd_parameters_present;

    struct {
        RK_S32  cpb_cnt;
        RK_S32  bit_rate_scale;
        RK_S32  cpb_size_scale;
        RK_S32  bit_rate_value;
        RK_S32  cpb_size_value;
        RK_S32  bit_rate_unscaled;
        RK_S32  cpb_size_unscaled;
        RK_S32  cbr_hrd;

        RK_S32  initial_cpb_removal_delay_length;
        RK_S32  cpb_removal_delay_length;
        RK_S32  dpb_output_delay_length;
        RK_S32  time_offset_length;
    } hrd;

    RK_S32      pic_struct_present;
    RK_S32      bitstream_restriction;
    RK_S32      motion_vectors_over_pic_boundaries;
    RK_S32      max_bytes_per_pic_denom;
    RK_S32      max_bits_per_mb_denom;
    RK_S32      log2_max_mv_length_horizontal;
    RK_S32      log2_max_mv_length_vertical;
    RK_S32      num_reorder_frames;
    RK_S32      max_dec_frame_buffering;
} H264eVui;

typedef struct H264eSps_t {
    RK_S32      profile_idc;

    RK_S32      constraint_set0;
    RK_S32      constraint_set1;
    RK_S32      constraint_set2;
    RK_S32      constraint_set3;
    RK_S32      constraint_set4;
    RK_S32      constraint_set5;

    RK_S32      level_idc;
    RK_S32      sps_id;

    RK_S32      chroma_format_idc;
    /* scaling list is in pps */

    RK_S32      log2_max_frame_num_minus4;
    RK_S32      pic_order_cnt_type;
    /* poc type 0 */
    RK_S32      log2_max_poc_lsb_minus4;

    RK_S32      num_ref_frames;
    RK_S32      gaps_in_frame_num_value_allowed;
    RK_S32      pic_width_in_mbs;
    RK_S32      pic_height_in_mbs;

    // alway frame encoding
    RK_S32      frame_mbs_only;
    RK_S32      direct8x8_inference;

    RK_S32      cropping;
    struct {
        RK_S32  left;
        RK_S32  right;
        RK_S32  top;
        RK_S32  bottom;
    } crop;

    H264eVui    vui;
} H264eSps;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264e_sps_update(H264eSps *sps, MppEncCfgSet *cfg);
MPP_RET h264e_sps_to_packet(H264eSps *sps, MppPacket packet, RK_S32 *len);
MPP_RET h264e_sps_dump(H264eSps *sps);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_SPS_H__ */
