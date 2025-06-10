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

#define MODULE_TAG "rc_base"

#include <math.h>
#include <memory.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "rc_base.h"

#define MPP_RC_DBG_FUNCTION          (0x00000001)
#define MPP_RC_DBG_BPS               (0x00000010)
#define MPP_RC_DBG_RC                (0x00000020)
#define MPP_RC_DBG_CFG               (0x00000100)
#define MPP_RC_DBG_RECORD            (0x00001000)
#define MPP_RC_DBG_VBV               (0x00002000)

#define mpp_rc_dbg(flag, fmt, ...)   _mpp_dbg(mpp_rc_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_f(flag, fmt, ...) _mpp_dbg_f(mpp_rc_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_rc_dbg_func(fmt, ...)    mpp_rc_dbg_f(MPP_RC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_bps(fmt, ...)     mpp_rc_dbg(MPP_RC_DBG_BPS, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_rc(fmt, ...)      mpp_rc_dbg(MPP_RC_DBG_RC, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_cfg(fmt, ...)     mpp_rc_dbg(MPP_RC_DBG_CFG, fmt, ## __VA_ARGS__)
#define mpp_rc_dbg_vbv(fmt, ...)     mpp_rc_dbg(MPP_RC_DBG_VBV, fmt, ## __VA_ARGS__)

#define SIGN(a)         ((a) < (0) ? (-1) : (1))
#define DIV(a, b)       (((a) + (SIGN(a) * (b)) / 2) / (b))

MPP_RET mpp_data_init_v2(MppDataV2 **data, RK_S32 size, RK_S32 value)
{
    if (NULL == data || size <= 0) {
        mpp_err_f("invalid data %p size %d\n", data, size);
        return MPP_ERR_NULL_PTR;
    }

    *data = NULL;
    MppDataV2 *p = mpp_malloc_size(MppDataV2, sizeof(MppDataV2) + sizeof(RK_S32) * size);
    if (NULL == p) {
        mpp_err_f("malloc size %d failed\n", size);
        return MPP_ERR_MALLOC;
    }
    p->size = size;
    p->pos_r = 0;
    p->pos_pw = 0;
    p->pos_w = 0;
    p->pos_ahead = 0;
    p->sum = 0;
    *data = p;

    mpp_data_reset_v2(p, value);
    return MPP_OK;
}

void mpp_data_deinit_v2(MppDataV2 *p)
{
    MPP_FREE(p);
}

void mpp_data_reset_v2(MppDataV2 *p, RK_S32 val)
{
    RK_S32 i;
    RK_S32 *data = p->val;

    p->pos_pw = 0;
    p->pos_w = 0;
    p->pos_r = p->size;
    p->sum = val * p->size;

    for (i = 0; i < p->size; i++)
        *data++ = val;
}

void mpp_data_preset_v2(MppDataV2 *p, RK_S32 val)
{
    mpp_assert(p);
    if (p->pos_r == p->size) {
        p->pos_r--;
        p->sum -= p->val[p->pos_pw];
    }
    mpp_assert(p->pos_r < p->size);
    p->val[p->pos_pw] = val;
    p->sum += p->val[p->pos_pw];
    p->pos_pw++;
    p->pos_r++;
    if (p->pos_pw >= p->size) {
        p->pos_pw = 0;
    }
    p->pos_ahead++;
}

void mpp_data_update_v2(MppDataV2 *p, RK_S32 val)
{
    if (p->pos_ahead) {
        p->sum += val - p->val[p->pos_w];
        p->val[p->pos_w] = val;
        p->pos_w++;
        if (p->pos_w >= p->size)
            p->pos_w = 0;

        p->pos_ahead--;

        return;
    }

    mpp_assert(p);
    if (p->pos_r == p->size) {
        p->pos_r--;
        p->sum -= p->val[p->pos_w];
    }
    mpp_assert(p->pos_r < p->size);
    p->val[p->pos_w] = val;
    p->sum += p->val[p->pos_w];
    p->pos_w++;
    p->pos_r++;
    if (p->pos_w >= p->size)
        p->pos_w = 0;
}

RK_S32 mpp_data_get_pre_val_v2(MppDataV2 *p, RK_S32 idx)
{
    if (idx < 0) {
        idx = p->size + idx;
    }
    mpp_assert(p->pos_w < p->size);
    mpp_assert(idx < p->size);
    RK_S32 pos = 0;

    pos = p->pos_w - 1;
    if (pos - idx < 0) {
        mpp_assert(p->pos_r == p->size);
        RK_S32 pos1 = idx - pos;
        pos = p->size - pos1;
    } else {
        pos = pos - idx;
    }
    mpp_assert(pos < p->size);
    return p->val[pos];
}

RK_S32 mpp_data_sum_v2(MppDataV2 *p)
{
    return (RK_S32)p->sum;
}
RK_S32 mpp_data_mean_v2(MppDataV2 *p)
{
    RK_S32 mean = (RK_S32)p->sum / p->size;
    return mean;
}

RK_S32 mpp_data_sum_with_ratio_v2(MppDataV2 *p, RK_S32 len, RK_S32 num, RK_S32 denom)
{
    mpp_assert(p);

    RK_S32 i;
    RK_S64 sum = 0;
    RK_S32 *data = p->val;

    mpp_assert(len <= p->size);

    if (num == denom) {
        for (i = 0; i < len; i++)
            sum += *data++;
    } else {
        // NOTE: use 64bit to avoid 0 in 32bit
        RK_S64 acc_num = 1;
        RK_S64 acc_denom = 1;

        for (i = 0; i < len; i++) {
            sum += p->val[i] * acc_num / acc_denom;
            acc_num *= num;
            acc_denom *= denom;
        }
    }

    return DIV(sum, len);
}
