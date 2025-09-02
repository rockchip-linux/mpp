/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#if defined(linux) && !defined(__ANDROID__)
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "os_env.h"

#define ENV_BUF_SIZE_LINUX  1024

RK_S32 os_get_env_u32(const char *name, RK_U32 *value, RK_U32 default_value)
{
    char *ptr = getenv(name);
    if (NULL == ptr) {
        *value = default_value;
    } else {
        char *endptr;
        int base = (ptr[0] == '0' && ptr[1] == 'x') ? (16) : (10);
        errno = 0;
        *value = strtoul(ptr, &endptr, base);
        if (errno || (ptr == endptr)) {
            errno = 0;
            *value = default_value;
        }
    }
    return 0;
}

RK_S32 os_get_env_str(const char *name, const char **value, const char *default_value)
{
    *value = getenv(name);
    if (NULL == *value) {
        *value = default_value;
    }
    return 0;
}

RK_S32 os_set_env_u32(const char *name, RK_U32 value)
{
    char buf[ENV_BUF_SIZE_LINUX];
    snprintf(buf, sizeof(buf) - 1, "%u", value);
    return setenv(name, buf, 1);
}

RK_S32 os_set_env_str(const char *name, char *value)
{
    return setenv(name, value, 1);
}

#endif
