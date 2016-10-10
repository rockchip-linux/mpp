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

#include "rk_type.h"

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
#define CMD_ID_MASK                     (0x0000FFFF)

#define MPP_ENC_OSD_PLT_WHITE           ((255<<24)|(128<<16)|(128<<8)|235)
#define MPP_ENC_OSD_PLT_YELLOW          ((255<<24)|(146<<16)|( 16<<8 )|210)
#define MPP_ENC_OSD_PLT_CYAN            ((255<<24)|( 16<<16 )|(166<<8)|170)
#define MPP_ENC_OSD_PLT_GREEN           ((255<<24)|( 34<<16 )|( 54<<8 )|145)
#define MPP_ENC_OSD_PLT_TRANS           ((  0<<24)|(222<<16)|(202<<8)|106)
#define MPP_ENC_OSD_PLT_RED             ((255<<24)|(240<<16)|( 90<<8 )|81)
#define MPP_ENC_OSD_PLT_BLUE            ((255<<24)|(110<<16)|(240<<8)|41)
#define MPP_ENC_OSD_PLT_BLACK           ((255<<24)|(128<<16)|(128<<8)|16)

typedef enum MppEncSeiMode_t {
    MPP_ENC_SEI_MODE_DISABLE,                /* default mode, SEI writing is disabled */
    MPP_ENC_SEI_MODE_ONE_SEQ,                /* one sequence has only one SEI */
    MPP_ENC_SEI_MODE_ONE_FRAME               /* one frame may have one SEI, if SEI info has changed */
} MppEncSeiMode;

typedef enum {
    MPP_OSAL_CMD_BASE                   = CMD_MODULE_OSAL,
    MPP_OSAL_CMD_END,

    MPP_CMD_BASE                        = CMD_MODULE_MPP,
    MPP_ENABLE_DEINTERLACE,
    MPP_SET_INPUT_BLOCK,
    MPP_SET_OUTPUT_BLOCK,
    MPP_CMD_END,

    MPP_CODEC_CMD_BASE                  = CMD_MODULE_CODEC,
    MPP_CODEC_GET_FRAME_INFO,
    MPP_CODEC_CMD_END,

    MPP_DEC_CMD_BASE                    = CMD_MODULE_CODEC | CMD_CTX_ID_DEC,
    MPP_DEC_SET_FRAME_INFO,             /* vpu api legacy control for buffer slot dimension init */
    MPP_DEC_SET_EXT_BUF_GROUP,          /* IMPORTANT: set external buffer group to mpp decoder */
    MPP_DEC_SET_INFO_CHANGE_READY,
    MPP_DEC_SET_INTERNAL_PTS_ENABLE,
    MPP_DEC_SET_PARSER_SPLIT_MODE,      /* Need to setup before init */
    MPP_DEC_SET_PARSER_FAST_MODE,       /* Need to setup before init */
    MPP_DEC_GET_STREAM_COUNT,
    MPP_DEC_GET_VPUMEM_USED_COUNT,
    MPP_DEC_SET_VC1_EXTRA_DATA,
    MPP_DEC_SET_OUTPUT_FORMAT,
    MPP_DEC_CMD_END,

    MPP_ENC_CMD_BASE                    = CMD_MODULE_CODEC | CMD_CTX_ID_ENC,
    MPP_ENC_SET_RC_CFG,
    MPP_ENC_GET_RC_CFG,
    MPP_ENC_SET_PREP_CFG,
    MPP_ENC_GET_PREP_CFG,
    MPP_ENC_SET_OSD_PLT_CFG,            /* set OSD palette, parameter should be pointer to MppEncOSDPlt */
    MPP_ENC_SET_OSD_DATA_CFG,           /* set OSD data with at most 8 regions, parameter should be pointer to MppEncOSDData */
    MPP_ENC_GET_OSD_CFG,
    MPP_ENC_SET_CFG,
    MPP_ENC_GET_CFG,
    MPP_ENC_SET_EXTRA_INFO,
    MPP_ENC_GET_EXTRA_INFO,
    MPP_ENC_SET_FORMAT,
    MPP_ENC_SET_IDR_FRAME,
    MPP_ENC_SET_SEI_CFG,               /*SEI: Supplement Enhancemant Information, parameter is MppSeiMode */
    MPP_ENC_GET_SEI_DATA,              /*SEI: Supplement Enhancemant Information, parameter is MppPacket */
    MPP_ENC_CMD_END,

    MPP_ISP_CMD_BASE                    = CMD_MODULE_CODEC | CMD_CTX_ID_ISP,
    MPP_ISP_CMD_END,

    MPP_HAL_CMD_BASE                    = CMD_MODULE_HAL,
    MPP_HAL_CMD_END,

    MPI_CMD_BUTT,
} MpiCmd;

