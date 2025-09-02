/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_ENV_H__
#define __MPP_ENV_H__

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpp_env_get_u32(const char *name, RK_U32 *value, RK_U32 default_value);
RK_S32 mpp_env_get_str(const char *name, const char **value, const char *default_value);

RK_S32 mpp_env_set_u32(const char *name, RK_U32 value);
RK_S32 mpp_env_set_str(const char *name, char *value);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENV_H__*/

