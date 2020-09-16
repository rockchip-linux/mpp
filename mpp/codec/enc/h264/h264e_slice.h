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

#ifndef __H264E_SLICE_H__
#define __H264E_SLICE_H__

#include "mpp_enc_cfg.h"

#include "h264e_syntax.h"
#include "h264e_sps.h"
#include "h264e_pps.h"

/*
 * For H.264 encoder slice header process.
 * Remove some syntax that encoder not supported.
 * Field, mbaff, B slice are not supported yet.
 *
 * The reorder and mmco syntax will be create by dpb and attach to slice.
 * Then slice syntax will be passed to hal to config hardware or modify the
 * hardware stream.
 */

/* reference picture list modification operation */
typedef struct H264eRplmo_t {
    RK_S32      modification_of_pic_nums_idc;
    RK_S32      abs_diff_pic_num_minus1;
    RK_S32      long_term_pic_idx;
    RK_S32      abs_diff_view_idx_minus1;
} H264eRplmo;

typedef struct H264eReorderInfo_t {
    RK_S32      rd_cnt;
    RK_S32      wr_cnt;
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
    RK_S32      rd_cnt;
    RK_S32      wr_cnt;
    RK_S32      size;
    H264eMmco   ops[MAX_H264E_MMCO_CNT];
} H264eMarkingInfo;

typedef struct H264eSlice_t {
    /* Copy of sps/pps parameter */
    RK_S32      mb_w;
    RK_S32      mb_h;
    RK_U32      max_num_ref_frames;
    RK_U32      entropy_coding_mode;
    RK_S32      log2_max_frame_num;
    RK_S32      log2_max_poc_lsb;
    RK_S32      pic_order_cnt_type;
    RK_S32      qp_init;

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
    RK_U32      pic_order_cnt_lsb;
    RK_S32      num_ref_idx_active;
    /* idr mmco flag */
    RK_S32      no_output_of_prior_pics;
    RK_S32      long_term_reference_flag;

    /* Changable parameters  */
    RK_S32      adaptive_ref_pic_buffering;

    /* for multi-slice writing */
    RK_S32      is_multi_slice;
} H264eSlice;

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * reorder context for both dpb and slice
 */
MPP_RET h264e_reorder_init(H264eReorderInfo *reorder);
MPP_RET h264e_reorder_wr_rewind(H264eReorderInfo *info);
MPP_RET h264e_reorder_rd_rewind(H264eReorderInfo *info);
MPP_RET h264e_reorder_wr_op(H264eReorderInfo *info, H264eRplmo *op);
MPP_RET h264e_reorder_rd_op(H264eReorderInfo *info, H264eRplmo *op);

/* mmco context for both dpb and slice */
MPP_RET h264e_marking_init(H264eMarkingInfo *marking);
RK_S32 h264e_marking_is_empty(H264eMarkingInfo *info);
MPP_RET h264e_marking_wr_rewind(H264eMarkingInfo *marking);
MPP_RET h264e_marking_rd_rewind(H264eMarkingInfo *marking);
MPP_RET h264e_marking_wr_op(H264eMarkingInfo *info, H264eMmco *op);
MPP_RET h264e_marking_rd_op(H264eMarkingInfo *info, H264eMmco *op);

/*
 * h264e_slice_update is called only on cfg is update.
 * When cfg has no changes just use slice next to setup
 */
void h264e_slice_init(H264eSlice *slice, H264eReorderInfo *reorder,
                      H264eMarkingInfo *marking);
RK_S32 h264e_slice_update(H264eSlice *slice, MppEncCfgSet *cfg,
                          H264eSps *sps, H264ePps *pps,
                          H264eDpbFrm *frm);

RK_S32 h264e_slice_read(H264eSlice *slice, void *p, RK_S32 size);
RK_S32 h264e_slice_write(H264eSlice *slice, void *p, RK_U32 size);
RK_S32 h264e_slice_write_pskip(H264eSlice *slice, void *p, RK_U32 size);
RK_S32 h264e_slice_move(RK_U8 *dst, RK_U8 *src, RK_S32 dst_bit, RK_S32 src_bit,
                        RK_S32 src_size);

RK_S32 h264e_slice_write_prefix_nal_unit_svc(H264ePrefixNal *nal, void *p, RK_S32 size);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_SLICE_H__ */
