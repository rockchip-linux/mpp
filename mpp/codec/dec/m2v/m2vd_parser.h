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

/*
 * @file        m2vd_parser.h
 * @brief
 * @author      lks(lks@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#ifndef __M2VD_PARSER_H__
#define __M2VD_PARSER_H__

#include "mpp_mem.h"
#include "mpp_bitread.h"

#include "mpp_dec.h"

#include "m2vd_syntax.h"
#include "m2vd_com.h"

#define M2VD_SKIP_ERROR_FRAME_EN  1
#define M2VD_DEC_OK                   0
#define M2VD_IFNO_CHG                 0x10
#define M2VD_DEC_UNSURPORT            -1
#define M2VD_DEC_MEMORY_FAIL      -2
#define M2VD_DEC_FILE_END         -3
#define M2VD_HW_DEC_UNKNOW_ERROR     -4
#define M2VD_DEC_PICHEAD_OK       1
#define M2VD_DEC_STREAM_END       2

#define TEMP_DBG_FM     0


#define PICTURE_START_CODE      0x100
#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF
#define USER_DATA_START_CODE    0x1B2
#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_ERROR_CODE     0x1B4
#define EXTENSION_START_CODE    0x1B5
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8
#define SYSTEM_START_CODE_MIN   0x1B9
#define SYSTEM_START_CODE_MAX   0x1FF

#define ISO_END_CODE            0x1B9
#define PACK_START_CODE         0x1BA
#define SYSTEM_START_CODE       0x1BB

#define VIDEO_ELEMENTARY_STREAM 0x1e0
#define NO_MORE_STREAM      0xffffffff

#define SEQUENCE_EXTENSION_ID                    1
#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define QUANT_MATRIX_EXTENSION_ID                3
#define COPYRIGHT_EXTENSION_ID                   4
#define SEQUENCE_SCALABLE_EXTENSION_ID           5
#define PICTURE_DISPLAY_EXTENSION_ID             7
#define PICTURE_CODING_EXTENSION_ID              8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID    9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  10


typedef enum M2VDPicCodingType_e {
    M2VD_CODING_TYPE_I = 1,
    M2VD_CODING_TYPE_P = 2,
    M2VD_CODING_TYPE_B = 3,
    M2VD_CODING_TYPE_D = 4
} M2VDPicCodingType;

typedef enum M2VDPicStruct_e {
    M2VD_PIC_STRUCT_TOP_FIELD    = 1,
    M2VD_PIC_STRUCT_BOTTOM_FIELD = 2,
    M2VD_PIC_STRUCT_FRAME        = 3
} M2VDPicStruct;

#define     M2VD_DBG_FILE_NUM               1
#define     M2VD_DBG_FILE_W                 1
#define     M2VD_BUF_SIZE_BITMEM            (512 * 1024)
#define     M2VD_BUF_SIZE_QPTAB            (256)
#define     M2V_OUT_FLAG                   0x1;

typedef enum M2VDBufGrpIdx_t {
    M2VD_BUF_GRP_BITMEM,
    M2VD_BUF_GRP_QPTAB,
    M2VD_BUF_GRP_BUTT,
} M2VDBufGrpIdx;

typedef struct M2VFrameHead_t {
    RK_U32          frameNumber;
    RK_U32          tr;
    RK_U32          picCodingType;
    RK_U32          totalMbInFrame;
    RK_U32          frameWidth; /* in macro blocks */
    RK_U32          frameHeight;    /* in macro blocks */
    RK_U32          mb_width;
    RK_U32          mb_height;
    RK_U32          vlcSet;
    RK_U32          qp;
    RK_U32          frame_codelen;
    // VPU_FRAME       *frame_space;
    MppFrame        f;
    RK_U32          flags;
    RK_S32          slot_index;
    //RK_U32          error_info;
} M2VDFrameHead;


/* ISO/IEC 13818-2 section 6.2.2.1:  sequence_header() */
typedef struct M2VDHeadSeq_t {
    RK_U32          decode_width; //horizontal_size_value
    RK_U32          decode_height; //vertical_size_value
    RK_S32          aspect_ratio_information;
    RK_S32          frame_rate_code;
    RK_S32          bit_rate_value;
    RK_S32          vbv_buffer_size;
    RK_S32          constrained_parameters_flag;
    RK_U32          load_intra_quantizer_matrix; //[TEMP]
    RK_U32          load_non_intra_quantizer_matrix; //[TEMP]
    RK_U8           *pIntra_table; //intra_quantiser_matrix[64]
    RK_U8           *pInter_table; //non_intra_quantiser_matrix[64]
} M2VDHeadSeq;

/* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
typedef struct M2VDHeadSeqExt_t {
    RK_U32             horizontal_size_extension; //[TEMP]
    RK_U32             vertical_size_extension; //[TEMP]
    RK_U32             bit_rate_extension; //[TEMP]
    RK_U32             vbv_buffer_size_extension; //[TEMP]
    RK_S32             profile_and_level_indication;
    RK_S32             progressive_sequence;
    RK_S32             chroma_format;
    RK_S32             low_delay;
    RK_S32             frame_rate_extension_n;
    RK_S32             frame_rate_extension_d;
} M2VDHeadSeqExt;

/* ISO/IEC 13818-2 section 6.2.2.6: group_of_pictures_header()  */
typedef struct M2VDHeadGop_t {
    RK_S32             drop_flag;
    RK_S32             hour;
    RK_S32             minute;
    RK_S32             sec;
    RK_S32             frame;
    RK_S32             closed_gop;
    RK_S32             broken_link;
} M2VDHeadGop;


