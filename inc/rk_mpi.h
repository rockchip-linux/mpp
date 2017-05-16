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
#include "rk_mpi_cmd.h"

/**
 * @addtogroup rk_mpi
 * @brief rockchip media process interface
 *
 *        Mpp provides application programming interface for the application layer.
 */

/**
 * @ingroup rk_mpi
 * @brief The type of mpp context
 */
typedef enum {
    MPP_CTX_DEC,  /**< decoder */
    MPP_CTX_ENC,  /**< encoder */
    MPP_CTX_ISP,  /**< isp */
    MPP_CTX_BUTT, /**< undefined */
} MppCtxType;

/**
 * @ingroup rk_mpi
 * @brief Enumeration used to define the possible video compression codings.
 *        sync with the omx_video.h
 *
 * @note  This essentially refers to file extensions. If the coding is
 *        being used to specify the ENCODE type, then additional work
 *        must be done to configure the exact flavor of the compression
 *        to be used.  For decode cases where the user application can
 *        not differentiate between MPEG-4 and H.264 bit streams, it is
 *        up to the codec to handle this.
 */
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
    MPP_VIDEO_CodingAVSPLUS,            /**< AVS+ */
    MPP_VIDEO_CodingAVS,                /**< AVS profile=0x20 */
    MPP_VIDEO_CodingKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    MPP_VIDEO_CodingVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    MPP_VIDEO_CodingMax = 0x7FFFFFFF
} MppCodingType;

typedef void* MppCtx;
typedef void* MppParam;

/*
 * in decoder mode application need to specify the coding type first
 * send a stream header to mpi ctx using parameter data / size
 * and decoder will try to decode the
 */
typedef struct MppEncCodecCfg_t {
    MppCodingType       coding;

    union {
        RK_U32          change;
        MppEncH264Cfg   h264;
        MppEncH265Cfg   h265;
        MppEncJpegCfg   jpeg;
        MppEncVp8Cfg    vp8;
    };
} MppEncCodecCfg;

typedef struct MppEncCfgSet_t {
    MppEncPrepCfg       prep;
    MppEncRcCfg         rc;
    MppEncCodecCfg      codec;
} MppEncCfgSet;

/**
 * @ingroup rk_mpi
 * @brief mpp main work function set
 *
 * @note all api function are seperated into two sets: data io api set and control api set
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
 */
typedef struct MppApi_t {
    RK_U32  size;
    RK_U32  version;

    // simple data flow interface
    /**
     * @brief both send video stream packet to decoder and get video frame from
     *        decoder at the same time
     * @param ctx The context of mpp
     * @param packet[in] The input video stream
     * @param frame[out] The output picture
     * @return 0 for decode success, others for failure
     */
    MPP_RET (*decode)(MppCtx ctx, MppPacket packet, MppFrame *frame);
    /**
     * @brief send video stream packet to decoder only, async interface
     * @param ctx The context of mpp
     * @param packet The input video stream
     * @return 0 for success, others for failure
     */
    MPP_RET (*decode_put_packet)(MppCtx ctx, MppPacket packet);
    /**
     * @brief get video frame from decoder only, async interface
     * @param ctx The context of mpp
     * @param frame The output picture
     * @return 0 for success, others for failure
     */
    MPP_RET (*decode_get_frame)(MppCtx ctx, MppFrame *frame);
    /**
     * @brief both send video frame to encoder and get encoded video stream from
     *        encoder at the same time
     * @param ctx The context of mpp
     * @param frame[in] The input video data
     * @param packet[out] The output compressed data
     * @return 0 for encode success, others for failure
     */
    MPP_RET (*encode)(MppCtx ctx, MppFrame frame, MppPacket *packet);
    /**
     * @brief send video frame to encoder only, async interface
     * @param ctx The context of mpp
     * @param frame The input video data
     * @return 0 for success, others for failure
     */
    MPP_RET (*encode_put_frame)(MppCtx ctx, MppFrame frame);
    /**
     * @brief get encoded video packet from encoder only, async interface
     * @param ctx The context of mpp
     * @param packet The output compressed data
     * @return 0 for success, others for failure
     */
    MPP_RET (*encode_get_packet)(MppCtx ctx, MppPacket *packet);

    MPP_RET (*isp)(MppCtx ctx, MppFrame dst, MppFrame src);
    MPP_RET (*isp_put_frame)(MppCtx ctx, MppFrame frame);
    MPP_RET (*isp_get_frame)(MppCtx ctx, MppFrame *frame);

    // advance data flow interface
    /**
     * @brief poll port for dequeue
     * @param ctx The context of mpp
     * @param type input port or output port which are both for data transaction
     * @return 0 for success there is valid task for dequeue, others for failure
     */
    MPP_RET (*poll)(MppCtx ctx, MppPortType type, MppPollType timeout);
    /**
     * @brief dequeue MppTask
     * @param ctx The context of mpp
     * @param type input port or output port which are both for data transaction
     * @param task MppTask which is sent to mpp for process
     * @return 0 for success, others for failure
     */
    MPP_RET (*dequeue)(MppCtx ctx, MppPortType type, MppTask *task);
    /**
     * @brief enqueue MppTask
     * @param ctx The context of mpp
     * @param type input port or output port which are both for data transaction
     * @param task MppTask which is sent to mpp for process
     * @return 0 for success, others for failure
     */
    MPP_RET (*enqueue)(MppCtx ctx, MppPortType type, MppTask task);

    // control interface
    /**
     * @brief discard all packet and frame, reset all component,
     *        for both decoder and encoder
     * @param ctx The context of mpp
     */
    MPP_RET (*reset)(MppCtx ctx);
    /**
     * @brief control function for mpp property setting
     * @param ctx The context of mpp
     * @param cmd The mpi command
     * @param param The mpi command parameter
     * @return 0 for success, others for failure
     */
    MPP_RET (*control)(MppCtx ctx, MpiCmd cmd, MppParam param);

    /**
     * @brief The reserved segment
     */
    RK_U32 reserv[16];
} MppApi;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup rk_mpi
 * @brief Create empty context structure and mpi function pointers.
 *        Use functions in MppApi to access mpp services.
 * @param ctx pointer of the mpp context
 * @param mpi pointer of mpi function
 */
MPP_RET mpp_create(MppCtx *ctx, MppApi **mpi);
/**
 * @ingroup rk_mpi
 * @brief Call after mpp_create to setup mpp type and video format.
 *        This function will call internal context init function.
 * @param ctx The context of mpp
 * @param type MppCtxType, decoder or encoder
 * @param coding video compression coding
 */
MPP_RET mpp_init(MppCtx ctx, MppCtxType type, MppCodingType coding);
/**
 * @ingroup rk_mpi
 * @brief Destroy mpp context and free both context and mpi structure
 * @param ctx The context of mpp
 */
MPP_RET mpp_destroy(MppCtx ctx);

// coding type format function
MPP_RET mpp_check_support_format(MppCtxType type, MppCodingType coding);
void    mpp_show_support_format(void);

#ifdef __cplusplus
}
#endif

#endif /*__RK_MPI_H__*/
