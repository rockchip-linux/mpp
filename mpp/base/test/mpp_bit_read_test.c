/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_bit_read_test"

#include <stdlib.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_bitread.h"

#define BIT_READ_BUFFER_SIZE        (1024)

typedef enum BitOpsType_e {
    BIT_GET,
    BIT_GET_UE,
    BIT_GET_SE,
    BIT_SKIP,
} BitOpsType;

static RK_U8 bitOpsStr[][10] = {
    "read_bit",
    "read_ue",
    "read_se",
    "skip_bit"
};

typedef struct BitOps_t {
    BitOpsType type;
    RK_S32 len;
    RK_S32 val;
    char syntax[50];
} BitOps;

// AVS2 I-frame data containing "00 00 02"
static RK_U8 test_data_1[] = {
    0x00, 0x00, 0x01, 0xB3, 0x00, 0x00, 0xFF, 0xFF,
    0x80, 0x00, 0x00, 0x02, 0x04, 0x40, 0xCA, 0xB3,
    0xD2, 0x2B, 0x33, 0x4C, 0x91, 0x88, 0xAC, 0xCD,
    0x14, 0xD8, 0xA1, 0x86, 0x14, 0x30, 0x8A, 0x58,
    0xE1, 0x46, 0x8C, 0x1D, 0x80, 0x00, 0x00, 0x01
};

static BitOps bit_ops_1 [] = {
    {BIT_SKIP, 32, 0, "Skip start code of I Pic"},
    {BIT_GET, 32, 65535, "bbv_delay"},
    {BIT_GET, 1, 1, "time_code_flag"},
    {BIT_GET, 24, 0, "time_code"},
    {BIT_GET, 8, 0, "coding_order"},
    {BIT_GET_UE, 0, 3, "picture_output_delay"},
    {BIT_GET, 1, 0, "use RCS in SPS"},
    {BIT_GET, 1, 1, "refered by others"},
    {BIT_GET, 3, 0, "number of reference picture"},
    {BIT_GET, 3, 0, "number of removed picture"},
    {BIT_GET, 1, 1, "marker bit"},
    {BIT_GET, 1, 1, "progressive_frame"},
    {BIT_GET, 1, 0, "top_field_first"},
    {BIT_GET, 1, 0, "repeat_first_field"},
    {BIT_GET, 1, 1, "fixed_picture_qp"},
    {BIT_GET, 7, 43, "picture_qp"},
    {BIT_GET, 1, 0, "loop_filter_disable"},
    {BIT_GET, 1, 0, "loop_filter_parameter_flag"},
    {BIT_GET, 1, 1, "chroma_quant_param_disable"}
};

// AVS2 Sequence header data without "00 00 02"
static RK_U8 test_data_2[] = {
    0x00, 0x00, 0x01, 0xB0, 0x20, 0x42, 0x81, 0xA0,
    0x03, 0xC1, 0x23, 0x0F, 0x42, 0x44, 0x00, 0x10,
    0x00, 0x01, 0x9F, 0xFE, 0x46, 0x10, 0x18, 0xE8,
    0x0D, 0x02, 0x4B, 0x10, 0xA4, 0x74, 0x09, 0x44,
    0x99, 0x02, 0x58, 0x90, 0x61, 0x0D, 0x0A, 0x20,
    0x90, 0x22, 0x91, 0x24, 0x38, 0x42, 0x39, 0x00,
    0x00, 0x01, 0xB5, 0x23, 0x0F, 0x01, 0x14, 0x99
};

