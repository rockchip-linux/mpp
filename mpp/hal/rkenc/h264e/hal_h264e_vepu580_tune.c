/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#include "vepu580_tune.h"

#define HAL_H264E_DBG_CONTENT           (0x00000200)
#define hal_h264e_dbg_content(fmt, ...) hal_h264e_dbg_f(HAL_H264E_DBG_CONTENT, fmt, ## __VA_ARGS__)

/*
 * Please follow the configuration below:
 *
 * FRAME_CONTENT_ANALYSIS_NUM >= 5
 * MD_WIN_LEN >= 3
 * MD_SHOW_LEN == 4
 */

typedef struct HalH264eVepu580Tune_t {
    HalH264eVepu580Ctx  *ctx;

    /* motion and texture statistic of previous frames */
    RK_S32  curr_scene_motion_flag;
    /* motion and texture statistic of previous frames */
    RK_S32  ap_motion_flag;
    // level: 0~2: 0 <--> static, 1 <-->medium motion, 2 <--> large motion
    RK_S32  md_madp[MD_WIN_LEN];
    // level: 0~2: 0 <--> simple texture, 1 <--> medium texture, 2 <--> complex texture
    RK_S32  txtr_madi[FRAME_CONTENT_ANALYSIS_NUM];
    RK_S32  scene_motion_flag_matrix[FRAME_MOTION_ANALYSIS_NUM];
    RK_S32  md_flag_matrix[MD_SHOW_LEN];

    RK_S32  pre_madp[2];
    RK_S32  pre_madi[2];
} HalH264eVepu580Tune;

static RK_S32 mb_avg_madp_thd[6] = {192, 128, 64, 192, 128, 64};

RK_S32 ctu_madp_cnt_thd[6][8] = {
    {50, 100, 130, 50, 100, 550, 500, 550},
    {100, 150, 200, 80, 120, 500, 450, 550},
    {150, 200, 250, 100, 150, 450, 400, 450},
    {50, 100, 130, 50, 100, 550, 500, 550},
    {100, 150, 200, 80, 120, 500, 450, 550},
    {150, 200, 250, 100, 150, 450, 400, 450}
};

RK_S32 madp_num_map[5][4] = {
    {0, 0, 0, 1},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 0},
    {1, 1, 1, 1},
};

static RK_S32 atr_wgt[4][9] = {
    {22, 19, 16, 22, 19, 18, 22, 19, 16},
    {19, 19, 19, 19, 19, 19, 19, 19, 19},
    {22, 19, 16, 22, 19, 18, 22, 19, 16},
    {20, 20, 20, 20, 20, 20, 20, 20, 20},
};

static RK_S32 skip_atf_wgt[4][13] = {
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 intra_atf_wgt[4][12] = {
    {24, 22, 21, 22, 21, 20, 20, 19, 18, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {22, 21, 20, 21, 20, 19, 20, 19, 18, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 cime_multi[4][4] = {
    {4, 8, 24, 24},
    {4, 7, 20, 20},
    {4, 7, 20, 20},
    {4, 4, 4, 4},
};

static RK_S32 rime_multi[4][3] = {
    {4, 32, 128},
    {4, 16, 64},
    {4, 16, 64},
    {4, 4, 4},
};

static HalH264eVepu580Tune *vepu580_h264e_tune_init(HalH264eVepu580Ctx *ctx)
{
    HalH264eVepu580Tune *tune = mpp_malloc(HalH264eVepu580Tune, 1);
    RK_S32 scene_mode = ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC ? 0 : 1;

    if (NULL == tune)
        return tune;

    tune->ctx = ctx;
    tune->curr_scene_motion_flag = 0;
    tune->ap_motion_flag = scene_mode;
    memset(tune->md_madp, 0, sizeof(tune->md_madp));
    memset(tune->txtr_madi, 0, sizeof(tune->txtr_madi));
    memset(tune->md_flag_matrix, 0, sizeof(tune->md_flag_matrix));
    memset(tune->scene_motion_flag_matrix, 0, sizeof(tune->scene_motion_flag_matrix));
    tune->pre_madi[0] = tune->pre_madi[1] = -1;
    tune->pre_madp[0] = tune->pre_madp[1] = -1;

    return tune;
}

static void vepu580_h264e_tune_deinit(void *tune)
{
    MPP_FREE(tune);
}

static void vepu580_h264e_tune_reg_patch(void *p)
{
    HalH264eVepu580Tune *tune = (HalH264eVepu580Tune *)p;
    HalH264eVepu580Ctx *ctx = NULL;
    RK_S32 scene_mode = 0;

    if (NULL == tune)
        return;

    ctx = tune->ctx;
    scene_mode = ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC ? 0 : 1;

    H264eSlice *slice = ctx->slice;
    HalVepu580RegSet *regs = ctx->regs_set;
    tune->ap_motion_flag = scene_mode;
    RK_U32 scene_motion_flag = tune->ap_motion_flag * 2 + tune->curr_scene_motion_flag;

    if (scene_motion_flag > 3) {
        mpp_err_f("scene_motion_flag is a wrong value %d\n", scene_motion_flag);
        return;
    }

    /* modify register here */
    if (slice->slice_type != H264_I_SLICE) {
        RK_U32 *src = tune->curr_scene_motion_flag ? &h264e_klut_weight[0] : &h264e_klut_weight[4];
        memcpy(&regs->reg_rc_klut.klut_wgt0, src, CHROMA_KLUT_TAB_SIZE);
    }

    regs->reg_rc_klut.md_sad_thd.md_sad_thd0 = 4;
    regs->reg_rc_klut.md_sad_thd.md_sad_thd1 = 9;
    regs->reg_rc_klut.md_sad_thd.md_sad_thd2 = 15;
    regs->reg_rc_klut.madi_thd.madi_thd0 = 4;
    regs->reg_rc_klut.madi_thd.madi_thd1 = 9;
    regs->reg_rc_klut.madi_thd.madi_thd2 = 15;

    if (tune->curr_scene_motion_flag) {
        regs->reg_s3.lvl16_intra_UL_CST_THD.lvl16_intra_ul_cst_thld = 2501;
        regs->reg_s3.RDO_QUANT.quant_f_bias_P = 341;
        regs->reg_base.iprd_csts.vthd_y     = 0;
        regs->reg_base.iprd_csts.vthd_c     = 0;
        regs->reg_rc_klut.klut_ofst.chrm_klut_ofst = 0;
        regs->reg_base.rdo_cfg.atf_intra_e = 0;
        regs->reg_rdo.rdo_sqi_cfg.atf_pskip_en = 0;
        regs->reg_s3.ATR_THD1.atr_thdqp = 51;
        regs->reg_s3.cime_sqi_cfg.cime_pmv_set_zero = 0;
        regs->reg_s3.rime_sqi_thd.cime_sad_th0  = 0;
        regs->reg_s3.fme_sqi_thd0.cime_sad_pu16_th = 0;
        regs->reg_s3.fme_sqi_thd1.move_lambda = 8;
    }

    regs->reg_rdo.rdo_intra_cime_thd0.atf_rdo_intra_cime_thd0 = 28;
    regs->reg_rdo.rdo_intra_cime_thd0.atf_rdo_intra_cime_thd1 = 44;
    regs->reg_rdo.rdo_intra_cime_thd1.atf_rdo_intra_cime_thd2 = 72;
    regs->reg_rdo.rdo_intra_atf_wgt0.atf_rdo_intra_wgt00 = intra_atf_wgt[scene_motion_flag][0];
    regs->reg_rdo.rdo_intra_atf_wgt0.atf_rdo_intra_wgt01 = intra_atf_wgt[scene_motion_flag][1];
    regs->reg_rdo.rdo_intra_atf_wgt0.atf_rdo_intra_wgt02 = intra_atf_wgt[scene_motion_flag][2];
    regs->reg_rdo.rdo_intra_atf_wgt1.atf_rdo_intra_wgt10 = intra_atf_wgt[scene_motion_flag][3];
    regs->reg_rdo.rdo_intra_atf_wgt1.atf_rdo_intra_wgt11 = intra_atf_wgt[scene_motion_flag][4];
    regs->reg_rdo.rdo_intra_atf_wgt1.atf_rdo_intra_wgt12 = intra_atf_wgt[scene_motion_flag][5];
    regs->reg_rdo.rdo_intra_atf_wgt2.atf_rdo_intra_wgt20 = intra_atf_wgt[scene_motion_flag][6];
    regs->reg_rdo.rdo_intra_atf_wgt2.atf_rdo_intra_wgt21 = intra_atf_wgt[scene_motion_flag][7];
    regs->reg_rdo.rdo_intra_atf_wgt2.atf_rdo_intra_wgt22 = intra_atf_wgt[scene_motion_flag][8];
    regs->reg_rdo.rdo_intra_atf_wgt3.atf_rdo_intra_wgt30 = intra_atf_wgt[scene_motion_flag][9];
    regs->reg_rdo.rdo_intra_atf_wgt3.atf_rdo_intra_wgt31 = intra_atf_wgt[scene_motion_flag][10];
    regs->reg_rdo.rdo_intra_atf_wgt3.atf_rdo_intra_wgt32 = intra_atf_wgt[scene_motion_flag][11];

    regs->reg_rdo.rdo_skip_cime_thd0.atf_rdo_skip_cime_thd0 = 10;
    regs->reg_rdo.rdo_skip_cime_thd0.atf_rdo_skip_cime_thd1 = 8;
    regs->reg_rdo.rdo_skip_cime_thd1.atf_rdo_skip_cime_thd2 = 15;
    regs->reg_rdo.rdo_skip_cime_thd1.atf_rdo_skip_cime_thd3 = 25;
    regs->reg_rdo.rdo_skip_atf_wgt0.atf_rdo_skip_atf_wgt00 = skip_atf_wgt[scene_motion_flag][0];
    regs->reg_rdo.rdo_skip_atf_wgt0.atf_rdo_skip_atf_wgt10 = skip_atf_wgt[scene_motion_flag][1];
    regs->reg_rdo.rdo_skip_atf_wgt0.atf_rdo_skip_atf_wgt11 = skip_atf_wgt[scene_motion_flag][2];
    regs->reg_rdo.rdo_skip_atf_wgt0.atf_rdo_skip_atf_wgt12 = skip_atf_wgt[scene_motion_flag][3];
    regs->reg_rdo.rdo_skip_atf_wgt1.atf_rdo_skip_atf_wgt20 = skip_atf_wgt[scene_motion_flag][4];
    regs->reg_rdo.rdo_skip_atf_wgt1.atf_rdo_skip_atf_wgt21 = skip_atf_wgt[scene_motion_flag][5];
    regs->reg_rdo.rdo_skip_atf_wgt1.atf_rdo_skip_atf_wgt22 = skip_atf_wgt[scene_motion_flag][6];
    regs->reg_rdo.rdo_skip_atf_wgt2.atf_rdo_skip_atf_wgt30 = skip_atf_wgt[scene_motion_flag][7];
    regs->reg_rdo.rdo_skip_atf_wgt2.atf_rdo_skip_atf_wgt31 = skip_atf_wgt[scene_motion_flag][8];
    regs->reg_rdo.rdo_skip_atf_wgt2.atf_rdo_skip_atf_wgt32 = skip_atf_wgt[scene_motion_flag][9];
    regs->reg_rdo.rdo_skip_atf_wgt3.atf_rdo_skip_atf_wgt40 = skip_atf_wgt[scene_motion_flag][10];
    regs->reg_rdo.rdo_skip_atf_wgt3.atf_rdo_skip_atf_wgt41 = skip_atf_wgt[scene_motion_flag][11];
    regs->reg_rdo.rdo_skip_atf_wgt3.atf_rdo_skip_atf_wgt42 = skip_atf_wgt[scene_motion_flag][12];

    if (slice->slice_type != H264_I_SLICE) {
        regs->reg_s3.Lvl16_ATR_WGT.lvl16_atr_wgt0 = atr_wgt[scene_motion_flag][0];
        regs->reg_s3.Lvl16_ATR_WGT.lvl16_atr_wgt1 = atr_wgt[scene_motion_flag][1];
        regs->reg_s3.Lvl16_ATR_WGT.lvl16_atr_wgt2 = atr_wgt[scene_motion_flag][2];
        regs->reg_s3.Lvl8_ATR_WGT.lvl8_atr_wgt0 = atr_wgt[scene_motion_flag][3];
        regs->reg_s3.Lvl8_ATR_WGT.lvl8_atr_wgt1 = atr_wgt[scene_motion_flag][4];
        regs->reg_s3.Lvl8_ATR_WGT.lvl8_atr_wgt2 = atr_wgt[scene_motion_flag][5];
        regs->reg_s3.Lvl4_ATR_WGT.lvl4_atr_wgt0 = atr_wgt[scene_motion_flag][6];
        regs->reg_s3.Lvl4_ATR_WGT.lvl4_atr_wgt1 = atr_wgt[scene_motion_flag][7];
        regs->reg_s3.Lvl4_ATR_WGT.lvl4_atr_wgt2 = atr_wgt[scene_motion_flag][8];
    }

    regs->reg_s3.cime_sqi_multi0.cime_multi0 = cime_multi[scene_motion_flag][0];
    regs->reg_s3.cime_sqi_multi0.cime_multi1 = cime_multi[scene_motion_flag][1];
    regs->reg_s3.cime_sqi_multi1.cime_multi2 = cime_multi[scene_motion_flag][2];
    regs->reg_s3.cime_sqi_multi1.cime_multi3 = cime_multi[scene_motion_flag][3];

    regs->reg_s3.rime_sqi_multi.rime_multi0 = rime_multi[scene_motion_flag][0];
    regs->reg_s3.rime_sqi_multi.rime_multi1 = rime_multi[scene_motion_flag][1];
    regs->reg_s3.rime_sqi_multi.rime_multi2 = rime_multi[scene_motion_flag][2];
}

static void vepu580_h264e_tune_stat_update(void *p, HalEncTask *task)
{
    HalH264eVepu580Tune *tune = (HalH264eVepu580Tune *)p;
    HalH264eVepu580Ctx *ctx = NULL;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    RK_S32 scene_mode = 0;

    if (NULL == tune)
        return;

    ctx = tune->ctx;
    scene_mode = ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC ? 0 : 1;
    tune->ap_motion_flag = scene_mode;
    /* update statistic info here */
    RK_S32 mb_num = 0;
    RK_S32 madp = 0;
    RK_S32 md_flag = 0;
    RK_S32 nScore = 0;
    RK_S32 j;
    RK_S32 nScoreT = ((MD_WIN_LEN - 2) * 6 + 2 * 8 + 2 * 11 + 2 * 13) / 2;
    RK_S32 i = 0;
    RK_S32 mvbit = 10;
    RK_S32 madp_cnt_statistics[5];
    HalVepu580RegSet *regs = &ctx->regs_sets[task->flags.reg_idx];

    for (i = 0; i < 5; i++) {
        madp_cnt_statistics[i] = regs->reg_st.md_sad_b16num0 * madp_num_map[i][0] +
                                 regs->reg_st.md_sad_b16num1 * madp_num_map[i][1] +
                                 regs->reg_st.md_sad_b16num2 * madp_num_map[i][2] +
                                 regs->reg_st.md_sad_b16num3 * madp_num_map[i][3];
    }

    rc_info->madi =
        tune->pre_madi[0] = (!regs->reg_st.st_bnum_b16.num_b16) ? 0 :
                            regs->reg_st.madi /  regs->reg_st.st_bnum_b16.num_b16;
    rc_info->madp =
        tune->pre_madp[0] = (!regs->reg_st.st_bnum_cme.num_ctu) ? 0 :
                            regs->reg_st.madp / regs->reg_st.st_bnum_cme.num_ctu;

    mb_num = regs->reg_st.madi_b16num0 + regs->reg_st.madi_b16num1 +
             regs->reg_st.madi_b16num2 + regs->reg_st.madi_b16num3;
    mb_num = mb_num ? mb_num : 1;
    if (0 != tune->ap_motion_flag)
        mvbit = 15;

    madp = MOTION_LEVEL_STILL;
    if (0 != madp_cnt_statistics[4]) {
        RK_S32 base = tune->ap_motion_flag * 3;

        for (i = 0; i < 3; i++, base++) {
            if (tune->pre_madp[0] >= mb_avg_madp_thd[base]) {
                if (madp_cnt_statistics[0] > mb_num * ctu_madp_cnt_thd[base][0] >> mvbit ||
                    madp_cnt_statistics[1] > mb_num * ctu_madp_cnt_thd[base][1] >> mvbit ||
                    madp_cnt_statistics[2] > mb_num * ctu_madp_cnt_thd[base][2] >> mvbit) {
                    madp =  MOTION_LEVEL_BIG_MOTION;
                } else if ((madp_cnt_statistics[0] > mb_num * ctu_madp_cnt_thd[base][3] >> mvbit ||
                            madp_cnt_statistics[1] > mb_num * ctu_madp_cnt_thd[base][4] >> mvbit) &&
                           madp_cnt_statistics[3] < mb_num * ctu_madp_cnt_thd[base][5] >> mvbit) {
                    madp =  MOTION_LEVEL_BIG_MOTION;
                } else if (madp_cnt_statistics[3] < mb_num * ctu_madp_cnt_thd[base][6] >> mvbit) {
                    madp =  MOTION_LEVEL_BIG_MOTION;
                } else if (madp_cnt_statistics[3] < mb_num * ctu_madp_cnt_thd[base][7] >> mvbit) {
                    madp =  MOTION_LEVEL_MOTION;
                }
                break;
            }
        }
    } else {
        madp = MOTION_LEVEL_UNKNOW_SCENE;
    }

    if (MOTION_LEVEL_UNKNOW_SCENE != madp) {
        nScore = madp * 13 + tune->md_madp[0] * 11 + tune->md_madp[1] * 8;
    } else {
        nScore = tune->md_madp[0] * 11 + tune->md_madp[1] * 8;
        nScoreT -= 13;
    }

    for (j = 2; j < MD_WIN_LEN; j++) {
        nScore += tune->md_madp[j] * 6;
    }

    if (nScore >= nScoreT) {
        md_flag = 1;
    }

    tune->curr_scene_motion_flag = 0;
    if (tune->md_flag_matrix[0] && tune->md_flag_matrix[1] && tune->md_flag_matrix[2]) {
        tune->curr_scene_motion_flag = 1;
    } else if ((tune->md_flag_matrix[0] && tune->md_flag_matrix[1]) || (tune->md_flag_matrix[1] && tune->md_flag_matrix[2] && tune->md_flag_matrix[3])) {
        tune->curr_scene_motion_flag = md_flag;
    }

    if (MOTION_LEVEL_UNKNOW_SCENE != madp) {
        for (j = MD_WIN_LEN - 2; j >= 0; j--) {
            tune->md_madp[j + 1] = tune->md_madp[j];
        }
        tune->md_madp[0] = madp;
    }
    for (j = MD_SHOW_LEN - 2; j >= 0;  j--) {
        tune->md_flag_matrix[j + 1] = tune->md_flag_matrix[j];
    }

    tune->md_flag_matrix[0] = md_flag;

    for (j = FRAME_MOTION_ANALYSIS_NUM - 2; j >= 0;  j--) {
        tune->scene_motion_flag_matrix[j + 1] = tune->scene_motion_flag_matrix[j];
    }
    tune->scene_motion_flag_matrix[0] = tune->curr_scene_motion_flag;

    tune->pre_madi[1] = tune->pre_madi[0];
    tune->pre_madp[1] = tune->pre_madp[0];
}
