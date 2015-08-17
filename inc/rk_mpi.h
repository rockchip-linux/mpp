/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __RK_MPI_H__
#define __RK_MPI_H__

#include "rk_type.h"

#include "mpp_err.h"
#include "mpp_packet.h"
#include "mpp_buffer.h"
#include "mpp_frame.h"

typedef enum {
    MPI_MPP_CMD_BASE                    = 0,
    MPI_MPP_ENABLE_DEINTERLACE,

    MPI_HAL_CMD_BASE                    = 0x10000,

    MPI_OSAL_CMD_BASE                   = 0x20000,
    MPI_OSAL_SET_VPUMEM_CONTEXT,

    MPI_CODEC_CMD_BASE                  = 0x30000,
    MPI_CODEC_INFO_CHANGE,
    MPI_CODEC_SET_DEFAULT_WIDTH_HEIGH,

    MPI_DEC_CMD_BASE                    = 0x40000,
    MPI_DEC_USE_PRESENT_TIME_ORDER,

    MPI_ENC_CMD_BASE                    = 0x50000,
    MPI_ENC_SETCFG,
    MPI_ENC_GETCFG,
    MPI_ENC_SETFORMAT,
    MPI_ENC_SETIDRFRAME,
} MPI_CMD;


typedef void* MppCtx;
typedef void* MppParam;

typedef struct {
    RK_U32  size;
    RK_U32  version;

    MPP_RET (*init)(MppCtx ctx, MppPacket packet);
    // sync interface
    MPP_RET (*decode)(MppCtx ctx, MppPacket packet, MppFrame *frame);
    MPP_RET (*encode)(MppCtx ctx, MppFrame frame, MppPacket *packet);

    // async interface
    MPP_RET (*decode_put_packet)(MppCtx ctx, MppPacket packet);
    MPP_RET (*decode_get_frame)(MppCtx ctx, MppFrame *frame);

    MPP_RET (*encode_put_frame)(MppCtx ctx, MppFrame frame);
    MPP_RET (*encode_get_packet)(MppCtx ctx, MppPacket *packet);

    MPP_RET (*flush)(MppCtx ctx);
    MPP_RET (*control)(MppCtx ctx, MPI_CMD cmd, MppParam param);

    RK_U32 reserv[16];
} MppApi;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * mpp interface
 */
MPP_RET mpp_init(MppCtx *ctx, MppApi **mpi);
MPP_RET mpp_deinit(MppCtx* ctx);

#ifdef __cplusplus
}
#endif

#endif /*__RK_MPI_H__*/
