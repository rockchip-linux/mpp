/*
 *
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

/*
 * @file        h265d_parser.h
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */


#ifndef __H265D_PARSER_H__
#define __H265D_PARSER_H__

#include <limits.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_dec.h"
#include "mpp_bitread.h"

#include "h265d_codec.h"

extern RK_U32 h265d_debug;

#define H265D_DBG_FUNCTION          (0x00000001)
#define H265D_DBG_VPS               (0x00000002)
#define H265D_DBG_SPS               (0x00000004)
#define H265D_DBG_PPS               (0x00000008)
#define H265D_DBG_SLICE_HDR         (0x00000010)
#define H265D_DBG_SEI               (0x00000020)
#define H265D_DBG_GLOBAL            (0x00000040)
#define H265D_DBG_REF               (0x00000080)
#define H265D_DBG_TIME              (0x00000100)


#define h265d_dbg(flag, fmt, ...) _mpp_dbg(h265d_debug, flag, fmt, ## __VA_ARGS__)

#define MAX_WIDTH    (4096)
#define MAX_HEIGHT   (2304)

#define MAX_DPB_SIZE 17 // A.4.1
#define MAX_REFS 16

/**
 * 7.4.2.1
 */
#define MAX_SUB_LAYERS 7
#define MAX_VPS_COUNT 16
#define MAX_SPS_COUNT 16
#define MAX_PPS_COUNT 64
#define MAX_SHORT_TERM_RPS_COUNT 64
#define MAX_CU_SIZE 128

//TODO: check if this is really the maximum
#define MAX_TRANSFORM_DEPTH 5

#define MAX_TB_SIZE 32
#define MAX_PB_SIZE 64
#define MAX_LOG2_CTB_SIZE 6
#define MAX_QP 51
#define DEFAULT_INTRA_TC_OFFSET 2

#define HEVC_CONTEXTS 183

#define MRG_MAX_NUM_CANDS     5

#define L0 0
#define L1 1

#define EPEL_EXTRA_BEFORE 1
#define EPEL_EXTRA_AFTER  2
#define EPEL_EXTRA        3
#define QPEL_EXTRA_BEFORE 3
#define QPEL_EXTRA_AFTER  4
#define QPEL_EXTRA        7

#define EDGE_EMU_BUFFER_STRIDE 80

#define MAX_FRAME_SIZE 2048000
#define MPP_INPUT_BUFFER_PADDING_SIZE 8

#define MPP_PROFILE_HEVC_MAIN                        1
#define MPP_PROFILE_HEVC_MAIN_10                     2
#define MPP_PROFILE_HEVC_MAIN_STILL_PICTURE          3


/**
 * Value of the luma sample at position (x, y) in the 2D array tab.
 */
#define IS_IDR(s) (s->nal_unit_type == NAL_IDR_W_RADL || s->nal_unit_type == NAL_IDR_N_LP)
#define IS_BLA(s) (s->nal_unit_type == NAL_BLA_W_RADL || s->nal_unit_type == NAL_BLA_W_LP || \
                   s->nal_unit_type == NAL_BLA_N_LP)
#define IS_IRAP(s) (s->nal_unit_type >= 16 && s->nal_unit_type <= 23)

/**
 * Table 7-3: NAL unit type codes
 */
enum NALUnitType {
    NAL_TRAIL_N    = 0,
    NAL_TRAIL_R    = 1,
    NAL_TSA_N      = 2,
    NAL_TSA_R      = 3,
    NAL_STSA_N     = 4,
    NAL_STSA_R     = 5,
    NAL_RADL_N     = 6,
    NAL_RADL_R     = 7,
    NAL_RASL_N     = 8,
    NAL_RASL_R     = 9,
    NAL_BLA_W_LP   = 16,
    NAL_BLA_W_RADL = 17,
    NAL_BLA_N_LP   = 18,
    NAL_IDR_W_RADL = 19,
    NAL_IDR_N_LP   = 20,
    NAL_CRA_NUT    = 21,
    NAL_VPS        = 32,
    NAL_SPS        = 33,
    NAL_PPS        = 34,
    NAL_AUD        = 35,
    NAL_EOS_NUT    = 36,
    NAL_EOB_NUT    = 37,
    NAL_FD_NUT     = 38,
    NAL_SEI_PREFIX = 39,
    NAL_SEI_SUFFIX = 40,
};

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

