/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_VPS_H
#define H265D_VPS_H

#include "mpp_bitread.h"
#include "h265_syntax.h"

typedef struct PTLInfo_t {
    RK_U32 profile_space;
    RK_U32 tier_flag;
    RK_U32 profile_idc;
    RK_U32 profile_compatibility_flag[32];
    RK_U32 level_idc;
    RK_U32 progressive_source_flag;
    RK_U32 interlaced_source_flag;
    RK_U32 non_packed_constraint_flag;
    RK_U32 frame_only_constraint_flag;
    RK_S32 bit_depth_constraint;
    H265ChromaFmt chroma_format_constraint;
    RK_U32 intra_constraint_flag;
    RK_U32 one_picture_only_constraint_flag;
    RK_U32 lower_bitrate_constraint_flag;
} PTLInfo;

typedef struct LayerInfo_t {
    PTLInfo ptl_info;
    RK_U32 max_dec_pic_buffering;
    RK_U32 num_reorder_pics;
    RK_S32 max_latency_increase;
} LayerInfo;

typedef struct H265dVps_t {
    RK_U32 vps_id;
    RK_U32 vps_temporal_id_nesting_flag;
    RK_S32 vps_max_layers;
    RK_S32 vps_max_sub_layers;
    LayerInfo layers[MAX_SUB_LAYERS];
    RK_S32 vps_sub_layer_ordering_info_present_flag;
    RK_S32 vps_max_layer_id;
    RK_S32 vps_num_layer_sets;
    RK_U32 vps_timing_info_present_flag;
    RK_U32 vps_num_units_in_tick;
    RK_U32 vps_time_scale;
    RK_U32 vps_poc_proportional_to_timing_flag;
    RK_S32 vps_num_ticks_poc_diff_one;
    RK_S32 vps_num_hrd_parameters;
    RK_S32 vps_extension_flag;
} H265dVps;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 h265d_layer_info(BitReadCtx_t *bit, LayerInfo *layers, RK_S32 max_num_sub_layers);
RK_S32 h265d_hrd_info(BitReadCtx_t *bit, RK_S32 common_inf_present, RK_S32 max_sublayers);
RK_S32 h265d_nal_vps(BitReadCtx_t *bit, H265dVps *vps);

#ifdef  __cplusplus
}
#endif

#endif /* H265D_VPS_H */
