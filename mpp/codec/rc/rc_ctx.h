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
#ifndef __RC_CTX_H__
#define __RC_CTX_H__

#include "mpp_rc_api.h"
#include "rc_base.h"

typedef struct RcModelV2Ctx_t {
    RcCfg           usr_cfg;
    EncRcTaskInfo   hal_cfg;

    RK_U32          frame_type;
    RK_U32          last_frame_type;
    RK_S64          gop_total_bits;
    RK_U32          bit_per_frame;
    RK_U32          first_frm_flg;
    RK_S64          avg_gbits;
    RK_S64          real_gbits;

    MppDataV2       *i_bit;
    RK_U32          i_sumbits;
    RK_U32          i_scale;

    MppDataV2       *idr_bit;
    RK_U32          idr_sumbits;
    RK_U32          idr_scale;

    MppDataV2       *vi_bit;
    RK_U32          vi_sumbits;
    RK_U32          vi_scale;
    MppDataV2       *p_bit;
    RK_U32          p_sumbits;
    RK_U32          p_scale;

    MppDataV2       *pre_p_bit;
    MppDataV2       *pre_i_bit;
    MppDataV2       *pre_i_mean_qp;
    MppDataV2       *madi;
    MppDataV2       *madp;

    RK_S32          target_bps;
    RK_S32          pre_target_bits;
    RK_S32          pre_real_bits;
    RK_S32          frm_bits_thr;
    RK_S32          ins_bps;
    RK_S32          last_inst_bps;

    RK_U32          motion_sensitivity;
    RK_U32          min_still_percent;
    RK_U32          max_still_qp;
    RK_S32          moving_ratio;
    RK_U32          pre_mean_qp;
    RK_U32          pre_i_scale;
    RK_S32          cur_super_thd;
    MppDataV2       *stat_bits;
    MppDataV2       *gop_bits;
    MppDataV2       *stat_rate;
    RK_S32          watl_thrd;
    RK_S32          stat_watl;
    RK_S32          watl_base;

    RK_S32          next_i_ratio;      // scale 64
    RK_S32          next_ratio;        // scale 64
    RK_S32          pre_i_qp;
    RK_S32          pre_p_qp;
    RK_U32          scale_qp;          // scale 64
    MppDataV2       *means_qp;
    RK_U32          frm_num;

    /* qp decision */
    RK_S32          cur_scale_qp;
    RK_S32          start_qp;
    RK_S32          prev_quality;
    RK_S32          prev_md_prop;
    RK_S32          gop_qp_sum;
    RK_S32          gop_frm_cnt;
    RK_S32          pre_iblk4_prop;

    RK_S32          reenc_cnt;
    RK_U32          drop_cnt;
    RK_S32          on_drop;
    RK_S32          on_pskip;
    RK_S32          qp_layer_id;
    RK_S32          hier_frm_cnt[4];

    MPP_RET         (*calc_ratio)(void* ctx, EncRcTaskInfo *cfg);
    MPP_RET         (*re_calc_ratio)(void* ctx, EncRcTaskInfo *cfg);
} RcModelV2Ctx;


#ifdef  __cplusplus
extern "C" {
#endif

/* basic helper function */
MPP_RET bits_model_init(RcModelV2Ctx *ctx);
MPP_RET bits_model_deinit(RcModelV2Ctx *ctx);

MPP_RET bits_model_alloc(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg, RK_S64 total_bits);
MPP_RET bits_model_update(RcModelV2Ctx *ctx, RK_S32 real_bit, RK_U32 madi);

MPP_RET calc_next_i_ratio(RcModelV2Ctx *ctx);
MPP_RET check_re_enc(RcModelV2Ctx *ctx, EncRcTaskInfo *cfg);

#ifdef  __cplusplus
}
#endif

#endif /* __RC_CTX_H__ */
