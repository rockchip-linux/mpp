/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __RK_VENC_CMD_H__
#define __RK_VENC_CMD_H__

#include "mpp_frame.h"
#include "rk_venc_rc.h"

/*
 * Configure of encoder is very complicated. So we divide configures into
 * four parts:
 *
 * 1. Rate control parameter
 *    This is quality and bitrate request from user.
 *
 * 2. Data source MppFrame parameter
 *    This is data source buffer information.
 *    Now it is PreP config
 *    PreP  : Encoder Preprocess configuration
 *
 * 3. Video codec infomation
 *    This is user custormized stream information.
 *    including:
 *    H.264 / H.265 / vp8 / mjpeg
 *
 * 4. Misc parameter
 *    including:
 *    Split : Slice split configuration
 *    GopRef: Reference gop configuration
 *    ROI   : Region Of Interest
 *    OSD   : On Screen Display
 *    MD    : Motion Detection
 *
 * The module transcation flow is as follows:
 *
 *                 +                      +
 *     User        |      Mpi/Mpp         |          EncImpl
 *                 |                      |            Hal
 *                 |                      |
 * +----------+    |    +---------+       |       +-----------+
 * |          |    |    |         +-----RcCfg----->           |
 * |  RcCfg   +--------->         |       |       |  EncImpl  |
 * |          |    |    |         |   +-Frame----->           |
 * +----------+    |    |         |   |   |       +--+-----^--+
 *                 |    |         |   |   |          |     |
 *                 |    |         |   |   |          |     |
 * +----------+    |    |         |   |   |       syntax   |
 * |          |    |    |         |   |   |          |     |
 * | MppFrame +--------->  MppEnc +---+   |          |   result
 * |          |    |    |         |   |   |          |     |
 * +----------+    |    |         |   |   |          |     |
 *                 |    |         |   |   |       +--v-----+--+
 *                 |    |         |   +-Frame----->           |
 * +----------+    |    |         |       |       |           |
 * |          |    |    |         +---CodecCfg---->    Hal    |
 * | CodecCfg +--------->         |       |       |           |
 * |          |    |    |         <-----Extra----->           |
 * +----------+    |    +---------+       |       +-----------+
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
 * encoder query interface is only for debug usage
 */
#define MPP_ENC_QUERY_STATUS        (0x00000001)
#define MPP_ENC_QUERY_WAIT          (0x00000002)
#define MPP_ENC_QUERY_FPS           (0x00000004)
#define MPP_ENC_QUERY_BPS           (0x00000008)
#define MPP_ENC_QUERY_ENC_IN_FRM    (0x00000010)
#define MPP_ENC_QUERY_ENC_WORK      (0x00000020)
#define MPP_ENC_QUERY_ENC_OUT_PKT   (0x00000040)

#define MPP_ENC_QUERY_ALL           (MPP_ENC_QUERY_STATUS       | \
                                     MPP_ENC_QUERY_WAIT         | \
                                     MPP_ENC_QUERY_FPS          | \
                                     MPP_ENC_QUERY_BPS          | \
                                     MPP_ENC_QUERY_ENC_IN_FRM   | \
                                     MPP_ENC_QUERY_ENC_WORK     | \
                                     MPP_ENC_QUERY_ENC_OUT_PKT)

typedef struct MppEncQueryCfg_t {
    /*
     * 32 bit query flag for query data check
     * Each bit represent a query data switch.
     * bit 0 - for querying encoder runtime status
     * bit 1 - for querying encoder runtime waiting status
     * bit 2 - for querying encoder realtime encode fps
     * bit 3 - for querying encoder realtime output bps
     * bit 4 - for querying encoder input frame count
     * bit 5 - for querying encoder start hardware times
     * bit 6 - for querying encoder output packet count
     */
    RK_U32      query_flag;

    /* 64 bit query data output */
    RK_U32      rt_status;
    RK_U32      rt_wait;
    RK_U32      rt_fps;
    RK_U32      rt_bps;
    RK_U32      enc_in_frm_cnt;
    RK_U32      enc_hw_run_cnt;
    RK_U32      enc_out_pkt_cnt;
} MppEncQueryCfg;

/*
 * base working mode parameter
 */
typedef struct MppEncBaseCfg_t {
    MppCodingType coding;
    RK_S32  low_delay;
    RK_S32  smart_en;
    RK_S32  smt1_en;
    RK_S32  smt3_en;
} MppEncBaseCfg;

/*
 * Rate control parameter
 */
