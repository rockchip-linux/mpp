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

#define MODULE_TAG "rc_model_v2_smt"

#include <string.h>
#include <math.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "rc_base.h"
#include "rc_debug.h"
#include "rc_model_v2_smt.h"
#include "mpp_rc.h"

#define LOW_QP 34
#define LOW_LOW_QP 35

typedef struct RcModelV2SmtCtx_t {
    RcCfg           usr_cfg;
    RK_U32          frame_type;
    RK_U32          last_frame_type;
    RK_U32          first_frm_flg;
    RK_S64          frm_num;
    RK_S32          qp_min;
    RK_S32          qp_max;
    MppEncGopMode   gop_mode;
    RK_S64          acc_intra_count;
    RK_S64          acc_inter_count;
    RK_S32          last_fps_bits;
    RK_S32          pre_gop_left_bit;
    MppData         *qp_p;
    MppDataV2       *motion_level;
    MppDataV2       *complex_level;
    MppDataV2       *stat_bits;
    MppDataV2       *rt_bits; /* real time bits */
    MppPIDCtx       pid_fps;
    RK_S64          count_real_bit;
    RK_S64          count_pred_bit;
    RK_S64          count_frame;
    RK_S64          fixed_i_pred_bit;
    RK_S64          fixed_p_pred_bit;
    RK_S32          change_bit_flag;

    RK_S32          bits_tgt_lower; /* bits target lower limit */
    RK_S32          bits_tgt_upper; /* bits target upper limit */
    RK_S32          bits_per_lower_i; /* bits per intra frame in low rate */
    RK_S32          bits_per_upper_i; /* bits per intra frame in high rate */
    RK_S32          bits_per_lower_p; /* bits per P frame in low rate */
    RK_S32          bits_per_upper_p; /* bits per P frame in high rate */

    RK_S32          pre_diff_bit_lower;
    RK_S32          pre_diff_bit_upper;
    RK_S32          igop;
    MppPIDCtx       pid_lower_i;
    MppPIDCtx       pid_upper_i;
    MppPIDCtx       pid_lower_all;
    MppPIDCtx       pid_upper_all;
    MppDataV2       *pid_lower_p;
    MppDataV2       *pid_upper_p;
    RK_S32          qp_out;
    RK_S32          qp_prev_out;
    RK_S32          pre_real_bit_i; /* real bit of last intra frame */
    RK_S32          pre_qp_i; /* qp of last intra frame */
    RK_S32          gop_qp_sum;
    RK_S32          gop_frm_cnt;
    RK_S32          pre_iblk4_prop;
    RK_S32          reenc_cnt;
    RK_U32          drop_cnt;
    RK_S32          on_drop;
    RK_S32          on_pskip;
} RcModelV2SmtCtx;

// rc_container_bitrate_thd2
static RK_S32 rc_ctnr_qp_thd1[6] = { 51, 42, 42, 51, 38, 38 };
static RK_S32 rc_ctnr_qp_thd2[6] = { 51, 44, 44, 51, 40, 40 };
static RK_S32 rc_ctnr_br_thd1[6] = { 100, 110, 110, 100, 110, 110 };
static RK_S32 rc_ctnr_br_thd2[6] = { 100, 120, 120, 100, 125, 125 };

