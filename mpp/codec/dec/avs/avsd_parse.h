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

#ifndef __AVSD_PARSE_H__
#define __AVSD_PARSE_H__

#include "parser_api.h"
#include "mpp_bitread.h"

#include "avsd_syntax.h"
#include "avsd_api.h"

#define AVSD_DBG_ERROR             (0x00000001)
#define AVSD_DBG_ASSERT            (0x00000002)
#define AVSD_DBG_WARNNING          (0x00000004)
#define AVSD_DBG_LOG               (0x00000008)

#define AVSD_DBG_INPUT             (0x00000010)   //!< input packet
#define AVSD_DBG_TIME              (0x00000020)   //!< input packet

#define AVSD_DBG_CALLBACK          (0x00008000)

extern RK_U32 avsd_parse_debug;

#define AVSD_PARSE_TRACE(fmt, ...)\
do {\
    if (AVSD_DBG_LOG & avsd_parse_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define AVSD_DBG(level, fmt, ...)\
do {\
    if (level & avsd_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        AVSD_DBG(AVSD_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)

//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if(!(val)) {\
        ret = MPP_ERR_MALLOC;\
        mpp_err_f("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

//!< function return check
#define FUN_CHECK(val)\
do{\
if ((val) < 0) {\
        AVSD_DBG(AVSD_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)

#define MAX_HEADER_SIZE     (2*1024)
#define MAX_STREAM_SIZE     (2*1024*1024)

//!< NALU type
#define SEQUENCE_DISPLAY_EXTENTION     0x00000002
#define COPYRIGHT_EXTENTION            0x00000004
#define PICTURE_DISPLAY_EXTENTION      0x00000007
#define CAMERA_PARAMETERS_EXTENTION    0x0000000B
#define SLICE_MIN_START_CODE           0x00000100
#define SLICE_MAX_START_CODE           0x000001AF
#define VIDEO_SEQUENCE_START_CODE      0x000001B0
#define VIDEO_SEQUENCE_END_CODE        0x000001B1
#define USER_DATA_CODE                 0x000001B2
#define I_PICUTRE_START_CODE           0x000001B3
#define EXTENSION_START_CODE           0x000001B5
#define PB_PICUTRE_START_CODE          0x000001B6
#define VIDEO_EDIT_CODE                0x000001B7
#define VIDEO_TIME_CODE                0x000001E0



#define EDGE_SIZE                     16
#define MB_SIZE                       16
#define YUV420                         0

//!< picture type
enum avsd_picture_type_e {
    I_PICTURE = 0,
    P_PICTURE = 1,
    B_PICTURE = 2
};

typedef struct avsd_nalu_t {
    RK_U32 header;
    RK_U32 size;
    RK_U32 length;
    RK_U8 *pdata;
    RK_U8  start_pos;
    RK_U8  eof; //!< end of frame stream
} AvsdNalu_t;


typedef struct avsd_sequence_header_t {
    RK_U8  profile_id;
    RK_U8  level_id;
    RK_U8  progressive_sequence;
    RK_U32 horizontal_size;
    RK_U32 vertical_size;

    RK_U8  chroma_format;
    RK_U8  sample_precision;
    RK_U8  aspect_ratio;
    RK_U8  frame_rate_code;
    RK_U32 bit_rate;
    RK_U8  low_delay;
    RK_U32 bbv_buffer_size;
} AvsdSeqHeader_t;

//!< sequence display extension header
typedef struct avsd_seqence_extension_header_t {
    RK_U32 video_format;
    RK_U32 sample_range;
    RK_U32 color_description;
    RK_U32 color_primaries;
    RK_U32 transfer_characteristics;
    RK_U32 matrix_coefficients;
    RK_U32 display_horizontalSize;
    RK_U32 display_verticalSize;
} AvsdSeqExtHeader_t;

typedef struct avsd_picture_header {
    RK_U16 bbv_delay;
    RK_U16 bbv_delay_extension;

    RK_U8  picture_coding_type;
    RK_U8  time_code_flag;
    RK_U32 time_code;
    RK_U8  picture_distance;
    RK_U32 bbv_check_times;
    RK_U8  progressive_frame;
    RK_U8  picture_structure;
    RK_U8  advanced_pred_mode_disable;
    RK_U8  top_field_first;
    RK_U8  repeat_first_field;
    RK_U8  fixed_picture_qp;
    RK_U8  picture_qp;
    RK_U8  picture_reference_flag;

    RK_U8  no_forward_reference_flag;
    RK_U8  pb_field_enhanced_flag;

    RK_U8  skip_mode_flag;
    RK_U8  loop_filter_disable;
    RK_U8  loop_filter_parameter_flag;
    RK_U32 alpha_c_offset;
    RK_U32 beta_offset;

    RK_U8  weighting_quant_flag;
    RK_U8  chroma_quant_param_disable;
    RK_S32 chroma_quant_param_delta_cb;
    RK_S32 chroma_quant_param_delta_cr;
    RK_U32 weighting_quant_param_index;
    RK_U32 weighting_quant_model;
    RK_S32 weighting_quant_param_delta1[6];
    RK_S32 weighting_quant_param_delta2[6];
    RK_S32 weighting_quant_param[6];
    RK_U8  aec_enable;
} AvsdPicHeader_t;

typedef struct avsd_frame_t {
    RK_U32   valid;

    RK_U32   pic_type;

    RK_U32   frame_mode; //!< set mpp frame flag

    RK_U32   width;
    RK_U32   height;
    RK_U32   ver_stride;
    RK_U32   hor_stride;

    RK_S64   pts;
    RK_S64   dts;

    RK_U32   had_display;
    RK_S32   slot_idx;
} AvsdFrame_t;



typedef struct avsd_stream_buf_t {
    RK_U8 *pbuf;
    RK_U32 size;
    RK_U32 len;
} AvsdStreamBuf_t;

typedef struct avsd_memory_t {
    struct avsd_stream_buf_t   headerbuf;
    struct avsd_stream_buf_t   streambuf;
    struct avsd_syntax_t       syntax;
    struct avsd_frame_t        save[3];
} AvsdMemory_t;


//!< decoder parameters
typedef struct avs_dec_ctx_t {
    MppBufSlots                frame_slots;
    MppBufSlots                packet_slots;

    MppPacket                  task_pkt;
    struct avsd_memory_t      *mem; //!< resotre slice data to decoder
    struct avsd_stream_buf_t  *p_stream;
    struct avsd_stream_buf_t  *p_header;
    //-------- input ---------------
    RK_U32      frame_no;
    ParserCfg   init;
    RK_U8  has_get_eos;
    RK_U64 pkt_no;
    //-------- current --------------
    struct avsd_nalu_t      *nal; //!< current nalu
    //--------  video  --------------
    struct bitread_ctx_t     bitctx;
    AvsdSeqHeader_t          vsh;
    AvsdSeqExtHeader_t       ext;

    AvsdPicHeader_t          ph;
    AvsdSyntax_t            *syn;

    AvsdFrame_t             *dpb[2];    //!< 2 refer frames or 4 refer field
    AvsdFrame_t             *cur;       //!< for decoder field
    //!<------------------------------------
    RK_U32                   got_vsh;
    RK_U32                   got_keyframe;
    RK_U32                   mb_width;
    RK_U32                   mb_height;
    RK_U32                   vec_flag; //!< video_edit_code_flag
} AvsdCtx_t;



#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET avsd_free_resource(AvsdCtx_t *p_dec);
MPP_RET avsd_reset_parameters(AvsdCtx_t *p_dec);

MPP_RET avsd_set_dpb(AvsdCtx_t *p_dec, HalDecTask *task);
MPP_RET avsd_commit_syntaxs(AvsdSyntax_t *syn, HalDecTask *task);
MPP_RET avsd_fill_parameters(AvsdCtx_t *p_dec, AvsdSyntax_t *syn);

MPP_RET avsd_update_dpb(AvsdCtx_t *p_dec);

MPP_RET avsd_parse_prepare(AvsdCtx_t *p_dec, MppPacket  *pkt, HalDecTask *task);
MPP_RET avsd_parse_stream(AvsdCtx_t *p_dec, HalDecTask *task);

#ifdef  __cplusplus
}
#endif

#endif /*__AVSD_PARSE_H__*/
