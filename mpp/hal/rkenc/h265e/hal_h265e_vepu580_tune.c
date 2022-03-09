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

#define HAL_H265E_DBG_CONTENT           (0x00200000)
#define hal_h264e_dbg_content(fmt, ...) hal_h264e_dbg_f(HAL_H264E_DBG_CONTENT, fmt, ## __VA_ARGS__)

/*
 * Please follow the configuration below:
 *
 * FRAME_CONTENT_ANALYSIS_NUM >= 5
 * MD_WIN_LEN >= 3
 * MD_SHOW_LEN == 4
 */

typedef struct HalH265eVepu580Tune_t {
    H265eV580HalContext  *ctx;

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
} HalH265eVepu580Tune;

static RK_S32 ctu_avg_madp_thd[6] = {896, 640, 384, 896, 640, 384};

static RK_U8 lvl32_preintra_cst_wgt[4][8] = {
    {23, 22, 21, 20, 22, 24, 26, 16},
    {19, 18, 17, 16, 18, 20, 21, 16},
    {20, 19, 18, 17, 19, 21, 22, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_U8 lvl16_preintra_cst_wgt[4][8] = {
    {23, 22, 21, 20, 22, 24, 26, 16},
    {19, 18, 17, 16, 18, 20, 21, 16},
    {20, 19, 18, 17, 19, 21, 22, 16},
    {16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 skip_b64_atf_wgt[4][13] = {
    {16, 13, 14, 15, 14, 14, 15, 15, 15, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 14, 15, 16, 14, 14, 15, 15, 15, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 intra_b32_atf_wgt[4][12] = {
    {26, 25, 25, 25, 24, 23, 21, 19, 18, 16, 16, 16},
    {21, 20, 19, 20, 19, 18, 19, 18, 18, 18, 18, 17},
    {20, 19, 18, 19, 18, 17, 18, 17, 17, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 skip_b32_atf_wgt[4][13] = {
    {18, 13, 14, 15, 14, 14, 15, 15, 15, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {18, 14, 14, 15, 14, 14, 15, 15, 15, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 intra_b16_atf_wgt[4][12] = {
    {26, 25, 25, 25, 24, 23, 21, 19, 18, 16, 16, 16},
    {21, 20, 19, 20, 19, 18, 19, 18, 18, 18, 18, 17},
    {20, 19, 18, 19, 18, 17, 18, 17, 17, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 skip_b16_atf_wgt[4][13] = {
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 intra_b8_atf_wgt[4][12] = {
    {26, 25, 25, 25, 24, 23, 21, 19, 18, 16, 16, 16},
    {21, 20, 19, 20, 19, 18, 19, 18, 18, 18, 18, 17},
    {20, 19, 18, 19, 18, 17, 18, 17, 17, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_S32 skip_b8_atf_wgt[4][13] = {
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_U32 intra_lvl16_sobel_a[4][9] = {
    {32, 32, 32, 32, 32, 32, 32, 32, 32},
    {16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_U32 intra_lvl16_sobel_c[4][9] = {
    {13, 13, 13, 13, 13, 13, 13, 13, 13},
    {16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 16, 16, 16},
};

static RK_U32 intra_lvl16_sobel_d[4][9] = {
    {23750, 23750, 23750, 23750, 23750, 23750, 23750, 23750, 23750},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static RK_U32 intra_lvl32_sobel_a[4][5] = {
    {18, 18, 18, 18, 18},
    {16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16},
};

static RK_U32 intra_lvl32_sobel_c[4][5] = {
    {16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16},
};

static RK_U32 qnt_bias_i[4] = {
    171, 171, 171, 171
};

static RK_U32 qnt_bias_p[4] = {
    85, 85, 171, 171
};

static RK_U32 rime_sqi_cime_sad_th[4] = {
    48, 0, 0, 0
};

static RK_U32 fme_sqi_cime_sad_pu16_th[4] = {
    16, 0, 0, 0
};

static RK_U32 fme_sqi_cime_sad_pu32_th[4] = {
    16, 0, 0, 0
};

static RK_U32 fme_sqi_cime_sad_pu64_th[4] = {
    16, 0, 0, 0
};

static RK_U32 chrm_klut_ofst[4] = {
    3, 0, 0, 0
};

static RK_S32 pre_intra_b32_cost[4][2] = {
    {31, 30},
    {23, 20},
    {31, 30},
    {23, 20},
};

static RK_S32 pre_intra_b16_cost[4][2] = {
    {31, 30},
    {23, 20},
    {31, 30},
    {23, 20},
};

static RK_S32 cime_multi[4][4] = {
    {4, 8, 24, 24},
    {4, 7, 20, 20},
    {4, 8, 24, 24},
    {4, 4, 4, 4},
};

static RK_S32 rime_multi[4][3] = {
    {4, 32, 128},
    {4, 16, 64},
    {4, 32, 128},
    {4, 4, 4},
};

static HalH265eVepu580Tune *vepu580_h265e_tune_init(H265eV580HalContext *ctx)
{
    HalH265eVepu580Tune *tune = mpp_malloc(HalH265eVepu580Tune, 1);
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

static void vepu580_h265e_tune_deinit(void *tune)
{
    MPP_FREE(tune);
}

static void vepu580_h265e_tune_reg_patch(void *p)
{
    HalH265eVepu580Tune *tune = (HalH265eVepu580Tune *)p;
    H265eV580HalContext *ctx = NULL;
    RK_S32 scene_mode = 0;

    if (NULL == tune)
        return;

    ctx = tune->ctx;
    scene_mode = ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC ? 0 : 1;
    tune->ap_motion_flag = scene_mode;
    /* modify register here */
    H265eV580RegSet *regs = (H265eV580RegSet *)ctx->regs[0];
    hevc_vepu580_rc_klut *rc_regs =  &regs->reg_rc_klut;
    hevc_vepu580_wgt *reg_wgt = &regs->reg_wgt;
    vepu580_rdo_cfg  *reg_rdo = &regs->reg_rdo;
    RdoAtfSkipCfg *p_rdo_atf_skip;
    RdoAtfCfg* p_rdo_atf;
    RK_U32 scene_motion_flag = tune->ap_motion_flag * 2 + tune->curr_scene_motion_flag;

    if (scene_motion_flag > 3) {
        mpp_err_f("scene_motion_flag is a wrong value %d\n", scene_motion_flag);
        return;
    }

    memcpy(&reg_wgt->lvl32_intra_CST_WGT0, lvl32_preintra_cst_wgt[scene_motion_flag], sizeof(lvl32_preintra_cst_wgt[scene_motion_flag]));
    memcpy(&reg_wgt->lvl16_intra_CST_WGT0, lvl16_preintra_cst_wgt[scene_motion_flag], sizeof(lvl16_preintra_cst_wgt[scene_motion_flag]));

    p_rdo_atf_skip = &reg_rdo->rdo_b64_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 2;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 4;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 6;

    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = skip_b64_atf_wgt[scene_motion_flag][0];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = skip_b64_atf_wgt[scene_motion_flag][1];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = skip_b64_atf_wgt[scene_motion_flag][2];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = skip_b64_atf_wgt[scene_motion_flag][3];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = skip_b64_atf_wgt[scene_motion_flag][4];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = skip_b64_atf_wgt[scene_motion_flag][5];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = skip_b64_atf_wgt[scene_motion_flag][6];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = skip_b64_atf_wgt[scene_motion_flag][7];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = skip_b64_atf_wgt[scene_motion_flag][8];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = skip_b64_atf_wgt[scene_motion_flag][9];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = skip_b64_atf_wgt[scene_motion_flag][10];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = skip_b64_atf_wgt[scene_motion_flag][11];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = skip_b64_atf_wgt[scene_motion_flag][12];

    p_rdo_atf = &reg_rdo->rdo_b32_intra_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 48;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 64;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = intra_b32_atf_wgt[scene_motion_flag][0];
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = intra_b32_atf_wgt[scene_motion_flag][1];
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = intra_b32_atf_wgt[scene_motion_flag][2];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = intra_b32_atf_wgt[scene_motion_flag][3];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = intra_b32_atf_wgt[scene_motion_flag][4];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = intra_b32_atf_wgt[scene_motion_flag][5];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = intra_b32_atf_wgt[scene_motion_flag][6];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = intra_b32_atf_wgt[scene_motion_flag][7];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = intra_b32_atf_wgt[scene_motion_flag][8];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = intra_b32_atf_wgt[scene_motion_flag][9];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = intra_b32_atf_wgt[scene_motion_flag][10];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = intra_b32_atf_wgt[scene_motion_flag][11];

    p_rdo_atf_skip = &reg_rdo->rdo_b32_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 =  12;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 =  2;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 =  4;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 =  6;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  =  skip_b32_atf_wgt[scene_motion_flag][0];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  =  skip_b32_atf_wgt[scene_motion_flag][1];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  =  skip_b32_atf_wgt[scene_motion_flag][2];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  =  skip_b32_atf_wgt[scene_motion_flag][3];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  =  skip_b32_atf_wgt[scene_motion_flag][4];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  =  skip_b32_atf_wgt[scene_motion_flag][5];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  =  skip_b32_atf_wgt[scene_motion_flag][6];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  =  skip_b32_atf_wgt[scene_motion_flag][7];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  =  skip_b32_atf_wgt[scene_motion_flag][8];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  =  skip_b32_atf_wgt[scene_motion_flag][9];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  =  skip_b32_atf_wgt[scene_motion_flag][10];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  =  skip_b32_atf_wgt[scene_motion_flag][11];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  =  skip_b32_atf_wgt[scene_motion_flag][12];

    p_rdo_atf = &reg_rdo->rdo_b16_intra_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 48;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 64;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = intra_b16_atf_wgt[scene_motion_flag][0];
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = intra_b16_atf_wgt[scene_motion_flag][1];
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = intra_b16_atf_wgt[scene_motion_flag][2];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = intra_b16_atf_wgt[scene_motion_flag][3];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = intra_b16_atf_wgt[scene_motion_flag][4];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = intra_b16_atf_wgt[scene_motion_flag][5];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = intra_b16_atf_wgt[scene_motion_flag][6];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = intra_b16_atf_wgt[scene_motion_flag][7];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = intra_b16_atf_wgt[scene_motion_flag][8];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = intra_b16_atf_wgt[scene_motion_flag][9];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = intra_b16_atf_wgt[scene_motion_flag][10];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = intra_b16_atf_wgt[scene_motion_flag][11];

    p_rdo_atf_skip = &reg_rdo->rdo_b16_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 48;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 96;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = skip_b16_atf_wgt[scene_motion_flag][0];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = skip_b16_atf_wgt[scene_motion_flag][1];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = skip_b16_atf_wgt[scene_motion_flag][2];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = skip_b16_atf_wgt[scene_motion_flag][3];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = skip_b16_atf_wgt[scene_motion_flag][4];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = skip_b16_atf_wgt[scene_motion_flag][5];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = skip_b16_atf_wgt[scene_motion_flag][6];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = skip_b16_atf_wgt[scene_motion_flag][7];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = skip_b16_atf_wgt[scene_motion_flag][8];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = skip_b16_atf_wgt[scene_motion_flag][9];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = skip_b16_atf_wgt[scene_motion_flag][10];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = skip_b16_atf_wgt[scene_motion_flag][11];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = skip_b16_atf_wgt[scene_motion_flag][12];

    p_rdo_atf = &reg_rdo->rdo_b8_intra_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 48;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 64;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = intra_b8_atf_wgt[scene_motion_flag][0];
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = intra_b8_atf_wgt[scene_motion_flag][1];
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = intra_b8_atf_wgt[scene_motion_flag][2];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = intra_b8_atf_wgt[scene_motion_flag][3];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = intra_b8_atf_wgt[scene_motion_flag][4];
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = intra_b8_atf_wgt[scene_motion_flag][5];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = intra_b8_atf_wgt[scene_motion_flag][6];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = intra_b8_atf_wgt[scene_motion_flag][7];
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = intra_b8_atf_wgt[scene_motion_flag][8];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = intra_b8_atf_wgt[scene_motion_flag][9];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = intra_b8_atf_wgt[scene_motion_flag][10];
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = intra_b8_atf_wgt[scene_motion_flag][11];

    p_rdo_atf_skip = &reg_rdo->rdo_b8_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 48;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 96;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = skip_b8_atf_wgt[scene_motion_flag][0];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = skip_b8_atf_wgt[scene_motion_flag][1];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = skip_b8_atf_wgt[scene_motion_flag][2];
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = skip_b8_atf_wgt[scene_motion_flag][3];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = skip_b8_atf_wgt[scene_motion_flag][4];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = skip_b8_atf_wgt[scene_motion_flag][5];
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = skip_b8_atf_wgt[scene_motion_flag][6];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = skip_b8_atf_wgt[scene_motion_flag][7];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = skip_b8_atf_wgt[scene_motion_flag][8];
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = skip_b8_atf_wgt[scene_motion_flag][9];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = skip_b8_atf_wgt[scene_motion_flag][10];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = skip_b8_atf_wgt[scene_motion_flag][11];
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = skip_b8_atf_wgt[scene_motion_flag][12];

    reg_rdo->preintra_b32_cst_wgt.pre_intra32_cst_wgt00 = pre_intra_b32_cost[scene_motion_flag][0];
    reg_rdo->preintra_b32_cst_wgt.pre_intra32_cst_wgt01 = pre_intra_b32_cost[scene_motion_flag][1];
    reg_rdo->preintra_b16_cst_wgt.pre_intra16_cst_wgt00 = pre_intra_b16_cost[scene_motion_flag][0];
    reg_rdo->preintra_b16_cst_wgt.pre_intra16_cst_wgt01 = pre_intra_b16_cost[scene_motion_flag][1];

    rc_regs->md_sad_thd.md_sad_thd0 = 4;
    rc_regs->md_sad_thd.md_sad_thd1 = 9;
    rc_regs->md_sad_thd.md_sad_thd2 = 15;
    rc_regs->madi_thd.madi_thd0     = 4;
    rc_regs->madi_thd.madi_thd1     = 9;
    rc_regs->madi_thd.madi_thd2     = 15;

    reg_wgt->cime_sqi_cfg.cime_pmv_set_zero = !tune->curr_scene_motion_flag;
    reg_wgt->cime_sqi_multi0.cime_multi0 = cime_multi[scene_motion_flag][0];
    reg_wgt->cime_sqi_multi0.cime_multi1 = cime_multi[scene_motion_flag][1];
    reg_wgt->cime_sqi_multi1.cime_multi2 = cime_multi[scene_motion_flag][2];
    reg_wgt->cime_sqi_multi1.cime_multi3 = cime_multi[scene_motion_flag][3];

    reg_wgt->rime_sqi_multi.rime_multi0 = rime_multi[scene_motion_flag][0];
    reg_wgt->rime_sqi_multi.rime_multi1 = rime_multi[scene_motion_flag][1];
    reg_wgt->rime_sqi_multi.rime_multi2 = rime_multi[scene_motion_flag][2];

    if (tune->curr_scene_motion_flag) {
        reg_wgt->fme_sqi_thd1.move_lambda = 8;
    }

    reg_rdo->rdo_sqi_cfg.rdo_segment_en = !tune->curr_scene_motion_flag;
    reg_rdo->rdo_sqi_cfg.rdo_smear_en = !tune->curr_scene_motion_flag;

    reg_wgt->i16_sobel_a_00.intra_l16_sobel_a0_qp0 = intra_lvl16_sobel_a[scene_motion_flag][0];
    reg_wgt->i16_sobel_a_00.intra_l16_sobel_a0_qp1 = intra_lvl16_sobel_a[scene_motion_flag][1];
    reg_wgt->i16_sobel_a_00.intra_l16_sobel_a0_qp2 = intra_lvl16_sobel_a[scene_motion_flag][2];
    reg_wgt->i16_sobel_a_00.intra_l16_sobel_a0_qp3 = intra_lvl16_sobel_a[scene_motion_flag][3];
    reg_wgt->i16_sobel_a_00.intra_l16_sobel_a0_qp4 = intra_lvl16_sobel_a[scene_motion_flag][4];
    reg_wgt->i16_sobel_a_01.intra_l16_sobel_a0_qp5 = intra_lvl16_sobel_a[scene_motion_flag][5];
    reg_wgt->i16_sobel_a_01.intra_l16_sobel_a0_qp6 = intra_lvl16_sobel_a[scene_motion_flag][6];
    reg_wgt->i16_sobel_a_01.intra_l16_sobel_a0_qp7 = intra_lvl16_sobel_a[scene_motion_flag][7];
    reg_wgt->i16_sobel_a_01.intra_l16_sobel_a0_qp8 = intra_lvl16_sobel_a[scene_motion_flag][8];
    reg_wgt->i16_sobel_c_00.intra_l16_sobel_c0_qp0 = intra_lvl16_sobel_c[scene_motion_flag][0];
    reg_wgt->i16_sobel_c_00.intra_l16_sobel_c0_qp1 = intra_lvl16_sobel_c[scene_motion_flag][1];
    reg_wgt->i16_sobel_c_00.intra_l16_sobel_c0_qp2 = intra_lvl16_sobel_c[scene_motion_flag][2];
    reg_wgt->i16_sobel_c_00.intra_l16_sobel_c0_qp3 = intra_lvl16_sobel_c[scene_motion_flag][3];
    reg_wgt->i16_sobel_c_00.intra_l16_sobel_c0_qp4 = intra_lvl16_sobel_c[scene_motion_flag][4];
    reg_wgt->i16_sobel_c_01.intra_l16_sobel_c0_qp5 = intra_lvl16_sobel_c[scene_motion_flag][5];
    reg_wgt->i16_sobel_c_01.intra_l16_sobel_c0_qp6 = intra_lvl16_sobel_c[scene_motion_flag][6];
    reg_wgt->i16_sobel_c_01.intra_l16_sobel_c0_qp7 = intra_lvl16_sobel_c[scene_motion_flag][7];
    reg_wgt->i16_sobel_c_01.intra_l16_sobel_c0_qp8 = intra_lvl16_sobel_c[scene_motion_flag][8];
    reg_wgt->i16_sobel_d_00.intra_l16_sobel_d0_qp0 = intra_lvl16_sobel_d[scene_motion_flag][0];
    reg_wgt->i16_sobel_d_00.intra_l16_sobel_d0_qp1 = intra_lvl16_sobel_d[scene_motion_flag][1];
    reg_wgt->i16_sobel_d_01.intra_l16_sobel_d0_qp2 = intra_lvl16_sobel_d[scene_motion_flag][2];
    reg_wgt->i16_sobel_d_01.intra_l16_sobel_d0_qp3 = intra_lvl16_sobel_d[scene_motion_flag][3];
    reg_wgt->i16_sobel_d_02.intra_l16_sobel_d0_qp4 = intra_lvl16_sobel_d[scene_motion_flag][4];
    reg_wgt->i16_sobel_d_02.intra_l16_sobel_d0_qp5 = intra_lvl16_sobel_d[scene_motion_flag][5];
    reg_wgt->i16_sobel_d_03.intra_l16_sobel_d0_qp6 = intra_lvl16_sobel_d[scene_motion_flag][6];
    reg_wgt->i16_sobel_d_03.intra_l16_sobel_d0_qp7 = intra_lvl16_sobel_d[scene_motion_flag][7];
    reg_wgt->i16_sobel_d_04.intra_l16_sobel_d0_qp8 = intra_lvl16_sobel_d[scene_motion_flag][8];
    reg_wgt->i32_sobel_a.intra_l32_sobel_a1_qp0 = intra_lvl32_sobel_a[scene_motion_flag][0];
    reg_wgt->i32_sobel_a.intra_l32_sobel_a1_qp1 = intra_lvl32_sobel_a[scene_motion_flag][1];
    reg_wgt->i32_sobel_a.intra_l32_sobel_a1_qp2 = intra_lvl32_sobel_a[scene_motion_flag][2];
    reg_wgt->i32_sobel_a.intra_l32_sobel_a1_qp3 = intra_lvl32_sobel_a[scene_motion_flag][3];
    reg_wgt->i32_sobel_a.intra_l32_sobel_a1_qp4 = intra_lvl32_sobel_a[scene_motion_flag][4];
    reg_wgt->i32_sobel_c.intra_l32_sobel_c1_qp0 = intra_lvl32_sobel_c[scene_motion_flag][0];
    reg_wgt->i32_sobel_c.intra_l32_sobel_c1_qp1 = intra_lvl32_sobel_c[scene_motion_flag][1];
    reg_wgt->i32_sobel_c.intra_l32_sobel_c1_qp2 = intra_lvl32_sobel_c[scene_motion_flag][2];
    reg_wgt->i32_sobel_c.intra_l32_sobel_c1_qp3 = intra_lvl32_sobel_c[scene_motion_flag][3];
    reg_wgt->i32_sobel_c.intra_l32_sobel_c1_qp4 = intra_lvl32_sobel_c[scene_motion_flag][4];

    reg_wgt->reg1484_qnt_bias_comb.qnt_bias_i = qnt_bias_i[scene_motion_flag];
    reg_wgt->reg1484_qnt_bias_comb.qnt_bias_p = qnt_bias_p[scene_motion_flag];
    reg_wgt->rime_sqi_thd.cime_sad_th0 = rime_sqi_cime_sad_th[scene_motion_flag];
    reg_wgt->fme_sqi_thd0.cime_sad_pu16_th = fme_sqi_cime_sad_pu16_th[scene_motion_flag];
    reg_wgt->fme_sqi_thd0.cime_sad_pu32_th = fme_sqi_cime_sad_pu32_th[scene_motion_flag];
    reg_wgt->fme_sqi_thd1.cime_sad_pu64_th = fme_sqi_cime_sad_pu64_th[scene_motion_flag];
    rc_regs->klut_ofst.chrm_klut_ofst = chrm_klut_ofst[scene_motion_flag];
}

static void vepu580_h265e_tune_stat_update(void *p)
{
    HalH265eVepu580Tune *tune = (HalH265eVepu580Tune *)p;
    H265eV580HalContext *ctx = NULL;
    RK_S32 scene_mode = 0;

    if (NULL == tune)
        return;

    ctx = tune->ctx;
    scene_mode = ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC ? 0 : 1;
    tune->ap_motion_flag = scene_mode;
    /* update statistic info here */
    RK_S32 j;
    RK_S32 i = 0;
    RK_S32 mvbit = 10;
    vepu580_h265_fbk *fb = &ctx->feedback;

    for (i = 0; i < (RK_S32)ctx->tile_num; i++) {
        H265eV580StatusElem *elem = (H265eV580StatusElem *)ctx->reg_out[i];
        fb->st_md_sad_b16num0 += elem->st.md_sad_b16num0;
        fb->st_md_sad_b16num1 += elem->st.md_sad_b16num1;
        fb->st_md_sad_b16num2 += elem->st.md_sad_b16num2;
        fb->st_md_sad_b16num3 += elem->st.md_sad_b16num3;
        fb->st_madi_b16num0 += elem->st.madi_b16num0;
        fb->st_madi_b16num1 += elem->st.madi_b16num1;
        fb->st_madi_b16num2 += elem->st.madi_b16num2;
        fb->st_madi_b16num3 += elem->st.madi_b16num3;
    }

    RK_S32 mb_num = fb->st_mb_num ? fb->st_mb_num : 1;
    RK_S32 madp = 0;
    RK_S32 md_flag = 0;
    RK_S32 nScore = 0;
    RK_S32 nScoreT = ((MD_WIN_LEN - 2) * 6 + 2 * 8 + 2 * 11 + 2 * 13) / 2;
    RK_S32 madp_cnt_statistics[5];
    for (i = 0; i < 5; i++) {
        madp_cnt_statistics[i] = fb->st_md_sad_b16num0 * madp_num_map[i][0] + fb->st_md_sad_b16num1 * madp_num_map[i][1]
                                 + fb->st_md_sad_b16num2 * madp_num_map[i][2] + fb->st_md_sad_b16num3 * madp_num_map[i][3];
    }

    tune->pre_madi[0] = fb->st_madi;
    tune->pre_madp[0] = fb->st_madp;

    if (0 != tune->ap_motion_flag)
        mvbit = 15;

    madp = MOTION_LEVEL_STILL;
    if (0 != madp_cnt_statistics[4]) {
        RK_S32 base = tune->ap_motion_flag * 3;

        for (i = 0; i < 3; i++, base++) {
            if (tune->pre_madp[0] >= ctu_avg_madp_thd[i]) {
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
