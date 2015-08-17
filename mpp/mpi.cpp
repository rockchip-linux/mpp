/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define MODULE_TAG "mpi"

#include <string.h>

#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpi_impl.h"


MPP_RET mpp_init(MppCtx *ctx, MppApi **mpi)
{
    MpiImpl *p;
    MppApi  *api;
    MPI_FUNCTION_ENTER();

    if (NULL == ctx || NULL == mpi) {
        mpp_err("mpp_init input ctx %p mpi %p found null pointer\n",
                ctx, mpi);
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_malloc(MpiImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_init failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    api = mpp_malloc(MppApi, 1);
    if (NULL == api) {
        mpp_err("mpp_init failed to allocate mpi\n");
        mpp_free(p);
        return MPP_ERR_MALLOC;
    }

    memset(p,   0, sizeof(*p));
    memset(api, 0, sizeof(*api));
    p->api      = api;
    p->check    = p;
    *ctx = p;

    MPI_FUNCTION_LEAVE_OK();
    return MPP_OK;
}

MPP_RET mpp_deinit(MppCtx* ctx)
{
    MpiImpl *p;
    MPI_FUNCTION_ENTER();

    if (NULL == ctx) {
        mpp_err("mpp_deinit input ctx %p is null pointer\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    p = (MpiImpl*)*ctx;

    if (p->check != p) {
        mpp_err("mpp_deinit input invalid MppCtx\n");
        return MPP_ERR_UNKNOW;
    }

    if (p->api)
        mpp_free(p->api);
    if (p)
        mpp_free(p->api);

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

