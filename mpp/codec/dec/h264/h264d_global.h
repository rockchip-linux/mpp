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

#ifndef __H264D_GLOBAL_H__
#define __H264D_GLOBAL_H__

#include <stdio.h>
#include "rk_type.h"
#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_bitread.h"

#include "h264d_api.h"
#include "h264d_syntax.h"


#define H264D_DBG_ERROR             (0x00000001)
#define H264D_DBG_ASSERT            (0x00000002)
#define H264D_DBG_WARNNING          (0x00000004)
#define H264D_DBG_LOG               (0x00000008)

#define H264D_DBG_INPUT             (0x00000010)   //!< input packet
#define H264D_DBG_PPS_SPS           (0x00000020)
#define H264D_DBG_LOOP_STATE        (0x00000040)
#define H264D_DBG_PARSE_NALU        (0x00000080)

#define H264D_DBG_DPB_INFO          (0x00000100)   //!< dpb size
#define H264D_DBG_DPB_MALLIC        (0x00000200)   //!< malloc


#define H264D_DBG_DPB_REF_ERR       (0x00001000)
#define H264D_DBG_SLOT_FLUSH        (0x00002000)   //!< dpb buffer slot remain
#define H264D_DBG_SEI               (0x00004000)
#define H264D_DBG_CALLBACK          (0x00008000)

#define H264D_DBG_WRITE_ES_EN       (0x00010000)   //!< write input ts stream
#define H264D_DBG_FIELD_PAIRED      (0x00020000)
#define H264D_DBG_DISCONTINUOUS     (0x00040000)

extern RK_U32 rkv_h264d_parse_debug;

