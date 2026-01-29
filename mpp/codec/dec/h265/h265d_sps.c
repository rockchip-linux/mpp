/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "H265d"

#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bit.h"

#include "h265d_debug.h"
#include "h265d_ctx.h"

static const RK_U8 default_scaling_list_intra[] = {
    16, 16, 16, 16, 17, 18, 21, 24,
    16, 16, 16, 16, 17, 19, 22, 25,
    16, 16, 17, 18, 20, 22, 25, 29,
    16, 16, 18, 21, 24, 27, 31, 36,
    17, 17, 20, 24, 30, 35, 41, 47,
    18, 19, 22, 27, 35, 44, 54, 65,
    21, 22, 25, 31, 41, 54, 70, 88,
    24, 25, 29, 36, 47, 65, 88, 115,
};

static const RK_U8 default_scaling_list_inter[] = {
    16, 16, 16, 16, 17, 18, 20, 24,
    16, 16, 16, 17, 18, 20, 24, 25,
    16, 16, 17, 18, 20, 24, 25, 28,
    16, 17, 18, 20, 24, 25, 28, 33,
    17, 18, 20, 24, 25, 28, 33, 41,
    18, 20, 24, 25, 28, 33, 41, 54,
    20, 24, 25, 28, 33, 41, 54, 71,
    24, 25, 28, 33, 41, 54, 71, 91,
};

const RK_U8 h265d_diag_scan4x4_map[16] = {
    0, 4,  1,  8,
    5, 2,  12, 9,
    6, 3,  13, 10,
    7, 14, 11, 15,
};

const RK_U8 h265d_diag_scan8x8_map[64] = {
    0,  8,  1,  16, 9,  2,  24, 17,
    10, 3,  32, 25, 18, 11, 4,  40,
    33, 26, 19, 12, 5,  48, 41, 34,
    27, 20, 13, 6,  56, 49, 42, 35,
    28, 21, 14, 7,  57, 50, 43, 36,
    29, 22, 15, 58, 51, 44, 37, 30,
    23, 59, 52, 45, 38, 31, 60, 53,
    46, 39, 61, 54, 47, 62, 55, 63,
};

static void h265d_sort_delta_poc(H265dStRps *rps)
{
    RK_S32 i;

    if (rps->num_deltas <= 1)
        return;

    for (i = 1; i < rps->num_deltas; i++) {
        RK_S32 delta_poc = rps->poc_delta[i];
        RK_U32 used_val = MPP_GET_BIT(rps->flags, i);
        RK_S32 j;

        for (j = i - 1; j >= 0 && rps->poc_delta[j] > delta_poc; j--) {
            rps->poc_delta[j + 1] = rps->poc_delta[j];
            MPP_MOD_BIT(rps->flags, j + 1, MPP_GET_BIT(rps->flags, j));
        }

        rps->poc_delta[j + 1] = delta_poc;
        MPP_MOD_BIT(rps->flags, j + 1, used_val);
    }
}

static void h265d_flip_negative_pics(H265dStRps *rps)
{
    RK_S32 left = 0;
    RK_S32 right = rps->num_neg - 1;
    RK_S32 tmp_poc;
    RK_U8 tmp_used;

    while (left < right) {
        tmp_poc = rps->poc_delta[left];
        tmp_used = MPP_GET_BIT(rps->flags, left);

        rps->poc_delta[left] = rps->poc_delta[right];
        MPP_MOD_BIT(rps->flags, left, MPP_GET_BIT(rps->flags, right));
        rps->poc_delta[right] = tmp_poc;
        MPP_MOD_BIT(rps->flags, right, tmp_used);

        left++;
        right--;
    }
}

