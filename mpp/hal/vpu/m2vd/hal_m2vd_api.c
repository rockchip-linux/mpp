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
    "m2vd_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingMPEG2,
    sizeof(M2VDHalContext),
    0,
    hal_m2vd_init,
    hal_m2vd_deinit,
    hal_m2vd_gen_regs,
    hal_m2vd_start,
    hal_m2vd_wait,
    hal_m2vd_reset,
    hal_m2vd_flush,
    hal_m2vd_control,
};





