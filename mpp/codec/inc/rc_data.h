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

#ifndef __RC_DATA_H__
#define __RC_DATA_H__

#include "mpp_rc_defs.h"
#include "rc_hal.h"

typedef enum RcDataIndexType_e {
    RC_DATA_SEQ_ID,
    RC_DATA_FRM_TYPE,
    RC_DATA_TID,
    RC_DATA_INDEX_BUTT,
} RcDataIndexType;

#define RC_FRM_TYPE_I           0
#define RC_FRM_TYPE_P           1

/*
 * base_cnt  - rate control base data storage number
 * extra_cnt - rate control extra data storage number
 */
typedef struct DataGroupCfg_t {
    RK_S32          base_cnt;
    RK_S32          extra_cnt;
} DataGroupCfg;

typedef void* DataGroup;
typedef void* RcData;

#ifdef __cplusplus
extern "C" {
#endif

/* rc data group API */
MPP_RET rc_data_init(DataGroup *grp, DataGroupCfg *cfg);
MPP_RET rc_data_deinit(DataGroup grp);
MPP_RET rc_data_reset(DataGroup grp);

RcData rc_data_get(DataGroup grp, RK_S32 seq_id);
RcData rc_data_index_type(DataGroup grp, RK_S32 type, RK_S32 whence, RK_S32 offset);
RcData rc_data_index_tid(DataGroup grp, RK_S32 tid, RK_S32 whence, RK_S32 offset);

RcData rc_data_next(RcData data, RcDataIndexType type);
RcData rc_data_prev(RcData data, RcDataIndexType type);

/* use const to ensure user will NOT change the data in RcData */
const RK_S32 *rc_data_get_seq_id(RcData data);
const RK_S32 *rc_data_get_qp_sum(RcData data);
const RK_S32 *rc_data_get_strm_size(RcData data);
const EncFrmStatus *rc_data_get_frm_status(RcData data);
const RcHalSet *rc_data_get_hal_set(RcData data);
const RcHalRet *rc_data_get_hal_ret(RcData data);

#ifdef __cplusplus
}
#endif

#endif /* __RC_DATA_H__ */
