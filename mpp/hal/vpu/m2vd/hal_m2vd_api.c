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

#define MODULE_TAG "hal_m2vd_api"

#include <string.h>

#include "mpp_hal.h"
#include "hal_m2vd_reg.h"

const MppHalApi hal_api_m2vd = {
    .name = "m2vd_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingMPEG2,
    .ctx_size = sizeof(M2VDHalContext),
    .flag = 0,
    .init = hal_m2vd_init,
    .deinit = hal_m2vd_deinit,
    .reg_gen = hal_m2vd_gen_regs,
    .start = hal_m2vd_start,
    .wait = hal_m2vd_wait,
    .reset = hal_m2vd_reset,
    .flush = hal_m2vd_flush,
    .control = hal_m2vd_control,
};

