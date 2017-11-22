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

#include <string.h>

#include "mpp_common.h"
#include "mpp_mem.h"

#include "h264_syntax.h"
#include "hal_h264e_rkv_nal.h"

static void h264e_rkv_nals_init(H264eRkvExtraInfo *out)
{
    out->nal_buf = mpp_calloc(RK_U8, H264E_EXTRA_INFO_BUF_SIZE);
    out->nal_num = 0;
}

static void h264e_rkv_nals_deinit(H264eRkvExtraInfo *out)
{
    MPP_FREE(out->nal_buf);

    out->nal_num = 0;
}

static RK_U8 *h264e_rkv_nal_escape_c(RK_U8 *dst, RK_U8 *src, RK_U8 *end)
{
    if (src < end) *dst++ = *src++;
    if (src < end) *dst++ = *src++;
    while (src < end) {
        if (src[0] <= 0x03 && !dst[-2] && !dst[-1])
            *dst++ = 0x03;
        *dst++ = *src++;
    }
    return dst;
}

static void h264e_rkv_nal_encode(RK_U8 *dst, H264eRkvNal *nal)
{
    RK_S32 b_annexb = 1;
    RK_S32 size = 0;
    RK_U8 *src = nal->p_payload;
    RK_U8 *end = nal->p_payload + nal->i_payload;
    RK_U8 *orig_dst = dst;

    if (b_annexb) {
        //if (nal->b_long_startcode)//fix by gsl
        //*dst++ = 0x00;//fix by gsl
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    } else /* save room for size later */
        dst += 4;

    /* nal header */
    *dst++ = (0x00 << 7) | (nal->i_ref_idc << 5) | nal->i_type;

    dst = h264e_rkv_nal_escape_c(dst, src, end);
    size = (RK_S32)((dst - orig_dst) - 4);

    /* Write the size header for mp4/etc */
    if (!b_annexb) {
        /* Size doesn't include the size of the header we're writing now. */
        orig_dst[0] = size >> 24;
        orig_dst[1] = size >> 16;
        orig_dst[2] = size >> 8;
        orig_dst[3] = size >> 0;
    }

    nal->i_payload = size + 4;
    nal->p_payload = orig_dst;
}

