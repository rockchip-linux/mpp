/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "H265d"

#include "mpp_mem.h"

#include "h265d_debug.h"
#include "h265d_ctx.h"

static RK_S32 h265d_ptl(BitReadCtx_t *bit, PTLInfo *info)
{
    RK_S32 i;

    READ_BITS(bit, 2, &info->profile_space);
    READ_ONEBIT(bit, &info->tier_flag);
    READ_BITS(bit, 5, &info->profile_idc);

    if (info->profile_idc == MPP_PROFILE_HEVC_MAIN)
        h265d_dbg_global("Main profile\n");
    else if (info->profile_idc == MPP_PROFILE_HEVC_MAIN_10)
        h265d_dbg_global("Main 10 profile\n");
    else if (info->profile_idc == MPP_PROFILE_HEVC_MAIN_STILL_PICTURE)
        h265d_dbg_global("Main Still Picture profile\n");
    else if (info->profile_idc == MPP_PROFILE_HEVC_FORMAT_RANGE_EXTENDIONS)
        h265d_dbg_global("Main 4:4:4 profile\n");
    else
        mpp_log("Unknown profile: %d\n", info->profile_idc);

    for (i = 0; i < 32; i++)
        READ_ONEBIT(bit, & info->profile_compatibility_flag[i]);
    READ_ONEBIT(bit, &info->progressive_source_flag);
    READ_ONEBIT(bit, &info->interlaced_source_flag);
    READ_ONEBIT(bit, &info->non_packed_constraint_flag);
    READ_ONEBIT(bit, &info->frame_only_constraint_flag);

    info->bit_depth_constraint = (info->profile_idc == MPP_PROFILE_HEVC_MAIN_10) ? 10 : 8;
    info->chroma_format_constraint = H265_CHROMA_420;
    info->intra_constraint_flag = 0;
    info->lower_bitrate_constraint_flag = 1;

    SKIP_BITS(bit, 16);
    SKIP_BITS(bit, 16);
    SKIP_BITS(bit, 12);

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

RK_S32 h265d_layer_info(BitReadCtx_t *bit, LayerInfo *layers, RK_S32 max_num_sub_layers)
{
    RK_U32 profile_present_flag[MAX_SUB_LAYERS - 1];
    RK_U32 level_present_flag[MAX_SUB_LAYERS - 1];
    RK_S32 i;

    h265d_ptl(bit, &layers[0].ptl_info);
    READ_BITS(bit, 8, &layers[0].ptl_info.level_idc);

    for (i = 0; i < max_num_sub_layers - 1; i++) {
        READ_ONEBIT(bit, &profile_present_flag[i]);
        READ_ONEBIT(bit, &level_present_flag[i]);
    }
    if (max_num_sub_layers - 1 > 0)
        SKIP_BITS(bit, (8 - (max_num_sub_layers - 1)) * 2);
    for (i = 0; i < max_num_sub_layers - 1; i++) {
        if (profile_present_flag[i])
            h265d_ptl(bit, &layers[i + 1].ptl_info);
        if (level_present_flag[i])
            READ_BITS(bit, 8, &layers[i + 1].ptl_info.level_idc);
    }

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 h265d_sublayer_hrd(BitReadCtx_t *bit, RK_U32 nb_cpb, RK_S32 subpic_params_present)
{
    RK_U32 i, value;

    for (i = 0; i < nb_cpb; i++) {
        READ_UE(bit, &value);
        READ_UE(bit, &value);

        if (subpic_params_present) {
            READ_UE(bit, &value);
            READ_UE(bit, &value);
        }
        SKIP_BITS(bit, 1);
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

RK_S32 h265d_hrd_info(BitReadCtx_t *bit, RK_S32 common_inf_present, RK_S32 max_sublayers)
{
    RK_S32 nal_params_present = 0;
    RK_S32 vcl_params_present = 0;
    RK_S32 subpic_params_present = 0;
    RK_S32 i;

    if (common_inf_present) {
        READ_ONEBIT(bit, &nal_params_present);
        READ_ONEBIT(bit, &vcl_params_present);

        if (nal_params_present || vcl_params_present) {
            READ_ONEBIT(bit, &subpic_params_present);

            if (subpic_params_present) {
                SKIP_BITS(bit, 8);
                SKIP_BITS(bit, 5);
                SKIP_BITS(bit, 1);
                SKIP_BITS(bit, 5);
            }

            SKIP_BITS(bit, 4);
            SKIP_BITS(bit, 4);

            if (subpic_params_present)
                SKIP_BITS(bit, 4);

            SKIP_BITS(bit, 5);
            SKIP_BITS(bit, 5);
            SKIP_BITS(bit, 5);
        }
    }

    for (i = 0; i < max_sublayers; i++) {
        RK_S32 low_delay = 0;
        RK_U32 nb_cpb = 1;
        RK_S32 fixed_rate = 0;
        RK_S32 value = 0;

        READ_ONEBIT(bit, &fixed_rate);

        if (!fixed_rate)
            READ_ONEBIT(bit, &fixed_rate);

        if (fixed_rate)
            READ_UE(bit, &value);
        else
            READ_ONEBIT(bit, &low_delay);

        if (!low_delay) {
            READ_UE(bit, &nb_cpb);
            nb_cpb = nb_cpb + 1;
        }

        if (nal_params_present)
            h265d_sublayer_hrd(bit, nb_cpb, subpic_params_present);
        if (vcl_params_present)
            h265d_sublayer_hrd(bit, nb_cpb, subpic_params_present);
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

RK_S32 h265d_nal_vps(BitReadCtx_t *gb, H265dVps *vps)
{
    RK_U32 vps_id = 0;
    RK_S32 value = 0;
    RK_S32 i;
    RK_S32 j;

    h265d_dbg_func("Parsing vps data\n");

    READ_BITS(gb, 4, &vps_id);
    vps->vps_id = vps_id;

    h265d_dbg_vps("vps: vps_id %d", vps->vps_id);

    if (vps_id >= MAX_VPS_COUNT) {
        mpp_loge("vps: vps_id %d over MAX_VPS_COUNT\n", vps_id);
        goto err;
    }
    READ_BITS(gb, 2, &value);
    if (value != 3) {
        mpp_loge("vps: invalid reserved bits (expected 3)\n");
        goto err;
    }

    READ_BITS(gb, 6, &vps->vps_max_layers);
    vps->vps_max_layers = vps->vps_max_layers + 1;

    READ_BITS(gb, 3, &vps->vps_max_sub_layers);
    vps->vps_max_sub_layers =  vps->vps_max_sub_layers + 1;

    READ_ONEBIT(gb, & vps->vps_temporal_id_nesting_flag);
    READ_BITS(gb, 16, &value);

    if (value != 0xffff) {
        mpp_loge("vps: invalid reserved 16-bit field\n");
        goto err;
    }

    if (vps->vps_max_sub_layers > MAX_SUB_LAYERS) {
        mpp_loge("vps: invalid max_sub_layers %d, limit is MAX_SUB_LAYERS\n",
                 vps->vps_max_sub_layers);
        goto err;
    }

    h265d_layer_info(gb, vps->layers, vps->vps_max_sub_layers);

    READ_ONEBIT(gb, &vps->vps_sub_layer_ordering_info_present_flag);

    i = (vps->vps_sub_layer_ordering_info_present_flag != 0) ? 0 : vps->vps_max_sub_layers - 1;
    for (; i < vps->vps_max_sub_layers; i++) {
        LayerInfo *layer = &vps->layers[i];

        READ_UE(gb, &layer->max_dec_pic_buffering);
        layer->max_dec_pic_buffering = layer->max_dec_pic_buffering + 1;
        READ_UE(gb, &layer->num_reorder_pics);
        READ_UE(gb, &layer->max_latency_increase);
        layer->max_latency_increase = layer->max_latency_increase - 1;

        if (layer->max_dec_pic_buffering > MAX_DPB_SIZE) {
            mpp_loge("vps: dpb size %d over MAX_DPB_SIZE\n",
                     layer->max_dec_pic_buffering);
            goto err;
        }
        if (layer->num_reorder_pics > layer->max_dec_pic_buffering - 1) {
            mpp_loge("vps: reorder pics %d over DPB-1\n",
                     layer->num_reorder_pics);
            goto err;
        }
    }

    READ_BITS(gb, 6, &vps->vps_max_layer_id);
    READ_UE(gb, &vps->vps_num_layer_sets);
    vps->vps_num_layer_sets += 1;
    for (i = 1; i < vps->vps_num_layer_sets; i++)
        for (j = 0; j <= vps->vps_max_layer_id; j++)
            SKIP_BITS(gb, 1);

    READ_ONEBIT(gb, &vps->vps_timing_info_present_flag);
    if (vps->vps_timing_info_present_flag) {
        mpp_read_longbits(gb, 32, &vps->vps_num_units_in_tick);
        mpp_read_longbits(gb, 32, &vps->vps_time_scale);
        READ_ONEBIT(gb, &vps->vps_poc_proportional_to_timing_flag);
        if (vps->vps_poc_proportional_to_timing_flag) {
            READ_UE(gb, &vps->vps_num_ticks_poc_diff_one);
            vps->vps_num_ticks_poc_diff_one +=  1;
        }
        READ_UE(gb, &vps->vps_num_hrd_parameters);
        for (i = 0; i < vps->vps_num_hrd_parameters; i++) {
            RK_S32 common_inf_present = 1;
            RK_S32 hrd_layer_set_idx = 0;

            READ_UE(gb, &hrd_layer_set_idx);
            if (i)
                READ_ONEBIT(gb, &common_inf_present);
            h265d_hrd_info(gb, common_inf_present, vps->vps_max_sub_layers);
        }
    }

    return 0;
__BITREAD_ERR:
err:
    return MPP_ERR_STREAM;
}