MPP_RET bits_model_smt_deinit(RcModelV2SmtCtx *ctx)
{
    rc_dbg_func("enter %p\n", ctx);

    if (ctx->qp_p) {
        mpp_data_deinit(ctx->qp_p);
        ctx->qp_p = NULL;
    }

    if (ctx->motion_level != NULL) {
        mpp_data_deinit_v2(ctx->motion_level);
        ctx->motion_level = NULL;
    }

    if (ctx->complex_level != NULL) {
        mpp_data_deinit_v2(ctx->complex_level);
        ctx->complex_level = NULL;
    }

    if (ctx->stat_bits != NULL) {
        mpp_data_deinit_v2(ctx->stat_bits);
        ctx->stat_bits = NULL;
    }

    if (ctx->rt_bits != NULL) {
        mpp_data_deinit_v2(ctx->rt_bits);
        ctx->rt_bits = NULL;
    }

    if (ctx->pid_lower_p != NULL) {
        mpp_data_deinit_v2(ctx->pid_lower_p);
        ctx->pid_lower_p = NULL;
    }

    if (ctx->pid_upper_p != NULL) {
        mpp_data_deinit_v2(ctx->pid_upper_p);
        ctx->pid_upper_p = NULL;
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_smt_init(RcModelV2SmtCtx *ctx)
{
    RK_S32 gop_len = ctx->usr_cfg.igop;
    RcFpsCfg *fps = &ctx->usr_cfg.fps;
    RK_S32 mad_len = 10;
    RK_S32 ave_bits_lower = 0, ave_bits_uppper = 0;
    RK_S32 bits_lower_i, bits_upper_i;
    RK_S32 bits_lower_p, bits_upper_p;
    RK_S32 bit_ratio[5] = { 7, 8, 9, 10, 11 };
    RK_S32 nfps = fps->fps_out_num / fps->fps_out_denom;
    RK_S32 win_len = mpp_clip(MPP_MAX3(gop_len, nfps, 10), 1, nfps);
    RK_S32 rt_stat_len = fps->fps_out_num / fps->fps_out_denom; /* real time stat len */
    RK_S32 stat_len = fps->fps_out_num * ctx->usr_cfg.stats_time / fps->fps_out_denom;
    stat_len = stat_len ? stat_len : (fps->fps_out_num * 8 / fps->fps_out_denom);

    rc_dbg_func("enter %p\n", ctx);
    ctx->frm_num = 0;
    ctx->first_frm_flg = 1;
    ctx->gop_frm_cnt = 0;
    ctx->gop_qp_sum = 0;

    // smt
    ctx->igop = gop_len;
    ctx->qp_min = 10;
    ctx->qp_max = 51;

    if (ctx->motion_level)
        mpp_data_deinit_v2(ctx->motion_level);
    mpp_data_init_v2(&ctx->motion_level, mad_len, 0);

    if (ctx->complex_level)
        mpp_data_deinit_v2(ctx->complex_level);
    mpp_data_init_v2(&ctx->complex_level, mad_len, 0);

    if (ctx->pid_lower_p)
        mpp_data_deinit_v2(ctx->pid_lower_p);
    mpp_data_init_v2(&ctx->pid_lower_p, stat_len, 0);

    if (ctx->pid_upper_p)
        mpp_data_deinit_v2(ctx->pid_upper_p);
    mpp_data_init_v2(&ctx->pid_upper_p, stat_len, 0);

    mpp_pid_reset(&ctx->pid_fps);
    mpp_pid_reset(&ctx->pid_lower_i);
    mpp_pid_reset(&ctx->pid_upper_i);
    mpp_pid_reset(&ctx->pid_lower_all);
    mpp_pid_reset(&ctx->pid_upper_all);

    mpp_pid_set_param(&ctx->pid_fps, 4, 6, 0, 90, win_len);
    mpp_pid_set_param(&ctx->pid_lower_i, 4, 6, 0, 100, win_len);
    mpp_pid_set_param(&ctx->pid_upper_i, 4, 6, 0, 100, win_len);
    mpp_pid_set_param(&ctx->pid_lower_all, 4, 6, 0, 100, gop_len);
    mpp_pid_set_param(&ctx->pid_upper_all, 4, 6, 0, 100, gop_len);

    ave_bits_lower = (RK_S64)ctx->usr_cfg.bps_min * fps->fps_out_denom / fps->fps_out_num;
    ave_bits_uppper = (RK_S64)ctx->usr_cfg.bps_max * fps->fps_out_denom / fps->fps_out_num;

    ctx->acc_intra_count = 0;
    ctx->acc_inter_count = 0;
    ctx->last_fps_bits = 0;

    if (gop_len == 0) {
        ctx->gop_mode = MPP_GOP_ALL_INTER;
        bits_lower_p = ave_bits_lower;
        bits_lower_i = ave_bits_lower * 10;
        bits_upper_p = ave_bits_uppper;
        bits_upper_i = ave_bits_uppper * 10;
    } else if (gop_len == 1) {
        ctx->gop_mode = MPP_GOP_ALL_INTRA;
        bits_lower_p = 0;
        bits_lower_i = ave_bits_lower;
        bits_upper_p = 0;
        bits_upper_i = ave_bits_uppper;
        /* disable debreath on all intra case */
        if (ctx->usr_cfg.debreath_cfg.enable)
            ctx->usr_cfg.debreath_cfg.enable = 0;
    } else if (gop_len < win_len) {
        ctx->gop_mode = MPP_GOP_SMALL;
        bits_lower_p = ave_bits_lower >> 1;
        bits_lower_i = bits_lower_p * (gop_len + 1);
        bits_upper_p = ave_bits_uppper >> 1;
        bits_upper_i = bits_upper_p * (gop_len + 1);
    } else {
        RK_S32 g = gop_len;
        RK_S32 idx = g <= 50 ? 0 : (g <= 100 ? 1 : (g <= 200 ? 2 : (g <= 300 ? 3 : 4)));

        ctx->gop_mode = MPP_GOP_LARGE;
        bits_lower_i = ave_bits_lower * bit_ratio[idx] / 2;
        bits_upper_i = ave_bits_uppper * bit_ratio[idx] / 2;
        bits_lower_p = ave_bits_lower - bits_lower_i / (nfps - 1);
        bits_upper_p = ave_bits_uppper - bits_upper_i / (nfps - 1);
        ctx->fixed_i_pred_bit = (ctx->usr_cfg.bps_max / nfps * 8) / 8;
        ctx->fixed_p_pred_bit = ((ctx->usr_cfg.bps_max * g / nfps - ctx->fixed_i_pred_bit) / (g - 1)) / 8;
    }

    ctx->bits_per_lower_i = bits_lower_i;
    ctx->bits_per_upper_i = bits_upper_i;
    ctx->bits_per_lower_p = bits_lower_p;
    ctx->bits_per_upper_p = bits_upper_p;

    rc_dbg_rc("bits_per_lower_i %d, bits_per_upper_i %d, "
              "bits_per_lower_p %d, bits_per_upper_p %d\n",
              bits_lower_i, bits_upper_i, bits_lower_p, bits_upper_p);

    if (ctx->stat_bits)
        mpp_data_deinit_v2(ctx->stat_bits);
    mpp_data_init_v2(&ctx->stat_bits, stat_len, bits_upper_p);

    if (ctx->rt_bits)
        mpp_data_deinit_v2(ctx->rt_bits);
    mpp_data_init_v2(&ctx->rt_bits, rt_stat_len, bits_upper_p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_update_smt(RcModelV2SmtCtx *ctx, RK_S32 real_bit)
{
    rc_dbg_func("enter %p\n", ctx);
    RcFpsCfg *fps = &ctx->usr_cfg.fps;
    RK_S32 bps_target_tmp = 0;
    RK_S32 mod = 0;

    rc_dbg_func("enter %p\n", ctx);

    mpp_data_update_v2(ctx->stat_bits, real_bit);
    ctx->pre_diff_bit_lower = ctx->bits_tgt_lower - real_bit;
    ctx->pre_diff_bit_upper = ctx->bits_tgt_upper - real_bit;

    ctx->count_real_bit = ctx->count_real_bit + real_bit / 8;
    if (ctx->frame_type == INTRA_FRAME) {
        ctx->count_pred_bit = ctx->count_pred_bit + ctx->fixed_i_pred_bit;
    } else {
        ctx->count_pred_bit = ctx->count_pred_bit + ctx->fixed_p_pred_bit;
    }
    ctx->count_frame ++;
    if (ctx->count_real_bit > 72057594037927935 || ctx->count_pred_bit > 72057594037927935) {
        ctx->count_real_bit = 0;
        ctx->count_pred_bit = 0;
    }

    if (ctx->change_bit_flag == 1) {
        real_bit = real_bit * 8 / 10;
    }

    if (ctx->frame_type == INTRA_FRAME) {
        ctx->acc_intra_count++;
        mpp_pid_update(&ctx->pid_lower_i, real_bit - ctx->bits_tgt_lower, 1);
        mpp_pid_update(&ctx->pid_upper_i, real_bit - ctx->bits_tgt_upper, 1);
    } else {
        ctx->acc_inter_count++;
        mpp_data_update_v2(ctx->pid_lower_p, real_bit - ctx->bits_tgt_lower);
        mpp_data_update_v2(ctx->pid_upper_p, real_bit - ctx->bits_tgt_upper);
    }
    mpp_pid_update(&ctx->pid_lower_all, real_bit - ctx->bits_tgt_lower, 1);
    mpp_pid_update(&ctx->pid_upper_all, real_bit - ctx->bits_tgt_upper, 1);

    ctx->last_fps_bits += real_bit;
    /* new fps start */
    mod = ctx->acc_intra_count + ctx->acc_inter_count;
    mod = mod % (fps->fps_out_num / fps->fps_out_denom);
    if (0 == mod) {
        bps_target_tmp = (ctx->usr_cfg.bps_min + ctx->usr_cfg.bps_max) >> 1;
        if (bps_target_tmp * 3 > (ctx->last_fps_bits * 2))
            mpp_pid_update(&ctx->pid_fps, bps_target_tmp - ctx->last_fps_bits, 0);
        else {
            bps_target_tmp = ctx->usr_cfg.bps_min * 4 / 10 + ctx->usr_cfg.bps_max * 6 / 10;
            mpp_pid_update(&ctx->pid_fps, bps_target_tmp - ctx->last_fps_bits, 0);
        }
        ctx->last_fps_bits = 0;
    }

    /* new frame start */
    ctx->qp_prev_out = ctx->qp_out;

    rc_dbg_func("leave %p\n", ctx);

    return MPP_OK;
}

MPP_RET rc_model_v2_smt_h265_init(void *ctx, RcCfg *cfg)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx*)ctx;

    rc_dbg_func("enter %p\n", ctx);

    memcpy(&p->usr_cfg, cfg, sizeof(RcCfg));
    bits_model_smt_init(p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_smt_h264_init(void *ctx, RcCfg *cfg)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx*)ctx;

    rc_dbg_func("enter %p\n", ctx);

    memcpy(&p->usr_cfg, cfg, sizeof(RcCfg));
    bits_model_smt_init(p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}


MPP_RET rc_model_v2_smt_deinit(void *ctx)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx *)ctx;
    rc_dbg_func("enter %p\n", ctx);
    bits_model_smt_deinit(p);
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

static void set_coef(void *ctx, RK_S32 *coef, RK_S32 val)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx*)ctx;
    RK_S32 cplx_lvl_0 = mpp_data_get_pre_val_v2(p->complex_level, 0);
    RK_S32 cplx_lvl_1 = mpp_data_get_pre_val_v2(p->complex_level, 1);
    RK_S32 cplx_lvl_sum = mpp_data_sum_v2(p->complex_level);

    if (cplx_lvl_sum == 0)
        *coef = val + 0;
    else if (cplx_lvl_sum == 1) {
        if (cplx_lvl_0 == 0)
            *coef = val + 10;
        else
            *coef = val + 25;
    } else if (cplx_lvl_sum == 2) {
        if (cplx_lvl_0 == 0)
            *coef = val + 25;
        else
            *coef = val + 35;
    } else if (cplx_lvl_sum == 3) {
        if (cplx_lvl_0 == 0)
            *coef = val + 35;
        else
            *coef = val + 51;
    } else if (cplx_lvl_sum >= 4 && cplx_lvl_sum <= 6) {
        if (cplx_lvl_0 == 0) {
            if (cplx_lvl_1 == 0)
                *coef = val + 35;
            else
                *coef = val + 51;
        } else
            *coef = val + 64;
    } else if (cplx_lvl_sum >= 7 && cplx_lvl_sum <= 9) {
        if (cplx_lvl_0 == 0) {
            if (cplx_lvl_1 == 0)
                *coef = val + 64;
            else
                *coef = val + 72;
        } else
            *coef = val + 72;
    } else
        *coef = val + 80;
}

