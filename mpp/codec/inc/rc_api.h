/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __RC_API_H__
#define __RC_API_H__

#include "mpp_enc_cfg.h"

/*
 * Mpp rate control has three parts:
 *
 * 1. MPI user config module
 *    MppEncRcCfg structure is provided to user for overall rate control config
 *    Mpp will receive MppEncRcCfg from user, check parameter and set it to
 *    encoder.
 *
 * 2. Encoder rate control module
 *    Encoder will implement the rate control strategy required by users
 *    including CBR, VBR, AVBR and so on.
 *    This module only implement the target bit calculation behavior and
 *    quality restriction. And the quality level will be controlled by hal.
 *
 * 3. Hal rate control module
 *    Hal will implement the rate control on hardware. Hal will calculate the
 *    QP parameter for hardware according to the frame level target bit
 *    specified by the encoder. And the report the real bitrate and quality to
 *    encoder.
 *
 * The header defines the communication interfaces and structures used between
 * MPI, encoder and hal.
 */

typedef enum RcMode_e {
    RC_CBR,
    RC_VBR,
    RC_AVBR,
    RC_CVBR,
    RC_QVBR,
    RC_FIXQP,
    RC_LEARNING,
    RC_MODE_BUTT,
} RcMode;

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
typedef struct RcFpsCfg_t {
    RK_S32      fps_in_flex;
    RK_S32      fps_in_num;
    RK_S32      fps_in_denorm;
    RK_S32      fps_out_flex;
    RK_S32      fps_out_num;
    RK_S32      fps_out_denorm;
} RcFpsCfg;

/*
 * Control parameter from external config
 *
 * It will be updated on rc/prep/gopref config changed.
 */
typedef struct RcCfg_s {
    /* Use rc_mode to find different api */
    RcMode      mode;

    RcFpsCfg    fps;

    /* I frame gop len */
    RK_S32      igop;
    /* visual gop len */
    RK_S32      vgop;

    /* bitrate parameter */
    RK_S32      bps_min;
    RK_S32      bps_target;
    RK_S32      bps_max;
    RK_S32      stat_times;

    /* max I frame bit ratio to P frame bit */
    RK_S32      max_i_bit_prop;
    RK_S32      min_i_bit_prop;
    /* layer bitrate proportion */
    RK_S32      layer_bit_prop[4];

    /* quality parameter */
    RK_S32      max_quality;
    RK_S32      min_quality;
    RK_S32      max_i_quality;
    RK_S32      min_i_quality;
    RK_S32      i_quality_delta;
    /* layer quality proportion */
    RK_S32      layer_quality_delta[4];

    /* reencode parameter */
    RK_S32      max_reencode_times;

    /* still / motion desision parameter */
    RK_S32      min_still_prop;
    RK_S32      max_still_quality;

    /*
     * vbr parameter
     *
     * vbr_hi_prop  - high proportion bitrate for reduce quality
     * vbr_lo_prop  - low proportion bitrate for increase quality
     */
    RK_S32      vbr_hi_prop;
    RK_S32      vbr_lo_prop;
} RcCfg;

/*
 * Bit and quality of each frame for each frame encoding
 *
 * Quality is not equal to qp.
 * Different codec and hardware will translate it to different meaning.
 */
typedef struct RcHalCfg_s {
    RK_S32      seq_idx;

    RK_S32      bit_target;
    RK_S32      bit_max;
    RK_S32      bit_min;
    RK_S32      bit_real;

    RK_S32      quality_target;
    RK_S32      quality_max;
    RK_S32      quality_min;
    RK_S32      quality_real;        /*scale 64 */

    RK_S32      inter_lv8_prop;     /*scale 256 */
    RK_S32      intra_lv4_prop;     /*scale 256 */
    RK_S32      sse;
    RK_S32      madi;
    RK_S32      madp;               /*scale 256 */

    RK_S32      next_i_ratio;
    RK_S32      next_ratio;
    RK_S32      need_reenc;
} RcHalCfg;

/*
 * Different rate control strategy like CBR/VBR/AVBR/QVBR/CVBR will be
 * implemented by different api structure
 */
typedef struct RcImplApi_t {
    char            *name;
    MppCodingType   type;
    RK_U32          ctx_size;

    MPP_RET         (*init)(void *ctx, RcCfg *cfg);
    MPP_RET         (*deinit)(void *ctx);

    /*
     * start - frame level rate control start.
     *         The RcHalCfg will be output to hal for hardware to implement.
     * end   - frame level rate control end
     *         The RcHalCfg is returned for real quality and bitrate.
     */
    MPP_RET         (*start)(void *ctx, RcHalCfg *cfg, EncFrmStatus *frm);
    MPP_RET         (*end)(void *ctx, RcHalCfg *cfg);
} RcImplApi;

#endif /* __RC_API_H__ */
