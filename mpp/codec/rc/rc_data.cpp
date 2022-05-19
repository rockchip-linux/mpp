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

#define MODULE_TAG "rc_data"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "rc_data.h"
#include "rc_data_impl.h"

#define RC_FRM_STATUS_VALID     (0x00000001)
#define RC_HAL_SET_VALID        (0x00000002)
#define RC_HAL_RET_VALID        (0x00000004)

MPP_RET rc_data_init(DataGroup *grp, DataGroupCfg *cfg)
{
    if (NULL == grp || NULL == cfg ||
        cfg->base_cnt < 0 || cfg->extra_cnt < 0) {
        mpp_err_f("invalid data group %p cfg %p\n", grp, cfg);
        return MPP_ERR_NULL_PTR;
    }

    DataGroupImpl *p = mpp_malloc(DataGroupImpl, 1);

    mpp_assert(p);
    MPP_RET ret = rc_data_group_init(p, cfg->base_cnt, cfg->extra_cnt);

    return ret;
}

MPP_RET rc_data_deinit(DataGroup grp)
{
    if (NULL == grp) {
        mpp_err_f("invalid data group %p \n", grp);
        return MPP_ERR_NULL_PTR;
    }

    DataGroupImpl *p = (DataGroupImpl *)grp;
    rc_data_group_deinit(p);
    MPP_FREE(grp);

    return MPP_OK;
}

MPP_RET rc_data_reset(DataGroup grp)
{
    if (NULL == grp) {
        mpp_err_f("invalid data group %p \n", grp);
        return MPP_ERR_NULL_PTR;
    }

    DataGroupImpl *p = (DataGroupImpl *)grp;

    return rc_data_group_reset(p);
}

RcData rc_data_get(DataGroup grp, RK_S32 seq_id)
{
    if (NULL == grp) {
        mpp_err_f("invalid data group %p\n", grp);
        return NULL;
    }

    DataGroupImpl *p = (DataGroupImpl *)grp;
    RcData data = (RcData)rc_data_group_get_node_by_seq_id(p, seq_id);

    return data;
}

#define check_is_rc_data(data) _check_is_rc_data(data, __FUNCTION__)

MPP_RET _check_is_rc_data(void *data, const char *caller)
{
    if (data && ((RcDataNode *)data)->head.node == data)
        return MPP_OK;

    mpp_err("%s node %p failed on check\n", caller, data);
    mpp_abort();
    return MPP_NOK;
}

const RK_S32 *rc_data_get_seq_id(RcData data)
{
    if (!check_is_rc_data(data))
        return NULL;

    RcDataNode *node = (RcDataNode *)data;

    return (const RK_S32 *)&node->head.seq_id;
}

const RK_S32 *rc_data_get_qp_sum(RcData data)
{
    if (!check_is_rc_data(data))
        return NULL;

    RcDataNode *node = (RcDataNode *)data;

    return (const RK_S32 *)&node->base.qp_sum;
}

const RK_S32 *rc_data_get_strm_size(RcData data)
{
    if (!check_is_rc_data(data))
        return NULL;

    RcDataNode *node = (RcDataNode *)data;

    return (const RK_S32 *)&node->base.stream_size;
}

const EncFrmStatus *rc_data_get_frm_status(RcData data)
{
    if (!check_is_rc_data(data))
        return NULL;

    RcDataNode *node = (RcDataNode *)data;

    return (const EncFrmStatus *)&node->head.frm_status;
}

const RcHalSet *rc_data_get_hal_set(RcData data)
{
    if (!check_is_rc_data(data))
        return NULL;

    RcDataNode *node = (RcDataNode *)data;
    if (NULL == node->extra)
        return NULL;

    return (const RcHalSet *)&node->extra->set;
}

const RcHalRet *rc_data_get_hal_ret(RcData data)
{
    if (!check_is_rc_data(data))
        return NULL;

    RcDataNode *node = (RcDataNode *)data;
    if (NULL == node->extra)
        return NULL;

    return (const RcHalRet *)&node->extra->ret;
}