static BitOps bit_ops_2 [] = {
    {BIT_SKIP, 32, 0, "Skip start code of Seq header"},
    {BIT_GET, 8, 32, "profile_id"},
    {BIT_GET, 8, 66, "level_id"},
    {BIT_GET, 1, 1, "progressive_sequence"},
    {BIT_GET, 1, 0, "field_coded_sequence"},
    {BIT_GET, 14, 416, "horizontal_size"},
    {BIT_GET, 14, 240, "vertical_size"},
    {BIT_GET, 2, 1, "chroma_format"},
    {BIT_GET, 3, 1, "sample_precision"},
    {BIT_GET, 4, 1, "aspect_ratio"},
    {BIT_GET, 4, 8, "frame_rate_code"},
    {BIT_GET, 18, 125000, "bit_rate_lower_18"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 12, 0, "bit_rate_upper_12"},
    {BIT_GET, 1, 0, "low_delay"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 0, "enable_temporal_id"},
    {BIT_GET, 18, 0, "bbv_uffer_size"},
    {BIT_GET, 3, 6, "lcu_size"},
    {BIT_GET, 1, 0, "enable_weighted_quant"},
    // weighted quant matrix
    {BIT_GET, 1, 1, "disable_background_picture"},
    {BIT_GET, 1, 1, "enable_mhp_skip"},
    {BIT_GET, 1, 1, "enable_dhp"},
    {BIT_GET, 1, 1, "enable_wsm"},
    {BIT_GET, 1, 1, "enable_amp"},
    {BIT_GET, 1, 1, "enable_nsqt"},
    {BIT_GET, 1, 1, "enable_nsip"},
    {BIT_GET, 1, 1, "enable_2nd_transform"},
    {BIT_GET, 1, 1, "enable_sao"},
    {BIT_GET, 1, 1, "enable_alf"},
    {BIT_GET, 1, 1, "enable_pmvr"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 6, 8, "num_of_rps"},
    {BIT_GET, 1, 1, "refered_by_others[0]"},
    {BIT_GET, 3, 4, "num_of_ref[0]"},
    {BIT_GET, 6, 8, "ref_pic[0][0]"},
    {BIT_GET, 6, 3, "ref_pic[0][1]"},
    {BIT_GET, 6, 7, "ref_pic[0][2]"},
    {BIT_GET, 6, 16, "ref_pic[0][3]"},
    {BIT_GET, 3, 0, "num_to_remove[0]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 1, "refered_by_others[1]"},
    {BIT_GET, 3, 2, "num_of_ref[1]"},
    {BIT_GET, 6, 1, "ref_pic[1][0]"},
    {BIT_GET, 6, 9, "ref_pic[1][1]"},
    {BIT_GET, 3, 3, "num_to_remove[1]"},
    {BIT_GET, 6, 4, "remove_pic[1][0]"},
    {BIT_GET, 6, 10, "remove_pic[1][0]"},
    {BIT_GET, 6, 17, "remove_pic[1][0]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 1, "refered_by_others[2]"},
    {BIT_GET, 3, 2, "num_of_ref[2]"},
    {BIT_GET, 6, 1, "ref_pic[2][0]"},
    {BIT_GET, 6, 10, "ref_pic[2][1]"},
    {BIT_GET, 3, 1, "num_to_remove[2]"},
    {BIT_GET, 6, 9, "remove_pic[2][0]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 0, "refered_by_others[3]"},
    {BIT_GET, 3, 2, "num_of_ref[3]"},
    {BIT_GET, 6, 1, "ref_pic[3][0]"},
    {BIT_GET, 6, 11, "ref_pic[3][1]"},
    {BIT_GET, 3, 0, "num_to_remove[3]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 0, "refered_by_others[4]"},
    {BIT_GET, 3, 2, "num_of_ref[4]"},
    {BIT_GET, 6, 3, "ref_pic[4][0]"},
    {BIT_GET, 6, 2, "ref_pic[4][1]"},
    {BIT_GET, 3, 0, "num_to_remove[4]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 1, "refered_by_others[5]"},
    {BIT_GET, 3, 2, "num_of_ref[5]"},
    {BIT_GET, 6, 5, "ref_pic[5][0]"},
    {BIT_GET, 6, 4, "ref_pic[5][1]"},
    {BIT_GET, 3, 0, "num_to_remove[5]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 0, "refered_by_6thers[6]"},
    {BIT_GET, 3, 2, "num_of_ref[6]"},
    {BIT_GET, 6, 1, "ref_pic[6][0]"},
    {BIT_GET, 6, 5, "ref_pic[6][1]"},
    {BIT_GET, 3, 1, "num_to_remove[6]"},
    {BIT_GET, 6, 4, "remove_pic[6][0]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 1, 0, "refered_by_others[7]"},
    {BIT_GET, 3, 2, "num_of_ref[7]"},
    {BIT_GET, 6, 7, "ref_pic[7][0]"},
    {BIT_GET, 6, 2, "ref_pic[7][1]"},
    {BIT_GET, 3, 0, "num_to_remove[7]"},
    {BIT_GET, 1, 1, "marker_bit"},
    {BIT_GET, 5, 3, "picture_reorder_delay"},
    {BIT_GET, 1, 1, "enable_clf"},
    {BIT_GET, 2, 0, "reserved 2bits 00"}
};

// H264 SPS data with "00 00 03 00"
static RK_U8 test_data_3[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAD, 0x00,
                              0x2C, 0xA4, 0x01, 0xE0, 0x11, 0x1F, 0x78, 0x0B, 0x50, 0x10,
                              0x10, 0x14, 0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x03,
                              0x00, 0xCB, 0x9B, 0x80, 0x03, 0xD0, 0x90, 0x03, 0xA9, 0xEF,
                              0x7B, 0xE0, 0xA0
                             };

// H264 PPS and SEI data without "00 00 03 00"
static RK_U8 test_data_4[] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xFE, 0x3C, 0xB0};

static BitOps bit_ops_3[] = {
    {BIT_SKIP,      40, 0,      "start code of SPS"},
    {BIT_GET,       8,  100,    "profile_idc"},
    {BIT_GET,       1,  0,      "constraint_set0_flag"},
    {BIT_GET,       1,  0,      "constraint_set1_flag"},
    {BIT_GET,       1,  0,      "constraint_set2_flag"},
    {BIT_GET,       1,  0,      "constraint_set3_flag"},
    {BIT_GET,       1,  0,      "constraint_set4_flag"},
    {BIT_GET,       1,  0,      "constraint_set5_flag"},
    {BIT_GET,       2,  0,      "reserved_zero_2bits"},
    {BIT_GET,       8,  40,     "level_idc"},
    {BIT_GET_UE,    0,  0,      "seq_parameter_set_id"},
    {BIT_GET_UE,    0,  1,      "chroma_format_idc"},
    {BIT_GET_UE,    0,  0,      "bit_depth_luma_minus8"},
    {BIT_GET_UE,    0,  0,      "bit_depth_chroma_minus8"},
    {BIT_GET,       1,  0,      "qp_prime_y_zero_transform_bypass"},
    {BIT_GET,       1,  1,      "seq_scaling_matrix_present_flag"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[0]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[1]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[2]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[3]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[4]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[5]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[6]"},
    {BIT_GET,       1,  0,      "seq_sacling_list_present_flag[7]"},
    {BIT_GET_UE,    0,  4,      "log2_max_frame_num_minus4"},
    {BIT_GET_UE,    0,  0,      "pic_order_cnt_type"},
    {BIT_GET_UE,    0,  4,      "log2_max_pic_order_cnt_lsb_minus"},
    {BIT_GET_UE,    0,  3,      "max_num_ref_frames"},
    {BIT_GET,       1,  0,      "gaps_in_frame_num_value_allowed_flag"},
    {BIT_GET_UE,    0,  119,     "pic_width_in_mbs_minus1"},
    {BIT_GET_UE,    0,  33,      "pic_height_in_map_units_minus1"},
    {BIT_GET,       1,  0,       "frame_mbs_only_flag"},
    {BIT_GET,       1,  0,       "mb_adaptive_frame_field_flag"},
    {BIT_GET,       1,  1,       "direct_8x8_inference_flag"},
    {BIT_GET,       1,  1,       "frame_cropping_flag"},
    {BIT_GET_UE,    0,  0,      "frame_crop_left_offset"},
    {BIT_GET_UE,    0,  0,      "frame_crop_right_offset"},
    {BIT_GET_UE,    0,  0,      "frame_crop_top_offset"},
    {BIT_GET_UE,    0,  2,      "frame_crop_bottom_offset"},
    {BIT_GET,       1,  1,      "vui_parameters_present_flag"},
    {BIT_GET,       1,  1,      "aspect_ratio_info_present_flag"},
    {BIT_GET,       8,  1,      "aspect_ratio_idc"},
    {BIT_GET,       1,  0,      "overscan_info_ipresent_flag"},
    {BIT_GET,       1,  1,      "video_signal_type_present_flag"},
    {BIT_GET,       3,  5,      "video_format"},
    {BIT_GET,       1,  0,      "video_full_range_flag"},
    {BIT_GET,       1,  1,      "colour_description_present_flag"},
    {BIT_GET,       8,  1,      "colour_primaries"},
    {BIT_GET,       8,  1,      "transfer_characteristics"},
    {BIT_GET,       8,  1,      "matrix_coefficient"},
    {BIT_GET,       1,  0,      "chroma_loc_info_present_flag"},
    {BIT_GET,       1,  1,      "timing_info_presetn_flag"},
    {BIT_GET,       32, 1,      "num_units_in_tick"},
    {BIT_GET,       32, 50,     "time_scale"},
    {BIT_GET,       1,  1,      "fixed_frame_rate_flag"},
    {BIT_GET,       1,  1,      "nal_hrd_parameters_present_flag"},
    {BIT_GET_UE,    0,  0,      "cpb_cnt_minus1"},
    {BIT_GET,       4,  3,      "bit_rate_scale"},
    {BIT_GET,       4,  7,      "cpb_size_scale"},
    {BIT_GET_UE,    0,  15624,  "bit_rate_value_minus1[0]"},
    {BIT_GET_UE,    0,  1874,   "cpb_size_value_minus1[0]"},
    {BIT_GET,       1,  1,      "cbr_flag[0]"},
    {BIT_GET,       5,  23,     "initial_cpb_removal_delay_length_minus1"},
    {BIT_GET,       5,  23,     "cpb_removal_delay_length_minus1"},
    {BIT_GET,       5,  23,     "dpb_output_delay_length_minus1"},
    {BIT_GET,       5,  24,     "time_offset_length"},
    {BIT_GET,       1,  0,      "vcl_hrd_parameters_present_flag"},
    {BIT_GET,       1,  0,      "low_delay_hrd_flag"},
    {BIT_GET,       1,  1,      "picture_sturct_present_flag"},
    {BIT_GET,       1,  0,      "bitstream_restriction_flag"},
};

static BitOps bit_ops_4[] = {
    {BIT_SKIP,      40, 0,      "Skip start code of PPS"},
    {BIT_GET_UE,    0,  0,      "pic_parameter_set_id"},
    {BIT_GET_UE,    0,  0,      "seq_parameter_set_id"},
    {BIT_GET,       1,  1,      "entropy_coding_mode_flag"},
    {BIT_GET,       1,  1,      "bottom_field_pic_order_in_frame_present_flag"},
    {BIT_GET_UE,    0,  0,      "num_slice_groups_minus1"},
    {BIT_GET_UE,    0,  0,      "num_ref_idx_l0_default_active_minus1"},
    {BIT_GET_UE,    0,  0,      "num_ref_idx_l1_default_active_minus1"},
    {BIT_GET,       1,  0,      "weighted_pred_flag"},
    {BIT_GET,       2,  0,      "weighted_bipred_idc"},
    {BIT_GET_SE,    0,  0,      "pic_init_qp_minus26"},
    {BIT_GET_SE,    0,  0,      "pic_init_qs_minus26"},
    {BIT_GET_SE,    0,  0,      "chroma_qp_index_offset"},
    {BIT_GET,       1,  1,      "debloking_filter_control_present_flag"},
    {BIT_GET,       1,  0,      "constrained_intra_pred_flag"},
    {BIT_GET,       1,  0,      "redundant_pic_cnt_present_flag"},
    {BIT_GET,       1,  1,      "transform_8x8_mode_flag"},
    {BIT_GET,       1,  0,      "pic_scaling_matrix_present_flag"},
    {BIT_GET_SE,    0,  0,      "second_chroma_qp_index_offset"}
};

MPP_RET proc_bit_ops(BitReadCtx_t *ctx, BitOps *ops, RK_S32 *ret_val)
{
    MPP_RET ret = MPP_OK;
    switch (ops->type) {
    case BIT_GET :
        if (ops->len >= 32) {
            READ_BITS_LONG(ctx, ops->len, ret_val);
        } else {
            READ_BITS(ctx, ops->len, ret_val);
        }
        break;
    case BIT_GET_UE:
        READ_UE(ctx, ret_val);
        break;
    case BIT_GET_SE:
        READ_SE(ctx, ret_val);
        break;
    case BIT_SKIP:
        if (ops->len >= 32) {
            SKIP_BITS_LONG(ctx, ops->len);
        } else {
            SKIP_BITS(ctx, ops->len);
        }
        break;
    }
    goto __NORMAL;
__BITREAD_ERR:
    mpp_err("Read failed: syntax %s, %s, %d bits\n", ops->syntax, bitOpsStr[ops->type], ops->len);
    return ret = MPP_ERR_VALUE;
__NORMAL:
    if (ops->val != *ret_val) {
        mpp_err("Read error: syntax %s, expect %d but %d\n", ops->syntax, ops->val, *ret_val);
        ret = MPP_ERR_VALUE;
    } else {
        mpp_log("Read OK, syntax %s, %d\n", ops->syntax, *ret_val);
    }
    return ret;
}

int main()
{
    BitReadCtx_t reader;
    RK_U32 i;
    RK_S32 tmp = 0;

    mpp_log("mpp bit read test start\n");
    mpp_log("Reading H264 data with 00 00 03 00...");
    memset(&reader, 0, sizeof(BitReadCtx_t));
    mpp_set_bitread_ctx(&reader, test_data_3, sizeof(test_data_3));
    // mpp_set_pre_detection(&reader);
    mpp_set_bitread_pseudo_code_type(&reader, PSEUDO_CODE_H264_H265);
    for (i = 0; i < MPP_ARRAY_ELEMS(bit_ops_3); i++) {

        if (proc_bit_ops(&reader, &bit_ops_3[i], &tmp))
            goto __READ_FAILED;

        tmp = 0;
    }

    mpp_log("Reading H264 data without 00 00 03 00...");
    memset(&reader, 0, sizeof(BitReadCtx_t));
    mpp_set_bitread_ctx(&reader, test_data_4, sizeof(test_data_4));

    for (i = 0; i < MPP_ARRAY_ELEMS(bit_ops_4); i++) {
        if (proc_bit_ops(&reader, &bit_ops_4[i], &tmp))
            goto __READ_FAILED;

        tmp = 0;
    }

    mpp_log("Reading AVS2 data without 00 00 02...");
    memset(&reader, 0, sizeof(BitReadCtx_t));
    mpp_set_bitread_ctx(&reader, test_data_2, sizeof(test_data_2));

    for (i = 0; i < MPP_ARRAY_ELEMS(bit_ops_2); i++) {
        if (proc_bit_ops(&reader, &bit_ops_2[i], &tmp))
            goto __READ_FAILED;

        tmp = 0;
    }

    mpp_log("Reading AVS2 data with 00 00 02...");
    memset(&reader, 0, sizeof(BitReadCtx_t));
    mpp_set_bitread_ctx(&reader, test_data_1, sizeof(test_data_1));
    mpp_set_bitread_pseudo_code_type(&reader, PSEUDO_CODE_AVS2);

    for (i = 0; i < MPP_ARRAY_ELEMS(bit_ops_1); i++) {
        if (proc_bit_ops(&reader, &bit_ops_1[i], &tmp))
            goto __READ_FAILED;

        tmp = 0;
    }
    mpp_log("mpp bit read test end\n");
    return 0;
__READ_FAILED:
    return -1;
}