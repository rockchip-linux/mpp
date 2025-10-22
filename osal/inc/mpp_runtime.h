/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_RUNTIME_H
#define MPP_RUNTIME_H

#include "mpp_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Runtime function detection is to support different binary on different
 * runtime environment. This is usefull on product environemnt.
 */
rk_u32 mpp_rt_allcator_is_valid(MppBufferType type);

#ifdef __cplusplus
}
#endif

#endif /* MPP_RUNTIME_H */

