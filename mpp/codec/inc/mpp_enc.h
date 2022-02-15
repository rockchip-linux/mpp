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

#ifndef __MPP_ENC_H__
#define __MPP_ENC_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "rk_mpi_cmd.h"

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
 *     User        |      Mpi/Mpp         |         EncImpl
 *                 |                      |            Hal
 *                 |                      |
 * +----------+    |    +---------+       |       +------------+
 * |          |    |    |         +-----RcCfg----->            |
 * |  RcCfg   +--------->         |       |       | EncImpl |
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

typedef void* MppEnc;

typedef struct MppEncInitCfg_t {
    MppCodingType       coding;
    RK_S32              task_cnt;
    void                *mpp;
} MppEncInitCfg;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_enc_init_v2(MppEnc *ctx, MppEncInitCfg *cfg);
MPP_RET mpp_enc_deinit_v2(MppEnc ctx);

MPP_RET mpp_enc_start_v2(MppEnc ctx);
MPP_RET mpp_enc_start_async(MppEnc ctx);
MPP_RET mpp_enc_stop_v2(MppEnc ctx);

MPP_RET mpp_enc_control_v2(MppEnc ctx, MpiCmd cmd, void *param);
MPP_RET mpp_enc_notify_v2(MppEnc ctx, RK_U32 flag);
MPP_RET mpp_enc_reset_v2(MppEnc ctx);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_H__*/