static RK_U32 mb_num[9] = {
    0, 200, 700, 1200, 2000, 4000, 8000, 16000, 20000
};

static RK_U32 tab_bit[9] = {
    3780, 3570, 3150, 2940, 2730, 3780, 2100, 1680, 2100
};

static RK_U8 qscale2qp[96] = {
    15,  15,  15,  15,  15,  16, 18, 20, 21, 22, 23,
    24,  25,  25,  26,  27,  28, 28, 29, 29, 30, 30,
    30,  31,  31,  32,  32,  33, 33, 33, 34, 34, 34,
    34,  35,  35,  35,  36,  36, 36, 36, 36, 37, 37,
    37,  37,  38,  38,  38,  38, 38, 39, 39, 39, 39,
    39,  39,  40,  40,  40,  40, 41, 41, 41, 41, 41,
    41,  41,  42,  42,  42,  42, 42, 42, 42, 42, 43,
    43,  43,  43,  43,  43,  43, 43, 44, 44, 44, 44,
    44,  44,  44,  44,  45,  45, 45, 45,
};

static RK_U8 inter_pqp0[52] = {
    1,  1,  1,  1,  1,  2,  3,  4,
    5,  6,  7,  8,  9,  10, 11, 12,
    13, 14, 15, 17, 18, 19, 20, 21,
    21, 21, 22, 23, 24, 25, 26, 26,
    27, 28, 28, 29, 29, 29, 30, 31,
    31, 32, 32, 33, 33, 34, 35, 35,
    35, 36, 36, 36
};

static RK_U8 inter_pqp1[52] = {
    1,  1,  2,  3,  4,  5,  6,  7,
    8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 20, 21, 22,
    23, 24, 25, 26, 26, 27, 28, 29,
    29, 30, 31, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 42,
    42, 43, 43, 44
};

static RK_U8 intra_pqp0[3][52] = {
    {
        1,  1,  1,  2,  3,  4,  5,  6,
        7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22,
        23, 23, 24, 25, 26, 27, 27, 28,
        28, 29, 30, 31, 32, 32, 33, 34,
        34, 34, 35, 35, 36, 36, 36, 36,
        37, 37, 37, 38
    },

    {
        1,  1,  1,  2,  3,  4,  5,  6,
        7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 17, 18, 18, 19, 19,
        20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 32, 33, 34,
        34, 34, 35, 35, 36, 36, 36, 36,
        37, 37, 37, 38
    },

    {
        1,  1,  1,  2,  3,  4,  5,  6,
        7,  8,  9,  10, 11, 12, 13, 14,
        14, 15, 15, 16, 16, 17, 17, 18,
        16, 16, 16, 17, 18, 19, 20, 21,
        23, 24, 26, 28, 30, 31, 32, 33,
        34, 34, 35, 35, 36, 36, 36, 36,
        37, 37, 37, 38
    },
};

static RK_U8 intra_pqp1[52] = {
    2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50,
    51, 51, 51, 51
};

static RK_S32 cal_smt_first_i_start_qp(RK_S32 target_bit, RK_U32 total_mb)
{
    RK_S32 cnt = 0;
    RK_S32 index;
    RK_S32 i;

    for (i = 0; i < 8; i++) {
        if (mb_num[i] > total_mb)
            break;
        cnt++;
    }

    index = (total_mb * tab_bit[cnt] - 350) / target_bit; // qscale
    index = mpp_clip(index, 4, 95);

    return qscale2qp[index];
}