/*
 * Configure of encoder is separated into four parts.
 *
 * 1. Rate control parameter
 *    This is quality and bitrate request from user.
 *    For controller only
 *
 * 2. Data source MppFrame parameter
 *    This is data source buffer information.
 *    For both controller and hal
 *
 * 3. Video codec infomation
 *    This is user custormized stream information.
 *    For hal only
 *
 * 4. Extra parameter
 *    including:
 *    PreP  : encoder Preprocess configuration
 *    ROI   : Region Of Interest
 *    OSD   : On Screen Display
 *    MD    : Motion Detection
 *    extra : SEI for h.264 / Exif for mjpeg
 *    For hal only
 *
 * The module transcation flow is as follows:
 *
 *                 +                      +
 *     User        |      Mpi/Mpp         |         Controller
 *                 |                      |            Hal
 *                 |                      |
 * +----------+    |    +---------+       |       +------------+
 * |          |    |    |         +-----RcCfg----->            |
 * |  RcCfg   +--------->         |       |       | Controller |
 * |          |    |    |         |   +-Frame----->            |
 * +----------+    |    |         |   |   |       +---+-----^--+
 *                 |    |         |   |   |           |     |
 *                 |    |         |   |   |           |     |
 * +----------+    |    |         |   |   |        syntax   |
 * |          |    |    |         |   |   |           |     |
 * | MppFrame +--------->  MppEnc +---+   |           |   result
 * |          |    |    |         |   |   |           |     |
 * +----------+    |    |         |   |   |           |     |
 *                 |    |         |   |   |       +---v-----+--+
 *                 |    |         |   +-Frame----->            |
 * +----------+    |    |         |       |       |            |
 * |          |    |    |         +---CodecCfg---->    Hal     |
 * | CodecCfg +--------->         |       |       |            |
 * |          |    |    |         <-----Extra----->            |
 * +----------+    |    +---------+       |       +------------+
 *                 |                      |
 *                 |                      |
 *                 +                      +
 *
 * The function call flow is shown below:
 *
 *  mpi                      mpp_enc         controller                  hal
 *   +                          +                 +                       +
 *   |                          |                 |                       |
 *   |                          |                 |                       |
 *   +----------init------------>                 |                       |
 *   |                          |                 |                       |
 *   |                          |                 |                       |
 *   |         MppFrame         |                 |                       |
 *   +---------control---------->     MppFrame    |                       |
 *   |                          +-----control----->                       |
 *   |                          |                 |        MppFrame       |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                    allocate
 *   |                          |                 |                     buffer
 *   |                          |                 |                       |
 *   |          RcCfg           |                 |                       |
 *   +---------control---------->      RcCfg      |                       |
 *   |                          +-----control----->                       |
 *   |                          |              rc_init                    |
 *   |                          |                 |                       |
 *   |                          |                 |                       |
 *   |         CodecCfg         |                 |                       |
 *   +---------control---------->                 |        CodecCfg       |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                    generate
 *   |                          |                 |                    sps/pps
 *   |                          |                 |     Get extra info    |
 *   |                          +--------------------------control-------->
 *   |      Get extra info      |                 |                       |
 *   +---------control---------->                 |                       |
 *   |                          |                 |                       |
 *   |                          |                 |                       |
 *   |         PrepCfg          |                 |                       |
 *   +---------control---------->                 |        PrepCfg        |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                       |
 *   |         ROICfg           |                 |                       |
 *   +---------control---------->                 |        ROICfg         |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                       |
 *   |         OSDCfg           |                 |                       |
 *   +---------control---------->                 |        OSDCfg         |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                       |
 *   |          MDCfg           |                 |                       |
 *   +---------control---------->                 |         MDCfg         |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                       |
 *   |      Set extra info      |                 |                       |
 *   +---------control---------->                 |     Set extra info    |
 *   |                          +--------------------------control-------->
 *   |                          |                 |                       |
 *   |           task           |                 |                       |
 *   +----------encode---------->      task       |                       |
 *   |                          +-----encode------>                       |
 *   |                          |              encode                     |
 *   |                          |                 |        syntax         |
 *   |                          +--------------------------gen_reg-------->
 *   |                          |                 |                       |
 *   |                          |                 |                       |
 *   |                          +---------------------------start--------->
 *   |                          |                 |                       |
 *   |                          |                 |                       |
 *   |                          +---------------------------wait---------->
 *   |                          |                 |                       |
 *   |                          |    callback     |                       |
 *   |                          +----------------->                       |
 *   +--OSD-MD--encode---------->                 |                       |
 *   |             .            |                 |                       |
 *   |             .            |                 |                       |
 *   |             .            |                 |                       |
 *   +--OSD-MD--encode---------->                 |                       |
 *   |                          |                 |                       |
 *   +----------deinit---------->                 |                       |
 *   +                          +                 +                       +
 */

