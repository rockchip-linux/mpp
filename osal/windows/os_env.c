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

#if defined(_WIN32)
#include <stdio.h>
#include <stdlib.h>
#include "os_env.h"

#define ENV_BUF_SIZE_WIN    1024

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
    char buf[ENV_BUF_SIZE_WIN];
    _snprintf(buf, sizeof(buf), "%s=%lu", name, value);
    return _putenv(buf);
}

RK_S32 os_set_env_str(const char *name, char *value)
{
    char buf[ENV_BUF_SIZE_WIN];
    _snprintf(buf, sizeof(buf), "%s=%s", name, value);
    return _putenv(buf);
}

#endif
