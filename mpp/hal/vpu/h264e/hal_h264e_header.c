/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_h264e_com.h"
#include "hal_h264e_vepu.h"
#include "hal_h264e_vpu_tbl.h"
#include "hal_h264e_header.h"

#include "h264e_stream.h"

static void hal_h264e_vpu_write_cabac_table(MppBuffer hw_cabac_tab_buf,
                                            RK_S32 cabac_init_idc)
{
    const RK_S32(*context)[460][2];
    RK_S32 i, j, qp;

    RK_U8 table[H264E_CABAC_TABLE_BUF_SIZE] = {0};

    h264e_hal_enter();

    for (qp = 0; qp < 52; qp++) { /* All QP values */
        for (j = 0; j < 2; j++) { /* Intra/Inter */
            if (j == 0)
                /*lint -e(545) */
                context = &h264_context_init_intra;
            else
                /*lint -e(545) */
                context = &h264_context_init[cabac_init_idc];

            for (i = 0; i < 460; i++) {
                RK_S32 m = (RK_S32)(*context)[i][0];
                RK_S32 n = (RK_S32)(*context)[i][1];

                RK_S32 pre_ctx_state = H264E_HAL_CLIP3(((m * (RK_S32)qp) >> 4)
                                                       + n, 1, 126);

                if (pre_ctx_state <= 63)
                    table[qp * 464 * 2 + j * 464 + i] =
                        (RK_U8)((63 - pre_ctx_state) << 1);
                else
                    table[qp * 464 * 2 + j * 464 + i] =
                        (RK_U8)(((pre_ctx_state - 64) << 1) | 1);
            }
        }
    }
    h264e_swap_endian((RK_U32 *)table, H264E_CABAC_TABLE_BUF_SIZE);
    mpp_buffer_write(hw_cabac_tab_buf, 0, table, H264E_CABAC_TABLE_BUF_SIZE);

    h264e_hal_leave();
}

static MPP_RET h264e_vpu_nal_start(H264eStream * stream,
                                   RK_S32 nalRefIdc,
                                   H264eNalUnitType nalUnitType)
{
    h264e_stream_put_bits(stream, 0, 8, "leadin_zero_8bits");
    h264e_stream_put_bits(stream, 0, 8, "start_code_prefix");
    h264e_stream_put_bits(stream, 0, 8, "start_code_prefix");
    h264e_stream_put_bits(stream, 1, 8, "start_code_prefix");
    h264e_stream_put_bits(stream, 0, 1, "forbidden_zero_bit");
    h264e_stream_put_bits(stream, nalRefIdc, 2, "nal_ref_idc");
    h264e_stream_put_bits(stream, (RK_S32)nalUnitType, 5,
                          "nal_unit_type");
    stream->zero_bytes = 0; /* we start new counter for zero bytes */

    return MPP_OK;
}