static MPP_RET h264e_rkv_encapsulate_nals(H264eRkvExtraInfo *out)
{
    RK_S32 i = 0;
    RK_S32 i_avcintra_class = 0;
    RK_S32 nal_size = 0;
    RK_S32 necessary_size = 0;
    RK_U8 *nal_buffer = out->nal_buf;
    RK_S32 nal_num = out->nal_num;
    H264eRkvNal *nal = out->nal;

    h264e_hal_enter();

    for (i = 0; i < nal_num; i++)
        nal_size += nal[i].i_payload;

    /* Worst-case NAL unit escaping: reallocate the buffer if it's too small. */
    necessary_size = nal_size * 3 / 2 + nal_num * 4 + 4 + 64;
    for (i = 0; i < nal_num; i++)
        necessary_size += nal[i].i_padding;

    for (i = 0; i < nal_num; i++) {
        nal[i].b_long_startcode = !i ||
                                  nal[i].i_type == H264E_NAL_SPS ||
                                  nal[i].i_type == H264E_NAL_PPS ||
                                  i_avcintra_class;
        h264e_rkv_nal_encode(nal_buffer, &nal[i]);
        nal_buffer += nal[i].i_payload;
    }

    h264e_hal_dbg(H264E_DBG_HEADER, "nals total size: %d bytes",
                  nal_buffer - out->nal_buf);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET
h264e_rkv_sei_write(H264eRkvStream *s, RK_U8 *payload,
                    RK_S32 payload_size, RK_S32 payload_type)
{
    RK_S32 i = 0;

    h264e_hal_enter();

    s->count_bit = 0;
    h264e_rkv_stream_realign(s);

    for (i = 0; i <= payload_type - 255; i += 255)
        h264e_rkv_stream_write_with_log(s, 8, 0xff,
                                        "sei_payload_type_ff_byte");
    h264e_rkv_stream_write_with_log(s, 8, payload_type - i,
                                    "sei_last_payload_type_byte");

    for (i = 0; i <= payload_size - 255; i += 255)
        h264e_rkv_stream_write_with_log(s, 8, 0xff,
                                        "sei_payload_size_ff_byte");
    h264e_rkv_stream_write_with_log( s, 8, payload_size - i,
                                     "sei_last_payload_size_byte");

    for (i = 0; i < payload_size; i++)
        h264e_rkv_stream_write_with_log(s, 8, (RK_U32)payload[i],
                                        "sei_payload_data");

    h264e_rkv_stream_rbsp_trailing(s);
    h264e_rkv_stream_flush(s);

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET h264e_rkv_sei_encode(H264eHalContext *ctx, RcSyntax *rc_syn)
{
    H264eRkvExtraInfo *info = (H264eRkvExtraInfo *)ctx->extra_info;
    char *str = (char *)info->sei_buf;
    RK_S32 str_len = 0;

    h264e_sei_pack2str(str + H264E_UUID_LENGTH, ctx, rc_syn);
    str_len = strlen(str) + 1;
    if (str_len > H264E_SEI_BUF_SIZE) {
        h264e_hal_err("SEI actual string length %d exceed malloced size %d",
                      str_len, H264E_SEI_BUF_SIZE);
        return MPP_NOK;
    } else {
        h264e_rkv_sei_write(&info->stream, (RK_U8 *)str, str_len,
                            H264E_SEI_USER_DATA_UNREGISTERED);
    }

    return MPP_OK;
}

static MPP_RET h264e_rkv_sps_write(H264eSps *sps, H264eRkvStream *s)
{
    h264e_hal_enter();

    s->count_bit = 0;
    h264e_rkv_stream_realign(s);
    h264e_rkv_stream_write_with_log(s, 8, sps->i_profile_idc, "profile_idc");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set0, "constraint_set0_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set1, "constraint_set1_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set2, "constraint_set2_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set3, "constraint_set3_flag");

    h264e_rkv_stream_write_with_log(s, 4, 0, "reserved_zero_4bits");

    h264e_rkv_stream_write_with_log(s, 8, sps->i_level_idc, "level_idc");

    h264e_rkv_stream_write_ue_with_log(s, sps->i_id, "seq_parameter_set_id");

    if (sps->i_profile_idc >= H264_PROFILE_HIGH) {
        h264e_rkv_stream_write_ue_with_log(s, sps->i_chroma_format_idc, "chroma_format_idc");
        if (sps->i_chroma_format_idc == H264E_CHROMA_444)
            h264e_rkv_stream_write1_with_log(s, 0, "separate_colour_plane_flag");
        h264e_rkv_stream_write_ue_with_log(s, H264_BIT_DEPTH - 8, "bit_depth_luma_minus8");
        h264e_rkv_stream_write_ue_with_log(s, H264_BIT_DEPTH - 8, "bit_depth_chroma_minus8");
        h264e_rkv_stream_write1_with_log(s, sps->b_qpprime_y_zero_transform_bypass, "qpprime_y_zero_transform_bypass_flag");
        h264e_rkv_stream_write1_with_log(s, 0, "seq_scaling_matrix_present_flag");
    }

    h264e_rkv_stream_write_ue_with_log(s, sps->i_log2_max_frame_num - 4, "log2_max_frame_num_minus4");
    h264e_rkv_stream_write_ue_with_log(s, sps->i_poc_type, "pic_order_cnt_type");
    if (sps->i_poc_type == 0)
        h264e_rkv_stream_write_ue_with_log(s, sps->i_log2_max_poc_lsb - 4, "log2_max_pic_order_cnt_lsb_minus4");
    h264e_rkv_stream_write_ue_with_log(s, sps->i_num_ref_frames, "max_num_ref_frames");
    h264e_rkv_stream_write1_with_log(s, sps->b_gaps_in_frame_num_value_allowed, "gaps_in_frame_num_value_allowed_flag");
    h264e_rkv_stream_write_ue_with_log(s, sps->i_mb_width - 1, "pic_width_in_mbs_minus1");
    h264e_rkv_stream_write_ue_with_log(s, (sps->i_mb_height >> !sps->b_frame_mbs_only) - 1, "pic_height_in_map_units_minus1");
    h264e_rkv_stream_write1_with_log(s, sps->b_frame_mbs_only, "frame_mbs_only_flag");
    if (!sps->b_frame_mbs_only)
        h264e_rkv_stream_write1_with_log(s, sps->b_mb_adaptive_frame_field, "mb_adaptive_frame_field_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_direct8x8_inference, "direct_8x8_inference_flag");

    h264e_rkv_stream_write1_with_log(s, sps->b_crop, "frame_cropping_flag");
    if (sps->b_crop) {
        RK_S32 h_shift = sps->i_chroma_format_idc == H264E_CHROMA_420 || sps->i_chroma_format_idc == H264E_CHROMA_422;
        RK_S32 v_shift = sps->i_chroma_format_idc == H264E_CHROMA_420;
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_left >> h_shift, "frame_crop_left_offset");
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_right >> h_shift, "frame_crop_right_offset");
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_top >> v_shift, "frame_crop_top_offset");
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_bottom >> v_shift, "frame_crop_bottom_offset");
    }

    h264e_rkv_stream_write1_with_log(s, sps->vui.b_vui, "vui_parameters_present_flag");
    if (sps->vui.b_vui) {
        h264e_rkv_stream_write1_with_log(s, sps->vui.b_aspect_ratio_info_present, "aspect_ratio_info_present_flag");
        if (sps->vui.b_aspect_ratio_info_present) {
            RK_S32 i = 0;
            static const struct { RK_U8 w, h, sar; } sar[] = {
                // aspect_ratio_idc = 0 -> unspecified
                { 1, 1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
                { 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
                { 80, 33, 9 }, { 18, 11, 10 }, { 15, 11, 11 }, { 64, 33, 12 },
                { 160, 99, 13 }, { 4, 3, 14 }, { 3, 2, 15 }, { 2, 1, 16 },
                // aspect_ratio_idc = [17..254] -> reserved
                { 0, 0, 255 }
            };
            for (i = 0; sar[i].sar != 255; i++) {
                if (sar[i].w == sps->vui.i_sar_width &&
                    sar[i].h == sps->vui.i_sar_height)
                    break;
            }
            h264e_rkv_stream_write_with_log(s, 8, sar[i].sar, "aspect_ratio_idc");
            if (sar[i].sar == 255) { /* aspect_ratio_idc (extended) */
                h264e_rkv_stream_write_with_log(s, 16, sps->vui.i_sar_width, "sar_width");
                h264e_rkv_stream_write_with_log(s, 16, sps->vui.i_sar_height, "sar_height");
            }
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_overscan_info_present, "overscan_info_present_flag");
        if (sps->vui.b_overscan_info_present)
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_overscan_info, "overscan_appropriate_flag");

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_signal_type_present, "video_signal_type_present_flag");
        if (sps->vui.b_signal_type_present) {
            h264e_rkv_stream_write_with_log(s, 3, sps->vui.i_vidformat, "video_format");
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_fullrange, "video_full_range_flag");
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_color_description_present, "colour_description_present_flag");
            if (sps->vui.b_color_description_present) {
                h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_colorprim, "colour_primaries");
                h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_transfer, "transfer_characteristics");
                h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_colmatrix, "matrix_coefficients");
            }
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_chroma_loc_info_present, "chroma_loc_info_present_flag");
        if (sps->vui.b_chroma_loc_info_present) {
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_chroma_loc_top, "chroma_loc_info_present_flag");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_chroma_loc_bottom, "chroma_sample_loc_type_bottom_field");
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_timing_info_present, "chroma_sample_loc_type_bottom_field");
        if (sps->vui.b_timing_info_present) {
            h264e_rkv_stream_write32(s, sps->vui.i_num_units_in_tick, "num_units_in_tick");
            h264e_rkv_stream_write32(s, sps->vui.i_time_scale, "time_scale");
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_fixed_frame_rate, "fixed_frame_rate_flag");
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_nal_hrd_parameters_present, "time_scale");
        if (sps->vui.b_nal_hrd_parameters_present) {
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_cpb_cnt - 1, "cpb_cnt_minus1");
            h264e_rkv_stream_write_with_log(s, 4, sps->vui.hrd.i_bit_rate_scale, "bit_rate_scale");
            h264e_rkv_stream_write_with_log(s, 4, sps->vui.hrd.i_cpb_size_scale, "cpb_size_scale");

            h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_bit_rate_value - 1, "bit_rate_value_minus1");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_cpb_size_value - 1, "cpb_size_value_minus1");

            h264e_rkv_stream_write1_with_log(s, sps->vui.hrd.b_cbr_hrd, "cbr_flag");

            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_initial_cpb_removal_delay_length - 1, "initial_cpb_removal_delay_length_minus1");
            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_cpb_removal_delay_length - 1, "cpb_removal_delay_length_minus1");
            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_dpb_output_delay_length - 1, "dpb_output_delay_length_minus1");
            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_time_offset_length, "time_offset_length");
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_vcl_hrd_parameters_present, "vcl_hrd_parameters_present_flag");

        if (sps->vui.b_nal_hrd_parameters_present || sps->vui.b_vcl_hrd_parameters_present)
            h264e_rkv_stream_write1_with_log(s, 0, "low_delay_hrd_flag");   /* low_delay_hrd_flag */

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_pic_struct_present, "pic_struct_present_flag");
        h264e_rkv_stream_write1_with_log(s, sps->vui.b_bitstream_restriction, "bitstream_restriction_flag");
        if (sps->vui.b_bitstream_restriction) {
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_motion_vectors_over_pic_boundaries, "motion_vectors_over_pic_boundaries_flag");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_bytes_per_pic_denom, "max_bytes_per_pic_denom");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_bits_per_mb_denom, "max_bits_per_mb_denom");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_log2_max_mv_length_horizontal, "log2_max_mv_length_horizontal");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_log2_max_mv_length_vertical, "log2_max_mv_length_vertical");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_num_reorder_frames, "max_num_reorder_frames");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_dec_frame_buffering, "max_dec_frame_buffering");
        }
    }
    h264e_rkv_stream_rbsp_trailing(s);
    h264e_rkv_stream_flush(s);

    h264e_hal_dbg(H264E_DBG_HEADER, "write pure sps head size: %d bits", s->count_bit);

    h264e_hal_leave();

    return MPP_OK;
}

