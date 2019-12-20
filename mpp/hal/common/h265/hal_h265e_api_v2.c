/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_h265e_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_platform.h"

#include "hal_h265e_api_v2.h"
#include "hal_h265e_rkv_ctx.h"
#include "hal_h265e_rkv.h"

extern RK_U32 hal_h265e_debug;

const MppEncHalApi hal_api_h265e_v2 = {
    "h265e_rkv_v2",
    MPP_VIDEO_CodingHEVC,
    sizeof(H265eRkvHalContext),
    0,
    hal_h265e_rkv_init,
    hal_h265e_rkv_deinit,
    hal_h265e_rkv_get_task,
    hal_h265e_rkv_gen_regs,
    hal_h265e_rkv_start,
    hal_h265e_rkv_wait,
    hal_h265e_rkv_ret_task,
};
