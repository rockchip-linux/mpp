/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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
#include "av1d_parser.h"
#include "av1d_common.h"

void Av1GetCDFs(AV1Context *ctx, RK_U32 ref_idx)
{

    ctx->cdfs = &ctx->cdfs_last[ref_idx];
    ctx->cdfs_ndvc = &ctx->cdfs_last_ndvc[ref_idx];
}

void Av1StoreCDFs(AV1Context *ctx, RK_U32 refresh_frame_flags)
{
    RK_U32 i;

    for (i = 0; i < NUM_REF_FRAMES; i++) {
        if (refresh_frame_flags & (1 << i)) {
            if (&ctx->cdfs_last[i] != ctx->cdfs) {
                ctx->cdfs_last[i] = *ctx->cdfs;
                ctx->cdfs_last_ndvc[i] = *ctx->cdfs_ndvc;
            }
        }
    }
}