static RK_S32 h265d_rps_direct(BitReadCtx_t *bit, H265dStRps *rps)
{
    RK_U32 num_positive;
    RK_S32 delta_poc;
    RK_U32 prev_poc;
    RK_S32 i;

    READ_UE(bit, &rps->num_neg);
    READ_UE(bit, &num_positive);

    if (rps->num_neg >= MAX_REFS || num_positive >= MAX_REFS) {
        mpp_loge("sps: short term rps refs count over max\n");
        return MPP_ERR_STREAM;
    }

    rps->num_deltas = rps->num_neg + num_positive;
    rps->flags = 0;

    if (rps->num_deltas == 0)
        return 0;

    prev_poc = 0;
    for (i = 0; (RK_U32)i < rps->num_neg; i++) {
        READ_UE(bit, &delta_poc);
        delta_poc += 1;
        prev_poc -= delta_poc;
        rps->poc_delta[i] = prev_poc;
        {
            RK_S32 _bit; bit->ret = mpp_read_bits(bit, 1, &_bit);
            if (bit->ret) goto __BITREAD_ERR;
            if (_bit) rps->flags |= MPP_BIT(i);
        }
    }

    prev_poc = 0;
    for (i = 0; (RK_U32)i < num_positive; i++) {
        RK_U32 idx = rps->num_neg + i;
        RK_S32 _bit;

        READ_UE(bit, &delta_poc);
        delta_poc += 1;
        prev_poc += delta_poc;
        rps->poc_delta[idx] = prev_poc;

        bit->ret = mpp_read_bits(bit, 1, &_bit);
        if (bit->ret)
            goto __BITREAD_ERR;
        if (_bit)
            rps->flags |= MPP_BIT(idx);
    }

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 h265d_rps_prediction(BitReadCtx_t *bit, H265dStRps *rps,
                                   const H265dSps *sps, RK_S32 is_slice_header)
{
    const H265dStRps *ref_rps;
    RK_U32 delta_idx = 0;
    RK_U32 sign_flag;
    RK_U32 abs_delta;
    RK_S32 ref_delta;
    RK_S32 pic_count = 0;
    RK_S32 neg_count = 0;
    RK_U8 used_array[32] = {0};
    RK_S32 i;

    rps->flags = 0;

    if (is_slice_header) {
        READ_UE(bit, &delta_idx);
        delta_idx++;
        if (delta_idx > sps->nb_st_rps) {
            mpp_loge("sps: delta_idx %u over nb_st_rps %u\n",
                     delta_idx, sps->nb_st_rps);
            return MPP_ERR_STREAM;
        }
        ref_rps = &sps->st_rps_sps[sps->nb_st_rps - delta_idx];
    } else {
        ref_rps = &sps->st_rps_sps[rps - sps->st_rps_sps - 1];
    }

    READ_BITS(bit, 1, &sign_flag);
    READ_UE(bit, &abs_delta);
    abs_delta++;

    ref_delta = sign_flag ? -((RK_S32)abs_delta) : (RK_S32)abs_delta;

    for (i = 0; i <= ref_rps->num_deltas; i++) {
        RK_U8 used_flag = 0;
        RK_U8 include_flag = 0;
        RK_S32 cur_poc;

        READ_ONEBIT(bit, &used_flag);

        if (!used_flag)
            READ_ONEBIT(bit, &include_flag);

        if (used_flag || include_flag) {
            if (i < ref_rps->num_deltas)
                cur_poc = ref_delta + ref_rps->poc_delta[i];
            else
                cur_poc = ref_delta;

            rps->poc_delta[pic_count] = cur_poc;
            used_array[pic_count] = used_flag;

            if (cur_poc < 0)
                neg_count++;

            pic_count++;
        }
    }

    rps->num_deltas = pic_count;
    rps->num_neg = neg_count;

    if (pic_count > 0) {
        h265d_sort_delta_poc(rps);
        h265d_flip_negative_pics(rps);

        for (i = 0; i < pic_count; i++)
            if (used_array[i])
                rps->flags |= MPP_BIT(i);
    }

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

RK_S32 h265d_st_rps(BitReadCtx_t *bit, H265dStRps *rps, const H265dSps *sps,
                    RK_S32 is_slice_header, RK_U8 *rps_need_update)
{
    RK_U32 use_prediction = 0;
    RK_S32 ret;

    if (rps != sps->st_rps_sps && sps->nb_st_rps)
        READ_ONEBIT(bit, &use_prediction);

    if (use_prediction) {
        ret = h265d_rps_prediction(bit, rps, sps, is_slice_header);
    } else {
        ret = h265d_rps_direct(bit, rps);
    }

    if (ret == 0 && rps_need_update)
        *rps_need_update = 1;

    return ret;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static void h265d_vui_init(H265dVui *vui)
{
    vui->sar_den = 1;

    vui->colour_primaries = MPP_FRAME_PRI_UNSPECIFIED;
    vui->transfer_characteristic = MPP_FRAME_TRC_UNSPECIFIED;
    vui->matrix_coeffs = MPP_FRAME_SPC_UNSPECIFIED;
}

static RK_S32 h265d_vui(BitReadCtx_t *bit, H265dSps *sps)
{
    static const RK_S32 vui_sar[17][2] = {
        { 0,   1  },
        { 1,   1  },
        { 12,  11 },
        { 10,  11 },
        { 16,  11 },
        { 40,  33 },
        { 24,  11 },
        { 20,  11 },
        { 32,  11 },
        { 80,  33 },
        { 18,  11 },
        { 15,  11 },
        { 64,  33 },
        { 160, 99 },
        { 4,   3  },
        { 3,   2  },
        { 2,   1  },
    };
    H265dVui *vui = &sps->vui;
    RK_S32 sar_present;

    h265d_dbg_func("Parsing vui data\n");

    vui->vui_present = 1;

    READ_ONEBIT(bit, &sar_present);
    if (sar_present) {
        RK_U32 sar_idx = 0;

        READ_BITS(bit, 8, &sar_idx);
        if (sar_idx < MPP_ARRAY_ELEMS(vui_sar)) {
            vui->sar_num = vui_sar[sar_idx][0];
            vui->sar_den = vui_sar[sar_idx][1];
        } else if (sar_idx == 255) {
            READ_BITS(bit, 16, &vui->sar_num);
            READ_BITS(bit, 16, &vui->sar_den);
        } else
            mpp_log("sps: invalid sample aspect ratio index %u\n", sar_idx);
    }

    READ_ONEBIT(bit, &vui->overscan_info_present_flag);
    if (vui->overscan_info_present_flag)
        READ_ONEBIT(bit, &vui->overscan_appropriate_flag);

    READ_ONEBIT(bit, &vui->video_signal_type_present_flag);
    if (vui->video_signal_type_present_flag) {
        READ_BITS(bit, 3, & vui->video_format);
        READ_ONEBIT(bit, &vui->video_full_range_flag);
        READ_ONEBIT(bit, &vui->colour_description_present_flag);

        if (vui->colour_description_present_flag) {
            READ_BITS(bit, 8, &vui->colour_primaries);
            READ_BITS(bit, 8, &vui->transfer_characteristic);
            READ_BITS(bit, 8, &vui->matrix_coeffs);

            if (vui->matrix_coeffs >= MPP_FRAME_SPC_NB)
                vui->matrix_coeffs = MPP_FRAME_SPC_UNSPECIFIED;
        }
    }

    READ_ONEBIT(bit, &vui->chroma_loc_info_present_flag );
    if (vui->chroma_loc_info_present_flag) {
        READ_UE(bit, &vui->chroma_sample_loc_type_top_field);
        READ_UE(bit, &vui->chroma_sample_loc_type_bottom_field);
    }

    READ_ONEBIT(bit, &vui->neutra_chroma_indication_flag);
    READ_ONEBIT(bit, &vui->field_seq_flag);
    READ_ONEBIT(bit, &vui->frame_field_info_present_flag);

    READ_ONEBIT(bit, &vui->default_display_window_flag);
    if (vui->default_display_window_flag) {
        READ_UE(bit, &vui->def_disp_win.left_offset);
        READ_UE(bit, &vui->def_disp_win.right_offset);
        READ_UE(bit, &vui->def_disp_win.top_offset);
        READ_UE(bit, &vui->def_disp_win.bottom_offset);
        if (sps) {
            if (sps->chroma_format_idc == H265_CHROMA_420) {
                vui->def_disp_win.left_offset   *= 2;
                vui->def_disp_win.right_offset  *= 2;
                vui->def_disp_win.top_offset    *= 2;
                vui->def_disp_win.bottom_offset *= 2;
            } else if (sps->chroma_format_idc == H265_CHROMA_422) {
                vui->def_disp_win.left_offset   *= 2;
                vui->def_disp_win.right_offset  *= 2;
            }
        }
    }

    READ_ONEBIT(bit, &vui->vui_timing_info_present_flag);

    if (vui->vui_timing_info_present_flag) {
        mpp_read_longbits(bit, 32, &vui->vui_num_units_in_tick);
        mpp_read_longbits(bit, 32, &vui->vui_time_scale);
        READ_ONEBIT(bit, &vui->vui_poc_proportional_to_timing_flag);
        if (vui->vui_poc_proportional_to_timing_flag)
            READ_UE(bit, &vui->vui_num_ticks_poc_diff_one_minus1);
        READ_ONEBIT(bit, &vui->vui_hrd_parameters_present_flag);
        if (vui->vui_hrd_parameters_present_flag)
            h265d_hrd_info(bit, 1, sps->max_sub_layers);
    }

    READ_ONEBIT(bit, &vui->bitstream_restriction_flag);
    if (vui->bitstream_restriction_flag) {
        READ_ONEBIT(bit, &vui->tiles_fixed_structure_flag);
        READ_ONEBIT(bit, &vui->motion_vectors_over_pic_boundaries_flag);
        READ_ONEBIT(bit, &vui->restricted_ref_pic_lists_flag);
        READ_UE(bit, &vui->min_spatial_segmentation_idc);
        READ_UE(bit, &vui->max_bytes_per_pic_denom);
        READ_UE(bit, &vui->max_bits_per_min_cu_denom);
        READ_UE(bit, &vui->log2_max_mv_length_horizontal);
        READ_UE(bit, &vui->log2_max_mv_length_vertical);
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

void h265d_set_default_scaling_list(H265dScalingList *sl)
{
    RK_S32 matrixId;

    for (matrixId = 0; matrixId < 6; matrixId++) {
        memset(sl->scaling_lists[SCALE_4X4_COPY][matrixId], 16, 16);
        sl->scaling_list_dc[0][matrixId] = 16;
        sl->scaling_list_dc[1][matrixId] = 16;
    }
    memcpy(sl->scaling_lists[SCALE_4X4][0], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_4X4][1], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_4X4][2], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_4X4][3], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_4X4][4], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_4X4][5], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_8X8][0], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_8X8][1], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_8X8][2], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_8X8][3], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_8X8][4], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_8X8][5], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_16X16][0], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_16X16][1], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_16X16][2], default_scaling_list_intra, 64);
    memcpy(sl->scaling_lists[SCALE_16X16][3], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_16X16][4], default_scaling_list_inter, 64);
    memcpy(sl->scaling_lists[SCALE_16X16][5], default_scaling_list_inter, 64);
}

