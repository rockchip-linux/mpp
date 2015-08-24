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

/**
 * Enumeration used to define the possible video compression codings.
 * NOTE:  This essentially refers to file extensions. If the coding is
 *        being used to specify the ENCODE type, then additional work
 *        must be done to configure the exact flavor of the compression
 *        to be used.  For decode cases where the user application can
 *        not differentiate between MPEG-4 and H.264 bit streams, it is
 *        up to the codec to handle this.
 */
//sync with the omx_video.h
typedef enum MPI_VIDEO_CODINGTYPE {
    MPI_VIDEO_CodingUnused,             /**< Value when coding is N/A */
    MPI_VIDEO_CodingAutoDetect,         /**< Autodetection of coding type */
    MPI_VIDEO_CodingMPEG2,              /**< AKA: H.262 */
    MPI_VIDEO_CodingH263,               /**< H.263 */
    MPI_VIDEO_CodingMPEG4,              /**< MPEG-4 */
    MPI_VIDEO_CodingWMV,                /**< Windows Media Video (WMV1,WMV2,WMV3)*/
    MPI_VIDEO_CodingRV,                 /**< all versions of Real Video */
    MPI_VIDEO_CodingAVC,                /**< H.264/AVC */
    MPI_VIDEO_CodingMJPEG,              /**< Motion JPEG */
    MPI_VIDEO_CodingVP8,                /**< VP8 */
    MPI_VIDEO_CodingVP9,                /**< VP9 */
    MPI_VIDEO_CodingVC1 = 0x01000000,   /**< Windows Media Video (WMV1,WMV2,WMV3)*/
    MPI_VIDEO_CodingFLV1,               /**< Sorenson H.263 */
    MPI_VIDEO_CodingDIVX3,              /**< DIVX3 */
    MPI_VIDEO_CodingVP6,
    MPI_VIDEO_CodingHEVC,               /**< H.265/HEVC */
    MPI_VIDEO_CodingKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    MPI_VIDEO_CodingVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    MPI_VIDEO_CodingMax = 0x7FFFFFFF
} MPI_VIDEO_CODINGTYPE;

typedef enum MPI_CODEC_TYPE {
    MPI_CODEC_NONE,
    MPI_CODEC_DECODER,
    MPI_CODEC_ENCODER,
    MPI_CODEC_BUTT,
} MPI_CODEC_TYPE;


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
MPP_RET mpp_deinit(MppCtx ctx);

#ifdef __cplusplus
}
#endif

#endif /*__RK_MPI_H__*/
