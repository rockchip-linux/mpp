/*
*
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
#define MODULE_TAG "hal_vp8d_api"

#include <string.h>

#include "mpp_hal.h"
#include "hal_vp8d_reg.h"

const MppHalApi hal_api_vp8d = {
    "vp8d_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingVP8,
    sizeof(VP8DHalContext_t),
    0,
    hal_vp8d_init,
    hal_vp8d_deinit,
    hal_vp8d_gen_regs,
    hal_vp8d_start,
    hal_vp8d_wait,
    hal_vp8d_reset,
    hal_vp8d_flush,
    hal_vp8d_control,
};





