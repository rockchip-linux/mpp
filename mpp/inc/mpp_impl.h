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

#include <stdio.h>

#include "rk_mpi.h"
#include "rk_type.h"
#include "mpp_log.h"

/* dump data */
typedef struct mpp_dump_info_t {
    MppCtxType      type;

    FILE            *fp_in;  // write input file
    FILE            *fp_out; // write output file
    RK_U8           *fp_buf; // for resample frame
    RK_U32          dump_width;
    RK_U32          dump_height;
} MppDumpInfo;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET mpp_dump_init(MppDumpInfo *info, MppCtxType type);
MPP_RET mpp_dump_deinit(MppDumpInfo *info);

MPP_RET mpp_dump_packet(MppDumpInfo *info, MppPacket pkt);
MPP_RET mpp_dump_frame(MppDumpInfo *info, MppFrame frame);

#ifdef  __cplusplus
}
#endif

#endif