static MPP_RET calc_smt_debreath_qp(RcModelV2SmtCtx * ctx)
{
    RK_S32 fm_qp_sum = 0;
    RK_S32 new_fm_qp = 0;
    RcDebreathCfg *debreath_cfg = &ctx->usr_cfg.debreath_cfg;
    RK_S32 dealt_qp = 0;
    RK_S32 gop_qp_sum = ctx->gop_qp_sum;
    RK_S32 gop_frm_cnt = ctx->gop_frm_cnt;
    static RK_S8 intra_qp_map[8] = {
        0, 0, 1, 1, 2, 2, 2, 2,
    };
    RK_U8 idx2 = MPP_MIN(ctx->pre_iblk4_prop >> 5, (RK_S32)sizeof(intra_qp_map) - 1);

    static RK_S8 strength_map[36] = {
        0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4,
        5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8,
        9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12
    };

    rc_dbg_func("enter %p\n", ctx);

    fm_qp_sum = MPP_MIN(gop_qp_sum / gop_frm_cnt, (RK_S32)sizeof(strength_map) - 1);

    rc_dbg_qp("i qp_out %d, qp_start_sum = %d, intra_lv4_prop %d",
              ctx->qp_out, fm_qp_sum, ctx->pre_iblk4_prop);

    dealt_qp = strength_map[debreath_cfg->strength] - intra_qp_map[idx2];
    if (fm_qp_sum > dealt_qp)
        new_fm_qp = fm_qp_sum - dealt_qp;
    else
        new_fm_qp = fm_qp_sum;

    ctx->qp_out = mpp_clip(new_fm_qp, ctx->usr_cfg.min_i_quality, ctx->usr_cfg.max_i_quality);
    ctx->gop_frm_cnt = 0;
    ctx->gop_qp_sum = 0;
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

static MPP_RET smt_start_prepare(void *ctx, EncRcTask *task)
{
    EncFrmStatus *frm = &task->frm;
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
    EncRcTaskInfo *info = &task->info;
    RcFpsCfg *fps = &p->usr_cfg.fps;
    RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
    RK_S32 b_min = p->usr_cfg.bps_min;
    RK_S32 b_max = p->usr_cfg.bps_max;
    RK_S32 bits_lower, bits_upper;

    p->frame_type = frm->is_intra ? INTRA_FRAME : INTER_P_FRAME;
    if (frm->ref_mode == REF_TO_PREV_INTRA)
        p->frame_type = info->frame_type = INTER_VI_FRAME;

    switch (p->gop_mode) {
    case MPP_GOP_ALL_INTER: {
        if (p->frame_type == INTRA_FRAME) {
            bits_lower = p->bits_per_lower_i;
            bits_upper = p->bits_per_upper_i;
        } else {
            bits_lower = p->bits_per_lower_p - mpp_data_mean_v2(p->pid_lower_p);
            bits_upper = p->bits_per_upper_p - mpp_data_mean_v2(p->pid_upper_p);
        }
    } break;
    case MPP_GOP_ALL_INTRA: {
        bits_lower = p->bits_per_lower_i - mpp_pid_calc(&p->pid_lower_i);
        bits_upper = p->bits_per_upper_i - mpp_pid_calc(&p->pid_upper_i);
    } break;
    default: {
        if (p->frame_type == INTRA_FRAME) {
            RK_S32 diff_bit = mpp_pid_calc(&p->pid_fps);

            p->pre_gop_left_bit = p->pid_fps.i - diff_bit;
            mpp_pid_reset(&p->pid_fps);
            if (p->acc_intra_count) {
                bits_lower = (p->bits_per_lower_i + diff_bit);
                bits_upper = (p->bits_per_upper_i + diff_bit);
            } else {
                bits_lower = p->bits_per_lower_i - mpp_pid_calc(&p->pid_lower_i);
                bits_upper = p->bits_per_upper_i - mpp_pid_calc(&p->pid_upper_i);
            }
        } else {
            if (p->last_frame_type == INTRA_FRAME) {
                RK_S32 bits_prev_i = p->pre_real_bit_i;

                bits_lower = p->bits_per_lower_p
                             = ((RK_S64)b_min * p->igop / fps_out - bits_prev_i +
                                p->pre_gop_left_bit) / (p->igop - 1);

                bits_upper = p->bits_per_upper_p
                             = ((RK_S64)b_max * p->igop / fps_out - bits_prev_i +
                                p->pre_gop_left_bit) / (p->igop - 1);

            } else {
                RK_S32 diff_bit_lr = mpp_data_mean_v2(p->pid_lower_p);
                RK_S32 diff_bit_hr = mpp_data_mean_v2(p->pid_upper_p);
                RK_S32 lr = (RK_S64)b_min * fps->fps_out_denom / fps->fps_out_num;
                RK_S32 hr = (RK_S64)b_max * fps->fps_out_denom / fps->fps_out_num;

                bits_lower = p->bits_per_lower_p - diff_bit_lr;
                if (bits_lower > 2 * lr)
                    bits_lower = 2 * lr;

                bits_upper = p->bits_per_upper_p - diff_bit_hr;
                if (bits_upper > 2 * hr)
                    bits_upper = 2 * hr;
            }

        }
    } break;
    }

    p->bits_tgt_lower = bits_lower;
    p->bits_tgt_upper = bits_upper;
    info->bit_max = (bits_lower + bits_upper) / 2;
    if (info->bit_max < 100)
        info->bit_max = 100;

    if (NULL == p->qp_p) {
        RK_S32 nfps = fps_out < 15 ? 4 * fps_out : (fps_out < 25 ? 3 * fps_out : 2 * fps_out);
        mpp_data_init(&p->qp_p, mpp_clip(MPP_MAX(p->igop, nfps), 20, 50));
    }

    rc_dbg_rc("bits_tgt_lower %d, bits_tgt_upper %d, bit_max %d, qp_out %d",
              p->bits_tgt_lower, p->bits_tgt_upper, info->bit_max, p->qp_out);

    return MPP_OK;
}

static RK_S32 smt_calc_coef(void *ctx)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
    RK_S32 coef = 1024;
    RK_S32 coef2 = 512;
    RK_S32 md_lvl_sum = mpp_data_sum_v2(p->motion_level);
    RK_S32 md_lvl_0 = mpp_data_get_pre_val_v2(p->motion_level, 0);
    RK_S32 md_lvl_1 = mpp_data_get_pre_val_v2(p->motion_level, 1);

    if (md_lvl_sum < 100)
        set_coef(ctx, &coef, 0);
    else if (md_lvl_sum < 200) {
        if (md_lvl_0 < 100)
            set_coef(ctx, &coef, 102);
        else
            set_coef(ctx, &coef, 154);
    } else if (md_lvl_sum < 300) {
        if (md_lvl_0 < 100)
            set_coef(ctx, &coef, 154);
        else if (md_lvl_0 == 100) {
            if (md_lvl_1  < 100)
                set_coef(ctx, &coef, 205);
            else if (md_lvl_1 == 100)
                set_coef(ctx, &coef, 256);
            else
                set_coef(ctx, &coef, 307);
        } else
            set_coef(ctx, &coef, 307);
    } else if (md_lvl_sum < 600) {
        if (md_lvl_0 < 100) {
            if (md_lvl_1  < 100)
                set_coef(ctx, &coef, 307);
            else if (md_lvl_1 == 100)
                set_coef(ctx, &coef, 358);
            else
                set_coef(ctx, &coef, 410);
        } else if (md_lvl_0 == 100) {
            if (md_lvl_1  < 100)
                set_coef(ctx, &coef, 358);
            else if (md_lvl_1 == 100)
                set_coef(ctx, &coef, 410);
            else
                set_coef(ctx, &coef, 461);
        } else
            set_coef(ctx, &coef, 461);
    } else if (md_lvl_sum < 900) {
        if (md_lvl_0 < 100) {
            if (md_lvl_1 < 100)
                set_coef(ctx, &coef, 410);
            else if (md_lvl_1 == 100)
                set_coef(ctx, &coef, 461);
            else
                set_coef(ctx, &coef, 512);
        } else if (md_lvl_0 == 100) {
            if (md_lvl_1 < 100)
                set_coef(ctx, &coef, 512);
            else if (md_lvl_1 == 100)
                set_coef(ctx, &coef, 563);
            else
                set_coef(ctx, &coef, 614);
        } else
            set_coef(ctx, &coef, 614);
    } else if (md_lvl_sum < 1500)
        set_coef(ctx, &coef, 666);
    else if (md_lvl_sum < 1900)
        set_coef(ctx, &coef, 768);
    else
        set_coef(ctx, &coef, 900);

    if (coef > 1024)
        coef = 1024;

    if (coef >= 900)
        coef2 = 1024;
    else if (coef >= 307)    // 0.7~0.3 --> 1.0~0.5
        coef2 = 512 + (coef - 307) * (1024 - 512) / (717 - 307);
    else    // 0.3~0.0 --> 0.5~0.0
        coef2 = 0 + coef * (512 - 0) / (307 - 0);
    if (coef2 >= 1024)
        coef2 = 1024;

    return coef2;
}

