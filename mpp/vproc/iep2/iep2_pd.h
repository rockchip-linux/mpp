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

#ifndef __IEP2_PD_H__
#define __IEP2_PD_H__

struct iep2_pd_info {
    int temporal[5];
    int spatial[5];
    int fcoeff[5];
    int i;
    int pdtype;
    int step;
};

enum PD_COMP_FLAG {
    PD_COMP_FLAG_CC,
    PD_COMP_FLAG_CN,
    PD_COMP_FLAG_NC,
    PD_COMP_FLAG_NON
};

enum PD_TYPES {
    PD_TYPES_3_2_3_2,
    PD_TYPES_2_3_2_3,
    PD_TYPES_2_3_3_2,
    PD_TYPES_3_2_2_3,
    PD_TYPES_UNKNOWN
};

struct iep2_api_ctx;

void iep2_check_pd(struct iep2_api_ctx *ctx);
int iep2_pd_get_output(struct iep2_pd_info *pd_inf);

#endif
