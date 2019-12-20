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

#ifndef __H265E_STREAM_H__
#define __H265E_STREAM_H__

#include "mpp_bitwrite.h"

typedef struct  H265eStream_t {
    RK_U8 *buf;
    RK_S32 size;
    MppWriteCtx enc_stream;
} H265eStream;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h265e_stream_init(H265eStream *s);
MPP_RET h265e_stream_deinit(H265eStream *s);
MPP_RET h265e_stream_reset(H265eStream *s);
MPP_RET h265e_stream_realign(H265eStream *s);
MPP_RET h265e_stream_write_with_log(H265eStream *s,
                                    RK_U32 val, RK_S32 i_count, char *name);
MPP_RET h265e_stream_write1_with_log(H265eStream *s,
                                     RK_U32 val, char *name);
MPP_RET h265e_stream_write_ue_with_log(H265eStream *s,
                                       RK_U32 val, char *name);
MPP_RET h265e_stream_write_se_with_log(H265eStream *s,
                                       RK_S32 val, char *name);
MPP_RET h265e_stream_write32(H265eStream *s, RK_U32 i_bits,
                             char *name);
RK_S32 h265e_stream_size_se( RK_S32 val );
MPP_RET h265e_stream_rbsp_trailing(H265eStream *s);
MPP_RET h265e_stream_flush(H265eStream *s);

#ifdef __cplusplus
}
#endif

#endif /* __H265E_STREAM_H__ */
