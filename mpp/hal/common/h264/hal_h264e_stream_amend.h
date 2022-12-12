/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H264E_STREAM_AMEND_H__
#define __HAL_H264E_STREAM_AMEND_H__

#include "h264e_slice.h"

typedef struct HalH264eVepuStreamAmend_t {
    RK_S32          enable;
    H264eSlice      *slice;
    H264ePrefixNal  *prefix;
    RK_S32          slice_enabled;
    RK_S32          diable_split_out;

    RK_U8           *src_buf;
    RK_U8           *dst_buf;
    RK_S32          buf_size;

    MppPacket       packet;
    RK_S32          buf_base;
    RK_S32          old_length;
    RK_S32          new_length;
} HalH264eVepuStreamAmend;

MPP_RET h264e_vepu_stream_amend_init(HalH264eVepuStreamAmend *ctx);
MPP_RET h264e_vepu_stream_amend_deinit(HalH264eVepuStreamAmend *ctx);
MPP_RET h264e_vepu_stream_amend_config(HalH264eVepuStreamAmend *ctx,
                                       MppPacket packet, MppEncCfgSet *cfg,
                                       H264eSlice *slice, H264ePrefixNal *prefix);
MPP_RET h264e_vepu_stream_amend_proc(HalH264eVepuStreamAmend *ctx, RK_U32 poc_type);
MPP_RET h264e_vepu_stream_amend_sync_ref_idc(HalH264eVepuStreamAmend *ctx);

#endif /* __HAL_H264E_STREAM_AMEND_H__ */
