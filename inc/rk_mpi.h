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

#ifndef __RK_MPI_H__
#define __RK_MPI_H__

#include "mpp_task.h"

typedef enum {
    MPP_CTX_DEC,
    MPP_CTX_ENC,
    MPP_CTX_BUTT,
} MppCtxType;

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
typedef enum {
    MPP_VIDEO_CodingUnused,             /**< Value when coding is N/A */
    MPP_VIDEO_CodingAutoDetect,         /**< Autodetection of coding type */
    MPP_VIDEO_CodingMPEG2,              /**< AKA: H.262 */
    MPP_VIDEO_CodingH263,               /**< H.263 */
    MPP_VIDEO_CodingMPEG4,              /**< MPEG-4 */
    MPP_VIDEO_CodingWMV,                /**< Windows Media Video (WMV1,WMV2,WMV3)*/
    MPP_VIDEO_CodingRV,                 /**< all versions of Real Video */
    MPP_VIDEO_CodingAVC,                /**< H.264/AVC */
    MPP_VIDEO_CodingMJPEG,              /**< Motion JPEG */
    MPP_VIDEO_CodingVP8,                /**< VP8 */
    MPP_VIDEO_CodingVP9,                /**< VP9 */
    MPP_VIDEO_CodingVC1 = 0x01000000,   /**< Windows Media Video (WMV1,WMV2,WMV3)*/
    MPP_VIDEO_CodingFLV1,               /**< Sorenson H.263 */
    MPP_VIDEO_CodingDIVX3,              /**< DIVX3 */
    MPP_VIDEO_CodingVP6,
    MPP_VIDEO_CodingHEVC,               /**< H.265/HEVC */
    MPP_VIDEO_CodingAVS,                /**< AVS+ */
    MPP_VIDEO_CodingKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    MPP_VIDEO_CodingVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    MPP_VIDEO_CodingMax = 0x7FFFFFFF
} MppCodingType;

typedef enum {
    MPP_CMD_BASE                        = 0,
    MPP_ENABLE_DEINTERLACE,
    MPP_SET_OUTPUT_BLOCK,

    MPP_HAL_CMD_BASE                    = 0x10000,

    MPP_OSAL_CMD_BASE                   = 0x20000,

    MPP_CODEC_CMD_BASE                  = 0x30000,
    MPP_CODEC_SET_INFO_CHANGE_READY,
    MPP_CODEC_SET_FRAME_INFO,
    MPP_CODEC_GET_FRAME_INFO,
    MPP_CODEC_GET_VPUMEM_USED_COUNT,

    MPP_DEC_CMD_BASE                    = 0x40000,
    MPP_DEC_SET_EXT_BUF_GROUP,          /* IMPORTANT: set external buffer group to mpp decoder */
    MPP_DEC_SET_INTERNAL_PTS_ENABLE,
    MPP_DEC_SET_VC1_EXTRA_DATA,
    MPP_DEC_SET_PARSER_SPLIT_MODE,      /* Need to setup before init */
    MPP_DEC_SET_PARSER_FAST_MODE,       /* Need to setup before init */
    MPP_DEC_GET_STREAM_COUNT,

    MPP_ENC_CMD_BASE                    = 0x50000,
    MPP_ENC_SETCFG,
    MPP_ENC_GETCFG,
    MPP_ENC_SETFORMAT,
    MPP_ENC_SETIDRFRAME,

    MPI_CMD_BUTT,
} MpiCmd;

typedef void* MppCtx;
typedef void* MppParam;

/*
 * in decoder mode application need to specify the coding type first
 * send a stream header to mpi ctx using parameter data / size
 * and decoder will try to decode the
 */
