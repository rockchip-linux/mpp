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

#define MODULE_TAG "h264e_rc"

#include <math.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "rc_debug.h"
#include "rc_ctx.h"
#include "rc_model_v2.h"

const RcImplApi default_h264e = {
    "default",
    MPP_VIDEO_CodingAVC,
    sizeof(RcModelV2Ctx),
    rc_model_v2_init,
    rc_model_v2_deinit,
    NULL,
    rc_model_v2_check_reenc,
    rc_model_v2_start,
    rc_model_v2_end,
    rc_model_v2_hal_start,
    rc_model_v2_hal_end,
};
