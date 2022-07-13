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

#ifndef __AVS2D_GLOBAL_H__
#define __AVS2D_GLOBAL_H__

#include "mpp_list.h"
#include "parser_api.h"
#include "mpp_bitread.h"

#include "avs2d_syntax.h"
#include "avs2d_api.h"

#define AVS2D_DBG_ERROR             (0x00000001)
#define AVS2D_DBG_ASSERT            (0x00000002)
#define AVS2D_DBG_WARNNING          (0x00000004)
#define AVS2D_DBG_LOG               (0x00000008)

#define AVS2D_DBG_INPUT             (0x00000010)   //!< input packet
#define AVS2D_DBG_TIME              (0x00000020)   //!< input packet
#define AVS2D_DBG_DPB               (0x00000040)

#define AVS2D_DBG_CALLBACK          (0x00008000)

extern RK_U32 avs2d_parse_debug;

#define AVS2D_PARSE_TRACE(fmt, ...)\
do {\
    if (AVS2D_DBG_LOG & avs2d_parse_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)

#define avs2d_dbg_dpb(fmt, ...)\
do {\
    if (AVS2D_DBG_DPB & avs2d_parse_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define AVS2D_DBG(level, fmt, ...)\
do {\
    if (level & avs2d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

//!< input check
#define INP_CHECK(ret, val, ...)\
do {\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        AVS2D_DBG(AVS2D_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)

//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do {\
    if(!(val)) {\
        ret = MPP_ERR_MALLOC;\
        mpp_err_f("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

//!< function return check
#define FUN_CHECK(val)\
do {\
    if ((val) < 0) {\
        AVS2D_DBG(AVS2D_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

//!< read & check marker bit
#define READ_MARKER_BIT(bitctx)\
do {\
    RK_U8 bval;\
    READ_ONEBIT(bitctx, &bval);\
    if ((bval) != 1) {\
        AVS2D_DBG(AVS2D_DBG_WARNNING, "expected marker_bit 1 while received 0(%d).\n", __LINE__); \
}} while (0)

#define MAX_HEADER_SIZE                     (2*1024)
#define MAX_STREAM_SIZE                     (2*1024*1024)

#define AVS2_START_CODE                     (0x00000100)

//!< NALU type
#define AVS2_SLICE_MIN_START_CODE           (AVS2_START_CODE | 0x00)
#define AVS2_SLICE_MAX_START_CODE           (AVS2_START_CODE | 0x8F)
#define AVS2_VIDEO_SEQUENCE_START_CODE      (AVS2_START_CODE | 0xb0)
#define AVS2_VIDEO_SEQUENCE_END_CODE        0x000001B1
#define AVS2_USER_DATA_START_CODE           0x000001B2
#define AVS2_I_PICTURE_START_CODE           0x000001B3
#define AVS2_EXTENSION_START_CODE           0x000001B5
#define AVS2_PB_PICTURE_START_CODE          0x000001B6
#define AVS2_VIDEO_EDIT_CODE                0x000001B7

//!< SEQ/PIC EXTENSION 4bit
#define AVS2_SEQUENCE_DISPLAY_EXTENTION     0x00000002
#define AVS2_COPYRIGHT_EXTENTION            0x00000004
#define AVS2_PICTURE_DISPLAY_EXTENTION      0x00000007
#define AVS2_CAMERA_PARAMETERS_EXTENTION    0x0000000B

//!< profile
#define MAIN_PICTURE_PROFILE            0x12
#define MAIN_PROFILE                    0x20
#define MAIN10_PROFILE                  0x22

//!< max reference frame number
#define AVS2_MAX_REFS                   7
//!< max rps number
#define AVS2_MAX_RPS_NUM                32
#define AVS2_MAX_BUF_NUM                19
#define AVS2_MAX_DPB_SIZE               15
#define AVS2_DOI_CYCLE                  256

#define ALF_MAX_FILTERS                 16
#define ALF_MAX_COEFS                   9

#define UNDETAILED                      0
#define DETAILED                        1
#define WQP_SIZE                        6
#define NO_VAL                         -1


//!< chroma format
enum avs2d_chroma_format_e {
    CHROMA_400  = 0,
    CHROMA_420  = 1,
    CHROMA_422  = 2,
    CHROMA_444  = 3
};

//!< picture type
typedef enum avs2d_picture_type_e {
    I_PICTURE   = 0,
    P_PICTURE   = 1,
    B_PICTURE   = 2,
    F_PICTURE   = 3,
    G_PICTURE   = 4,
    GB_PICTURE  = 5,
    S_PICTURE   = 6,
} Avs2dPicType;

#define PICTURE_TYPE_TO_CHAR(x) ((x) == 0 ? 'I' : ((x) == 1 ? 'P' : ((x) == 2 ? 'B' : ((x) == 3 ? 'F' : ((x) == 4 ? 'G' : ((x) == 5 ? 'g' : ((x) == 6 ? 'S' : 'E')))))))

typedef struct avs2d_nalu_t {
    RK_U32      header;
    RK_U32      size;
    RK_U32      length;
    RK_U8      *pdata;
    RK_U8       start_pos;
    RK_U8       eof; //!< end of frame stream
} Avs2dNalu_t;

//!< reference picture set (RPS)
typedef struct avs2d_rps_t {
    RK_U8       ref_pic[AVS2_MAX_REFS]; //!< delta DOI of ref pic
    RK_U8       remove_pic[8];          //!< delta DOI of removed pic
    RK_U8       num_of_ref;             //!< number of reference picture
    RK_U8       num_to_remove;          //!< number of removed picture
    RK_U8       refered_by_others;      //!< referenced by others
    RK_U32      reserved;               //!< reserved 4 bytes
} Avs2dRps_t;

//!< sequence set information
typedef struct avs2d_sequence_header_t {
    RK_U8       profile_id;
    RK_U8       level_id;
    RK_U8       progressive_sequence;
    RK_U8       field_coded_sequence;
    RK_U32      horizontal_size;
    RK_U32      vertical_size;

    RK_U8       chroma_format;
    RK_U8       sample_precision;
    RK_U8       encoding_precision;
    RK_U8       bit_depth;
    RK_U8       aspect_ratio;
    RK_U8       frame_rate_code;
    RK_U32      bit_rate;
    RK_U8       low_delay;
    RK_U8       enable_temporal_id;
    RK_U32      bbv_buffer_size;
    RK_U8       lcu_size;

    RK_U8       enable_weighted_quant;    //!< weight quant enable?
    RK_U8       enable_background_picture;//!< background picture enabled?
    RK_U8       enable_mhp_skip;          //!< mhpskip enabled?
    RK_U8       enable_dhp;               //!< dhp enabled?
    RK_U8       enable_wsm;               //!< wsm enabled?
    RK_U8       enable_amp;               //!< AMP(asymmetric motion partitions) enabled?
    RK_U8       enable_nsqt;              //!< use NSQT?
    RK_U8       enable_nsip;              //!< use NSIP?
    RK_U8       enable_2nd_transform;     //!< secondary transform enabled?
    RK_U8       enable_sao;               //!< SAO enabled?
    RK_U8       enable_alf;               //!< ALF enabled?
    RK_U8       enable_pmvr;              //!< PMVR enabled?
    RK_U8       enable_clf;               //!< cross loop filter flag
    RK_U8       picture_reorder_delay;    //!< picture reorder delay
    RK_U8       num_of_rps;               //!< rps set number

    Avs2dRps_t  seq_rps[AVS2_MAX_RPS_NUM];//!< RPS at sequence level
    RK_U32      seq_wq_matrix[2][64];     //!< sequence base weighting quantization matrix

} Avs2dSeqHeader_t;

//!< sequence display extension header
typedef struct avs2d_sequence_extension_header_t {
    RK_U8       video_format;
    RK_U8       sample_range;
    RK_U8       color_description;
    RK_U32      color_primaries;
    RK_U32      transfer_characteristics;
    RK_U32      matrix_coefficients;
    RK_U32      display_horizontal_size;
    RK_U32      display_vertical_size;
    RK_U8       td_mode_flag;
    RK_U32      td_packing_mode;
    RK_U8       view_reverse_flag;
} Avs2dSeqExtHeader_t;

//!< intra/inter picture header
typedef struct avs2d_picture_header {
    Avs2dPicType picture_type;
    RK_U8       picture_coding_type;
    RK_U32      bbv_delay;
    RK_U8       time_code_flag;
    RK_U32      time_code;
    RK_U8       background_picture_flag;
    RK_U8       background_picture_output_flag;
    RK_U8       background_pred_flag;
    RK_U8       background_reference_flag;

    RK_S32      doi;
    RK_S32      poi;
    RK_U8       temporal_id;
    RK_U8       picture_output_delay;
    RK_U32      bbv_check_times;
    RK_U8       progressive_frame;
    RK_U8       picture_structure;
    RK_U8       top_field_first;
    RK_U8       repeat_first_field;
    RK_U8       is_top_field;
    RK_U8       fixed_picture_qp;
    RK_U8       picture_qp;

    RK_U8       enable_random_decodable;
    RK_U8       enable_loop_filter;
    RK_U8       loop_filter_parameter_flag;
    RK_S8       alpha_c_offset;
    RK_S8       beta_offset;
    RK_U8       enable_chroma_quant_param;
    RK_S8       chroma_quant_param_delta_cb;
    RK_S8       chroma_quant_param_delta_cr;
    RK_U8       enable_pic_weight_quant;
    RK_U8       pic_wq_data_index;
    RK_U8       wq_param_index;
    RK_U8       wq_model;
    RK_S32      wq_param_delta1[WQP_SIZE];
    RK_S32      wq_param_delta2[WQP_SIZE];

    RK_U8       enable_pic_alf_y;
    RK_U8       enable_pic_alf_cb;
    RK_U8       enable_pic_alf_cr;
    RK_U8       alf_filter_num;
    RK_U8       alf_filter_pattern[ALF_MAX_FILTERS];
    RK_U8       alf_coeff_idx_tab[ALF_MAX_FILTERS];
    RK_S32      alf_coeff_y[ALF_MAX_FILTERS][ALF_MAX_COEFS];
    RK_S32      alf_coeff_cb[ALF_MAX_COEFS];
    RK_S32      alf_coeff_cr[ALF_MAX_COEFS];

    RK_U32      pic_wq_matrix[2][64];     //!< picture weighting quantization matrix
    RK_U32      pic_wq_param[WQP_SIZE];
} Avs2dPicHeader_t;

typedef struct avs2d_frame_t {
    MppFrame    frame;
    RK_U32      frame_mode;         //!< mode used for display
    RK_U32      frame_coding_mode;  //!< mode used for decoding, Field/Frame
    Avs2dPicType picture_type;       //!< I/G/GB/P/F/B/S
    RK_S32      slot_idx;
    RK_S32      doi;
    RK_S32      poi;
    RK_S32      out_delay;
    RK_U8       invisible;          //!< GB visible = 0
    RK_U8       scene_frame_flag;   //!< G/GB
    RK_U8       intra_frame_flag;   //!< I/G/GB
    RK_U8       refered_bg_frame;   //!< ref G/GB
    RK_U8       refered_by_others;
    RK_U8       refered_by_scene;
    RK_U8       error_flag;
    RK_U8       is_output;      // mark for picture has been output to display queue
} Avs2dFrame_t;

typedef enum avs2d_nal_type {

    NAL_SEQ_START   = 0xb0,
    NAL_SEQ_END     = 0xb1,
    NAL_USER_DATA   = 0xb2,
    NAL_INTRA_PIC   = 0xb3,
    NAL_EXTENSION   = 0xb5,
    NAL_INTER_PIC   = 0xb6,
    NAL_VIDEO_EDIT  = 0xb7
} Avs2dNalType;

typedef struct avs2d_nal_t {
    RK_U8 *data;
    RK_U32 size;
    RK_U32 nal_type;
    RK_U32 start_found;
    RK_U32 end_found;
} Avs2dNal;

typedef struct avs2d_stream_buf_t {
    RK_U8      *pbuf;
    RK_U32      size;   //Total buffer size
    RK_U32      len;    // Used length
} Avs2dStreamBuf_t;

typedef struct avs2d_memory_t {
    Avs2dStreamBuf_t        headerbuf;
    Avs2dStreamBuf_t        streambuf;
} Avs2dMemory_t;

typedef struct avs2d_frame_mgr_t {
    RK_U32                  dpb_size;
    RK_U32                  used_size;
    Avs2dFrame_t          **dpb;
    RK_U8                   num_of_ref;
    Avs2dFrame_t           *refs[AVS2_MAX_REFS];
    Avs2dFrame_t           *scene_ref;
    Avs2dFrame_t           *cur_frm;
    Avs2dRps_t              cur_rps;

    RK_S32                  prev_doi;
    RK_S32                  output_poi;
    RK_S32                  tr_wrap_cnt;
    RK_U8                   poi_interval;
    RK_U8                   initial_flag;
} Avs2dFrameMgr_t;

#define END_NOT_FOUND (-100)
#define START_NOT_FOUND (-101)
#define MPP_INPUT_BUFFER_PADDING_SIZE 8
#define AVS2D_PACKET_SPLIT_LAST_KEPT_LENGTH     (3)
#define AVS2D_PACKET_SPLIT_CHECKER_BUFFER_SIZE  (7)
#define AVS2D_START_CODE_SIZE                   (4)

//!< decoder parameters
typedef struct avs2_dec_ctx_t {
    MppBufSlots             frame_slots;
    MppBufSlots             packet_slots;

    MppPacket               task_pkt;
    Avs2dMemory_t          *mem; //!< resotre slice data to decoder
    Avs2dStreamBuf_t       *p_stream;
    Avs2dStreamBuf_t       *p_header;

    //-------- input ---------------
    ParserCfg               init;
    RK_U64                  frame_no;
    RK_U64                  pkt_no;
    RK_U8                   has_get_eos;

    //-------- current --------------
    Avs2dNalu_t            *nal; //!< current nalu
    RK_U32                  nal_cnt;

    //--------  video  --------------
    BitReadCtx_t            bitctx;
    Avs2dSeqHeader_t        vsh;
    Avs2dSeqExtHeader_t     exh;
    Avs2dPicHeader_t        ph;

    Avs2dSyntax_t           syntax;
    Avs2dFrameMgr_t         frm_mgr;
    RK_U32                  cur_wq_matrix[2][64];

    //!<------------------------------------
    RK_U32                  got_vsh;
    RK_U32                  got_exh;
    RK_U32                  got_keyframe;
    RK_U32                  vec_flag; //!< video_edit_code_flag
    RK_U8                   enable_wq;//!< seq&pic weight quant enable
    RK_U32                  prev_start_code;
    RK_U32                  new_seq_flag;
    RK_U8                   prev_tail_data[AVS2D_PACKET_SPLIT_CHECKER_BUFFER_SIZE]; // store the last 3 bytes at the lowest addr
    RK_U32                  prev_state;
    RK_U32                  new_frame_flag;
} Avs2dCtx_t;

#endif /*__AVS2D_GLOBAL_H__*/
