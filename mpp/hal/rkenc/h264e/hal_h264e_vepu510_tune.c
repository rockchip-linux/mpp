/*
 * Copyright 2024 Rockchip Electronics Co. LTD
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

#include "hal_enc_task.h"
#include "hal_h264e_vepu510_reg.h"

#define HAL_H264E_DBG_CONTENT           (0x00000200)
#define hal_h264e_dbg_content(fmt, ...) hal_h264e_dbg_f(HAL_H264E_DBG_CONTENT, fmt, ## __VA_ARGS__)

typedef struct HalH264eVepu510Tune_t {
    HalH264eVepu510Ctx  *ctx;

    RK_S32  pre_madp[2];
    RK_S32  pre_madi[2];
} HalH264eVepu510Tune;

static HalH264eVepu510Tune *vepu510_h264e_tune_init(HalH264eVepu510Ctx *ctx)
{
    HalH264eVepu510Tune *tune = mpp_malloc(HalH264eVepu510Tune, 1);

    if (NULL == tune)
        return tune;

    tune->ctx = ctx;
    tune->pre_madi[0] = tune->pre_madi[1] = -1;
    tune->pre_madp[0] = tune->pre_madp[1] = -1;

    return tune;
}

static void vepu510_h264e_tune_deinit(void *tune)
{
    MPP_FREE(tune);
}

static void vepu510_h264e_tune_reg_patch(void *p)
{
    HalH264eVepu510Tune *tune = (HalH264eVepu510Tune *)p;

    if (NULL == tune)
        return;
}

static void vepu510_h264e_tune_stat_update(void *p, HalEncTask *task)
{
    HalH264eVepu510Tune *tune = (HalH264eVepu510Tune *)p;

    if (NULL == tune)
        return;

    EncRcTaskInfo *rc_info = &task->rc_task->info;
    HalH264eVepu510Ctx *ctx = tune->ctx;
    HalVepu510RegSet *regs = &ctx->regs_sets[task->flags.reg_idx];
    Vepu510Status *st = &regs->reg_st;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 b16_num = mb_w * mb_h;
    RK_U32 madi_cnt = 0;

    RK_U32 madi_th_cnt0 = st->st_madi_lt_num0.madi_th_lt_cnt0 +
                          st->st_madi_rt_num0.madi_th_rt_cnt0 +
                          st->st_madi_lb_num0.madi_th_lb_cnt0 +
                          st->st_madi_rb_num0.madi_th_rb_cnt0;
    RK_U32 madi_th_cnt1 = st->st_madi_lt_num0.madi_th_lt_cnt1 +
                          st->st_madi_rt_num0.madi_th_rt_cnt1 +
                          st->st_madi_lb_num0.madi_th_lb_cnt1 +
                          st->st_madi_rb_num0.madi_th_rb_cnt1;
    RK_U32 madi_th_cnt2 = st->st_madi_lt_num1.madi_th_lt_cnt2 +
                          st->st_madi_rt_num1.madi_th_rt_cnt2 +
                          st->st_madi_lb_num1.madi_th_lb_cnt2 +
                          st->st_madi_rb_num1.madi_th_rb_cnt2;
    RK_U32 madi_th_cnt3 = st->st_madi_lt_num1.madi_th_lt_cnt3 +
                          st->st_madi_rt_num1.madi_th_rt_cnt3 +
                          st->st_madi_lb_num1.madi_th_lb_cnt3 +
                          st->st_madi_rb_num1.madi_th_rb_cnt3;
    RK_U32 madp_th_cnt0 = st->st_madp_lt_num0.madp_th_lt_cnt0 +
                          st->st_madp_rt_num0.madp_th_rt_cnt0 +
                          st->st_madp_lb_num0.madp_th_lb_cnt0 +
                          st->st_madp_rb_num0.madp_th_rb_cnt0;
    RK_U32 madp_th_cnt1 = st->st_madp_lt_num0.madp_th_lt_cnt1 +
                          st->st_madp_rt_num0.madp_th_rt_cnt1 +
                          st->st_madp_lb_num0.madp_th_lb_cnt1 +
                          st->st_madp_rb_num0.madp_th_rb_cnt1;
    RK_U32 madp_th_cnt2 = st->st_madp_lt_num1.madp_th_lt_cnt2 +
                          st->st_madp_rt_num1.madp_th_rt_cnt2 +
                          st->st_madp_lb_num1.madp_th_lb_cnt2 +
                          st->st_madp_rb_num1.madp_th_rb_cnt2;
    RK_U32 madp_th_cnt3 = st->st_madp_lt_num1.madp_th_lt_cnt3 +
                          st->st_madp_rt_num1.madp_th_rt_cnt3 +
                          st->st_madp_lb_num1.madp_th_lb_cnt3 +
                          st->st_madp_rb_num1.madp_th_rb_cnt3;

    madi_cnt = (6 * madi_th_cnt3 + 5 * madi_th_cnt2 + 4 * madi_th_cnt1) >> 2;
    rc_info->complex_level = (madi_cnt * 100 > 30 * b16_num) ? 2 :
                             (madi_cnt * 100 > 13 * b16_num) ? 1 : 0;

    {
        RK_U32 md_cnt = 0, motion_level = 0;

        if (ctx->smart_en)
            md_cnt = (12 * madp_th_cnt3 + 11 * madp_th_cnt2 + 8 * madp_th_cnt1) >> 2;
        else
            md_cnt = (24 * madp_th_cnt3 + 22 * madp_th_cnt2 + 17 * madp_th_cnt1) >> 2;

        if (md_cnt * 100 > 15 * b16_num)
            motion_level = 200;
        else if (md_cnt * 100 > 5 * b16_num)
            motion_level = 100;
        else if (md_cnt * 100 > (b16_num >> 2))
            motion_level = 1;
        else
            motion_level = 0;
        rc_info->motion_level = motion_level;
    }
    hal_h264e_dbg_rc("complex_level %d motion_level %d\n",
                     rc_info->complex_level, rc_info->motion_level);

    (void)madi_th_cnt0;
    (void)madp_th_cnt0;
}
