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

#define MODULE_TAG "vp8e_rc"

#include <math.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "rc_debug.h"
#include "rc_ctx.h"
#include "rc_model_v2.h"

#define UPSCALE 8000

static RK_S32 vp8_initial_qp(RK_S32 bits, RK_S32 pels)
{
    RK_S32 i = -1;
    static const RK_S32 qp_tbl[2][12] = {
        {47, 57, 73, 93, 122, 155, 214, 294, 373, 506, 781, 0x7FFFFFFF},
        {120, 110, 100, 90, 80, 70, 60, 50, 40, 30, 20, 10}
    };

    if (bits > 1000000)
        return 10;

    pels >>= 8;
    bits >>= 5;

    bits *= pels + 250;
    bits /= 350 + (3 * pels) / 4;
    bits = axb_div_c(bits, UPSCALE, pels << 6);

    while (qp_tbl[0][++i] < bits);

    return qp_tbl[1][i];
}

MPP_RET rc_model_v2_vp8_hal_start(void *ctx, EncRcTask *task)
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

    if (usr_cfg->mode == RC_FIXQP)
        return MPP_OK;

    /* setup quality parameters */
    if (p->first_frm_flg && frm->is_intra) {
        if (info->quality_target < 0) {
            if (info->bit_target) {
                p->start_qp = vp8_initial_qp(info->bit_target, mb_w * mb_h * 16 * 16);
                p->cur_scale_qp = (p->start_qp) << 6;
            } else {
                mpp_log("fix qp case but init qp no set");
                info->quality_target = 40;
                p->start_qp = 40;
                p->cur_scale_qp = (p->start_qp) << 6;
            }
        } else {
            p->start_qp = info->quality_target;
            p->cur_scale_qp = (p->start_qp) << 6;
        }

        if (p->reenc_cnt > 0) {
            p->cur_scale_qp += p->next_ratio;
            p->start_qp = p->cur_scale_qp >> 6;
            rc_dbg_rc("p->start_qp = %d, p->cur_scale_qp %d,p->next_ratio %d ", p->start_qp, p->cur_scale_qp, p->next_ratio);
        } else {
            p->start_qp -= usr_cfg->i_quality_delta;
        }
        p->cur_scale_qp = mpp_clip(p->cur_scale_qp, (info->quality_min << 6), (info->quality_max << 6));
        p->pre_i_qp = p->cur_scale_qp >> 6;
        p->pre_p_qp = p->cur_scale_qp >> 6;
    } else {
        RK_S32 qp_scale = p->cur_scale_qp + p->next_ratio;
        RK_S32 start_qp = 0;
        RK_S32 dealt_qp = 0;

        if (frm->is_intra) {
            qp_scale = mpp_clip(qp_scale, (info->quality_min << 6), (info->quality_max << 6));

            start_qp = ((p->pre_i_qp + ((qp_scale + p->next_i_ratio) >> 6)) >> 1);

            start_qp = mpp_clip(start_qp, info->quality_min, info->quality_max);
            p->pre_i_qp = start_qp;
            p->start_qp = start_qp;
            p->cur_scale_qp = qp_scale;

            if (usr_cfg->i_quality_delta && !p->reenc_cnt)
                p->start_qp -= dealt_qp;
        } else {
            qp_scale = mpp_clip(qp_scale, (info->quality_min << 6), (info->quality_max << 6));
            p->cur_scale_qp = qp_scale;
            p->start_qp = qp_scale >> 6;
            if (frm->ref_mode == REF_TO_PREV_INTRA && usr_cfg->vi_quality_delta) {
                p->start_qp -= usr_cfg->vi_quality_delta;
            }
        }
        rc_dbg_rc("i_quality_delta %d, vi_quality_delta %d", dealt_qp, usr_cfg->vi_quality_delta);
    }

    p->start_qp = mpp_clip(p->start_qp, info->quality_min, info->quality_max);
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

const RcImplApi default_vp8e = {
    "default",
    MPP_VIDEO_CodingVP8,
    sizeof(RcModelV2Ctx),
    rc_model_v2_init,
    rc_model_v2_deinit,
    NULL,
    rc_model_v2_check_reenc,
    rc_model_v2_start,
    rc_model_v2_end,
    rc_model_v2_vp8_hal_start,
    rc_model_v2_hal_end,
};
