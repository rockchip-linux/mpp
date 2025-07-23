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

#ifndef __H265E_SYNTAX_NEW_H__
#define __H265E_SYNTAX_NEW_H__
#include "mpp_rc.h"
#include "h265_syntax.h"
#include "rc.h"

typedef struct H265PicEntry_t {
    RK_U8 slot_idx;
} H265ePicEntry;

typedef struct H265ePicParams_t {
    RK_U16      pic_width;
    RK_U16      pic_height;
    RK_U16      hor_stride;
    RK_U16      ver_stride;
    RK_U16      pic_ofsty;
    RK_U16      pic_oftx;
    RK_U32      mpp_format;
    union {
        struct {
            RK_U16  chroma_format_idc                       : 2;
            RK_U16  separate_colour_plane_flag              : 1;
            RK_U16  bit_depth_luma_minus8                   : 3;
            RK_U16  bit_depth_chroma_minus8                 : 3;
            RK_U16  log2_max_pic_order_cnt_lsb_minus4       : 4;
            RK_U16  NoPicReorderingFlag                     : 1;
            RK_U16  NoBiPredFlag                            : 1;
            RK_U16  ReservedBits1                           : 1;
        };
        RK_U16 wFormatAndSequenceInfoFlags;
    };
    RK_U8   sps_max_dec_pic_buffering_minus1;
    RK_U8   log2_min_luma_coding_block_size_minus3;
    RK_U8   log2_diff_max_min_luma_coding_block_size;
    RK_U8   log2_min_transform_block_size_minus2;
    RK_U8   log2_diff_max_min_transform_block_size;
    RK_U8   max_transform_hierarchy_depth_inter;
    RK_U8   max_transform_hierarchy_depth_intra;
    RK_U8   num_short_term_ref_pic_sets;
    RK_U8   num_long_term_ref_pics_sps;
    RK_U8   num_ref_idx_l0_default_active_minus1;
    RK_U8   num_ref_idx_l1_default_active_minus1;
    RK_S8   init_qp_minus26;
    RK_U16  ReservedBits2;

    union {
        struct {
            RK_U32  scaling_list_enabled_flag                    : 1;
            RK_U32  amp_enabled_flag                            : 1;
            RK_U32  sample_adaptive_offset_enabled_flag         : 1;
            RK_U32  pcm_enabled_flag                            : 1;
            RK_U32  pcm_sample_bit_depth_luma_minus1            : 4;
            RK_U32  pcm_sample_bit_depth_chroma_minus1          : 4;
            RK_U32  log2_min_pcm_luma_coding_block_size_minus3  : 2;
            RK_U32  log2_diff_max_min_pcm_luma_coding_block_size : 2;
            RK_U32  pcm_loop_filter_disabled_flag                : 1;
            RK_U32  long_term_ref_pics_present_flag             : 1;
            RK_U32  sps_temporal_mvp_enabled_flag               : 1;
            RK_U32  strong_intra_smoothing_enabled_flag         : 1;
            RK_U32  dependent_slice_segments_enabled_flag       : 1;
            RK_U32  output_flag_present_flag                    : 1;
            RK_U32  num_extra_slice_header_bits                 : 3;
            RK_U32  sign_data_hiding_enabled_flag               : 1;
            RK_U32  cabac_init_present_flag                     : 1;
            RK_U32  ReservedBits3                               : 5;
        };
        RK_U32 CodingParamToolFlags;
    };

    union {
        struct {
            RK_U32  constrained_intra_pred_flag                 : 1;
            RK_U32  transform_skip_enabled_flag                 : 1;
            RK_U32  cu_qp_delta_enabled_flag                    : 1;
            RK_U32  pps_slice_chroma_qp_offsets_present_flag    : 1;
            RK_U32  weighted_pred_flag                          : 1;
            RK_U32  weighted_bipred_flag                        : 1;
            RK_U32  transquant_bypass_enabled_flag              : 1;
            RK_U32  tiles_enabled_flag                          : 1;
            RK_U32  entropy_coding_sync_enabled_flag            : 1;
            RK_U32  uniform_spacing_flag                        : 1;
            RK_U32  loop_filter_across_tiles_enabled_flag       : 1;
            RK_U32  pps_loop_filter_across_slices_enabled_flag  : 1;
            RK_U32  deblocking_filter_override_enabled_flag     : 1;
            RK_U32  pps_deblocking_filter_disabled_flag         : 1;
            RK_U32  lists_modification_present_flag             : 1;
            RK_U32  slice_segment_header_extension_present_flag : 1;
            RK_U32  ReservedBits4                               : 16;
        };
        RK_U32 CodingSettingPicturePropertyFlags;
    };
    RK_S8  pps_cb_qp_offset;
    RK_S8  pps_cr_qp_offset;
    RK_U8   num_tile_columns_minus1;
    RK_U8   num_tile_rows_minus1;
    RK_S32  column_width_minus1[19];
    RK_S32  row_height_minus1[21];
    RK_U8  diff_cu_qp_delta_depth;
    RK_S8  pps_beta_offset_div2;
    RK_S8  pps_tc_offset_div2;
    RK_U8  log2_parallel_merge_level_minus2;
    RK_U32 vps_id;
    RK_U32 pps_id;
    RK_U32 sps_id;
    RK_U8 scaling_list_data_present_flag;
} H265ePicParams;

