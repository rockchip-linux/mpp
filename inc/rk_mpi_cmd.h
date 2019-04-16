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
#include "mpp_frame.h"

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

typedef enum MppEncRcMode_t {
    MPP_ENC_RC_MODE_VBR,
    MPP_ENC_RC_MODE_CBR,
    MPP_ENC_RC_MODE_BUTT
} MppEncRcMode;

typedef enum MppEncRcQuality_t {
    MPP_ENC_RC_QUALITY_WORST,
    MPP_ENC_RC_QUALITY_WORSE,
    MPP_ENC_RC_QUALITY_MEDIUM,
    MPP_ENC_RC_QUALITY_BETTER,
    MPP_ENC_RC_QUALITY_BEST,
    MPP_ENC_RC_QUALITY_CQP,
    MPP_ENC_RC_QUALITY_AQ_ONLY,
    MPP_ENC_RC_QUALITY_BUTT
} MppEncRcQuality;

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
    MPP_ENC_SET_OSD_PLT_CFG,            /* set OSD palette, parameter should be pointer to MppEncOSDPlt */
    MPP_ENC_SET_OSD_DATA_CFG,           /* set OSD data with at most 8 regions, parameter should be pointer to MppEncOSDData */
    MPP_ENC_GET_OSD_CFG,
    MPP_ENC_SET_EXTRA_INFO,
    MPP_ENC_GET_EXTRA_INFO,             /* get vps / sps / pps from hal */
    MPP_ENC_SET_SEI_CFG,                /* SEI: Supplement Enhancemant Information, parameter is MppSeiMode */
    MPP_ENC_GET_SEI_DATA,               /* SEI: Supplement Enhancemant Information, parameter is MppPacket */
    MPP_ENC_PRE_ALLOC_BUFF,             /* allocate buffers before encoding */
    MPP_ENC_SET_QP_RANGE,               /* used for adjusting qp range, the parameter can be 1 or 2 */
    MPP_ENC_SET_ROI_CFG,                /* set MppEncROICfg structure */
    MPP_ENC_SET_CTU_QP,                 /* for H265 Encoder,set CTU's size and QP */
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
 *   |         PrepCfg          |                 |                       |
 *   +---------control---------->     PrepCfg     |                       |
 *   |                          +-----control----->                       |
 *   |                          |                 |        PrepCfg        |
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
 */
typedef enum MppEncRcCfgChange_e {
    MPP_ENC_RC_CFG_CHANGE_RC_MODE       = (1 << 0),
    MPP_ENC_RC_CFG_CHANGE_QUALITY       = (1 << 1),
    MPP_ENC_RC_CFG_CHANGE_BPS           = (1 << 2),     /* change on bps target / max / min */
    MPP_ENC_RC_CFG_CHANGE_FPS_IN        = (1 << 5),     /* change on fps in  flex / numerator / denorminator */
    MPP_ENC_RC_CFG_CHANGE_FPS_OUT       = (1 << 6),     /* change on fps out flex / numerator / denorminator */
    MPP_ENC_RC_CFG_CHANGE_GOP           = (1 << 7),
    MPP_ENC_RC_CFG_CHANGE_SKIP_CNT      = (1 << 8),
    MPP_ENC_RC_CFG_CHANGE_ALL           = (0xFFFFFFFF),
} MppEncRcCfgChange;

