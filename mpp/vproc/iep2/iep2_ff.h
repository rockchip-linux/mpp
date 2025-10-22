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

#ifndef IEP2_FF_H
#define IEP2_FF_H

#include "rk_type.h"

#define FIELD_ORDER_RATIO_SIZE (10)
struct iep2_ff_info {
    RK_S32 tff_offset;
    RK_S32 bff_offset;
    RK_S32 tff_score;
    RK_S32 bff_score;
    RK_S32 frm_offset;
    RK_S32 fie_offset;
    RK_S32 frm_score;
    RK_S32 fie_score;
    RK_U32 field_order;
    RK_U32 frm_mode;
    RK_U32 fo_ratio[FIELD_ORDER_RATIO_SIZE];
    RK_U32 fo_ratio_idx;
    RK_U32 fo_ratio_cnt;
    RK_U32 fo_ratio_sum;
    RK_U32 fo_ratio_avg;
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