typedef enum MppEncRcQuality_e {
    MPP_ENC_RC_QUALITY_WORST,
    MPP_ENC_RC_QUALITY_WORSE,
    MPP_ENC_RC_QUALITY_MEDIUM,
    MPP_ENC_RC_QUALITY_BETTER,
    MPP_ENC_RC_QUALITY_BEST,
    MPP_ENC_RC_QUALITY_CQP,
    MPP_ENC_RC_QUALITY_AQ_ONLY,
    MPP_ENC_RC_QUALITY_BUTT
} MppEncRcQuality;

typedef struct MppEncRcCfg_t {
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
     * fps_in_denom
     * input frame rate denominator, if 0 then default 1
     *
     * fps_out_flex
     * 0 - fix output frame rate
     * 1 - variable output frame rate
     *
     * fps_out_num
     * output frame rate numerator, if 0 then default 30
     *
     * fps_out_denom
     * output frame rate denominator, if 0 then default 1
     */
    RK_S32  fps_in_flex;
    RK_S32  fps_in_num;
    RK_S32  fps_in_denom;
    RK_S32  fps_out_flex;
    RK_S32  fps_out_num;
    RK_S32  fps_out_denom;
    /*
     * Whether to encoder IDR when fps_out is changed.
     * 0 -- default value, SPS, PPS headers and IDR will be added.
     * 1 -- only SPS, PPS headers is added.
     */
    RK_S32  fps_chg_no_idr;

    /*
     * gop - group of picture, gap between Intra frame
     * 0 for only 1 I frame the rest are all P frames
     * 1 for all I frame
     * 2 for I P I P I P
     * 3 for I P P I P P
     * etc...
     */
    RK_S32  gop;
    /* internal gop mode: 0 - normal P; 1 - smart P */
    RK_S32                  gop_mode;
    void    *ref_cfg;

    /*
     * skip_cnt - max continuous frame skip count
     * 0 - frame skip is not allow
     */
    RK_S32  skip_cnt;

    /*
     * max_reenc_times - max reencode time for one frame
     * 0 - reencode is not allowed
     * 1~3 max reencode time is limited to 3
     */
    RK_U32  max_reenc_times;

    /*
     * stats_time   - the time of bitrate statistics
     */
    RK_S32  stats_time;

    /*
     * drop frame parameters
     * used on bitrate is far over the max bitrate
     *
     * drop_mode
     *
     * MPP_ENC_RC_DROP_FRM_DISABLED
     * - do not drop frame when bitrate overflow.
     * MPP_ENC_RC_DROP_FRM_NORMAL
     * - do not encode the dropped frame when bitrate overflow.
     * MPP_ENC_RC_DROP_FRM_PSKIP
     * - encode a all skip frame when bitrate overflow.
     *
     * drop_threshold
     *
     * The percentage threshold over max_bitrate for trigger frame drop.
     *
     * drop_gap
     * The max continuous frame drop number
     */
    MppEncRcDropFrmMode     drop_mode;
    RK_U32                  drop_threshold;
    RK_U32                  drop_gap;

    MppEncRcSuperFrameMode  super_mode;
    RK_U32                  super_i_thd;
    RK_U32                  super_p_thd;

    MppEncRcPriority        rc_priority;

    RK_U32                  debreath_en;
    RK_U32                  debre_strength;
    RK_S32                  max_i_prop;
    RK_S32                  min_i_prop;
    RK_S32                  init_ip_ratio;

    /* general qp control */
    RK_S32                  qp_init;
    RK_S32                  qp_max;
    RK_S32                  qp_max_i;
    RK_S32                  qp_min;
    RK_S32                  qp_min_i;
    RK_S32                  qp_max_step;                /* delta qp between each two P frame */
    RK_S32                  qp_delta_ip;                /* delta qp between I and P */
    RK_S32                  qp_delta_vi;                /* delta qp between vi and P */
    RK_S32                  fqp_min_i;
    RK_S32                  fqp_min_p;
    RK_S32                  fqp_max_i;
    RK_S32                  fqp_max_p;
    RK_S32                  mt_st_swth_frm_qp;

    RK_S32                  hier_qp_en;
    RK_S32                  hier_qp_delta[4];
    RK_S32                  hier_frame_num[4];

    RK_U32                  refresh_en;
    MppEncRcRefreshMode     refresh_mode;
    RK_U32                  refresh_num;
    RK_S32                  refresh_length;
    RK_S32                  inst_br_lvl;
} MppEncRcCfg;

/*
 * Hardware related rate control config
 *
 * This config will open some detail feature to external user to control
 * hardware behavior directly.
 */
typedef struct MppEncHwCfg_t {
    /* vepu541/vepu540 */
    RK_S32                  qp_delta_row;               /* delta qp between two row in P frame */
    RK_S32                  qp_delta_row_i;             /* delta qp between two row in I frame */
    RK_S32                  qbias_i;
    RK_S32                  qbias_p;
    RK_S32                  qbias_en;
    RK_S32                  flt_str_i;
    RK_S32                  flt_str_p;
    RK_U32                  aq_thrd_i[16];
    RK_U32                  aq_thrd_p[16];
    RK_S32                  aq_step_i[16];
    RK_S32                  aq_step_p[16];

    /* vepu1/2 */
    RK_S32                  mb_rc_disable;

    /* vepu580 */
    RK_S32                  extra_buf;

    /*
     * block mode decision bias config
     * 0 - intra32x32
     * 1 - intra16x16
     * 2 - intra8x8
     * 3 - intra4x4
     * 4 - inter64x64
     * 5 - inter32x32
     * 6 - inter16x16
     * 7 - inter8x8
     * value range 0 ~ 15, default : 8
     * If the value is smaller then encoder will be more likely to encode corresponding block mode.
     */
    RK_S32                  mode_bias[8];

    /*
     * skip mode bias config
     * skip_bias_en - enable flag for skip bias config
     * skip_sad     - sad threshold for skip / non-skip
     * skip_bias    - tendency for skip, value range 0 ~ 15, default : 8
     *                If the value is smaller then encoder will be more likely to encode skip block.
     */
    RK_S32                  skip_bias_en;
    RK_S32                  skip_sad;
    RK_S32                  skip_bias;

    /* vepu500
     * 0-2: I frame thd; 3-6: I frame bias
     * 7-9: P frame thd; 10-13: I block bias of P frame
     * 14-17: P block bias of P frame
     */
    RK_S32                  qbias_arr[18];
    /* vepu500
     * 0: aq16_range; 1: aq32_range; 2: aq8_range
     * 3: aq16_diff0; 4: aq16_diff1
     * 0 ~ 4 for I frame, 5 ~ 9 for P frame
     */
    RK_S32                  aq_rnge_arr[10];
} MppEncHwCfg;

/*
 * Mpp preprocess parameter
 */
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
typedef enum MppEncRotationCfg_e {
    MPP_ENC_ROT_0,
    MPP_ENC_ROT_90,
    MPP_ENC_ROT_180,
    MPP_ENC_ROT_270,
    MPP_ENC_ROT_BUTT
} MppEncRotationCfg;

typedef struct MppEncPrepCfg_t {
    /*
     * Mpp encoder input data dimension config
     *
     * width / height / hor_stride / ver_stride / format
     * These information will be used for buffer allocation and rc config init
     * The output format is always YUV420. So if input is RGB then color
     * conversion will be done internally
     */
    /* width / height set - user set value */
    RK_S32              width_set;
    RK_S32              height_set;
    /* width / height - final value in bitstream */
    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;
    RK_S32              max_width;
    RK_S32              max_height;

    /* resolution change flag */
    RK_S32              change_res;

    /*
     * Mpp encoder input/output color config
     */
    MppFrameFormat      format;
    MppFrameColorSpace  color;
    MppFrameColorPrimaries colorprim;
    MppFrameColorTransferCharacteristic colortrc;
    MppFrameColorRange  range;
    MppFrameChromaFormat format_out;
    MppFrameChromaDownSampleMode chroma_ds_mode;
    MppFrameColorRange  range_out;
    RK_S32              fix_chroma_en;
    RK_S32              fix_chroma_u;
    RK_S32              fix_chroma_v;

    /* suffix ext means the user set config externally */
    MppEncRotationCfg   rotation;
    MppEncRotationCfg   rotation_ext;

    /*
     * input frame mirroring parameter
     * 0 - disable mirroring
     * 1 - horizontal mirroring
     */
    RK_S32              mirroring;
    RK_S32              mirroring_ext;

    /*
     * input frame flip parameter
     * 0 - disable flip
     * 1 - flip, vertical mirror transformation
     */
    RK_S32              flip;

    /*
     * TODO:
     */
    RK_S32              denoise;

    MppEncPrepSharpenCfg sharpen;
} MppEncPrepCfg;

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

typedef enum MppEncHeaderMode_e {
    /* default mode: attach vps/sps/pps only on first frame */
    MPP_ENC_HEADER_MODE_DEFAULT,
    /* IDR mode: attach vps/sps/pps on each IDR frame */
    MPP_ENC_HEADER_MODE_EACH_IDR,
    MPP_ENC_HEADER_MODE_BUTT,
} MppEncHeaderMode;

typedef enum MppEncSeiMode_e {
    MPP_ENC_SEI_MODE_DISABLE,                /* default mode, SEI writing is disabled */
    MPP_ENC_SEI_MODE_ONE_SEQ,                /* one sequence has only one SEI */
    MPP_ENC_SEI_MODE_ONE_FRAME               /* one frame may have one SEI, if SEI info has changed */
} MppEncSeiMode;

