/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef __MPI_IMPL_H__
#define __MPI_IMPL_H__

#include "mpp.h"

#define MPI_DBG_FUNCTION            (0x00000001)

#define mpi_dbg(flag, fmt, ...)     _mpp_dbg(mpi_debug, flag, fmt, ## __VA_ARGS__)
#define mpi_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpi_debug, flag, fmt, ## __VA_ARGS__)

#define mpi_dbg_func(fmt, ...)      mpi_dbg_f(MPI_DBG_FUNCTION, fmt, ## __VA_ARGS__)

typedef struct MpiImpl_t MpiImpl;

struct MpiImpl_t {
    MpiImpl         *check;
    MppCtxType      type;
    MppCodingType   coding;

    MppApi          *api;
    Mpp             *ctx;
};

extern RK_U32 mpi_debug;

#endif /*__MPI_IMPL_H__*/
