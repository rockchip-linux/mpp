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

#ifndef __HAL_H264E_RKV_STREAM_H__
#define __HAL_H264E_RKV_STREAM_H__

#include "h264_syntax.h"

typedef struct  H264eRkvStream_t {
    RK_U8 *p_start;
    RK_U8 *p;
    //RK_U8 *p_end;

    RK_U32 cur_bits;
    RK_S32 i_left;  /* i_count number of available bits */

    //add buf pointer
    RK_U8 *buf;
    RK_U8 *buf_plus8;
    RK_U32 count_bit; /* only for debug */
} H264eRkvStream;

MPP_RET h264e_rkv_stream_init(H264eRkvStream *s);
MPP_RET h264e_rkv_stream_deinit(H264eRkvStream *s);
RK_S32 h264e_rkv_stream_get_pos(H264eRkvStream *s);
MPP_RET h264e_rkv_stream_reset(H264eRkvStream *s);
MPP_RET h264e_rkv_stream_realign(H264eRkvStream *s);
MPP_RET h264e_rkv_stream_write_with_log(H264eRkvStream *s,
                                        RK_S32 i_count, RK_U32 val, char *name);
MPP_RET h264e_rkv_stream_write1_with_log(H264eRkvStream *s,
                                         RK_U32 val, char *name);
MPP_RET h264e_rkv_stream_write_ue_with_log(H264eRkvStream *s,
                                           RK_U32 val, char *name);
MPP_RET h264e_rkv_stream_write_se_with_log(H264eRkvStream *s,
                                           RK_S32 val, char *name);
MPP_RET h264e_rkv_stream_write32(H264eRkvStream *s, RK_U32 i_bits,
                                 char *name);
RK_S32 h264e_rkv_stream_size_se( RK_S32 val );
MPP_RET h264e_rkv_stream_rbsp_trailing(H264eRkvStream *s);
MPP_RET h264e_rkv_stream_flush(H264eRkvStream *s);

#endif /* __HAL_H264E_RKV_STREAM_H__ */
