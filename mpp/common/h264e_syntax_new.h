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

#ifndef __H264E_SYNTAX_NEW_H__
#define __H264E_SYNTAX_NEW_H__

#include "h264_syntax.h"
#include "mpp_rc_api.h"

typedef enum H264eSyntaxType_e {
    H264E_SYN_CFG,
    H264E_SYN_SPS,
    H264E_SYN_PPS,
    H264E_SYN_DPB,
    H264E_SYN_SLICE,
    H264E_SYN_FRAME,
    H264E_SYN_RC,
    H264E_SYN_ROI,
    H264E_SYN_OSD,
    H264E_SYN_RC_RET,
    H264E_SYN_BUTT,
} H264eSyntaxType;

#define SYN_TYPE_FLAG(type)         (1 << (type))

typedef struct H264eSyntaxDesc_t {
    H264eSyntaxType     type;
    void                *p;
} H264eSyntaxDesc;

typedef struct SynH264eVui_t {
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
    RK_S32      transfer;
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
} SynH264eVui;

typedef struct SynH264eSps_t {
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

    SynH264eVui    vui;
} SynH264eSps;

typedef struct SynH264ePps_t {
    RK_S32      pps_id;
    RK_S32      sps_id;

    // 0 - CAVLC 1 - CABAC
    RK_S32      entropy_coding_mode;

    RK_S32      bottom_field_pic_order_in_frame_present;
    RK_S32      num_slice_groups;

    RK_S32      num_ref_idx_l0_default_active;
    RK_S32      num_ref_idx_l1_default_active;

    RK_S32      weighted_pred;
    RK_S32      weighted_bipred_idc;

    RK_S32      pic_init_qp;
    RK_S32      pic_init_qs;

    RK_S32      chroma_qp_index_offset;
    RK_S32      second_chroma_qp_index_offset;

    RK_S32      deblocking_filter_control;
    RK_S32      constrained_intra_pred;
    RK_S32      redundant_pic_cnt;

    RK_S32      transform_8x8_mode;

    // Only support flat and default scaling list
    RK_S32      pic_scaling_matrix_present;
    RK_S32      use_default_scaling_matrix[H264_SCALING_MATRIX_TYPE_BUTT];
} SynH264ePps;

/*
 * For H.264 encoder slice header process.
 * Remove some syntax that encoder not supported.
 * Field, mbaff, B slice are not supported yet.
 *
 * The reorder and mmco syntax will be create by dpb and attach to slice.
 * Then slice syntax will be passed to hal to config hardware or modify the
 * hardware stream.
 */

#define H264E_MAX_REFS_CNT          16

/* reference picture list modification operation */
typedef struct H264eRplmo_t {
    RK_S32      modification_of_pic_nums_idc;
    RK_S32      abs_diff_pic_num_minus1;
    RK_S32      long_term_pic_idx;
    RK_S32      abs_diff_view_idx_minus1;
} H264eRplmo;

typedef struct H264eReorderInfo_t {
    RK_S32      pos_rd;
    RK_S32      pos_wr;
    RK_S32      size;
    H264eRplmo  ops[H264E_MAX_REFS_CNT];
} H264eReorderInfo;

/*
 * mmco (Memory management control operation) value
 * 0 - End memory_management_control_operation syntax element loop
 * 1 - Mark a short-term reference picture as "unused for reference"
 * 2 - Mark a long-term reference picture as "unused for reference"
 * 3 - Mark a short-term reference picture as "used for long-term
 *     reference" and assign a long-term frame index to it
 * 4 - Specify the maximum long-term frame index and mark all long-term
 *     reference pictures having long-term frame indices greater than
 *     the maximum value as "unused for reference"
 * 5 - Mark all reference pictures as "unused for reference" and set the
 *     MaxLongTermFrameIdx variable to "no long-term frame indices"
 * 6 - Mark the current picture as "used for long-term reference" and
 *     assign a long-term frame index to it
 */
typedef struct H264eMmco_t {
    RK_S32      mmco;
    RK_S32      difference_of_pic_nums_minus1;   // for MMCO 1 & 3
    RK_S32      long_term_pic_num;               // for MMCO 2
    RK_S32      long_term_frame_idx;             // for MMCO 3 & 6
    RK_S32      max_long_term_frame_idx_plus1;   // for MMCO 4
} H264eMmco;

#define MAX_H264E_MMCO_CNT      8

typedef struct H264eMarkingInfo_t {
    RK_S32      idr_flag;
    /* idr marking flag */
    RK_S32      no_output_of_prior_pics;
    RK_S32      long_term_reference_flag;
    /* non-idr marking flag */
    RK_S32      adaptive_ref_pic_buffering;
    RK_S32      pos_rd;
    RK_S32      pos_wr;
    RK_S32      count;
    RK_S32      size;
    H264eMmco   ops[MAX_H264E_MMCO_CNT];
} H264eMarkingInfo;

typedef struct H264eSlice_t {
    /* Copy of sps/pps parameter */
    RK_U32      max_num_ref_frames;
    RK_U32      entropy_coding_mode;
    RK_S32      log2_max_frame_num;
    RK_S32      log2_max_poc_lsb;

    /* Nal parameters */
    RK_S32      nal_reference_idc;
    RK_S32      nalu_type;

    /* Unchanged parameters  */
    RK_U32      first_mb_in_slice;
    RK_U32      slice_type;
    RK_U32      pic_parameter_set_id;
    RK_S32      frame_num;
    RK_S32      num_ref_idx_override;
    RK_S32      qp_delta;
    RK_U32      cabac_init_idc;
    RK_U32      disable_deblocking_filter_idc;
    RK_S32      slice_alpha_c0_offset_div2;
    RK_S32      slice_beta_offset_div2;

    /* reorder parameter */
    RK_S32      ref_pic_list_modification_flag;
    H264eReorderInfo    *reorder;
    H264eMarkingInfo    *marking;

    RK_S32      idr_flag;
    RK_U32      idr_pic_id;
    RK_U32      next_idr_pic_id;
    /* for poc mode 0 */
    RK_S32      pic_order_cnt_lsb;
    RK_S32      num_ref_idx_active;
    /* idr mmco flag */
    RK_S32      no_output_of_prior_pics;
    RK_S32      long_term_reference_flag;

    /* Changable parameters  */
    RK_S32      adaptive_ref_pic_buffering;
} H264eSlice;

/*
 * Split reference frame configure to two parts
 * The first part is slice depended info like poc / frame_num, and frame
 * type and flags.
 * The other part is gop structure depended info like gop index, ref_status
 * and ref_frm_index. This part is inited from dpb gop hierarchy info.
 */
typedef struct H264eFrmInfo_s {
    RK_S32              seq_idx;

    RK_S32              curr_idx;
    RK_S32              refr_idx;

    // current frame rate control and dpb status info
    MppRcHalCfg         rc_cfg;
    EncFrmStatus        status;

    RK_S32              usage[H264E_MAX_REFS_CNT + 1];
} H264eFrmInfo;

/* macro to add syntax to description array */
#define H264E_ADD_SYNTAX(desc, number, syn_type, syn_ptr) \
    do { \
        desc[number].type  = syn_type; \
        desc[number].p     = syn_ptr; \
        number++; \
    } while (0)

#endif /* __H264E_SYNTAX_NEW_H__ */