/*
 * Rate control parameter
 *
 * rc_mode      - rate control mode
 * Mpp balances quality and bit rate by the mode index
 * Mpp provide 5 level of balance mode of quality and bit rate
 * 1 - only quality mode: only quality parameter takes effect
 * 2 - more quality mode: quality parameter takes more effect
 * 3 - balance mode     : balance quality and bitrate 50 to 50
 * 4 - more bitrate mode: bitrate parameter takes more effect
 * 5 - only bitrate mode: only bitrate parameter takes effect
 *
 * quality      - quality parameter
 * mpp does not give the direct parameter in different protocol.
 * mpp provide total 5 quality level 1 ~ 5
 * 0 - auto
 * 1 - worst
 * 2 - worse
 * 3 - medium
 * 4 - better
 * 5 - best
 *
 * bit rate parameters
 * mpp gives three bit rate control parameter for control
 * bps_target   - target  bit rate, unit: bit per second
 * bps_max      - maximun bit rate, unit: bit per second
 * bps_min      - minimun bit rate, unit: bit per second
 * if user need constant bit rate set parameters to the similar value
 * if user need variable bit rate set parameters as they need
 *
 * frame rate parameters have great effect on rate control
 * all fps parameter is in 32bit
 * low  16bit is denominator
 * high 16bit is numerator
 * if high 16bit is zero then the whole integer is just fps
 * fps_in       - input  frame rate, unit: frame per second
 *                if 0 then default set to 30
 * fps_out      - output frame rate, unit: frame per second
 *                if 0 then default set to fps_in
 * gop          - gap between Intra frame
 *                0 for only 1 I frame the rest are all P frames
 *                1 for all I frame
 *                2 for I P I P I P
 *                3 for I P P I P P
 *                etc...
 * skip_cnt     - max continuous frame skip count
 *                0 - frame skip is not allow
 *
 */
typedef struct MppEncRcCfg_t {
    RK_S32  rc_mode;
    RK_S32  quality;
    RK_S32  bps_target;
    RK_S32  bps_max;
    RK_S32  bps_min;
    RK_S32  fps_in;
    RK_S32  fps_out;
    RK_S32  gop;
    RK_S32  skip_cnt;
} MppEncRcCfg;