typedef struct ShortTermRPS {
    RK_U32 num_negative_pics;
    RK_S32 num_delta_pocs;
    RK_S32 rps_idx_num_delta_pocs;
    RK_S32 delta_poc[32];
    RK_U8  used[32];
} ShortTermRPS;

typedef struct LongTermRPS {
    RK_S32  poc[32];
    RK_U8   used[32];
    RK_U8   nb_refs;
} LongTermRPS;

typedef struct RefPicList {
    struct HEVCFrame *ref[MAX_REFS];
    RK_S32 list[MAX_REFS];
    RK_S32 isLongTerm[MAX_REFS];
    RK_S32 nb_refs;
} RefPicList;

typedef struct RefPicListTab {
    RefPicList refPicList[2];
} RefPicListTab;

typedef struct HEVCWindow {
    RK_S32 left_offset;
    RK_S32 right_offset;
    RK_S32 top_offset;
    RK_S32 bottom_offset;
} HEVCWindow;

typedef struct VUI {
    MppRational_t sar;

    RK_S32 overscan_info_present_flag;
    RK_S32 overscan_appropriate_flag;

    RK_S32 video_signal_type_present_flag;
    RK_S32 video_format;
    RK_S32 video_full_range_flag;
    RK_S32 colour_description_present_flag;
    RK_U8  colour_primaries;
    RK_U8  transfer_characteristic;
    RK_U8  matrix_coeffs;

    RK_S32 chroma_loc_info_present_flag;
    RK_S32 chroma_sample_loc_type_top_field;
    RK_S32 chroma_sample_loc_type_bottom_field;
    RK_S32 neutra_chroma_indication_flag;

    RK_S32 field_seq_flag;
    RK_S32 frame_field_info_present_flag;

    RK_S32 default_display_window_flag;
    HEVCWindow def_disp_win;

    RK_S32 vui_timing_info_present_flag;
    RK_U32 vui_num_units_in_tick;
    RK_U32 vui_time_scale;
    RK_S32 vui_poc_proportional_to_timing_flag;
    RK_S32 vui_num_ticks_poc_diff_one_minus1;
    RK_S32 vui_hrd_parameters_present_flag;

    RK_S32 bitstream_restriction_flag;
    RK_S32 tiles_fixed_structure_flag;
    RK_S32 motion_vectors_over_pic_boundaries_flag;
    RK_S32 restricted_ref_pic_lists_flag;
    RK_S32 min_spatial_segmentation_idc;
    RK_S32 max_bytes_per_pic_denom;
    RK_S32 max_bits_per_min_cu_denom;
    RK_S32 log2_max_mv_length_horizontal;
    RK_S32 log2_max_mv_length_vertical;
} VUI;

typedef struct PTLCommon {
    RK_U8 profile_space;
    RK_U8 tier_flag;
    RK_U8 profile_idc;
    RK_U8 profile_compatibility_flag[32];
    RK_U8 level_idc;
    RK_U8 progressive_source_flag;
    RK_U8 interlaced_source_flag;
    RK_U8 non_packed_constraint_flag;
    RK_U8 frame_only_constraint_flag;
} PTLCommon;

typedef struct PTL {
    PTLCommon general_ptl;
    PTLCommon sub_layer_ptl[MAX_SUB_LAYERS];

    RK_U8  sub_layer_profile_present_flag[MAX_SUB_LAYERS];
    RK_U8  sub_layer_level_present_flag[MAX_SUB_LAYERS];

    RK_S32 sub_layer_profile_space[MAX_SUB_LAYERS];
    RK_U8  sub_layer_tier_flag[MAX_SUB_LAYERS];
    RK_S32 sub_layer_profile_idc[MAX_SUB_LAYERS];
    RK_U8  sub_layer_profile_compatibility_flags[MAX_SUB_LAYERS][32];
    RK_S32 sub_layer_level_idc[MAX_SUB_LAYERS];
} PTL;