/*
 * Mpp codec parameter
 * parameter is defined from here
 */
typedef struct MppEncVuiCfg_t {
    RK_U32              change;
    RK_S32              vui_en;
    RK_S32              vui_aspect_ratio;
    RK_S32              vui_sar_size;
    RK_S32              full_range;
    RK_S32              time_scale;
} MppEncVuiCfg;

/*
 * H.264 configurable parameter
 */
/* default H.264 hardware config */
typedef struct MppEncH264HwCfg_t {
    /*
     * VEPU 1/2 : 2
     * others   : 0
     */
    RK_U32 hw_poc_type;
    /*
     * VEPU 1/2 : fixed to 12
     * others   : changeable, default 12
     */
    RK_U32 hw_log2_max_frame_num_minus4;
    /* default 0, only RKVENC2 support split out */
    RK_U32 hw_split_out;
} MppEncH264HwCfg;

typedef struct MppEncH264Cfg_t {
    /*
     * H.264 stream format
     * 0 - H.264 Annex B: NAL unit starts with '00 00 00 01'
     * 1 - Plain NAL units without startcode
     */
    RK_S32              stream_type;

    /*
     * H.264 codec syntax config
     *
     * do NOT setup the three option below unless you are familiar with encoder detail
     * poc_type             - picture order count type 0 ~ 2
     * log2_max_poc_lsb     - used in sps with poc_type 0,
     * log2_max_frame_num   - used in sps
     */
    RK_U32              poc_type;
    RK_U32              log2_max_poc_lsb;
    RK_U32              log2_max_frame_num; /* actually log2_max_frame_num_minus4 */
    RK_U32              gaps_not_allowed;

    MppEncH264HwCfg     hw_cfg;

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
    RK_S32              entropy_coding_mode_ex;
    RK_S32              cabac_init_idc;
    RK_S32              cabac_init_idc_ex;

    /*
     * 8x8 intra prediction and 8x8 transform enable flag
     * This flag can only be enable under High profile
     * 0 : disable (BP/MP)
     * 1 : enable  (HP)
     */
    RK_S32              transform8x8_mode;
    RK_S32              transform8x8_mode_ex;

    /*
     * 0 : disable
     * 1 : enable
     */
    RK_S32              constrained_intra_pred_mode;

    /*
     * 0 : flat scaling list
     * 1 : default scaling list for all cases
     * 2 : customized scaling list (not supported)
     */
    RK_S32              scaling_list_mode;

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
     * qp_max       - 8  ~ 51
     * qp_max_i     - 10 ~ 40
     * qp_min       - 8  ~ 48
     * qp_min_i     - 10 ~ 40
     * qp_max_step  - max delta qp step between two frames
     */
    RK_S32              qp_init;
    RK_S16              qp_max;
    RK_S16              qp_max_i;
    RK_S16              qp_min;
    RK_S16              qp_min_i;
    RK_S16              qp_max_step;
    RK_S16              qp_delta_ip;

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

    /* extra mode config */
    RK_S32              max_ltr_frames;
    RK_S32              max_tid;
    RK_S32              prefix_mode;
    RK_S32              base_layer_pid;
    /*
     * Mpp encoder constraint_set parameter
     * Mpp encoder constraint_set controls constraint_setx_flag in AVC.
     * Mpp encoder constraint_set uses type RK_U32 to store force_flag and constraint_force as followed.
     * | 00 | force_flag | 00 | constraint_force |
     * As for force_flag and constraint_force, only low 6 bits are valid,
     * corresponding to constraint_setx_flag from 5 to 0.
     * If force_flag bit is enabled, constraint_setx_flag will be set correspondingly.
     * Otherwise, constraint_setx_flag will use default value.
     */
    RK_U32              constraint_set;

    /* extra info */
    MppEncVuiCfg        vui;
} MppEncH264Cfg;

#define H265E_MAX_ROI_NUMBER  64

typedef struct H265eRect_t {
    RK_S32              left;
    RK_S32              right;
    RK_S32              top;
    RK_S32              bottom;
} H265eRect;

typedef struct H265eRoi_Region_t {
    RK_U8               level;
    H265eRect           rect;
} H265eRoiRegion;

typedef struct H265eCtuQp_t {
    /* the qp value using in ctu region */
    RK_U32              qp;

    /*
     * define the ctu region
     * method = H265E_METHOD_CUT_SIZE, the value of rect is in ctu size
     * method = H264E_METHOD_COORDINATE,the value of rect is in coordinates
     */
    H265eRect           rect;
} H265eCtu;

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
typedef struct MppEncH265SliceCfg_t {
    /* default value: 0, means no slice split*/
    RK_U32  split_enable;

    /* 0: by bits number; 1: by lcu line number*/
    RK_U32  split_mode;

    /*
     * when splitmode is 0, this value presents bits number,
     * when splitmode is 1, this value presents lcu line number
     */
    RK_U32  slice_size;
    RK_U32  slice_out;
} MppEncH265SliceCfg;

