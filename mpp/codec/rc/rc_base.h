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

#ifndef __RC_BASE_H__
#define __RC_BASE_H__

#include "mpp_list.h"
#include "mpp_enc_cfg.h"
#include "mpp_rc.h"

/*
 * mpp rate control contain common caculation methd
 */

/*
 * 1. MppData - data statistic struct
 *    size  - max valid data number
 *    len   - valid data number
 *    pos_r - current data read position
 *    pos_w - current data write position
 *    val   - buffer array pointer
 *
 *    When statistic length is less than 8 use direct save mode which will move
 *    all the data on each update.
 *    When statistic length is larger than 8 use loop save mode which will
 *    cyclically reuse the data position.
 */
typedef struct MppDataV2_t {
    RK_S32  size;
    RK_S32  len;
    RK_S32  pos_w;
    RK_S32  pos_r;
    RK_S32  *val;
    RK_S64  sum;
} MppDataV2;

typedef struct RecordNodeV2_t {
    struct list_head list;
    ENC_FRAME_TYPE   frm_type;
    /* @frm_cnt starts from ONE */
    RK_U32           frm_cnt;
    RK_U32           bps;
    RK_U32           fps;
    RK_S32           gop;
    RK_S32           bits_per_pic;
    RK_S32           bits_per_intra;
    RK_S32           bits_per_inter;
    RK_U32           tgt_bits;
    RK_U32           bit_min;
    RK_U32           bit_max;
    RK_U32           real_bits;
    RK_S32           acc_intra_bits_in_fps;
    RK_S32           acc_inter_bits_in_fps;
    RK_S32           last_fps_bits;
    float            last_intra_percent;

    /* hardware result */
    RK_S32           qp_sum;
    RK_S64           sse_sum;
    RK_S32           set_qp;
    RK_S32           qp_min;
    RK_S32           qp_max;
    RK_S32           real_qp;
    RK_S32           wlen;
} RecordNodeV2;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_data_init_v2(MppDataV2 **p, RK_S32 len, RK_S32 value);
void mpp_data_deinit_v2(MppDataV2 *p);
void mpp_data_reset_v2(MppDataV2 *p, RK_S32 val);
void mpp_data_update_v2(MppDataV2 *p, RK_S32 val);
RK_S32 mpp_data_sum_v2(MppDataV2 *p);
RK_S32 mpp_data_mean_v2(MppDataV2 *p);
RK_S32 mpp_data_get_pre_val_v2(MppDataV2 *p, RK_S32 idx);
RK_S32 mpp_data_sum_with_ratio_v2(MppDataV2 *p, RK_S32 len, RK_S32 num, RK_S32 denorm);

#ifdef __cplusplus
}
#endif

#endif /* __RC_BASE_H__ */
