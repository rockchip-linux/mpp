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
 *    size    - max valid data number
 *    pos_pw  - current data preset write position
 *    pos_w   - current data write position
 *    pos_r   - current data read position
 *    val     - buffer array pointer
 *
 *    When statistic length is less than 8 use direct save mode which will move
 *    all the data on each update.
 *    When statistic length is larger than 8 use loop save mode which will
 *    cyclically reuse the data position.
 */
typedef struct MppDataV2_t {
    RK_S32  size;
    RK_S32  pos_r;
    RK_S32  pos_pw;
    RK_S32  pos_w;
    RK_S32  pos_ahead;
    RK_S64  sum;
    RK_S32  val[];
} MppDataV2;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_data_init_v2(MppDataV2 **p, RK_S32 len, RK_S32 value);
void mpp_data_deinit_v2(MppDataV2 *p);
void mpp_data_reset_v2(MppDataV2 *p, RK_S32 val);
void mpp_data_preset_v2(MppDataV2 *p, RK_S32 val);
void mpp_data_update_v2(MppDataV2 *p, RK_S32 val);
RK_S32 mpp_data_sum_v2(MppDataV2 *p);
RK_S32 mpp_data_mean_v2(MppDataV2 *p);
RK_S32 mpp_data_get_pre_val_v2(MppDataV2 *p, RK_S32 idx);
RK_S32 mpp_data_sum_with_ratio_v2(MppDataV2 *p, RK_S32 len, RK_S32 num, RK_S32 denorm);

#ifdef __cplusplus
}
#endif

#endif /* __RC_BASE_H__ */