typedef struct MppEncH265CuCfg_t {
    RK_U32  cu32x32_en;                             /*default: 1 */
    RK_U32  cu16x16_en;                             /*default: 1 */
    RK_U32  cu8x8_en;                               /*default: 1 */
    RK_U32  cu4x4_en;                               /*default: 1 */

    // intra pred
    RK_U32  constrained_intra_pred_flag;            /*default: 0 */
    RK_U32  strong_intra_smoothing_enabled_flag;    /*INTRA_SMOOTH*/
    RK_U32  pcm_enabled_flag;                       /*default: 0, enable ipcm*/
    RK_U32  pcm_loop_filter_disabled_flag;
} MppEncH265CuCfg;

typedef struct MppEncH265DblkCfg_t {
    RK_U32  slice_deblocking_filter_disabled_flag;  /* default value: 0. {0,1} */
    RK_S32  slice_beta_offset_div2;                 /* default value: 0. [-6,+6] */
    RK_S32  slice_tc_offset_div2;                   /* default value: 0. [-6,+6] */
} MppEncH265DblkCfg_t;

typedef struct MppEncH265SaoCfg_t {
    RK_U32  slice_sao_luma_disable;
    RK_U32  slice_sao_chroma_disable;
    RK_U32  sao_bit_ratio;
} MppEncH265SaoCfg;

typedef struct MppEncH265TransCfg_t {
    RK_U32  transquant_bypass_enabled_flag;
    RK_U32  transform_skip_enabled_flag;
    RK_U32  scaling_list_mode;                      /* default: 0 */
    RK_S32  cb_qp_offset;
    RK_S32  cr_qp_offset;
    RK_S32  diff_cu_qp_delta_depth;
} MppEncH265TransCfg;

typedef struct MppEncH265MergeCfg_t {
    RK_U32  max_mrg_cnd;
    RK_U32  merge_up_flag;
    RK_U32  merge_left_flag;
} MppEncH265MergesCfg;

typedef struct MppEncH265EntropyCfg_t {
    RK_U32  cabac_init_flag;                        /* default: 0 */
} MppEncH265EntropyCfg;

typedef struct MppEncH265Cfg_t {
    /* H.265 codec syntax config */
    RK_S32              profile;
    RK_S32              level;
    RK_S32              tier;

    /* constraint intra prediction flag */
    RK_S32              const_intra_pred;
    RK_S32              ctu_size;
    RK_S32              max_cu_size;
    RK_S32              tmvp_enable;
    RK_S32              amp_enable;
    RK_S32              wpp_enable;
    RK_S32              merge_range;
    RK_S32              sao_enable;
    RK_U32              num_ref;

    /* quality config */
    RK_S32              intra_qp;
    RK_U8               qpmax_map[8];
    RK_U8               qpmin_map[8];
    RK_S32              qpmap_mode;

    /* extra mode config */
    RK_S32              max_ltr_frames;
    RK_S32              max_tid;
    RK_S32              base_layer_pid;

    MppEncH265CuCfg      cu_cfg;
    MppEncH265SliceCfg   slice_cfg;
    MppEncH265EntropyCfg entropy_cfg;
    MppEncH265TransCfg   trans_cfg;
    MppEncH265SaoCfg     sao_cfg;
    MppEncH265DblkCfg_t  dblk_cfg;
    MppEncH265MergesCfg  merge_cfg;
    RK_S32               auto_tile;
    RK_U32               lpf_acs_sli_en;
    RK_U32               lpf_acs_tile_disable;

    /* extra info */
    MppEncVuiCfg        vui;
} MppEncH265Cfg;

/*
 * motion jpeg configurable parameter
 */
typedef enum MppEncJpegQpMode_e {
    JPEG_QP_NA                              = 0,
    JPEG_QUANT                              = 1,
    JPEG_QFACTOR                            = 2,
    JPEG_QTABLE                             = 3,
} MppEncJpegQpMode;

typedef struct MppEncJpegCfg_t {
    RK_S32              q_mode;
    RK_S32              update;

    RK_S32              quant;
    RK_S32              quant_ext;
    /*
     * quality factor config
     *
     * q_factor     - 1  ~ 99
     * qf_max       - 1  ~ 99
     * qf_min       - 1  ~ 99
     * qtable_y: qtable for luma
     * qtable_u: qtable for chroma
     * qtable_v: default equal qtable_u
     */
    RK_S32              q_factor;
    RK_S32              q_factor_ext;
    RK_S32              qf_max;
    RK_S32              qf_max_ext;
    RK_S32              qf_min;
    RK_S32              qf_min_ext;
    /*
     * qtable_y: qtable for luma
     * qtable_u: qtable for chroma u
     * qtable_v: qtable for chroma v
     * for most case u and v use the same table
     */
    RK_U8               qtable_y[64];
    RK_U8               qtable_u[64];
    RK_U8               qtable_v[64];
} MppEncJpegCfg;