/* bit_target_use: average of bits_tgt_lower and bits_tgt_upper */
static RK_S32 derive_iframe_qp_by_bitrate(RcModelV2SmtCtx *p, RK_S32 bit_target_use)
{
    RcFpsCfg *fps = &p->usr_cfg.fps;
    RK_S32 avg_bps = (p->usr_cfg.bps_min + p->usr_cfg.bps_max) / 2;
    RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
    RK_S32 avg_pqp = mpp_data_avg(p->qp_p, -1, 1, 1);
    RK_S32 avg_qp = mpp_clip(avg_pqp, p->qp_min, p->qp_max);
    RK_S32 prev_iqp = p->pre_qp_i;
    RK_S32 prev_pqp = p->qp_prev_out;
    RK_S32 pre_bits_i = p->pre_real_bit_i;
    RK_S32 qp_out_i = 0;

    if (bit_target_use <= pre_bits_i) {
        qp_out_i = (bit_target_use * 5 < pre_bits_i) ? prev_iqp + 3 :
                   (bit_target_use * 2 < pre_bits_i) ? prev_iqp + 2 :
                   (bit_target_use * 3 < pre_bits_i * 2) ? prev_iqp + 1 : prev_iqp;
    } else {
        qp_out_i = (pre_bits_i * 3 < bit_target_use) ? prev_iqp - 3 :
                   (pre_bits_i * 2 < bit_target_use) ? prev_iqp - 2 :
                   (pre_bits_i * 3 < bit_target_use * 2) ? prev_iqp - 1 : prev_iqp;
    }
    rc_dbg_rc("frame %lld bit_target_use %d pre_bits_i %d prev_iqp %d qp_out_i %d\n",
              p->frm_num, bit_target_use, pre_bits_i, prev_iqp, qp_out_i);

    //FIX: may be invalid(2025.01.06)
    if (!p->reenc_cnt && p->usr_cfg.debreath_cfg.enable)
        calc_smt_debreath_qp(p);

    qp_out_i = mpp_clip(qp_out_i, inter_pqp0[avg_qp], inter_pqp1[avg_qp]);
    qp_out_i = mpp_clip(qp_out_i, inter_pqp0[prev_pqp], inter_pqp1[prev_pqp]);
    if (qp_out_i > 27)
        qp_out_i = mpp_clip(qp_out_i, intra_pqp0[0][prev_iqp], intra_pqp1[prev_iqp]);
    else if (qp_out_i > 22)
        qp_out_i = mpp_clip(qp_out_i, intra_pqp0[1][prev_iqp], intra_pqp1[prev_iqp]);
    else
        qp_out_i = mpp_clip(qp_out_i, intra_pqp0[2][prev_iqp], intra_pqp1[prev_iqp]);

    rc_dbg_rc("frame %lld qp_out_i %d avg_qp %d prev_pqp %d prev_iqp %d qp_out_i %d\n",
              p->frm_num, qp_out_i, avg_qp, prev_pqp, prev_iqp, qp_out_i);

    if (p->pre_gop_left_bit < 0) {
        if (abs(p->pre_gop_left_bit) * 5 > avg_bps * (p->igop / fps_out))
            qp_out_i = mpp_clip(qp_out_i, 20, 51);
        else if (abs(p->pre_gop_left_bit) * 20 > avg_bps * (p->igop / fps_out))
            qp_out_i = mpp_clip(qp_out_i, 15, 51);

        rc_dbg_rc("frame %lld pre_gop_left_bit %d avg_bps %d qp_out_i %d\n",
                  p->frm_num, p->pre_gop_left_bit, avg_bps, qp_out_i);
    }

    return qp_out_i;
}

