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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpp_log.h"
#include "mpp_common.h"

#include "iep2.h"
#include "iep2_api.h"
#include "iep2_ff.h"

#define RKABS(a)                (((a) >= 0) ? (a) : -(a))
#define RKMIN(a, b)             (((a) < (b)) ? (a) : (b))
#define RKMAX(a, b)             (((a) > (b)) ? (a) : (b))

void iep2_check_ffo(struct iep2_api_ctx *ctx)
{
    RK_U32 tdiff = ctx->output.ff_gradt_tcnt + 1;
    RK_U32 bdiff = ctx->output.ff_gradt_bcnt + 1;
    RK_U32 ff00t  = (ctx->output.dect_ff_cur_tcnt << 5) / tdiff;
    RK_U32 ff00b  = (ctx->output.dect_ff_cur_bcnt << 5) / bdiff;
    RK_U32 ff11t  = (ctx->output.dect_ff_nxt_tcnt << 5) / tdiff;
    RK_U32 ff11b  = (ctx->output.dect_ff_nxt_bcnt << 5) / bdiff;
    RK_S32 ff0t1b = (ctx->output.dect_ff_ble_tcnt << 5) / bdiff;
    RK_S32 ff0b1t = (ctx->output.dect_ff_ble_bcnt << 5) / bdiff;

    RK_U32 ff00 = RKMIN(ff00t, ff00b);
    RK_U32 ff11 = RKMIN(ff11t, ff11b);
    RK_U32 ffx = RKMIN(ff0t1b, ff0b1t);
    RK_U32 ffi = RKMAX(ff00, ff11);
    RK_U32 thr = ffx / 10;

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

    ctx->ff_inf.tff_score += ctx->ff_inf.tff_offset;
    ctx->ff_inf.bff_score += ctx->ff_inf.bff_offset;

    if (RKABS(ctx->ff_inf.tff_score - ctx->ff_inf.bff_score) > 5) {
        if (ctx->ff_inf.tff_score > ctx->ff_inf.bff_score) {
            iep_dbg_trace("deinterlace field order tff\n");
            ctx->params.dil_field_order = 0;
        } else {
            iep_dbg_trace("deinterlace field order bff\n");
            ctx->params.dil_field_order = 1;
        }
        ctx->ff_inf.fo_detected = 1;
    } else {
        ctx->ff_inf.fo_detected = 1;
    }

    iep_dbg_trace("deinterlace ffi %u ffx %u\n", ffi, ffx);
    if (ffi > ffx * 2) {
        ctx->ff_inf.frm_score = RKCLIP(ctx->ff_inf.frm_score + 1, 0, 10);
        ctx->ff_inf.fie_score = RKCLIP(ctx->ff_inf.fie_score - 1, 0, 10);
    } else {
        ctx->ff_inf.frm_score = RKCLIP(ctx->ff_inf.frm_score - 1, 0, 10);
        ctx->ff_inf.fie_score = RKCLIP(ctx->ff_inf.fie_score + 1, 0, 10);
    }

    if (RKABS(ctx->ff_inf.frm_score - ctx->ff_inf.fie_score) > 5) {
        if (ctx->ff_inf.frm_score > ctx->ff_inf.fie_score) {
            ctx->ff_inf.is_frm = 1;
            iep_dbg_trace("deinterlace frame mode\n");
        } else {
            ctx->ff_inf.is_frm = 0;
            iep_dbg_trace("deinterlace field mode\n");
        }
    }
}