typedef struct MppEncRcCfg_t {
    RK_U32  change;

    /*
     * rc_mode - rate control mode
     *
     * mpp provide two rate control mode:
     *
     * Constant Bit Rate (CBR) mode
     * - paramter 'bps*' define target bps
     * - paramter quality and qp will not take effect
     *
     * Variable Bit Rate (VBR) mode
     * - paramter 'quality' define 5 quality levels
     * - paramter 'bps*' is used as reference but not strict condition
     * - special Constant QP (CQP) mode is under VBR mode
     *   CQP mode will work with qp in CodecCfg. But only use for test
     *
     * default: CBR
     */
    MppEncRcMode rc_mode;

    /*
     * quality - quality parameter, only takes effect in VBR mode
     *
     * Mpp does not give the direct parameter in different protocol.
     *
     * Mpp provide total 5 quality level:
     * Worst - worse - Medium - better - best
     *
     * extra CQP level means special constant-qp (CQP) mode
     *
     * default value: Medium
     */
    MppEncRcQuality quality;

    /*
     * bit rate parameters
     * mpp gives three bit rate control parameter for control
     * bps_target   - target  bit rate, unit: bit per second
     * bps_max      - maximun bit rate, unit: bit per second
     * bps_min      - minimun bit rate, unit: bit per second
     * if user need constant bit rate set parameters to the similar value
     * if user need variable bit rate set parameters as they need
     */
    RK_S32  bps_target;
    RK_S32  bps_max;
    RK_S32  bps_min;

    /*
     * frame rate parameters have great effect on rate control
     *
     * fps_in_flex
     * 0 - fix input frame rate
     * 1 - variable input frame rate
     *
     * fps_in_num
     * input frame rate numerator, if 0 then default 30
     *
     * fps_in_denorm
     * input frame rate denorminator, if 0 then default 1
     *
     * fps_out_flex
     * 0 - fix output frame rate
     * 1 - variable output frame rate
     *
     * fps_out_num
     * output frame rate numerator, if 0 then default 30
     *
     * fps_out_denorm
     * output frame rate denorminator, if 0 then default 1
     */
    RK_S32  fps_in_flex;
    RK_S32  fps_in_num;
    RK_S32  fps_in_denorm;
    RK_S32  fps_out_flex;
    RK_S32  fps_out_num;
    RK_S32  fps_out_denorm;

    /*
     * gop - group of picture, gap between Intra frame
     * 0 for only 1 I frame the rest are all P frames
     * 1 for all I frame
     * 2 for I P I P I P
     * 3 for I P P I P P
     * etc...
     */
    RK_S32  gop;

    /*
     * skip_cnt - max continuous frame skip count
     * 0 - frame skip is not allow
     */
    RK_S32  skip_cnt;
} MppEncRcCfg;

/*
 * Mpp preprocess parameter
 */
typedef enum MppEncPrepCfgChange_e {
    MPP_ENC_PREP_CFG_CHANGE_INPUT       = (1 << 0),     /* change on input config */
    MPP_ENC_PREP_CFG_CHANGE_FORMAT      = (1 << 2),     /* change on format */
    /* transform parameter */
    MPP_ENC_PREP_CFG_CHANGE_ROTATION    = (1 << 4),     /* change on ration */
    MPP_ENC_PREP_CFG_CHANGE_MIRRORING   = (1 << 5),     /* change on mirroring */
    /* enhancement parameter */
    MPP_ENC_PREP_CFG_CHANGE_DENOISE     = (1 << 8),     /* change on denoise */
    MPP_ENC_PREP_CFG_CHANGE_SHARPEN     = (1 << 9),     /* change on denoise */
    MPP_ENC_PREP_CFG_CHANGE_ALL         = (0xFFFFFFFF),
} MppEncPrepCfgChange;

/*
 * Preprocess sharpen parameter
 *
 * 5x5 sharpen core
 *
 * enable_y  - enable luma sharpen
 * enable_uv - enable chroma sharpen
 */
typedef struct {
    RK_U32              enable_y;
    RK_U32              enable_uv;
    RK_S32              coef[5];
    RK_S32              div;
    RK_S32              threshold;
} MppEncPrepSharpenCfg;

/*
 * input frame rotation parameter
 * 0 - disable rotation
 * 1 - 90 degree
 * 2 - 180 degree
 * 3 - 270 degree
 */
typedef enum MppEncRotationCfg_t {
    MPP_ENC_ROT_0,
    MPP_ENC_ROT_90,
    MPP_ENC_ROT_180,
    MPP_ENC_ROT_270,
    MPP_ENC_ROT_BUTT
} MppEncRotationCfg;

