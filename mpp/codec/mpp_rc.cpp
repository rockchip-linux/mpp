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

MPP_RET mpp_linreg_init(MppLinReg **ctx, RK_S32 size, RK_S32 weight_mode)
{
    if (NULL == ctx) {
        mpp_log_f("invalid ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    MppLinReg *p = mpp_calloc_size(MppLinReg,
                                   sizeof(MppLinReg) +
                                   size * (sizeof(RK_S32) * 2 + sizeof(RK_S64)));
    if (NULL == p) {
        mpp_log_f("malloc failed\n");
        ret = MPP_ERR_MALLOC;
    } else {
        p->x = (RK_S32 *)(p + 1);
        p->r = p->x + size;
        p->y = (RK_S64 *)(p->r + size);
        p->size = size;
        p->weight_mode = weight_mode;
    }

    *ctx = p;
    return ret;
}

MPP_RET mpp_linreg_deinit(MppLinReg *ctx)
{
    if (NULL == ctx) {
        mpp_log_f("invalid ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    mpp_free(ctx);
    return MPP_OK;
}

static RK_S64 linreg_weight[6][15] = {
    {1000, 500, 250, 125, 63, 31, 16, 8, 4, 2, 1, 0, 0, 0, 0},
    {1000, 600, 360, 216, 130, 78, 47, 28, 17, 10, 6, 4, 2, 1, 0},
    {1000, 700, 490, 343, 240, 168, 117, 82, 57, 40, 32, 26, 20, 16, 13},
    {1000, 800, 640, 512, 409, 328, 262, 210, 168, 134, 107, 86, 69, 55, 44},
    {1000, 900, 810, 729, 656, 590, 531, 478, 430, 387, 348, 313, 282, 254, 229},
    {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000}
};

static inline RK_S64 POW(RK_S64 A, RK_S64 B)
{
    RK_S64 X = 1;
    while (B--)
        X *= A;

    return X;
}

void mpp_save_regdata(MppLinReg *ctx, RK_S32 x, RK_S32 r)
{
    RK_S64 y = (RK_S64)x * x * r;
    ctx->x[ctx->i] = x;
    ctx->r[ctx->i] = r;
    ctx->y[ctx->i] = y;

    mpp_rc_dbg_rc("RC: linreg %p save index %d x %d r %d x*x*r %lld\n",
                  ctx, ctx->i, x, r, y);

    if (++ctx->i >= ctx->size)
        ctx->i = 0;

    if (ctx->n < ctx->size)
        ctx->n++;
}

MPP_RET mpp_quadreg_update(MppLinReg *ctx, RK_S32 wlen)
{
    double A[3][3];
    double B[3];

    RK_S32 cq = 0;
    RK_S32 est_b = 0;
    RK_S32 w = 0;
    RK_S32 err = 0;
    RK_S32 std = 0;

    RK_S32 k, l;

    double a = 0, b = 0, c = 0;

    memset(A, 0, sizeof(A));
    memset(B, 0, sizeof(B));

    /* step 2: update coefficient */
    RK_S32 i = 0;
    RK_S32 n;

    RK_S32 *cx = ctx->x;
    RK_S32 *cr = ctx->r;
    RK_S64 *cy = ctx->y;
    RK_S32 idx = 0;
    RK_S32 aver_x = 0;

    n = ctx->n;
    i = ctx->i;

    while (n--) {
        if (i == 0)
            i = ctx->size - 1;
        else
            i--;

        cq = cx[i];
        aver_x += cx[i];

        /* limite wlen when complexity change sharply */
        if (idx++ > wlen)
            break;
    }

    w = idx;
    aver_x = round(1.0 * aver_x / w);

    idx = 0;
    n = w;
    i = ctx->i;

    while (n--) {
        if (i == 0)
            i = ctx->size - 1;
        else
            i--;

        if (cq != cx[i])
            est_b = 1;

        err = cx[i] - aver_x;
        std += err * err;
        b += 1.0 * cx[i] * cr[i] / w;
    }

    mpp_rc_dbg_rc("qstep std %f average %d\n", sqrt(1.0 * std / w), aver_x);

    /*
     * Do not calculate quadratic model when samples distributing in narrow range,
     * avoid the quadratic model distortion.
     */
    if (sqrt(1.0 * std / w) * 32 < aver_x)
        return MPP_OK;

    if (est_b && w >= 1) {
        n = w;
        i = ctx->i;
        while (n--) {
            if (i == 0)
                i = ctx->size - 1;
            else
                i--;

            RK_S64 wt = linreg_weight[ctx->weight_mode][idx++];

            mpp_rc_dbg_rc("qs[%d] %d, r[%d] %d, y[%d] %lld\n",
                          idx - 1, cx[i], idx - 1, cr[i], idx - 1, cy[i]);

            for (k = 0; k < 3; k++) {
                for (l = 0; l < 3; l++) {
                    A[k][l] += wt * POW(cx[i], k + l);
                }
            }

            for (k = 0; k < 3; k++)
                B[k] += wt * cy[i] * POW(cx[i], k);
        }

        mpp_rc_dbg_rc("\nmatrix A:\n");
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\n", A[0][0], A[0][1], A[0][2]);
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\n", A[1][0], A[1][1], A[1][2]);
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\n", A[2][0], A[2][1], A[2][2]);

        mpp_rc_dbg_rc("\nvector B:\n");
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\n", B[0], B[1], B[2]);

        A[2][1] = A[2][1] - A[0][1] * (A[2][0] / A[0][0]);
        A[2][2] = A[2][2] - A[0][2] * (A[2][0] / A[0][0]);
        B[2]    = B[2]    - B[0]    * (A[2][0] / A[0][0]);
        A[2][0] = 0;

        A[1][1] = A[1][1] - A[0][1] * (A[1][0] / A[0][0]);
        A[1][2] = A[1][2] - A[0][2] * (A[1][0] / A[0][0]);
        B[1]    = B[1]    - B[0]    * (A[1][0] / A[0][0]);
        A[1][0] = 0;

        A[2][2] = A[2][2] - A[1][2] * (A[2][1] / A[1][1]);
        B[2]    = B[2]    - B[1]    * (A[2][1] / A[1][1]);
        A[2][1] = 0;

        c = B[2] / A[2][2];
        b = (B[1] - A[1][2] * c) / A[1][1];
        a = (B[0] - A[0][2] * c - A[0][1] * b) / A[0][0];

        mpp_rc_dbg_rc("\nmatrix after gaussian elimination:\n");
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\t\t| %e\n", A[0][0], A[0][1], A[0][2], B[0]);
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\t\t| %e\n", A[1][0], A[1][1], A[1][2], B[1]);
        mpp_rc_dbg_rc("%e\t\t%e\t\t%e\t\t| %e\n", A[2][0], A[2][1], A[2][2], B[2]);
    }

    if (c < 0) {
        mpp_linreg_update(ctx);
    } else {
        ctx->a = a;
        ctx->b = b;
        ctx->c = c;

        mpp_rc_dbg_rc("quadreg: a %e b %e c %e\n",
                      a, b, c);
    }

    return MPP_OK;
}

/*
 * This function want to calculate coefficient 'b' 'a' using ordinary
 * least square.
 * y = b * x + a
 * b_n = accumulate(x * y) - n * (average(x) * average(y))
 * a_n = accumulate(x * x) * accumulate(y) - accumulate(x) * accumulate(x * y)
 * denom = accumulate(x * x) - n * (square(average(x))
 * b = b_n / denom
 * a = a_n / denom
 */
MPP_RET mpp_linreg_update(MppLinReg *ctx)
{
    if (NULL == ctx) {
        mpp_log_f("invalid ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    /* step 2: update coefficient */
    RK_S32 i = 0;
    RK_S32 n;
    RK_S64 acc_xy = 0;
    RK_S64 acc_x = 0;
    RK_S64 acc_y = 0;
    RK_S64 acc_sq_x = 0;

    RK_S64 b_num = 0;
    RK_S64 denom = 0;

    RK_S32 *cx = ctx->x;
    RK_S64 *cy = ctx->y;
    RK_S64 ws = 0;
    RK_S32 idx = 0;
    RK_S64 w;

    n = ctx->n;
    i = ctx->i;

    while (n--) {
        if (i == 0)
            i = ctx->size - 1;
        else
            i--;

        w = linreg_weight[ctx->weight_mode][idx++];
        ws += w;
        acc_xy += w * cx[i] * cy[i];
        acc_x += w * cx[i];
        acc_y += w * cy[i];
        acc_sq_x += w * cx[i] * cx[i];
    }

    b_num = acc_xy - acc_y * acc_x / ws;
    denom = acc_sq_x - acc_x * acc_x / ws;

    mpp_rc_dbg_rc("RC: linreg %p acc_xy %lld acc_x %lld acc_y %lld acc_sq_x %lld\n",
                  ctx, acc_xy, acc_x, acc_y, acc_sq_x);
    mpp_rc_dbg_rc("RC: linreg %p n %d b_num %lld denom %lld\n",
                  ctx, ctx->n, b_num, denom);

    mpp_rc_dbg_rc("RC: linreg %p before update coefficient a %d b %d\n",
                  ctx, ctx->a, ctx->b);

    if (denom)
        ctx->b = DIV(b_num, denom);
    else
        ctx->b = 0;

    ctx->a = DIV(acc_y - acc_x * ctx->b, ws);
    ctx->c = 0;

    mpp_rc_dbg_rc("RC: linreg %p after  update coefficient a %d b %d\n",
                  ctx, ctx->a, ctx->b);

    return MPP_OK;
}

RK_S64 mpp_quadreg_calc(MppLinReg *ctx, RK_S32 x)
{
    if (x <= 0)
        return -1;

    return ctx->c + DIV(ctx->b, x) + DIV(ctx->a, x * x);
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
            case RC_RECORD_LIN_REG : {
                memcpy(&pos->lin_reg, (MppLinReg*)arg, sizeof(MppLinReg));
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


