/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __OS_ENV_H__
#define __OS_ENV_H__

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 os_get_env_u32(const char *name, RK_U32 *value, RK_U32 default_value);
RK_S32 os_get_env_str(const char *name, const char **value, const char *default_value);

RK_S32 os_set_env_u32(const char *name, RK_U32 value);
RK_S32 os_set_env_str(const char *name, char *value);

#ifdef __cplusplus
}
#endif

#endif /*__OS_ENV_H__*/

