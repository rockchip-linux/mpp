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

#ifndef __M2VD_SYNTAX_H__
#define __M2VD_SYNTAX_H__

#include "rk_type.h"

typedef struct _DXVA_PicEntry_M2V {
    union {
        struct {
            RK_U8 Index7Bits     : 7;
            RK_U8 AssociatedFlag : 1;
        };
        RK_U8 bPicEntry;
    };
} DXVA_PicEntry_M2V;


/* ISO/IEC 13818-2 section 6.2.2.1:  sequence_Dxvaer() */
typedef struct M2VDDxvaSeq_t {
    RK_U32          decode_width; //horizontal_size_value
    RK_U32          decode_height; //vertical_size_value
    RK_S32          aspect_ratio_information;
    RK_S32          frame_rate_code;
    RK_S32          bit_rate_value;
    RK_S32          vbv_buffer_size;
    RK_S32          constrained_parameters_flag;
    RK_U32          load_intra_quantizer_matrix; //[TEMP]
    RK_U32          load_non_intra_quantizer_matrix; //[TEMP]
} M2VDDxvaSeq;

/* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
typedef struct M2VDDxvaSeqExt_t {
    RK_S32             profile_and_level_indication;
    RK_S32             progressive_sequence;
    RK_S32             chroma_format;
    RK_S32             low_delay;
    RK_S32             frame_rate_extension_n;
    RK_S32             frame_rate_extension_d;
} M2VDDxvaSeqExt;

/* ISO/IEC 13818-2 section 6.2.2.6: group_of_pictures_Dxvaer()  */
typedef struct M2VDDxvaGop_t {
    RK_S32             drop_flag;
    RK_S32             hour;
    RK_S32             minute;
    RK_S32             sec;
    RK_S32             frame;
    RK_S32             closed_gop;
    RK_S32             broken_link;
} M2VDDxvaGop;


/* ISO/IEC 13818-2 section 6.2.3: picture_Dxvaer() */
typedef struct M2VDDxvaPic_t {
    RK_S32             temporal_reference;
    RK_S32             picture_coding_type;
    RK_S32             pre_picture_coding_type;
    RK_S32             vbv_delay;
    RK_S32             full_pel_forward_vector;
    RK_S32             forward_f_code;
    RK_S32             full_pel_backward_vector;
    RK_S32             backward_f_code;
    RK_S32             pre_temporal_reference;
} M2VDDxvaPic;


/* ISO/IEC 13818-2 section 6.2.2.4:  sequence_display_extension() */
typedef struct M2VDDxvaSeqDispExt_t {
    RK_S32             video_format;
    RK_S32             color_description;
    RK_S32             color_primaries;
    RK_S32             transfer_characteristics;
    RK_S32             matrix_coefficients;
} M2VDDxvaSeqDispExt;

/* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() Dxvaer */
typedef struct M2VDDxvaPicCodeExt_t {
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
} M2VDDxvaPicCodeExt;


/* ISO/IEC 13818-2 section 6.2.3.3: picture_display_extension() Dxvaer */
typedef struct M2VDDxvaPicDispExt_t {
    RK_S32             frame_center_horizontal_offset[3];
    RK_S32             frame_center_vertical_offset[3];
} M2VDDxvaPicDispExt;


typedef struct M2VDDxvaParam_t {
    RK_U32              bitstream_length;
    RK_U32              bitstream_start_bit;
    RK_U32              bitstream_offset;
    RK_U8               *qp_tab;

    DXVA_PicEntry_M2V   CurrPic;
    DXVA_PicEntry_M2V   frame_refs[4];

    RK_U32              seq_ext_head_dec_flag;

    M2VDDxvaSeq         seq;
    M2VDDxvaSeqExt      seq_ext;
    M2VDDxvaGop         gop;
    M2VDDxvaPic         pic;
    M2VDDxvaSeqDispExt  seq_disp_ext;
    M2VDDxvaPicCodeExt  pic_code_ext;
    M2VDDxvaPicDispExt  pic_disp_ext;
} M2VDDxvaParam;

#endif
