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

static MPP_RET mpi_init(MppCtx ctx, MppPacket packet)
{
    (void)ctx;
    (void)packet;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_decode(MppCtx ctx, MppPacket packet, MppFrame *frame)
{
    (void)ctx;
    (void)packet;
    (void)frame;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_encode(MppCtx ctx, MppFrame frame, MppPacket *packet)
{
    (void)ctx;
    (void)packet;
    (void)frame;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_decode_put_packet(MppCtx ctx, MppPacket packet)
{
    (void)ctx;
    (void)packet;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_decode_get_frame(MppCtx ctx, MppFrame *frame)
{
    (void)ctx;
    (void)frame;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_encode_put_frame(MppCtx ctx, MppFrame frame)
{
    (void)ctx;
    (void)frame;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_encode_get_packet(MppCtx ctx, MppPacket *packet)
{
    (void)ctx;
    (void)packet;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_flush(MppCtx ctx)
{
    (void)ctx;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_control(MppCtx ctx, MPI_CMD cmd, MppParam param)
{
    (void)ctx;
    (void)cmd;
    (void)param;
    MPI_FUNCTION_ENTER();

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MppApi mpp_api = {
    sizeof(mpp_api),
    1,
    mpi_init,
    mpi_decode,
    mpi_encode,
    mpi_decode_put_packet,
    mpi_decode_get_frame,
    mpi_encode_put_frame,
    mpi_encode_get_packet,
    mpi_flush,
    mpi_control,
    {0},
};

MPP_RET mpp_init(MppCtx *ctx, MppApi **mpi)
{
    MpiImpl *p;
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

    memset(p, 0, sizeof(*p));
    p->api   = &mpp_api;
    p->check = p;
    *ctx = p;
    *mpi = p->api;

    get_mpi_debug();

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

    if (p)
        mpp_free(p);
    *ctx = NULL;

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