static RK_S32 derive_pframe_qp_by_bitrate(RcModelV2SmtCtx *p)
{
    RcFpsCfg *fps = &p->usr_cfg.fps;
    RK_S32 avg_bps = (p->usr_cfg.bps_min + p->usr_cfg.bps_max) / 2;
    RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
    RK_S32 bits_target_use = 0;
    RK_S32 pre_diff_bit_use = 0;
    RK_S32 coef = smt_calc_coef(p);
    RK_S32 m_tbr = p->bits_tgt_upper - p->bits_tgt_lower;
    RK_S32 m_dbr = p->pre_diff_bit_upper - p->pre_diff_bit_lower;
    RK_S32 diff_bit = (p->pid_lower_all.i + p->pid_upper_all.i) >> 1;
    RK_S32 prev_pqp = p->qp_prev_out;
    RK_S32 qp_out = p->qp_out;
    RK_S32 qp_add = 0, qp_minus = 0;

    bits_target_use = ((RK_S64)m_tbr * coef + (RK_S64)p->bits_tgt_lower * 1024) >> 10;
    pre_diff_bit_use = ((RK_S64)m_dbr * coef + (RK_S64)p->pre_diff_bit_lower * 1024) >> 10;

    if (bits_target_use < 100)
        bits_target_use = 100;

    rc_dbg_rc("frame %lld bits_target_use %d m_tbr %d coef %d bits_tgt_lower %d\n"
              "pre_diff_bit_use %d m_dbr %d  pre_diff_bit_lower %d "
              "bits_tgt_upper %d pre_diff_bit_upper %d qp_out_0 %d\n",
              p->frm_num, bits_target_use, m_tbr, coef, p->bits_tgt_lower,
              pre_diff_bit_use, m_dbr, p->pre_diff_bit_lower,
              p->bits_tgt_upper, p->pre_diff_bit_upper, qp_out);

    if (abs(pre_diff_bit_use) * 100 <= bits_target_use * 3)
        qp_out = prev_pqp - 1;
    else if (pre_diff_bit_use * 100 > bits_target_use * 3) {
        if (pre_diff_bit_use >= bits_target_use)
            qp_out = qp_out >= 30 ? prev_pqp - 4 : prev_pqp - 3;
        else if (pre_diff_bit_use * 4 >= bits_target_use * 1)
            qp_out = qp_out >= 30 ? prev_pqp - 3 : prev_pqp - 2;
        else if (pre_diff_bit_use * 10 > bits_target_use * 1)
            qp_out = prev_pqp - 2;
        else
            qp_out = prev_pqp - 1;
    } else {
        RK_S32 qp_add_tmp = (prev_pqp >= 36) ? 0 : 1;
        pre_diff_bit_use = abs(pre_diff_bit_use);
        qp_out = (pre_diff_bit_use >= 2 * bits_target_use) ? prev_pqp + 2 + qp_add_tmp :
                 (pre_diff_bit_use * 3 >= bits_target_use * 2) ? prev_pqp + 1 + qp_add_tmp :
                 (pre_diff_bit_use * 5 >  bits_target_use) ? prev_pqp + 1 : prev_pqp;
    }
    rc_dbg_rc("frame %lld prev_pqp %d qp_out_1 %d\n", p->frm_num, prev_pqp, qp_out);

    qp_out = mpp_clip(qp_out, p->qp_min, p->qp_max);
    if (qp_out > LOW_QP) {
        pre_diff_bit_use = ((RK_S64)m_dbr * coef + (RK_S64)p->pre_diff_bit_lower * 1024) >> 10;
        bits_target_use = avg_bps / fps_out;
        bits_target_use = -bits_target_use / 5;
        coef += pre_diff_bit_use <= 2 * bits_target_use ? 205 :
                ((pre_diff_bit_use <= bits_target_use) ? 102 : 51);

        if (coef >= 1024 || qp_out > LOW_LOW_QP)
            coef = 1024;
        rc_dbg_rc("frame %lld pre_diff_bit_use %d bits_target_use %d coef %d\n",
                  p->frm_num, pre_diff_bit_use, bits_target_use, coef);

        pre_diff_bit_use = ((RK_S64)m_dbr * coef + (RK_S64)p->pre_diff_bit_lower * 1024) >> 10;
        bits_target_use = ((RK_S64)m_tbr * coef + (RK_S64)p->bits_tgt_lower * 1024) >> 10;
        if (bits_target_use < 100)
            bits_target_use = 100;

        if (abs(pre_diff_bit_use) * 100 <= bits_target_use * 3)
            qp_out = prev_pqp;
        else if (pre_diff_bit_use * 100 > bits_target_use * 3) {
            if (pre_diff_bit_use >= bits_target_use)
                qp_out = qp_out >= 30 ? prev_pqp - 3 : prev_pqp - 2;
            else if (pre_diff_bit_use * 4 >= bits_target_use * 1)
                qp_out = qp_out >= 30 ? prev_pqp - 2 : prev_pqp - 1;
            else if (pre_diff_bit_use * 10 > bits_target_use * 1)
                qp_out = prev_pqp - 1;
            else
                qp_out = prev_pqp;
        } else {
            pre_diff_bit_use = abs(pre_diff_bit_use);
            qp_out = prev_pqp + (pre_diff_bit_use * 3 >= bits_target_use * 2 ? 1 : 0);
        }
        rc_dbg_rc("frame %lld pre_diff_bit_use %d bits_target_use %d prev_pqp %d qp_out_2 %d\n",
                  p->frm_num, pre_diff_bit_use, bits_target_use, prev_pqp, qp_out);
    }

    qp_out = mpp_clip(qp_out, p->qp_min, p->qp_max);

    //Add rc_container
    p->change_bit_flag = 0;
    if (p->usr_cfg.rc_container) {
        RK_S32 cnt = p->usr_cfg.scene_mode * 3 + p->usr_cfg.rc_container;
        if (p->count_real_bit < p->count_pred_bit * rc_ctnr_br_thd1[cnt] / 100) {
            if (qp_out > rc_ctnr_qp_thd1[cnt]) {
                p->change_bit_flag = 1;
            }

            qp_out = mpp_clip(qp_out, 10, rc_ctnr_qp_thd1[cnt]);
        } else if (p->count_real_bit < p->count_pred_bit * rc_ctnr_br_thd2[cnt] / 100) {
            if (qp_out > rc_ctnr_qp_thd2[cnt]) {
                p->change_bit_flag = 1;
            }
            qp_out = mpp_clip(qp_out, 10, rc_ctnr_qp_thd2[cnt]);
        }
    }

    qp_add = qp_out > 36 ? 1 : (qp_out > 33 ? 2 : (qp_out > 30 ? 3 : 4));
    qp_minus = qp_out > 40 ? 4 : (qp_out > 36 ? 3 : (qp_out > 33 ? 2 : 1));
    qp_out = mpp_clip(qp_out, prev_pqp - qp_minus, prev_pqp + qp_add);
    rc_dbg_rc("frame %lld qp_out_3 %d qp_add %d qp_minus %d\n",
              p->frm_num, qp_out, qp_add, qp_minus);

    if (diff_bit > 0) {
        if (avg_bps * 5 > avg_bps) //FIXME: avg_bps is typo error?(2025.01.06)
            qp_out = mpp_clip(qp_out, 25, 51);
        else if (avg_bps * 20 > avg_bps)
            qp_out = mpp_clip(qp_out, 21, 51);
        rc_dbg_rc("frame %lld avg_bps %d qp_out_4 %d\n", p->frm_num, avg_bps, qp_out);
    }

    return qp_out;
}

