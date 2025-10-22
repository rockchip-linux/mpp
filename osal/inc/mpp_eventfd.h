/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_EVENTFD_H
#define MPP_EVENTFD_H

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpp_eventfd_get(RK_U32 init);
RK_S32 mpp_eventfd_put(RK_S32 fd);

RK_S32 mpp_eventfd_read(RK_S32 fd, RK_U64 *val, RK_S64 timeout);
RK_S32 mpp_eventfd_write(RK_S32 fd, RK_U64 val);

#ifdef __cplusplus
}
#endif

#endif /* MPP_EVENTFD_H */

