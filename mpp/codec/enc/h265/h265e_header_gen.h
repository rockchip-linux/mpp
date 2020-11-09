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

#ifndef __H265E_HEADER_GEN_H__
#define __H265E_HEADER_GEN_H__

#include "h265e_stream.h"
#define H265E_UUID_LENGTH 16

typedef enum H265eNalIdx_t {
    H265E_NAL_IDX_VPS,
    H265E_NAL_IDX_SPS,
    H265E_NAL_IDX_PPS,
    H265E_NAL_IDX_SEI,
    H265E_NAL_IDX_BUTT,
} H265eNalIdx;

typedef enum H265SeiType_e {
    H265_SEI_BUFFERING_PERIOD                       = 0,
    H265_SEI_PICTURE_TIMING                         = 1,
    H265_SEI_PAN_SCAN_RECT                          = 2,
    H265_SEI_FILLER_PAYLOAD                         = 3,
    H265_SEI_USER_DATA_REGISTERED_ITU_T_T35         = 4,
    H265_SEI_USER_DATA_UNREGISTERED                 = 5,
    H265_SEI_RECOVERY_POINT                         = 6,
    H265_SEI_SCENE_INFO                             = 9,
    H265_SEI_FULL_FRAME_SNAPSHOT                    = 15,
    H265_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START   = 16,
    H265_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END     = 17,
    H265_SEI_FILM_GRAIN_CHARACTERISTICS             = 19,
    H265_SEI_POST_FILTER_HINT                       = 22,
    H265_SEI_TONE_MAPPING_INFO                      = 23,
    H265_SEI_FRAME_PACKING                          = 45,
    H265_SEI_DISPLAY_ORIENTATION                    = 47,
    H265_SEI_SOP_DESCRIPTION                        = 128,
    H265_SEI_ACTIVE_PARAMETER_SETS                  = 129,
    H265_SEI_DECODING_UNIT_INFO                     = 130,
    H265_SEI_TEMPORAL_LEVEL0_INDEX                  = 131,
    H265_SEI_DECODED_PICTURE_HASH                   = 132,
    H265_SEI_SCALABLE_NESTING                       = 133,
    H265_SEI_REGION_REFRESH_INFO                    = 134,
    H265_SEI_MAX_ELEMENTS = 255, //!< number of maximum syntax elements
} H265SeiType;

typedef struct  H265eNal_t {
    RK_S32 i_ref_idc;  /* nal_priority_e */
    RK_S32 i_type;     /* nal_unit_type_e */
    RK_S32 b_long_startcode;
    RK_S32 i_first_mb; /* If this NAL is a slice, the index of the first MB in the slice. */
    RK_S32 i_last_mb;  /* If this NAL is a slice, the index of the last MB in the slice. */

    /* Size of payload (including any padding) in bytes. */
    RK_S32     i_payload;
    /* If param->b_annexb is set, Annex-B bytestream with startcode.
     * Otherwise, startcode is replaced with a 4-byte size.
     * This size is the size used in mp4/similar muxing; it is equal to i_payload-4 */
    RK_U8 *p_payload;

    /* Size of padding in bytes. */
    RK_S32 i_padding;
    RK_S32 sh_head_len;
} H265eNal;

typedef struct H265eExtraInfo_t {
    RK_S32          nal_num;
    H265eNal        nal[H265E_NAL_IDX_BUTT];
    RK_U8           *nal_buf;
    RK_U8           *sei_buf;
    RK_U32          sei_change_flg;
    H265eStream     stream;
//    H265eSei        sei;
} H265eExtraInfo;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h265e_init_extra_info(void *extra_info);
MPP_RET h265e_deinit_extra_info(void *extra_info);
void h265e_rkv_nal_start(H265eExtraInfo *out, RK_S32 i_type,
                         RK_S32 i_ref_idc);

void h265e_nal_end(H265eExtraInfo *out);

RK_U32 h265e_data_to_sei(void *dst, RK_U8 uuid[16], const void *payload, RK_S32 size);

MPP_RET h265e_set_extra_info(H265eCtx *ctx);
MPP_RET h265e_get_extra_info(H265eCtx *ctx, MppPacket pkt_out);

#ifdef __cplusplus
}
#endif

#endif /*__H265E_HEADER_GEN_H__ */
