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

#include "iep2.h"
#include "iep2_api.h"
#include "iep2_ff.h"

void iep2_check_ffo(struct iep2_api_ctx *ctx)
{
    RK_S32 tcnt = ctx->output.dect_pd_tcnt;
    RK_S32 bcnt = ctx->output.dect_pd_bcnt;
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
    RK_U32 field_diff_ratio = 0;

    iep_dbg_trace("deinterlace pd_cnt %d : %d, gradt cnt %d : %d, cur cnt %d : %d, nxt cnt %d : %d, ble 01:%d 10:%d",
                  tcnt, bcnt, tdiff, bdiff, ctx->output.ff_gradt_tcnt, ctx->output.ff_gradt_bcnt,
                  ctx->output.dect_ff_cur_tcnt, ctx->output.dect_ff_cur_bcnt,
                  ctx->output.dect_ff_nxt_tcnt, ctx->output.dect_ff_nxt_bcnt,
                  ctx->output.dect_ff_ble_tcnt, ctx->output.dect_ff_ble_bcnt);

    iep_dbg_trace("deinterlace tdiff %u, bdiff %u, ff00t %u, ff00b %u, ff11t %u ff11b %u ff0t1b %u ff0b1t %u, ff00 %d, ff11 %d, ffx %d, ffi %d thr %d\n",
                  tdiff, bdiff, ff00t, ff00b, ff11t, ff11b, ff0t1b, ff0b1t, ff00, ff11, ffx, ffi, thr);

    RK_S32 tff_score = 0;
    RK_S32 bff_score = 0;
    RK_S32 coef = 0;
    RK_S32 frm_score = 0;
    RK_S32 fie_score = 0;

    iep_dbg_trace("deinterlace cur %u, %u, nxt %u, %u, ble %u, %u, diff %u, %u, nz %u, f %u, comb %u\n",
                  ctx->output.dect_ff_cur_tcnt, ctx->output.dect_ff_cur_bcnt,
                  ctx->output.dect_ff_nxt_tcnt, ctx->output.dect_ff_nxt_bcnt,
                  ctx->output.dect_ff_ble_tcnt, ctx->output.dect_ff_ble_bcnt,
                  tdiff, bdiff,
                  ctx->output.dect_ff_nz, ctx->output.dect_ff_comb_f,
                  ctx->output.out_comb_cnt);

    if (ff00t > 120 || ff00b > 120) {
        iep_dbg_trace("deinterlace check ffo abort at %d\n", __LINE__);
        return;
    }

    iep_dbg_trace("deinterlace ffi %u ffx %u\n", ffi, ffx);

    if (ffi <= 3 && ffx <= 3) {
        iep_dbg_trace("deinterlace check ffo abort at %d\n", __LINE__);
        return;
    }

    coef = 2;

    if ((ffi * coef <= ffx) && (ffx - ffi * coef) > 2 * ffx / 10) {
        ctx->ff_inf.frm_score = RKCLIP(ctx->ff_inf.frm_score + 1, 0, 20);
        ctx->ff_inf.fie_score = RKCLIP(ctx->ff_inf.fie_score - 1, 0, 20);
    } else {
        ctx->ff_inf.frm_score = RKCLIP(ctx->ff_inf.frm_score - 1, 0, 20);
        ctx->ff_inf.fie_score = RKCLIP(ctx->ff_inf.fie_score + 1, 0, 20);
    }

    iep_dbg_trace("deinterlace (frm,fie) offset %d, %d, score %d, %d\n",
                  ctx->ff_inf.frm_offset, ctx->ff_inf.fie_offset,
                  ctx->ff_inf.frm_score, ctx->ff_inf.fie_score);

    frm_score = ctx->ff_inf.frm_score + ctx->ff_inf.frm_offset;
    fie_score = ctx->ff_inf.fie_score + ctx->ff_inf.fie_offset;


    if (RKABS(frm_score - fie_score) > 10) {
        if (frm_score > fie_score) {
            ctx->ff_inf.is_frm = 1;
            iep_dbg_trace("deinterlace frame mode\n");
        } else {
            ctx->ff_inf.is_frm = 0;
            iep_dbg_trace("deinterlace field mode\n");
        }
    }

    if (tcnt <= 3 && bcnt <= 3) {
        iep_dbg_trace("deinterlace check ffo abort at %d\n", __LINE__);
        return;
    }

    thr = (thr == 0) ? 1 : thr;

    // field order detection
    field_diff_ratio = RKABS(ff0t1b - ff0b1t) / thr * 10;
    ctx->ff_inf.fo_ratio_sum = ctx->ff_inf.fo_ratio_sum + field_diff_ratio - ctx->ff_inf.fo_ratio[ctx->ff_inf.fo_ratio_idx];
    ctx->ff_inf.fo_ratio[ctx->ff_inf.fo_ratio_idx] = field_diff_ratio;
    ctx->ff_inf.fo_ratio_idx = (ctx->ff_inf.fo_ratio_idx + 1) % FIELD_ORDER_RATIO_SIZE;

    ctx->ff_inf.fo_ratio_cnt++;
    ctx->ff_inf.fo_ratio_cnt = RKMIN(ctx->ff_inf.fo_ratio_cnt, FIELD_ORDER_RATIO_SIZE);
    ctx->ff_inf.fo_ratio_avg = ctx->ff_inf.fo_ratio_sum / ctx->ff_inf.fo_ratio_cnt;

    if (field_diff_ratio > 10) {
        if (ff0t1b > ff0b1t) {
            ctx->ff_inf.tff_score = RKCLIP(ctx->ff_inf.tff_score + 1, 0, 10);
            ctx->ff_inf.bff_score = RKCLIP(ctx->ff_inf.bff_score - 1, 0, 10);
        } else {
            ctx->ff_inf.tff_score = RKCLIP(ctx->ff_inf.tff_score - 1, 0, 10);
            ctx->ff_inf.bff_score = RKCLIP(ctx->ff_inf.bff_score + 1, 0, 10);
        }
    }

    tff_score = ctx->ff_inf.tff_score + ctx->ff_inf.tff_offset;
    bff_score = ctx->ff_inf.bff_score + ctx->ff_inf.bff_offset;
    iep_dbg_trace("deinterlace ff score %d : %d, offset %d : %d\n", tff_score, bff_score, ctx->ff_inf.tff_offset, ctx->ff_inf.bff_offset);

    if (RKABS(tff_score - bff_score) > 5) {
        if (tff_score > bff_score) {
            iep_dbg_trace("deinterlace field order tff\n");
            ctx->params.dil_field_order = IEP2_FIELD_ORDER_TFF;
        } else {
            iep_dbg_trace("deinterlace field order bff\n");
            ctx->params.dil_field_order = IEP2_FIELD_ORDER_BFF;
        }
    } else {
        iep_dbg_trace("deinterlace field order unknown, %d\n", ctx->params.dil_field_order);
    }

    ctx->ff_inf.fo_detected = 1;
}
