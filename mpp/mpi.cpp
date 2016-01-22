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
#include "mpp.h"
#include "mpp_info.h"

#define check_mpp_ctx(ctx)  _check_mpp_ctx(ctx, __FUNCTION__)

static MPP_RET _check_mpp_ctx(MpiImpl *p, const char *caller)
{
    if (NULL == p || p->check != p || NULL == p->ctx) {
        _mpp_err(MODULE_TAG, "found invalid context %p\n", caller, p);
        return MPP_ERR_UNKNOW;
    }
    return MPP_OK;
}

static MPP_RET mpi_config(MppCtx ctx, MppEncConfig cfg)
{
    (void)cfg;

    MPI_FUNCTION_ENTER();
    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_decode(MppCtx ctx, MppPacket packet, MppFrame *frame)
{
    (void)packet;
    (void)frame;

    MPI_FUNCTION_ENTER();
    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_encode(MppCtx ctx, MppFrame frame, MppPacket *packet)
{
    (void)packet;
    (void)frame;

    MPI_FUNCTION_ENTER();
    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

static MPP_RET mpi_decode_put_packet(MppCtx ctx, MppPacket packet)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (NULL == packet) {
        mpp_err_f("found NULL input packet\n", packet);
        return MPP_ERR_NULL_PTR;
    }

    ret = p->ctx->put_packet(packet);

    MPI_FUNCTION_LEAVE();
    return ret;
}

static MPP_RET mpi_decode_get_frame(MppCtx ctx, MppFrame *frame)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (NULL == frame) {
        mpp_err_f("found NULL input frame\n", frame);
        return MPP_ERR_NULL_PTR;
    }

    ret = p->ctx->get_frame(frame);

    MPI_FUNCTION_LEAVE();
    return ret;
}

static MPP_RET mpi_encode_put_frame(MppCtx ctx, MppFrame frame)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (NULL == frame) {
        mpp_err_f("found NULL input frame\n", frame);
        return MPP_ERR_NULL_PTR;
    }

    ret = p->ctx->put_frame(frame);

    MPI_FUNCTION_LEAVE();
    return ret;
}

static MPP_RET mpi_encode_get_packet(MppCtx ctx, MppPacket *packet)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (NULL == packet) {
        mpp_err_f("found NULL input packet\n", packet);
        return MPP_ERR_NULL_PTR;
    }

    ret = p->ctx->get_packet(packet);

    MPI_FUNCTION_LEAVE();
    return ret;
}

static MPP_RET mpi_reset(MppCtx ctx)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (NULL == p || p->check != p || NULL == p->ctx) {
        mpp_err_f("found invalid context input ctx %p\n", ctx);
        return MPP_ERR_UNKNOW;
    }

    ret = p->ctx->reset();

    MPI_FUNCTION_LEAVE();
    return ret;
}

static MPP_RET mpi_control(MppCtx ctx, MpiCmd cmd, MppParam param)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl *)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (NULL == p || p->check != p || NULL == p->ctx) {
        mpp_err_f("found invalid context input ctx %p\n", ctx);
        return MPP_ERR_UNKNOW;
    }

    ret = p->ctx->control(cmd, param);

    MPI_FUNCTION_LEAVE();
    return ret;
}

static MppApi mpp_api = {
    sizeof(mpp_api),
    0,
    mpi_decode,
    mpi_encode,
    mpi_decode_put_packet,
    mpi_decode_get_frame,
    mpi_encode_put_frame,
    mpi_encode_get_packet,
    mpi_reset,
    mpi_control,
    mpi_config,
    {0},
};

MPP_RET mpp_create(MppCtx *ctx, MppApi **mpi)
{
    MPI_FUNCTION_ENTER();

    if (NULL == ctx || NULL == mpi) {
        mpp_err_f("invalid input ctx %p mpi %p\n", ctx, mpi);
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;
    *mpi = NULL;

    MpiImpl *p = mpp_malloc(MpiImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }

    memset(p, 0, sizeof(*p));
    p->ctx = new Mpp();
    if (NULL == p->ctx) {
        mpp_free(p);
        mpp_err_f("failed to new Mpp\n");
        return MPP_ERR_MALLOC;
    }

    mpp_api.version = mpp_info_get_revision();
    p->api      = &mpp_api;
    p->check    = p;
    *ctx = p;
    *mpi = p->api;

    mpp_log("mpp version: %d\n", mpp_api.version);

    MPI_FUNCTION_LEAVE_OK();
    return MPP_OK;
}

MPP_RET mpp_init(MppCtx ctx, MppCtxType type, MppCodingType coding)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl*)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (type >= MPP_CTX_BUTT ||
        coding >= MPP_VIDEO_CodingMax) {
        mpp_err_f("invalid input type %d coding %d\n", type, coding);
        return MPP_ERR_UNKNOW;
    }

    p->ctx->init(type, coding);
    p->type     = type;
    p->coding   = coding;

    get_mpi_debug();
    MPI_FUNCTION_LEAVE_OK();
    return MPP_OK;
}

MPP_RET mpp_destroy(MppCtx ctx)
{
    MPI_FUNCTION_ENTER();

    MpiImpl *p = (MpiImpl*)ctx;
    MPP_RET ret = check_mpp_ctx(p);
    if (ret)
        return ret;

    if (p->ctx)
        delete p->ctx;

    mpp_free(p);

    MPI_FUNCTION_LEAVE();
    return MPP_OK;
}