static RK_S32 revise_qp_by_complexity(RcModelV2SmtCtx *p, RK_S32 fm_min_iqp,
                                      RK_S32 fm_min_pqp, RK_S32 fm_max_iqp, RK_S32 fm_max_pqp)
{
    RK_S32 md_lvl_sum = mpp_data_sum_v2(p->motion_level);
    RK_S32 md_lvl_0 = mpp_data_get_pre_val_v2(p->motion_level, 0);
    RK_S32 cplx_lvl_sum = mpp_data_sum_v2(p->complex_level);
    RK_S32 qp_add = 0, qp_add_p = 0;
    RK_S32 qp_final = p->qp_out;

    qp_add = 4;
    qp_add_p = 4;
    if (md_lvl_sum >= 700 || md_lvl_0 == 200) {
        qp_add = 6;
        qp_add_p = 6;
    } else if (md_lvl_sum >= 400 || md_lvl_0 == 100) {
        qp_add = 5;
        qp_add_p = 5;
    }
    if (cplx_lvl_sum >= 12) {
        qp_add++;
        qp_add_p++;
    }

    rc_dbg_rc("frame %lld md_lvl_sum %d md_lvl_0 %d cplx_lvl_sum %d "
              "qp_add_cplx %d qp_add_p %d qp_final_0 %d\n",
              p->frm_num, md_lvl_sum, md_lvl_0, cplx_lvl_sum,
              qp_add, qp_add_p, qp_final);

    if (p->frame_type == INTRA_FRAME)
        qp_final = mpp_clip(qp_final, fm_min_iqp + qp_add, fm_max_iqp);
    else if (p->frame_type == INTER_VI_FRAME) {
        RK_S32 vi_max_qp = (fm_max_pqp > 42) ? (fm_max_pqp - 5) :
                           (fm_max_pqp > 39) ? (fm_max_pqp - 3) :
                           (fm_max_pqp > 35) ? (fm_max_pqp - 2) : fm_max_pqp;
        qp_final -= 1;
        qp_final = mpp_clip(qp_final, fm_min_pqp + qp_add - 1, fm_max_pqp);
        qp_final = mpp_clip(qp_final, qp_final, vi_max_qp);
    } else
        qp_final = mpp_clip(qp_final, fm_min_pqp + qp_add_p, fm_max_pqp);

    qp_final = mpp_clip(qp_final, p->qp_min, p->qp_max);
    rc_dbg_rc("frame %lld frm_type %d frm_qp %d:%d:%d:%d blk_qp %d:%d qp_final_1 %d\n",
              p->frm_num, p->frame_type, fm_min_iqp, fm_max_iqp,
              fm_min_pqp, fm_max_pqp, p->qp_min, p->qp_max, qp_final);

    return qp_final;
}

MPP_RET rc_model_v2_smt_start(void *ctx, EncRcTask * task)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
    EncFrmStatus *frm = &task->frm;
    EncRcTaskInfo *info = &task->info;
    RcFpsCfg *fps = &p->usr_cfg.fps;
    RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
    RK_S32 avg_pqp = 0;
    RK_S32 fm_min_iqp = p->usr_cfg.fqp_min_i;
    RK_S32 fm_min_pqp = p->usr_cfg.fqp_min_p;
    RK_S32 fm_max_iqp = p->usr_cfg.fqp_max_i;
    RK_S32 fm_max_pqp = p->usr_cfg.fqp_max_p;

    if (frm->reencode)
        return MPP_OK;

    smt_start_prepare(ctx, task);
    avg_pqp = mpp_data_avg(p->qp_p, -1, 1, 1);

    if (p->frm_num == 0) {
        RK_S32 mb_w = MPP_ALIGN(p->usr_cfg.width, 16) / 16;
        RK_S32 mb_h = MPP_ALIGN(p->usr_cfg.height, 16) / 16;
        RK_S32 ratio = mpp_clip(fps_out  / 10 + 1, 1, 3);
        RK_S32 qp_out_f0 = 0;
        if (p->usr_cfg.init_quality < 0) {
            qp_out_f0 = cal_smt_first_i_start_qp(p->bits_tgt_upper * ratio, mb_w * mb_h);
            qp_out_f0 = (fm_min_iqp > 31) ? mpp_clip(qp_out_f0, fm_min_iqp, p->qp_max) :
                        mpp_clip(qp_out_f0, 31, p->qp_max);
        } else
            qp_out_f0 = p->usr_cfg.init_quality;

        p->qp_out = qp_out_f0;
        p->count_real_bit = 0;
        p->count_pred_bit = 0;
        p->count_frame = 0;
        rc_dbg_rc("first frame init_quality %d bits_tgt_upper %d "
                  "mb_w %d mb_h %d ratio %d qp_out %d\n",
                  p->usr_cfg.init_quality, p->bits_tgt_upper,
                  mb_w, mb_h, ratio, p->qp_out);
    } else if (p->frame_type == INTRA_FRAME) {
        // if (p->frm_num > 0)
        p->qp_out = derive_iframe_qp_by_bitrate(p, info->bit_max);
    } else {
        if (p->last_frame_type == INTRA_FRAME) {
            RK_S32 prev_qp = p->qp_prev_out;
            p->qp_out = prev_qp + (prev_qp < 33 ? 3 : (prev_qp < 35 ? 2 : 1));
        } else
            p->qp_out = derive_pframe_qp_by_bitrate(p);
    }

    p->qp_out = revise_qp_by_complexity(p, fm_min_iqp, fm_min_pqp, fm_max_iqp, fm_max_pqp);

    info->quality_target = p->qp_out;
    info->complex_scene = 0;
    if (p->frame_type == INTER_P_FRAME && avg_pqp >= fm_max_pqp - 1 &&
        p->qp_out == fm_max_pqp && p->qp_prev_out == fm_max_pqp)
        info->complex_scene = 1;

    info->quality_max = p->usr_cfg.max_quality;
    info->quality_min = p->usr_cfg.min_quality;

    rc_dbg_rc("frame %lld quality [%d : %d : %d] complex_scene %d\n",
              p->frm_num, info->quality_min, info->quality_target,
              info->quality_max, info->complex_scene);

    p->frm_num++;
    p->reenc_cnt = 0;
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET check_super_frame_smt(RcModelV2SmtCtx *ctx, EncRcTaskInfo *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_S32 frame_type = ctx->frame_type;
    RK_U32 bits_thr = 0;
    RcCfg *usr_cfg = &ctx->usr_cfg;

    rc_dbg_func("enter %p\n", ctx);
    if (usr_cfg->super_cfg.super_mode) {
        bits_thr = usr_cfg->super_cfg.super_p_thd;
        if (frame_type == INTRA_FRAME)
            bits_thr = usr_cfg->super_cfg.super_i_thd;

        if ((RK_U32)cfg->bit_real >= bits_thr) {
            if (usr_cfg->super_cfg.super_mode == MPP_ENC_RC_SUPER_FRM_DROP) {
                rc_dbg_rc("super frame drop current frame");
                usr_cfg->drop_mode = MPP_ENC_RC_DROP_FRM_NORMAL;
                usr_cfg->drop_gap  = 0;
            }
            ret = MPP_NOK;
        }
    }
    rc_dbg_func("leave %p\n", ctx);
    return ret;
}