typedef struct H265eSlicParams_t {
    union {
        struct {
            RK_U32 sli_splt               : 1;
            RK_U32 sli_splt_mode          : 1;
            RK_U32 sli_splt_cpst          : 1;
            RK_U32 sli_flsh               : 1;
            RK_U32 cbc_init_flg          : 1;
            RK_U32 mvd_l1_zero_flg       : 1;
            RK_U32 merge_up_flag         : 1;
            RK_U32 merge_left_flag       : 1;
            RK_U32 ref_pic_lst_mdf_l0    : 1;
            RK_U32 num_refidx_act_ovrd   : 1;
            RK_U32 sli_sao_chrm_flg      : 1;
            RK_U32 sli_sao_luma_flg      : 1;
            RK_U32 sli_tmprl_mvp_en      : 1;
            RK_U32 pic_out_flg           : 1;
            RK_U32 dpdnt_sli_seg_flg     : 1;
            RK_U32 no_out_pri_pic        : 1;
            RK_U32 sli_lp_fltr_acrs_sli  : 1;
            RK_U32 sli_dblk_fltr_dis     : 1;
            RK_U32 dblk_fltr_ovrd_flg    : 1;
            RK_U32 col_ref_idx           : 1;
            RK_U32 col_frm_l0_flg        : 1;
            RK_U32 st_ref_pic_flg        : 1;
            RK_U32 num_pos_pic           : 1;
            RK_U32 dlt_poc_msb_prsnt0    : 1;
            RK_U32 dlt_poc_msb_prsnt1    : 1;
            RK_U32 dlt_poc_msb_prsnt2    : 1;
            RK_U32 used_by_lt_flg0       : 1;
            RK_U32 used_by_lt_flg1       : 1;
            RK_U32 used_by_lt_flg2       : 1;
            RK_U32 ReservedBits          : 3;
        };
        RK_U32 CodingSliceFlags;
    };

    H265ePicEntry  recon_pic;
    H265ePicEntry  ref_pic;
    RK_S8 sli_tc_ofst_div2;
    RK_S8 sli_beta_ofst_div2;
    RK_S8 sli_cb_qp_ofst;
    RK_U8 sli_qp;
    RK_U8 max_mrg_cnd;
    RK_U8 lst_entry_l0;
    RK_U8 num_refidx_l1_act;
    RK_U8 num_refidx_l0_act;
    RK_U8 slice_type;
    RK_U8 slice_rsrv_flg;
    RK_U8 sli_pps_id;
    RK_U8 lt_idx_sps;
    RK_U8 num_lt_pic;
    RK_U8 st_ref_pic_idx;
    RK_U8 num_lt_sps;
    RK_U8 used_by_s0_flg;
    RK_U8 num_neg_pic;
    RK_U16 sli_poc_lsb;
    RK_U16 sli_hdr_ext_len;
    RK_U16 poc_lsb_lt0;
    RK_U16 sli_max_num_m1;
    RK_U16 sli_splt_cnum_m1;
    RK_U16 dlt_poc_msb_cycl0;
    RK_U16 dlt_poc_s0_m10;
    RK_U16 dlt_poc_s0_m11;
    RK_U16 dlt_poc_s0_m12;
    RK_U16 dlt_poc_s0_m13;
    RK_U16 poc_lsb_lt1;
    RK_U16 poc_lsb_lt2;
    RK_U16 dlt_poc_msb_cycl1;
    RK_U16 dlt_poc_msb_cycl2;
    RK_U32 sli_splt_byte;
    RK_U32 tot_poc_num;
    RK_U32 non_reference_flag;
    RK_S32 temporal_id;
} H265eSlicParams;
/*
 * Split reference frame configure to two parts
 * The first part is slice depended info like poc / frame_num, and frame
 * type and flags.
 * The other part is gop structure depended info like gop index, ref_status
 * and ref_frm_index. This part is inited from dpb gop hierarchy info.
 */

typedef struct UserDatas_t {
    void *plt_data;
} UserDatas;

typedef struct H265eSyntax_new_t {
    RK_S32          idr_request;
    H265ePicParams  pp;
    H265eSlicParams sp;
    void            *dpb;
} H265eSyntax_new;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 h265e_syntax_fill(void *ctx);
RK_S32 h265e_get_nal_type(H265eSlicParams* sp, RK_S32 frame_type);

#ifdef __cplusplus
}
#endif

#endif