/*
 * vp8 configurable parameter
 */
typedef struct MppEncVp8Cfg_t {
    RK_S32              quant;
    RK_S32              disable_ivf;
} MppEncVp8Cfg;

typedef enum MppEncSplitMode_e {
    MPP_ENC_SPLIT_NONE,
    MPP_ENC_SPLIT_BY_BYTE,
    MPP_ENC_SPLIT_BY_CTU,
    MPP_ENC_SPLIT_MODE_BUTT,
} MppEncSplitMode;

typedef enum MppEncSplitOutMode_e {
    MPP_ENC_SPLIT_OUT_LOWDELAY              = (1 << 0),
    MPP_ENC_SPLIT_OUT_SEGMENT               = (1 << 1),
} MppEncSplitOutMode;

typedef struct MppEncSliceSplit_t {
    /*
     * slice split mode
     *
     * MPP_ENC_SPLIT_NONE    - No slice is split
     * MPP_ENC_SPLIT_BY_BYTE - Slice is split by byte number
     * MPP_ENC_SPLIT_BY_CTU  - Slice is split by macroblock / ctu number
     */
    RK_U32  split_mode;

    /*
     * slice split size parameter
     *
     * When split by byte number this value is the max byte number for each
     * slice.
     * When split by macroblock / ctu number this value is the MB/CTU number
     * for each slice.
     */
    RK_U32  split_arg;

    /*
     * slice split output mode
     *
     * MPP_ENC_SPLIT_OUT_LOWDELAY
     * - When enabled encoder will lowdelay output each slice in a single packet
     * MPP_ENC_SPLIT_OUT_SEGMENT
     * - When enabled encoder will packet with segment info for each slice
     */
    RK_U32  split_out;
} MppEncSliceSplit;

/**
 * @brief Mpp ROI parameter
 *        Region configure define a rectangle as ROI
 * @note  x, y, w, h are calculated in pixels, which had better be 16-pixel aligned.
 *        These parameters MUST retain in memory when encoder is running.
 *        Both absolute qp and relative qp are supported in vepu541.
 *        Only absolute qp is supported in rv1108
 */
typedef struct MppEncROIRegion_t {
    RK_U16              x;              /**< horizontal position of top left corner */
    RK_U16              y;              /**< vertical position of top left corner */
    RK_U16              w;              /**< width of ROI rectangle */
    RK_U16              h;              /**< height of ROI rectangle */
    RK_U16              intra;          /**< flag of forced intra macroblock */
    RK_S16              quality;        /**< absolute / relative qp of macroblock */
    RK_U16              qp_area_idx;    /**< qp min max area select*/
    RK_U8               area_map_en;    /**< enable area map */
    RK_U8               abs_qp_en;      /**< absolute qp enable flag*/
} MppEncROIRegion;

/**
 * @brief MPP encoder's ROI configuration
 */
typedef struct MppEncROICfg_t {
    RK_U32              number;         /**< ROI rectangle number */
    MppEncROIRegion     *regions;       /**< ROI parameters */
} MppEncROICfg;

typedef struct MppEncROICfg0_t {
    RK_U32              change;         /**< change flag */
    RK_U32              number;         /**< ROI rectangle number */
    MppEncROIRegion     regions[8];     /**< ROI parameters */
} MppEncROICfgLegacy;

/**
 * @brief Mpp ROI parameter for vepu54x / vepu58x
 * @note  These encoders have more complex roi configure structure.
 *        User need to generate roi structure data for different soc.
 *        And send buffers to encoder through metadata.
 */
typedef struct MppEncROICfg2_t {
    MppBuffer          base_cfg_buf;
    MppBuffer          qp_cfg_buf;
    MppBuffer          amv_cfg_buf;
    MppBuffer          mv_cfg_buf;

    RK_U32             roi_qp_en    : 1;
    RK_U32             roi_amv_en   : 1;
    RK_U32             roi_mv_en    : 1;
    RK_U32             reserve_bits : 29;
    RK_U32             reserve[3];
} MppEncROICfg2;

typedef struct MppJpegROIRegion_t {
    RK_U16              x;              /* horizontal position of top left corner */
    RK_U16              y;              /* vertical position of top left corner */
    RK_U16              w;              /* width of ROI rectangle */
    RK_U16              h;              /* height of ROI rectangle */
    RK_U8               level;          /* the strength of erasing residuals for roi */
    RK_U8               roi_en;         /* enable roi */
} MppJpegROIRegion;

