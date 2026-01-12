/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_SYS_H
#define MPP_SYS_H

#include "mpi_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_sys_attach(MpiImpl *ctx);
rk_s32 mpp_sys_detach(MpiImpl *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MPP_SYS_H */
