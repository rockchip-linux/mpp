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

#include "iep2_ff.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iep2.h"

#define RKABS(a)                (((a) >= 0) ? (a) : -(a))
#define RKMIN(a, b)             (((a) < (b)) ? (a) : (b))
#define RKMAX(a, b)             (((a) > (b)) ? (a) : (b))

void iep2_check_ffo(struct iep2_api_ctx *ctx)
{
    uint32_t tdiff = ctx->output.ff_gradt_tcnt + 1;
    uint32_t bdiff = ctx->output.ff_gradt_bcnt + 1;
    uint32_t ff00t  = (ctx->output.dect_ff_cur_tcnt << 5) / tdiff;
    uint32_t ff00b  = (ctx->output.dect_ff_cur_bcnt << 5) / bdiff;
    uint32_t ff11t  = (ctx->output.dect_ff_nxt_tcnt << 5) / tdiff;
    uint32_t ff11b  = (ctx->output.dect_ff_nxt_bcnt << 5) / bdiff;
    int32_t ff0t1b = (ctx->output.dect_ff_ble_tcnt << 5) / bdiff;
    int32_t ff0b1t = (ctx->output.dect_ff_ble_bcnt << 5) / bdiff;

    uint32_t ff00 = RKMIN(ff00t, ff00b);
    uint32_t ff11 = RKMIN(ff11t, ff11b);
    uint32_t ffx = RKMIN(ff0t1b, ff0b1t);
    uint32_t ffi = RKMAX(ff00, ff11);
    uint32_t thr = ffx / 10;

    if (ff00t > 100 || ff00b > 100)
        return;

    if (RKABS(ff0t1b - ff0b1t) > thr) {
        if (ff0t1b > ff0b1t) {
            ctx->ff_inf.tff_score = RKCLIP(ctx->ff_inf.tff_score + 1, 0, 10);
            ctx->ff_inf.bff_score = RKCLIP(ctx->ff_inf.bff_score - 1, 0, 10);
        } else {
            ctx->ff_inf.tff_score = RKCLIP(ctx->ff_inf.tff_score - 1, 0, 10);
            ctx->ff_inf.bff_score = RKCLIP(ctx->ff_inf.bff_score + 1, 0, 10);
        }
    }

    if (RKABS(ctx->ff_inf.tff_score - ctx->ff_inf.bff_score) > 5) {
        if (ctx->ff_inf.tff_score > ctx->ff_inf.bff_score) {
            ctx->params.dil_field_order = 0;
        } else {
            ctx->params.dil_field_order = 1;
        }
    }

    if (ffi * 2 < ffx) {
        ctx->ff_inf.frm_score = RKCLIP(ctx->ff_inf.frm_score + 1, 0, 10);
        ctx->ff_inf.fie_score = RKCLIP(ctx->ff_inf.fie_score - 1, 0, 10);
    } else {
        ctx->ff_inf.frm_score = RKCLIP(ctx->ff_inf.frm_score - 1, 0, 10);
        ctx->ff_inf.fie_score = RKCLIP(ctx->ff_inf.fie_score + 1, 0, 10);
    }

    if (RKABS(ctx->ff_inf.frm_score - ctx->ff_inf.fie_score) > 5) {
        if (ctx->ff_inf.frm_score > ctx->ff_inf.fie_score) {
            ;
        } else {
            ;
        }
    }
}

