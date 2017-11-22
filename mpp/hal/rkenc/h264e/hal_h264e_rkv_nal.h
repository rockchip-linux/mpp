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

#ifndef __HAL_H264E_RKV_NAL_H__
#define __HAL_H264E_RKV_NAL_H__

#include "hal_h264e_com.h"
#include "hal_h264e_rkv_stream.h"

typedef enum H264eRkvNalIdx_t {
    H264E_RKV_NAL_IDX_SPS,
    H264E_RKV_NAL_IDX_PPS,
    H264E_RKV_NAL_IDX_SEI,
    H264E_RKV_NAL_IDX_BUTT,
} H264eRkvNalIdx;

typedef struct  H264eRkvNal_t {
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
} H264eRkvNal;

typedef struct H264eRkvExtraInfo_t {
    RK_S32          nal_num;
    H264eRkvNal     nal[H264E_RKV_NAL_IDX_BUTT];
    RK_U8           *nal_buf;
    RK_U8           *sei_buf;
    RK_U32          sei_change_flg;
    H264eRkvStream  stream;
    H264eSps        sps;
    H264ePps        pps;
    H264eSei        sei;
} H264eRkvExtraInfo;

MPP_RET h264e_rkv_init_extra_info(H264eRkvExtraInfo *extra_info);
MPP_RET h264e_rkv_deinit_extra_info(void *extra_info);
void h264e_rkv_nal_start(H264eRkvExtraInfo *out, RK_S32 i_type,
                         RK_S32 i_ref_idc);
void h264e_rkv_nal_end(H264eRkvExtraInfo *out);
MPP_RET h264e_rkv_set_extra_info(H264eHalContext *ctx);

MPP_RET h264e_rkv_get_extra_info(H264eHalContext *ctx, MppPacket *pkt_out);
MPP_RET h264e_rkv_sei_encode(H264eHalContext *ctx, RcSyntax *rc_syn);

#endif /* __HAL_H264E_RKV_NAL_H__ */
