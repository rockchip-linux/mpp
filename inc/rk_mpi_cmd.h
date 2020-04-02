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

#ifndef __RK_MPI_CMD_H__
#define __RK_MPI_CMD_H__

/*
 * Command id bit usage is defined as follows:
 * bit 20 - 23  - module id
 * bit 16 - 19  - contex id
 * bit  0 - 15  - command id
 */
#define CMD_MODULE_ID_MASK              (0x00F00000)
#define CMD_MODULE_OSAL                 (0x00100000)
#define CMD_MODULE_MPP                  (0x00200000)
#define CMD_MODULE_CODEC                (0x00300000)
#define CMD_MODULE_HAL                  (0x00400000)

#define CMD_CTX_ID_MASK                 (0x000F0000)
#define CMD_CTX_ID_DEC                  (0x00010000)
#define CMD_CTX_ID_ENC                  (0x00020000)
#define CMD_CTX_ID_ISP                  (0x00030000)

/* separate encoder control command to different segment */
#define CMD_CFG_ID_MASK                 (0x0000FF00)
#define CMD_ENC_CFG_ALL                 (0x00000000)
#define CMD_ENC_CFG_RC_API              (0x00000100)
#define CMD_ENC_CFG_FRM                 (0x00000200)
#define CMD_ENC_CFG_PREP                (0x00000300)
#define CMD_ENC_CFG_CODEC               (0x00001000)
#define CMD_ENC_CFG_H264                (0x00001000)
#define CMD_ENC_CFG_H265                (0x00001800)
#define CMD_ENC_CFG_VP8                 (0x00002000)
#define CMD_ENC_CFG_VP9                 (0x00002800)
#define CMD_ENC_CFG_AV1                 (0x00003000)
#define CMD_ENC_CFG_MJPEG               (0x00004000)

#define CMD_ENC_CFG_MISC                (0x00008000)
#define CMD_ENC_CFG_SPLIT               (0x00008100)
#define CMD_ENC_CFG_GOPREF              (0x00008200)
#define CMD_ENC_CFG_ROI                 (0x00008300)
#define CMD_ENC_CFG_OSD                 (0x00008400)