typedef struct HEVCVPS {
    RK_U8  vps_temporal_id_nesting_flag;
    RK_S32 vps_max_layers;
    RK_S32 vps_max_sub_layers; ///< vps_max_temporal_layers_minus1 + 1

    PTL     ptl;
    RK_S32 vps_sub_layer_ordering_info_present_flag;
    RK_U32 vps_max_dec_pic_buffering[MAX_SUB_LAYERS];
    RK_U32 vps_num_reorder_pics[MAX_SUB_LAYERS];
    RK_U32 vps_max_latency_increase[MAX_SUB_LAYERS];
    RK_S32 vps_max_layer_id;
    RK_S32 vps_num_layer_sets; ///< vps_num_layer_sets_minus1 + 1
    RK_U8  vps_timing_info_present_flag;
    RK_U32 vps_num_units_in_tick;
    RK_U32 vps_time_scale;
    RK_U8  vps_poc_proportional_to_timing_flag;
    RK_S32 vps_num_ticks_poc_diff_one; ///< vps_num_ticks_poc_diff_one_minus1 + 1
    RK_S32 vps_num_hrd_parameters;

    RK_S32 vps_extension_flag;

} HEVCVPS;

typedef struct ScalingList {
    /* This is a little wasteful, since sizeID 0 only needs 8 coeffs,
     * and size ID 3 only has 2 arrays, not 6. */
    RK_U8 sl[4][6][64];
    RK_U8 sl_dc[2][6];
} ScalingList;

typedef struct HEVCSPS {
    RK_U32 vps_id;
    RK_S32 sps_id;
    RK_S32 chroma_format_idc;
    RK_U8 separate_colour_plane_flag;

    ///< output (i.e. cropped) values
    RK_S32 output_width, output_height;
    HEVCWindow output_window;

    HEVCWindow pic_conf_win;

    RK_S32 bit_depth;
    RK_S32 bit_depth_chroma;///<- zrh add
    RK_S32 pixel_shift;
    RK_S32 pix_fmt;

    RK_U32 log2_max_poc_lsb;
    RK_S32 pcm_enabled_flag;

    RK_S32 max_sub_layers;
    struct {
        int max_dec_pic_buffering;
        int num_reorder_pics;
        int max_latency_increase;
    } temporal_layer[MAX_SUB_LAYERS];

    VUI vui;
    PTL ptl;

    RK_U8 scaling_list_enable_flag;
    ScalingList scaling_list;

    RK_U32 nb_st_rps;
    ShortTermRPS st_rps[MAX_SHORT_TERM_RPS_COUNT];

    RK_U8 amp_enabled_flag;
    RK_U8 sao_enabled;

    RK_U8 long_term_ref_pics_present_flag;
    RK_U16 lt_ref_pic_poc_lsb_sps[32];
    RK_U8 used_by_curr_pic_lt_sps_flag[32];
    RK_U8 num_long_term_ref_pics_sps;

    struct {
        RK_U8  bit_depth;
        RK_U8  bit_depth_chroma;
        RK_U32 log2_min_pcm_cb_size;
        RK_U32 log2_max_pcm_cb_size;
        RK_U8  loop_filter_disable_flag;
    } pcm;
    RK_U8 sps_temporal_mvp_enabled_flag;
    RK_U8 sps_strong_intra_smoothing_enable_flag;

    RK_U32 log2_min_cb_size;
    RK_U32 log2_diff_max_min_coding_block_size;
    RK_U32 log2_min_tb_size;
    RK_U32 log2_max_trafo_size;
    RK_S32 log2_ctb_size;
    RK_U32 log2_min_pu_size;

    RK_S32 max_transform_hierarchy_depth_inter;
    RK_S32 max_transform_hierarchy_depth_intra;

    ///< coded frame dimension in various units
    RK_S32 width;
    RK_S32 height;
    RK_S32 ctb_width;
    RK_S32 ctb_height;
    RK_S32 ctb_size;
    RK_S32 min_cb_width;
    RK_S32 min_cb_height;
    RK_S32 min_tb_width;
    RK_S32 min_tb_height;
    RK_S32 min_pu_width;
    RK_S32 min_pu_height;

    RK_S32 hshift[3];
    RK_S32 vshift[3];

    RK_S32 qp_bd_offset;
#ifdef SCALED_REF_LAYER_OFFSETS
    HEVCWindow      scaled_ref_layer_window;
#endif
#ifdef REF_IDX_MFM
    RK_S32 set_mfm_enabled_flag;
#endif
} HEVCSPS;

