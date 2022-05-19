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

#define MODULE_TAG "mpp_compat"

#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_compat.h"
#include "mpp_compat_impl.h"

static MppCompat compats[] = {
    {
        MPP_COMPAT_INC_FBC_BUF_SIZE,
        MPP_COMPAT_BOOL,
        1,
        0,
        "increase decoder fbc buffer size",
        &compats[1],
    },
    {
        MPP_COMPAT_ENC_ASYNC_INPUT,
        MPP_COMPAT_BOOL,
        1,
        0,
        "support encoder async input mode",
        NULL,
    },
};

RK_S32 *compat_ext_fbc_buf_size = &compats[MPP_COMPAT_INC_FBC_BUF_SIZE].value_usr;
RK_S32 *compat_ext_async_input  = &compats[MPP_COMPAT_ENC_ASYNC_INPUT].value_usr;

MppCompat *mpp_compat_query(void)
{
    return compats;
}

MppCompat *mpp_compat_query_by_id(MppCompatId id)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(compats); i++) {
        if (compats[i].feature_id == id)
            return &compats[i];
    }

    return NULL;
}

MPP_RET mpp_compat_update(MppCompat *compat, RK_S32 value)
{
    if (NULL == compat)
        return MPP_NOK;

    if (compat->feature_id >= MPP_COMPAT_BUTT)
        return MPP_NOK;

    if (compat != &compats[compat->feature_id])
        return MPP_NOK;

    if (compat->feature_type == MPP_COMPAT_BOOL)
        if (value != 0 && value != 1)
            return MPP_NOK;

    compat->value_usr = value;
    return MPP_OK;
}

void mpp_compat_show(void)
{
    const MppCompat *compat = compats;

    mpp_log("id| name -- mpp compat info\n");

    while (compat) {
        mpp_log("%d | %s\n", compat->feature_id, compat->name);

        compat = compat->next;
    }
}
