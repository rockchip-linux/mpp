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

static const uint8_t zigzag[64] = {
    0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
    33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
    28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
    23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63
};

static const uint8_t intra_scl[64] = {
    10, 11, 14, 16, 17, 19, 21, 23,
    11, 12, 16, 17, 19, 21, 23, 25,
    14, 16, 17, 19, 21, 23, 25, 27,
    16, 17, 19, 21, 23, 25, 27, 28,
    17, 19, 21, 23, 25, 27, 28, 29,
    19, 21, 23, 25, 27, 28, 29, 30,
    21, 23, 25, 27, 28, 29, 30, 31,
    23, 25, 27, 28, 29, 30, 31, 32,
};

static const uint8_t inter_scl[64] = {
    12, 13, 15, 16, 17, 19, 20, 21,
    13, 14, 16, 17, 19, 20, 21, 22,
    15, 16, 17, 19, 20, 21, 22, 23,
    16, 17, 19, 20, 21, 22, 23, 25,
    17, 19, 20, 21, 22, 23, 25, 27,
    19, 20, 21, 22, 23, 25, 27, 28,
    20, 21, 22, 23, 25, 27, 28, 29,
    21, 22, 23, 25, 27, 28, 29, 30,
};

MPP_RET h264e_pps_update(H264ePps *pps, MppEncCfgSet *cfg)
{
    MppEncH264Cfg *codec = &cfg->h264;

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
        if (pps->pic_scaling_matrix_present) {
            pps->pic_scaling_matrix_present = 0;
            mpp_log_f("warning: for profile %d pic_scaling_matrix_present should be 0\n",
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

RK_S32 h264e_pps_to_packet(H264ePps *pps, MppPacket packet, RK_S32 *offset, RK_S32 *len)
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

        /* TODO: scaling_list_mode */
        mpp_writer_put_bits(bit, pps->pic_scaling_matrix_present != 0, 1);
        if (pps->pic_scaling_matrix_present)
            mpp_writer_put_bits(bit, 0, 6);

        if (1 == pps->pic_scaling_matrix_present)
            mpp_writer_put_bits(bit, 0, 2); /* default scaling list */
        else if (2 == pps->pic_scaling_matrix_present) {
            /* user defined scaling list */
            if (pps->transform_8x8_mode) {
                RK_S32 run = 0;
                RK_S32 len2 = 64;
                RK_S32 j = 0;

                mpp_writer_put_bits(bit, 1, 1);
                for (run = len2; run > 1; run --)
                    if (intra_scl[zigzag[run - 1]] != intra_scl[zigzag[run - 2]])
                        break;
                for (j = 0; j < run; j ++)
                    mpp_writer_put_se(bit, (int8_t)(intra_scl[zigzag[j]] - (j > 0 ? intra_scl[zigzag[j - 1]] : 8)));
                if (run < len2)
                    mpp_writer_put_se(bit, (int8_t) - intra_scl[zigzag[run]]);

                mpp_writer_put_bits(bit, 1, 1);
                for (run = len2; run > 1; run --)
                    if (inter_scl[zigzag[run - 1]] != inter_scl[zigzag[run - 2]])
                        break;
                for (j = 0; j < run; j ++)
                    mpp_writer_put_se(bit, (int8_t)(inter_scl[zigzag[j]] - (j > 0 ? inter_scl[zigzag[j - 1]] : 8)));
                if (run < len2)
                    mpp_writer_put_se(bit, (int8_t) - inter_scl[zigzag[run]]);
            } else
                mpp_writer_put_bits(bit, 0, 2);
        }

        /* second_chroma_qp_index_offset */
        mpp_writer_put_se(bit, pps->second_chroma_qp_index_offset);
    }

    mpp_writer_trailing(bit);

    pps_size = mpp_writer_bytes(bit);
    if (len)
        *len = pps_size;
    if (offset)
        *offset = length;

    mpp_packet_set_length(packet, length + pps_size);

    return pps_size;
}

MPP_RET h264e_pps_dump(H264ePps *pps)
{
    (void) pps;
    return MPP_OK;
}
