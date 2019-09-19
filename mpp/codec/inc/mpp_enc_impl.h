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

#ifndef __MPP_ENC_IMPL_H__
#define __MPP_ENC_IMPL_H__

#include "mpp_thread.h"
#include "mpp_controller.h"
#include "mpp_hal.h"

typedef struct MppEncImpl_t {
    MppCodingType       coding;
    Controller          controller;
    MppHal              hal;
    void                *mpp;

    // common resource
    MppBufSlots         frame_slots;
    MppBufSlots         packet_slots;
    HalTaskGroup        tasks;

    // internal status and protection
    Mutex               lock;
    RK_U32              reset_flag;
    sem_t               enc_reset;

    RK_U32              wait_count;
    RK_U32              work_count;
    RK_U32              status_flag;
    RK_U32              notify_flag;

    /* Encoder configure set */
    MppEncCfgSet        cfg;
    MppEncCfgSet        set;
} MppEncImpl;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_IMPL_H__*/