typedef struct HEVCPPS {
    RK_S32 sps_id;
    RK_S32 pps_id;

    RK_U8 sign_data_hiding_flag;

    RK_U8 cabac_init_present_flag;

    RK_S32 num_ref_idx_l0_default_active; ///< num_ref_idx_l0_default_active_minus1 + 1
    RK_S32 num_ref_idx_l1_default_active; ///< num_ref_idx_l1_default_active_minus1 + 1
    RK_S32 pic_init_qp_minus26;

    RK_U8 constrained_intra_pred_flag;
    RK_U8 transform_skip_enabled_flag;

    RK_U8 cu_qp_delta_enabled_flag;
    RK_S32 diff_cu_qp_delta_depth;

    RK_S32 cb_qp_offset;
    RK_S32 cr_qp_offset;
    RK_U8 pic_slice_level_chroma_qp_offsets_present_flag;
    RK_U8 weighted_pred_flag;
    RK_U8 weighted_bipred_flag;
    RK_U8 output_flag_present_flag;
    RK_U8 transquant_bypass_enable_flag;

    RK_U8 dependent_slice_segments_enabled_flag;
    RK_U8 tiles_enabled_flag;
    RK_U8 entropy_coding_sync_enabled_flag;

    RK_S32 num_tile_columns;   ///< num_tile_columns_minus1 + 1
    RK_S32 num_tile_rows;      ///< num_tile_rows_minus1 + 1
    RK_U8 uniform_spacing_flag;
    RK_U8 loop_filter_across_tiles_enabled_flag;

    RK_U8 seq_loop_filter_across_slices_enabled_flag;

    RK_U8 deblocking_filter_control_present_flag;
    RK_U8 deblocking_filter_override_enabled_flag;
    RK_U8 disable_dbf;
    RK_S32 beta_offset;    ///< beta_offset_div2 * 2
    RK_S32 tc_offset;      ///< tc_offset_div2 * 2

    RK_U8 scaling_list_data_present_flag;
    ScalingList scaling_list;

    RK_U8 lists_modification_present_flag;
    RK_S32 log2_parallel_merge_level; ///< log2_parallel_merge_level_minus2 + 2
    RK_S32 num_extra_slice_header_bits;
    RK_U8 slice_header_extension_present_flag;

    RK_U8 pps_extension_flag;
    RK_U8 pps_extension_data_flag;
    // Inferred parameters
    RK_U32 *column_width;  ///< ColumnWidth
    RK_U32 *row_height;    ///< RowHeight

} HEVCPPS;

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
    int short_term_ref_pic_set_sps_flag;
    int short_term_ref_pic_set_size;
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

    RK_S16 luma_weight_l0[16];
    RK_S16 chroma_weight_l0[16][2];
    RK_S16 chroma_weight_l1[16][2];
    RK_S16 luma_weight_l1[16];

    RK_S16 luma_offset_l0[16];
    RK_S16 chroma_offset_l0[16][2];

    RK_S16 luma_offset_l1[16];
    RK_S16 chroma_offset_l1[16][2];

