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

#define HAL_H265E_DBG_CONTENT           (0x00200000)
#define hal_h264e_dbg_content(fmt, ...) hal_h264e_dbg_f(HAL_H264E_DBG_CONTENT, fmt, ## __VA_ARGS__)

/*
 * Please follow the configuration below:
 *
 * FRAME_CONTENT_ANALYSIS_NUM >= 5
 * MD_WIN_LEN >= 3
 * MD_SHOW_LEN == 4
 */
#define FRAME_CONTENT_ANALYSIS_NUM  5
#define MD_WIN_LEN                  5
#define MD_SHOW_LEN                 4

typedef struct HalH265eVepu580Tune_t {
    H265eV580HalContext  *ctx;

    /* motion and texture statistic of previous frames */
    RK_S32  curr_scene_motion_flag;
    // level: 0~2: 0 <--> static, 1 <-->medium motion, 2 <--> large motion
    RK_S32  md_madp[MD_WIN_LEN];
    // level: 0~2: 0 <--> simple texture, 1 <--> medium texture, 2 <--> complex texture
    RK_S32  txtr_madi[FRAME_CONTENT_ANALYSIS_NUM];
    RK_S32  md_flag_matrix[MD_SHOW_LEN];

    RK_S32  pre_madp[2];
    RK_S32  pre_madi[2];
} HalH265eVepu580Tune;

static HalH265eVepu580Tune *vepu580_h265e_tune_init(H265eV580HalContext *ctx)
{
    HalH265eVepu580Tune *tune = mpp_malloc(HalH265eVepu580Tune, 1);
    if (NULL == tune)
        return tune;

    tune->ctx = ctx;
    tune->curr_scene_motion_flag = 0;
    memset(tune->md_madp, 0, sizeof(tune->md_madp));
    memset(tune->txtr_madi, 0, sizeof(tune->txtr_madi));
    memset(tune->md_flag_matrix, 0, sizeof(tune->md_flag_matrix));
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

    if (NULL == tune)
        return;

    ctx = tune->ctx;
    (void)ctx;

    /* modify register here */
}

static void vepu580_h265e_tune_stat_update(void *p)
{
    HalH265eVepu580Tune *tune = (HalH265eVepu580Tune *)p;
    H265eV580HalContext *ctx = NULL;

    if (NULL == tune)
        return;

    ctx = tune->ctx;
    (void)ctx;

    /* update statistic info here */
}