RK_S32 h265d_scaling_list_data(BitReadCtx_t *bit, H265dScalingList *sl, H265dSps *sps)
{
    RK_S32 sl_dc_coef[2][6];
    RK_S32 sz_id, mat_id, pos;
    RK_S32 coef_count, idx;
    RK_U8 pred_flag;

    for (sz_id = 0; sz_id < 4; sz_id++) {
        RK_U32 mat_step = (sz_id == 3) ? 3 : 1;
        for (mat_id = 0; mat_id < 6; mat_id += mat_step) {
            READ_ONEBIT(bit, &pred_flag);

            if (!pred_flag) {
                RK_U32 delta = 0;
                READ_UE(bit, &delta);
                if (delta) {
                    delta *= mat_step;
                    if ((RK_U32)mat_id < delta) {
                        mpp_loge("sps: scaling list delta %u over matrix_id %u\n", delta, mat_id);
                        return MPP_ERR_STREAM;
                    }

                    memcpy(sl->scaling_lists[sz_id][mat_id],
                           sl->scaling_lists[sz_id][mat_id - delta],
                           sz_id > 0 ? 64 : 16);
                    if (sz_id > 1)
                        sl->scaling_list_dc[sz_id - 2][mat_id] = sl->scaling_list_dc[sz_id - 2][mat_id - delta];
                }
            } else {
                RK_S32 coef_val = 8;
                RK_S32 delta_coef;

                coef_count = MPP_MIN(64, 1 << (4 + (sz_id << 1)));

                if (sz_id > 1) {
                    RK_S32 dc_minus8;
                    READ_SE(bit, &dc_minus8);
                    if (dc_minus8 < -7 || dc_minus8 > 247) {
                        mpp_loge("sps: dc coeff out of range %d\n", dc_minus8);
                        return MPP_ERR_STREAM;
                    }
                    sl_dc_coef[sz_id - 2][mat_id] = dc_minus8 + 8;
                    coef_val = sl_dc_coef[sz_id - 2][mat_id];
                    sl->scaling_list_dc[sz_id - 2][mat_id] = coef_val;
                }

                for (idx = 0; idx < coef_count; idx++) {
                    if (sz_id == 0)
                        pos = h265d_diag_scan4x4_map[idx];
                    else
                        pos = h265d_diag_scan8x8_map[idx];

                    READ_SE(bit, &delta_coef);
                    coef_val = (coef_val + delta_coef + 256) % 256;
                    sl->scaling_lists[sz_id][mat_id][pos] = coef_val;
                }
            }
        }
    }

    if (sps->chroma_format_idc == H265_CHROMA_444) {
        for (idx = 0; idx < 64; idx++) {
            sl->scaling_lists[SCALE_16X16][1][idx] = sl->scaling_lists[SCALE_8X8][1][idx];
            sl->scaling_lists[SCALE_16X16][2][idx] = sl->scaling_lists[SCALE_8X8][2][idx];
            sl->scaling_lists[SCALE_16X16][4][idx] = sl->scaling_lists[SCALE_8X8][4][idx];
            sl->scaling_lists[SCALE_16X16][5][idx] = sl->scaling_lists[SCALE_8X8][5][idx];
        }
        sl->scaling_list_dc[1][1] = sl->scaling_list_dc[0][1];
        sl->scaling_list_dc[1][2] = sl->scaling_list_dc[0][2];
        sl->scaling_list_dc[1][4] = sl->scaling_list_dc[0][4];
        sl->scaling_list_dc[1][5] = sl->scaling_list_dc[0][5];
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

RK_S32 h265d_nal_sps(BitReadCtx_t *bit, H265dSps *sps, const H265dVps *vps_list[])
{
    RK_U32 sps_id = 0;
    RK_S32 log2_diff_max_min_transform_block_size;
    RK_U32 bit_depth_chroma;
    RK_S32 start;
    RK_S32 sublayer_ordering_info;
    RK_S32 value = 0;
    RK_S32 ret = 0;
    RK_S32 i;

    h265d_dbg_func("Parsing sps data\n");

    READ_BITS(bit, 4, &sps->vps_id);
    h265d_dbg_sps("sps: vps_id %d", sps->vps_id);
    if (sps->vps_id >= MAX_VPS_COUNT) {
        mpp_loge("sps: invalid vps_id %d over MAX_VPS_COUNT\n", sps->vps_id);
        ret = MPP_ERR_STREAM;
        goto err;
    }

    if (!vps_list[sps->vps_id]) {
        mpp_loge("sps: vps_id %d missing in list\n", sps->vps_id);
    }

    READ_BITS(bit, 3, &sps->max_sub_layers);
    sps->max_sub_layers += 1;

    if (sps->max_sub_layers > MAX_SUB_LAYERS) {
        mpp_loge("sps: max_sub_layers %d over limit %d\n",
                 sps->max_sub_layers, MAX_SUB_LAYERS);
        ret = MPP_ERR_STREAM;
        goto err;
    }

    SKIP_BITS(bit, 1);

    h265d_layer_info(bit, sps->layers, sps->max_sub_layers);

    READ_UE(bit, &sps_id);
    sps->sps_id = sps_id;
    h265d_dbg_sps("sps: sps_id %d", sps->sps_id);
    if (sps_id >= MAX_SPS_COUNT) {
        mpp_loge("sps: sps_id %d over MAX_SPS_COUNT\n", sps_id);
        ret = MPP_ERR_STREAM;
        goto err;
    }

    READ_UE(bit, &sps->chroma_format_idc);
    h265d_dbg_sps("sps: chroma_format_idc %d", sps->chroma_format_idc);

    if (sps->chroma_format_idc == H265_CHROMA_444)
        READ_ONEBIT(bit, &sps->separate_colour_plane_flag);

    READ_UE(bit, &sps->width);
    h265d_dbg_sps("sps: width %d", sps->width);
    READ_UE(bit, &sps->height);
    h265d_dbg_sps("sps: height %d", sps->height);

    READ_ONEBIT(bit, &value);

    if (value) {
        READ_UE(bit, &sps->conf_win.left_offset);
        READ_UE(bit, &sps->conf_win.right_offset);
        READ_UE(bit, &sps->conf_win.top_offset);
        READ_UE(bit, &sps->conf_win.bottom_offset);
        if (sps->chroma_format_idc == H265_CHROMA_420) {
            sps->conf_win.left_offset   *= 2;
            sps->conf_win.right_offset  *= 2;
            sps->conf_win.top_offset    *= 2;
            sps->conf_win.bottom_offset *= 2;
        } else if (sps->chroma_format_idc == H265_CHROMA_422) {
            sps->conf_win.left_offset   *= 2;
            sps->conf_win.right_offset  *= 2;
        }
    }

    READ_UE(bit, &sps->bit_depth);

    sps->bit_depth   =  sps->bit_depth + 8;
    h265d_dbg_sps("sps: bit_depth %d", sps->bit_depth);
    READ_UE(bit, &bit_depth_chroma);
    bit_depth_chroma = bit_depth_chroma + 8;
    if (bit_depth_chroma != sps->bit_depth) {
        mpp_loge("sps: bit depth luma %d chroma %d mismatch\n",
                 sps->bit_depth, bit_depth_chroma);
        ret = MPP_ERR_STREAM;
        goto err;
    }

    switch (sps->chroma_format_idc) {
    case H265_CHROMA_400 : {
        sps->pix_fmt = MPP_FMT_YUV400;
    } break;
    case H265_CHROMA_420 : {
        switch (sps->bit_depth) {
        case 8:  sps->pix_fmt = MPP_FMT_YUV420SP; break;
        case 10: sps->pix_fmt = MPP_FMT_YUV420SP_10BIT; break;
        default:
            mpp_loge("sps: unsupported bit depth %d\n", sps->bit_depth);
            ret = MPP_ERR_PROTOL;
            goto err;
        }
    } break;
    case H265_CHROMA_422 : {
        switch (sps->bit_depth) {
        case 8:  sps->pix_fmt = MPP_FMT_YUV422SP; break;
        case 10: sps->pix_fmt = MPP_FMT_YUV422SP_10BIT; break;
        default:
            mpp_loge("sps: unsupported bit depth %d\n", sps->bit_depth);
            ret = MPP_ERR_PROTOL;
            goto err;
        }
    } break;
    case H265_CHROMA_444 : {
        switch (sps->bit_depth) {
        case 8:  sps->pix_fmt = MPP_FMT_YUV444SP; break;
        case 10: sps->pix_fmt = MPP_FMT_YUV444SP_10BIT; break;
        default:
            mpp_loge("sps: unsupported bit depth %d\n", sps->bit_depth);
            ret = MPP_ERR_PROTOL;
            goto err;
        }
    } break;
    }

    READ_UE(bit, &sps->log2_max_poc_lsb);
    sps->log2_max_poc_lsb += 4;
    if (sps->log2_max_poc_lsb > 16) {
        mpp_loge("sps: log2_max_pic_order_cnt_lsb_minus4 out range: %d\n",
                 sps->log2_max_poc_lsb - 4);
        ret = MPP_ERR_STREAM;
        goto err;
    }

    READ_ONEBIT(bit, &sublayer_ordering_info);
    start = (sublayer_ordering_info != 0) ? 0 : sps->max_sub_layers - 1;
    for (i = start; i < sps->max_sub_layers; i++) {
        READ_UE(bit, &sps->layers[i].max_dec_pic_buffering) ;
        sps->layers[i].max_dec_pic_buffering += 1;
        READ_UE(bit, &sps->layers[i].num_reorder_pics);
        READ_UE(bit, &sps->layers[i].max_latency_increase );
        sps->layers[i].max_latency_increase  -= 1;
        if (sps->layers[i].max_dec_pic_buffering > MAX_DPB_SIZE) {
            mpp_loge("sps: dpb size %d over max %d\n",
                     sps->layers[i].max_dec_pic_buffering, MAX_DPB_SIZE);
            ret = MPP_ERR_STREAM;
            goto err;
        }
        if (sps->layers[i].num_reorder_pics > sps->layers[i].max_dec_pic_buffering - 1) {
            mpp_loge("sps: reorder pics %d over dpb limit %d\n",
                     sps->layers[i].num_reorder_pics, sps->layers[i].max_dec_pic_buffering - 1);
            if (sps->layers[i].num_reorder_pics > MAX_DPB_SIZE - 1) {
                ret = MPP_ERR_STREAM;
                goto err;
            }
            sps->layers[i].max_dec_pic_buffering = sps->layers[i].num_reorder_pics + 1;
        }
    }

    if (!sublayer_ordering_info) {
        for (i = 0; i < start; i++) {
            sps->layers[i].max_dec_pic_buffering = sps->layers[start].max_dec_pic_buffering;
            sps->layers[i].num_reorder_pics      = sps->layers[start].num_reorder_pics;
            sps->layers[i].max_latency_increase  = sps->layers[start].max_latency_increase;
        }
    }

    if (!sps->layers[0].ptl_info.progressive_source_flag &&
        sps->layers[0].ptl_info.interlaced_source_flag) {
        for (i = 0; i < sps->max_sub_layers; i++) {
            sps->layers[i].num_reorder_pics += 2;
        }
    }

    READ_UE(bit, &sps->log2_min_cb_size) ;
    if (sps->log2_min_cb_size > (LOG2_MAX_CU_SIZE - 3)) {
        mpp_loge("sps: log2_min_cb_size over LOG2_MAX_CU_SIZE\n");
        ret = MPP_ERR_STREAM;
        goto err;
    }
    sps->log2_min_cb_size += 3;

    h265d_dbg_sps("sps: log2_min_cb_size %d", sps->log2_min_cb_size);
    READ_UE(bit, &sps->log2_diff_max_min_coding_block_size);
    if (sps->log2_diff_max_min_coding_block_size > (LOG2_MAX_CU_SIZE - LOG2_MIN_CU_SIZE)) {
        mpp_loge("sps: log2_diff_max_min_coding_block_size over limit\n");
        ret = MPP_ERR_STREAM;
        goto err;
    }

    h265d_dbg_sps("sps: log2_diff_max_min_coding_block_size %d",
                  sps->log2_diff_max_min_coding_block_size);
    READ_UE(bit, &sps->log2_min_tb_size);
    if (sps->log2_min_tb_size > (LOG2_MAX_TU_SIZE - 2)) {
        mpp_loge("sps: log2_min_tb_size over LOG2_MAX_TU_SIZE\n");
        ret = MPP_ERR_STREAM;
        goto err;
    }
    sps->log2_min_tb_size += 2;

    h265d_dbg_sps("sps: log2_min_tb_size %d", sps->log2_min_tb_size);
    READ_UE(bit, &log2_diff_max_min_transform_block_size);
    if (log2_diff_max_min_transform_block_size > (LOG2_MAX_TU_SIZE - LOG2_MIN_TU_SIZE)) {
        mpp_loge("sps: log2_diff_max_min_transform_block_size over limit\n");
        ret = MPP_ERR_STREAM;
        goto err;
    }

    h265d_dbg_sps("sps: log2_diff_max_min_transform_block_size %d",
                  log2_diff_max_min_transform_block_size);
    sps->log2_max_trafo_size = log2_diff_max_min_transform_block_size +
                               sps->log2_min_tb_size;

    h265d_dbg_sps("sps: log2_max_trafo_size %d", sps->log2_max_trafo_size);

    if (sps->log2_min_tb_size >= sps->log2_min_cb_size) {
        mpp_loge("sps: log2_min_tb_size over LOG2_MAX_TU_SIZE\n");
        ret = MPP_ERR_STREAM;
        goto err;
    }

    READ_UE(bit, &sps->max_transform_hierarchy_depth_inter);
    READ_UE(bit, &sps->max_transform_hierarchy_depth_intra);

    READ_ONEBIT(bit, &sps->scaling_list_enable_flag );

    if (sps->scaling_list_enable_flag) {
        value = 0;
        h265d_set_default_scaling_list(&sps->scaling_list);
        READ_ONEBIT(bit, &value);
        if (value) {
            ret = h265d_scaling_list_data(bit, &sps->scaling_list, sps);
            if (ret < 0)
                goto err;
        }
    }

    READ_ONEBIT(bit, &sps->amp_enabled_flag);
    READ_ONEBIT(bit, &sps->sao_enabled);
    READ_ONEBIT(bit, &sps->pcm_enabled_flag);

    h265d_dbg_sps("sps: amp_enabled_flag %d", sps->amp_enabled_flag);
    h265d_dbg_sps("sps: sao_enabled %d", sps->sao_enabled);
    h265d_dbg_sps("sps: pcm_enabled_flag %d", sps->pcm_enabled_flag);

    if (sps->pcm_enabled_flag) {
        READ_BITS(bit, 4, &sps->pcm.bit_depth);
        sps->pcm.bit_depth +=  1;
        READ_BITS(bit, 4, &sps->pcm.bit_depth_chroma);
        sps->pcm.bit_depth_chroma += 1;
        READ_UE(bit, &sps->pcm.log2_min_pcm_cb_size );
        sps->pcm.log2_min_pcm_cb_size += 3;
        READ_UE(bit, &value);
        sps->pcm.log2_max_pcm_cb_size = sps->pcm.log2_min_pcm_cb_size + value;

        if (sps->pcm.bit_depth > sps->bit_depth) {
            mpp_loge("PCM bit depth %d over normal bit depth %d\n",
                     sps->pcm.bit_depth, sps->bit_depth);
            ret = MPP_ERR_STREAM;
            goto err;
        }
        READ_ONEBIT(bit, &sps->pcm.loop_filter_disable_flag);
    }

    READ_UE(bit, &sps->nb_st_rps);
    h265d_dbg_sps("sps: nb_st_rps %d", sps->nb_st_rps);
    if (sps->nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {
        mpp_loge("sps: short term rps count %d over MAX_SHORT_TERM_RPS_COUNT\n",
                 sps->nb_st_rps);
        ret = MPP_ERR_STREAM;
        goto err;
    }
    for (i = 0; i < (RK_S32)sps->nb_st_rps; i++) {
        if ((ret = h265d_st_rps(bit, &sps->st_rps_sps[i],
                                sps, 0, NULL)) < 0)
            goto err;
    }

    READ_ONEBIT(bit, &sps->lt_rps_sps.valid);
    if (sps->lt_rps_sps.valid) {
        RK_U8 used_tmp = 0;

        READ_UE(bit, &sps->lt_rps_sps.num_refs);
        sps->lt_rps_sps.used_flag = 0;
        for (i = 0; i < (RK_S32)sps->lt_rps_sps.num_refs; i++) {
            READ_BITS(bit, sps->log2_max_poc_lsb, &sps->lt_rps_sps.poc_lsb[i]);
            READ_ONEBIT(bit, &used_tmp);
            MPP_MOD_BIT(sps->lt_rps_sps.used_flag, i, used_tmp);
        }
    }

    READ_ONEBIT(bit, &sps->sps_temporal_mvp_enabled_flag);
    READ_ONEBIT(bit, &sps->sps_strong_intra_smoothing_enable_flag);

    h265d_vui_init(&sps->vui);
    READ_ONEBIT(bit, &sps->vui.vui_present);
    if (sps->vui.vui_present) {
        ret = h265d_vui(bit, sps);
        if (ret < 0)
            goto err;
    }

    sps->output_window = sps->conf_win;
    if (sps->vui.default_display_window_flag) {
        sps->output_window.left_offset   += sps->vui.def_disp_win.left_offset;
        sps->output_window.right_offset  += sps->vui.def_disp_win.right_offset;
        sps->output_window.top_offset    += sps->vui.def_disp_win.top_offset;
        sps->output_window.bottom_offset += sps->vui.def_disp_win.bottom_offset;
    }

    READ_ONEBIT(bit, &sps->sps_extension_flag);
    if (sps->sps_extension_flag) {
        READ_ONEBIT(bit, &sps->sps_range_extension_flag);
        SKIP_BITS(bit, 7);
        if (sps->sps_range_extension_flag) {
            READ_ONEBIT(bit, &sps->transform_skip_rotation_enabled_flag);
            READ_ONEBIT(bit, &sps->transform_skip_context_enabled_flag);
            READ_ONEBIT(bit, &sps->implicit_rdpcm_enabled_flag);
            READ_ONEBIT(bit, &sps->explicit_rdpcm_enabled_flag);

            READ_ONEBIT(bit, &sps->extended_precision_processing_flag);
            if (sps->extended_precision_processing_flag)
                mpp_log("sps: extended precision not implemented\n");

            READ_ONEBIT(bit, &sps->intra_smoothing_disabled_flag);
            READ_ONEBIT(bit, &sps->high_precision_offsets_enabled_flag);
            if (sps->high_precision_offsets_enabled_flag)
                mpp_log("sps: high precision offsets not implemented\n");

            READ_ONEBIT(bit, &sps->persistent_rice_adaptation_enabled_flag);

            READ_ONEBIT(bit, &sps->cabac_bypass_alignment_enabled_flag);
            if (sps->cabac_bypass_alignment_enabled_flag)
                mpp_log("sps: cabac bypass alignment not implemented\n");
        }
    }

    sps->output_width  = sps->width -
                         (sps->output_window.left_offset + sps->output_window.right_offset);
    sps->output_height = sps->height -
                         (sps->output_window.top_offset + sps->output_window.bottom_offset);
    h265d_dbg_sps("sps: output_width %d", sps->output_width);
    h265d_dbg_sps("sps: output_height %d", sps->output_height);
    if (sps->output_width <= 0 || sps->output_height <= 0) {
        mpp_log("sps: invalid visible frame dimensions %dx%d\n",
                sps->output_width, sps->output_height);
        sps->conf_win.left_offset   = 0;
        sps->conf_win.right_offset  = 0;
        sps->conf_win.top_offset    = 0;
        sps->conf_win.bottom_offset = 0;
        sps->output_window.left_offset   = 0;
        sps->output_window.right_offset  = 0;
        sps->output_window.top_offset    = 0;
        sps->output_window.bottom_offset = 0;
        sps->output_width                = sps->width;
        sps->output_height               = sps->height;
    }

    sps->log2_ctb_size = sps->log2_min_cb_size + sps->log2_diff_max_min_coding_block_size;
    h265d_dbg_sps("sps: log2_ctb_size %d", sps->log2_ctb_size);

    sps->ctb_width  = (sps->width  + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;
    h265d_dbg_sps("sps: ctb_width %d", sps->ctb_width);
    sps->ctb_height = (sps->height + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;
    h265d_dbg_sps("sps: ctb_height %d", sps->ctb_height);

    sps->min_cb_width  = sps->width  >> sps->log2_min_cb_size;
    sps->min_cb_height = sps->height >> sps->log2_min_cb_size;

    sps->qp_bd_offset = 6 * (sps->bit_depth - 8);

    if (sps->width  & ((1 << sps->log2_min_cb_size) - 1) ||
        sps->height & ((1 << sps->log2_min_cb_size) - 1)) {
        mpp_loge("sps: invalid frame dimensions\n");
        goto err;
    }

    if (sps->log2_ctb_size > MAX_LOG2_CTB_SIZE) {
        mpp_loge("sps: ctb size invalid 2^%d\n", sps->log2_ctb_size);
        goto err;
    }
    if (sps->max_transform_hierarchy_depth_inter >
        (RK_S32)(sps->log2_ctb_size - sps->log2_min_tb_size)) {
        mpp_loge("transform_hierarchy_depth_inter %d over limit\n",
                 sps->max_transform_hierarchy_depth_inter);
        goto err;
    }
    if (sps->max_transform_hierarchy_depth_intra >
        (RK_S32)(sps->log2_ctb_size - sps->log2_min_tb_size)) {
        mpp_loge("sps: transform_hierarchy_depth_intra %d over limit\n",
                 sps->max_transform_hierarchy_depth_intra);
        goto err;
    }
    if (sps->log2_max_trafo_size > (RK_U32)MPP_MIN(sps->log2_ctb_size, 5)) {
        mpp_loge("sps: log2_max_trafo_size %d over limit\n",
                 sps->log2_max_trafo_size);
        goto err;
    }

    return 0;
__BITREAD_ERR:
    ret = MPP_ERR_STREAM;
err:
    return ret;
}
