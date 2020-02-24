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

struct iep2_ff_info {
    int tff_score;
    int bff_score;
    int frm_score;
    int fie_score;
};

struct iep2_api_ctx;

void iep2_check_ffo(struct iep2_api_ctx *ctx);

#endif