static void h264e_rkv_scaling_list_write( H264eRkvStream *s,
                                          H264ePps *pps, RK_S32 idx )
{
    RK_S32 k = 0;
    const RK_S32 len = idx < 4 ? 16 : 64;
    const RK_U8 *zigzag = idx < 4 ? h264e_zigzag_scan4[0] : h264e_zigzag_scan8[0];
    const RK_U8 *list = pps->scaling_list[idx];
    const RK_U8 *def_list = (idx == H264E_CQM_4IC) ? pps->scaling_list[H264E_CQM_4IY]
                            : (idx == H264E_CQM_4PC) ? pps->scaling_list[H264E_CQM_4PY]
                            : (idx == H264E_CQM_8IC + 4) ? pps->scaling_list[H264E_CQM_8IY + 4]
                            : (idx == H264E_CQM_8PC + 4) ? pps->scaling_list[H264E_CQM_8PY + 4]
                            : h264e_cqm_jvt[idx];
    if ( !memcmp( list, def_list, len ) )
        h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_present_flag");   // scaling_list_present_flag
    else if ( !memcmp( list, h264e_cqm_jvt[idx], len ) ) {
        h264e_rkv_stream_write1_with_log( s, 1, "scaling_list_present_flag");   // scaling_list_present_flag
        h264e_rkv_stream_write_se_with_log( s, -8, "use_jvt_list"); // use jvt list
    } else {
        RK_S32 run;
        h264e_rkv_stream_write1_with_log( s, 1, "scaling_list_present_flag");   // scaling_list_present_flag

        // try run-length compression of trailing values
        for ( run = len; run > 1; run-- )
            if ( list[zigzag[run - 1]] != list[zigzag[run - 2]] )
                break;
        if ( run < len && len - run < h264e_rkv_stream_size_se( (RK_S8) - list[zigzag[run]] ) )
            run = len;

        for ( k = 0; k < run; k++ )
            h264e_rkv_stream_write_se_with_log( s, (RK_S8)(list[zigzag[k]] - (k > 0 ? list[zigzag[k - 1]] : 8)), "delta_scale"); // delta

        if ( run < len )
            h264e_rkv_stream_write_se_with_log( s, (RK_S8) - list[zigzag[run]], "-scale");
    }
}

static MPP_RET h264e_rkv_pps_write(H264ePps *pps, H264eSps *sps, H264eRkvStream *s)
{
    h264e_hal_enter();

    s->count_bit = 0;
    h264e_rkv_stream_realign( s );

    h264e_rkv_stream_write_ue_with_log( s, pps->i_id, "pic_parameter_set_id");
    h264e_rkv_stream_write_ue_with_log( s, pps->i_sps_id, "seq_parameter_set_id");

    h264e_rkv_stream_write1_with_log( s, pps->b_cabac, "entropy_coding_mode_flag");
    h264e_rkv_stream_write1_with_log( s, pps->b_pic_order, "bottom_field_pic_order_in_frame_present_flag");
    h264e_rkv_stream_write_ue_with_log( s, pps->i_num_slice_groups - 1, "num_slice_groups_minus1");

    h264e_rkv_stream_write_ue_with_log( s, pps->i_num_ref_idx_l0_default_active - 1, "num_ref_idx_l0_default_active_minus1");
    h264e_rkv_stream_write_ue_with_log( s, pps->i_num_ref_idx_l1_default_active - 1, "num_ref_idx_l1_default_active_minus1");
    h264e_rkv_stream_write1_with_log( s, pps->b_weighted_pred, "weighted_pred_flag");
    h264e_rkv_stream_write_with_log( s, 2, pps->i_weighted_bipred_idc, "weighted_bipred_idc");

    h264e_rkv_stream_write_se_with_log( s, pps->i_pic_init_qp - 26 - H264_QP_BD_OFFSET, "pic_init_qp_minus26");
    h264e_rkv_stream_write_se_with_log( s, pps->i_pic_init_qs - 26 - H264_QP_BD_OFFSET, "pic_init_qs_minus26");
    h264e_rkv_stream_write_se_with_log( s, pps->i_chroma_qp_index_offset, "chroma_qp_index_offset");

    h264e_rkv_stream_write1_with_log( s, pps->b_deblocking_filter_control, "deblocking_filter_control_present_flag");
    h264e_rkv_stream_write1_with_log( s, pps->b_constrained_intra_pred, "constrained_intra_pred_flag");
    h264e_rkv_stream_write1_with_log( s, pps->b_redundant_pic_cnt, "redundant_pic_cnt_present_flag");

    if ( pps->b_transform_8x8_mode || pps->b_cqm_preset != H264E_CQM_FLAT ) {
        h264e_rkv_stream_write1_with_log( s, pps->b_transform_8x8_mode, "transform_8x8_mode_flag");
        h264e_rkv_stream_write1_with_log( s, (pps->b_cqm_preset != H264E_CQM_FLAT), "pic_scaling_matrix_present_flag");
        if ( pps->b_cqm_preset != H264E_CQM_FLAT ) {
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4IY);
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4IC );
            h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag"); // Cr = Cb TODO:replaced with real name
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4PY );
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4PC );
            h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag"); // Cr = Cb TODO:replaced with real name
            if ( pps->b_transform_8x8_mode ) {
                if ( sps->i_chroma_format_idc == H264E_CHROMA_444 ) {
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8IY + 4 );
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8IC + 4 );
                    h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag" ); // Cr = Cb TODO:replaced with real name
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8PY + 4 );
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8PC + 4 );
                    h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag" ); // Cr = Cb TODO:replaced with real name
                } else {
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8IY + 4 );
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8PY + 4 );
                }
            }
        }
        h264e_rkv_stream_write_se_with_log( s, pps->i_second_chroma_qp_index_offset, "second_chroma_qp_index_offset");
    }

    h264e_rkv_stream_rbsp_trailing( s );
    h264e_rkv_stream_flush( s );

    h264e_hal_dbg(H264E_DBG_HEADER, "write pure pps size: %d bits", s->count_bit);

    h264e_hal_leave();

    return MPP_OK;
}