/*
 * Mpp frame parameter
 * direct use MppFrame to store information
 */

/*
 * Mpp codec parameter
 * parameter is defined in different syntax header
 */

/*
 * Mpp preprocess parameter
 */
typedef struct MppEncPrepCfg_t {
    MppFrameFormat      format_in;
    MppFrameFormat      format_out;
    RK_U32              rotation;
} MppEncPrepCfg;

/*
 * Mpp ROI parameter
 * Region configture define a rectangle as ROI
 */
typedef struct MppEncROIRegion_t {
    RK_U16              x;
    RK_U16              y;
    RK_U16              w;
    RK_U16              h;
    RK_U16              intra;
    RK_U16              quality;
} MppEncROIRegion;

typedef struct MppEncROICfg_t {
    RK_U32              number;
    MppEncROIRegion     *regions;
} MppEncROICfg;

/*
 * Mpp OSD parameter
 *
 * Mpp OSD support total 8 regions
 * Mpp OSD support 256-color palette two mode palette:
 * 1. Configurable OSD palette
 *    When palette is set.
 * 2. fixed OSD palette
 *    When palette is NULL.
 *
 * if MppEncOSDPlt.buf != NULL , palette includes maximun 256 levels,
 * every level composed of 32 bits defined below:
 * Y     : 8 bits
 * U     : 8 bits
 * V     : 8 bits
 * alpha : 8 bits
 */
typedef struct MppEncOSDPlt_t {
    RK_U32 buf[256];
} MppEncOSDPlt;

/* position info is unit in 16 pixels(one MB), and
 * x-directon range in pixels = (rd_pos_x - lt_pos_x + 1) * 16;
 * y-directon range in pixels = (rd_pos_y - lt_pos_y + 1) * 16;
 */
typedef struct MppEncOSDRegion_t {
    RK_U32    enable;
    RK_U32    inverse;
    RK_U32    start_mb_x;
    RK_U32    start_mb_y;
    RK_U32    num_mb_x;
    RK_U32    num_mb_y;
    RK_U32    buf_offset;
} MppEncOSDRegion;


/* if num_region > 0 && region==NULL
 * use old osd data
 */
typedef struct MppEncOSDData_t {
    MppBuffer       buf;
    RK_U32          num_region;
    MppEncOSDRegion region[8];
} MppEncOSDData;

/*
 * Mpp Motion Detection parameter
 *
 * Mpp can output Motion Detection infomation for each frame.
 * If user euqueue a encode task with KEY_MOTION_INFO by following function
 * then encoder will output Motion Detection information to the buffer.
 *
 * mpp_task_meta_set_buffer(task, KEY_MOTION_INFO, buffer);
 *
 * Motion Detection information will be organized in this way:
 * 1. Each 16x16 block will have a 32 bit block information which contains
 *    15 bit SAD(Sum of Abstract Difference value
 *    9 bit signed horizontal motion vector
 *    8 bit signed vertical motion vector
 * 2. The sequence of MD information in the buffer is corresponding to the
 *    block position in the frame, left-to right, top-to-bottom.
 * 3. If the width of the frame is not a multiple of 256 pixels (16 macro
 *    blocks), DMA would extend the frame to a multiple of 256 pixels and
 *    the extended blocks' MD information are 32'h0000_0000.
 * 4. Buffer must be ion buffer and 1024 byte aligned.
 */
typedef struct MppEncMDBlkInfo_t {
    RK_U32              sad     : 15;   /* bit  0~14 - SAD */
    RK_S32              mvx     : 9;    /* bit 15~23 - signed horizontal mv */
    RK_S32              mvy     : 8;    /* bit 24~31 - signed vertical mv */
} MppEncMDBlkInfo;

#endif /*__RK_MPI_CMD_H__*/
