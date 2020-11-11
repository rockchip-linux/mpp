/*
 *
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H265D_COM_H__
#define __HAL_H265D_COM_H__

#include "rk_type.h"

#define SCALING_LIST_SIZE       (81 * 1360)
#define RPS_SIZE                (600 * 32)
#define SCALING_LIST_SIZE_NUM   4

#define L0          0
#define L1          1
#define MAX_REFS    16

#define IS_IDR(nal_type)    (nal_type == 19 || nal_type == 20)
#define IS_BLA(nal_type)    (nal_type == 17 || nal_type == 16 || nal_type == 18)
#define IS_IRAP(nal_type)   (nal_type >= 16 && nal_type <= 23)

enum RPSType {
    ST_CURR_BEF = 0,
    ST_CURR_AFT,
    ST_FOLL,
    LT_CURR,
    LT_FOLL,
    NB_RPS_TYPE,
};

enum SliceType {
    B_SLICE = 0,
    P_SLICE = 1,
    I_SLICE = 2,
};

typedef struct slice_ref_map {
    RK_U8 dpb_index;
    RK_U8 is_long_term;
} slice_ref_map_t;

typedef struct ShortTermRPS {
    RK_U32 num_negative_pics;
    RK_S32 num_delta_pocs;
    RK_S32 delta_poc[32];
    RK_U8  used[32];
} ShortTermRPS;

typedef struct LongTermRPS {
    RK_S32  poc[32];
    RK_U8   used[32];
    RK_U8   nb_refs;
} LongTermRPS;

typedef struct RefPicList {
    RK_U32  dpb_index[MAX_REFS];
    RK_U32 nb_refs;
} RefPicList_t;

typedef struct RefPicListTab {
    RefPicList_t refPicList[2];
} RefPicListTab_t;

typedef struct SliceHeader {
    RK_U32 pps_id;

    ///< address (in raster order) of the first block in the current slice segment
    RK_U32   slice_segment_addr;
    ///< address (in raster order) of the first block in the current slice
    RK_U32   slice_addr;

    enum SliceType slice_type;

    RK_S32 pic_order_cnt_lsb;

    RK_U8 first_slice_in_pic_flag;
    RK_U8 dependent_slice_segment_flag;
    RK_U8 pic_output_flag;
    RK_U8 colour_plane_id;

    ///< RPS coded in the slice header itself is stored here
    ShortTermRPS slice_rps;
    const ShortTermRPS *short_term_rps;
    LongTermRPS long_term_rps;
    RK_U32 list_entry_lx[2][32];

    RK_U8 rpl_modification_flag[2];
    RK_U8 no_output_of_prior_pics_flag;
    RK_U8 slice_temporal_mvp_enabled_flag;

    RK_U32 nb_refs[2];

    RK_U8 slice_sample_adaptive_offset_flag[3];
    RK_U8 mvd_l1_zero_flag;

    RK_U8 cabac_init_flag;
    RK_U8 disable_deblocking_filter_flag; ///< slice_header_disable_deblocking_filter_flag
    RK_U8 slice_loop_filter_across_slices_enabled_flag;
    RK_U8 collocated_list;

    RK_U32 collocated_ref_idx;

    RK_S32 slice_qp_delta;
    RK_S32 slice_cb_qp_offset;
    RK_S32 slice_cr_qp_offset;

    RK_S32 beta_offset;    ///< beta_offset_div2 * 2
    RK_S32 tc_offset;      ///< tc_offset_div2 * 2

    RK_U32 max_num_merge_cand; ///< 5 - 5_minus_max_num_merge_cand

    RK_S32 *entry_point_offset;
    RK_S32 * offset;
    RK_S32 * size;
    RK_S32 num_entry_point_offsets;

    RK_S8 slice_qp;

    RK_U8 luma_log2_weight_denom;
    RK_S16 chroma_log2_weight_denom;

    RK_S32 slice_ctb_addr_rs;
} SliceHeader_t;

extern RK_U8 hal_hevc_diag_scan4x4_x[16];
extern RK_U8 hal_hevc_diag_scan4x4_y[16];
extern RK_U8 hal_hevc_diag_scan8x8_x[64];
extern RK_U8 hal_hevc_diag_scan8x8_y[64];
extern RK_U8 cabac_table[27456];

#ifdef __cplusplus
extern "C" {
#endif

RK_U32 hevc_ver_align(RK_U32 val);
RK_U32 hevc_hor_align(RK_U32 val);
void hal_record_scaling_list(scalingFactor_t *pScalingFactor_out, scalingList_t *pScalingList);
RK_S32 hal_h265d_slice_hw_rps(void *dxva, void *rps_buf);
RK_S32 hal_h265d_slice_output_rps(void *dxva, void *rps_buf);
void hal_h265d_output_scalinglist_packet(void *hal, void *ptr, void *dxva);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_H265D_COM_H__*/
