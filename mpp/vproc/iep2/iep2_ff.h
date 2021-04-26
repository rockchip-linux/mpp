/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __IEP2_FF_H__
#define __IEP2_FF_H__

#include "rk_type.h"

struct iep2_ff_info {
    RK_S32 tff_offset;
    RK_S32 bff_offset;
    RK_S32 tff_score;
    RK_S32 bff_score;
    RK_S32 frm_offset;
    RK_S32 fie_offset;
    RK_S32 frm_score;
    RK_S32 fie_score;
    RK_S32 fo_detected;
    RK_S32 is_frm;
};

struct iep2_api_ctx;

#ifdef __cplusplus
extern "C" {
#endif

void iep2_check_ffo(struct iep2_api_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif
