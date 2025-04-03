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
#include "hal_h265e_vepu510_reg.h"

typedef struct HalH265eVepu510Tune_t {
    H265eV510HalContext *ctx;

    RK_U8 *qm_mv_buf; /* qpmap move flag buffer */
    RK_U32 qm_mv_buf_size;

    RK_S32 pre_madp[2];
    RK_S32 pre_madi[2];
} HalH265eVepu510Tune;

static RK_U32 aq_thd_default[16] = {
    0,  0,  0,  0,  3,  3,  5,  5,
    8,  8,  15, 15, 20, 25, 25, 25
};

static RK_S32 aq_qp_delta_default[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    1,  2,  3,  4,  5,  6,  7,  8
};

static RK_U32 aq_thd_smt_I[16] = {
    1,  2,  3,   3,  3,  3,  5,  5,
    8,  8,  13,  15, 20, 25, 25, 25
};

static RK_S32 aq_qp_delta_smt_I[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    1,  2,  3,  5,  7,  8,  9,  9
};

static RK_U32 aq_thd_smt_P[16] = {
    0,  0,  0,   0,  3,  3,  5,  5,
    8,  8,  15, 15, 20, 25, 25, 25
};

static RK_S32 aq_qp_delta_smt_P[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    1,  2,  3,  4,  6,  7,  9,  9
};

static HalH265eVepu510Tune *vepu510_h265e_tune_init(H265eV510HalContext *ctx)
{
    HalH265eVepu510Tune *tune = mpp_calloc(HalH265eVepu510Tune, 1);

    if (NULL == tune)
        return tune;

    tune->ctx = ctx;
    tune->pre_madi[0] = tune->pre_madi[1] = -1;
    tune->pre_madp[0] = tune->pre_madp[1] = -1;

    return tune;
}

static void vepu510_h265e_tune_deinit(void *tune)
{
    HalH265eVepu510Tune *t = (HalH265eVepu510Tune *)tune;

    MPP_FREE(t->qm_mv_buf);
    MPP_FREE(tune);
}

static void vepu510_h265e_tune_aq_prepare(HalH265eVepu510Tune *tune)
{
    if (tune == NULL) {
        return;
    }

    H265eV510HalContext *ctx = tune->ctx;
    MppEncHwCfg *hw = &ctx->cfg->hw;

    if (ctx->smart_en) {
        memcpy(hw->aq_thrd_i, aq_thd_smt_I, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, aq_thd_smt_P, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, aq_qp_delta_smt_I, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, aq_qp_delta_smt_P, sizeof(hw->aq_step_p));
    } else {
        memcpy(hw->aq_thrd_i, aq_thd_default, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, aq_thd_default, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, aq_qp_delta_default, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, aq_qp_delta_default, sizeof(hw->aq_step_p));
    }
}

static void vepu510_h265e_tune_aq(HalH265eVepu510Tune *tune)
{
    H265eV510HalContext *ctx = tune->ctx;
    Vepu510H265eFrmCfg *frm_cfg = ctx->frm;
    H265eV510RegSet *regs = frm_cfg->regs_set;
    Vepu510RcRoi *r = &regs->reg_rc_roi;
    MppEncHwCfg *hw = &ctx->cfg->hw;
    RK_U32 i = 0;
    RK_S32 aq_step[16];

    for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
        if (ctx->frame_type == INTRA_FRAME) {
            r->aq_tthd[i] = hw->aq_thrd_i[i];
            aq_step[i] = hw->aq_step_i[i] & 0x1F;
        } else {
            r->aq_tthd[i] = hw->aq_thrd_p[i];
            aq_step[i] = hw->aq_step_p[i] & 0x1F;
        }
    }

    r->aq_stp0.aq_stp_s0 = aq_step[0];
    r->aq_stp0.aq_stp_0t1 = aq_step[1];
    r->aq_stp0.aq_stp_1t2 = aq_step[2];
    r->aq_stp0.aq_stp_2t3 = aq_step[3];
    r->aq_stp0.aq_stp_3t4 = aq_step[4];
    r->aq_stp0.aq_stp_4t5 = aq_step[5];
    r->aq_stp1.aq_stp_5t6 = aq_step[6];
    r->aq_stp1.aq_stp_6t7 = aq_step[7];
    r->aq_stp1.aq_stp_7t8 = 0;
    r->aq_stp1.aq_stp_8t9 = aq_step[8];
    r->aq_stp1.aq_stp_9t10 = aq_step[9];
    r->aq_stp1.aq_stp_10t11 = aq_step[10];
    r->aq_stp2.aq_stp_11t12 = aq_step[11];
    r->aq_stp2.aq_stp_12t13 = aq_step[12];
    r->aq_stp2.aq_stp_13t14 = aq_step[13];
    r->aq_stp2.aq_stp_14t15 = aq_step[14];
    r->aq_stp2.aq_stp_b15 = aq_step[15];

    r->aq_clip.aq16_rnge = 5;
    r->aq_clip.aq32_rnge = 5;
    r->aq_clip.aq8_rnge = 10;
    r->aq_clip.aq16_dif0 = 12;
    r->aq_clip.aq16_dif1 = 12;
}

static void vepu510_h265e_tune_reg_patch(void *p, HalEncTask *task)
{
    HalH265eVepu510Tune *tune = (HalH265eVepu510Tune *)p;
    (void)task;

    if (NULL == tune)
        return;

    vepu510_h265e_tune_aq(tune);
}

static void vepu510_h265e_tune_stat_update(void *p, HalEncTask *task)
{
    HalH265eVepu510Tune *tune = (HalH265eVepu510Tune *)p;
    EncRcTaskInfo *hal_rc_ret = (EncRcTaskInfo *)&task->rc_task->info;

    if (NULL == tune)
        return;

    H265eV510HalContext *ctx = tune->ctx;;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu510H265eFrmCfg *frm = ctx->frms[task_idx];
    Vepu510H265Fbk *fb = &frm->feedback;
    H265eV510RegSet *regs_set = frm->regs_set;
    H265eV510StatusElem *elem = frm->regs_ret;
    MppEncCfgSet *cfg = ctx->cfg;
    RK_U32 b16_num = MPP_ALIGN(cfg->prep.width, 16) * MPP_ALIGN(cfg->prep.height, 16) / 256;
    RK_U32 madi_cnt = 0, madp_cnt = 0;
    RK_S32 i = 0;

    RK_U32 madi_th_cnt0 = elem->st.st_madi_lt_num0.madi_th_lt_cnt0 +
                          elem->st.st_madi_rt_num0.madi_th_rt_cnt0 +
                          elem->st.st_madi_lb_num0.madi_th_lb_cnt0 +
                          elem->st.st_madi_rb_num0.madi_th_rb_cnt0;
    RK_U32 madi_th_cnt1 = elem->st.st_madi_lt_num0.madi_th_lt_cnt1 +
                          elem->st.st_madi_rt_num0.madi_th_rt_cnt1 +
                          elem->st.st_madi_lb_num0.madi_th_lb_cnt1 +
                          elem->st.st_madi_rb_num0.madi_th_rb_cnt1;
    RK_U32 madi_th_cnt2 = elem->st.st_madi_lt_num1.madi_th_lt_cnt2 +
                          elem->st.st_madi_rt_num1.madi_th_rt_cnt2 +
                          elem->st.st_madi_lb_num1.madi_th_lb_cnt2 +
                          elem->st.st_madi_rb_num1.madi_th_rb_cnt2;
    RK_U32 madi_th_cnt3 = elem->st.st_madi_lt_num1.madi_th_lt_cnt3 +
                          elem->st.st_madi_rt_num1.madi_th_rt_cnt3 +
                          elem->st.st_madi_lb_num1.madi_th_lb_cnt3 +
                          elem->st.st_madi_rb_num1.madi_th_rb_cnt3;
    RK_U32 madp_th_cnt0 = elem->st.st_madp_lt_num0.madp_th_lt_cnt0 +
                          elem->st.st_madp_rt_num0.madp_th_rt_cnt0 +
                          elem->st.st_madp_lb_num0.madp_th_lb_cnt0 +
                          elem->st.st_madp_rb_num0.madp_th_rb_cnt0;
    RK_U32 madp_th_cnt1 = elem->st.st_madp_lt_num0.madp_th_lt_cnt1 +
                          elem->st.st_madp_rt_num0.madp_th_rt_cnt1 +
                          elem->st.st_madp_lb_num0.madp_th_lb_cnt1 +
                          elem->st.st_madp_rb_num0.madp_th_rb_cnt1;
    RK_U32 madp_th_cnt2 = elem->st.st_madp_lt_num1.madp_th_lt_cnt2 +
                          elem->st.st_madp_rt_num1.madp_th_rt_cnt2 +
                          elem->st.st_madp_lb_num1.madp_th_lb_cnt2 +
                          elem->st.st_madp_rb_num1.madp_th_rb_cnt2;
    RK_U32 madp_th_cnt3 = elem->st.st_madp_lt_num1.madp_th_lt_cnt3 +
                          elem->st.st_madp_rt_num1.madp_th_rt_cnt3 +
                          elem->st.st_madp_lb_num1.madp_th_lb_cnt3 +
                          elem->st.st_madp_rb_num1.madp_th_rb_cnt3;

    madi_cnt = (6 * madi_th_cnt3 + 5 * madi_th_cnt2 + 4 * madi_th_cnt1) >> 2;
    hal_rc_ret->complex_level = (madi_cnt * 100 > 30 * b16_num) ? 2 :
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
        hal_rc_ret->motion_level = motion_level;
    }
    hal_h265e_dbg_output("complex_level %d motion_level %d\n",
                         hal_rc_ret->complex_level, hal_rc_ret->motion_level);

    fb->st_madi = madi_th_cnt0 * regs_set->reg_rc_roi.madi_st_thd.madi_th0 +
                  madi_th_cnt1 * (regs_set->reg_rc_roi.madi_st_thd.madi_th0 +
                                  regs_set->reg_rc_roi.madi_st_thd.madi_th1) / 2 +
                  madi_th_cnt2 * (regs_set->reg_rc_roi.madi_st_thd.madi_th1 +
                                  regs_set->reg_rc_roi.madi_st_thd.madi_th2) / 2 +
                  madi_th_cnt3 * regs_set->reg_rc_roi.madi_st_thd.madi_th2;

    madi_cnt = madi_th_cnt0 + madi_th_cnt1 + madi_th_cnt2 + madi_th_cnt3;
    if (madi_cnt)
        fb->st_madi = fb->st_madi / madi_cnt;

    fb->st_madp = madp_th_cnt0 * regs_set->reg_rc_roi.madp_st_thd0.madp_th0 +
                  madp_th_cnt1 * (regs_set->reg_rc_roi.madp_st_thd0.madp_th0 +
                                  regs_set->reg_rc_roi.madp_st_thd0.madp_th1) / 2 +
                  madp_th_cnt2 * (regs_set->reg_rc_roi.madp_st_thd0.madp_th1 +
                                  regs_set->reg_rc_roi.madp_st_thd1.madp_th2) / 2 +
                  madp_th_cnt3 * regs_set->reg_rc_roi.madp_st_thd1.madp_th2;

    madp_cnt = madp_th_cnt0 + madp_th_cnt1 + madp_th_cnt2 + madp_th_cnt3;
    if (madp_cnt)
        fb->st_madp =  fb->st_madp  / madp_cnt;

    fb->st_mb_num += elem->st.st_bnum_b16.num_b16;
    fb->frame_type = task->rc_task->frm.is_intra ? INTRA_FRAME : INTER_P_FRAME;
    hal_rc_ret->bit_real += fb->out_strm_size * 8;
    hal_h265e_dbg_output("bit_real %d quality_real %d\n",
                         hal_rc_ret->bit_real, hal_rc_ret->quality_real);

    {
        /* This code snippet may be unnecessary, but it is kept for rv1103b compatibility. */
        RK_S32 bit_tgt = hal_rc_ret->bit_target;
        RK_S32 bit_real = hal_rc_ret->bit_real;
        RK_S32 real_lvl = 0;

        memcpy(fb->tgt_sub_real_lvl, ctx->last_frame_fb.tgt_sub_real_lvl, 6 * sizeof(RK_S8));
        for (i = 3; i >= 0; i--)
            fb->tgt_sub_real_lvl[i + 1] = fb->tgt_sub_real_lvl[i];

        if (bit_tgt > bit_real) {
            fb->tgt_sub_real_lvl[0] = (bit_tgt > bit_real * 6 / 4) ? 3 :
                                      (bit_tgt > bit_real * 5 / 4) ? 2 :
                                      (bit_tgt > bit_real * 9 / 8) ? 1 : 0;
        } else {
            fb->tgt_sub_real_lvl[0] = (bit_real > bit_tgt * 2) ? -5 :
                                      (bit_real > bit_tgt * 7 / 4) ? -4 :
                                      (bit_real > bit_tgt * 6 / 4) ? -3 :
                                      (bit_real > bit_tgt * 5 / 4) ? -2 : -1;
        }

        for (i = 0; i < 5; i ++)
            real_lvl += fb->tgt_sub_real_lvl[i];
        if (task->rc_task->frm.is_intra)
            fb->tgt_sub_real_lvl[5] = 0;

        if (real_lvl < -9)
            fb->tgt_sub_real_lvl[5] = 2;
        else if (real_lvl < -2 && fb->tgt_sub_real_lvl[5] < 2)
            fb->tgt_sub_real_lvl[5] = 1;
    }

    if (fb->st_mb_num)
        fb->st_madi = fb->st_madi / fb->st_mb_num;
    else
        fb->st_madi = 0;

    if (fb->st_ctu_num)
        fb->st_madp = fb->st_madp / fb->st_ctu_num;
    else
        fb->st_madp = 0;

    hal_rc_ret->madi = fb->st_madi;
    hal_rc_ret->madp = fb->st_madp; /* unused ?? */
}