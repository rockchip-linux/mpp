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

#include "mpp_common.h"

#include "mpp_bitwrite.h"
#include "h264e_debug.h"
#include "h264e_pps.h"

static void write_scaling_list(MppWriteCtx *bit, RK_S32 mode)
{
    switch (mode) {
    case 0 : {
        // flat scaling matrix
        /* scaling_list_present_flag */
        mpp_writer_put_bits(bit, 0, 1);
    } break;
    case 1 : {
        /* scaling_list_present_flag */
        mpp_writer_put_bits(bit, 1, 1);
        /* delta_scale */
        mpp_writer_put_se(bit, -8);
    } break;
    default : {
        mpp_err_f("unsupport scaling list mode %d\n", mode);
    } break;
    }
}

MPP_RET h264e_pps_update(H264ePps *pps, MppEncCfgSet *cfg)
{
    MppEncH264Cfg *codec = &cfg->codec.h264;

    pps->pps_id = 0;
    pps->sps_id = 0;

    pps->entropy_coding_mode = codec->entropy_coding_mode;
    pps->bottom_field_pic_order_in_frame_present = 0;
    pps->num_slice_groups = 1;

    pps->num_ref_idx_l0_default_active = 1;
    pps->num_ref_idx_l1_default_active = 1;

    pps->weighted_pred = 0;
    pps->weighted_bipred_idc = 0;

    pps->pic_init_qp = 26;
    pps->pic_init_qs = pps->pic_init_qp;

    pps->chroma_qp_index_offset = codec->chroma_cb_qp_offset;
    pps->second_chroma_qp_index_offset = codec->chroma_cb_qp_offset;
    pps->deblocking_filter_control = 1;
    pps->constrained_intra_pred = codec->constrained_intra_pred_mode;
    pps->redundant_pic_cnt = 0;

    // if (more_rbsp_data())
    pps->transform_8x8_mode = codec->transform8x8_mode;
    mpp_assert(codec->scaling_list_mode == 0 || codec->scaling_list_mode == 1);
    pps->pic_scaling_matrix_present = codec->scaling_list_mode;
    if (codec->scaling_list_mode) {
        /* NOTE: H.264 current encoder do NOT split detail matrix case */
        pps->use_default_scaling_matrix[H264_INTRA_4x4_Y] = 1;
        pps->use_default_scaling_matrix[H264_INTRA_4x4_U] = 1;
        pps->use_default_scaling_matrix[H264_INTRA_4x4_V] = 1;
        pps->use_default_scaling_matrix[H264_INTER_4x4_Y] = 1;
        pps->use_default_scaling_matrix[H264_INTER_4x4_U] = 1;
        pps->use_default_scaling_matrix[H264_INTER_4x4_V] = 1;
        pps->use_default_scaling_matrix[H264_INTRA_8x8_Y] = 1;
        pps->use_default_scaling_matrix[H264_INTER_8x8_Y] = 1;
    }

    if (codec->profile < H264_PROFILE_HIGH) {
        pps->second_chroma_qp_index_offset_present = 0;
        if (pps->transform_8x8_mode) {
            pps->transform_8x8_mode = 0;
            mpp_log_f("warning: for profile %d transform_8x8_mode should be 0\n",
                      codec->profile);
        }
    } else {
        pps->second_chroma_qp_index_offset_present = 1;
        pps->second_chroma_qp_index_offset = codec->chroma_cr_qp_offset;
    }

    if (codec->profile == H264_PROFILE_BASELINE && pps->entropy_coding_mode) {
        mpp_log_f("warning: for baseline profile entropy_coding_mode should be 0\n");
        pps->entropy_coding_mode = 0;
    }

    return MPP_OK;
}