typedef enum {
    MPP_OSAL_CMD_BASE                   = CMD_MODULE_OSAL,
    MPP_OSAL_CMD_END,

    MPP_CMD_BASE                        = CMD_MODULE_MPP,
    MPP_ENABLE_DEINTERLACE,
    MPP_SET_INPUT_BLOCK,                /* deprecated */
    MPP_SET_INTPUT_BLOCK_TIMEOUT,       /* deprecated */
    MPP_SET_OUTPUT_BLOCK,               /* deprecated */
    MPP_SET_OUTPUT_BLOCK_TIMEOUT,       /* deprecated */
    /*
     * timeout setup, refer to  MPP_TIMEOUT_XXX
     * zero     - non block
     * negative - block with no timeout
     * positive - timeout in milisecond
     */
    MPP_SET_INPUT_TIMEOUT,              /* parameter type RK_S64 */
    MPP_SET_OUTPUT_TIMEOUT,             /* parameter type RK_S64 */
    MPP_CMD_END,

    MPP_CODEC_CMD_BASE                  = CMD_MODULE_CODEC,
    MPP_CODEC_GET_FRAME_INFO,
    MPP_CODEC_CMD_END,

    MPP_DEC_CMD_BASE                    = CMD_MODULE_CODEC | CMD_CTX_ID_DEC,
    MPP_DEC_SET_FRAME_INFO,             /* vpu api legacy control for buffer slot dimension init */
    MPP_DEC_SET_EXT_BUF_GROUP,          /* IMPORTANT: set external buffer group to mpp decoder */
    MPP_DEC_SET_INFO_CHANGE_READY,
    MPP_DEC_SET_PRESENT_TIME_ORDER,     /* use input time order for output */
    MPP_DEC_SET_PARSER_SPLIT_MODE,      /* Need to setup before init */
    MPP_DEC_SET_PARSER_FAST_MODE,       /* Need to setup before init */
    MPP_DEC_GET_STREAM_COUNT,
    MPP_DEC_GET_VPUMEM_USED_COUNT,
    MPP_DEC_SET_VC1_EXTRA_DATA,
    MPP_DEC_SET_OUTPUT_FORMAT,
    MPP_DEC_SET_DISABLE_ERROR,          /* When set it will disable sw/hw error (H.264 / H.265) */
    MPP_DEC_SET_IMMEDIATE_OUT,
    MPP_DEC_SET_ENABLE_DEINTERLACE,     /* MPP enable deinterlace by default. Vpuapi can disable it */
    MPP_DEC_CMD_END,

    MPP_ENC_CMD_BASE                    = CMD_MODULE_CODEC | CMD_CTX_ID_ENC,
    /* basic encoder setup control */
    MPP_ENC_SET_ALL_CFG,                /* set MppEncCfgSet structure */
    MPP_ENC_GET_ALL_CFG,                /* get MppEncCfgSet structure */
    MPP_ENC_SET_PREP_CFG,               /* set MppEncPrepCfg structure */
    MPP_ENC_GET_PREP_CFG,               /* get MppEncPrepCfg structure */
    MPP_ENC_SET_RC_CFG,                 /* set MppEncRcCfg structure */
    MPP_ENC_GET_RC_CFG,                 /* get MppEncRcCfg structure */
    MPP_ENC_SET_CODEC_CFG,              /* set MppEncCodecCfg structure */
    MPP_ENC_GET_CODEC_CFG,              /* get MppEncCodecCfg structure */
    /* runtime encoder setup control */
    MPP_ENC_SET_IDR_FRAME,              /* next frame will be encoded as intra frame */
    MPP_ENC_SET_OSD_LEGACY_0,           /* deprecated */
    MPP_ENC_SET_OSD_LEGACY_1,           /* deprecated */
    MPP_ENC_SET_OSD_LEGACY_2,           /* deprecated */
    MPP_ENC_GET_HDR_SYNC,               /* get vps / sps / pps which has better sync behavior parameter is MppPacket */
    MPP_ENC_GET_EXTRA_INFO,             /* deprecated */
    MPP_ENC_SET_SEI_CFG,                /* SEI: Supplement Enhancemant Information, parameter is MppSeiMode */
    MPP_ENC_GET_SEI_DATA,               /* SEI: Supplement Enhancemant Information, parameter is MppPacket */
    MPP_ENC_PRE_ALLOC_BUFF,             /* allocate buffers before encoding */
    MPP_ENC_SET_QP_RANGE,               /* used for adjusting qp range, the parameter can be 1 or 2 */
    MPP_ENC_SET_ROI_CFG,                /* set MppEncROICfg structure */
    MPP_ENC_SET_CTU_QP,                 /* for H265 Encoder,set CTU's size and QP */

    /* User define rate control stategy API control */
    MPP_ENC_CFG_RC_API                  = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_RC_API,
    /*
     * Get RcApiQueryAll structure
     * Get all available rate control stategy string and count
     */
    MPP_ENC_GET_RC_API_ALL              = MPP_ENC_CFG_RC_API + 1,
    /*
     * Get RcApiQueryType structure
     * Get available rate control stategy string with certain type
     */
    MPP_ENC_GET_RC_API_BY_TYPE          = MPP_ENC_CFG_RC_API + 2,
    /*
     * Set RcImplApi structure
     * Add new or update rate control stategy function pointers
     */
    MPP_ENC_SET_RC_API_CFG              = MPP_ENC_CFG_RC_API + 3,
    /*
     * Get RcApiBrief structure
     * Get current used rate control stategy brief information (type and name)
     */
    MPP_ENC_GET_RC_API_CURRENT          = MPP_ENC_CFG_RC_API + 4,
    /*
     * Set RcApiBrief structure
     * Set current used rate control stategy brief information (type and name)
     */
    MPP_ENC_SET_RC_API_CURRENT          = MPP_ENC_CFG_RC_API + 5,

    MPP_ENC_CFG_FRM                     = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_FRM,
    MPP_ENC_SET_FRM,                    /* set MppFrame structure */
    MPP_ENC_GET_FRM,                    /* get MppFrame structure */


    MPP_ENC_CFG_PREP                    = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_PREP,
    MPP_ENC_SET_PREP,                   /* set MppEncPrepCfg structure */
    MPP_ENC_GET_PREP,                   /* get MppEncPrepCfg structure */

    MPP_ENC_CFG_H264                    = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_H264,

    MPP_ENC_CFG_H265                    = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_H265,

    MPP_ENC_CFG_VP8                     = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_VP8,

    MPP_ENC_CFG_MJPEG                   = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_MJPEG,

    MPP_ENC_CFG_MISC                    = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_MISC,
    MPP_ENC_SET_HEADER_MODE,            /* set MppEncHeaderMode */
    MPP_ENC_GET_HEADER_MODE,            /* get MppEncHeaderMode */

    MPP_ENC_CFG_SPLIT                   = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_SPLIT,
    MPP_ENC_SET_SPLIT,                  /* set MppEncSliceSplit structure */
    MPP_ENC_GET_SPLIT,                  /* get MppEncSliceSplit structure */

    MPP_ENC_CFG_GOPREF                  = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_GOPREF,
    MPP_ENC_SET_GOPREF,                 /* set MppEncGopRef structure */

    MPP_ENC_CFG_OSD                     = CMD_MODULE_CODEC | CMD_CTX_ID_ENC | CMD_ENC_CFG_OSD,
    MPP_ENC_SET_OSD_PLT_CFG,            /* set OSD palette, parameter should be pointer to MppEncOSDPltCfg */
    MPP_ENC_GET_OSD_PLT_CFG,            /* get OSD palette, parameter should be pointer to MppEncOSDPltCfg */
    MPP_ENC_SET_OSD_DATA_CFG,           /* set OSD data with at most 8 regions, parameter should be pointer to MppEncOSDData */
    MPP_ENC_GET_OSD_DATA_CFG,           /* get OSD data with at most 8 regions, parameter should be pointer to MppEncOSDData */

    MPP_ENC_CMD_END,

    MPP_ISP_CMD_BASE                    = CMD_MODULE_CODEC | CMD_CTX_ID_ISP,
    MPP_ISP_CMD_END,

    MPP_HAL_CMD_BASE                    = CMD_MODULE_HAL,
    MPP_HAL_CMD_END,

    MPI_CMD_BUTT,
} MpiCmd;

#include "rk_venc_cmd.h"

#endif /*__RK_MPI_CMD_H__*/