#define H264D_DBG(level, fmt, ...)\
do {\
    if (level & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


#define H264D_ERR(fmt, ...)\
do {\
    if (H264D_DBG_ERROR & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define ASSERT(val)\
do {\
    if (H264D_DBG_ASSERT & rkv_h264d_parse_debug)\
        { mpp_assert(val); }\
} while (0)

#define H264D_WARNNING(fmt, ...)\
do {\
    if (H264D_DBG_WARNNING & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define H264D_LOG(fmt, ...)\
do {\
    if (H264D_DBG_LOG & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


//!< vaule check
#define VAL_CHECK(ret, val, ...)\
do{\
    if (!(val)){\
        ret = MPP_ERR_VALUE; \
        H264D_WARNNING("value error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)
//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        H264D_ERR("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)


//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
if ((val)) {\
        ret = MPP_ERR_INIT; \
        H264D_WARNNING("input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)
//!< function return check
#define FUN_CHECK(val)\
do{\
if ((val) < 0) {\
        H264D_WARNNING("Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)



#define START_PREFIX_3BYTE        3
#define MAX_NUM_DPB_LAYERS        2
#define MAX_LIST_SIZE             33   //!< for init list reorder
#define MAX_DPB_SIZE              16   //!< for prepare dpb info
#define MAX_REF_SIZE              32   //!< for prepare ref pic info


#define MAX_MARK_SIZE             35   //!< for malloc buffer mark, can be changed

#define HEAD_BUF_MAX_SIZE         (1*1024*1024)
#define HEAD_BUF_ADD_SIZE         (512)
#define NALU_BUF_MAX_SIZE         (2*1024*1024)
#define NALU_BUF_ADD_SIZE         (512)
#define SODB_BUF_MAX_SIZE         (2*1024*1024)
#define SODB_BUF_ADD_SIZE         (512)

//!< PPS parameters
#define MAXnum_slice_groups_minus1  8
enum {
    H264ScalingList4x4Length = 16,
    H264ScalingList8x8Length = 64,
} ScalingListLength;

typedef enum {
    YUV400 = 0,     //!< Monochrome
    YUV420 = 1,     //!< 4:2:0
    YUV422 = 2,     //!< 4:2:2
    YUV444 = 3      //!< 4:4:4
} ColorFormat;

typedef enum {
    STRUCT_NULL  = 0,
    TOP_FIELD    = 0x1,
    BOTTOM_FIELD = 0x2,
    FRAME        = 0x3,
    STRUCT_MAX,
} PictureStructure;           //!< New enum for field processing

typedef enum {
    PIC_ERR_NULL  = 0,
    PIC_ERR_TOP   = 0x1,
    PIC_ERR_BOT   = 0x2,
    PIC_ERR_FRAME = 0x3,
    PIC_ERR_WHOLE = 0x3,
    PIC_ERR_MAX,
} PictureError;

typedef enum {
    FIELD_ORDER_NULL,
    FIELD_ORDER_TOP_FIRST,
    FIELD_ORDER_BOT_FIRST,
    FIELD_ORDER_SAME,
    FIELD_ORDER_MAX
} FieldOrder;

//!< Field Coding Types
typedef enum {
    FRAME_CODING = 0,
    FIELD_CODING = 1,
    ADAPTIVE_CODING = 2,
    FRAME_MB_PAIR_CODING = 3
} CodingType;

typedef enum {
    P_SLICE = 0,
    B_SLICE = 1,
    I_SLICE = 2,
    SP_SLICE = 3,
    SI_SLICE = 4,
    NUM_SLICE_TYPES = 5
} SliceType;

typedef enum {
    LIST_0 = 0,
    LIST_1 = 1,
    BI_PRED = 2,
    BI_PRED_L0 = 3,
    BI_PRED_L1 = 4
} ListType;

//!< NAL Unit structure
typedef struct h264_nalu_t {
    RK_S32         startcodeprefix_len;   //!< 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    RK_U32         sodb_len;              //!< Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    RK_S32         forbidden_bit;         //!< should be always FALSE
    Nalu_type       nalu_type;         //!< NALU_TYPE_xxxx
    NalRefIdc_type  nal_reference_idc;     //!< NALU_PRIORITY_xxxx
    RK_U8          *sodb_buf;             //!< Data of the NAL unit (Excluding the start code, which does not belong to the NALU)
    RK_U16          lost_packets;          //!< true, if packet loss is detected, used in RTPNALU
    //---- MVC extension
    RK_S32   svc_extension_flag;    //!< should be always 0, for MVC
    RK_S32   non_idr_flag;          //!< 0 = current is IDR
    RK_S32   priority_id;           //!< a lower value of priority_id specifies a higher priority
    RK_S32   view_id;               //!< view identifier for the NAL unit
    RK_S32   temporal_id;           //!< temporal identifier for the NAL unit
    RK_S32   anchor_pic_flag;       //!< anchor access unit
    RK_S32   inter_view_flag;       //!< inter-view prediction enable
    RK_S32   reserved_one_bit;      //!< shall be equal to 1
    RK_S32   MVCExt_is_valid;
    RK_S32   MVCExt_is_prefixNALU;
    //------
    RK_U8    ualu_header_bytes;     //!< for rbsp start
    RK_S32   used_bits;

} H264_Nalu_t;

typedef struct h264_nalu_mvc_ext_t {
    RK_U32    valid;
    RK_U32    non_idr_flag;
    RK_U32    priority_id;
    RK_U32    view_id;
    RK_U32    temporal_id;
    RK_U32    anchor_pic_flag;
    RK_U32    inter_view_flag;
    RK_U32    reserved_one_bit;
    RK_U32    iPrefixNALU;
} H264_NaluMvcExt_t;

//!< decoder refence picture marking
typedef struct h264_drpm_t {
    RK_S32 memory_management_control_operation;
    RK_S32 difference_of_pic_nums_minus1;
    RK_S32 long_term_pic_num;
    RK_S32 long_term_frame_idx;
    RK_S32 max_long_term_frame_idx_plus1;
    struct  h264_drpm_t *Next;
} H264_DRPM_t;

typedef enum {
    Mem_NULL     = 0,
    Mem_Malloc   = 1,
    Mem_Clone    = 2,
    Mem_UnPaired = 3,
    Mem_TopOnly  = 4,
    Mem_BotOnly  = 5,
    Mem_Fake     = 6,
    Mem_Max,
} H264_Mem_type;

//!< decoder picture memory
typedef struct h264_dpb_mark_t {
    RK_U8    top_used;
    RK_U8    bot_used;
    RK_U8    out_flag;
    RK_U8    mark_idx;
    MppFrame mframe;
    RK_S32   slot_idx;
    RK_S32   poc;
    RK_S64   pts;
    struct h264_store_pic_t *pic;
} H264_DpbMark_t;

//!< decoder picture buffer information
typedef struct h264_dpb_info_t {
    RK_U8     colmv_is_used;
    RK_S32    slot_index;

    RK_S32    TOP_POC;
    RK_S32    BOT_POC;
    RK_U16    frame_num;
    RK_U32    field_flag;
    RK_U32    is_long_term;
    RK_U32    is_ilt_flag;
    RK_U32    long_term_frame_idx;
    RK_U32    long_term_pic_num;
    RK_U32    voidx;
    RK_U32    view_id;
    RK_U32    is_used;
    RK_U32    top_valid;
    RK_U32    bot_valid;
    struct h264_store_pic_t *refpic;

    RK_U32    have_same;
} H264_DpbInfo_t;

//!< refence picture information
typedef struct h264_refpic_info_t {
    RK_U32    valid;
    RK_S32    dpb_idx;
    RK_S32    bottom_flag;
} H264_RefPicInfo_t;

//!< definition a picture (field or frame)
typedef struct h264_store_pic_t {
    RK_S32    structure;
    RK_S32    poc;
    RK_S32    top_poc;
    RK_S32    bottom_poc;
    RK_S32    frame_poc;
    RK_S32    ThisPOC;
    RK_S32    pic_num;
    RK_S32    long_term_pic_num;
    RK_S32    long_term_frame_idx;

    RK_U32    frame_num;
    RK_U8     is_long_term;
    RK_S32    used_for_reference;
    RK_S32    is_output;
    RK_S32    non_existing;
    RK_S16    max_slice_id;
    RK_S32    mb_aff_frame_flag;
    RK_U32    PicWidthInMbs;
    RK_U8     colmv_no_used_flag;
    struct h264_store_pic_t *top_field;     //!< for mb aff, if frame for referencing the top field
    struct h264_store_pic_t *bottom_field;  //!< for mb aff, if frame for referencing the bottom field
    struct h264_store_pic_t *frame;         //!< for mb aff, if field for referencing the combined frame

    RK_S32       slice_type;
    RK_S32       idr_flag;
    RK_S32       no_output_of_prior_pics_flag;
    RK_S32       long_term_reference_flag;
    RK_S32       adaptive_ref_pic_buffering_flag;
    RK_S32       chroma_format_idc;
    RK_S32       frame_mbs_only_flag;
    RK_S32       frame_cropping_flag;

    RK_S32       frame_crop_left_offset;
    RK_S32       frame_crop_right_offset;
    RK_S32       frame_crop_top_offset;
    RK_S32       frame_crop_bottom_offset;
    RK_S32       width;
    RK_S32       height;
    RK_U32       width_after_crop;
    RK_U32       height_after_crop;

    struct h264_drpm_t   *dec_ref_pic_marking_buffer;                    //!< stores the memory management control operations
    RK_S32       proc_flag;
    RK_S32       view_id;
    RK_S32       inter_view_flag;
    RK_S32       anchor_pic_flag;
    RK_S32       iCodingType;

    RK_S32       layer_id;
    RK_U8        is_mmco_5;
    RK_S32       poc_mmco5;
    RK_S32       top_poc_mmco5;
    RK_S32       bot_poc_mmco5;
    RK_S32       combine_flag;  // top && bottom field combined flag
    H264_Mem_type       mem_malloc_type;
    struct h264_dpb_mark_t     *mem_mark;
} H264_StorePic_t;

//!< Frame Stores for Decoded Picture Buffer
typedef struct h264_frame_store_t {
    RK_S32    is_used;                //!< 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
    RK_S32    is_reference;           //!< 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
    RK_S32    is_long_term;           //!< 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
    RK_S32    is_orig_reference;      //!< original marking by nal_ref_idc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
    RK_S32    is_non_existent;
    RK_S32    frame_num_wrap;
    RK_S32    long_term_frame_idx;
    RK_S32    is_output;
    RK_S32    poc;
    RK_S32    view_id;
    RK_S32    inter_view_flag[2];
    RK_S32    anchor_pic_flag[2];
    RK_S32    layer_id;
    RK_S32    slice_type;
    RK_U32    frame_num;
    RK_S32    structure;
    RK_U32    is_directout;
    struct h264_store_pic_t *frame;
    struct h264_store_pic_t *top_field;
    struct h264_store_pic_t *bottom_field;

} H264_FrameStore_t;

//!< decode picture buffer
typedef struct h264_dpb_buf_t {
    RK_U32   size;
    RK_U32   used_size;
    RK_U32   ref_frames_in_buffer;
    RK_U32   ltref_frames_in_buffer;
    RK_U32   used_size_il;

    RK_S32   poc_interval;
    RK_S32   last_output_poc;
    RK_S32   last_output_view_id;
    RK_S32   max_long_term_pic_idx;
    RK_S32   init_done;
    RK_S32   num_ref_frames;
    RK_S32   layer_id;

    struct h264_frame_store_t  **fs;
    struct h264_frame_store_t  **fs_ref;
    struct h264_frame_store_t  **fs_ltref;
    struct h264_frame_store_t  **fs_ilref;   //!< inter-layer reference (for multi-layered codecs)
    struct h264_frame_store_t   *last_picture;

    struct h264d_video_ctx_t   *p_Vid;
} H264_DpbBuf_t;

//!< HRD
#define MAXIMUMVALUEOFcpb_cnt   32
typedef struct h264_hrd_t {
    RK_U32    cpb_cnt_minus1;                                  // ue(v)
    RK_U32    bit_rate_scale;                                  // u(4)
    RK_U32    cpb_size_scale;                                  // u(4)
    RK_U32    bit_rate_value_minus1[MAXIMUMVALUEOFcpb_cnt];    // ue(v)
    RK_U32    cpb_size_value_minus1[MAXIMUMVALUEOFcpb_cnt];    // ue(v)
    RK_U32    cbr_flag[MAXIMUMVALUEOFcpb_cnt];                 // u(1)
    RK_U32    initial_cpb_removal_delay_length_minus1;         // u(5)
    RK_U32    cpb_removal_delay_length_minus1;                 // u(5)
    RK_U32    dpb_output_delay_length_minus1;                  // u(5)
    RK_U32    time_offset_length;                              // u(5)
} H264_HRD_t;

//!< VUI
typedef struct h264_vui_t {
    RK_S32       aspect_ratio_info_present_flag;                   // u(1)
    RK_U32       aspect_ratio_idc;                                 // u(8)
    RK_U16       sar_width;                                        // u(16)
    RK_U16       sar_height;                                       // u(16)
    RK_S32       overscan_info_present_flag;                       // u(1)
    RK_S32       overscan_appropriate_flag;                        // u(1)
    RK_S32       video_signal_type_present_flag;                   // u(1)
    RK_U32       video_format;                                     // u(3)
    RK_S32       video_full_range_flag;                            // u(1)
    RK_S32       colour_description_present_flag;                  // u(1)
    RK_U32       colour_primaries;                                 // u(8)
    RK_U32       transfer_characteristics;                         // u(8)
    RK_U32       matrix_coefficients;                              // u(8)
    RK_S32       chroma_location_info_present_flag;                // u(1)
    RK_U32       chroma_sample_loc_type_top_field;                // ue(v)
    RK_U32       chroma_sample_loc_type_bottom_field;             // ue(v)
    RK_S32       timing_info_present_flag;                         // u(1)
    RK_U32       num_units_in_tick;                                // u(32)
    RK_U32       time_scale;                                       // u(32)
    RK_S32       fixed_frame_rate_flag;                            // u(1)
    RK_S32       nal_hrd_parameters_present_flag;                  // u(1)
    struct h264_hrd_t    nal_hrd_parameters;                        // hrd_paramters_t
    RK_S32       vcl_hrd_parameters_present_flag;                  // u(1)
    struct h264_hrd_t    vcl_hrd_parameters;                        // hrd_paramters_t
    // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
    RK_S32       low_delay_hrd_flag;                               // u(1)
    RK_S32       pic_struct_present_flag;                          // u(1)
    RK_S32       bitstream_restriction_flag;                       // u(1)
    RK_S32       motion_vectors_over_pic_boundaries_flag;          // u(1)
    RK_U32       max_bytes_per_pic_denom;                          // ue(v)
    RK_U32       max_bits_per_mb_denom;                            // ue(v)
    RK_U32       log2_max_mv_length_vertical;                      // ue(v)
    RK_U32       log2_max_mv_length_horizontal;                    // ue(v)
    RK_U32       num_reorder_frames;                               // ue(v)
    RK_U32       max_dec_frame_buffering;                          // ue(v)
} H264_VUI_t;

//!< MVC_VUI
typedef struct h264_mvc_vui_t {
    RK_S32  num_ops_minus1;
    RK_S8   *temporal_id;
    RK_S32  *num_target_output_views_minus1;
    RK_S32  **view_id;
    RK_S8   *timing_info_present_flag;
    RK_S32  *num_units_in_tick;
    RK_S32  *time_scale;
    RK_S8   *fixed_frame_rate_flag;
    RK_S8   *nal_hrd_parameters_present_flag;
    RK_S8   *vcl_hrd_parameters_present_flag;
    RK_S8   *low_delay_hrd_flag;
    RK_S8   *pic_struct_present_flag;

    //hrd parameters;
    RK_S8   cpb_cnt_minus1;
    RK_S8   bit_rate_scale;
    RK_S8   cpb_size_scale;
    RK_S32  bit_rate_value_minus1[32];
    RK_S32  cpb_size_value_minus1[32];
    RK_S8   cbr_flag[32];
    RK_S8   initial_cpb_removal_delay_length_minus1;
    RK_S8   cpb_removal_delay_length_minus1;
    RK_S8   dpb_output_delay_length_minus1;
    RK_S8   time_offset_length;
} H264_mvcVUI_t;

//!< SPS
#define MAXnum_ref_frames_in_POC_cycle  256

typedef struct h264_sps_t {
    RK_S32    Valid;                  // indicates the parameter set is valid

    RK_S32    profile_idc;                                       // u(8)
    RK_S32    constrained_set0_flag;                             // u(1)
    RK_S32    constrained_set1_flag;                             // u(1)
    RK_S32    constrained_set2_flag;                             // u(1)
    RK_S32    constrained_set3_flag;                             // u(1)
    RK_S32    constrained_set4_flag;                             // u(1)
    RK_S32    constrained_set5_flag;                             // u(2)

    RK_S32    level_idc;                                         // u(8)
    RK_S32    seq_parameter_set_id;                              // ue(v)
    RK_S32    chroma_format_idc;                                 // ue(v)

    RK_S32    seq_scaling_matrix_present_flag;                   // u(1)
    RK_S32    seq_scaling_list_present_flag[12];                 // u(1)
    RK_S32    ScalingList4x4[6][H264ScalingList4x4Length];       // se(v)
    RK_S32    ScalingList8x8[6][H264ScalingList8x8Length];       // se(v)
    RK_S32    UseDefaultScalingMatrix4x4Flag[6];
    RK_S32    UseDefaultScalingMatrix8x8Flag[6];

    RK_S32    bit_depth_luma_minus8;                             // ue(v)
    RK_S32    bit_depth_chroma_minus8;                           // ue(v)
    RK_S32    log2_max_frame_num_minus4;                         // ue(v)
    RK_S32    pic_order_cnt_type;
    // if( pic_order_cnt_type == 0 )
    RK_S32    log2_max_pic_order_cnt_lsb_minus4;                 // ue(v)
    // else if( pic_order_cnt_type == 1 )
    RK_S32    delta_pic_order_always_zero_flag;                  // u(1)
    RK_S32    offset_for_non_ref_pic;                            // se(v)
    RK_S32    offset_for_top_to_bottom_field;                    // se(v)
    RK_S32    num_ref_frames_in_pic_order_cnt_cycle;             // ue(v)
    // for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
    RK_S32    offset_for_ref_frame[MAXnum_ref_frames_in_POC_cycle];   // se(v)
    RK_S32    max_num_ref_frames;                                     // ue(v)
    RK_S32    gaps_in_frame_num_value_allowed_flag;             // u(1)
    RK_S32    pic_width_in_mbs_minus1;                          // ue(v)
    RK_S32    pic_height_in_map_units_minus1;                   // ue(v)
    RK_S32    frame_mbs_only_flag;                              // u(1)
    // if( !frame_mbs_only_flag )
    RK_S32    mb_adaptive_frame_field_flag;                     // u(1)
    RK_S32    direct_8x8_inference_flag;                        // u(1)
    RK_S32    frame_cropping_flag;                              // u(1)
    RK_S32    frame_crop_left_offset;                // ue(v)
    RK_S32    frame_crop_right_offset;               // ue(v)
    RK_S32    frame_crop_top_offset;                 // ue(v)
    RK_S32    frame_crop_bottom_offset;              // ue(v)
    RK_S32    vui_parameters_present_flag;           // u(1)
    struct h264_vui_t vui_seq_parameters;             // vui_seq_parameters_t
    RK_S32    separate_colour_plane_flag;            // u(1)
    RK_S32    max_dec_frame_buffering;
    RK_S32    qpprime_y_zero_transform_bypass_flag;
    //----
    RK_S32    expected_delta_per_pic_order_cnt_cycle;

} H264_SPS_t;

//!< subSPS
typedef struct h264_subsps_t {
    struct h264_sps_t sps;

    RK_S32     bit_equal_to_one;
    RK_S32     num_views_minus1;
    RK_S32     *view_id;
    RK_S32     *num_anchor_refs_l0;
    RK_S32     **anchor_ref_l0;
    RK_S32     *num_anchor_refs_l1;
    RK_S32     **anchor_ref_l1;

    RK_S32     *num_non_anchor_refs_l0;
    RK_S32     **non_anchor_ref_l0;
    RK_S32     *num_non_anchor_refs_l1;
    RK_S32     **non_anchor_ref_l1;

    RK_S32     num_level_values_signalled_minus1;
    RK_S32     *level_idc;
    RK_S32     *num_applicable_ops_minus1;
    RK_S32     **applicable_op_temporal_id;
    RK_S32     **applicable_op_num_target_views_minus1;
    RK_S32     ***applicable_op_target_view_id;
    RK_S32     **applicable_op_num_views_minus1;

    RK_S32     mvc_vui_parameters_present_flag;
    RK_S32     Valid;          // indicates the parameter set is valid

} H264_subSPS_t;

//!< PPS

typedef struct h264_pps_t {
    RK_S32   Valid;                  // indicates the parameter set is valid
    RK_S32   pic_parameter_set_id;                             // ue(v)
    RK_S32   seq_parameter_set_id;                             // ue(v)
    RK_S32   entropy_coding_mode_flag;                            // u(1)
    RK_S32   transform_8x8_mode_flag;                             // u(1)

    RK_S32   pic_scaling_matrix_present_flag;                     // u(1)
    RK_S32   pic_scaling_list_present_flag[12];                   // u(1)
    RK_S32   ScalingList4x4[6][H264ScalingList4x4Length];                               // se(v)
    RK_S32   ScalingList8x8[6][H264ScalingList8x8Length];                               // se(v)
    RK_S32   UseDefaultScalingMatrix4x4Flag[6];
    RK_S32   UseDefaultScalingMatrix8x8Flag[6];

    // if( pic_order_cnt_type < 2 )  in the sequence parameter set
    RK_S32   bottom_field_pic_order_in_frame_present_flag;                           // u(1)
    RK_S32   num_slice_groups_minus1;                          // ue(v)
    RK_S32   slice_group_map_type;                             // ue(v)
    // if( slice_group_map_type = = 0 )
    RK_S32   run_length_minus1[MAXnum_slice_groups_minus1];    // ue(v)
    // else if( slice_group_map_type = = 2 )
    RK_S32   top_left[MAXnum_slice_groups_minus1];             // ue(v)
    RK_S32   bottom_right[MAXnum_slice_groups_minus1];         // ue(v)
    // else if( slice_group_map_type = = 3 || 4 || 5
    RK_S32   slice_group_change_direction_flag;                // u(1)
    RK_S32   slice_group_change_rate_minus1;                   // ue(v)
    // else if( slice_group_map_type = = 6 )
    RK_S32   pic_size_in_map_units_minus1;                     // ue(v)
    RK_U8    *slice_group_id;                                  // complete MBAmap u(v)

    RK_S32   num_ref_idx_l0_default_active_minus1;             // ue(v)
    RK_S32   num_ref_idx_l1_default_active_minus1;             // ue(v)
    RK_S32   weighted_pred_flag;                               // u(1)
    RK_S32   weighted_bipred_idc;                              // u(2)
    RK_S32   pic_init_qp_minus26;                              // se(v)
    RK_S32   pic_init_qs_minus26;                              // se(v)
    RK_S32   chroma_qp_index_offset;                           // se(v)

    RK_S32   cb_qp_index_offset;                               // se(v)
    RK_S32   cr_qp_index_offset;                               // se(v)
    RK_S32   second_chroma_qp_index_offset;                    // se(v)

    RK_S32   deblocking_filter_control_present_flag;           // u(1)
    RK_S32   constrained_intra_pred_flag;                      // u(1)
    RK_S32   redundant_pic_cnt_present_flag;                   // u(1)
    RK_S32   vui_pic_parameters_flag;                          // u(1)
    RK_S32   scaleing_list_enable_flag;
    RK_U32   scaleing_list_address;

} H264_PPS_t;

//!< SEI
typedef enum {
    SEI_BUFFERING_PERIOD = 0,
    SEI_PIC_TIMING,
    SEI_PAN_SCAN_RECT,
    SEI_FILLER_PAYLOAD,
    SEI_USER_DATA_REGISTERED_ITU_T_T35,
    SEI_USER_DATA_UNREGISTERED,
    SEI_RECOVERY_POINT,
    SEI_DEC_REF_PIC_MARKING_REPETITION,
    SEI_SPARE_PIC,
    SEI_SCENE_INFO,
    SEI_SUB_SEQ_INFO,
    SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
    SEI_SUB_SEQ_CHARACTERISTICS,
    SEI_FULL_FRAME_FREEZE,
    SEI_FULL_FRAME_FREEZE_RELEASE,
    SEI_FULL_FRAME_SNAPSHOT,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
    SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
    SEI_FILM_GRAIN_CHARACTERISTICS,
    SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
    SEI_STEREO_VIDEO_INFO,
    SEI_POST_FILTER_HINTS,
    SEI_TONE_MAPPING,
    SEI_SCALABILITY_INFO,
    SEI_SUB_PIC_SCALABLE_LAYER,
    SEI_NON_REQUIRED_LAYER_REP,
    SEI_PRIORITY_LAYER_INFO,
    SEI_LAYERS_NOT_PRESENT,
    SEI_LAYER_DEPENDENCY_CHANGE,
    SEI_SCALABLE_NESTING,
    SEI_BASE_LAYER_TEMPORAL_HRD,
    SEI_QUALITY_LAYER_INTEGRITY_CHECK,
    SEI_REDUNDANT_PIC_PROPERTY,
    SEI_TL0_DEP_REP_INDEX,
    SEI_TL_SWITCHING_POINT,
    SEI_PARALLEL_DECODING_INFO,
    SEI_MVC_SCALABLE_NESTING,
    SEI_VIEW_SCALABILITY_INFO,
    SEI_MULTIVIEW_SCENE_INFO,
    SEI_MULTIVIEW_ACQUISITION_INFO,
    SEI_NON_REQUIRED_VIEW_COMPONENT,
    SEI_VIEW_DEPENDENCY_CHANGE,
    SEI_OPERATION_POINTS_NOT_PRESENT,
    SEI_BASE_VIEW_TEMPORAL_HRD,
    SEI_FRAME_PACKING_ARRANGEMENT,

    SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
} RKV_H264SEI_Type;

//!< MVC_scalable_nesting
typedef struct {
    RK_S32   operation_point_flag;
    RK_S32   all_view_components_in_au_flag;
    RK_S32   num_view_components_minus1;
    RK_S32   *sei_view_id;
    RK_S32   num_view_components_op_minus1;
    RK_S32   *sei_op_view_id;
    RK_S32   sei_op_temporal_id;
    RK_S32   Valid;
} MVC_scalable_nesting_t;

//!< MVC_scalability_info
typedef struct {
    RK_S32 num_operation_points_minus1;
    RK_S32 *operation_point_id;
    RK_S32 *priority_id;
    RK_S32 *temporal_id;
    RK_S32 *num_target_output_views_minus1;
    RK_S32 **view_id;
    RK_S32 *profile_level_info_present_flag;
    RK_S32 *bitrate_info_present_flag;
    RK_S32 *frm_rate_info_present_flag;

    RK_S32 *view_dependency_info_present_flag;
    RK_S32 *parameter_sets_info_present_flag;
    RK_S32 *bitstream_restriction_info_present_flag;
    RK_S32 *op_profile_level_idc;
    RK_S32 *avg_bitrate;
    RK_S32 *max_bitrate;
    RK_S32 *max_bitrate_calc_window;
    RK_S32 *constant_frm_rate_idc;
    RK_S32 *avg_frm_rate;
    RK_S32 *num_directly_dependent_views;
    RK_S32 **directly_dependent_view_id;
    RK_S32 *view_dependency_info_src_op_id;
    RK_S32 *num_seq_parameter_sets;
    RK_S32 **seq_parameter_set_id_delta;
    RK_S32 *num_subset_seq_parameter_sets;
    RK_S32 **subset_seq_parameter_set_id_delta;
    RK_S32 *num_pic_parameter_sets_minus1;
    RK_S32 **pic_parameter_set_id_delta;
    RK_S32 *parameter_sets_info_src_op_id;
    RK_S32 *motion_vectors_over_pic_boundaries_flag;
    RK_S32 *max_bytes_per_pic_denom;
    RK_S32 *max_bits_per_mb_denom;
    RK_S32 *log2_max_mv_length_horizontal;
    RK_S32 *log2_max_mv_length_vertical;
    RK_S32 *max_num_reorder_frames;
    RK_S32 *max_dec_frame_buffering;
} MVC_scalability_info_t;

typedef struct h264_sei_pic_timing_t {
    RK_S32 cpb_removal_delay;
    RK_S32 dpb_output_delay;
    RK_S32 pic_struct;
    RK_S32 clock_timestamp_flag[3];
    RK_S32 clock_timestamp[3];
    RK_S32 ct_type[3];
    RK_S32 nuit_field_based_flag[3];
    RK_S32 counting_type[3];
    RK_S32 full_timestamp_flag[3];
    RK_S32 discontinuity_flag[3];
    RK_S32 cnt_dropped_flag[3];
    RK_S32 n_frames[3];
    RK_S32 seconds_flag[3];
    RK_S32 seconds_value[3];
    RK_S32 minutes_flag[3];
    RK_S32 minutes_value[3];
    RK_S32 hours_flag[3];
    RK_S32 hours_value[3];
    RK_S32 time_offset[3];
} H264_SEI_PIC_TIMING_t;

//!< SEI struct
typedef struct h264_sei_t {
    RK_S32 type;
    RK_S32 payload_size;
    struct {
        RK_U32  recovery_frame_cnt;
        RK_S32  exact_match_flag;
        RK_S32  broken_link_flag;
        RK_S32  changing_slice_group_idc;
    } recovery_point;
    struct {
        RK_S32 operation_point_flag;
        //-- for adding
    } scalable_nesting;

    RK_U32 user_data_DivX_flag;
    // Placeholder; in future more supported types will contribute to more
    //---- follow is used in other parts
    RK_S32 mvc_scalable_nesting_flag;
    RK_S32  seq_parameter_set_id;

    H264_SEI_PIC_TIMING_t pic_timing;

    struct h264_dec_ctx_t *p_Dec;
} H264_SEI_t;

//!< SLICE
typedef struct h264_slice_t {
    struct h264_nalu_mvc_ext_t mvcExt;
    //--- slice property;
    RK_S32       layer_id;
    RK_S32       idr_flag;
    RK_S32       idr_pic_id;
    RK_S32       nal_reference_idc;            //!< nal_reference_idc from NAL unit
    RK_U32       start_mb_nr;                  //!< MUST be set by NAL even in case of ei_flag == 1
    RK_S32       slice_type;                   //!< slice type
    RK_S32       pic_parameter_set_id;
    RK_S32       colour_plane_id;
    RK_S32       frame_num;
    RK_S32       field_pic_flag;
    RK_S32       bottom_field_flag;
    RK_S32       structure;
    RK_S32       mb_aff_frame_flag;
    RK_S32       pic_order_cnt_lsb;              // for poc mode 0
    RK_S32       delta_pic_order_cnt_bottom;
    RK_S32       delta_pic_order_cnt[2];         // for poc mode 1
    RK_U32       poc_used_bitlen;
    RK_S32       redundant_pic_cnt;
    RK_S32       direct_spatial_mv_pred_flag;
    RK_S32       num_ref_idx_active[2];          //!< number of available list references
    RK_S32       num_ref_idx_override_flag;
    RK_S32       ref_pic_list_reordering_flag[2];
    RK_S32       *modification_of_pic_nums_idc[2];
    RK_S32       *abs_diff_pic_num_minus1[2];
    RK_S32       *long_term_pic_idx[2];
    RK_S32       *abs_diff_view_idx_minus1[2];
    struct h264_drpm_t   *dec_ref_pic_marking_buffer;
    RK_U32       drpm_used_bitlen;
    RK_S32       no_output_of_prior_pics_flag;
    RK_S32       long_term_reference_flag;
    RK_S32       adaptive_ref_pic_buffering_flag;
    RK_U32       model_number;
    RK_S32       slice_qp_delta;
    RK_S32       qp;
    RK_S32       current_mb_nr;
    RK_S32       ThisPOC;
    RK_S32       toppoc;      //poc for this top field
    RK_S32       bottompoc;   //poc of bottom field of frame
    RK_S32       framepoc;    //poc of this frame
    RK_U32       AbsFrameNum;
    RK_S32       PicOrderCntMsb;
    //RK_S32       new_frame_flag;
    struct h264_sps_t    *active_sps;
    struct h264_subsps_t *active_subsps;
    struct h264_pps_t    *active_pps;

    struct h264_dpb_buf_t  *p_Dpb;
    struct h264_dpb_buf_t  *p_Dpb_layer[MAX_NUM_DPB_LAYERS];
    //----
    RK_S32       listinterviewidx0;
    RK_S32       listinterviewidx1;

    RK_U8        listXsizeP[2];
    RK_U8        listXsizeB[2];

    struct h264_frame_store_t    **fs_listinterview0;
    struct h264_frame_store_t    **fs_listinterview1;
    //struct h264_store_pic_t     **listX[6];
    struct h264_store_pic_t      **listP[2];
    struct h264_store_pic_t      **listB[2];

    //---- MVC extend ----
    RK_S32   svc_extension_flag;      // should be always 0, for MVC
    RK_S32   non_idr_flag;            // 0 = current is IDR
    RK_S32   priority_id;             // a lower value of priority_id specifies a higher priority
    RK_S32   view_id;                 // view identifier for the NAL unit
    RK_S32   temporal_id;             // temporal identifier for the NAL unit
    RK_S32   anchor_pic_flag;         // anchor access unit
    RK_S32   inter_view_flag;         // inter-view prediction enable
    RK_S32   reserved_one_bit;        // shall be equal to 1
    RK_S32   MVCExt_is_valid;
    RK_S32   MVCExt_is_prefixNALU;

    struct h264_dec_ctx_t     *p_Dec;    // H264_DecCtx_t
    struct h264d_input_ctx_t  *p_Inp;    // H264_InputParameters
    struct h264d_cur_ctx_t    *p_Cur;    // H264_CurParameters
    struct h264d_video_ctx_t  *p_Vid;
} H264_SLICE_t;

//!< Old slice parameter for check new frame
typedef struct h264_old_slice_par_t {
    RK_U32  current_mb_nr;
    RK_U8   field_pic_flag;
    RK_U8   frame_num;
    RK_S32  nal_ref_idc;
    RK_U8   pic_oder_cnt_lsb;
    RK_S32  delta_pic_oder_cnt_bottom;
    RK_S32  delta_pic_order_cnt[2];
    RK_U8   bottom_field_flag;
    RK_U8   idr_flag;
    RK_S32  idr_pic_id;
    RK_S32  pps_id;
    RK_S32  view_id;
    RK_S32  inter_view_flag;
    RK_S32  anchor_pic_flag;
    RK_S32  layer_id;
} H264_OldSlice_t;

//!< DXVA context
#define MAX_SLICE_NUM              (20)
#define ADD_SLICE_SIZE             (5)
#define BITSTREAM_MAX_SIZE         (2*1024*1024)
#define BITSTREAM_ADD_SIZE         (512)
#define SYNTAX_BUF_SIZE            (5)
typedef struct h264d_dxva_ctx_t {
    RK_U8                            cfgBitstrmRaw;
    struct _DXVA_PicParams_H264_MVC  pp;
    struct _DXVA_Qmatrix_H264        qm;
    RK_U32                           max_slice_size;
    RK_U32                           slice_count;
    struct _DXVA_Slice_H264_Long     *slice_long;   //!<  MAX_SLICES
    RK_U8                            *bitstream;
    RK_U32                           max_strm_size;
    RK_U32                           strm_offset;
    struct h264d_syntax_t            syn;
    struct h264_dec_ctx_t            *p_Dec;
} H264dDxvaCtx_t;

//!< input parameter
typedef struct h264d_input_ctx_t {
    struct h264_dec_ctx_t      *p_Dec;
    struct h264d_cur_ctx_t     *p_Cur;   //!< current parameters, use in read nalu
    struct h264d_video_ctx_t   *p_Vid;   //!< parameters for video decoder
    enum   mpp_decmtd_type      dec_mtd;
    ParserCfg                   init;
    //!< input data
    RK_U8  *in_buf;
    size_t in_length;
    RK_U32 pkt_eos;

    MppPacket in_pkt;

    RK_S64 in_pts;
    RK_S64 in_dts;
    RK_U8  has_get_eos;
    RK_U32 mvc_disable;
    //!< output data
    RK_U8  task_valid;
    RK_U32 task_eos;

    //!< have extradata
    RK_U8  is_nalff;
    RK_U8  profile;
    RK_U8  level;
    RK_U32 nal_size;
    RK_S32 sps_num;
    RK_S32 pps_num;
    //!< write stream
    char fname[2][512];
    FILE *fp;
    RK_U8 *spspps_buf;
    RK_U32 spspps_size;
    RK_U32 spspps_len;
    RK_U32 spspps_offset;
    RK_U32 spspps_update_flag;

} H264dInputCtx_t;


//!< current stream
typedef struct h264d_curstrm_t {
    RK_U32    nalu_offset;     //!< The offset of the input stream
    RK_U32    nalu_max_size;   //!< Cur Unit Buffer size
    RK_U8     *curdata;

    RK_S32    nalu_type;
    RK_U32    nalu_len;
    RK_U8     *nalu_buf;       //!< store read nalu data

    RK_U32    head_offset;
    RK_U32    head_max_size;
    RK_U8     *head_buf;       //!< store header data, sps/pps/slice header

    RK_U32    prefixdata;
    RK_U8     startcode_found;
    RK_U8     endcode_found;

} H264dCurStream_t;

#define MAX_REORDER_TIMES   17
#define MAX_MARKING_TIMES   35
//!< current parameters
typedef struct h264d_cur_ctx_t {
    struct h264_sps_t        sps;
    struct h264_subsps_t     subsps;
    struct h264_pps_t        pps;
    struct h264_sei_t        sei;
    struct h264_nalu_t       nalu;
    struct bitread_ctx_t     bitctx; //!< for control bit_read
    struct h264d_curstrm_t   strm;
    struct h264_slice_t      slice;

    struct h264_store_pic_t **listP[2];
    struct h264_store_pic_t **listB[2];
    struct h264d_input_ctx_t *p_Inp;
    struct h264_dec_ctx_t    *p_Dec;
    struct h264d_video_ctx_t *p_Vid;   //!< parameters for video decoder

    RK_S64                    last_pts;
    RK_S64                    last_dts;
    RK_S64                    curr_pts;
    RK_S64                    curr_dts;
    //!< malloc buffer for current slice
    RK_S32                    modification_of_pic_nums_idc[2][MAX_REORDER_TIMES];
    RK_S32                    abs_diff_pic_num_minus1[2][MAX_REORDER_TIMES];
    RK_S32                    long_term_pic_idx[2][MAX_REORDER_TIMES];
    RK_S32                    abs_diff_view_idx_minus1[2][MAX_REORDER_TIMES];

    struct h264_drpm_t        dec_ref_pic_marking_buffer[MAX_MARKING_TIMES];
} H264dCurCtx_t;

//!< parameters for video decoder
typedef struct h264d_video_ctx_t {
    struct h264_sps_t            spsSet[MAXSPS];      //!< MAXSPS, all sps storage
    struct h264_subsps_t         subspsSet[MAXSPS];   //!< MAXSPS, all subpps storage
    struct h264_pps_t            ppsSet[MAXPPS];      //!< MAXPPS, all pps storage
    struct h264_sps_t            *active_sps;
    struct h264_subsps_t         *active_subsps;
    struct h264_pps_t            *active_pps;
    struct h264_dec_ctx_t        *p_Dec;  //!< H264_DecCtx_t
    struct h264d_input_ctx_t     *p_Inp;  //!< H264_InputParameters
    struct h264d_cur_ctx_t       *p_Cur;  //!< H264_CurParameters
    struct h264_dpb_buf_t        *p_Dpb_layer[MAX_NUM_DPB_LAYERS];
    struct h264_store_pic_t      *dec_pic;    //!< current decoder picture
    struct h264_store_pic_t      *last_pic;

    struct h264_store_pic_t      *no_ref_pic; //!< no reference picture
    struct h264_frame_store_t    out_buffer;
    struct h264_dpb_mark_t       *active_dpb_mark[MAX_NUM_DPB_LAYERS];  //!< acitve_dpb_memory

    struct h264_store_pic_t      old_pic;
    struct h264_old_slice_par_t  old_slice;
    RK_S32    *qmatrix[12];  //!< scanlist pointer
    RK_U32    stream_size;
    RK_S32    last_toppoc[MAX_NUM_DPB_LAYERS];
    RK_S32    last_bottompoc[MAX_NUM_DPB_LAYERS];
    RK_S32    last_framepoc[MAX_NUM_DPB_LAYERS];
    RK_S32    last_thispoc[MAX_NUM_DPB_LAYERS];
    //!<
    RK_S32     profile_idc;                                       // u(8)
    RK_S32     slice_type;
    RK_S32     structure;
    RK_S32     iNumOfSlicesDecoded;
    RK_S32     no_output_of_prior_pics_flag;
    RK_S32     last_has_mmco_5;
    RK_S32     max_frame_num;
    RK_S32     active_sps_id[MAX_NUM_DPB_LAYERS];
    RK_U32     PicWidthInMbs;
    RK_U32     FrameHeightInMbs;
    RK_S32     yuv_format;
    RK_U32     bit_depth_chroma;
    RK_U32     bit_depth_luma;
    RK_S32     width;
    RK_S32     height;
    RK_U32     width_after_crop;
    RK_U32     height_after_crop;
    RK_S32     width_cr;                               //!< width chroma
    RK_S32     height_cr;                              //!< height chroma
    RK_S32     last_pic_structure;
    RK_S32     last_pic_bottom_field;
    RK_S32     last_pic_width_in_mbs_minus1[2];
    RK_S32     last_pic_height_in_map_units_minus1[2];
    RK_S32     last_profile_idc[2];
    RK_S32     last_level_idc[2];
    RK_S32     PrevPicOrderCntMsb;
    RK_S32     PrevPicOrderCntLsb;
    RK_U32     PreviousFrameNum;
    RK_U32     FrameNumOffset;
    RK_S32     PreviousFrameNumOffset;
    RK_S32     ExpectedDeltaPerPicOrderCntCycle;
    RK_S32     ExpectedPicOrderCnt;
    RK_S32     PicOrderCntCycleCnt;
    RK_S32     FrameNumInPicOrderCntCycle;
    RK_S32     ThisPOC;
    RK_S32     type;
    //!< for control running
    RK_S32     have_outpicture_flag;
    RK_S32     exit_picture_flag;
    RK_S32     active_mvc_sps_flag;
    //!< for error tolerance
    RK_U32     g_framecnt;
    RK_U32     dpb_size[MAX_NUM_DPB_LAYERS];

} H264dVideoCtx_t;

typedef struct h264d_mem_t {
    struct h264_dpb_mark_t     dpb_mark[MAX_MARK_SIZE];         //!< for fpga register check, dpb mark
    struct h264_dpb_info_t     dpb_info[MAX_DPB_SIZE];         //!< 16
    struct h264_refpic_info_t  refpic_info_p[MAX_REF_SIZE];    //!< 32
    struct h264_refpic_info_t  refpic_info_b[2][MAX_REF_SIZE];   //!< [2][32]
    struct h264d_dxva_ctx_t    dxva_ctx;
} H264_DecMem_t;

//!< nalu state used in read nalu
typedef enum nalu_state_tpye {
    NALU_NULL = 0,

    StreamError,
    HaveNoStream,
    NaluNotSupport,
    ReadNaluError,
    StartofNalu,
    EndofStream,
    MvcDisAble,
    MidOfNalu,
    EndOfNalu,
    StartOfPicture,
    StartOfSlice,
    SkipNALU,
    NALU_SPS,
    NALU_SubSPS,
    NALU_PPS,
    NALU_SEI,

    NALU_MAX,
} NALU_STATUS;

//!< slice state used in parse loop
typedef enum slice_state_type {
    SliceSTATE_NULL = 0,
    SliceSTATE_IDLE,

    SliceSTATE_ResetSlice,
    SliceSTATE_ReadNalu,
    SliceSTATE_ParseNalu,
    SliceSTATE_InitPicture,
    ////SliceSTATE_InitSlice,
    SliceSTATE_GetSliceData,
    //SliceSTATE_RegisterOneSlice,
    SliceSTATE_RegisterOneFrame,
    SliceSTATE_Error,
    SliceSTATE_MAX,

} SLICE_STATUS;

typedef struct h264_err_ctx_t {
    RK_U32    un_spt_flag;
    RK_U32    cur_err_flag;    //!< current decoded picture error
    RK_U32    used_ref_flag;
    RK_U32    dpb_err_flag;    //!< dpb storage had error

    RK_U32    i_slice_no;
    RK_S32    first_iframe_poc;
} H264dErrCtx_t;
//!< decoder video parameter
typedef struct h264_dec_ctx_t {
    struct h264d_mem_t        *mem;
    struct h264_dpb_mark_t    *dpb_mark;         //!< for write out, MAX_DPB_SIZE
    struct h264_dpb_info_t    *dpb_info;         //!< 16
    struct h264_refpic_info_t *refpic_info_p;    //!< 32
    struct h264_refpic_info_t *refpic_info_b[2]; //!< [2][32]
    struct h264d_dxva_ctx_t   *dxva_ctx;

    struct h264d_input_ctx_t  *p_Inp;
    struct h264d_cur_ctx_t    *p_Cur;            //!< current parameters, use in read nalu
    struct h264d_video_ctx_t  *p_Vid;            //!< parameters for video decoder
    RK_U32                     spt_decode_mtds;  //!< support decoder methods
    NALU_STATUS                nalu_ret;         //!< current nalu state
    SLICE_STATUS               next_state;       //!< RKV_SLICE_STATUS
    RK_U8                      have_slice_data;
    RK_U8                      is_new_frame;
    RK_U8                      is_parser_end;
    RK_U8                      mvc_valid;

    //!< add
    MppBufSlots                frame_slots;   //!< corresponding to dpb_mark
    MppBufSlots                packet_slots;

    MppPacket                  task_pkt;

    RK_S64                     task_pts;
    RK_U32                     task_eos;
    HalDecTask                *in_task;
    RK_S32                     last_frame_slot_idx;
    RK_U32                     disable_error;
    RK_U32                     immediate_out;
    struct h264_err_ctx_t      errctx;
} H264_DecCtx_t;

#endif /* __H264D_GLOBAL_H__ */
