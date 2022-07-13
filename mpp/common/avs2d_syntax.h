/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __AVS2D_SYNTAX_H__
#define __AVS2D_SYNTAX_H__

#include "rk_type.h"

//!< avs2 decoder picture parameters structure
typedef struct _DXVA_PicParams_AVS2 {
    //!< sequence header
    RK_U32      chroma_format_idc : 2;
    RK_U32      pic_width_in_luma_samples : 16;
    RK_U32      pic_height_in_luma_samples : 16;
    RK_U32      bit_depth_luma_minus8 : 3;
    RK_U32      bit_depth_chroma_minus8 : 3;
    RK_U32      lcu_size : 3;
    RK_U32      progressive_sequence : 1;
    RK_U32      field_coded_sequence : 1;
    RK_U32      multi_hypothesis_skip_enable_flag : 1;
    RK_U32      dual_hypothesis_prediction_enable_flag : 1;
    RK_U32      weighted_skip_enable_flag : 1;
    RK_U32      asymmetrc_motion_partitions_enable_flag : 1;
    RK_U32      nonsquare_quadtree_transform_enable_flag : 1;
    RK_U32      nonsquare_intra_prediction_enable_flag : 1;
    RK_U32      secondary_transform_enable_flag : 1;
    RK_U32      sample_adaptive_offset_enable_flag : 1;
    RK_U32      adaptive_loop_filter_enable_flag : 1;
    RK_U32      pmvr_enable_flag : 1;
    RK_U32      cross_slice_loopfilter_enable_flag : 1;
    //!< picture header
    RK_U32      picture_type : 3;
    RK_U32      scene_reference_enable_flag : 1;
    RK_U32      bottom_field_picture_flag : 1;
    RK_U32      fixed_picture_qp : 1;
    RK_U32      picture_qp : 7;
    RK_U32      loop_filter_disable_flag : 1;
    RK_S8       alpha_c_offset;                 //!< 5bits signed
    RK_S8       beta_offset;                    //!< 5bits signed
} PicParams_Avs2d, *LP_PicParams_Avs2d;

typedef struct _DXVA_RefParams_AVS2 {
    RK_U8       ref_pic_num;
    RK_S32      ref_poc_list[32];
} RefParams_Avs2d, *LP_RefParams_Avs2d;

typedef struct _DXVA_AlfParams_AVS2 {
    RK_U8       enable_pic_alf_y;       //!< 1bits
    RK_U8       enable_pic_alf_cb;      //!< 1bits
    RK_U8       enable_pic_alf_cr;      //!< 1bits
    RK_U8       alf_filter_num_minus1;  //!< 4bits
    RK_U8       alf_coeff_idx_tab[16];
    RK_S32      alf_coeff_y[16][9];
    RK_S32      alf_coeff_cb[9];
    RK_S32      alf_coeff_cr[9];
} AlfParams_Avs2d, *LP_AlfParams_Avs2d;

typedef struct _DXVA_WqmParams_AVS2 {
    RK_U8               pic_weight_quant_enable_flag;
    RK_S8               chroma_quant_param_delta_cb;    //!< 6bits
    RK_S8               chroma_quant_param_delta_cr;    //!< 6bits
    RK_U32              wq_matrix[2][64];
} WqmParams_Avs2d, *LP_WqmParams_Avs2d;

typedef struct avs2d_syntax_t {
    PicParams_Avs2d     pp;
    RefParams_Avs2d     refp;
    AlfParams_Avs2d     alfp;
    WqmParams_Avs2d     wqmp;
    RK_U8              *bitstream;
    RK_U32              bitstream_size;
} Avs2dSyntax_t;

#endif /*__AVS2D_SYNTAX_H__*/
