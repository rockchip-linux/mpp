/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPI_IMPL_H
#define MPI_IMPL_H

#include "rk_mpi.h"

#include "mpp_list.h"
#include "mpp.h"

#define MPI_DBG_FUNCTION            (0x00000001)

#define mpi_dbg(flag, fmt, ...)     mpp_dbg(mpi_debug, flag, fmt, ## __VA_ARGS__)
#define mpi_dbg_f(flag, fmt, ...)   mpp_dbg_f(mpi_debug, flag, fmt, ## __VA_ARGS__)

#define mpi_dbg_func(fmt, ...)      mpi_dbg_f(MPI_DBG_FUNCTION, fmt, ## __VA_ARGS__)

typedef struct MpiImpl_t MpiImpl;

struct MpiImpl_t {
    MpiImpl             *check;
    struct list_head    list;

    MppCtxType          type;
    MppCodingType       coding;

    RK_U32              ctx_id;
    MppApi              *api;
    Mpp                 *ctx;
};

extern RK_U32 mpi_debug;

#endif /* MPI_IMPL_H */
