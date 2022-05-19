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

#define MODULE_TAG "iep2"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpp_common.h"

#include "iep2_api.h"
#include "iep2_gmv.h"

static void iep2_sort(uint32_t bin[], int map[], int size)
{
    int i, m, n;
    uint32_t *dat = malloc(size * sizeof(uint32_t));

    for (i = 0; i < size; ++i) {
        map[i] = i;
        dat[i] = bin[i];
    }

    for (m = 0; m < size; ++m) {
        int max = m;
        uint32_t temp;
        int p;

        for (n = m + 1; n < size; ++n) if (dat[n] > dat[max]) max = n;
        // Put found minimum element in its place
        temp = dat[m];
        p = map[m];

        map[m] = map[max];
        map[max] = p;
        dat[m] = dat[max];
        dat[max] = temp;
    }

    free(dat);
}

static int iep2_is_subt_mv(int mv, struct mv_list *mv_ls)
{
    int i;

    for (i = 0; i < mv_ls->idx; ++i) {
        if (RKABS(mv_ls->mv[i] - (mv * 4)) < 3)
            return 1;
    }

    return 0;
}

void iep2_update_gmv(struct iep2_api_ctx *ctx, struct mv_list *mv_ls)
{
    int rows = ctx->params.tile_rows;
    int cols = ctx->params.tile_cols;
    uint32_t *bin = ctx->output.mv_hist;
    int lbin = MPP_ARRAY_ELEMS(ctx->output.mv_hist);
    int i;

    int map[MPP_ARRAY_ELEMS(ctx->output.mv_hist)];

    uint32_t r = 6;

    // print mvc histogram of current motion estimation.
    for (i = 0; i < lbin; ++i) {
        if (bin[i] == 0)
            continue;
        iep_dbg_trace("mv(%d) %d\n", i - MVL, bin[i]);
    }

    bin[MVL] = 0; // disable 0 mv

    // update motion vector candidates
    iep2_sort(bin, map, lbin);

    iep_dbg_trace("sort map:\n");
    if (0) {
        for (i = 0; i < lbin; ++i) {
            fprintf(stderr, "%d ", map[i]);
        }
        fprintf(stderr, "\n");
    }

    memset(ctx->params.mv_tru_list, 0, sizeof(ctx->params.mv_tru_list));
    memset(ctx->params.mv_tru_vld, 0, sizeof(ctx->params.mv_tru_vld));

    // Get top 8 candidates of current motion estimation.
    for (i = 0; i < 8; ++i) {
        int8_t x = map[i] - MVL;

        if (bin[map[i]] > r * ((rows * cols) >> 7) ||
            iep2_is_subt_mv(x, mv_ls)) {

            // 1 bit at low endian for mv valid check
            ctx->params.mv_tru_list[i] = x;
            ctx->params.mv_tru_vld[i] = 1;
        } else {
            if (i == 0) {
                ctx->params.mv_tru_list[0] = 0;
                ctx->params.mv_tru_vld[0] = 1;
            }
            break;
        }
    }

    for (i = 0; i < 8; ++i)
        iep_dbg_trace("new mv candidates list[%d] (%d,%d)\n",
                      i, ctx->params.mv_tru_list[i], 0);
}

