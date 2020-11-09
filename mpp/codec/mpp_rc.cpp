/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_rc"

#include <math.h>
#include <memory.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_rc.h"

#define MPP_RC_DBG_FUNCTION          (0x00000001)
#define MPP_RC_DBG_BPS               (0x00000010)
#define MPP_RC_DBG_RC                (0x00000020)
#define MPP_RC_DBG_CFG               (0x00000100)
#define MPP_RC_DBG_RECORD            (0x00001000)
#define MPP_RC_DBG_VBV               (0x00002000)


#define mpp_rc_dbg(flag, fmt, ...)   _mpp_dbg(mpp_rc_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_f(flag, fmt, ...) _mpp_dbg_f(mpp_rc_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_rc_dbg_func(fmt, ...)    mpp_rc_dbg_f(MPP_RC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_bps(fmt, ...)     mpp_rc_dbg(MPP_RC_DBG_BPS, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_rc(fmt, ...)      mpp_rc_dbg(MPP_RC_DBG_RC, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_cfg(fmt, ...)     mpp_rc_dbg(MPP_RC_DBG_CFG, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_vbv(fmt, ...)     mpp_rc_dbg(MPP_RC_DBG_VBV, fmt, ## __VA_ARGS__)


#define SIGN(a)         ((a) < (0) ? (-1) : (1))
#define DIV(a, b)       (((a) + (SIGN(a) * (b)) / 2) / (b))

static RK_U32 mpp_rc_debug = 0;

MPP_RET mpp_data_init(MppData **data, RK_S32 size)
{
    if (NULL == data || size <= 0) {
        mpp_err_f("invalid data %p size %d\n", data, size);
        return MPP_ERR_NULL_PTR;
    }

    *data = NULL;
    MppData *p = mpp_malloc_size(MppData, sizeof(MppData) + sizeof(RK_S32) * size);
    if (NULL == p) {
        mpp_err_f("malloc size %d failed\n", size);
        return MPP_ERR_MALLOC;
    }
    p->size = size;
    p->len = 0;
    p->pos = 0;
    p->val = (RK_S32 *)(p + 1);
    *data = p;

    return MPP_OK;
}

void mpp_data_deinit(MppData *p)
{
    if (p)
        mpp_free(p);
}

void mpp_data_update(MppData *p, RK_S32 val)
{
    mpp_assert(p);

    p->val[p->pos] = val;

    if (++p->pos >= p->size)
        p->pos = 0;

    if (p->len < p->size)
        p->len++;
}

RK_S32 mpp_data_avg(MppData *p, RK_S32 len, RK_S32 num, RK_S32 denorm)
{
    mpp_assert(p);

    RK_S32 i;
    RK_S32 sum = 0;
    RK_S32 pos = p->pos;

    if (!p->len)
        return 0;

    if (len < 0 || len > p->len)
        len = p->len;

    if (num == denorm) {
        i = len;
        while (i--) {
            if (pos)
                pos--;
            else
                pos = p->len - 1;

            sum += p->val[pos];
        }
    } else {
        /* This case is not used so far, but may be useful in the future */
        mpp_assert(num > denorm);
        RK_S32 acc_num = num;
        RK_S32 acc_denorm = denorm;

        i = len - 1;
        sum = p->val[--pos];
        while (i--) {
            if (pos)
                pos--;
            else
                pos = p->len - 1;

            sum += p->val[pos] * acc_num / acc_denorm;
            acc_num *= num;
            acc_denorm *= denorm;
        }
    }
    return DIV(sum, len);
}

void mpp_pid_reset(MppPIDCtx *p)
{
    p->p = 0;
    p->i = 0;
    p->d = 0;
    p->count = 0;
}

void mpp_pid_set_param(MppPIDCtx *ctx, RK_S32 coef_p, RK_S32 coef_i, RK_S32 coef_d, RK_S32 div, RK_S32 len)
{
    ctx->coef_p = coef_p;
    ctx->coef_i = coef_i;
    ctx->coef_d = coef_d;
    ctx->div = div;
    ctx->len = len;
    ctx->count = 0;

    mpp_rc_dbg_rc("RC: pid ctx %p coef: P %d I %d D %d div %d len %d\n",
                  ctx, coef_p, coef_i, coef_d, div, len);
}

void mpp_pid_update(MppPIDCtx *ctx, RK_S32 val)
{
    mpp_rc_dbg_rc("RC: pid ctx %p update val %d\n", ctx, val);
    mpp_rc_dbg_rc("RC: pid ctx %p before update P %d I %d D %d\n", ctx, ctx->p, ctx->i, ctx->d);

    ctx->d = val - ctx->p;      /* Derivative */
    ctx->i = val + ctx->i;      /* Integral */
    ctx->p = val;               /* Proportional */

    mpp_rc_dbg_rc("RC: pid ctx %p after  update P %d I %d D %d\n", ctx, ctx->p, ctx->i, ctx->d);
    ctx->count++;
    /*
     * pid control is a short time control, it needs periodically reset
     */
    if (ctx->count >= ctx->len)
        mpp_pid_reset(ctx);
}

RK_S32 mpp_pid_calc(MppPIDCtx *p)
{
    RK_S32 a = p->p * p->coef_p + p->i * p->coef_i + p->d * p->coef_d;
    RK_S32 b = p->div;

    mpp_rc_dbg_rc("RC: pid ctx %p p %10d coef %d\n", p, p->p, p->coef_p);
    mpp_rc_dbg_rc("RC: pid ctx %p i %10d coef %d\n", p, p->i, p->coef_i);
    mpp_rc_dbg_rc("RC: pid ctx %p d %10d coef %d\n", p, p->d, p->coef_d);
    mpp_rc_dbg_rc("RC: pid ctx %p a %10d b %d\n", p, a, b);

    return DIV(a, b);
}

MPP_RET mpp_rc_param_ops(struct list_head *head, RK_U32 frm_cnt,
                         RC_PARAM_OPS ops, void *arg)
{
    MPP_RET ret = MPP_OK;

    if (mpp_rc_debug & MPP_RC_DBG_RECORD) {
        RecordNode *pos, *n;
        RK_U32 found_match = 0;

        list_for_each_entry_safe(pos, n, head, RecordNode, list) {
            if (frm_cnt == pos->frm_cnt) {
                found_match = 1;
                break;
            }
        }

        if (!found_match) {
            mpp_err("frame %d is not found in list_head %p!\n", frm_cnt, head);
            ret = MPP_NOK;
        } else {
            switch (ops) {
            case RC_RECORD_REAL_BITS : {
                pos->real_bits = *((RK_U32*)arg);
            } break;
            case RC_RECORD_QP_SUM : {
                pos->qp_sum = *((RK_S32*)arg);
            } break;
            case RC_RECORD_QP_MIN : {
                pos->qp_min = *((RK_S32*)arg);
            } break;
            case RC_RECORD_QP_MAX : {
                pos->qp_max = *((RK_S32*)arg);
            } break;
            case RC_RECORD_WIN_LEN : {
                pos->wlen = *((RK_S32*)arg);
            } break;
            case RC_RECORD_SET_QP : {
                pos->set_qp = *((RK_S32*)arg);
            } break;
            case RC_RECORD_REAL_QP : {
                pos->real_qp = *((RK_S32*)arg);
            } break;
            case RC_RECORD_SSE_SUM : {
                pos->sse_sum = *((RK_S64*)arg);
            } break;
            default : {
                mpp_err("frame %d found invalid operation code %d\n", frm_cnt, ops);
                ret = MPP_NOK;
            }
            }
        }
    }

    return ret;
}


