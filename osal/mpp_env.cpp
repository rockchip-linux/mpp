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

#include "mpp_env.h"
#include "os_env.h"

// TODO: add previous value compare to save call times

RK_S32 mpp_env_get_u32(const char *name, RK_U32 *value, RK_U32 default_value)
{
    return os_get_env_u32(name, value, default_value);
}

RK_S32 mpp_env_get_str(const char *name, const char **value, const char *default_value)
{
    return os_get_env_str(name, value, default_value);
}

RK_S32 mpp_env_set_u32(const char *name, RK_U32 value)
{
    return os_set_env_u32(name, value);
}

RK_S32 mpp_env_set_str(const char *name, char *value)
{
    return os_set_env_str(name, value);
}