#ifdef REF_IDX_FRAMEWORK
    RK_S32 inter_layer_pred_enabled_flag;
#endif

#ifdef JCTVC_M0458_INTERLAYER_RPS_SIG
    RK_S32     active_num_ILR_ref_idx;        //< Active inter-layer reference pictures
    RK_S32     inter_layer_pred_layer_idc[MAX_VPS_LAYER_ID_PLUS1];
#endif

    RK_S32 slice_ctb_addr_rs;
} SliceHeader;

typedef struct CurrentFameInf {
    HEVCVPS vps[MAX_VPS_COUNT];
    HEVCSPS sps[MAX_SPS_COUNT];
    HEVCPPS pps[MAX_PPS_COUNT];
    SliceHeader sh;
} CurrentFameInf_t;

typedef struct DBParams {
    RK_S32 beta_offset;
    RK_S32 tc_offset;
} DBParams;

#define HEVC_FRAME_FLAG_OUTPUT    (1 << 0)
#define HEVC_FRAME_FLAG_SHORT_REF (1 << 1)
#define HEVC_FRAME_FLAG_LONG_REF  (1 << 2)

typedef struct HEVCFrame {
    MppFrame   frame;
    RefPicList *refPicList;
    RK_S32 ctb_count;
    RK_S32 poc;
    struct HEVCFrame *collocated_ref;

    HEVCWindow window;

    /**
     * A sequence counter, so that old frames are output first
     * after a POC reset
     */
    RK_U16 sequence;

    /**
     * A combination of HEVC_FRAME_FLAG_*
     */
    RK_U8 flags;
    RK_S32 slot_index;
    RK_U8  error_flag;
} HEVCFrame;

typedef struct HEVCNAL {
    RK_U8 *rbsp_buffer;
    RK_S32 rbsp_buffer_size;
    RK_S32 size;
    const RK_U8 *data;
} HEVCNAL;

typedef struct HEVCLocalContext {
    BitReadCtx_t gb;
} HEVCLocalContext;


typedef struct REF_PIC_DEC_INFO {
    RK_U8          dbp_index;
    RK_U8          is_long_term;
} REF_PIC_DEC_INFO;

