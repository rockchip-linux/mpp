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
        mpp_err_f("invalid data %data size %d\n", data, size);
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

MPP_RET mpp_rc_init(MppRateControl **ctx)
{
    if (NULL == ctx) {
        mpp_log_f("invalid ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    MppRateControl *p = mpp_calloc(MppRateControl, 1);
    if (NULL == ctx) {
        mpp_log_f("malloc failed\n");
        ret = MPP_ERR_MALLOC;
    } else {
        p->gop = -1;
    }

    mpp_env_get_u32("mpp_rc_debug", &mpp_rc_debug, 0x1000);

    *ctx = p;
    return ret;
}


RK_S32 mpp_rc_vbv_check(MppVirtualBuffer *vb, RK_S32 timeInc, RK_S32 hrd)
{
    RK_S32 drift, target, bitPerPic = vb->bitPerPic;
    if (hrd) {
#if RC_CBR_HRD
        /* In CBR mode, bucket _must not_ underflow. Insert filler when
         * needed. */
        vb->bucketFullness -= bitPerPic;
#else
        if (vb->bucketFullness >= bitPerPic) {
            vb->bucketFullness -= bitPerPic;
        } else {
            vb->realBitCnt += (bitPerPic - vb->bucketFullness);
            vb->bucketFullness = 0;
        }
#endif
    }

    /* Saturate realBitCnt, this is to prevent overflows caused by much greater
       bitrate setting than is really possible to reach */
    if (vb->realBitCnt > 0x1FFFFFFF)
        vb->realBitCnt = 0x1FFFFFFF;
    if (vb->realBitCnt < -0x1FFFFFFF)
        vb->realBitCnt = -0x1FFFFFFF;

    vb->picTimeInc    += timeInc;
    vb->virtualBitCnt += axb_div_c(vb->bitRate, timeInc, vb->timeScale);
    target = vb->virtualBitCnt - vb->realBitCnt;

    /* Saturate target, prevents rc going totally out of control.
       This situation should never happen. */
    if (target > 0x1FFFFFFF)
        target = 0x1FFFFFFF;
    if (target < -0x1FFFFFFF)
        target = -0x1FFFFFFF;

    /* picTimeInc must be in range of [0, timeScale) */
    while (vb->picTimeInc >= vb->timeScale) {
        vb->picTimeInc    -= vb->timeScale;
        vb->virtualBitCnt -= vb->bitRate;
        vb->realBitCnt    -= vb->bitRate;
    }
    drift = axb_div_c(vb->bitRate, vb->picTimeInc, vb->timeScale);
    drift -= vb->virtualBitCnt;
    vb->virtualBitCnt += drift;

    mpp_rc_dbg_vbv("virtualBitCnt:\t\t%6i  realBitCnt: %i  ",
                   vb->virtualBitCnt, vb->realBitCnt);
    mpp_rc_dbg_vbv("target: %i  timeInc: %i\n", target, timeInc);
    return target;
}

RK_S32 mpp_rc_vbv_update(MppRateControl *ctx, int bitCnt)
{
    MppVirtualBuffer *vb = &ctx->vb;
    RK_S32 stat;
    if (ctx->hrd && (bitCnt > (vb->bufferSize - vb->bucketFullness))) {
        mpp_rc_dbg_vbv("Be: %7i  ", vb->bucketFullness);
        mpp_rc_dbg_vbv("fillerBits %5i  ", 0);
        mpp_rc_dbg_vbv("bitCnt %d  spaceLeft %d ",
                       bitCnt, (vb->bufferSize - vb->bucketFullness));
        mpp_rc_dbg_vbv("bufSize %d  bucketFullness %d  bitPerPic %d\n",
                       vb->bufferSize, vb->bucketFullness, vb->bitPerPic);
        mpp_rc_dbg_vbv("HRD overflow, frame discard\n");
        return MPP_ERR_BUFFER_FULL;
    } else {
        vb->bucketFullness += bitCnt;
        vb->realBitCnt += bitCnt;
    }
    if (!ctx->hrd) {
        return 0;
    }
#if RC_CBR_HRD
    /* Bits needed to prevent bucket underflow */
    tmp = vb->bitPerPic - vb->bucketFullness;

    if (tmp > 0) {
        tmp = (tmp + 7) / 8;
        vb->bucketFullness += tmp * 8;
        vb->realBitCnt += tmp * 8;
    } else {
        tmp = 0;
    }
#endif

    /* Update Buffering Info */
    stat = vb->bufferSize - vb->bucketFullness;

    return stat;
}

static void mpp_rc_vbv_init(MppRateControl *ctx, MppEncRcCfg *cfg)
{
    RK_S32 tmp = 3 * 8 * ctx->mb_per_frame * 256 / 2;
    RK_S32 bps = ctx->bps_target;
    RK_S32 cpbSize = -1;
    RK_S32 i = 0;
    /* bits per second */
    tmp = axb_div_c(tmp, cfg->fps_out_num, cfg->fps_out_denorm);
    if (bps > (tmp / 2))
        bps = tmp / 2;

    cpbSize = bps;

    /* Limit minimum CPB size based on average bits per frame */
    tmp = axb_div_c(bps, cfg->fps_out_denorm, cfg->fps_out_num);
    cpbSize = MPP_MAX(cpbSize, tmp);

    /* cpbSize must be rounded so it is exactly the size written in stream */
    tmp = cpbSize;
    while (4095 < (tmp >> (4 + i++)));
    cpbSize = (tmp >> (4 + i)) << (4 + i);
    ctx->vb.bufferSize = cpbSize;
    ctx->vb.bitRate = ctx->bps_target;
    ctx->vb.timeScale = cfg->fps_in_num;
    ctx->vb.unitsInTic = cfg->fps_in_denorm;
    ctx->vb.windowRem = ctx->window_len;
    ctx->vb.bitPerPic = ctx->bits_per_pic;
    if (ctx->hrd) {
        ctx->vb.bucketFullness = axb_div_c(ctx->vb.bufferSize, 60, 100);
        ctx->vb.bucketFullness = ctx->vb.bufferSize - ctx->vb.bucketFullness;
        ctx->vb.bucketFullness += ctx->vb.bitPerPic;
    }
}

MPP_RET mpp_rc_deinit(MppRateControl *ctx)
{
    if (NULL == ctx) {
        mpp_log_f("invalid ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    if (ctx->intra) {
        mpp_data_deinit(ctx->intra);
        ctx->intra = NULL;
    }

    if (ctx->inter) {
        mpp_data_deinit(ctx->inter);
        ctx->inter = NULL;
    }

    if (ctx->gop_bits) {
        mpp_data_deinit(ctx->gop_bits);
        ctx->gop_bits = NULL;
    }

    if (ctx->intra_percent) {
        mpp_data_deinit(ctx->intra_percent);
        ctx->intra_percent = NULL;
    }

    mpp_free(ctx);
    return MPP_OK;
}

MPP_RET mpp_rc_update_user_cfg(MppRateControl *ctx, MppEncRcCfg *cfg, RK_S32 force_idr)
{
    if (NULL == ctx || NULL == cfg) {
        mpp_log_f("invalid ctx %p cfg %p\n", ctx, cfg);
        return MPP_ERR_NULL_PTR;
    }

    RK_U32 change = cfg->change;
    RK_U32 clear_acc = 0;
    RK_U32 gop_start = 0;

    /*
     * step 1: update parameters
     */
    if (change & MPP_ENC_RC_CFG_CHANGE_BPS) {
        mpp_rc_dbg_cfg("bps: %d [%d %d]\n", cfg->bps_target,
                       cfg->bps_min, cfg->bps_max);
        ctx->bps_min = cfg->bps_min;
        ctx->bps_max = cfg->bps_max;
        ctx->bps_target = cfg->bps_target;

        ctx->min_rate = ctx->bps_min * 1.0 / ctx->bps_target;
        ctx->max_rate = ctx->bps_max * 1.0 / ctx->bps_target;
    }

    if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
        mpp_rc_dbg_cfg("fps: %d / %d\n", cfg->fps_out_num, cfg->fps_out_denorm);
        ctx->fps_num = cfg->fps_out_num;
        ctx->fps_denom = cfg->fps_out_denorm;
        if (ctx->fps_denom == 0) {
            mpp_err("denorm can not be 0, change to default 1");
            ctx->fps_denom = 1;
        }
        ctx->fps_out = ctx->fps_num / ctx->fps_denom;
        if (ctx->fps_out == 0) {
            mpp_err("fps out can not be 0, change to default 30");
            ctx->fps_out = 30;
            ctx->fps_num = 30;
            ctx->fps_denom = 1;
        }
        clear_acc = 1;
    }

    if ((change & MPP_ENC_RC_CFG_CHANGE_GOP) &&
        (ctx->gop != cfg->gop)) {
        RK_S32 gop = cfg->gop;

        mpp_rc_dbg_cfg("gop: %d\n", cfg->gop);

        if (ctx->intra)
            mpp_data_deinit(ctx->intra);
        mpp_data_init(&ctx->intra, gop);

        if (ctx->inter)
            mpp_data_deinit(ctx->inter);
        mpp_data_init(&ctx->inter, ctx->fps_out); /* need test */

        if (ctx->gop_bits)
            mpp_data_deinit(ctx->gop_bits);
        mpp_data_init(&ctx->gop_bits, gop);

        if (ctx->intra_percent)
            mpp_data_deinit(ctx->intra_percent);
        mpp_data_init(&ctx->intra_percent, gop);

        ctx->gop = gop;
        if (gop < ctx->fps_out)
            ctx->window_len = ctx->fps_out;
        else
            ctx->window_len = gop;

        if (ctx->window_len < 10)
            ctx->window_len = 10;

        if (ctx->window_len > ctx->fps_out)
            ctx->window_len = ctx->fps_out;

        mpp_pid_reset(&ctx->pid_intra);
        mpp_pid_reset(&ctx->pid_inter);
        mpp_pid_set_param(&ctx->pid_intra, 4, 6, 0, 100, ctx->window_len);
        mpp_pid_set_param(&ctx->pid_inter, 4, 6, 0, 100, ctx->window_len);
        mpp_pid_set_param(&ctx->pid_fps, 4, 6, 0, 100, ctx->window_len);

        clear_acc = 1;

        /* if gop cfg changed, current frame is regarded as IDR frame */
        gop_start = 1;
    }

    /*
     * step 2: derivate parameters
     */
    ctx->bits_per_pic = axb_div_c(cfg->bps_target, ctx->fps_denom, ctx->fps_num);
    mpp_rc_dbg_rc("RC: rc ctx %p target bit per picture %d\n", ctx, ctx->bits_per_pic);

    if (clear_acc) {
        ctx->acc_intra_bits_in_fps = 0;
        ctx->acc_inter_bits_in_fps = 0;
        ctx->acc_total_bits = 0;
        ctx->acc_intra_count = 0;
        ctx->acc_inter_count = 0;
        ctx->last_fps_bits = 0;
    }

    RK_S32 gop = ctx->gop;
    RK_S32 avg = ctx->bits_per_pic;

    if (clear_acc) {
        if (gop == 0) {
            /* only one intra then all inter */
            ctx->gop_mode = MPP_GOP_ALL_INTER;
            ctx->bits_per_inter = avg;
            ctx->bits_per_intra = avg * 10;
            ctx->intra_to_inter_rate = 0;
        } else if (gop == 1) {
            /* all intra */
            ctx->gop_mode = MPP_GOP_ALL_INTRA;
            ctx->bits_per_inter = 0;
            ctx->bits_per_intra = avg;
            ctx->intra_to_inter_rate = 0;
            gop_start = 1;
        } else if (ctx->gop < ctx->window_len) {
            /* small gop - use fix allocation */
            ctx->gop_mode = MPP_GOP_SMALL;
            ctx->intra_to_inter_rate = gop + 1;
            ctx->bits_per_inter = avg / 2;
            ctx->bits_per_intra = ctx->bits_per_inter * ctx->intra_to_inter_rate;
        } else {
            /* large gop - use dynamic allocation */
            RK_U32 intra_to_inter_rate;
            ctx->gop_mode = MPP_GOP_LARGE;
            mpp_env_get_u32("intra_rate", &intra_to_inter_rate, 3);
            ctx->intra_to_inter_rate = intra_to_inter_rate;
            ctx->bits_per_inter = ctx->bits_per_pic;
            ctx->bits_per_intra = ctx->bits_per_pic * 3;
            ctx->bits_per_inter -= ctx->bits_per_intra / (ctx->fps_out - 1);
        }
        mpp_rc_vbv_init(ctx, cfg);
    }

    if (ctx->acc_total_count == gop)
        gop_start = 1;

    if (force_idr) {
        ctx->cur_frmtype = INTRA_FRAME;
    } else {
        if (gop_start)
            ctx->cur_frmtype = INTRA_FRAME;
        else
            ctx->cur_frmtype = INTER_P_FRAME;
    }
    if (ctx->cur_frmtype == INTRA_FRAME) {
        ctx->acc_total_count = 0;
        ctx->acc_inter_bits_in_fps = 0;
        ctx->acc_intra_bits_in_fps = 0;
    }
    ctx->quality = cfg->quality;
    cfg->change = 0;

    return MPP_OK;
}

MPP_RET mpp_rc_bits_allocation(MppRateControl *ctx, RcSyntax *rc_syn)
{
    if (NULL == ctx || NULL == rc_syn) {
        mpp_log_f("invalid ctx %p rc_syn %p\n", ctx, rc_syn);
        return MPP_ERR_NULL_PTR;
    }
    mpp_rc_vbv_check(&ctx->vb, 1, 1);
    /* step 1: calc target frame bits */
    switch (ctx->gop_mode) {
    case MPP_GOP_ALL_INTER : {
        if (ctx->cur_frmtype == INTRA_FRAME)
            ctx->bits_target = ctx->bits_per_intra;
        else
            ctx->bits_target = ctx->bits_per_inter - mpp_pid_calc(&ctx->pid_inter);
    } break;
    case MPP_GOP_ALL_INTRA : {
        ctx->bits_target = ctx->bits_per_intra - mpp_pid_calc(&ctx->pid_intra);
    } break;
    default : {
        if (ctx->cur_frmtype == INTRA_FRAME) {
            float intra_percent = 0.0;
            RK_S32 diff_bit = mpp_pid_calc(&ctx->pid_fps);
            /* only affected by last gop */
            ctx->pre_gop_left_bit =  ctx->pid_fps.i - diff_bit;
            if ( abs(ctx->pre_gop_left_bit) / (ctx->gop - 1) > (ctx->bits_per_pic / 5)) {
                RK_S32 val = 1;
                if (ctx->pre_gop_left_bit < 0) {
                    val = -1;
                }
                ctx->pre_gop_left_bit  = val * ctx->bits_per_pic * (ctx->gop - 1) / 5;
            }
            mpp_pid_reset(&ctx->pid_fps);
            if (ctx->acc_intra_count) {
                intra_percent = mpp_data_avg(ctx->intra_percent, 1, 1, 1) / 100.0;
                ctx->last_intra_percent = intra_percent;
                ctx->bits_target = (ctx->fps_out * ctx->bits_per_pic + diff_bit)
                                   * intra_percent;
            } else {
                ctx->bits_target = ctx->bits_per_intra
                                   - mpp_pid_calc(&ctx->pid_intra);
            }
        } else {
            if (ctx->pre_frmtype == INTRA_FRAME) {
                RK_S32 diff_bit = mpp_pid_calc(&ctx->pid_fps);
                /*
                 * case - inter frame after intra frame
                 * update inter target bits with compensation of previous intra frame
                 */
                RK_S32 bits_prev_intra = mpp_data_avg(ctx->intra, 1, 1, 1);

                ctx->bits_per_inter = (ctx->bps_target *
                                       (ctx->gop * 1.0 / ctx->fps_out) -
                                       bits_prev_intra + diff_bit *
                                       (1 - ctx->last_intra_percent) + ctx->pre_gop_left_bit) /
                                      (ctx->gop - 1);

                mpp_rc_dbg_rc("RC: rc ctx %p bits pic %d win %d intra %d inter %d\n",
                              ctx, ctx->bits_per_pic, ctx->window_len,
                              bits_prev_intra, ctx->bits_per_inter);

                mpp_rc_dbg_rc("RC: rc ctx %p update inter target bits to %d\n",
                              ctx, ctx->bits_per_inter);
                ctx->bits_target = ctx->bits_per_inter;
            } else {
                RK_S32 diff_bit = mpp_pid_calc(&ctx->pid_inter);
                ctx->bits_target = ctx->bits_per_inter - diff_bit;
                if (ctx->bits_target > ctx->bits_per_pic * 2) {
                    ctx->bits_target = 2 * ctx->bits_per_pic;
                    ctx->pid_inter.i = ctx->pid_inter.i / 2;
                }
                mpp_rc_dbg_rc("RC: rc ctx %p inter pid diff %d target %d\n",
                              ctx, diff_bit, ctx->bits_target);

                if (ctx->quality != MPP_ENC_RC_QUALITY_AQ_ONLY) {
                    if (ctx->pid_inter.p > ctx->bits_per_inter * 1 / 2) {
                        ctx->prev_aq_prop_offset -= 4;
                    } else if (ctx->pid_inter.p > ctx->bits_per_inter * 1 / 3) {
                        ctx->prev_aq_prop_offset -= 3;
                    } else if (ctx->pid_inter.p > ctx->bits_per_inter * 1 / 6) {
                        ctx->prev_aq_prop_offset -= 2;
                    } else if (ctx->pid_inter.p > ctx->bits_per_inter * 1 / 10) {
                        ctx->prev_aq_prop_offset -= 1;
                    }
                    if (ctx->bits_target > (ctx->bits_per_inter * 11 / 12) && ctx->prev_aq_prop_offset < 0) {
                        ctx->prev_aq_prop_offset += 1;
                    }
                } else {
                    if (ctx->pid_inter.p > ctx->bits_per_inter) {
                        ctx->prev_aq_prop_offset -= 4;
                    } else if (ctx->pid_inter.p > ctx->bits_per_inter * 2 / 3) {
                        ctx->prev_aq_prop_offset -= 3;
                    } else if (ctx->pid_inter.p > ctx->bits_per_inter * 1 / 2) {
                        ctx->prev_aq_prop_offset -= 2;
                    } else if (ctx->pid_inter.p > ctx->bits_per_inter * 1 / 3) {
                        ctx->prev_aq_prop_offset -= 1;
                    }
                    if (ctx->bits_target > ctx->bits_per_inter * 5 / 4) {
                        ctx->prev_aq_prop_offset += 2 ;
                    } else if (ctx->bits_target > ctx->bits_per_inter * 8 / 7) {
                        ctx->prev_aq_prop_offset += 1;
                    }
                }
            }
        }
    } break;
    }

    /* If target bit is zero, it will exist mosaic in the encoded picture.
     * In this case, half of target bit rate of previous P frame  is
     * assigned to target bit.
     */
    if (ctx->bits_target <= 0) {
        if (ctx->cur_frmtype == INTRA_FRAME) {
            mpp_rc_dbg_rc("unbelievable case: intra frame target bits is zero!\n");
            ctx->bits_target = ctx->prev_intra_target / 2;
        } else {
            mpp_rc_dbg_rc("inter frame target bits is zero!"
                          "intra frame %d, inter frame %d, total_cnt %d\n",
                          ctx->acc_intra_count, ctx->acc_inter_count,
                          ctx->acc_total_count);
            ctx->bits_target = ctx->prev_inter_target / 4;
            mpp_rc_dbg_rc("after adjustment, target bits %d\n", ctx->bits_target);
        }
    }

    if (ctx->quality == MPP_ENC_RC_QUALITY_AQ_ONLY) {
        rc_syn->aq_prop_offset = ctx->prev_aq_prop_offset;
        ctx->prev_aq_prop_offset = 0;
    } else {
        ctx->prev_aq_prop_offset = mpp_clip(ctx->prev_aq_prop_offset, -8, 0);
        rc_syn->aq_prop_offset = ctx->prev_aq_prop_offset;
    }

    rc_syn->bit_target = ctx->bits_target;

    /* step 2: calc min and max bits */
    rc_syn->bit_min = ctx->bits_target * ctx->min_rate;
    rc_syn->bit_max = ctx->bits_target * ctx->max_rate;

    /* step 3: save bit target as previous target for next frame */
    const char *type_str;
    rc_syn->type = ctx->cur_frmtype;
    rc_syn->gop_mode = ctx->gop_mode;
    if (ctx->cur_frmtype == INTRA_FRAME) {
        type_str = "intra";
        ctx->prev_intra_target = ctx->bits_target;
    } else {
        type_str = "inter";
        ctx->prev_inter_target = ctx->bits_target;
    }
    mpp_rc_dbg_rc("RC: rc ctx %p %s target bits %d\n", ctx, type_str,
                  ctx->bits_target);

    return MPP_OK;
}

MPP_RET mpp_rc_update_hw_result(MppRateControl *ctx, RcHalResult *result)
{
    if (NULL == ctx || NULL == result) {
        mpp_log_f("invalid ctx %p result %p\n", ctx, result);
        return MPP_ERR_NULL_PTR;
    }

    RK_S32 bits = result->bits;
    const char *type_str;
    RK_S32 bits_target;
    if (result->type == INTRA_FRAME) {
        ctx->acc_intra_count++;
        ctx->acc_intra_bits_in_fps += bits;
        mpp_data_update(ctx->intra, bits);
        mpp_data_update(ctx->gop_bits, bits);
        mpp_pid_update(&ctx->pid_intra, bits - ctx->bits_target);
        type_str = "intra";
        bits_target = ctx->bits_per_intra;
    } else {
        ctx->acc_inter_count++;
        ctx->acc_inter_bits_in_fps += bits;
        mpp_data_update(ctx->inter, bits);
        mpp_data_update(ctx->gop_bits, bits);
        mpp_pid_update(&ctx->pid_inter, bits - ctx->bits_target);
        type_str = "inter";
        bits_target = ctx->bits_per_inter;
    }

    mpp_rc_dbg_rc("RC: rc ctx %p %s real bits %d target %d\n",
                  ctx, type_str, bits, bits_target);

    ctx->acc_total_count++;
    ctx->last_fps_bits += bits;

    /* new fps start */
    if ((ctx->acc_intra_count + ctx->acc_inter_count) % ctx->fps_out == 0) {
        mpp_pid_update(&ctx->pid_fps, ctx->bps_target - ctx->last_fps_bits);
        if (ctx->acc_intra_bits_in_fps && ctx->acc_inter_bits_in_fps)
            mpp_data_update(ctx->intra_percent,
                            ctx->acc_intra_bits_in_fps * 100 /
                            (ctx->acc_inter_bits_in_fps + ctx->acc_intra_bits_in_fps));

        if (!ctx->time_in_second)
            mpp_rc_dbg_bps("|--time--|---kbps---|--- I ---|--- P ---|-rate-|\n");

        mpp_rc_dbg_bps("|%8d|%10.2f|%9.2f|%9.2f|%6.2f|\n",
                       ctx->time_in_second,
                       (float)ctx->last_fps_bits / 1000.0,
                       (float)ctx->acc_intra_bits_in_fps / 1000.0,
                       (float)ctx->acc_inter_bits_in_fps / 1000.0,
                       (float)ctx->acc_intra_bits_in_fps * 100.0 /
                       (float)ctx->acc_inter_bits_in_fps);

        ctx->acc_intra_bits_in_fps = 0;
        ctx->acc_inter_bits_in_fps = 0;
        ctx->last_fps_bits = 0;
        ctx->time_in_second++;
    }
    mpp_rc_vbv_update(ctx, bits);
    ctx->pre_frmtype = ctx->cur_frmtype;

    return MPP_OK;
}

MPP_RET mpp_rc_record_param(struct list_head *head, MppRateControl *ctx,
                            RcSyntax *rc_syn)
{
    MPP_RET ret = MPP_OK;

    if (mpp_rc_debug & MPP_RC_DBG_RECORD) {
        RecordNode *node = (RecordNode *)mpp_calloc(RecordNode, 1);
        mpp_assert(node);
        INIT_LIST_HEAD(&node->list);
        node->frm_type = ctx->cur_frmtype;
        node->frm_cnt = ++ctx->frm_cnt;
        node->bps = ctx->bps_target;
        node->fps = ctx->fps_out;
        node->gop = ctx->gop;
        node->bits_per_pic = ctx->bits_per_pic;
        node->bits_per_intra = ctx->bits_per_intra;
        node->bits_per_inter = ctx->bits_per_inter;
        node->tgt_bits = ctx->bits_target;
        node->bit_min = ctx->bits_target * ctx->min_rate;
        node->bit_max = ctx->bits_target * ctx->max_rate;
        node->acc_intra_bits_in_fps = ctx->acc_intra_bits_in_fps;
        node->acc_inter_bits_in_fps = ctx->acc_inter_bits_in_fps;
        node->last_fps_bits = ctx->last_fps_bits;
        node->last_intra_percent = ctx->last_intra_percent;

        list_add_tail(&node->list, head);
        rc_syn->rc_head = head;
    }

    return ret;
}

MPP_RET mpp_rc_calc_real_bps(struct list_head *head, MppRateControl *ctx,
                             RK_S32 cur_bits)
{
    MPP_RET ret = MPP_OK;

    if (mpp_rc_debug & MPP_RC_DBG_RECORD) {
        RK_U32 print_flag = 0;
        ctx->real_bps += cur_bits;

        if ((ctx->acc_intra_count + ctx->acc_inter_count) % ctx->fps_out == 0) {
            if (ctx->real_bps > ctx->bps_target * 5) /* threshold can be modified */
                print_flag = 0; /* used to debug only */

            RecordNode *pos, *n;
            MppLinReg *lin_reg;

            list_for_each_entry_safe(pos, n, head, RecordNode, list) {
                if (print_flag) {
                    mpp_log("Start to duplicate RC parameter of frame %d!\n", pos->frm_cnt);
                    mpp_log("bps %d, real_bps %d\n", pos->bps, ctx->real_bps);
                    mpp_log("fps %d, gop %d, last_intra_percent %0.3f\n",
                            pos->fps, pos->gop, pos->last_intra_percent);
                    mpp_log("bits_per_pic %d, bits_per_intra %d, bits_per_inter %d\n",
                            pos->bits_per_pic, pos->bits_per_intra, pos->bits_per_inter);
                    mpp_log("acc_intra_bits_in_fps %d, acc_inter_bits_in_fps %d, last_fps_bits %d\n",
                            pos->acc_intra_bits_in_fps, pos->acc_inter_bits_in_fps,
                            pos->last_fps_bits);
                    mpp_log("qp_sum %d, sse_sum %lld\n", pos->qp_sum, pos->sse_sum);
                    mpp_log("tgt_bits %d, real_bits %d\n", pos->tgt_bits, pos->real_bits);
                    mpp_log("set_qp %d, real_qp %d\n", pos->set_qp, pos->real_qp);

                    lin_reg = &pos->lin_reg;
                    mpp_log("\nStart to duplicate RQ model parameter!\n");
                    mpp_log("size %d, n %d, i %d\n", lin_reg->size, lin_reg->n, lin_reg->i);
                    mpp_log("a %f, b %f, c %f\n", lin_reg->a, lin_reg->b, lin_reg->c);
                    mpp_log("x %p, r %p, y %p\n", lin_reg->x, lin_reg->r, lin_reg->y);
                    mpp_log("weight_mode %d, wlen %d\n", lin_reg->weight_mode, pos->wlen);
                    mpp_log("\n\n");
                }

                /*
                 * Only @FPS nodes are freed every time, and the node whose frame
                 * number is (@acc_intra_count + @acc_inter_count) belongs to next
                 * group, so just free it next time.
                 */
                if ((ctx->acc_intra_count + ctx->acc_inter_count) != (RK_S32)pos->frm_cnt) {
                    list_del_init(&pos->list);
                    mpp_free(pos);
                }
            }

            ctx->real_bps = 0;
        }
    }

    return ret;
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


