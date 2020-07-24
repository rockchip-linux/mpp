/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#include "iep_common.h"

#include "mpp_common.h"
#include "mpp_log.h"

#include "iep_api.h"
#include "iep2_api.h"

struct dev_compatible dev_comp[] = {
    {
        .compatible = "/dev/iep",
        .get = rockchip_iep_api_alloc_ctx,
        .put = rockchip_iep_api_release_ctx,
        .ver = 1,
    },
    {
        .compatible = "/dev/mpp_service",
        .get = rockchip_iep2_api_alloc_ctx,
        .put = rockchip_iep2_api_release_ctx,
        .ver = 2,
    },
};

iep_com_ctx* get_iep_ctx()
{
    uint32_t i;

    for (i = 0; i < MPP_ARRAY_ELEMS(dev_comp); ++i) {
        if (!access(dev_comp[i].compatible, F_OK)) {
            iep_com_ctx *ctx = dev_comp[i].get();

            ctx->ver = dev_comp[i].ver;
            mpp_log("device %s select in vproc\n", dev_comp[i].compatible);

            ctx->ops->release = dev_comp[i].put;

            return ctx;
        }
    }

    return NULL;
}

void put_iep_ctx(iep_com_ctx *ictx)
{
    if (ictx->ops->release)
        ictx->ops->release(ictx);
}