typedef struct MppJpegROICfg_t {
    RK_U32              change;
    RK_U16              non_roi_level;  /* the strength of erasing residuals for non-roi */
    RK_U16              non_roi_en;     /* enable non-roi */
    MppJpegROIRegion    regions[16];
} MppJpegROICfg;

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
#define MPP_ENC_OSD_PLT_WHITE           ((255<<24)|(128<<16)|(128<<8)|235)
#define MPP_ENC_OSD_PLT_YELLOW          ((255<<24)|(146<<16)|( 16<<8)|210)
#define MPP_ENC_OSD_PLT_CYAN            ((255<<24)|( 16<<16)|(166<<8)|170)
#define MPP_ENC_OSD_PLT_GREEN           ((255<<24)|( 34<<16)|( 54<<8)|145)
#define MPP_ENC_OSD_PLT_TRANS           ((  0<<24)|(222<<16)|(202<<8)|106)
#define MPP_ENC_OSD_PLT_RED             ((255<<24)|(240<<16)|( 90<<8)| 81)
#define MPP_ENC_OSD_PLT_BLUE            ((255<<24)|(110<<16)|(240<<8)| 41)
#define MPP_ENC_OSD_PLT_BLACK           ((255<<24)|(128<<16)|(128<<8)| 16)

typedef enum MppEncOSDPltType_e {
    MPP_ENC_OSD_PLT_TYPE_DEFAULT,
    MPP_ENC_OSD_PLT_TYPE_USERDEF,
    MPP_ENC_OSD_PLT_TYPE_BUTT,
} MppEncOSDPltType;

/* OSD palette value define */
typedef union MppEncOSDPltVal_u {
    struct {
        RK_U32          v       : 8;
        RK_U32          u       : 8;
        RK_U32          y       : 8;
        RK_U32          alpha   : 8;
    };
    RK_U32              val;
} MppEncOSDPltVal;

typedef struct MppEncOSDPlt_t {
    MppEncOSDPltVal     data[256];
} MppEncOSDPlt;

typedef enum MppEncOSDPltCfgChange_e {
    MPP_ENC_OSD_PLT_CFG_CHANGE_MODE     = (1 << 0),     /* change osd plt type */
    MPP_ENC_OSD_PLT_CFG_CHANGE_PLT_VAL  = (1 << 1),     /* change osd plt table value */
    MPP_ENC_OSD_PLT_CFG_CHANGE_ALL      = (0xFFFFFFFF),
} MppEncOSDPltCfgChange;

typedef struct MppEncOSDPltCfg_t {
    RK_U32              change;
    MppEncOSDPltType    type;
    MppEncOSDPlt        *plt;
} MppEncOSDPltCfg;

/* position info is unit in 16 pixels(one MB), and
 * x-directon range in pixels = (rd_pos_x - lt_pos_x + 1) * 16;
 * y-directon range in pixels = (rd_pos_y - lt_pos_y + 1) * 16;
 */
typedef struct MppEncOSDRegion_t {
    RK_U32              enable;
    RK_U32              inverse;
    RK_U32              start_mb_x;
    RK_U32              start_mb_y;
    RK_U32              num_mb_x;
    RK_U32              num_mb_y;
    RK_U32              buf_offset;
} MppEncOSDRegion;

/* if num_region > 0 && region==NULL
 * use old osd data
 */
typedef struct MppEncOSDData_t {
    MppBuffer           buf;
    RK_U32              num_region;
    MppEncOSDRegion     region[8];
} MppEncOSDData;

typedef struct MppEncOSDRegion2_t {
    RK_U32              enable;
    RK_U32              inverse;
    RK_U32              start_mb_x;
    RK_U32              start_mb_y;
    RK_U32              num_mb_x;
    RK_U32              num_mb_y;
    RK_U32              buf_offset;
    MppBuffer           buf;
} MppEncOSDRegion2;

typedef struct MppEncOSDData2_t {
    RK_U32              num_region;
    MppEncOSDRegion2    region[8];
} MppEncOSDData2;

/* kmpp osd configure */
typedef struct EncOSDInvCfg_t {
    RK_U32              yg_inv_en;
    RK_U32              uvrb_inv_en;
    RK_U32              alpha_inv_en;
    RK_U32              inv_sel;
    RK_U32              uv_sw_inv_en;
    RK_U32              inv_size;
    RK_U32              inv_stride;
    KmppShmPtr          inv_buf;
} EncOSDInvCfg;