void h264e_rkv_nal_start(H264eRkvExtraInfo *out, RK_S32 i_type,
                         RK_S32 i_ref_idc)
{
    H264eRkvStream *s = &out->stream;
    H264eRkvNal *nal = &out->nal[out->nal_num];

    nal->i_ref_idc = i_ref_idc;
    nal->i_type = i_type;
    nal->b_long_startcode = 1;

    nal->i_payload = 0;
    /* NOTE: consistent with stream_init */
    nal->p_payload = &s->buf_plus8[h264e_rkv_stream_get_pos(s) / 8];
    nal->i_padding = 0;
}

void h264e_rkv_nal_end(H264eRkvExtraInfo *out)
{
    H264eRkvNal *nal = &(out->nal[out->nal_num]);
    H264eRkvStream *s = &out->stream;
    /* NOTE: consistent with stream_init */
    RK_U8 *end = &s->buf_plus8[h264e_rkv_stream_get_pos(s) / 8];
    nal->i_payload = (RK_S32)(end - nal->p_payload);
    /*
     * Assembly implementation of nal_escape reads past the end of the input.
     * While undefined padding wouldn't actually affect the output,
     * it makes valgrind unhappy.
     */
    memset(end, 0xff, 64);
    out->nal_num++;
}

MPP_RET h264e_rkv_init_extra_info(H264eRkvExtraInfo *extra_info)
{
    // random ID number generated according to ISO-11578
    // NOTE: any element of h264e_sei_uuid should NOT be 0x00,
    // otherwise the string length of sei_buf will always be the distance between the
    // element 0x00 address and the sei_buf start address.
    static const RK_U8 h264e_sei_uuid[H264E_UUID_LENGTH] = {
        0x63, 0xfc, 0x6a, 0x3c, 0xd8, 0x5c, 0x44, 0x1e,
        0x87, 0xfb, 0x3f, 0xab, 0xec, 0xb3, 0xb6, 0x77
    };

    h264e_rkv_nals_init(extra_info);
    h264e_rkv_stream_init(&extra_info->stream);

    extra_info->sei_buf = mpp_calloc_size(RK_U8, H264E_SEI_BUF_SIZE);
    memcpy(extra_info->sei_buf, h264e_sei_uuid, H264E_UUID_LENGTH);

    return MPP_OK;
}

