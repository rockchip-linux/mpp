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

#include "iep2_gmv.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_buffer.h"

#include "iep2_api.h"

void iep2_sort(uint32_t bin[], uint32_t map[], int size)
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

        for (n = m + 1; n < size; ++n)
            if (dat[n] > dat[max])
                max = n;
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

static int iep2_osd_check(int8_t *mv, int w, int sx, int ex, int sy, int ey,
                          int *mvx)
{
    /* (28 + 27) * 4 + 1 */
    uint32_t hist[221];
    uint32_t map[221];
    int total = (ey - sy + 1) * (ex - sx + 1);
    int non_zero = 0;
    int domin = 0;
    int i, j;

    memset(hist, 0, sizeof(hist));

    for (i = sy; i <= ey; ++i) {
        for (j = sx; j <= ex; ++j) {
            int8_t v = mv[i * w + j];
            uint32_t idx = v + 28 * 4;

            if (idx >= MPP_ARRAY_ELEMS(hist)) {
                mpp_log("invalid mv at (%d, %d)\n", j, i);
                continue;
            }
            hist[idx]++;
        }
    }

    non_zero = total - hist[28 * 4];

    iep2_sort(hist, map, MPP_ARRAY_ELEMS(hist));

    domin = hist[map[0]];
    if (map[0] + 1 < MPP_ARRAY_ELEMS(hist))
        domin += hist[map[0] + 1];
    if (map[0] >= 1)
        domin += hist[map[0] - 1];

    iep_dbg_trace("total tiles in current osd: %d, non-zero %d\n",
                  total, non_zero);

    if (domin * 4 < non_zero * 3) {
        iep_dbg_trace("main mv %d count %d not dominant\n",
                      map[0] - 28 * 4, domin);
        return 0;
    }

    *mvx = map[0] - 28 * 4;

    return 1;
}

void iep2_set_osd(struct iep2_api_ctx *ctx, struct mv_list *ls)
{
    uint32_t i, j;
    int idx = 0;

    int sx[8];
    int ex[8];
    int sy[8];
    int ey[8];

    uint32_t osd_tile_cnt = 0;
    int mvx;
    int8_t *pmv = mpp_buffer_get_ptr(ctx->mv_buf);

    memset(ls, 0, sizeof(*ls));

    for (i = 0; i < ctx->output.dect_osd_cnt; ++i) {
        sx[i] = ctx->output.x_sta[i];
        ex[i] = ctx->output.x_end[i];
        sy[i] = ctx->output.y_sta[i];
        ey[i] = ctx->output.y_end[i];
    }

    /* Hardware isn't supporting subtitle regions overlap. */
    for (i = 0; i < ctx->output.dect_osd_cnt; ++i) {
        for (j = i + 1; j < ctx->output.dect_osd_cnt; ++j) {
            if (sy[j] == ey[i]) {
                if (ex[i] - sx[i] > ex[j] - sx[j]) {
                    sy[j]++;
                } else {
                    ey[i]--;
                }
            } else {
                break;
            }
        }
    }

    for (i = 0; i < ctx->output.dect_osd_cnt; ++i) {
        if (!iep2_osd_check(pmv, ctx->params.tile_cols,
                            sx[i], ex[i], sy[i], ey[i], &mvx))
            continue;

        ctx->params.osd_x_sta[idx] = sx[i];
        ctx->params.osd_x_end[idx] = ex[i];
        ctx->params.osd_y_sta[idx] = sy[i];
        ctx->params.osd_y_end[idx] = ey[i];

        osd_tile_cnt += (ex[i] - sx[i] + 1) * (ey[i] - sy[i] + 1);

        ls->mv[idx] = mvx;
        ls->vld[idx] = 1;

        iep_dbg_trace("[%d] from [%d,%d][%d,%d] to [%d,%d][%d,%d] mv %d\n", i,
                      sx[i], ex[i], sy[i], ey[i],
                      ctx->params.osd_x_sta[idx], ctx->params.osd_x_end[idx],
                      ctx->params.osd_y_sta[idx], ctx->params.osd_y_end[idx],
                      ls->mv[idx]);
        idx++;
    }

    ctx->params.osd_area_num = idx;
    ls->idx = idx;

    iep_dbg_trace("osd tile count %d comb %d\n",
                  osd_tile_cnt, ctx->output.out_osd_comb_cnt);
    if (osd_tile_cnt * 2 > ctx->output.out_osd_comb_cnt * 3) {
        memset(ctx->params.comb_osd_vld, 0, sizeof(ctx->params.comb_osd_vld));
    } else {
        memset(ctx->params.comb_osd_vld, 1, sizeof(ctx->params.comb_osd_vld));
    }
}

