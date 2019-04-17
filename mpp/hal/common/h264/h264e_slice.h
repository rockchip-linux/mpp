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

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_list.h"

/*
 * For H.264 encoder slice header process.
 * Remove some syntax that encoder not supported.
 * Field, mbaff, B slice are not supported yet.
 */

#define MAX_H264E_REFS_CNT      8

typedef struct H264eMmco_t  H264eMmco;
typedef struct H264eSlice_t H264eSlice;

typedef struct H264eMmco_t {
    struct list_head    list;

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
    RK_S32 mmco;
    RK_S32 difference_of_pic_nums_minus1;   // for MMCO 1 & 3
    RK_S32 long_term_pic_num;               // for MMCO 2
    RK_S32 long_term_frame_idx;             // for MMCO 3 & 6
    RK_S32 max_long_term_frame_idx_plus1;   // for MMCO 4
} H264eMmco;

typedef struct H264eSlice_t {
    /* Input parameter before reading */
    RK_U32      max_num_ref_frames;
    RK_U32      entropy_coding_mode;
    RK_S32      log2_max_poc_lsb;

    /* Nal parameters */
    RK_S32      forbidden_bit;
    RK_S32      nal_reference_idc;
    RK_S32      nalu_type;

    /* Unchanged parameters  */
    RK_U32      start_mb_nr;
    RK_U32      slice_type;
    RK_U32      pic_parameter_set_id;
    RK_S32      frame_num;
    RK_S32      num_ref_idx_override;
    RK_S32      qp_delta;
    RK_U32      cabac_init_idc;
    RK_U32      disable_deblocking_filter_idc;
    RK_S32      slice_alpha_c0_offset_div2;
    RK_S32      slice_beta_offset_div2;

    RK_S32      idr_flag;
    RK_U32      idr_pic_id;
    /* for poc mode 0 */
    RK_S32      pic_order_cnt_lsb;
    RK_S32      num_ref_idx_active;
    /* idr mmco flag */
    RK_S32      no_output_of_prior_pics;
    RK_S32      long_term_reference_flag;

    /* Changable parameters  */
    RK_S32      adaptive_ref_pic_buffering;

    /* reorder parameter */
    RK_S32      ref_pic_list_modification_flag;
    RK_S32      modification_of_pic_nums_idc;
    RK_S32      abs_diff_pic_num_minus1;
    RK_S32      long_term_pic_idx;
    RK_S32      abs_diff_view_idx_minus1;

    /* mmco parameter */
    struct list_head    mmco;
    RK_S32              mmco_cnt;

    /* slice modification infomation */
    void *      hw_buf;
    void *      sw_buf;
    RK_S32      hw_length;
    RK_S32      sw_length;
} H264eSlice;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 h264e_slice_read(H264eSlice *slice, void *p, RK_S32 size);
RK_S32 h264e_slice_write(H264eSlice *slice, void *p, RK_U32 size);
RK_S32 h264e_slice_move(RK_U8 *dst, RK_U8 *src, RK_S32 dst_bit, RK_S32 src_bit,
                        RK_S32 src_size);
MPP_RET h264e_slice_update(H264eSlice *slice, void *p);

H264eMmco *h264e_slice_gen_mmco(RK_S32 id, RK_S32 arg0, RK_S32 arg1);
MPP_RET h264e_slice_add_mmco(H264eSlice *slice, H264eMmco *mmco);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_SLICE_H__ */