MPP_RET h264e_rkv_deinit_extra_info(void *extra_info)
{
    H264eRkvExtraInfo *info = (H264eRkvExtraInfo *)extra_info;
    h264e_rkv_stream_deinit(&info->stream);
    h264e_rkv_nals_deinit(info);

    MPP_FREE(info->sei_buf);

    return MPP_OK;
}

MPP_RET h264e_rkv_set_extra_info(H264eHalContext *ctx)
{
    H264eRkvExtraInfo *info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &info->sps;
    H264ePps *pps = &info->pps;

    h264e_hal_enter();

    info->nal_num = 0;
    h264e_rkv_stream_reset(&info->stream);

    h264e_rkv_nal_start(info, H264E_NAL_SPS, H264E_NAL_PRIORITY_HIGHEST);
    h264e_set_sps(ctx, sps);
    h264e_rkv_sps_write(sps, &info->stream);
    h264e_rkv_nal_end(info);

    h264e_rkv_nal_start(info, H264E_NAL_PPS, H264E_NAL_PRIORITY_HIGHEST);
    h264e_set_pps(ctx, pps, sps);
    h264e_rkv_pps_write(pps, sps, &info->stream);
    h264e_rkv_nal_end(info);

    h264e_rkv_encapsulate_nals(info);

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET h264e_rkv_get_extra_info(H264eHalContext *ctx, MppPacket *pkt_out)
{
    RK_S32 k = 0;
    size_t offset = 0;
    MppPacket  pkt = ctx->packeted_param;
    H264eRkvExtraInfo *src = (H264eRkvExtraInfo *)ctx->extra_info;

    for (k = 0; k < src->nal_num; k++) {
        h264e_hal_dbg(H264E_DBG_HEADER, "get extra info nal type %d, size %d bytes",
                      src->nal[k].i_type, src->nal[k].i_payload);
        mpp_packet_write(pkt, offset, src->nal[k].p_payload, src->nal[k].i_payload);
        offset += src->nal[k].i_payload;
    }
    mpp_packet_set_length(pkt, offset);
    *pkt_out = pkt;

    return MPP_OK;
}
