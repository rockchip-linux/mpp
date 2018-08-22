/*
 *
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

#ifndef __MPP_IMPL_H__
#define __MPP_IMPL_H__

#include "rk_mpi.h"

typedef void* MppDump;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET mpp_dump_init(MppDump *info);
MPP_RET mpp_dump_deinit(MppDump *info);

MPP_RET mpp_ops_init(MppDump info, MppCtxType type, MppCodingType coding);

MPP_RET mpp_ops_dec_put_pkt(MppDump info, MppPacket pkt);
MPP_RET mpp_ops_dec_get_frm(MppDump info, MppFrame frame);
MPP_RET mpp_ops_enc_put_frm(MppDump info, MppFrame frame);
MPP_RET mpp_ops_enc_get_pkt(MppDump info, MppPacket pkt);

MPP_RET mpp_ops_ctrl(MppDump info, MpiCmd cmd);
MPP_RET mpp_ops_reset(MppDump info);

#ifdef  __cplusplus
}
#endif

#endif
