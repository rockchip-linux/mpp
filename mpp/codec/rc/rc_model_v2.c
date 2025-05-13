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

#define MODULE_TAG "rc_model_v2"

#include <math.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_time.h"
#include "rc_debug.h"
#include "rc_ctx.h"
#include "rc_model_v2.h"

#define I_WINDOW_LEN 2
#define P_WINDOW1_LEN 5
#define P_WINDOW2_LEN 8
#define MAX_BIT_COUNT_VALUE 1000000000000000

static const RK_S32 max_i_delta_qp[51] = {
    640, 640, 640, 640, 640, 640, 640, 640, 640, 640, 640, 640, 640, 640,
    576, 576, 512, 512, 448, 448, 384, 384, 320, 320, 320, 256, 256, 256,
    192, 192, 192, 192, 192, 128, 128, 128, 128, 128, 128, 64,  64,  64,
    64,  64,  64,  0,   0,   0,   0,   0,   0,
};

RK_S32 tab_lnx[64] = {
    -1216, -972, -830, -729, -651, -587, -533, -486,
    -445, -408, -374, -344, -316, -290, -265, -243,
    -221, -201, -182, -164, -147, -131, -115, -100,
    -86,  -72,  -59,  -46,  -34,  -22,  -11,    0,
    10,   21,   31,   41,   50,   60,   69,   78,
    86,   95,   87,  103,  111,  119,  127,  134,
    142,  149,  156,  163,  170,  177,  183,  190,
    196,  202,  208,  214,  220,  226,  232,  237,
};

RK_S32 mean_qp2scale[16] = {
    14,  15,  16, 18, 20, 22, 25, 28,
    32,  36,  40, 44, 50, 56, 64, 72
};
const RK_S8 max_ip_qp_dealt[8] = {
    7, 7, 6, 6, 5, 5, 4, 4
};
RK_U32 bit2percent[100] = {
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 95, 90, 85, 80, 75, 72, 69, 66, 64,
    62, 59, 58, 57, 56, 55, 54, 53, 52, 51,
    50, 49, 48, 47, 47, 46, 46, 45, 45, 44,
    44, 43, 43, 43, 42, 42, 42, 41, 41, 41,
    40, 40, 40, 39, 39, 39, 38, 38, 38, 37,
    37, 37, 37, 37, 37, 37, 36, 36, 36, 36,
    36, 36, 36, 35, 35, 35, 34, 34, 34, 34,
    34, 33, 33, 33, 33, 33, 33, 33, 33, 33,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
};

RK_S8 intra_qp_map[8] = {
    0, 0, 1, 1, 2, 2, 2, 2,
};

MPP_RET bits_model_param_deinit(RcModelV2Ctx *ctx)
{
    rc_dbg_func("enter %p\n", ctx);

    if (ctx->i_bit != NULL) {
        mpp_data_deinit_v2(ctx->i_bit);
        ctx->i_bit = NULL;
    }

    if (ctx->p_bit != NULL) {
        mpp_data_deinit_v2(ctx->p_bit);
        ctx->p_bit = NULL;
    }

    if (ctx->vi_bit != NULL) {
        mpp_data_deinit_v2(ctx->vi_bit);
        ctx->vi_bit = NULL;
    }

    if (ctx->pre_p_bit != NULL) {
        mpp_data_deinit_v2(ctx->pre_p_bit);
        ctx->pre_p_bit = NULL;
    }

    if (ctx->pre_i_bit != NULL) {
        mpp_data_deinit_v2(ctx->pre_i_bit);
        ctx->pre_i_bit = NULL;
    }

    if (ctx->pre_i_mean_qp != NULL) {
        mpp_data_deinit_v2(ctx->pre_i_mean_qp);
        ctx->pre_i_mean_qp = NULL;
    }

    if (ctx->madi != NULL) {
        mpp_data_deinit_v2(ctx->madi);
        ctx->madi = NULL;
    }
    if (ctx->madp != NULL) {
        mpp_data_deinit_v2(ctx->madp);
        ctx->madp = NULL;
    }

    if (ctx->stat_rate != NULL) {
        mpp_data_deinit_v2(ctx->stat_rate);
        ctx->stat_rate = NULL;
    }

    if (ctx->stat_bits != NULL) {
        mpp_data_deinit_v2(ctx->stat_bits);
        ctx->stat_bits = NULL;
    }
    if (ctx->gop_bits != NULL) {
        mpp_data_deinit_v2(ctx->gop_bits);
        ctx->gop_bits = NULL;
    }
    if (ctx->motion_level != NULL) {
        mpp_data_deinit_v2(ctx->motion_level);
        ctx->motion_level = NULL;
    }
    if (ctx->complex_level != NULL) {
        mpp_data_deinit_v2(ctx->complex_level);
        ctx->complex_level = NULL;
    }
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_param_init(RcModelV2Ctx *ctx)
{
    RK_S32 gop_len = ctx->usr_cfg.igop;
    RcFpsCfg *fps = &ctx->usr_cfg.fps;
    RK_S32 mad_len = 10;
    RK_U32 stat_len = fps->fps_out_num * ctx->usr_cfg.stats_time / fps->fps_out_denom;
    stat_len = stat_len ? stat_len : 1;

    bits_model_param_deinit(ctx);
    mpp_data_init_v2(&ctx->i_bit, I_WINDOW_LEN, 0);
    if (ctx->i_bit == NULL) {
        mpp_err("i_bit init fail");
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->vi_bit, I_WINDOW_LEN, 0);
    if (ctx->vi_bit == NULL) {
        mpp_err("vi_bit init fail");
        return MPP_ERR_MALLOC;
    }
    mpp_data_init_v2(&ctx->p_bit, P_WINDOW1_LEN, 0);
    if (ctx->p_bit == NULL) {
        mpp_err("p_bit init fail");
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->pre_p_bit, P_WINDOW2_LEN, 0);
    if (ctx->pre_p_bit == NULL) {
        mpp_err("pre_p_bit init fail");
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->pre_i_bit, I_WINDOW_LEN, 0);
    if (ctx->pre_i_bit == NULL) {
        mpp_err("pre_i_bit init fail");
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->pre_i_mean_qp, I_WINDOW_LEN, -1);
    if (ctx->pre_i_mean_qp == NULL) {
        mpp_err("pre_i_mean_qp init fail");
        return MPP_ERR_MALLOC;
    }
    mpp_data_init_v2(&ctx->madi, P_WINDOW2_LEN, 0);
    if (ctx->madi == NULL) {
        mpp_err("madi init fail");
        return MPP_ERR_MALLOC;
    }
    mpp_data_init_v2(&ctx->madp, P_WINDOW2_LEN, 0);
    if (ctx->madp == NULL) {
        mpp_err("madp init fail");
        return MPP_ERR_MALLOC;
    }
    mpp_data_init_v2(&ctx->stat_rate, fps->fps_out_num, 0);
    if (ctx->stat_rate == NULL) {
        mpp_err("stat_rate init fail fps_out_num %d", fps->fps_out_num);
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->stat_bits, stat_len, ctx->bit_per_frame);
    if (ctx->stat_bits == NULL) {
        mpp_err("stat_bits init fail stat_len %d", stat_len);
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->gop_bits, gop_len, 0);
    if (ctx->gop_bits == NULL) {
        mpp_err("gop_bits init fail gop_len %d", gop_len);
        return MPP_ERR_MALLOC;
    }

    if (ctx->usr_cfg.refresh_len) {
        mpp_data_init_v2(&ctx->i_refresh_bit, ctx->usr_cfg.refresh_len, 0);
        if (ctx->i_refresh_bit == NULL) {
            mpp_err("i_refresh_bit init fail refresh_len %d", ctx->usr_cfg.refresh_len);
            return MPP_ERR_MALLOC;
        }
    }

    mpp_data_init_v2(&ctx->motion_level, mad_len, 0);
    if (ctx->motion_level == NULL) {
        mpp_err("motion_level init fail mad_len %d\n", mad_len);
        return MPP_ERR_MALLOC;
    }

    mpp_data_init_v2(&ctx->complex_level, mad_len, 0);
    if (ctx->complex_level == NULL) {
        mpp_err("complex_level init fail mad_len %d\n", mad_len);
        return MPP_ERR_MALLOC;
    }

    return MPP_OK;
}

void bits_frm_init(RcModelV2Ctx *ctx)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_U32 gop_len = usr_cfg->igop;
    RK_U32 p_bit = 0;
    RK_U32 refresh_bit = 0;

    rc_dbg_func("enter %p\n", ctx);

    switch (usr_cfg->gop_mode) {
    case NORMAL_P: {
        ctx->i_scale = ctx->usr_cfg.init_ip_ratio;
        ctx->p_scale = 16;
        if (gop_len <= 1)
            p_bit = ctx->gop_total_bits * 16;
        else
            p_bit = ctx->gop_total_bits * 16 / (ctx->i_scale + ctx->p_scale * (gop_len - 1));
        mpp_data_reset_v2(ctx->p_bit, p_bit);
        ctx->p_sumbits = 5 * p_bit;
        mpp_data_reset_v2(ctx->i_bit, p_bit * ctx->i_scale / 16);
        ctx->i_sumbits = 2 * p_bit * ctx->i_scale / 16;
        if (usr_cfg->refresh_len) {
            ctx->i_refresh_scale = ctx->i_scale / usr_cfg->refresh_len  +  ctx->p_scale;
            refresh_bit = ctx->gop_total_bits *  16 / (ctx->i_refresh_scale * usr_cfg->refresh_len
                                                       + ctx->p_scale * (gop_len - usr_cfg->refresh_len));
            mpp_data_reset_v2(ctx->i_refresh_bit, refresh_bit);
            ctx->i_refresh_sumbits = usr_cfg->refresh_len * refresh_bit;
        }
    } break;
    case SMART_P: {
        RK_U32 vi_num = 0;
        mpp_assert(usr_cfg->vgop > 1);
        ctx->i_scale = 320;
        ctx->p_scale = 16;
        ctx->vi_scale = 32;
        vi_num = gop_len / usr_cfg->vgop;
        if (vi_num > 0) {
            vi_num = vi_num - 1;
        }
        p_bit = ctx->gop_total_bits * 16 / (ctx->i_scale + ctx->vi_scale * vi_num + ctx->p_scale * (gop_len - vi_num));
        mpp_data_reset_v2(ctx->p_bit, p_bit);
        ctx->p_sumbits = 5 * p_bit;

        mpp_data_reset_v2(ctx->i_bit, p_bit * ctx->i_scale / 16);
        ctx->i_sumbits = 2 * p_bit * ctx->i_scale / 16;

        mpp_data_reset_v2(ctx->vi_bit, p_bit * ctx->vi_scale / 16);
        ctx->vi_sumbits = 2 * p_bit * ctx->vi_scale / 16;
    } break;
    default:
        break;
    }
    rc_dbg_rc("p_sumbits %d i_sumbits %d vi_sumbits %d\n", ctx->p_sumbits, ctx->i_sumbits, ctx->vi_sumbits);
    rc_dbg_func("leave %p\n", ctx);
}