typedef struct EncOSDAlphaCfg_t {
    RK_U32              alpha_swap;
    RK_U32              bg_alpha;
    RK_U32              fg_alpha;
    RK_U32              fg_alpha_sel;
} EncOSDAlphaCfg;

typedef struct EncOSDQpCfg_t {
    RK_U32              qp_adj_en;
    RK_U32              qp_adj_sel;
    RK_S32              qp;
    RK_U32              qp_max;
    RK_U32              qp_min;
    RK_U32              qp_prj;
} EncOSDQpCfg;

typedef struct MppEncOSDRegion3_t {
    RK_U32              enable;
    RK_U32              range_trns_en;
    RK_U32              range_trns_sel;
    RK_U32              fmt;
    RK_U32              rbuv_swap;
    RK_U32              lt_x;
    RK_U32              lt_y;
    RK_U32              rb_x;
    RK_U32              rb_y;
    RK_U32              stride;
    RK_U32              ch_ds_mode;
    RK_U32              osd_endn;
    EncOSDInvCfg        inv_cfg;
    EncOSDAlphaCfg      alpha_cfg;
    EncOSDQpCfg         qp_cfg;
    KmppShmPtr          osd_buf;
    RK_U8               lut[8];  //vuy vuy alpha
} MppEncOSDRegion3;

typedef struct MppEncOSDData3_t {
    RK_U32              change;
    RK_U32              num_region;
    MppEncOSDRegion3    region[8];
} MppEncOSDData3;
/* kmpp osd configure end */

typedef struct MppEncUserData_t {
    RK_U32              len;
    void                *pdata;
} MppEncUserData;

typedef struct MppEncUserDataFull_t {
    RK_U32              len;
    RK_U8               *uuid;
    void                *pdata;
} MppEncUserDataFull;

typedef struct MppEncUserDataSet_t {
    RK_U32              count;
    MppEncUserDataFull  *datas;
} MppEncUserDataSet;

typedef enum MppEncSceneMode_e {
    MPP_ENC_SCENE_MODE_DEFAULT,
    MPP_ENC_SCENE_MODE_IPC,
    MPP_ENC_SCENE_MODE_IPC_PTZ,
    MPP_ENC_SCENE_MODE_BUTT,
} MppEncSceneMode;

typedef struct MppEncFineTuneCfg_t {
    MppEncSceneMode     scene_mode;
    MppEncSeMode        se_mode;
    RK_S32              deblur_en; /* qpmap_en */
    RK_S32              deblur_str; /* deblur strength */
    RK_S32              anti_flicker_str;
    RK_S32              lambda_idx_i;
    RK_S32              lambda_idx_p;
    RK_S32              atr_str_i; /* line_en */
    RK_S32              atr_str_p; /* line_en */
    RK_S32              atl_str; /* anti_stripe */
    RK_S32              sao_str_i; /* anti blur */
    RK_S32              sao_str_p; /* anti blur */
    RK_S32              rc_container;
    RK_S32              vmaf_opt;

    RK_S32              motion_static_switch_enable;
    RK_S32              atf_str;
    /* vepu500 only */
    RK_S32              lgt_chg_lvl; /* light change level, [0, 3] */
    RK_S32              static_frm_num; /* static frame number, [0, 7] */
    RK_S32              madp16_th; /* madp threshold for static block detection, [0, 63] */
    RK_S32              skip16_wgt; /* weight for skip16, 0 or [3, 8] */
    RK_S32              skip32_wgt; /* weight for skip32, 0 or [3, 8] */
    RK_S32              qpmap_en;
    RK_S32              speed; /* encoder speed [0..3], 0:normal; 1:fast; 2:faster; 3:fastest */

    /* smart v3 only */
    RK_S32              bg_delta_qp_i; /* background delta qp for i frame */
    RK_S32              bg_delta_qp_p; /* background delta qp for p frame */
    RK_S32              fg_delta_qp_i; /* foreground delta qp for i frame */
    RK_S32              fg_delta_qp_p; /* foreground delta qp for p frame */
    RK_S32              bmap_qpmin_i; /* min qp for i frame in bmap */
    RK_S32              bmap_qpmin_p; /* min qp for p frame in bmap */
    RK_S32              bmap_qpmax_i; /* max qp for i frame in bmap */
    RK_S32              bmap_qpmax_p; /* max qp for p frame in bmap */
    RK_S32              min_bg_fqp; /* min frame qp for background region */
    RK_S32              max_bg_fqp; /* max frame qp for background region */
    RK_S32              min_fg_fqp; /* min frame qp for foreground region */
    RK_S32              max_fg_fqp; /* max frame qp for foreground region */
    RK_S32              fg_area; /* foreground area, [-1, 100] */
} MppEncFineTuneCfg;

#endif /*__RK_VENC_CMD_H__*/