MPP_RET check_re_enc_smt(RcModelV2SmtCtx *ctx, EncRcTaskInfo *cfg)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 frame_type = ctx->frame_type;
    RK_S32 bit_thr = 0;
    RK_S32 stat_time = usr_cfg->stats_time;
    RK_S32 last_ins_bps = mpp_data_sum_v2(ctx->stat_bits) / stat_time;
    RK_S32 ins_bps = (last_ins_bps * stat_time - mpp_data_get_pre_val_v2(ctx->stat_bits, -1)
                      + cfg->bit_real) / stat_time;
    RK_S32 bps;
    RK_S32 ret = MPP_OK;

    rc_dbg_func("enter %p\n", ctx);
    rc_dbg_rc("reenc check target_bps %d last_ins_bps %d ins_bps %d",
              usr_cfg->bps_target, last_ins_bps, ins_bps);

    if (ctx->reenc_cnt >= usr_cfg->max_reencode_times)
        return MPP_OK;

    if (check_super_frame_smt(ctx, cfg))
        return MPP_NOK;

    if (usr_cfg->debreath_cfg.enable && !ctx->first_frm_flg)
        return MPP_OK;

    rc_dbg_drop("drop mode %d frame_type %d\n", usr_cfg->drop_mode, frame_type);
    if (usr_cfg->drop_mode && frame_type == INTER_P_FRAME) {
        bit_thr = (RK_S32)(usr_cfg->bps_max * (100 + usr_cfg->drop_thd) / (float)100);
        rc_dbg_drop("drop mode %d check max_bps %d bit_thr %d ins_bps %d",
                    usr_cfg->drop_mode, usr_cfg->bps_target, bit_thr, ins_bps);
        return (ins_bps > bit_thr) ? MPP_NOK : MPP_OK;
    }

    switch (frame_type) {
    case INTRA_FRAME:
        bit_thr = 3 * cfg->bit_max / 2;
        break;
    case INTER_P_FRAME:
        bit_thr = 3 * cfg->bit_max;
        break;
    default:
        break;
    }

    if (cfg->bit_real > bit_thr) {
        bps = usr_cfg->bps_max;
        if ((bps - (bps >> 3) < ins_bps) && (bps / 20  < ins_bps - last_ins_bps))
            ret =  MPP_NOK;
    }

    rc_dbg_func("leave %p ret %d\n", ctx, ret);
    return ret;
}

MPP_RET rc_model_v2_smt_check_reenc(void *ctx, EncRcTask *task)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx *)ctx;
    EncRcTaskInfo *cfg = (EncRcTaskInfo *)&task->info;
    EncFrmStatus *frm = &task->frm;
    RcCfg *usr_cfg = &p->usr_cfg;

    rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);

    frm->reencode = 0;

    if ((usr_cfg->mode == RC_FIXQP) ||
        (task->force.force_flag & ENC_RC_FORCE_QP) ||
        p->on_drop || p->on_pskip)
        return MPP_OK;

    if (check_re_enc_smt(p, cfg)) {
        MppEncRcDropFrmMode drop_mode = usr_cfg->drop_mode;

        if (frm->is_intra)
            drop_mode = MPP_ENC_RC_DROP_FRM_DISABLED;

        if (usr_cfg->drop_gap && p->drop_cnt >= usr_cfg->drop_gap)
            drop_mode = MPP_ENC_RC_DROP_FRM_DISABLED;

        rc_dbg_drop("reenc drop_mode %d drop_cnt %d\n", drop_mode, p->drop_cnt);

        switch (drop_mode) {
        case MPP_ENC_RC_DROP_FRM_NORMAL : {
            frm->drop = 1;
            frm->reencode = 1;
            p->on_drop = 1;
            p->drop_cnt++;
            rc_dbg_drop("drop\n");
        } break;
        case MPP_ENC_RC_DROP_FRM_PSKIP : {
            frm->force_pskip = 1;
            frm->reencode = 1;
            p->on_pskip = 1;
            p->drop_cnt++;
            rc_dbg_drop("force_pskip\n");
        } break;
        case MPP_ENC_RC_DROP_FRM_DISABLED :
        default : {
            RK_S32 bits_thr = usr_cfg->super_cfg.super_p_thd;
            if (p->frame_type == INTRA_FRAME)
                bits_thr = usr_cfg->super_cfg.super_i_thd;

            if (cfg->bit_real > bits_thr * 2)
                cfg->quality_target += 3;
            else if (cfg->bit_real > bits_thr * 3 / 2)
                cfg->quality_target += 2;
            else if (cfg->bit_real > bits_thr)
                cfg->quality_target ++;

            if (cfg->quality_target < cfg->quality_max) {
                p->reenc_cnt++;
                frm->reencode = 1;
            }
            p->drop_cnt = 0;
        } break;
        }
    }

    return MPP_OK;
}

MPP_RET rc_model_v2_smt_end(void *ctx, EncRcTask * task)
{
    RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
    EncRcTaskInfo *cfg = (EncRcTaskInfo *) & task->info;
    RK_S32 bit_real = cfg->bit_real;

    rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);
    rc_dbg_rc("motion_level %u, complex_level %u\n", cfg->motion_level, cfg->complex_level);

    mpp_data_update_v2(p->rt_bits, bit_real);
    cfg->rt_bits = mpp_data_sum_v2(p->rt_bits);
    rc_dbg_rc("frame %lld real_bit %d real time bits %d\n",
              p->frm_num - 1, bit_real, cfg->rt_bits);

    mpp_data_update_v2(p->motion_level, cfg->motion_level);
    mpp_data_update_v2(p->complex_level, cfg->complex_level);
    p->first_frm_flg = 0;

    if (p->frame_type == INTER_P_FRAME || p->gop_mode == MPP_GOP_ALL_INTRA)
        mpp_data_update(p->qp_p, p->qp_out);
    else {
        p->pre_qp_i = p->qp_out;
        p->pre_real_bit_i = bit_real;
    }

    bits_model_update_smt(p, bit_real);
    p->qp_prev_out = p->qp_out;
    p->last_frame_type = p->frame_type;
    p->pre_iblk4_prop = cfg->iblk4_prop;
    p->gop_frm_cnt++;
    p->gop_qp_sum += p->qp_out;

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_smt_hal_start(void *ctx, EncRcTask *task)
{
    rc_dbg_func("smt_hal_start enter ctx %p task %p\n", ctx, task);
    return MPP_OK;
}

MPP_RET rc_model_v2_smt_hal_end(void *ctx, EncRcTask *task)
{
    rc_dbg_func("smt_hal_end enter ctx %p task %p\n", ctx, task);
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

const RcImplApi smt_h264e = {
    "smart",
    MPP_VIDEO_CodingAVC,
    sizeof(RcModelV2SmtCtx),
    rc_model_v2_smt_h264_init,
    rc_model_v2_smt_deinit,
    NULL,
    rc_model_v2_smt_check_reenc,
    rc_model_v2_smt_start,
    rc_model_v2_smt_end,
    rc_model_v2_smt_hal_start,
    rc_model_v2_smt_hal_end,
};

const RcImplApi smt_h265e = {
    "smart",
    MPP_VIDEO_CodingHEVC,
    sizeof(RcModelV2SmtCtx),
    rc_model_v2_smt_h265_init,
    rc_model_v2_smt_deinit,
    NULL,
    rc_model_v2_smt_check_reenc,
    rc_model_v2_smt_start,
    rc_model_v2_smt_end,
    rc_model_v2_smt_hal_start,
    rc_model_v2_smt_hal_end,
};