typedef struct MppEncPrepCfg_t {
    RK_U32              change;

    /*
     * Mpp encoder input data dimension config
     *
     * width / height / hor_stride / ver_stride / format
     * These information will be used for buffer allocation and rc config init
     * The output format is always YUV420. So if input is RGB then color
     * conversion will be done internally
     */
    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;

    /*
     * Mpp encoder input data format config
     */
    MppFrameFormat      format;
    MppFrameColorSpace  color;

    MppEncRotationCfg   rotation;

    /*
     * input frame mirroring parameter
     * 0 - disable mirroring
     * 1 - horizontal mirroring
     * 2 - vertical mirroring
     */
    RK_S32              mirroring;

    /*
     * TODO:
     */
    RK_S32              denoise;

    MppEncPrepSharpenCfg sharpen;
} MppEncPrepCfg;

/**
 * @brief Mpp ROI parameter
 *        Region configure define a rectangle as ROI
 * @note  x, y, w, h are calculated in pixels, which had better be 16-pixel aligned.
 *        These parameters MUST retain in memory when encoder is running.
 *  TODO: Only absolute qp is supported so far, relative qp should be supported
 *        in the future. Also, the ROI regions can be overlaid with each other,
 *        so overlay priority should be considered.
 */
typedef struct MppEncROIRegion_t {
    RK_U16              x;       /**< horizontal position of top left corner */
    RK_U16              y;       /**< vertical position of top left corner */
    RK_U16              w;       /**< width of ROI rectangle */
    RK_U16              h;       /**< height of ROI rectangle */
    RK_U16              intra;   /**< flag of forced intra macroblock */
    RK_U16              quality; /**< absolute qp of macroblock */
} MppEncROIRegion;

/**
 * @brief MPP encoder's ROI configuration
 */