RK_S32 moving_judge_update(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg)
{
    switch (cfg->frame_type) {
    case INTRA_FRAME: {
        mpp_data_update_v2(ctx->pre_i_bit, cfg->bit_real);
        mpp_data_update_v2(ctx->pre_i_mean_qp, cfg->quality_real);
    } break;

    case INTER_P_FRAME: {
        mpp_data_update_v2(ctx->pre_p_bit, cfg->bit_real);
        mpp_data_update_v2(ctx->madp, cfg->madp);
    } break;

    default:
        break;
    }
    return 0;
}

void bit_statics_update(RcModelV2Ctx *ctx, RK_U32 real_bit)
{
    RK_S32 mean_pbits, mean_ibits;
    RcCfg  *usr_cfg = &ctx->usr_cfg;
    RK_U32 gop_len = usr_cfg->igop;

    mpp_data_update_v2(ctx->gop_bits, real_bit);
    mean_pbits = mpp_data_mean_v2(ctx->pre_p_bit);
    mean_ibits = mpp_data_mean_v2(ctx->pre_i_bit);
    ctx->real_gbits  = mpp_data_sum_v2(ctx->gop_bits);
    ctx->avg_gbits  = (gop_len - 1) * (RK_S64)mean_pbits + mean_ibits;
}

MPP_RET bits_model_preset(RcModelV2Ctx *ctx, EncRcTaskInfo *info)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 preset_bit = info->bit_target;
    RK_S32 water_level = 0;

    mpp_data_preset_v2(ctx->stat_rate, preset_bit != 0);
    mpp_data_preset_v2(ctx->stat_bits, preset_bit);

    ctx->ins_bps = mpp_data_sum_v2(ctx->stat_bits) / usr_cfg->stats_time;
    if (preset_bit + ctx->stat_watl > ctx->watl_thrd)
        water_level = ctx->watl_thrd - ctx->bit_per_frame;
    else
        water_level = preset_bit + ctx->stat_watl - ctx->bit_per_frame;

    if (water_level < 0) {
        water_level = 0;
    }
    ctx->stat_watl = water_level;
    switch (info->frame_type) {
    case INTRA_FRAME : {
        mpp_data_preset_v2(ctx->i_bit, preset_bit);
        ctx->i_sumbits = mpp_data_sum_v2(ctx->i_bit);
        ctx->i_scale = 80 * ctx->i_sumbits / (2 * ctx->p_sumbits);
        rc_dbg_rc("i_sumbits %d p_sumbits %d i_scale %d\n",
                  ctx->i_sumbits, ctx->p_sumbits, ctx->i_scale);
    } break;
    case INTER_P_FRAME : {
        mpp_data_preset_v2(ctx->p_bit, preset_bit);
        ctx->p_sumbits = mpp_data_sum_v2(ctx->p_bit);

        /* Avoid div zero when P frame successive drop */
        if (!ctx->p_sumbits)
            ctx->p_sumbits = 1;

        ctx->p_scale = 16;
    } break;
    case INTER_VI_FRAME : {
        mpp_data_preset_v2(ctx->vi_bit, preset_bit);
        ctx->vi_sumbits = mpp_data_sum_v2(ctx->vi_bit);
        ctx->vi_scale = 80 * ctx->vi_sumbits / (2 * ctx->p_sumbits);
        /* NOTE: vi_scale may be set to zero. So we should limit the range */
        ctx->vi_scale = mpp_clip(ctx->vi_scale, 16, 320);
    } break;
    case INTRA_RFH_FRAME: {
        mpp_data_update_v2(ctx->i_refresh_bit, preset_bit);
        mpp_data_update_v2(ctx->madi,  info->madi);
        ctx->i_refresh_sumbits = mpp_data_sum_v2(ctx->i_refresh_bit);
        ctx->i_refresh_scale = 80 * ctx->i_refresh_sumbits / (usr_cfg->refresh_len * ctx->p_sumbits);
        ctx->i_refresh_scale = mpp_clip(ctx->i_refresh_scale, 16, 64);
    } break;
    default : {
    } break;
    }
    return MPP_OK;
}

