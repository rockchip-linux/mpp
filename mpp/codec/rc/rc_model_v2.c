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

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "rc_base.h"
#include "rc_debug.h"
#include "rc_model_v2.h"
#include "string.h"


#define I_WINDOW_LEN 2
#define P_WINDOW1_LEN 5
#define P_WINDOW2_LEN 8

RK_S32 max_i_delta_qp[51] = {
    0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA,
    0x9, 0x9, 0x8, 0x8, 0x7, 0x7, 0x6, 0x6, 0x5, 0x5, 0x5, 0x4, 0x4, 0x4,
    0x3, 0x3, 0x3, 0x3, 0x3, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x1, 0x1, 0x1,
    0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
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

typedef struct RcModelV2Ctx_t {
    RcCfg        usr_cfg;
    RcHalCfg     hal_cfg;

    RK_U32       frame_type;
    RK_U32       last_frame_type;
    RK_S64       gop_total_bits;
    RK_U32       bit_per_frame;

    MppDataV2    *i_bit;
    RK_U32       i_sumbits;
    RK_U32       i_scale;

    MppDataV2    *idr_bit;
    RK_U32       idr_sumbits;
    RK_U32       idr_scale;

    MppDataV2    *p_bit;
    RK_U32       p_sumbits;
    RK_U32       p_scale;

    MppDataV2    *pre_p_bit;

    RK_S32       target_bps;
    RK_S32       pre_target_bits;
    RK_S32       pre_real_bits;
    RK_S32       frm_bits_thr;
    RK_S32       ins_bps;
    RK_S32       last_inst_bps;
    RK_U32       water_level_thr;

    MppDataV2    *stat_bits;
    MppDataV2    *stat_rate;
    RK_S32       stat_watl_thrd;
    RK_S32       stat_watl;
    RK_S32       stat_last_watl;

    RK_S32       next_i_ratio;      // scale 64
    RK_S32       next_ratio;        // scale 64
    RK_S32       pre_i_qp;
    RK_S32       pre_p_qp;
    RK_S32       scale_qp;          // scale 64
    MppDataV2    *means_qp;

    RK_U32       frm_num;
    RK_S32       reenc_cnt;
} RcModelV2Ctx;

MPP_RET bits_model_deinit(RcModelV2Ctx *ctx)
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

    if (ctx->pre_p_bit != NULL) {
        mpp_data_deinit_v2(ctx->pre_p_bit);
        ctx->pre_p_bit = NULL;
    }

    if (ctx->stat_rate != NULL) {
        mpp_data_deinit_v2(ctx->stat_rate);
        ctx->stat_rate = NULL;
    }

    if (ctx->stat_bits != NULL) {
        mpp_data_deinit_v2(ctx->stat_bits);
        ctx->stat_bits = NULL;
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_init(RcModelV2Ctx *ctx)
{
    RK_U32 gop_len = ctx->usr_cfg.igop;
    RcFpsCfg *fps = &ctx->usr_cfg.fps;
    RK_S64 gop_bits = 0;
    RK_U32 stat_times = ctx->usr_cfg.stat_times;
    RK_U32 stat_len;
    RK_U32 target_bps;
    RK_U32 total_stat_bits;
    RK_U32 p_bit = 0;

    rc_dbg_func("enter %p\n", ctx);

    if (stat_times == 0) {
        stat_times = 3;
        ctx->usr_cfg.stat_times = stat_times;
    }

    rc_dbg_rc("gop_len %d ctx->usr_cfg.stat_times %d \n", gop_len, ctx->usr_cfg.stat_times);
    stat_len = fps->fps_in_num * ctx->usr_cfg.stat_times;
    if (ctx->usr_cfg.mode == RC_CBR) {
        target_bps = ctx->usr_cfg.bps_target;
    } else {
        target_bps = ctx->usr_cfg.bps_max;
    }

    gop_bits = gop_len * target_bps * fps->fps_out_denorm;
    ctx->gop_total_bits = gop_bits / fps->fps_out_num;
    mpp_log("gop_total_bits = %lld, gop_len %d fps->fps_out_denorm %d, fps->fps_out_num %d",  ctx->gop_total_bits, gop_len,
            fps->fps_out_denorm, fps->fps_out_num);

    bits_model_deinit(ctx);
    mpp_data_init_v2(&ctx->i_bit, I_WINDOW_LEN);
    if (ctx->i_bit == NULL) {
        mpp_err("i_bit init fail");
        return -1;
    }

    mpp_data_init_v2(&ctx->p_bit, P_WINDOW1_LEN);
    if (ctx->p_bit == NULL) {
        mpp_err("p_bit init fail");
        return -1;
    }

    mpp_data_init_v2(&ctx->pre_p_bit, P_WINDOW2_LEN);
    if (ctx->pre_p_bit == NULL) {
        mpp_err("pre_p_bit init fail");
        return -1;
    }
    mpp_data_init_v2(&ctx->stat_rate, fps->fps_in_num);
    if (ctx->stat_rate == NULL) {
        mpp_err("stat_rate init fail fps_in_num %d", fps->fps_in_num);
        return -1;
    }

    mpp_data_init_v2(&ctx->stat_bits, stat_len);
    if (ctx->stat_bits == NULL) {
        mpp_err("stat_bits init fail stat_len %d", stat_len);
        return -1;
    }
    mpp_data_reset_v2(ctx->stat_rate, 0);
    ctx->i_scale = 160;
    ctx->p_scale = 16;

    total_stat_bits =  stat_times * target_bps;
    ctx->target_bps = target_bps;
    ctx->bit_per_frame = target_bps / fps->fps_in_num;
    mpp_data_reset_v2(ctx->stat_bits, ctx->bit_per_frame);
    ctx->stat_watl_thrd = total_stat_bits;
    ctx->stat_watl = total_stat_bits >> 3;

    mpp_log("bit_per_frame = %d",  ctx->bit_per_frame);

    if (gop_len <= 1)
        p_bit = ctx->gop_total_bits * 16;
    else
        p_bit = ctx->gop_total_bits * 16 / (ctx->i_scale + 16 * (gop_len - 1));

    mpp_data_reset_v2(ctx->p_bit, p_bit);

    ctx->p_sumbits = 5 * p_bit;

    mpp_data_reset_v2(ctx->i_bit, p_bit * ctx->i_scale / 16);

    ctx->i_sumbits = 2 * p_bit * ctx->i_scale / 16;
    rc_dbg_rc("ctx->p_sumbits %d,ctx->i_sumbits %d", ctx->p_sumbits, ctx->i_sumbits);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_update(RcModelV2Ctx *ctx, RK_S32 real_bit)
{
    RK_S32 water_level = 0, last_water_level;

    rc_dbg_func("enter %p\n", ctx);
    mpp_data_update_v2(ctx->stat_rate, real_bit != 0);
    mpp_data_update_v2(ctx->stat_bits, real_bit);
    last_water_level = ctx->stat_last_watl;
    if (real_bit + ctx->stat_watl > ctx->stat_watl_thrd)
        water_level = ctx->stat_watl_thrd - ctx->bit_per_frame;
    else
        water_level = real_bit + ctx->stat_watl - ctx->bit_per_frame;

    if (last_water_level < water_level) {
        last_water_level = water_level;
    }

    ctx->stat_watl = last_water_level;
    rc_dbg_rc("ctx->stat_watl = %d", ctx->stat_watl);
    switch (ctx->frame_type) {
    case INTRA_FRAME: {
        mpp_data_update_v2(ctx->i_bit, real_bit);
        ctx->i_sumbits = mpp_data_sum_v2(ctx->i_bit);
        rc_dbg_rc("ctx->i_sumbits %d ctx->p_sumbits %d", ctx->i_sumbits, ctx->p_sumbits);
        ctx->i_scale = 80 * ctx->i_sumbits / (2 * ctx->p_sumbits);
        mpp_log(" ctx->i_scale %d", ctx->i_scale);
    } break;

    case INTER_P_FRAME: {
        mpp_data_update_v2(ctx->p_bit, real_bit);
        ctx->p_sumbits = mpp_data_sum_v2(ctx->p_bit);
        ctx->p_scale = 16;
    } break;

    default:
        break;
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET bits_model_alloc(RcModelV2Ctx *ctx, RcHalCfg *cfg)
{
    RK_U32 max_i_prop = ctx->usr_cfg.max_i_bit_prop * 16;
    RK_U32 gop_len = ctx->usr_cfg.igop;
    RK_S64 total_bits = ctx->gop_total_bits;
    RK_S32 ins_bps = mpp_data_sum_v2(ctx->stat_bits) / ctx->usr_cfg.stat_times;
    rc_dbg_func("enter %p\n", ctx);
    rc_dbg_rc("ctx->frame_type %d", ctx->frame_type);
    RK_S32 i_scale = ctx->i_scale;

    if (ctx->frame_type == INTRA_FRAME) {
        i_scale = mpp_clip(i_scale, 16, max_i_prop);
        total_bits = total_bits * i_scale;
    } else {
        i_scale = mpp_clip(i_scale, 16, 16000);
        total_bits = total_bits * 16;
    }

    rc_dbg_rc("i_scale  %d, total_bits %lld", i_scale, total_bits);
    cfg->bit_target = total_bits / (i_scale + 16 * (gop_len - 1));
    ctx->ins_bps = ins_bps;

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET calc_next_i_ratio(RcModelV2Ctx *ctx)
{
    RK_S32 max_i_prop = ctx->usr_cfg.min_i_bit_prop * 16;
    RK_S32 gop_len    = ctx->usr_cfg.igop;
    RK_S32 bits_alloc = ctx->gop_total_bits * max_i_prop / (max_i_prop + 16 * (gop_len - 1));
    RK_S32 pre_qp     = ctx->pre_i_qp;

    rc_dbg_func("enter %p\n", ctx);

    if (ctx->pre_real_bits > bits_alloc || ctx->next_i_ratio) {
        RK_S32 ratio = ((ctx->pre_real_bits - bits_alloc) << 8) / bits_alloc;
        if (ratio >= 256)
            ratio = 256;
        if (ratio < -256)
            ratio = ctx->next_i_ratio - 256;
        else
            ratio = ctx->next_i_ratio + ratio;
        if (ratio >= 0) {
            if (ratio > (max_i_delta_qp[pre_qp] << 6))
                ratio = (max_i_delta_qp[pre_qp] << 6);
        } else {
            ratio = 0;
        }
        ctx->next_i_ratio = ratio;
        rc_dbg_rc("ctx->next_i_ratio %d", ctx->next_i_ratio);
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;

}

MPP_RET calc_cbr_ratio(RcModelV2Ctx *ctx)
{
    RK_S32 target_bps = ctx->target_bps;
    RK_S32 ins_bps = ctx->ins_bps;
    RK_S32 pre_target_bits = ctx->pre_target_bits;
    RK_S32 pre_real_bits = ctx->pre_real_bits;
    RK_S32 pre_ins_bps = ctx->last_inst_bps;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio, wl_ratio;
    RK_S32 flag = 0;
    RK_S32 fluc_l = 1;

    rc_dbg_func("enter %p\n", ctx);
    rc_dbg_rc("ins_bps %d pre_ins_bps %d, pre_target_bits %d pre_real_bits %d",
              ins_bps, pre_ins_bps, pre_target_bits, pre_real_bits);

    mpp_assert(target_bps > 0);

    if (pre_target_bits > pre_real_bits)
        bit_diff_ratio = 48 * (pre_real_bits - pre_target_bits) / pre_target_bits;
    else
        bit_diff_ratio = 64 * (pre_real_bits - pre_target_bits) / pre_target_bits;

    idx1 = ins_bps / (target_bps >> 5);
    idx2 = pre_ins_bps / (target_bps >> 5);

    idx1 = mpp_clip(idx1, 0, 64);
    idx2 = mpp_clip(idx2, 0, 64);
    ins_ratio = tab_lnx[idx1] - tab_lnx[idx2];

    rc_dbg_rc("bit_diff_ratio %d ins_ratio = %d", bit_diff_ratio, ins_ratio);

    if (ins_bps <= pre_ins_bps || pre_ins_bps <= target_bps - ((target_bps + 15) >> 4)) {
        if (ins_bps >= pre_ins_bps || pre_ins_bps >= target_bps + ((target_bps + 15) >> 4)) {
            if (bit_diff_ratio < -128) {
                bit_diff_ratio = -128;
                flag = 1;
            } else {
                if (bit_diff_ratio > 256) {
                    bit_diff_ratio = 256;
                }
                ins_ratio = 0;
            }
        } else {
            ins_ratio = 4 * ins_ratio;
        }
    } else {
        ins_ratio = 6 * ins_ratio;
    }

    if (bit_diff_ratio < -128) {
        bit_diff_ratio = -128;
    }

    if (!flag) {
        if (ins_ratio < -128) {
            ins_ratio = bit_diff_ratio - 128;
        } else if (ins_ratio > 256) {
            ins_ratio = bit_diff_ratio + 256;
        } else {
            ins_ratio = bit_diff_ratio + ins_ratio;
        }
    }

    rc_dbg_rc("ins_bps  %d,target_bps %d ctx->stat_watl %d stat_watl_thrd %d", ins_bps, target_bps, ctx->stat_watl, ctx->stat_watl_thrd >> 3);
    bps_ratio = (ins_bps - target_bps) * fluc_l / (target_bps >> 4);
    wl_ratio = 4 * (ctx->stat_watl - (ctx->stat_watl_thrd >> 3)) * fluc_l / (ctx->stat_watl_thrd >> 3);

    rc_dbg_rc("bps_ratio = %d wl_ratio %d", bps_ratio, wl_ratio);
    if (bps_ratio >= 32)
        bps_ratio = 32;

    if (wl_ratio >= 16)
        wl_ratio = 16;

    if (bps_ratio < -32)
        bps_ratio = ins_ratio - 32;
    else
        bps_ratio = bps_ratio + ins_ratio;

    if (wl_ratio < -16)
        wl_ratio = bps_ratio - 16;
    else
        wl_ratio = bps_ratio + wl_ratio;

    ctx->next_ratio = wl_ratio;
    rc_dbg_rc("get  next_ratio %d", wl_ratio);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET reenc_calc_cbr_ratio(RcModelV2Ctx *ctx, RcHalCfg *cfg)
{
    RK_S32 stat_time = ctx->usr_cfg.stat_times;
    RK_S32 last_ins_bps = mpp_data_sum_v2(ctx->stat_bits) / stat_time;
    RK_S32 ins_bps = (last_ins_bps - ctx->stat_bits->val[0] + cfg->bit_real) / stat_time;
    RK_S32 real_bit = cfg->bit_real;
    RK_S32 target_bit = cfg->bit_target;
    RK_S32 target_bps = ctx->target_bps;
    RK_S32 water_level = 0;
    RK_S32 idx1, idx2;
    RK_S32 i_flag = 0;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio, wl_ratio;

    rc_dbg_func("enter %p\n", ctx);

    i_flag = (ctx->frame_type == INTRA_FRAME);

    if (real_bit + ctx->stat_watl > ctx->stat_watl_thrd)
        water_level = ctx->stat_watl_thrd - ctx->bit_per_frame;
    else
        water_level = real_bit + ctx->stat_watl_thrd - ctx->bit_per_frame;

    if (water_level < ctx->stat_last_watl) {
        water_level = ctx->stat_last_watl;
    }

    if (target_bit > real_bit)
        bit_diff_ratio = 32 * (real_bit - target_bit) / target_bit;
    else
        bit_diff_ratio = 32 * (real_bit - target_bit) / real_bit;

    idx1 = ins_bps / (target_bps >> 5);
    idx2 = last_ins_bps / (target_bps >> 5);

    idx1 = mpp_clip(idx1, 0, 64);
    idx2 = mpp_clip(idx2, 0, 64);
    ins_ratio = tab_lnx[idx1] - tab_lnx[idx2];

    bps_ratio = 96 * (ins_bps - target_bps) / target_bps;
    wl_ratio = 32 * (water_level - (ctx->water_level_thr >> 3)) / (ctx->water_level_thr >> 3);
    if (last_ins_bps < ins_bps && target_bps != last_ins_bps) {
        ins_ratio = 6 * ins_ratio;
        ins_ratio = mpp_clip(ins_ratio, -192, 256);
    } else {
        if (i_flag) {
            ins_ratio = 3 * ins_ratio;
            ins_ratio = mpp_clip(ins_ratio, -192, 256);
        } else {
            ins_ratio = 0;
        }
    }
    if (bit_diff_ratio >= 256)
        bit_diff_ratio = 256;

    if (bps_ratio >= 32)
        bps_ratio = 32;

    if (wl_ratio >= 32)
        wl_ratio = 32;

    if (bit_diff_ratio < -128)
        ins_ratio = ins_ratio - 128;
    else
        ins_ratio = ins_ratio + bit_diff_ratio;

    if (bps_ratio < -32)
        ins_ratio = ins_ratio - 32;
    else
        ins_ratio = ins_ratio + bps_ratio;

    if (wl_ratio < -32)
        wl_ratio = ins_ratio - 32;
    else
        wl_ratio = ins_ratio + wl_ratio;

    ctx->next_ratio = wl_ratio;

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}


MPP_RET calc_vbr_ratio(RcModelV2Ctx *ctx)
{
    RK_S32 bps_change = ctx->target_bps;
    RK_S32 max_bps_target = ctx->usr_cfg.bps_max;
    RK_S32 ins_bps = ctx->ins_bps;
    RK_S32 pre_target_bits = ctx->pre_target_bits;
    RK_S32 pre_real_bits = ctx->pre_real_bits;
    RK_S32 pre_ins_bps = ctx->last_inst_bps;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio;
    RK_S32 flag = 0;

    rc_dbg_func("enter %p\n", ctx);

    if (pre_target_bits > pre_real_bits)
        bit_diff_ratio = 32 * (pre_real_bits - pre_target_bits) / pre_target_bits;
    else
        bit_diff_ratio = 64 * (pre_real_bits - pre_target_bits) / pre_target_bits;

    idx1 = ins_bps / (max_bps_target >> 5);
    idx2 = pre_ins_bps / (max_bps_target >> 5);

    idx1 = mpp_clip(idx1, 0, 64);
    idx2 = mpp_clip(idx2, 0, 64);
    ins_ratio = tab_lnx[idx1] - tab_lnx[idx2];
    if (ins_bps <= bps_change || (ins_bps > bps_change && ins_bps <= pre_ins_bps)) {
        flag = ins_bps < pre_ins_bps;
        if (bps_change <= pre_ins_bps)
            flag = 0;
        if (!flag) {
            bit_diff_ratio = mpp_clip(bit_diff_ratio, -128, 256);
        } else {
            ins_ratio = 3 * ins_ratio;
        }
    } else {
        ins_ratio = 6 * ins_ratio;
    }
    ins_ratio = mpp_clip(ins_ratio, -128, 256);
    bps_ratio = 3 * (ins_bps - bps_change) / (max_bps_target >> 4);
    bps_ratio = mpp_clip(bps_ratio, -16, 32);
    if (ctx->i_scale > 640) {
        bit_diff_ratio = mpp_clip(bit_diff_ratio, -16, 32);
        ins_ratio = mpp_clip(ins_ratio, -16, 32);
    }
    ctx->next_ratio = bit_diff_ratio + ins_ratio + bps_ratio;

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET reenc_calc_vbr_ratio(RcModelV2Ctx *ctx, RcHalCfg *cfg)
{
    RK_S32 stat_time = ctx->usr_cfg.stat_times;
    RK_S32 last_ins_bps = mpp_data_sum_v2(ctx->stat_bits) / stat_time;
    RK_S32 ins_bps = (last_ins_bps - ctx->stat_bits->val[0] + cfg->bit_real) / stat_time;
    RK_S32 bps_change = ctx->target_bps;
    RK_S32 max_bps_target = ctx->usr_cfg.bps_max;
    RK_S32 real_bit = cfg->bit_real;
    RK_S32 target_bit = cfg->bit_target;
    RK_S32 idx1, idx2;
    RK_S32 bit_diff_ratio, ins_ratio, bps_ratio;
    rc_dbg_func("enter %p\n", ctx);

    if (target_bit <= real_bit)
        bit_diff_ratio = 32 * (real_bit - target_bit) / target_bit;
    else
        bit_diff_ratio = 32 * (real_bit - target_bit) / real_bit;

    idx1 = ins_bps / (max_bps_target >> 5);
    idx2 =  last_ins_bps / (max_bps_target >> 5);
    idx1 = mpp_clip(idx1, 0, 64);
    idx2 = mpp_clip(idx2, 0, 64);
    if (last_ins_bps < ins_bps && bps_change < ins_bps) {
        ins_ratio = 6 * (tab_lnx[idx1] - tab_lnx[idx2]);
        ins_ratio = mpp_clip(ins_ratio, -192, 256);
    } else {
        ins_ratio = 0;
    }

    bps_ratio = 96 * (ins_bps - bps_change) / bps_change;

    if (bit_diff_ratio >= 256)
        bit_diff_ratio = 256;
    if (bps_ratio >= 32)
        bps_ratio = 32;

    if (bit_diff_ratio < -128)
        ins_ratio = ins_ratio - 128;
    else
        ins_ratio = bit_diff_ratio + ins_ratio;

    if (bps_ratio < -32)
        ins_ratio = ins_ratio - 32;
    else
        ins_ratio = ins_ratio + bps_ratio;

    ctx->next_ratio = ins_ratio;
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}


MPP_RET bits_mode_reset(RcModelV2Ctx *ctx)
{
    rc_dbg_func("enter %p\n", ctx);
    (void) ctx;
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET check_re_enc(RcModelV2Ctx *ctx, RcHalCfg *cfg)
{
    RK_S32 frame_type = ctx->frame_type;
    RK_S32 i_flag = 0;
    RK_S32 big_flag = 0;
    RK_S32 stat_time = ctx->usr_cfg.stat_times;
    RK_S32 last_ins_bps = mpp_data_sum_v2(ctx->stat_bits) / stat_time;
    RK_S32 ins_bps = (last_ins_bps - ctx->stat_bits->val[0] + cfg->bit_real) / stat_time;
    RK_S32 target_bps;
    RK_S32 flag1 = 0;
    RK_S32 flag2 = 0;

    rc_dbg_func("enter %p\n", ctx);

    if (ctx->usr_cfg.mode == RC_CBR)
        target_bps = ctx->usr_cfg.bps_target;
    else
        target_bps = ctx->usr_cfg.bps_max;

    if (ctx->reenc_cnt >= ctx->usr_cfg.max_reencode_times) {
        return 0;
    }

    i_flag = (frame_type == INTRA_FRAME);
    if (!i_flag && cfg->bit_real > 3 * cfg->bit_target) {
        big_flag = 1;
    }

    if (i_flag && cfg->bit_real > 3 * cfg->bit_target / 2) {
        big_flag = 1;
    }

    if (ctx->usr_cfg.mode == RC_CBR) {
        flag1 = target_bps / 20 < ins_bps - last_ins_bps;
        if (target_bps + target_bps / 10 < ins_bps ||
            target_bps - target_bps / 10 > ins_bps) {
            flag2 = 1;
        }
    } else {
        flag1 = target_bps - (target_bps >> 3) < ins_bps;
        flag2 = target_bps / 20  < ins_bps - last_ins_bps;
    }

    if (!(big_flag && flag1 && flag2)) {
        return 0;
    }

    rc_dbg_func("leave %p\n", ctx);
    return 1;
}


MPP_RET rc_model_v2_init(void *ctx, RcCfg *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx*)ctx;

    rc_dbg_func("enter %p\n", ctx);

    memcpy(&p->usr_cfg, cfg, sizeof(RcCfg));
    bits_model_init(p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_deinit(void *ctx)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;

    rc_dbg_func("enter %p\n", ctx);
    bits_model_deinit(p);

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_start(void *ctx, RcHalCfg *cfg, EncFrmStatus *frm)
{
    rc_dbg_func("enter %p\n", ctx);
    RcModelV2Ctx *p = (RcModelV2Ctx*)ctx;

    cfg->need_reenc = 0;

    if (frm->reencode || p->usr_cfg.mode == RC_FIXQP) {
        return MPP_OK;
    }
    if (frm->is_intra) {
        p->frame_type = INTRA_FRAME;
    } else {
        p->frame_type = INTER_P_FRAME;
    }

    bits_model_alloc(p, cfg);

    rc_dbg_rc("cfg->bit_target = %d", cfg->bit_target);
    cfg->next_ratio = 0;
    if (p->last_frame_type == INTRA_FRAME) {
        calc_next_i_ratio(p);
        cfg->next_i_ratio = p->next_i_ratio;
    }
    if (p->frm_num) {
        if (p->usr_cfg.mode == RC_CBR) {
            calc_cbr_ratio(p);
        } else {
            calc_vbr_ratio(p);
        }
    }
    cfg->next_ratio = p->next_ratio;
    p->frm_num++;
    p->reenc_cnt = 0;
    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET rc_model_v2_end(void *ctx, RcHalCfg *cfg)
{
    RcModelV2Ctx *p = (RcModelV2Ctx *)ctx;

    rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);
    if (p->usr_cfg.mode == RC_FIXQP) {
        return MPP_OK;
    }

    if (check_re_enc(p, cfg)) {
        if (p->usr_cfg.mode == RC_CBR) {
            reenc_calc_cbr_ratio(p, cfg);
        } else {
            reenc_calc_vbr_ratio(p, cfg);
        }

        if (p->next_ratio != 0) {
            p->reenc_cnt++;
            cfg->next_ratio = p->next_ratio;
            cfg->need_reenc = 1;
        }
    } else {
        rc_dbg_rc("bits_mode_update real_bit %d", cfg->bit_real);
        bits_model_update(p, cfg->bit_real);
        p->pre_target_bits = cfg->bit_target;
        p->pre_real_bits = cfg->bit_real;
        p->last_inst_bps = p->ins_bps;
        p->last_frame_type = p->frame_type;
    }

    rc_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

const RcImplApi default_h264e = {
    "default",
    MPP_VIDEO_CodingAVC,
    sizeof(RcModelV2Ctx),
    rc_model_v2_init,
    rc_model_v2_deinit,
    rc_model_v2_start,
    rc_model_v2_end,
};

const RcImplApi default_h265e = {
    "default",
    MPP_VIDEO_CodingHEVC,
    sizeof(RcModelV2Ctx),
    rc_model_v2_init,
    rc_model_v2_deinit,
    rc_model_v2_start,
    rc_model_v2_end,
};
