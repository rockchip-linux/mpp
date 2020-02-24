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

#include "iep2_roi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iep2_api.h"

void iep2_set_roi(struct iep2_api_ctx *ctx, struct iep2_rect *r,
                  enum ROI_MODE mode)
{
    int idx = ctx->params.roi_layer_num;

    ctx->params.roi_en = 1;

    ctx->params.xsta[idx] = r->x;
    ctx->params.ysta[idx] = r->y;
    ctx->params.xend[idx] = r->x + r->w - 1;
    ctx->params.yend[idx] = r->y + r->h - 1;
    ctx->params.roi_mode[idx] = mode;

    ctx->params.roi_layer_num++;
}
