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

#if defined(__ANDROID__)
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "os_env.h"
#include <sys/system_properties.h>

/*
 * NOTE: __system_property_set only available after android-21
 * So the library should compiled on latest ndk
 */

RK_S32 os_get_env_u32(const char *name, RK_U32 *value, RK_U32 default_value)
{
    char prop[PROP_VALUE_MAX + 1];
    int len = __system_property_get(name, prop);
    if (len > 0) {
        char *endptr;
        int base = (prop[0] == '0' && prop[1] == 'x') ? (16) : (10);
        errno = 0;
        *value = strtoul(prop, &endptr, base);
        if (errno || (prop == endptr)) {
            errno = 0;
            *value = default_value;
        }
    } else {
        *value = default_value;
    }
    return 0;
}

RK_S32 os_get_env_str(const char *name, const char **value, const char *default_value)
{
    // use unsigned char to avoid warnning
    static unsigned char env_str[2][PROP_VALUE_MAX + 1];
    static RK_U32 env_idx = 0;
    char *prop = (char *)env_str[env_idx];
    int len = __system_property_get(name, prop);
    if (len > 0) {
        *value  = prop;
        env_idx = !env_idx;
    } else {
        *value = default_value;
    }
    return 0;
}

RK_S32 os_set_env_u32(const char *name, RK_U32 value)
{
    char buf[PROP_VALUE_MAX + 1 + 2];
    snprintf(buf, sizeof(buf), "0x%x", value);
    int len = __system_property_set(name, buf);
    return (len) ? (0) : (-1);
}

RK_S32 os_set_env_str(const char *name, char *value)
{
    int len = __system_property_set(name, value);
    return (len) ? (0) : (-1);
}

#endif