MPP_RET bits_model_update(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 real_bit = cfg->bit_real;
    RK_U32 madi = cfg->madi;
    RK_S32 water_level = 0;

    rc_dbg_func("enter %p\n", ctx);

    mpp_data_update_v2(ctx->stat_rate, real_bit != 0);
    mpp_data_update_v2(ctx->stat_bits, real_bit);

    ctx->ins_bps = mpp_data_sum_v2(ctx->stat_bits) / usr_cfg->stats_time;
    if (real_bit + ctx->stat_watl > ctx->watl_thrd)
        water_level = ctx->watl_thrd - ctx->bit_per_frame;
    else
        water_level = real_bit + ctx->stat_watl - ctx->bit_per_frame;

    if (water_level < 0)
        water_level = 0;

    ctx->stat_watl = water_level;

    switch (cfg->frame_type) {
    case INTRA_FRAME : {
        mpp_data_update_v2(ctx->i_bit, real_bit);
        ctx->i_sumbits = mpp_data_sum_v2(ctx->i_bit);
        ctx->i_scale = 80 * ctx->i_sumbits / (2 * ctx->p_sumbits);
        rc_dbg_rc("i_sumbits %d p_sumbits %d i_scale %d\n",
                  ctx->i_sumbits, ctx->p_sumbits, ctx->i_scale);
    } break;
    case INTER_P_FRAME : {
        mpp_data_update_v2(ctx->p_bit, real_bit);
        mpp_data_update_v2(ctx->madi,  madi);
        ctx->p_sumbits = mpp_data_sum_v2(ctx->p_bit);

        /* Avoid div zero when P frame successive drop */
        if (!ctx->p_sumbits)
            ctx->p_sumbits = 1;

        ctx->p_scale = 16;
    } break;
    case INTER_VI_FRAME: {
        mpp_data_update_v2(ctx->vi_bit, real_bit);
        ctx->vi_sumbits = mpp_data_sum_v2(ctx->vi_bit);
        ctx->vi_scale = 80 * ctx->vi_sumbits / (2 * ctx->p_sumbits);
        /* NOTE: vi_scale may be set to zero. So we should limit the range */
        ctx->vi_scale = mpp_clip(ctx->vi_scale, 16, 320);
    } break;
    case INTRA_RFH_FRAME: {
        mpp_data_update_v2(ctx->i_refresh_bit, real_bit);
        mpp_data_update_v2(ctx->madi,  madi);
        ctx->i_refresh_sumbits = mpp_data_sum_v2(ctx->i_refresh_bit);
        ctx->i_refresh_scale = 80 * ctx->i_refresh_sumbits / (usr_cfg->refresh_len * ctx->p_sumbits);
        ctx->i_refresh_scale = mpp_clip(ctx->i_refresh_scale, 16, 64);
    } break;
    default : {
    } break;
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_alloc(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg, RK_S64 total_bits)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_U32 max_i_prop = usr_cfg->max_i_bit_prop * 16;
    RK_S32 gop_len = usr_cfg->igop;
    RK_S32 i_scale = ctx->i_scale;
    RK_S32 vi_scale = ctx->vi_scale;
    RK_S32 alloc_bits = 0;
    RK_S32 super_bit_thr = 0x7fffffff;
    RK_S64 total_bits_fix = total_bits;

    ctx->i_scale = 80 * ctx->i_sumbits / (2 * ctx->p_sumbits);
    i_scale = ctx->i_scale;

    rc_dbg_func("enter %p\n", ctx);
    rc_dbg_rc("frame_type %d max_i_prop %d i_scale %d total_bits %lld\n",
              cfg->frame_type, max_i_prop, i_scale, total_bits);
    if (usr_cfg->super_cfg.super_mode) {
        super_bit_thr = usr_cfg->super_cfg.super_p_thd;
    }
    if (usr_cfg->gop_mode == SMART_P) {
        RK_U32 vi_num = 0;
        mpp_assert(usr_cfg->vgop > 1);
        vi_num = gop_len / usr_cfg->vgop;
        if (vi_num > 0) {
            vi_num = vi_num - 1;
        }
        switch (cfg->frame_type) {
        case INTRA_FRAME: {
            i_scale = mpp_clip(i_scale, 16, 16000);
            total_bits = total_bits * i_scale;
            if (usr_cfg->super_cfg.super_mode) {
                super_bit_thr = usr_cfg->super_cfg.super_i_thd;
            }
        } break;
        case INTER_P_FRAME: {
            i_scale = mpp_clip(i_scale, 16, max_i_prop);
            total_bits = total_bits * 16;
        } break;
        case INTER_VI_FRAME: {
            i_scale = mpp_clip(i_scale, 16, max_i_prop);
            total_bits = total_bits * vi_scale;
        } break;
        default:
            break;
        }

        alloc_bits = total_bits / (i_scale + 16 * (gop_len - vi_num) + vi_num * vi_scale);

        if (!alloc_bits) {
            mpp_log_f("found zero alloc bits\n");
            mpp_log_f("total_bits %lld, i_scale %d, gop_len %d, vi_num %d, vi_scale %d",
                      total_bits, i_scale, gop_len, vi_num, vi_scale);
            mpp_log_f("gop_total_bits %lld, i_sumbits %d, p_sumbits %d, vgop %d igop %d",
                      ctx->gop_total_bits, ctx->i_sumbits, ctx->p_sumbits, usr_cfg->vgop, usr_cfg->igop);
        }
    } else {
        switch (cfg->frame_type) {
        case INTRA_FRAME: {
            i_scale = mpp_clip(i_scale, 16, 16000);
            total_bits = total_bits * i_scale;
            if (usr_cfg->super_cfg.super_mode) {
                super_bit_thr = usr_cfg->super_cfg.super_i_thd;
            }
        } break;

        case INTER_P_FRAME: {
            i_scale = mpp_clip(i_scale, 16, max_i_prop);
            total_bits = total_bits * 16;
        } break;
        case INTRA_RFH_FRAME: {
            i_scale = mpp_clip(i_scale, 16, max_i_prop);
            total_bits = total_bits * ctx->i_refresh_scale;
            rc_dbg_rc("ctx->i_refresh_scale = %d", ctx->i_refresh_scale);
        } break;
        default:
            break;
        }
        if (gop_len > 1) {
            if (!usr_cfg->refresh_len || cfg->frame_type == INTRA_FRAME)
                alloc_bits = total_bits / (i_scale + 16 * (gop_len - 1));
            else
                alloc_bits = total_bits /
                             (ctx->i_refresh_scale * usr_cfg->refresh_len +
                              16 * (gop_len - usr_cfg->refresh_len));
        } else {
            alloc_bits = total_bits / i_scale;
        }
    }
    rc_dbg_rc("i_scale  %d, total_bits %lld", i_scale, total_bits);
    if (alloc_bits > super_bit_thr &&
        (usr_cfg->super_cfg.rc_priority == MPP_ENC_RC_BY_FRM_SIZE_FIRST)) {
        alloc_bits = super_bit_thr - (super_bit_thr >> 4);
        rc_dbg_rc("alloc bits max then super_bit_thr %d", super_bit_thr);
    }

    ctx->cur_super_thd = super_bit_thr;
    cfg->bit_target = alloc_bits;
    if (cfg->frame_type == INTRA_FRAME)
        cfg->bit_target_fix = total_bits_fix * 240 / (240 + 16 * (gop_len - 1));
    else
        cfg->bit_target_fix = total_bits_fix * 16 / (240 + 16 * (gop_len - 1));

    mpp_assert(alloc_bits);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET calc_next_i_ratio(RcModelV2Ctx *ctx)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 max_i_prop = usr_cfg->max_i_bit_prop * 16;
    RK_S32 gop_len    = usr_cfg->igop;
    RK_S32 pre_qp     = ctx->pre_i_qp;
    RK_S32 bits_alloc;

    rc_dbg_func("enter %p\n", ctx);
    if (gop_len > 1) {
        bits_alloc = ctx->gop_total_bits * max_i_prop / (max_i_prop + 16 * (gop_len - 1));
    } else {
        bits_alloc = ctx->gop_total_bits * max_i_prop / max_i_prop;
    }

    if (ctx->pre_real_bits > bits_alloc || ctx->next_i_ratio) {
        RK_S32 ratio = ((ctx->pre_real_bits - bits_alloc) << 8) / bits_alloc;

        ratio = mpp_clip(ratio, -256, 256);
        ratio = ctx->next_i_ratio + ratio;
        if (ratio >= 0) {
            if (ratio > max_i_delta_qp[pre_qp])
                ratio = max_i_delta_qp[pre_qp];
        } else {
            ratio = 0;
        }
        ctx->next_i_ratio = ratio;
        rc_dbg_rc("ctx->next_i_ratio %d", ctx->next_i_ratio);
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;

}

MPP_RET calc_debreath_qp(RcModelV2Ctx *ctx)
{
    rc_dbg_func("enter %p\n", ctx);
    RcDebreathCfg *debreath_cfg = &ctx->usr_cfg.debreath_cfg;
    RK_S32 qp_start_sum = 0;
    RK_S32 new_start_qp = 0;
    RK_U8 idx2 = MPP_MIN(ctx->pre_iblk4_prop >> 5, (RK_S32)sizeof(intra_qp_map) - 1);

    static RK_S8 strength_map[36] = {
        0, 1, 1, 2,  2,  2,  3,  3,  3,  4,  4,  4,
        5, 5, 5, 6,  6,  6,  7,  7,  7,  8,  8,  8,
        9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12
    };

    qp_start_sum = MPP_MIN(ctx->gop_qp_sum / ctx->gop_frm_cnt, (RK_S32)sizeof(strength_map) - 1);

    rc_dbg_qp("i start_qp %d, qp_start_sum = %d, intra_lv4_prop %d",
              ctx->start_qp, qp_start_sum, ctx->pre_iblk4_prop);

    RK_S32 dealt_qp = strength_map[debreath_cfg->strength] - intra_qp_map[idx2];
    if (qp_start_sum > dealt_qp)
        new_start_qp = qp_start_sum - dealt_qp;
    else
        new_start_qp = qp_start_sum;

    ctx->start_qp = mpp_clip(new_start_qp, ctx->usr_cfg.min_i_quality, ctx->usr_cfg.max_i_quality);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET calc_cbr_ratio(void *ctx, EncRcTaskInfo *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    RK_S32 target_bps = p->target_bps;
    RK_S32 ins_bps = p->ins_bps;
    RK_S32 pre_target_bits = p->pre_target_bits;
    RK_S32 pre_real_bits = p->pre_real_bits;
    RK_S32 pre_ins_bps = p->last_inst_bps;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio, wl_ratio;
    RK_S32 fluc_l = 3;

    rc_dbg_func("enter %p\n", p);

    rc_dbg_bps("%10s|%10s|%10s|%10s|%10s|%8s", "r_bits", "t_bits", "ins_bps", "p_ins_bps", "target_bps", "watl");
    rc_dbg_bps("%10d %10d %10d %10d %10d %8d", pre_real_bits, pre_target_bits, ins_bps, pre_ins_bps, target_bps, p->stat_watl >> 10);

    bits_model_alloc(p, cfg, p->gop_total_bits);

    mpp_assert(target_bps > 0);

    p->pre_real_bits_count = p->pre_real_bits_count + pre_real_bits;
    p->pre_target_bits_fix_count = p->pre_target_bits_fix_count + p->pre_target_bits_fix;
    if (p->pre_real_bits_count > MAX_BIT_COUNT_VALUE || p->pre_target_bits_fix_count > MAX_BIT_COUNT_VALUE) {
        p->pre_real_bits_count = 0;
        p->pre_target_bits_fix_count = 0;
    }

    if (pre_target_bits > pre_real_bits)
        bit_diff_ratio = 52 * (pre_real_bits - pre_target_bits) / pre_target_bits;
    else
        bit_diff_ratio = 64 * (pre_real_bits - pre_target_bits) / pre_target_bits;

    idx1 = (ins_bps << 5) / target_bps;
    idx2 = (pre_ins_bps << 5) / target_bps;

    idx1 = mpp_clip(idx1, 0, 63);
    idx2 = mpp_clip(idx2, 0, 63);
    ins_ratio = tab_lnx[idx1] - tab_lnx[idx2]; // %3

    /*ins_bps is increase and pre_ins > target_bps*15/16 will raise up ins_ratio to decrease bit
     *ins_bps is decresase and pre_ins < target_bps*17/16 will decrease ins_ratio to increase bit
     */

    if (ins_bps > pre_ins_bps && target_bps - pre_ins_bps < (target_bps >> 4)) { // %6
        ins_ratio = 6 * ins_ratio;
    } else if ( ins_bps < pre_ins_bps && pre_ins_bps - target_bps < (target_bps >> 4)) {
        ins_ratio = 4 * ins_ratio;
    } else {
        ins_ratio = 0;
    }

    bit_diff_ratio = mpp_clip(bit_diff_ratio, -128, 256);

    ins_ratio = mpp_clip(ins_ratio, -128, 256);

    bps_ratio = (ins_bps - target_bps) * fluc_l / (target_bps >> 4);
    wl_ratio = 4 * (p->stat_watl - p->watl_base) * fluc_l / p->watl_base;
    bps_ratio = mpp_clip(bps_ratio, -32, 32);
    wl_ratio  = mpp_clip(wl_ratio, -16, 32);
    p->next_ratio = bit_diff_ratio + ins_ratio + bps_ratio + wl_ratio;

    rc_dbg_qp("%10s|%10s|%10s|%10s|%10s|%10s", "diff_ratio", "ins_ratio", "bps_ratio",
              "wl_ratio", "next_ratio", "cur_qp_s");
    rc_dbg_qp("%10d %10d %10d %10d %10d|%10d", bit_diff_ratio, ins_ratio - bit_diff_ratio,
              bps_ratio, wl_ratio, p->next_ratio, p->cur_scale_qp);

    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

MPP_RET reenc_calc_super_frm_ratio(void *ctx, EncRcTaskInfo *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;

    rc_dbg_func("enter %p\n", p);
    p->next_ratio = 160 * (4 * (cfg->bit_real - p->cur_super_thd) / cfg->bit_target);
    p->next_ratio = mpp_clip(p->next_ratio, 128, 640);
    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

MPP_RET reenc_calc_cbr_ratio(void *ctx, EncRcTaskInfo *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    RcCfg *usr_cfg = &p->usr_cfg;
    RK_S32 stat_time = usr_cfg->stats_time;
    RK_S32 pre_ins_bps = mpp_data_sum_v2(p->stat_bits) / stat_time;
    RK_S32 ins_bps = (pre_ins_bps * stat_time - mpp_data_get_pre_val_v2(p->stat_bits, -1) + cfg->bit_real) / stat_time;
    RK_S32 real_bit = cfg->bit_real;
    RK_S32 target_bit = cfg->bit_target;
    RK_S32 target_bps = p->target_bps;
    RK_S32 water_level = 0;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio, wl_ratio;
    RK_S32 mb_w = MPP_ALIGN(usr_cfg->width, 16) / 16;
    RK_S32 mb_h = MPP_ALIGN(usr_cfg->height, 16) / 16;

    rc_dbg_func("enter %p\n", p);

    if (p->cur_super_thd <= cfg->bit_real &&
        usr_cfg->super_cfg.rc_priority == MPP_ENC_RC_BY_FRM_SIZE_FIRST) {
        return reenc_calc_super_frm_ratio(ctx, cfg);
    }

    if (real_bit + p->stat_watl > p->watl_thrd)
        water_level = p->watl_thrd - p->bit_per_frame;
    else
        water_level = real_bit + p->stat_watl - p->bit_per_frame;

    if (water_level < 0) {
        water_level = 0;
    }

    if (target_bit > real_bit)
        bit_diff_ratio = 32 * (real_bit - target_bit) / real_bit;
    else
        bit_diff_ratio = 32 * (real_bit - target_bit) / target_bit;

    idx1 = ins_bps / (target_bps >> 5);
    idx2 = pre_ins_bps / (target_bps >> 5);

    idx1 = mpp_clip(idx1, 0, 63);
    idx2 = mpp_clip(idx2, 0, 63);
    ins_ratio = tab_lnx[idx1] - tab_lnx[idx2];

    bps_ratio = 96 * (ins_bps - target_bps) / target_bps;
    wl_ratio = 32 * (water_level - p->watl_base) /  p->watl_base;
    if (pre_ins_bps < ins_bps && target_bps != pre_ins_bps) {
        ins_ratio = 6 * ins_ratio;
        ins_ratio = mpp_clip(ins_ratio, -192, 256);
    } else {
        if (cfg->frame_type == INTRA_FRAME) {
            ins_ratio = 3 * ins_ratio;
            ins_ratio = mpp_clip(ins_ratio, -192, 256);
        } else {
            ins_ratio = 0;
        }
    }

    bit_diff_ratio = mpp_clip(bit_diff_ratio, -128, 256);
    bps_ratio = mpp_clip(bps_ratio, -32, 32);
    wl_ratio  = mpp_clip(wl_ratio, -32, 32);
    p->next_ratio = bit_diff_ratio + ins_ratio + bps_ratio + wl_ratio;
    if (cfg->frame_type  == INTRA_FRAME && (cfg->madi > 0)) {
        RK_U32 tar_bpp = target_bit / (mb_w * mb_h);
        float lnb_t = log(tar_bpp);
        float c = 6.7204, a = -0.1435, b = 0.0438;
        float start_qp = (p->cur_scale_qp >> 6);
        int qp_c = ((lnb_t - cfg->madi * b - c) / a + 14);
        if (qp_c > start_qp)
            p->next_ratio  = (qp_c << 6) - p->cur_scale_qp;
    }
    rc_dbg_rc("cbr target_bit %d real_bit %d reenc next ratio %d", target_bit, real_bit, p->next_ratio);
    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

void rc_hier_calc_dealt_qp(RcModelV2Ctx *p, EncRcTaskInfo *info)
{
    RcHierQPCfg *hier_qp_cfg = &p->usr_cfg.hier_qp_cfg;

    if (!hier_qp_cfg->hier_qp_en)
        return;

    if (info->frame_type == INTRA_FRAME || info->frame_type == INTER_VI_FRAME) {
        p->hier_frm_cnt[3] = 0;
        p->hier_frm_cnt[2] = 0;
        p->hier_frm_cnt[1] = 0;
        p->hier_frm_cnt[0] = 0;
        p->qp_layer_id = 0;
    } else {
        if (p->hier_frm_cnt[3] < hier_qp_cfg->hier_frame_num[3]) {
            p->hier_frm_cnt[3]++;
            p->qp_layer_id = 4;
        } else if (p->hier_frm_cnt[2] < hier_qp_cfg->hier_frame_num[2]) {
            p->hier_frm_cnt[2]++;
            p->hier_frm_cnt[3] = 0;
            p->qp_layer_id = 3;
        } else if (p->hier_frm_cnt[1] < hier_qp_cfg->hier_frame_num[1]) {
            p->hier_frm_cnt[1]++;
            p->hier_frm_cnt[3] = 0;
            p->hier_frm_cnt[2] = 0;
            p->qp_layer_id = 2;
        } else if (p->hier_frm_cnt[0] < hier_qp_cfg->hier_frame_num[0]) {
            p->hier_frm_cnt[0]++;
            p->hier_frm_cnt[3] = 0;
            p->hier_frm_cnt[2] = 0;
            p->hier_frm_cnt[1] = 0;
            p->qp_layer_id = 1;
        } else {
            p->hier_frm_cnt[3] = 0;
            p->hier_frm_cnt[2] = 0;
            p->hier_frm_cnt[1] = 0;
            p->hier_frm_cnt[0] = 0;
            p->qp_layer_id = 0;
        }
    }

    return;
}

MPP_RET calc_vbr_ratio(void *ctx, EncRcTaskInfo *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    RcCfg *usr_cfg = &p->usr_cfg;
    RK_S32 bps_change = p->target_bps;
    RK_S32 max_bps_target = usr_cfg->bps_max;
    RK_S32 ins_bps = p->ins_bps;
    RK_S32 pre_target_bits = p->pre_target_bits;
    RK_S32 pre_real_bits = p->pre_real_bits;
    RK_S32 pre_ins_bps = p->last_inst_bps;

    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio;

    rc_dbg_func("enter %p\n", p);

    bits_model_alloc(p, cfg, p->gop_total_bits);
    if (pre_target_bits > pre_real_bits)
        bit_diff_ratio = 32 * (pre_real_bits - pre_target_bits) / pre_target_bits;
    else
        bit_diff_ratio = 64 * (pre_real_bits - pre_target_bits) / pre_target_bits;

    idx1 = ins_bps / (max_bps_target >> 5);
    idx2 = pre_ins_bps / (max_bps_target >> 5);

    idx1 = mpp_clip(idx1, 0, 63);
    idx2 = mpp_clip(idx2, 0, 63);
    ins_ratio = tab_lnx[idx1] - tab_lnx[idx2];

    rc_dbg_bps("%10s|%10s|%10s|%10s|%10s|%10s", "r_bits", "t_bits", "ins_bps", "p_ins_bps",
               "bps_ch", "max_bps");
    rc_dbg_bps("%10d %10d %10d %10d %10d %10d", pre_real_bits, pre_target_bits, ins_bps,
               pre_ins_bps, bps_change, max_bps_target);


    if (ins_bps > bps_change && ins_bps > pre_ins_bps) {
        ins_ratio = 6 * ins_ratio;
    } else if (ins_bps <= pre_ins_bps && bps_change > pre_ins_bps) {
        ins_ratio = 3 * ins_ratio;
    } else {
        ins_ratio = 0;
    }

    bit_diff_ratio = mpp_clip(bit_diff_ratio, -128, 256);
    ins_ratio = mpp_clip(ins_ratio, -128, 256);
    bps_ratio = 3 * (ins_bps - bps_change) / (max_bps_target >> 4);
    bps_ratio = mpp_clip(bps_ratio, -16, 32);
    if (p->i_scale > 640) {
        bit_diff_ratio = mpp_clip(bit_diff_ratio, -16, 32);
        ins_ratio = mpp_clip(ins_ratio, -16, 32);
    }

    p->next_ratio = bit_diff_ratio + ins_ratio + bps_ratio;

    rc_dbg_qp("%10s|%10s|%10s|%10s|%10s", "diff_ratio", "ins_ratio", "bps_ratio", "next_ratio", "cur_qp_s");
    rc_dbg_qp("%10d %10d %10d %10d|%10d", bit_diff_ratio, ins_ratio, bps_ratio, p->next_ratio, p->cur_scale_qp);
    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

MPP_RET reenc_calc_vbr_ratio(void *ctx, EncRcTaskInfo *cfg)
{

    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    RcCfg *usr_cfg = &p->usr_cfg;
    RK_S32 stat_time = usr_cfg->stats_time;
    RK_S32 pre_ins_bps = mpp_data_sum_v2(p->stat_bits) / stat_time;
    RK_S32 ins_bps = (pre_ins_bps * stat_time - mpp_data_get_pre_val_v2(p->stat_bits, -1) + cfg->bit_real) / stat_time;
    RK_S32 bps_change = p->target_bps;
    RK_S32 max_bps_target = usr_cfg->bps_max;

    RK_S32 real_bit = cfg->bit_real;
    RK_S32 target_bit = cfg->bit_target;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio;

    rc_dbg_func("enter %p\n", p);

    if (p->cur_super_thd <= cfg->bit_real &&
        usr_cfg->super_cfg.rc_priority == MPP_ENC_RC_BY_FRM_SIZE_FIRST) {
        return reenc_calc_super_frm_ratio(ctx, cfg);
    }

    if (target_bit <= real_bit)
        bit_diff_ratio = 32 * (real_bit - target_bit) / target_bit;
    else
        bit_diff_ratio = 32 * (real_bit - target_bit) / real_bit;

    idx1 = ins_bps / (max_bps_target >> 5);
    idx2 = pre_ins_bps / (max_bps_target >> 5);
    idx1 = mpp_clip(idx1, 0, 63);
    idx2 = mpp_clip(idx2, 0, 63);
    if (pre_ins_bps < ins_bps && bps_change < ins_bps) {
        ins_ratio = 6 * (tab_lnx[idx1] - tab_lnx[idx2]);
        ins_ratio = mpp_clip(ins_ratio, -192, 256);
    } else {
        ins_ratio = 0;
    }

    bps_ratio = 96 * (ins_bps - bps_change) / bps_change;
    bit_diff_ratio = mpp_clip(bit_diff_ratio, -128, 256);
    bps_ratio = mpp_clip(bps_ratio, -32, 32);

    p->next_ratio = bit_diff_ratio + ins_ratio + bps_ratio;
    rc_dbg_rc("vbr reenc next ratio %d", p->next_ratio);
    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

RK_S32 moving_ratio_calc(RcModelV2Ctx *ctx)
{
    RK_S32 motion_sensitivity = ctx->motion_sensitivity;
    RK_S32 scale = 0, i;
    RK_S32 total_bit = 0, pbit_sum = 0;
    RK_S32 madi_sum = 0, madp_sum = 0;
    RK_S32 percent = 0;

    for (i = 0; i < 2; i++) {
        RK_S32 pre_I_bit = mpp_data_get_pre_val_v2(ctx->pre_i_bit, i);
        RK_S32 pre_mean_qp = mpp_data_get_pre_val_v2(ctx->pre_i_mean_qp, i);
        if (pre_mean_qp == -1) {
            scale = 32;
        } else {
            RK_S32 index = pre_mean_qp + 8 - ctx->pre_mean_qp;
            if (index >= 0) {
                index = mpp_clip(index, 0 , 15);
                scale = mean_qp2scale[index];
            } else {
                scale = 14;
            }
        }
        total_bit += (scale * pre_I_bit >> 5);
        rc_dbg_rc("pre_mean_qp = %d, ctx->pre_mean_qp %d", pre_mean_qp, ctx->pre_mean_qp);
        rc_dbg_rc("scale = %d, pre_I_bit %d", scale, pre_I_bit);
    }

    pbit_sum = mpp_data_sum_v2(ctx->pre_p_bit);
    madi_sum = mpp_data_sum_v2(ctx->madi);
    madp_sum = mpp_data_sum_v2(ctx->madp);
    rc_dbg_rc("pbit_sum %d,madi_sum = %d, madp_sum = %d", pbit_sum, madi_sum, madp_sum);
    if ( pbit_sum == 0 || total_bit == 0) {
        percent = 255;
    } else {
        RK_S32 index = (total_bit << 6) / pbit_sum;
        index = mpp_clip(index >> 4, 1, 99);
        percent = (bit2percent[index] << 8) / 100;
    }
    rc_dbg_rc("means qp percent %d min_still_percent %d", percent, ctx->min_still_percent);
    RK_S32 percent_a = (ctx->min_still_percent - 30) << 8;
    RK_S32 percent_b = 100 - ctx->min_still_percent;

    percent = (percent_a + percent * percent_b) / 70;
    rc_dbg_rc("percent_a = %d percent_b %d", percent_a, percent_b);
    RK_S32 mv_percnt = (ctx->prev_md_prop * 100) >> 8;
    RK_S32 mv_ratio = (percent_a + 7680 + mv_percnt * percent_b) / 100;
    rc_dbg_rc("mv_ratio = %d", mv_ratio);
    RK_S32 mad_ratio = 256;
    if (madi_sum) {
        mad_ratio = 20 * madp_sum / madi_sum;
        mad_ratio = mpp_clip(mad_ratio, 5, 100);
        rc_dbg_rc("mad_ratio = %d", mad_ratio);
        mad_ratio = (mad_ratio << 8) / 100;
    }
    mad_ratio = (percent_a + 7680 + percent_b * mad_ratio) / 100;

    RK_S32 moving_ratio = (percent + 1 + (mv_ratio * motion_sensitivity + (100 - motion_sensitivity) * mad_ratio) / 100) >> 1;
    rc_dbg_rc("moving_ratio = %d, motion_sensitivity = %d", moving_ratio, motion_sensitivity);
    rc_dbg_rc("percent %d mad_ratio %d hr_ratio %d, moving_ratio %d", percent, mad_ratio, mv_ratio, moving_ratio);
    return moving_ratio;
}

MPP_RET calc_avbr_ratio(void *ctx, EncRcTaskInfo *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    RcCfg *usr_cfg = &p->usr_cfg;
    RK_S32 bps_change = p->target_bps;
    RK_S32 max_bps_target = usr_cfg->bps_max;
    RK_S32 ins_bps = p->ins_bps;
    RK_S32 pre_target_bits = p->pre_target_bits;
    RK_S32 pre_real_bits = p->pre_real_bits;
    RK_S32 pre_ins_bps = p->last_inst_bps;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, agop_dratio, rgop_dratio;
    RK_S32 moving_ratio = 0, moving_percent;
    RK_S32 gop_bits = 0, gop_kbits = 0;
    RK_S32 i_ratio, max_bps;
    RK_S32 qratio, final_qratio;

    rc_dbg_func("enter %p\n", p);

    moving_ratio = moving_ratio_calc(p);
    if (p->moving_ratio - 2 >= moving_ratio)
        moving_ratio = p->moving_ratio - 2;

    if (p->moving_ratio > moving_ratio && (p->max_still_qp << 6) <= p->scale_qp) {
        p->moving_ratio = mpp_clip(p->moving_ratio + 1, 1, 255);
    } else {
        p->moving_ratio = moving_ratio;
    }
    rc_dbg_rc("final moving_ratio = %d", moving_ratio);

    if (moving_ratio <= 0)
        moving_ratio = 1;

    gop_bits = moving_ratio * p->gop_total_bits >> 8;
    gop_kbits = gop_bits >> 10;
    if (gop_kbits < 1)
        gop_kbits = 1;

    bits_model_alloc(p, cfg, gop_bits);
    bps_change = moving_ratio * bps_change >> 8;
    if (moving_ratio < 0) {
        moving_ratio += 255;
    }
    moving_percent = 100 * moving_ratio >> 8;

    rc_dbg_bps("%10s|%10s|%10s|%10s|%10s", "m_ratio", "r_bits", "t_bits", "ins_bps", "p_ins_bps");
    rc_dbg_bps("%10d %10d %10d %10d %10d", p->moving_ratio, pre_real_bits, pre_target_bits, ins_bps, pre_ins_bps);

    if (pre_target_bits > pre_real_bits)
        bit_diff_ratio = 32 * (pre_real_bits - pre_target_bits) / pre_target_bits;
    else
        bit_diff_ratio = 64 * (pre_real_bits - pre_target_bits) / pre_target_bits * moving_percent;

    i_ratio = mpp_clip(p->pre_i_scale >> 4, 10, 200);
    idx1 = ins_bps / (max_bps_target >> 5);
    idx2 = pre_ins_bps / (max_bps_target >> 5);
    idx1 = mpp_clip(idx1, 0, 63);
    idx2 = mpp_clip(idx2, 0, 63);
    ins_ratio = tab_lnx[idx2] - tab_lnx[idx1];
    max_bps = bps_change;

    if (max_bps < pre_ins_bps) {
        max_bps = pre_ins_bps;
    }

    if (ins_bps >  max_bps) {
        ins_ratio = 6 * ins_ratio;
    } else  if (ins_bps < pre_ins_bps && bps_change > pre_ins_bps) {
        ins_ratio = 3 * ins_ratio;
    } else {
        ins_ratio = 0;
    }

    ins_ratio = mpp_clip(ins_ratio >> 2, -128, 256);
    bit_diff_ratio = mpp_clip(10 * bit_diff_ratio / i_ratio, -128, 256);
    rgop_dratio  = mpp_clip(24 * ((p->real_gbits - gop_bits) >> 10) / gop_kbits, -1, 1);
    agop_dratio  = mpp_clip(48 * ((p->avg_gbits - gop_bits) >> 10) / gop_kbits , -1, 1);
    if (p->pre_i_scale > 640) {
        bit_diff_ratio = mpp_clip(bit_diff_ratio, -16, 32);
        ins_ratio = mpp_clip(ins_ratio, -16, 32);
    }
    qratio = 0;
    final_qratio = ins_ratio + bit_diff_ratio + agop_dratio + rgop_dratio;
    if (max_bps_target >= ins_bps) {
        if (final_qratio > 0) {
            if (p->scale_qp >= (p->max_still_qp << 6)) {
                final_qratio = ins_ratio + agop_dratio + rgop_dratio;
                qratio = -6;
            }
        }
    }
    p->next_ratio = qratio + final_qratio;

    rc_dbg_qp("%10s|%10s|%10s|%10s|%10s|%10s|%10s", "diff_ratio", "ins_ratio", "rg_ratio",
              "ag_ratio", "qratio", "next_ratio", "cur_qp_s");
    rc_dbg_qp("%10d %10d %10d %10d %10d %10d %10d", bit_diff_ratio, ins_ratio, rgop_dratio,
              agop_dratio, qratio, p->next_ratio, p->cur_scale_qp);
    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

MPP_RET bits_model_init(RcModelV2Ctx *ctx)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 gop_len = ctx->usr_cfg.igop;
    RcFpsCfg *fps = &ctx->usr_cfg.fps;
    RK_S64 gop_bits = 0;
    RK_U32 target_bps = 0;

    rc_dbg_func("enter %p\n", ctx);

    usr_cfg->min_i_bit_prop = mpp_clip(usr_cfg->min_i_bit_prop, 10, 100);
    usr_cfg->max_i_bit_prop = mpp_clip(usr_cfg->max_i_bit_prop,
                                       usr_cfg->min_i_bit_prop, 100);
    usr_cfg->init_ip_ratio  = mpp_clip(usr_cfg->init_ip_ratio, 160, 640);

    rc_dbg_rc("min_i_bit_prop %d max_i_bit_prop %d, init_ip_ratio %d",
              usr_cfg->min_i_bit_prop, usr_cfg->max_i_bit_prop, usr_cfg->init_ip_ratio);

    if (!gop_len) {
        rc_dbg_rc("infinte gop, set default for rc bit calc\n");
        usr_cfg->igop = gop_len = 500;
    } else if (gop_len == 1) {
        rc_dbg_rc("all intra gop \n");
        usr_cfg->init_ip_ratio  = 16;

        /* disable debreath on all intra case */
        if (usr_cfg->debreath_cfg.enable)
            usr_cfg->debreath_cfg.enable = 0;

        usr_cfg->igop = gop_len = 500;
    } else {
        usr_cfg->igop = gop_len = mpp_clip(usr_cfg->igop, usr_cfg->igop, 500);
    }
    ctx->max_still_qp = 35;
    ctx->motion_sensitivity = 90;

    ctx->first_frm_flg = 1;
    ctx->gop_frm_cnt = 0;
    ctx->gop_qp_sum = 0;

    if (!usr_cfg->fps_chg_prop) {
        usr_cfg->fps_chg_prop = 25;
    }

    target_bps = ctx->usr_cfg.bps_max;
    ctx->re_calc_ratio = reenc_calc_vbr_ratio;
    switch (ctx->usr_cfg.mode) {
    case RC_CBR: {
        ctx->calc_ratio = calc_cbr_ratio;
        ctx->re_calc_ratio = reenc_calc_cbr_ratio;
        target_bps = ctx->usr_cfg.bps_target;
    } break;
    case RC_VBR: {
        ctx->calc_ratio = calc_vbr_ratio;
    } break;
    case RC_FIXQP: {
        return MPP_OK;
    }
    case RC_AVBR: {
        ctx->calc_ratio = calc_avbr_ratio;
        if (usr_cfg->bps_min && usr_cfg->bps_max) {
            ctx->min_still_percent = (RK_S64)usr_cfg->bps_min * 100 / usr_cfg->bps_max;
        } else {
            ctx->min_still_percent = 25;
        }
        rc_dbg_rc("min_still_percent  %d", ctx->min_still_percent);
    } break;
    case RC_SMT: {
        mpp_log("rc mode is smart");
    } break;
    case RC_SE: {
        mpp_log("rc mode is super-encode");
    } break;
    default:
        mpp_log("rc mode set error");
        break;
    }

    ctx->target_bps = ctx->usr_cfg.bps_target;

    if (gop_len >= 1)
        gop_bits = (RK_S64)gop_len * target_bps * fps->fps_out_denom;
    else
        gop_bits = (RK_S64)fps->fps_out_num * target_bps * fps->fps_out_denom;

    ctx->gop_total_bits = gop_bits / fps->fps_out_num;
    ctx->bit_per_frame = target_bps * fps->fps_out_denom / fps->fps_out_num;
    ctx->watl_thrd = 3 * target_bps;
    ctx->stat_watl = ctx->watl_thrd  >> 3;
    ctx->watl_base = ctx->stat_watl;
    ctx->last_fps = fps->fps_out_num / fps->fps_out_denom;

    rc_dbg_rc("gop %d total bit %lld per_frame %d statistics time %d second\n",
              ctx->usr_cfg.igop, ctx->gop_total_bits, ctx->bit_per_frame,
              ctx->usr_cfg.stats_time);

    if (bits_model_param_init(ctx)) {
        return MPP_NOK;
    }
    ctx->time_base = mpp_time();
    bits_frm_init(ctx);
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

static MPP_RET update_mode_param(RcModelV2Ctx *ctx, RK_U32 fps)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 gop_len = ctx->usr_cfg.igop;
    RK_S64 gop_bits = 0;
    RK_U32 target_bps;
    RK_U32  stat_len = fps * usr_cfg->stats_time;

    target_bps = ctx->usr_cfg.bps_max;
    if (ctx->usr_cfg.mode == RC_CBR) {
        target_bps = ctx->usr_cfg.bps_target;
    }

    if (gop_len >= 1)
        gop_bits = (RK_S64)gop_len * target_bps;
    else
        gop_bits = (RK_S64)target_bps;

    ctx->gop_total_bits = gop_bits / fps;

    ctx->bit_per_frame = target_bps / fps;

    if (ctx->stat_bits != NULL) {
        mpp_data_deinit_v2(ctx->stat_bits);
        ctx->stat_bits = NULL;
    }
    mpp_data_init_v2(&ctx->stat_bits, stat_len , ctx->bit_per_frame);
    if (ctx->stat_bits == NULL) {
        mpp_err("stat_bits init fail stat_len %d", stat_len);
        return MPP_ERR_MALLOC;
    }
    return MPP_OK;
}

static MPP_RET fps_chg_update_mode(RcModelV2Ctx *ctx)
{
    ctx->time_end = mpp_time();
    ctx->frm_cnt++;

    if (ctx->time_base && ctx->time_end &&
        ((ctx->time_end - ctx->time_base) >= (RK_S64)(250 * 1000))) {

        RK_S32 time_diff = ((RK_S32)(ctx->time_end - ctx->time_base) / 1000);
        RK_U32 fps = ctx->frm_cnt * 1000 / time_diff;

        if (ctx->last_fps > 0 && fps &&
            abs(ctx->last_fps - (RK_S32)fps) * 100 / ctx->last_fps > (RK_S32)ctx->usr_cfg.fps_chg_prop) {
            update_mode_param(ctx, fps);
            mpp_log("fps chg from %d -> %d", ctx->last_fps, fps);
            ctx->last_fps = fps;
        }
        ctx->time_base = ctx->time_end;
        ctx->frm_cnt = 0;
    }

    return MPP_OK;
}

MPP_RET bits_mode_reset(RcModelV2Ctx *ctx)
{
    rc_dbg_func("enter %p\n", ctx);
    (void) ctx;
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET check_super_frame(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_S32 frame_type = cfg->frame_type;
    RK_U32 bits_thr = 0;
    RcCfg *usr_cfg = &ctx->usr_cfg;

    rc_dbg_func("enter %p\n", ctx);
    if (usr_cfg->super_cfg.super_mode) {
        bits_thr = usr_cfg->super_cfg.super_p_thd;
        if (frame_type == INTRA_FRAME) {
            bits_thr = usr_cfg->super_cfg.super_i_thd;
        }

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

MPP_RET check_re_enc(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RK_S32 frame_type = cfg->frame_type;
    RK_S32 bit_thr = 0;
    RK_S32 stat_time = usr_cfg->stats_time;
    RK_S32 last_ins_bps = mpp_data_sum_v2(ctx->stat_bits) / stat_time;
    RK_S32 ins_bps = (last_ins_bps * stat_time - mpp_data_get_pre_val_v2(ctx->stat_bits, -1)
                      + cfg->bit_real) / stat_time;
    RK_S32 target_bps;
    RK_S32 ret = MPP_OK;

    rc_dbg_func("enter %p\n", ctx);
    rc_dbg_rc("reenc check target_bps %d last_ins_bps %d ins_bps %d",
              usr_cfg->bps_target, last_ins_bps, ins_bps);

    if (ctx->reenc_cnt >= usr_cfg->max_reencode_times)
        return MPP_OK;

    if (check_super_frame(ctx, cfg))
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
        bit_thr = 3 * cfg->bit_target / 2;
        break;
    case INTER_P_FRAME:
        bit_thr = 3 * cfg->bit_target;
        break;
    default:
        break;
    }

    if (cfg->bit_real > bit_thr) {
        if (usr_cfg->mode == RC_CBR) {
            target_bps = usr_cfg->bps_target;
            if (target_bps / 20 < ins_bps - last_ins_bps &&
                (target_bps + target_bps / 10 < ins_bps
                 || target_bps - target_bps / 10 > ins_bps)) {
                ret =  MPP_NOK;
            }
        } else {
            target_bps = usr_cfg->bps_max;
            if ((target_bps - (target_bps >> 3) < ins_bps) &&
                (target_bps / 20  < ins_bps - last_ins_bps)) {
                ret =  MPP_NOK;
            }
        }
    }

    rc_dbg_func("leave %p ret %d\n", ctx, ret);
    return ret;
}


MPP_RET rc_model_v2_init(void *ctx, RcCfg *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx*)ctx;

    rc_dbg_func("enter %p\n", ctx);

    memcpy(&p->usr_cfg, cfg, sizeof(RcCfg));
    rc_dbg_rc("init rc param: fqp %d:%d:%d:%d\n", cfg->fqp_min_i, cfg->fqp_max_i,
              cfg->fqp_min_p, cfg->fqp_max_p);

    bits_model_init(p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_deinit(void *ctx)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;

    rc_dbg_func("enter %p\n", ctx);
    bits_model_param_deinit(p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_start(void *ctx, EncRcTask *task)
{
    RcModelV2Ctx *p = (RcModelV2Ctx*)ctx;
    EncFrmStatus *frm = &task->frm;
    EncRcTaskInfo *info = &task->info;
    RcCfg *usr_cfg = &p->usr_cfg;

    rc_dbg_func("enter %p\n", ctx);

    if (usr_cfg->mode == RC_FIXQP) {
        if (usr_cfg->init_quality <= 0) {
            mpp_log("invalid fix %d qp found set default qp 26\n",
                    usr_cfg->init_quality);
            usr_cfg->init_quality = 26;
        }

        if (usr_cfg->max_quality <= 0)
            usr_cfg->max_quality = usr_cfg->init_quality;
        if (usr_cfg->min_quality <= 0)
            usr_cfg->min_quality = usr_cfg->init_quality;
        if (usr_cfg->max_i_quality <= 0)
            usr_cfg->max_i_quality = usr_cfg->max_quality;
        if (usr_cfg->min_i_quality <= 0)
            usr_cfg->min_i_quality = usr_cfg->min_quality;

        if (frm->is_intra) {
            info->quality_max = usr_cfg->max_i_quality;
            info->quality_min = usr_cfg->min_i_quality;
            info->quality_target = usr_cfg->init_quality;
        } else {
            info->quality_max = usr_cfg->max_quality;
            info->quality_min = usr_cfg->min_quality;
            info->quality_target = usr_cfg->init_quality;
        }

        rc_dbg_rc("seq_idx %d intra %d\n", frm->seq_idx, frm->is_intra);
        rc_dbg_rc("bitrate [%d : %d : %d]\n", info->bit_min, info->bit_target, info->bit_max);
        rc_dbg_rc("quality [%d : %d : %d]\n", info->quality_min, info->quality_target, info->quality_max);

        return MPP_OK;
    }

    if (usr_cfg->fps.fps_out_flex) {
        fps_chg_update_mode(ctx);
    }

    info->frame_type = (frm->is_intra) ? (INTRA_FRAME) : (INTER_P_FRAME);

    if (frm->ref_mode == REF_TO_PREV_INTRA)
        info->frame_type = INTER_VI_FRAME;

    if (frm->is_i_refresh)
        info->frame_type = INTRA_RFH_FRAME;

    p->next_ratio = 0;
    if (p->last_frame_type == INTRA_FRAME) {
        calc_next_i_ratio(p);
    }

    if (!p->first_frm_flg) {
        if (p->calc_ratio)
            p->calc_ratio(p, info);
    } else {
        bits_model_alloc(p, info, p->gop_total_bits);
        /* quality determination */
        info->quality_target = (usr_cfg->init_quality <= 0) ? -1 : usr_cfg->init_quality;
    }

    if (frm->is_intra) {
        info->quality_max = usr_cfg->max_i_quality;
        info->quality_min = usr_cfg->min_i_quality;
    } else {
        info->quality_max = usr_cfg->max_quality;
        info->quality_min = usr_cfg->min_quality;
    }

    bits_model_preset(p, info);

    rc_dbg_rc("seq_idx %d intra %d\n", frm->seq_idx, frm->is_intra);
    rc_dbg_rc("bitrate [%d : %d : %d]\n", info->bit_min, info->bit_target, info->bit_max);
    rc_dbg_rc("quality [%d : %d : %d]\n", info->quality_min, info->quality_target, info->quality_max);

    p->reenc_cnt = 0;
    p->last_frame_type = info->frame_type;

    rc_dbg_func("leave %p\n", ctx);

    return MPP_OK;
}

static RK_S32 cal_first_i_start_qp(RK_S32 target_bit, RK_U32 total_mb)
{
    static RK_U32 mb_num[9] = {
        0,     200,   700,   1200,
        2000,  4000,  8000,  16000,
        20000
    };

    static RK_U32 tab_bit[9] = {
        3780,  3570,  3150,  2940,
        2730,  3780,  2100,  1680,
        2100
    };

    static RK_U8 qscale2qp[96] = {
        15,  15,  15,  15,  15,  16,  18,  20,  21,  22,  23,  24,
        25,  25,  26,  27,  28,  28,  29,  29,  30,  30,  30,  31,
        31,  32,  32,  33,  33,  33,  34,  34,  34,  34,  35,  35,
        35,  36,  36,  36,  36,  36,  37,  37,  37,  37,  38,  38,
        38,  38,  38,  39,  39,  39,  39,  39,  39,  40,  40,  40,
        40,  41,  41,  41,  41,  41,  41,  41,  42,  42,  42,  42,
        42,  42,  42,  42,  43,  43,  43,  43,  43,  43,  43,  43,
        44,  44,  44,  44,  44,  44,  44,  44,  45,  45,  45,  45,
    };

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

static RK_S32 derive_min_qp_from_complexity(RcModelV2Ctx *ctx, EncRcTaskInfo *info, RK_U32 is_intra)
{
    RcCfg *usr_cfg = &ctx->usr_cfg;
    RcMode rc_mode = usr_cfg->mode;
    RK_S32 qp_min = info->quality_min;
    RK_S32 fqp_min_i = usr_cfg->fqp_min_i;
    RK_S32 fqp_min_p = usr_cfg->fqp_min_p;
    RK_S32 cplx = mpp_data_sum_v2(ctx->complex_level);
    RK_S32 md = mpp_data_sum_v2(ctx->motion_level);
    RK_S32 md3 = mpp_data_get_pre_val_v2(ctx->motion_level, 0) +
                 mpp_data_get_pre_val_v2(ctx->motion_level, 1) +
                 mpp_data_get_pre_val_v2(ctx->motion_level, 2);

    if (RC_AVBR == rc_mode || RC_VBR == rc_mode || RC_CBR == rc_mode) {
        if (md >= 700) {
            qp_min = is_intra ? fqp_min_i : fqp_min_p;
            if (md >= 1400)
                qp_min += md3 > 300 ? 3 : 2;
            else
                qp_min += md3 > 300 ? 2 : 1;

            if (cplx >= 15)
                qp_min++;
        } else if (RC_CBR != rc_mode) {
            if (md > 100) {
                if (cplx >= 16)
                    qp_min = (is_intra ? fqp_min_i : fqp_min_p) + 1;
                else if (cplx >= 10)
                    qp_min = (is_intra ? fqp_min_i : fqp_min_p) + 0;
            } else {
                qp_min = (is_intra ? fqp_min_i : fqp_min_p);
                qp_min += (cplx >= 15) ? 3 : (cplx >= 10) ? 2 : (cplx >= 5) ? 1 : 0;
            }
        }

        qp_min = mpp_clip(qp_min, info->quality_min, info->quality_max);

        rc_dbg_rc("frame %d complex_level %d motion_level %d md3 %d qp_min %d\n",
                  ctx->frm_num, cplx, md, md3, qp_min);
    }

    return qp_min;
}

MPP_RET rc_model_v2_hal_start(void *ctx, EncRcTask *task)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    EncFrmStatus *frm = &task->frm;
    EncRcTaskInfo *info = &task->info;
    EncRcForceCfg *force = &task->force;
    RcCfg *usr_cfg = &p->usr_cfg;
    RK_S32 mb_w = MPP_ALIGN(usr_cfg->width, 16) / 16;
    RK_S32 mb_h = MPP_ALIGN(usr_cfg->height, 16) / 16;
    RK_S32 bit_min = info->bit_min;
    RK_S32 bit_max = info->bit_max;
    RK_S32 bit_target = info->bit_target;
    RK_S32 quality_min = info->quality_min;
    RK_S32 quality_max = info->quality_max;
    RK_S32 quality_target = info->quality_target;

    rc_dbg_func("enter p %p task %p\n", p, task);
    rc_dbg_rc("seq_idx %d intra %d\n", frm->seq_idx, frm->is_intra);

    if (force->force_flag & ENC_RC_FORCE_QP) {
        RK_S32 qp = force->force_qp;
        info->quality_target = qp;
        info->quality_max = qp;
        info->quality_min = qp;
        return MPP_OK;
    }

    if (usr_cfg->mode == RC_FIXQP) {
        RK_S32 i_quality_delta = usr_cfg->i_quality_delta;

        if (frm->is_intra && i_quality_delta)
            p->start_qp = quality_target - i_quality_delta;
        else
            p->start_qp = quality_target;

        info->quality_target = p->start_qp;

        return MPP_OK;
    }

    /* setup quality parameters */
    if (p->first_frm_flg && frm->is_intra) {
        RK_S32 i_quality_delta = usr_cfg->i_quality_delta;
        p->pre_target_bits_fix_count = 0;
        p->pre_real_bits_count = 0;

        if (info->quality_target < 0) {
            if (info->bit_target) {
                info->quality_target = cal_first_i_start_qp(info->bit_target, mb_w * mb_h);
            } else {
                mpp_log("init qp not set on fix qp mode, use default qp\n");
                info->quality_target = 26;
            }
        }

        if (p->reenc_cnt) {
            p->cur_scale_qp += p->next_ratio;
            p->start_qp = (p->cur_scale_qp >> 6) - i_quality_delta;
        } else {
            p->start_qp = info->quality_target;
            p->cur_scale_qp = (info->quality_target + i_quality_delta) << 6;
        }

        rc_dbg_rc("qp: start %2d cur_scale %d next_ratio %d reenc %d\n",
                  p->start_qp, p->cur_scale_qp, p->next_ratio, p->reenc_cnt);

        p->cur_scale_qp = mpp_clip(p->cur_scale_qp, (info->quality_min << 6), (info->quality_max << 6));
    } else {
        RK_S32 qp_scale = p->cur_scale_qp + p->next_ratio;
        RK_S32 start_qp = 0;
        RK_S32 qpmin = derive_min_qp_from_complexity(p, info, frm->is_intra);

        if (frm->is_intra && !frm->is_i_refresh) {
            RK_S32 i_quality_delta = usr_cfg->i_quality_delta;
            RK_S32 qp_scale_t = qp_scale =
                                    mpp_clip(qp_scale, (info->quality_min << 6), (info->quality_max << 6));

            qp_scale_t = (qp_scale + p->next_i_ratio) >> 6;
            if (qp_scale_t >= 35 && p->pre_i_qp <= 33)
                start_qp = (p->pre_i_qp * 307 + qp_scale_t * 717) >> 10;
            else
                start_qp = (p->pre_i_qp + qp_scale_t) >> 1;

            if (i_quality_delta) {
                RK_U32 index = mpp_clip(mpp_data_mean_v2(p->madi) / 4, 0, 7);
                RK_S32 max_ip_delta = max_ip_qp_dealt[index];

                if (i_quality_delta > max_ip_delta)
                    i_quality_delta = max_ip_delta;

                rc_dbg_rc("qp prev %d:%d curr %d - %d (max %d) -> %d reenc %d\n",
                          p->pre_i_qp, qp_scale >> 6, start_qp,
                          usr_cfg->i_quality_delta, max_ip_delta,
                          start_qp - i_quality_delta, p->reenc_cnt);

                //if (!p->reenc_cnt) {
                start_qp -= i_quality_delta;
                //}
            }

            start_qp = mpp_clip(start_qp, qpmin, usr_cfg->fqp_max_i);
            start_qp = mpp_clip(start_qp, info->quality_min, info->quality_max);
            p->start_qp = start_qp;

            if (!p->reenc_cnt) {
                p->cur_scale_qp = qp_scale;
                if (p->usr_cfg.debreath_cfg.enable) {
                    calc_debreath_qp(ctx);
                }
            }

            p->gop_frm_cnt = 0;
            p->gop_qp_sum = 0;
        } else {
            qp_scale = mpp_clip(qp_scale, (qpmin << 6), (info->quality_max << 6));
            qp_scale = mpp_clip(qp_scale, (info->quality_min << 6), (info->quality_max << 6));
            p->cur_scale_qp = qp_scale;
            rc_dbg_rc("qp %d -> %d\n", p->start_qp, qp_scale >> 6);
            p->start_qp = qp_scale >> 6;
            if (frm->ref_mode == REF_TO_PREV_INTRA && usr_cfg->vi_quality_delta) {
                rc_dbg_rc("qp %d -> %d (vi)\n", p->start_qp, p->start_qp - usr_cfg->vi_quality_delta);
                p->start_qp -= usr_cfg->vi_quality_delta;
            }
            p->start_qp = mpp_clip(p->start_qp, qpmin, usr_cfg->fqp_max_p);
        }
        if (p->pre_target_bits_fix_count * 90 / 100 > p->pre_real_bits_count) {
            p->start_qp = mpp_clip(p->start_qp, info->quality_min, 35);
        } else if (p->pre_target_bits_fix_count * 100 / 100 > p->pre_real_bits_count) {
            p->start_qp = mpp_clip(p->start_qp, info->quality_min, 37);
        } else if (p->pre_target_bits_fix_count * 107 / 100 > p->pre_real_bits_count) {
            p->start_qp = mpp_clip(p->start_qp, info->quality_min, 39);
        }
        p->start_qp = mpp_clip(p->start_qp, qpmin, 51);
    }

    if (usr_cfg->hier_qp_cfg.hier_qp_en && !p->reenc_cnt) {
        rc_hier_calc_dealt_qp(p, info);
        if (p->qp_layer_id) {
            RK_S32 hier_qp_delta = usr_cfg->hier_qp_cfg.hier_qp_delta[p->qp_layer_id - 1];

            p->start_qp -= hier_qp_delta;
            rc_dbg_qp("hier_qp: layer %d delta %d", p->qp_layer_id, hier_qp_delta);
        }
    }

    if (frm->is_intra)
        p->start_qp = mpp_clip(p->start_qp, usr_cfg->fqp_min_i, usr_cfg->fqp_max_i);
    else
        p->start_qp = mpp_clip(p->start_qp, usr_cfg->fqp_min_p, usr_cfg->fqp_max_p);
    info->quality_target = p->start_qp;

    rc_dbg_rc("bitrate [%d : %d : %d] -> [%d : %d : %d]\n",
              bit_min, bit_target, bit_max,
              info->bit_min, info->bit_target, info->bit_max);
    rc_dbg_rc("quality [%d : %d : %d] -> [%d : %d : %d]\n",
              quality_min, quality_target, quality_max,
              info->quality_min, info->quality_target, info->quality_max);
    rc_dbg_func("leave %p\n", p);
    return MPP_OK;
}

MPP_RET rc_model_v2_hal_end(void *ctx, EncRcTask *task)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    EncFrmStatus *frm = &task->frm;

    rc_dbg_func("enter ctx %p task %p\n", ctx, task);

    if (frm->is_intra)
        p->pre_i_qp = p->cur_scale_qp >> 6;
    else
        p->pre_p_qp = p->cur_scale_qp >> 6;

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_check_reenc(void *ctx, EncRcTask *task)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    EncRcTaskInfo *cfg = (EncRcTaskInfo *)&task->info;
    EncFrmStatus *frm = &task->frm;
    RcCfg *usr_cfg = &p->usr_cfg;

    rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);

    frm->reencode = 0;

    if ((usr_cfg->mode == RC_FIXQP) ||
        (task->force.force_flag & ENC_RC_FORCE_QP) ||
        p->on_drop || p->on_pskip)
        return MPP_OK;

    if (check_re_enc(p, cfg)) {
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
            if (p->re_calc_ratio)
                p->re_calc_ratio(p, cfg);

            if (p->next_ratio != 0 && cfg->quality_target < cfg->quality_max) {
                p->reenc_cnt++;
                frm->reencode = 1;
            }
            p->drop_cnt = 0;
        } break;
        }
    }

    return MPP_OK;
}

MPP_RET rc_model_v2_end(void *ctx, EncRcTask *task)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;
    EncRcTaskInfo *cfg = (EncRcTaskInfo *)&task->info;
    RcCfg *usr_cfg = &p->usr_cfg;

    rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);

    rc_dbg_rc("bits_mode_update real_bit %d", cfg->bit_real);

    if (usr_cfg->mode == RC_FIXQP)
        goto DONE;

    rc_dbg_rc("motion_level %u, complex_level %u\n", cfg->motion_level, cfg->complex_level);
    mpp_data_update_v2(p->motion_level, cfg->motion_level);
    mpp_data_update_v2(p->complex_level, cfg->complex_level);
    cfg->rt_bits = p->ins_bps;
    p->last_inst_bps = p->ins_bps;
    p->first_frm_flg = 0;

    bits_model_update(p, cfg);
    if (usr_cfg->mode == RC_AVBR) {
        moving_judge_update(p, cfg);
        bit_statics_update(p, cfg->bit_real);
    }

    p->gop_frm_cnt++;
    p->gop_qp_sum += p->start_qp;

    p->pre_mean_qp = cfg->quality_real;
    p->pre_iblk4_prop = cfg->iblk4_prop;
    p->scale_qp = p->cur_scale_qp;
    p->prev_md_prop = 0;
    p->pre_target_bits = cfg->bit_target;
    p->pre_target_bits_fix = cfg->bit_target_fix;
    p->pre_real_bits = cfg->bit_real;

    p->on_drop = 0;
    p->on_pskip = 0;

DONE:
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}
