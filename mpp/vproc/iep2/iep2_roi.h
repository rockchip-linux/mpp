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

#ifndef __IEP2_ROI_H__
#define __IEP2_ROI_H__

#include "iep2.h"

struct iep2_api_ctx;

struct iep2_rect {
    int x;
    int y;
    int w;
    int h;
};

enum ROI_MODE {
    ROI_MODE_NORMAL,
    ROI_MODE_BYPASS,
    ROI_MODE_SPATIAL,
    ROI_MODE_MA,
    ROI_MODE_MA_MC,
    ROI_MODE_MC_SPATIAL,
    ROI_MODE_RESERVED0,
    ROI_MODE_RESERVED1,
    ROI_MODE_RESERVED2,
    ROI_MODE_SPEC_MC
};

void iep2_set_roi(struct iep2_api_ctx *ctx, struct iep2_rect *r,
                  enum ROI_MODE mode);

#endif