typedef struct MppEncROICfg_t {
    RK_U32              number;  /**< ROI rectangle number */
    MppEncROIRegion     *regions;/**< ROI parameters */
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

/*
 * Mpp video codec related configuration
 */
typedef struct MppEncHwCfg_t {
    RK_U32              change;
    RK_S32              me_search_range_x;
    RK_S32              me_search_range_y;
} MppEncHwCfg;


/*
 * Mpp codec parameter
 * parameter is defined from here
 */

/*
 * H.264 configurable parameter
 */
typedef struct MppEncH264VuiCfg_t {
    RK_U32 change;

    RK_U32 b_vui;

    RK_S32 b_aspect_ratio_info_present;
    RK_S32 i_sar_width;
    RK_S32 i_sar_height;

    RK_S32 b_overscan_info_present;
    RK_S32 b_overscan_info;

    RK_S32 b_signal_type_present;
    RK_S32 i_vidformat;
    RK_S32 b_fullrange;
    RK_S32 b_color_description_present;
    RK_S32 i_colorprim;
    RK_S32 i_transfer;
    RK_S32 i_colmatrix;

    RK_S32 b_chroma_loc_info_present;
    RK_S32 i_chroma_loc_top;
    RK_S32 i_chroma_loc_bottom;

    RK_S32 b_timing_info_present;
    RK_U32 i_num_units_in_tick;
    RK_U32 i_time_scale;
    RK_S32 b_fixed_frame_rate;

    RK_S32 b_nal_hrd_parameters_present;
    RK_S32 b_vcl_hrd_parameters_present;

    struct {
        RK_S32 i_cpb_cnt;
        RK_S32 i_bit_rate_scale;
        RK_S32 i_cpb_size_scale;
        RK_S32 i_bit_rate_value;
        RK_S32 i_cpb_size_value;
        RK_S32 i_bit_rate_unscaled;
        RK_S32 i_cpb_size_unscaled;
        RK_S32 b_cbr_hrd;

        RK_S32 i_initial_cpb_removal_delay_length;
        RK_S32 i_cpb_removal_delay_length;
        RK_S32 i_dpb_output_delay_length;
        RK_S32 i_time_offset_length;
    } hrd;

    RK_S32 b_pic_struct_present;
    RK_S32 b_bitstream_restriction;
    RK_S32 b_motion_vectors_over_pic_boundaries;
    RK_S32 i_max_bytes_per_pic_denom;
    RK_S32 i_max_bits_per_mb_denom;
    RK_S32 i_log2_max_mv_length_horizontal;
    RK_S32 i_log2_max_mv_length_vertical;
    RK_S32 i_num_reorder_frames;
    RK_S32 i_max_dec_frame_buffering;

    /* FIXME to complete */
} MppEncH264VuiCfg;

typedef struct MppEncH264RefCfg_t {
    RK_S32         i_frame_reference;  /* Maximum number of reference frames */
    RK_S32         i_ref_pos;
    RK_S32         i_long_term_en;
    RK_S32         i_long_term_internal;
    RK_S32         hw_longterm_mode;
    RK_S32         i_dpb_size;         /* Force a DPB size larger than that implied by B-frames and reference frames.
                                        * Useful in combination with interactive error resilience. */
} MppEncH264RefCfg;

typedef struct MppEncH264SeiCfg_t {
    RK_U32              change;
} MppEncH264SeiCfg;

typedef enum MppEncH264CfgChange_e {
    /* change on stream type */
    MPP_ENC_H264_CFG_STREAM_TYPE            = (1 << 0),
    /* change on svc / profile / level */
    MPP_ENC_H264_CFG_CHANGE_PROFILE         = (1 << 1),
    /* change on entropy_coding_mode / cabac_init_idc */
    MPP_ENC_H264_CFG_CHANGE_ENTROPY         = (1 << 2),

    /* change on transform8x8_mode */
    MPP_ENC_H264_CFG_CHANGE_TRANS_8x8       = (1 << 4),
    /* change on constrained_intra_pred_mode */
    MPP_ENC_H264_CFG_CHANGE_CONST_INTRA     = (1 << 5),
    /* change on chroma_cb_qp_offset/ chroma_cr_qp_offset */
    MPP_ENC_H264_CFG_CHANGE_CHROMA_QP       = (1 << 6),
    /* change on deblock_disable / deblock_offset_alpha / deblock_offset_beta */
    MPP_ENC_H264_CFG_CHANGE_DEBLOCKING      = (1 << 7),
    /* change on use_longterm */
    MPP_ENC_H264_CFG_CHANGE_LONG_TERM       = (1 << 8),

    /* change on max_qp / min_qp / max_qp_step */
    MPP_ENC_H264_CFG_CHANGE_QP_LIMIT        = (1 << 16),
    /* change on intra_refresh_mode / intra_refresh_arg */
    MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH   = (1 << 17),
    /* change on slice_mode / slice_arg */
    MPP_ENC_H264_CFG_CHANGE_SLICE_MODE      = (1 << 18),

    /* change on vui */
    MPP_ENC_H264_CFG_CHANGE_VUI             = (1 << 28),
    /* change on sei */
    MPP_ENC_H264_CFG_CHANGE_SEI             = (1 << 29),
    MPP_ENC_H264_CFG_CHANGE_REF             = (1 << 30),
    MPP_ENC_H264_CFG_CHANGE_ALL             = (0xFFFFFFFF),
} MppEncH264CfgChange;

typedef struct MppEncH264Cfg_t {
    RK_U32              change;

    /*
     * H.264 stream format
     * 0 - H.264 Annex B: NAL unit starts with '00 00 00 01'
     * 1 - Plain NAL units without startcode
     */
    RK_S32              stream_type;

    /* H.264 codec syntax config */
    RK_S32              svc;                        /* 0 - avc 1 - svc */

    /*
     * H.264 profile_idc parameter
     * 66  - Baseline profile
     * 77  - Main profile
     * 100 - High profile
     */
    RK_S32              profile;

    /*
     * H.264 level_idc parameter
     * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
     * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
     * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
     * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
     * 50 / 51 / 52         - 4K@30fps
     */
    RK_S32              level;

    /*
     * H.264 entropy coding method
     * 0 - CAVLC
     * 1 - CABAC
     * When CABAC is select cabac_init_idc can be range 0~2
     */
    RK_S32              entropy_coding_mode;
    RK_S32              cabac_init_idc;

    /*
     * 8x8 intra prediction and 8x8 transform enable flag
     * This flag can only be enable under High profile
     * 0 : disable (BP/MP)
     * 1 : enable  (HP)
     */
    RK_S32              transform8x8_mode;

    /*
     * 0 : disable
     * 1 : enable
     */
    RK_S32              constrained_intra_pred_mode;

    /*
     * chroma qp offset (-12 - 12)
     */
    RK_S32              chroma_cb_qp_offset;
    RK_S32              chroma_cr_qp_offset;

    /*
     * H.264 deblock filter mode flag
     * 0 : enable
     * 1 : disable
     * 2 : disable deblocking filter at slice boundaries
     *
     * deblock filter offset alpha (-6 - 6)
     * deblock filter offset beta  (-6 - 6)
     */
    RK_S32              deblock_disable;
    RK_S32              deblock_offset_alpha;
    RK_S32              deblock_offset_beta;

    /*
     * H.264 long term reference picture enable flag
     * 0 - disable
     * 1 - enable
     */
    RK_S32              use_longterm;

    /*
     * quality config
     * qp_max       - 8 ~ 51
     * qp_min       - 0 ~ 48
     * qp_max_step  - max delta qp step between two frames
     */
    RK_S32              qp_init;
    RK_S32              qp_max;
    RK_S32              qp_min;
    RK_S32              qp_max_step;

    /*
     * intra fresh config
     *
     * intra_refresh_mode
     * 0 - no intra refresh
     * 1 - intra refresh by MB row
     * 2 - intra refresh by MB column
     * 3 - intra refresh by MB gap
     *
     * intra_refresh_arg
     * mode 0 - no effect
     * mode 1 - refresh MB row number
     * mode 2 - refresh MB colmn number
     * mode 3 - refresh MB gap count
     */
    RK_S32              intra_refresh_mode;
    RK_S32              intra_refresh_arg;

    /* slice mode config */
    RK_S32              slice_mode;
    RK_S32              slice_arg;

    /* extra info */
    MppEncH264VuiCfg    vui;
    MppEncH264SeiCfg    sei;
    MppEncH264RefCfg    ref;
} MppEncH264Cfg;


#define H265E_MAX_ROI_NUMBER  64
typedef struct H265eRect_t {
    RK_S32 left;
    RK_S32 right;
    RK_S32 top;
    RK_S32 bottom;
} H265eRect;

typedef struct H265eRoi_Region_t {
    RK_U8 level;
    H265eRect rect;
} H265eRoiRegion;

/*
 * roi region only can be setting when rc_enable = 1
 */
typedef struct MppEncH265RoiCfg_t {
    /*
     * the value is defined by H265eCtuMethod
     */

    RK_U8 method;
    /*
     * the number of roi,the value must less than H265E_MAX_ROI_NUMBER
     */
    RK_S32 num;

    /* delat qp using in roi region*/
    RK_U32 delta_qp;

    /* roi region */
    H265eRoiRegion region[H265E_MAX_ROI_NUMBER];
} MppEncH265RoiCfg;

typedef struct H265eCtuQp_t {
    /* the qp value using in ctu region */
    RK_U32 qp;

    /*
     * define the ctu region
     * method = H265E_METHOD_CUT_SIZE, the value of rect is in ctu size
     * method = H264E_METHOD_COORDINATE,the value of rect is in coordinates
     */
    H265eRect rect;
} H265eCtu;

typedef struct H265eCtuRegion_t {
    /*
     * the value is defined by H265eCtuMethod
     */
    RK_U8 method;

    /*
     * the number of ctu,the value must less than H265E_MAX_ROI_NUMBER
     */
    RK_S32 num;

    /* ctu region */
    H265eCtu ctu[H265E_MAX_ROI_NUMBER];
} MppEncH265CtuCfg;

/*
 * define the method when set CTU/ROI parameters
 * this value is using by method in H265eCtuRegion or H265eRoi struct
 */
typedef enum {
    H265E_METHOD_CTU_SIZE,
    H264E_METHOD_COORDINATE,
} H265eCtuMethod;

/*
 * H.265 configurable parameter
 */
typedef struct MppEncH265VuiCfg_t {
    RK_U32              change;
    RK_S32              vui_present;
    RK_S32              vui_aspect_ratio;
    RK_S32              vui_sar_size;
    RK_S32              full_range;
    RK_S32              time_scale;
} MppEncH265VuiCfg;

typedef struct MppEncH265SeiCfg_t {
    RK_U32              change;
} MppEncH265SeiCfg;


typedef enum MppEncH265CfgChange_e {
    /* change on stream type */
    MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE  = (1 << 0),
    MPP_ENC_H265_CFG_INTRA_QP_CHANGE             = (1 << 1),
    MPP_ENC_H265_CFG_FRAME_RATE_CHANGE           = (1 << 2),
    MPP_ENC_H265_CFG_BITRATE_CHANGE              = (1 << 3),
    MPP_ENC_H265_CFG_GOP_SIZE                    = (1 << 4),
    MPP_ENC_H265_CFG_RC_QP_CHANGE                = (1 << 5),
    MPP_ENC_H265_CFG_INTRA_REFRESH_CHANGE        = (1 << 6),
    MPP_ENC_H265_CFG_INDEPEND_SLICE_CHANGE       = (1 << 7),
    MPP_ENC_H265_CFG_DEPEND_SLICE_CHANGE         = (1 << 8),
    MPP_ENC_H265_CFG_CTU_CHANGE                  = (1 << 9),
    MPP_ENC_H265_CFG_ROI_CHANGE                  = (1 << 10),
    MPP_ENC_H265_CFG_CHANGE_ALL                  = (0xFFFFFFFF),
} MppEncH265CfgChange;

typedef struct MppEncH265Cfg_t {
    RK_U32              change;

    /* H.265 codec syntax config */
    RK_S32              profile;
    RK_S32              level;
    RK_S32              tier;
    RK_S32              const_intra_pred;           /* constraint intra prediction flag */
    RK_S32              ctu_size;
    RK_S32              tmvp_enable;
    RK_S32              wpp_enable;
    RK_S32              merge_range;
    RK_S32              sao_enable;

    /* quality config */
    RK_S32              max_qp;
    RK_S32              min_qp;
    RK_S32              max_delta_qp;
    RK_S32              intra_qp;
    RK_S32              gop_delta_qp;

    /* intra fresh config */
    RK_S32              intra_refresh_mode;
    RK_S32              intra_refresh_arg;

    /* slice mode config */
    RK_S32              independ_slice_mode;
    RK_S32              independ_slice_arg;
    RK_S32              depend_slice_mode;
    RK_S32              depend_slice_arg;

    /* extra info */
    MppEncH265VuiCfg    vui;
    MppEncH265SeiCfg    sei;

    MppEncH265CtuCfg    ctu;
    MppEncH265RoiCfg    roi;
} MppEncH265Cfg;

/*
 * motion jpeg configurable parameter
 */
typedef enum MppEncJpegCfgChange_e {
    /* change on quant parameter */
    MPP_ENC_JPEG_CFG_CHANGE_QP              = (1 << 0),
    MPP_ENC_JPEG_CFG_CHANGE_ALL             = (0xFFFFFFFF),
} MppEncJpegCfgChange;

typedef struct MppEncJpegCfg_t {
    RK_U32              change;
    RK_S32              quant;
} MppEncJpegCfg;

/*
 * vp8 configurable parameter
 */
typedef struct MppEncVp8Cfg_t {
    RK_U32              change;
    RK_S32              quant;
} MppEncVp8Cfg;

#endif /*__RK_MPI_CMD_H__*/