RK_S32 h264e_pps_to_packet(H264ePps *pps, MppPacket packet, RK_S32 *len)
{
    void *pos = mpp_packet_get_pos(packet);
    void *data = mpp_packet_get_data(packet);
    size_t size = mpp_packet_get_size(packet);
    size_t length = mpp_packet_get_length(packet);
    void *p = pos + length;
    RK_S32 buf_size = (data + size) - (pos + length);
    MppWriteCtx bit_ctx;
    MppWriteCtx *bit = &bit_ctx;
    RK_S32 pps_size = 0;

    mpp_writer_init(bit, p, buf_size);

    /* start_code_prefix 00 00 00 01 */
    mpp_writer_put_raw_bits(bit, 0, 24);
    mpp_writer_put_raw_bits(bit, 1, 8);
    /* forbidden_zero_bit */
    mpp_writer_put_raw_bits(bit, 0, 1);
    /* nal_ref_idc */
    mpp_writer_put_raw_bits(bit, H264_NALU_PRIORITY_HIGHEST, 2);
    /* nal_unit_type */
    mpp_writer_put_raw_bits(bit, H264_NALU_TYPE_PPS, 5);

    /* pic_parameter_set_id */
    mpp_writer_put_ue(bit, pps->pps_id);
    /* seq_parameter_set_id */
    mpp_writer_put_ue(bit, pps->sps_id);
    /* entropy_coding_mode_flag */
    mpp_writer_put_bits(bit, pps->entropy_coding_mode, 1);
    /* bottom_field_pic_order_in_frame_present */
    mpp_writer_put_bits(bit, pps->bottom_field_pic_order_in_frame_present, 1);
    /* num_slice_groups_minus1 */
    mpp_writer_put_ue(bit, pps->num_slice_groups - 1);
    /* num_ref_idx_l0_active_minus1 */
    mpp_writer_put_ue(bit, pps->num_ref_idx_l0_default_active - 1);
    /* num_ref_idx_l1_active_minus1 */
    mpp_writer_put_ue(bit, pps->num_ref_idx_l1_default_active - 1);
    /* weighted_pred_flag */
    mpp_writer_put_bits(bit, pps->weighted_pred, 1);
    /* weighted_bipred_idc */
    mpp_writer_put_bits(bit, pps->weighted_bipred_idc, 2);
    /* pic_init_qp_minus26 */
    mpp_writer_put_se(bit, pps->pic_init_qp - 26);
    /* pic_init_qs_minus26 */
    mpp_writer_put_se(bit, pps->pic_init_qs - 26);
    /* chroma_qp_index_offset */
    mpp_writer_put_se(bit, pps->chroma_qp_index_offset);
    /* deblocking_filter_control_present_flag */
    mpp_writer_put_bits(bit, pps->deblocking_filter_control, 1);
    /* constrained_intra_pred_flag */
    mpp_writer_put_bits(bit, pps->constrained_intra_pred, 1);
    /* redundant_pic_cnt_present_flag */
    mpp_writer_put_bits(bit, pps->redundant_pic_cnt, 1);

    if (pps->transform_8x8_mode ||
        pps->second_chroma_qp_index_offset_present ||
        pps->pic_scaling_matrix_present) {
        /* transform_8x8_mode_flag */
        mpp_writer_put_bits(bit, pps->transform_8x8_mode, 1);

        /* pic_scaling_matrix_present_flag */
        mpp_writer_put_bits(bit, pps->pic_scaling_matrix_present, 1);
        if (pps->pic_scaling_matrix_present) {
            /* Only support default scaling list */
            /* pic_scaling_list_present_flag[i] */
            RK_S32 count = pps->transform_8x8_mode ? 8 : 6;
            RK_S32 i;

            for (i = 0; i < count; i++)
                write_scaling_list(bit, pps->use_default_scaling_matrix[i]);
        }

        /* second_chroma_qp_index_offset */
        mpp_writer_put_se(bit, pps->second_chroma_qp_index_offset);
    }

    mpp_writer_trailing(bit);

    pps_size = mpp_writer_bytes(bit);
    if (len)
        *len = pps_size;

    mpp_packet_set_length(packet, length + pps_size);

    return pps_size;
}

MPP_RET h264e_pps_dump(H264ePps *pps)
{
    (void) pps;
    return MPP_OK;
}