typedef struct HEVCContext {
    H265dContext_t *h265dctx;

    HEVCLocalContext    *HEVClc;

    MppFrame frame;

    const HEVCVPS *vps;
    const HEVCSPS *sps;
    const HEVCPPS *pps;
    RK_U8 *vps_list[MAX_VPS_COUNT];
    RK_U8 *sps_list[MAX_SPS_COUNT];
    RK_U8 *pps_list[MAX_PPS_COUNT];

    SliceHeader sh;

    ///< candidate references for the current frame
    RefPicList rps[5];

    enum NALUnitType nal_unit_type;
    RK_S32 temporal_id;  ///< temporal_id_plus1 - 1
    HEVCFrame *ref;
    HEVCFrame DPB[MAX_DPB_SIZE];
    RK_S32 poc;
    RK_S32 pocTid0;
    RK_S32 slice_idx; ///< number of the slice being currently decoded
    RK_S32 eos;       ///< current packet contains an EOS/EOB NAL
    RK_S32 max_ra;

    RK_S32 is_decoded;


    /** used on BE to byteswap the lines for checksumming */
    RK_U8  *checksum_buf;
    RK_S32      checksum_buf_size;

    /**
     * Sequence counters for decoded and output frames, so that old
     * frames are output first after a POC reset
     */
    RK_U16 seq_decode;
    RK_U16 seq_output;

    RK_S32 wpp_err;
    RK_S32 skipped_bytes;

    RK_U8 *data;

    HEVCNAL *nals;
    RK_S32 nb_nals;
    RK_S32 nals_allocated;
    // type of the first VCL NAL of the current frame
    enum NALUnitType first_nal_type;

    RK_U8 context_initialized;
    RK_U8 is_nalff;       ///< this flag is != 0 if bitstream is encapsulated
    ///< as a format defined in 14496-15
    RK_S32 temporal_layer_id;
    RK_S32 decoder_id;
    RK_S32 apply_defdispwin;

    RK_S32 active_seq_parameter_set_id;

    RK_S32 nal_length_size;    ///< Number of bytes used for nal length (1, 2 or 4)
    RK_S32 nuh_layer_id;

    /** frame packing arrangement variables */
    RK_S32 sei_frame_packing_present;
    RK_S32 frame_packing_arrangement_type;
    RK_S32 content_interpretation_type;
    RK_S32 quincunx_subsampling;

    RK_S32 picture_struct;

    /** 1 if the independent slice segment header was successfully parsed */
    RK_U8 slice_initialized;

    RK_S32     decode_checksum_sei;


    RK_U8     scaling_list[81][1360];
    RK_U8     scaling_list_listen[81];
    RK_U8     sps_list_of_updated[MAX_SPS_COUNT];///< zrh add
    RK_U8     pps_list_of_updated[MAX_PPS_COUNT];///< zrh add

    RK_S32         rps_used[16];
    RK_S32         nb_rps_used;
    REF_PIC_DEC_INFO rps_pic_info[600][2][15];      // zrh add
    RK_U8     lowdelay_flag[600];
    RK_U8     rps_bit_offset[600];
    RK_U8     rps_bit_offset_st[600];
    RK_U8     slice_nb_rps_poc[600];

    RK_S32        frame_size;

    RK_S32         framestrid;

    RK_U32    nb_frame;

    RK_U8     output_frame_idx;

    RK_U32    got_frame;
    RK_U32    extra_has_frame;

    MppFrameMasteringDisplayMetadata mastering_display;
    MppFrameContentLightMetadata content_light;

    MppBufSlots slots;

    MppBufSlots packet_slots;
    HalDecTask *task;

    MppPacket input_packet;
    void *hal_pic_private;

    RK_S64 pts;
    RK_U8  has_get_eos;
    RK_U8  miss_ref_flag;
    IOInterruptCB notify_cb;
} HEVCContext;

RK_S32 mpp_hevc_decode_short_term_rps(HEVCContext *s, ShortTermRPS *rps,
                                      const HEVCSPS *sps, RK_S32 is_slice_header);
RK_S32 mpp_hevc_decode_nal_vps(HEVCContext *s);
RK_S32 mpp_hevc_decode_nal_sps(HEVCContext *s);
RK_S32 mpp_hevc_decode_nal_pps(HEVCContext *s);
RK_S32 mpp_hevc_decode_nal_sei(HEVCContext *s);

RK_S32 mpp_hevc_extract_rbsp(HEVCContext *s, const RK_U8 *src, RK_S32 length,
                             HEVCNAL *nal);


/**
 * Mark all frames in DPB as unused for reference.
 */
void mpp_hevc_clear_refs(HEVCContext *s);

/**
 * Drop all frames currently in DPB.
 */
void mpp_hevc_flush_dpb(HEVCContext *s);

/**
 * Compute POC of the current frame and return it.
 */
int mpp_hevc_compute_poc(HEVCContext *s, RK_S32 poc_lsb);


/**
 * Construct the reference picture sets for the current frame.
 */
RK_S32 mpp_hevc_frame_rps(HEVCContext *s);

/**
 * Construct the reference picture list(s) for the current slice.
 */
RK_S32 mpp_hevc_slice_rpl(HEVCContext *s);

/**
 * Get the number of candidate references for the current frame.
 */
RK_S32 mpp_hevc_frame_nb_refs(HEVCContext *s);
RK_S32 mpp_hevc_set_new_ref(HEVCContext *s, MppFrame *frame, RK_S32 poc);

/**
 * Find next frame in output order and put a reference to it in frame.
 * @return 1 if a frame was output, 0 otherwise
 */
void mpp_hevc_unref_frame(HEVCContext *s, HEVCFrame *frame, RK_S32 flags);

void mpp_hevc_pps_free(RK_U8 *data);




#endif /* __H265D_PAESER_H__ */