typedef struct MppEncConfig_t {
    /*
     * input source data format
     */
    RK_S32  width;
    RK_S32  height;
    RK_S32  format;

    /*
     * rate control parameter
     *
     * rc_mode  - rate control mode
     *            0 - fix qp mode
     *            1 - constant bit rate mode (CBR)
     *            2 - variable bit rate mode (VBR)
     * bps      - target bit rate, unit: bit per second
     * fps_in   - input  frame rate, unit: frame per second
     * fps_out  - output frame rate, unit: frame per second
     * qp       - constant qp for fix qp mode
     *            initial qp for CBR / VBR
     * gop      - gap between Intra frame
     *            0 for only 1 I frame the rest are all P frames
     *            1 for all I frame
     *            2 for I P I P I P
     *            3 for I P P I P P
     *            etc...
     */
    RK_S32  rc_mode;
    RK_S32  bps;
    RK_S32  fps_in;
    RK_S32  fps_out;
    RK_S32  qp;
    RK_S32  gop;

    /*
     * stream feature parameter
     */
    RK_S32  profile;
    RK_S32  level;
    RK_S32  cabac_en;
    RK_S32  trans8x8_en;
} MppEncConfig;

/*
 * mpp main work function set
 * size     : MppApi structure size
 * version  : Mpp svn revision
 *
 * all api function are seperated into two sets: data io api set and control api set
 *
 * the data api set is for data input/output flow including:
 *
 * simple data api set:
 *
 * decode   : both send video stream packet to decoder and get video frame from
 *            decoder at the same time.
 * encode   : both send video frame to encoder and get encoded video stream from
 *            encoder at the same time.
 *
 * decode_put_packet: send video stream packet to decoder only, async interface
 * decode_get_frame : get video frame from decoder only, async interface
 *
 * encode_put_frame : send video frame to encoder only, async interface
 * encode_get_packet: get encoded video packet from encoder only, async interface
 *
 * advance task api set:
 *
 *
 * the control api set is for mpp context control including:
 * control  : similiar to ioctl in kernel driver, setup or get mpp internal parameter
 * reset    : clear all data in mpp context, reset to initialized status
 * the simple api set is for simple codec usage including:
 *
 *
 * reset    : discard all packet and frame, reset all component,
 *            for both decoder and encoder
 * control  : control function for mpp property setting
 * config   : config function for encoder, not implement yet.
 */
typedef struct MppApi_t {
    RK_U32  size;
    RK_U32  version;

    // simple data flow interface
    MPP_RET (*decode)(MppCtx ctx, MppPacket packet, MppFrame *frame);
    MPP_RET (*decode_put_packet)(MppCtx ctx, MppPacket packet);
    MPP_RET (*decode_get_frame)(MppCtx ctx, MppFrame *frame);

    MPP_RET (*encode)(MppCtx ctx, MppFrame frame, MppPacket *packet);
    MPP_RET (*encode_put_frame)(MppCtx ctx, MppFrame frame);
    MPP_RET (*encode_get_packet)(MppCtx ctx, MppPacket *packet);

    MPP_RET (*isp)(MppCtx ctx, MppFrame dst, MppFrame src);
    MPP_RET (*isp_put_frame)(MppCtx ctx, MppFrame frame);
    MPP_RET (*isp_get_frame)(MppCtx ctx, MppFrame *frame);

    // advance data flow interface
    MPP_RET (*dequeue)(MppCtx ctx, MppPortType type, MppTask *task);
    MPP_RET (*enqueue)(MppCtx ctx, MppPortType type, MppTask task);

    // control interface
    MPP_RET (*reset)(MppCtx ctx);
    MPP_RET (*control)(MppCtx ctx, MpiCmd cmd, MppParam param);
    MPP_RET (*config)(MppCtx ctx, MppEncConfig cfg);

    RK_U32 reserv[16];
} MppApi;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * mpp interface work flow
 *
 * 1. mpp_create : Create empty context structure and mpi function pointers.
 * 2. mpp_init   : Call after mpp_create to setup mpp type and video format.
 *                 This function will call internal context init function.
 * 3. Use functions in MppApi to access mpp services.
 * 4. mpp_destory: Destroy mpp context and free both context and mpi structure
 */
MPP_RET mpp_create(MppCtx *ctx, MppApi **mpi);
MPP_RET mpp_init(MppCtx ctx, MppCtxType type, MppCodingType coding);
MPP_RET mpp_destroy(MppCtx ctx);

// coding type format function
MPP_RET mpp_check_support_format(MppCtxType type, MppCodingType coding);
void    mpp_show_support_format();

#ifdef __cplusplus
}
#endif

#endif /*__RK_MPI_H__*/
