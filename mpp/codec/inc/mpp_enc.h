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

#include "rk_mpi.h"
#include "mpp_controller.h"
#include "mpp_hal.h"

typedef struct MppEnc_t MppEnc;

struct MppEnc_t {
    MppCodingType       coding;
    Controller          controller;
    MppHal              hal;

    // common resource
    MppBufSlots         frame_slots;
    MppBufSlots         packet_slots;
    HalTaskGroup        tasks;

    RK_U32              reset_flag;
    void                *mpp;

    H264EncConfig encCfg;
    h264e_control_extra_info_cfg extra_info_cfg;
    h264e_control_extra_info extra_info;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * main thread for all encoder. This thread will connect encoder / hal / mpp
 */
void *mpp_enc_control_thread(void *data);
void *mpp_enc_hal_thread(void *data);

MPP_RET mpp_enc_init(MppEnc *enc, MppCodingType coding);
MPP_RET mpp_enc_deinit(MppEnc *enc);
MPP_RET mpp_enc_config(MppEnc *enc, MpiCmd cmd, void *param);
MPP_RET mpp_enc_notify(void *ctx, void *info);
MPP_RET mpp_enc_reset(MppEnc *enc);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_H__*/