static MPP_RET hal_h264e_vpu_write_sps(H264eStream *stream,
                                       H264eSps *sps)
{
    h264e_hal_enter();

    h264e_vpu_nal_start(stream, 1, H264E_NAL_SPS);

    h264e_stream_put_bits_with_detect(stream, sps->i_profile_idc, 8,
                                      "profile_idc"); //FIXED: 77, 42
    h264e_stream_put_bits_with_detect(stream, sps->b_constraint_set0, 1,
                                      "constraint_set0_flag"); //E0
    h264e_stream_put_bits_with_detect(stream, sps->b_constraint_set1, 1,
                                      "constraint_set1_flag");
    h264e_stream_put_bits_with_detect(stream, sps->b_constraint_set2, 1,
                                      "constraint_set2_flag");
    h264e_stream_put_bits_with_detect(stream, sps->b_constraint_set3, 1,
                                      "constraint_set3_flag");

    h264e_stream_put_bits_with_detect(stream, 0, 4,
                                      "reserved_zero_4bits");
    h264e_stream_put_bits_with_detect(stream, sps->i_level_idc, 8,
                                      "level_idc"); //28

    h264e_stream_write_ue(stream, sps->i_id, "seq_parameter_set_id"); //8D

    if (sps->i_profile_idc >= 100) { //High profile
        h264e_stream_write_ue(stream, sps->i_chroma_format_idc,
                              "chroma_format_idc");
        h264e_stream_write_ue(stream, H264_BIT_DEPTH - 8,
                              "bit_depth_luma_minus8");
        h264e_stream_write_ue(stream, H264_BIT_DEPTH - 8,
                              "bit_depth_chroma_minus8");
        h264e_stream_put_bits_with_detect(stream,
                                          sps->b_qpprime_y_zero_transform_bypass,
                                          1, "qpprime_y_zero_transform_bypass_flag");
        h264e_stream_put_bits_with_detect(stream, 0, 1,
                                          "seq_scaling_matrix_present_flag");
    }

    h264e_stream_write_ue(stream, sps->i_log2_max_frame_num - 4,
                          "log2_max_frame_num_minus4");

    h264e_stream_write_ue(stream, sps->i_poc_type, "pic_order_cnt_type"); //68 16

    h264e_stream_write_ue(stream, sps->i_num_ref_frames,
                          "num_ref_frames");

    h264e_stream_put_bits_with_detect(stream,
                                      sps->b_gaps_in_frame_num_value_allowed, 1,
                                      "gaps_in_frame_num_value_allowed_flag");

    h264e_stream_write_ue(stream, sps->i_mb_width - 1,
                          "pic_width_in_mbs_minus1");

    h264e_stream_write_ue(stream, sps->i_mb_height - 1,
                          "pic_height_in_map_units_minus1"); //09 64

    h264e_stream_put_bits_with_detect(stream,
                                      sps->b_frame_mbs_only, 1, "frame_mbs_only_flag");

    h264e_stream_put_bits_with_detect(stream,
                                      sps->b_direct8x8_inference, 1, "direct_8x8_inference_flag");

    h264e_stream_put_bits_with_detect(stream,
                                      sps->b_crop, 1, "frame_cropping_flag");
    if (sps->b_crop) {
        h264e_stream_write_ue(stream, sps->crop.i_left / 2,
                              "frame_crop_left_offset");
        h264e_stream_write_ue(stream, sps->crop.i_right / 2,
                              "frame_crop_right_offset");
        h264e_stream_write_ue(stream, sps->crop.i_top / 2,
                              "frame_crop_top_offset");
        h264e_stream_write_ue(stream, sps->crop.i_bottom / 2,
                              "frame_crop_bottom_offset");
    }

    h264e_stream_put_bits_with_detect(stream, sps->vui.b_vui, 1,
                                      "vui_parameters_present_flag");
    if (sps->vui.b_vui) {
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_aspect_ratio_info_present, 1,
                                          "aspect_ratio_info_present_flag");
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_overscan_info_present, 1,
                                          "overscan_info_present_flag");
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_signal_type_present, 1,
                                          "video_signal_type_present_flag");
        if (sps->vui.b_signal_type_present) {
            h264e_stream_put_bits_with_detect(stream, sps->vui.i_vidformat, 3, "video_format");
            h264e_stream_put_bits_with_detect(stream, sps->vui.b_fullrange, 1, "video_full_range_flag");
            h264e_stream_put_bits_with_detect(stream, sps->vui.b_color_description_present, 1,
                                              "colour_description_present_flag");
            if (sps->vui.b_color_description_present) {
                h264e_stream_put_bits_with_detect(stream, sps->vui.i_colorprim, 8, "colour_primaries");
                h264e_stream_put_bits_with_detect(stream, sps->vui.i_transfer,  8,  "transfer_characteristics");
                h264e_stream_put_bits_with_detect(stream, sps->vui.i_colmatrix, 8,  "matrix_coefficients");
            }
        }
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_chroma_loc_info_present, 1,
                                          "chroma_loc_info_present_flag");

        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_timing_info_present, 1,
                                          "timing_info_present_flag");
        if (sps->vui.b_timing_info_present) {
            h264e_stream_put_bits_with_detect(stream,
                                              sps->vui.i_num_units_in_tick >> 16, 16,
                                              "num_units_in_tick msb");
            h264e_stream_put_bits_with_detect(stream,
                                              sps->vui.i_num_units_in_tick & 0xffff, 16,
                                              "num_units_in_tick lsb");
            h264e_stream_put_bits_with_detect(stream,
                                              sps->vui.i_time_scale >> 16, 16,
                                              "time_scale msb");
            h264e_stream_put_bits_with_detect(stream,
                                              sps->vui.i_time_scale & 0xffff, 16,
                                              "time_scale lsb");
            h264e_stream_put_bits_with_detect(stream,
                                              sps->vui.b_fixed_frame_rate, 1,
                                              "fixed_frame_rate_flag");
        }
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_nal_hrd_parameters_present, 1,
                                          "nal_hrd_parameters_present_flag");
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_vcl_hrd_parameters_present, 1,
                                          "vcl_hrd_parameters_present_flag");
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_pic_struct_present, 1,
                                          "pic_struct_present_flag");
        h264e_stream_put_bits_with_detect(stream,
                                          sps->vui.b_bitstream_restriction, 1,
                                          "bit_stream_restriction_flag");
        if (sps->vui.b_bitstream_restriction) {
            h264e_stream_put_bits_with_detect(stream,
                                              sps->vui.b_motion_vectors_over_pic_boundaries, 1,
                                              "motion_vectors_over_pic_boundaries");
            h264e_stream_write_ue(stream,
                                  sps->vui.i_max_bytes_per_pic_denom,
                                  "max_bytes_per_pic_denom");
            h264e_stream_write_ue(stream,
                                  sps->vui.i_max_bits_per_mb_denom,
                                  "max_bits_per_mb_denom");
            h264e_stream_write_ue(stream,
                                  sps->vui.i_log2_max_mv_length_horizontal,
                                  "log2_mv_length_horizontal");
            h264e_stream_write_ue(stream,
                                  sps->vui.i_log2_max_mv_length_vertical,
                                  "log2_mv_length_vertical");
            h264e_stream_write_ue(stream,
                                  sps->vui.i_num_reorder_frames,
                                  "num_reorder_frames");
            h264e_stream_write_ue(stream,
                                  sps->vui.i_max_dec_frame_buffering,
                                  "max_dec_frame_buffering");
        }
    }

    h264e_stream_trailing_bits(stream);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET hal_h264e_vpu_write_pps(H264eStream *stream,
                                       H264ePps *pps)
{
    h264e_hal_enter();

    h264e_vpu_nal_start(stream, 1, H264E_NAL_PPS);

    h264e_stream_write_ue(stream, pps->i_id, "pic_parameter_set_id");
    h264e_stream_write_ue(stream, pps->i_sps_id, "seq_parameter_set_id");

    h264e_stream_put_bits_with_detect(stream, pps->b_cabac, 1,
                                      "entropy_coding_mode_flag");
    h264e_stream_put_bits_with_detect(stream, pps->b_pic_order, 1,
                                      "pic_order_present_flag");

    h264e_stream_write_ue(stream, pps->i_num_slice_groups - 1,
                          "num_slice_groups_minus1");
    h264e_stream_write_ue(stream, pps->i_num_ref_idx_l0_default_active - 1,
                          "num_ref_idx_l0_active_minus1");
    h264e_stream_write_ue(stream, pps->i_num_ref_idx_l1_default_active - 1,
                          "num_ref_idx_l1_active_minus1");

    h264e_stream_put_bits_with_detect(stream, pps->b_weighted_pred, 1,
                                      "weighted_pred_flag");
    h264e_stream_put_bits_with_detect(stream,
                                      pps->i_weighted_bipred_idc, 2,
                                      "weighted_bipred_idc");

    h264e_stream_write_se(stream, pps->i_pic_init_qp - 26,
                          "pic_init_qp_minus26");
    h264e_stream_write_se(stream, pps->i_pic_init_qs - 26,
                          "pic_init_qs_minus26");
    h264e_stream_write_se(stream, pps->i_chroma_qp_index_offset,
                          "chroma_qp_index_offset");

    h264e_stream_put_bits_with_detect(stream,
                                      pps->b_deblocking_filter_control, 1,
                                      "deblocking_filter_control_present_flag");
    h264e_stream_put_bits_with_detect(stream,
                                      pps->b_constrained_intra_pred, 1,
                                      "constrained_intra_pred_flag");

    h264e_stream_put_bits_with_detect(stream, pps->b_redundant_pic_cnt,
                                      1, "redundant_pic_cnt_present_flag");

    if (pps->b_transform_8x8_mode) {
        h264e_stream_put_bits_with_detect(stream,
                                          pps->b_transform_8x8_mode, 1,
                                          "transform_8x8_mode_flag");
        h264e_stream_put_bits_with_detect(stream,
                                          pps->b_cqm_preset, 1,
                                          "pic_scaling_matrix_present_flag");
        h264e_stream_write_se(stream,
                              pps->i_chroma_qp_index_offset,
                              "chroma_qp_index_offset");
    }

    h264e_stream_trailing_bits(stream);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET hal_h264e_vpu_write_sei(H264eStream *s, RK_U8 *payload,
                                       RK_S32 payload_size, RK_S32 payload_type)
{
    RK_S32 i = 0;

    h264e_hal_enter();

    h264e_vpu_nal_start(s, H264E_NAL_PRIORITY_DISPOSABLE, H264E_NAL_SEI);

    for (i = 0; i <= payload_type - 255; i += 255)
        h264e_stream_put_bits_with_detect(s, 0xff, 8,
                                          "sei_payload_type_ff_byte");
    h264e_stream_put_bits_with_detect(s, payload_type - i, 8,
                                      "sei_last_payload_type_byte");

    for (i = 0; i <= payload_size - 255; i += 255)
        h264e_stream_put_bits_with_detect(s, 0xff, 8,
                                          "sei_payload_size_ff_byte");
    h264e_stream_put_bits_with_detect(s, payload_size - i, 8,
                                      "sei_last_payload_size_byte");

    for (i = 0; i < payload_size; i++)
        h264e_stream_put_bits_with_detect(s, (RK_U32)payload[i], 8,
                                          "sei_payload_data");

    h264e_stream_trailing_bits(s);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_vpu_sei_encode(H264eHalContext *ctx)
{
    H264eVpuExtraInfo *info =
        (H264eVpuExtraInfo *)ctx->extra_info;
    H264eStream *sei_stream = &info->sei_stream;
    char *str = (char *)info->sei_buf;
    RK_S32 str_len = 0;

    h264e_sei_pack2str(str + H264E_UUID_LENGTH, ctx, NULL);

    str_len = strlen(str) + 1;
    if (str_len > H264E_SEI_BUF_SIZE) {
        return MPP_NOK;
    } else {
        hal_h264e_vpu_write_sei(sei_stream, (RK_U8 *)str, str_len,
                                H264E_SEI_USER_DATA_UNREGISTERED);
    }

    return MPP_OK;
}

MPP_RET h264e_vpu_init_extra_info(void *extra_info)
{
    static const RK_U8 h264e_sei_uuid[H264E_UUID_LENGTH] = {
        0x63, 0xfc, 0x6a, 0x3c, 0xd8, 0x5c, 0x44, 0x1e,
        0x87, 0xfb, 0x3f, 0xab, 0xec, 0xb3, 0xb6, 0x77
    };
    H264eVpuExtraInfo *info = (H264eVpuExtraInfo *)extra_info;
    MPP_RET ret = MPP_OK;
    RK_S32 size = 128;
    RK_U8 *p = mpp_calloc_size(RK_U8, size);

    // sps buffer 128Bytes
    ret = h264e_stream_init(&info->sps_stream, p, size);
    if (ret) {
        mpp_err("sps stream sw buf init failed");
        return MPP_NOK;
    }

    // pps buffer 128Bytes
    p = mpp_calloc_size(RK_U8, size);
    ret = h264e_stream_init(&info->pps_stream, p, size);
    if (ret) {
        mpp_err("pps stream sw buf init failed");
        return MPP_NOK;
    }

    // sei buffer 1024Bytes
    size = H264E_SEI_BUF_SIZE;
    p = mpp_calloc_size(RK_U8, size);
    ret = h264e_stream_init(&info->sei_stream, p, size);
    if (ret) {
        mpp_err("sei stream sw buf init failed");
        return MPP_NOK;
    }
    info->sei_buf = mpp_calloc_size(RK_U8, H264E_SEI_BUF_SIZE);
    memcpy(info->sei_buf, h264e_sei_uuid, H264E_UUID_LENGTH);

    return MPP_OK;
}

MPP_RET h264e_vpu_deinit_extra_info(void *extra_info)
{
    H264eVpuExtraInfo *info = (H264eVpuExtraInfo *)extra_info;
    H264eStream *sps_stream = &info->sps_stream;
    H264eStream *pps_stream = &info->pps_stream;
    H264eStream *sei_stream = &info->sei_stream;

    MPP_FREE(sps_stream->buffer);
    MPP_FREE(pps_stream->buffer);
    MPP_FREE(sei_stream->buffer);
    MPP_FREE(info->sei_buf);

    return MPP_OK;
}

MPP_RET h264e_vpu_set_extra_info(H264eHalContext *ctx)
{
    H264eVpuExtraInfo *info =
        (H264eVpuExtraInfo *)ctx->extra_info;
    H264eStream *sps_stream = &info->sps_stream;
    H264eStream *pps_stream = &info->pps_stream;
    H264eSps *sps = &info->sps;
    H264ePps *pps = &info->pps;

    h264e_hal_enter();

    h264e_stream_reset(sps_stream);
    h264e_stream_reset(pps_stream);

    h264e_set_sps(ctx, sps);
    h264e_set_pps(ctx, pps, sps);

    hal_h264e_vpu_write_sps(sps_stream, sps);
    hal_h264e_vpu_write_pps(pps_stream, pps);

    if (ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_SEQ
        || ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_FRAME) {
        info->sei_change_flg |= H264E_SEI_CHG_SPSPPS;
        h264e_vpu_sei_encode(ctx);
    }


    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET h264e_vpu_free_buffers(H264eHalContext *ctx)
{
    MPP_RET ret = MPP_OK;
    RK_S32 k = 0;
    h264e_hal_vpu_buffers *p = (h264e_hal_vpu_buffers *)ctx->buffers;
    h264e_hal_enter();

    if (p->hw_cabac_table_buf) {
        ret = mpp_buffer_put(p->hw_cabac_table_buf);
        if (ret)
            mpp_err("hw_cabac_table_buf put failed ret %d\n", ret);

        p->hw_cabac_table_buf = NULL;
    }

    if (p->hw_nal_size_table_buf) {
        ret = mpp_buffer_put(p->hw_nal_size_table_buf);
        if (ret)
            mpp_err("hw_nal_size_table_buf put failed ret %d\n", ret);

        p->hw_nal_size_table_buf = NULL;
    }

    for (k = 0; k < 2; k++) {
        if (p->hw_rec_buf[k]) {
            ret = mpp_buffer_put(p->hw_rec_buf[k]);
            if (ret)
                mpp_err("hw_rec_buf[%d] put failed ret %d\n", k, ret);

            p->hw_rec_buf[k] = NULL;
        }
    }

    if (p->hw_buf_grp) {
        ret = mpp_buffer_group_put(p->hw_buf_grp);
        if (ret)
            mpp_err("buf group put failed ret %d\n", ret);

        p->hw_buf_grp = NULL;
    }

    h264e_hal_leave();
    return ret;
}

MPP_RET h264e_vpu_allocate_buffers(H264eHalContext *ctx)
{
    MPP_RET ret = MPP_OK;
    h264e_hal_vpu_buffers *buffers = (h264e_hal_vpu_buffers *)ctx->buffers;
    h264e_hal_enter();

    // init parameters
    buffers->cabac_init_idc = 0;
    buffers->align_width = 0;
    buffers->align_height = 0;

    ret = mpp_buffer_group_get_internal(&buffers->hw_buf_grp,
                                        MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("buf group get failed ret %d\n", ret);
        return ret;
    }
    ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_cabac_table_buf,
                         H264E_CABAC_TABLE_BUF_SIZE);
    if (ret) {
        mpp_err("hw_cabac_table_buf get failed\n");
        return ret;
    }
    hal_h264e_vpu_write_cabac_table(buffers->hw_cabac_table_buf,
                                    buffers->cabac_init_idc);

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET h264e_vpu_update_buffers(H264eHalContext *ctx, H264eHwCfg *hw_cfg)
{
    MPP_RET ret = MPP_OK;

    h264e_hal_vpu_buffers *buffers = (h264e_hal_vpu_buffers *)ctx->buffers;
    h264e_hal_enter();

    if (hw_cfg->cabac_init_idc != buffers->cabac_init_idc) {
        hal_h264e_vpu_write_cabac_table(buffers->hw_cabac_table_buf,
                                        hw_cfg->cabac_init_idc);
        buffers->cabac_init_idc = hw_cfg->cabac_init_idc;
    }

    RK_S32 align_width = MPP_ALIGN(hw_cfg->width, 16);
    RK_S32 align_height = MPP_ALIGN(hw_cfg->height, 16);
    if ((align_width != buffers->align_width) ||
        (align_height != buffers->align_height)) {
        RK_S32 k = 0;
        // free original buffer
        if (buffers->hw_nal_size_table_buf) {
            mpp_buffer_put(buffers->hw_nal_size_table_buf);
            buffers->hw_nal_size_table_buf = NULL;
        }
        for (k = 0; k < 2; k++) {
            if (buffers->hw_rec_buf[k]) {
                mpp_buffer_put(buffers->hw_rec_buf[k]);
                buffers->hw_rec_buf[k] = NULL;
            }
        }

        // realloc buffer
        RK_U32 frame_size = align_width * align_height * 3 / 2;
        ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_nal_size_table_buf,
                             (sizeof(RK_U32) * align_height));
        if (ret) {
            mpp_err("hw_nal_size_table_buf get failed\n");
            return ret;
        }
        for (k = 0; k < 2; k++) {
            ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_rec_buf[k],
                                 frame_size + 4096);
            if (ret) {
                mpp_err("hw_rec_buf[%d] get failed\n", k);
                return ret;
            }
        }
        buffers->align_width = align_width;
        buffers->align_height = align_height;
    }

    h264e_hal_leave();
    return MPP_OK;
}