/* ISO/IEC 13818-2 section 6.2.3: picture_header() */
typedef struct M2VDHeadPic_t {
    RK_S32             temporal_reference;
    RK_S32             picture_coding_type;
    RK_S32             pre_picture_coding_type;
    RK_S32             vbv_delay;
    RK_S32             full_pel_forward_vector;
    RK_S32             forward_f_code;
    RK_S32             full_pel_backward_vector;
    RK_S32             backward_f_code;
    RK_S32             pre_temporal_reference;
} M2VDHeadPic;


/* ISO/IEC 13818-2 section 6.2.2.4:  sequence_display_extension() */
typedef struct M2VDHeadSeqDispExt_t {
    RK_S32             video_format;
    RK_S32             color_description;
    RK_S32             color_primaries;
    RK_S32             transfer_characteristics;
    RK_S32             matrix_coefficients;
} M2VDHeadSeqDispExt;

/* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header */
typedef struct M2VDHeadPicCodeExt_t {
    RK_S32             f_code[2][2];
    RK_S32             intra_dc_precision;
    RK_S32             picture_structure;
    RK_S32             top_field_first;
    RK_S32             frame_pred_frame_dct;
    RK_S32             concealment_motion_vectors;
    RK_S32             q_scale_type;
    RK_S32             intra_vlc_format;
    RK_S32             alternate_scan;
    RK_S32             repeat_first_field;
    RK_S32             chroma_420_type;
    RK_S32             progressive_frame;
    RK_S32             composite_display_flag;
    RK_S32             v_axis;
    RK_S32             field_sequence;
    RK_S32             sub_carrier;
    RK_S32             burst_amplitude;
    RK_S32             sub_carrier_phase;
} M2VDHeadPicCodeExt;


/* ISO/IEC 13818-2 section 6.2.3.3: picture_display_extension() header */
typedef struct M2VDHeadPicDispExt_t {
    RK_S32             frame_center_horizontal_offset[3];
    RK_S32             frame_center_vertical_offset[3];
} M2VDHeadPicDispExt;

typedef struct M2VDCombMem_t {
    MppBuffer hw_buf;
    RK_U8*    sw_buf;
    RK_U8     buf_size;
    RK_U8*    sw_pos;
    RK_U32    length;
} M2VDCombMem;

typedef struct M2VDParserContext_t {
    M2VDDxvaParam *dxva_ctx;
    BitReadCtx_t  *bitread_ctx;
    RK_U8           *bitstream_sw_buf;
    RK_U8           *qp_tab_sw_buf;
    RK_U32          max_stream_size;
    RK_U32          left_length;
    RK_U32          need_split;
    RK_U32          state;
    RK_U32          vop_header_found;

    RK_U32          frame_size;

    RK_U32          display_width;
    RK_U32          display_height;
    RK_U32          frame_width;
    RK_U32          frame_height;
    RK_U32          mb_width;
    RK_U32          mb_height;
    RK_U32          MPEG2_Flag;

    M2VDHeadSeq         seq_head;
    M2VDHeadSeqExt      seq_ext_head;
    M2VDHeadGop         gop_head;
    M2VDHeadPic         pic_head;
    M2VDHeadSeqDispExt  seq_disp_ext_head;
    M2VDHeadPicCodeExt  pic_code_ext_head;
    M2VDHeadPicDispExt  pic_disp_ext_head;

    RK_S32             resetFlag;

    RK_U32          PreGetFrameTime;
    RK_S32          Group_start_Time;
    RK_U32          GroupFrameCnt;
    RK_U32          pretemporal_reference;
    RK_U32          pretime_temporal;
    RK_U32          max_temporal_reference;
    RK_U32          PreChangeTime_index;
    RK_U32          frame_period;
    RK_U32          preframe_period;
    RK_U32          maxFrame_inGOP;
    RK_U32          ref_frame_cnt;
    long long       top_first_cnt;
    long long       bottom_first_cnt;
    RK_S32            mHeaderDecFlag;
    RK_S32            mExtraHeaderDecFlag;
    M2VDFrameHead     Framehead[3];
    M2VDFrameHead    *frame_ref0;
    M2VDFrameHead    *frame_ref1;
    M2VDFrameHead    *frame_cur;
#if M2VD_SKIP_ERROR_FRAME_EN
    RK_S32           mHwDecStatus;
#endif
    RK_S32           flush_dpb_eos;

    MppPacket       input_packet;
    RK_U32       eos;

    RK_S32 initFlag;
    RK_S32 decoder_err;

    MppBufSlots packet_slots;
    MppBufSlots frame_slots;
    RK_U32  cur_slot_index;

    RK_U64 pts;

    FILE *fp_dbg_file[M2VD_DBG_FILE_NUM];
    FILE *fp_dbg_yuv;
} M2VDParserContext;

MPP_RET  m2vd_parser_init   (void *ctx, ParserCfg *cfg);
MPP_RET  m2vd_parser_deinit (void *ctx);
MPP_RET  m2vd_parser_reset  (void *ctx);
MPP_RET  m2vd_parser_flush  (void *ctx);
MPP_RET  m2vd_parser_control(void *ctx, RK_S32 cmd_type, void *param);
MPP_RET  m2vd_parser_prepare(void *ctx, MppPacket pkt, HalDecTask *task);
MPP_RET  m2vd_parser_parse  (void *ctx, HalDecTask *task);
MPP_RET  m2vd_parser_callback(void *ctx, void *err_info);

#endif

