/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include "hal_h265e_stream_amend.h"

#include "mpp_common.h"
#include "mpp_log.h"

MPP_RET hal_h265e_amend_temporal_id(HalEncTask *enc_task, RK_U32 strm_size)
{
    MPP_RET ret = MPP_OK;
    H265eSyntax_new *syn = (H265eSyntax_new *)enc_task->syntax.data;
    RK_U32 offset = mpp_packet_get_length(enc_task->packet);
    RK_U8 *stream_ptr = (RK_U8 *)(mpp_buffer_get_ptr(enc_task->output) + offset);

    if (strm_size < 5) {
        mpp_err("Stream size is too small, maybe there is hw encoder error!");
        return MPP_NOK;
    }

    if (syn->sp.temporal_id)
        stream_ptr[5] = (stream_ptr[5] & 0xf8) | ((syn->sp.temporal_id + 1) & 0x7);

    return ret;
}